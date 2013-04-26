// Copyright (c) 2013 Tobias Hoffmann
//               2013 Paul Harris
//
// License: http://opensource.org/licenses/MIT

#include <assert.h>
#include <string.h>
#include "csvbase.h"

#include <boost/utility.hpp>
#include <boost/variant.hpp>

//#define DEBUG

#ifdef DEBUG
  #include <stdio.h>
  #include <typeinfo>
#endif

namespace cppcsv {
namespace csvFSM {

// States - must all be emptry structs
struct Start {};
struct ReadSkipPre {};
struct ReadQuoted {};
struct ReadQuotedCheckEscape {};
struct ReadQuotedSkipPost {};
struct ReadDosCR {};
struct ReadUnquoted {};
struct ReadUnquotedWhitespace {};
struct ReadComment {};
struct ReadError {};

typedef boost::variant<Start,
                       ReadDosCR,
                       ReadSkipPre,
                       ReadQuoted,
                       ReadQuotedCheckEscape,
                       ReadQuotedSkipPost,
                       ReadUnquoted,
                       ReadUnquotedWhitespace,
                       ReadComment,
                       ReadError> States;

// Events
struct Echar {};
struct Ewhitespace {};
struct Eqchar {};
struct Esep {};
struct Enewline {};
struct Edos_cr {};   // DOS carriage return
struct Ecomment {};


struct Trans {
  Trans(csv_builder &out, bool trim_whitespace, bool collapse_separators ) :
     value(0),
     active_qchar(0),
     cell_index(0),
     error_message(NULL),
     out(out),
     trim_whitespace(trim_whitespace),
     collapse_separators(collapse_separators)
   {}

  // this is set before the state change is called,
  // that way the Events do not need to carry their state with them.
  unsigned char value;

  // active quote character, required in the situation where multiple quote chars are possible
  // if =0 then there is no active_qchar
  unsigned char active_qchar;

private:
  // keeps a count of the number of cells emitted in this row
  // this is used to help with the comments support.
  // IF we only allow comments at the start of the line, then
  // the character-feeder will only emit a Ecomment event when
  // the cell_index == 0
  unsigned int cell_index;

public:
  bool row_empty() const {
     return cell_index == 0 && cell.empty() && whitespace_state.empty();
  }

  const char* error_message;

    // note: states and events are just empty structs, so pass by copy should be faster
#define TTS(State, Event, NextState, code) States on(State, Event) { code; return NextState(); }
  //  State          Event        Next_State    Transition_Action
  TTS(Start,         Eqchar,      ReadQuoted,   { active_qchar = value; begin_row(); });
  TTS(Start,         Esep,        ReadSkipPre,  { begin_row(); next_cell(false); });
  TTS(Start,         Enewline,    Start,        { begin_row(); end_row(); });
  TTS(Start,         Edos_cr,     ReadDosCR,   {});
  TTS(Start,         Ewhitespace, ReadSkipPre,  { if (!trim_whitespace) { remember_whitespace(); } begin_row(); });   // we MAY want to remember this whitespace
  TTS(Start,         Echar,       ReadUnquoted, { begin_row(); add(); });
  TTS(Start,         Ecomment,    ReadComment,  { begin_row(); end_row(); });  // comment at the start of the line --> everything discarded but blank row still generated

  TTS(ReadDosCR,    Eqchar,    ReadError, { error_message = "quote after CR"; });
  TTS(ReadDosCR,    Esep,      ReadError, { error_message = "sep after CR"; });
  TTS(ReadDosCR,    Enewline,  Start,     { begin_row(); end_row(); });
  TTS(ReadDosCR,    Edos_cr,   ReadError, { error_message = "CR after CR"; });
  TTS(ReadDosCR,    Ewhitespace, ReadError, { error_message = "whitespace after CR"; });
  TTS(ReadDosCR,    Echar,     ReadError, { error_message = "char after CR"; });
  TTS(ReadDosCR,    Ecomment,  ReadError, { error_message = "comment after CR"; });

  TTS(ReadSkipPre,   Eqchar,      ReadQuoted,   { active_qchar = value; drop_whitespace(); });   // we always want to forget whitespace before the quotes
  TTS(ReadSkipPre,   Esep,        ReadSkipPre,  { if (!collapse_separators) { next_cell(false); } });
  TTS(ReadSkipPre,   Enewline,    Start,        { next_cell(false); end_row(); });
  TTS(ReadSkipPre,   Edos_cr,     ReadDosCR,    { next_cell(false); end_row(); });  // same as newline, except we expect to see newline next
  TTS(ReadSkipPre,   Ewhitespace, ReadSkipPre,  { if (!trim_whitespace) { remember_whitespace(); } });  // we MAY want to remember this whitespace
  TTS(ReadSkipPre,   Echar,       ReadUnquoted, { add_whitespace(); add(); });   // we add whitespace IF any was recorded
  TTS(ReadSkipPre,   Ecomment,    ReadComment,  { add_whitespace(); if (!cell.empty()) { next_cell(true); } end_row(); } );   // IF there was anything, then emit a cell else completely ignore the current cell (ie do not emit a null)

  TTS(ReadQuoted,    Eqchar,      ReadQuotedCheckEscape, {});
  TTS(ReadQuoted,    Esep,        ReadQuoted,            { add(); });
  TTS(ReadQuoted,    Enewline,    ReadQuoted,            { add(); });
  TTS(ReadQuoted,    Edos_cr,     ReadQuoted,            { add(); });
  TTS(ReadQuoted,    Ewhitespace, ReadQuoted,            { add(); });
  TTS(ReadQuoted,    Echar,       ReadQuoted,            { add(); });
  TTS(ReadQuoted,    Ecomment,    ReadQuoted,            { add(); });

  // we are reading quoted text, we see a "... here we are looking to see if its followed by another "
  // if so, then output a quote, else its the end of the quoted section.
  TTS(ReadQuotedCheckEscape, Eqchar,      ReadQuoted,          { add(); });
  TTS(ReadQuotedCheckEscape, Esep,        ReadSkipPre,         { active_qchar = 0; next_cell(); });
  TTS(ReadQuotedCheckEscape, Enewline,    Start,               { active_qchar = 0; next_cell(); end_row(); });
  TTS(ReadQuotedCheckEscape, Edos_cr,     ReadDosCR,           { active_qchar = 0; next_cell(); end_row(); });
  TTS(ReadQuotedCheckEscape, Ewhitespace, ReadQuotedSkipPost,  { active_qchar = 0; });
  TTS(ReadQuotedCheckEscape, Echar,       ReadError,           { error_message = "char after possible endquote"; });
  TTS(ReadQuotedCheckEscape, Ecomment,    ReadComment,         { active_qchar = 0; next_cell(); end_row(); });

  // we have finished a quoted cell, we are just looking for the next separator or end point so we can emit the cell
  TTS(ReadQuotedSkipPost, Eqchar,      ReadError,           { error_message = "quote after endquote"; });
  TTS(ReadQuotedSkipPost, Esep,        ReadSkipPre,         { next_cell(); });
  TTS(ReadQuotedSkipPost, Enewline,    Start,               { next_cell(); end_row(); });
  TTS(ReadQuotedSkipPost, Edos_cr,     ReadDosCR,           { next_cell(); end_row(); });
  TTS(ReadQuotedSkipPost, Ewhitespace, ReadQuotedSkipPost,  {});
  TTS(ReadQuotedSkipPost, Echar,       ReadError,           { error_message = "char after endquote"; });
  TTS(ReadQuotedSkipPost, Ecomment,    ReadComment,         { next_cell(); end_row(); });

  // we have seen some text, and we are reading it in
  TTS(ReadUnquoted, Eqchar,      ReadError,              { error_message = "unexpected quote in unquoted string"; });
  TTS(ReadUnquoted, Esep,        ReadSkipPre,            { next_cell(); });
  TTS(ReadUnquoted, Enewline,    Start,                  { next_cell(); end_row(); });
  TTS(ReadUnquoted, Edos_cr,     ReadDosCR,              { next_cell(); end_row(); });
  TTS(ReadUnquoted, Ewhitespace, ReadUnquotedWhitespace, { remember_whitespace(); });  // must remember whitespace in case its in the middle of the cell
  TTS(ReadUnquoted, Echar,       ReadUnquoted,           { add_whitespace(); add(); });
  TTS(ReadUnquoted, Ecomment,    ReadComment,            { next_cell(); end_row(); });

  // we have been reading some text, and we are working through some whitespace,
  // which could be in the middle of the text or at the end of the cell.
  TTS(ReadUnquotedWhitespace, Eqchar,      ReadError,                { error_message = "unexpected quote after unquoted string"; });
  TTS(ReadUnquotedWhitespace, Esep,        ReadSkipPre,              { add_unquoted_post_whitespace(); next_cell(); });
  TTS(ReadUnquotedWhitespace, Enewline,    Start,                    { add_unquoted_post_whitespace(); next_cell(); end_row(); });
  TTS(ReadUnquotedWhitespace, Edos_cr,     ReadDosCR,                { add_unquoted_post_whitespace(); next_cell(); end_row(); });
  TTS(ReadUnquotedWhitespace, Ewhitespace, ReadUnquotedWhitespace,   { remember_whitespace(); });
  TTS(ReadUnquotedWhitespace, Echar,       ReadUnquoted,             { add_whitespace(); add(); });
  TTS(ReadUnquotedWhitespace, Ecomment,    ReadComment,              { add_unquoted_post_whitespace(); next_cell(); end_row(); });

  TTS(ReadError, Eqchar,      ReadError,  { assert(error_message); });
  TTS(ReadError, Esep,        ReadError,  { assert(error_message); });
  TTS(ReadError, Enewline,    ReadError,  { assert(error_message); });
  TTS(ReadError, Edos_cr,     ReadError,  { assert(error_message); });
  TTS(ReadError, Ewhitespace, ReadError,  { assert(error_message); });
  TTS(ReadError, Echar,       ReadError,  { assert(error_message); });
  TTS(ReadError, Ecomment,    ReadError,  { assert(error_message); });

  TTS(ReadComment, Eqchar,       ReadComment,   {});
  TTS(ReadComment, Esep,         ReadComment,   {});
  TTS(ReadComment, Enewline,     Start,         {});  // end of line --> end of comment
  TTS(ReadComment, Edos_cr,      ReadComment,   {});
  TTS(ReadComment, Ewhitespace,  ReadComment,   {});
  TTS(ReadComment, Echar,        ReadComment,   {});
  TTS(ReadComment, Ecomment,     ReadComment,   {});

#undef TTS

private:
  // adds current character to cell buffer
  void add()
  {
     cell.push_back(value);
  }

  // whitespace is remembered until we know if we need to add to the output or forget
  void remember_whitespace()
  {
     whitespace_state.push_back(value);
  }

  // adds remembered whitespace to cell buffer
  void add_whitespace()
  {
     cell.append(whitespace_state);
     drop_whitespace();
  }

  // only writes out whitespace IF we aren't trimming
  void add_unquoted_post_whitespace()
  {
     if (!trim_whitespace)
        cell.append(whitespace_state);
     drop_whitespace();
  }

  void drop_whitespace()
  {
     whitespace_state.clear(); 
  }


  void begin_row()
  {
     cell_index = 0;
     out.begin_row();
  }


  // emits cell to builder and clears buffer
  void next_cell(bool has_content = true)
  {
     ++cell_index;
    if (has_content) {
      out.cell(cell.c_str(),cell.size());
    } else {
      assert(cell.empty());
      out.cell(NULL,0);
    }
    cell.clear();
  }

  void end_row()
  {
     cell_index = 0;
     out.end_row();
  }

  csv_builder &out;
  std::string cell;
  std::string whitespace_state;
  bool trim_whitespace;
  bool collapse_separators;
};

  template <class Event>
  struct NextState : boost::static_visitor<States> {
    NextState(Trans &t) : t(t) {}

    // note: states are just empty structs, so pass by copy should be faster
    template <class State>
    States operator()(State s) const {
#ifdef DEBUG
  printf("%s %c (%d)\n",typeid(State).name(), t.value, (int)t.value);
#endif
      return t.on(s,Event());
    }

  private:
    Trans &t;
  };


} // namespace csvFSM


// has template so users can specify either string (multiples of)
// or char (single quote/separator)

// so only use either string or char as the template parameters!
template <class QuoteChars = char, class Separators = char, class CommentChars = char>
struct csvparser {

   // trim_whitespace: remove whitespace around unquoted cells
   // the standard says you should NOT trim, but many people would want to.
   // You can always quote the whitespace, and that will be kept

// default constructor, gives you a parser for standards-compliant CSV
csvparser(csv_builder &out)
 : comment(0),
   comments_must_be_at_start_of_line(true),
   errmsg(NULL),
   trans(out, false, false)
{
   add_char(qchar, '"');
   add_char(sep, ',');
   // no comment-chars
}

// constructor that was used before
csvparser(csv_builder &out, QuoteChars qchar, Separators sep, bool trim_whitespace = false, bool collapse_separators = false)
 : qchar(qchar), sep(sep), comment(0),
   comments_must_be_at_start_of_line(true),
   errmsg(NULL),
   trans(out, trim_whitespace, collapse_separators)
{}


// constructor with everything
csvparser(csv_builder &out, QuoteChars qchar, Separators sep, bool trim_whitespace, bool collapse_separators, CommentChars comment, bool comments_must_be_at_start_of_line)
 : qchar(qchar), sep(sep), comment(comment),
   comments_must_be_at_start_of_line(comments_must_be_at_start_of_line),
   errmsg(NULL),
   trans(out, trim_whitespace, collapse_separators)
{}

  
  // NOTE: returns true on error
bool operator()(const std::string &line) // not required to be linewise
{
  const char *buf=line.c_str();
  return (operator())(buf,line.size());
}

bool operator()(const char *&buf,int len)
{
  while (len > 0) {
     // note: current character is written directly to trans,
     // so that events become empty structs.
     trans.value = *buf;

     using namespace csvFSM;

         if (match_char('\r'))      process_event(Edos_cr());
    else if (match_char('\n'))      process_event(Enewline());
    else if (is_quote_char(qchar))  process_event(Eqchar());
    else if (match_char(sep))       process_event(Esep());

    else if ( (!comments_must_be_at_start_of_line || trans.row_empty()) && match_char(comment)) {
       // this one is more complex gate... only emit a comment event if comments can be anywhere or
       // the row is still empty
       process_event(Ecomment());
    }

    else if (match_char(' '))       process_event(Ewhitespace()); // check space
    else if (match_char('\t'))      process_event(Ewhitespace()); // check tab
    else                            process_event(Echar());

    if (trans.error_message) {
#ifdef DEBUG
       fprintf(stderr, "State index: %d\n", state.which());
       fprintf(stderr,"csv parse error: %s\n",error());
#endif
      errmsg = trans.error_message;
      return true;
    }
    buf++;
    len--;
  }
  return false;
}

  const char * error() const { return errmsg; }

private:
  QuoteChars qchar;  // could be char or string
  Separators sep;    // could be char or string
  CommentChars comment;   // could be char or string
  bool comments_must_be_at_start_of_line;
  const char *errmsg;

  csvFSM::States state;
  csvFSM::Trans trans;

  // support either single or multiple quote characters
  bool is_quote_char( char qchar ) const
  {
    return match_char(qchar);
  }

  bool is_quote_char( std::string const& qchar ) const
  {
    if (trans.active_qchar != 0)
       return match_char(trans.active_qchar);
    return match_char(qchar);
  }

  // support either a single char or a string of chars

  // note: ignores the null (0) character (ie if there is no comment char)
  bool match_char( char target ) const {
     return target != 0 && target == trans.value;
  }

  bool match_char( std::string const& target ) const {
    return target.find_first_of(trans.value) != std::string::npos;
  }


   // helpers for setting up the separator/quote/comment chars
   static void add_char( char & target, char src )        { target = src; }
   static void add_char( std::string & target, char src ) { target.push_back(src); }


   // finds the next state, performs transition activity, sets the new state
   template <class Event>
   void process_event( Event event ) {
     state = boost::apply_visitor( csvFSM::NextState<Event>(trans), state);
   }
};


}
