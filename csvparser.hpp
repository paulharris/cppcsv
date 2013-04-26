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
struct ReadError {};

typedef boost::variant<Start,
                       ReadDosCR,
                       ReadSkipPre,
                       ReadQuoted,
                       ReadQuotedCheckEscape,
                       ReadQuotedSkipPost,
                       ReadUnquoted,
                       ReadUnquotedWhitespace,
                       ReadError> States;

// Events
struct Echar {};
struct Ewhitespace {};
struct Eqchar {};
struct Esep {};
struct Enewline {};
struct Edos_cr {};   // DOS carriage return


struct Trans {
  Trans(csv_builder &out, bool trim_whitespace, bool collapse_separators) : value(0), active_qchar(0), error_message(NULL), out(out), trim_whitespace(trim_whitespace), collapse_separators(collapse_separators) {}

  // this is set before the state change is called,
  // that way the Events do not need to carry their state with them.
  unsigned char value;

  // active quote character, required in the situation where multiple quote chars are possible
  // if =0 then there is no active_qchar
  unsigned char active_qchar;

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

  TTS(ReadDosCR,    Eqchar,    ReadError, { error_message = "quote after CR"; });
  TTS(ReadDosCR,    Esep,      ReadError, { error_message = "sep after CR"; });
  TTS(ReadDosCR,    Enewline,  Start,     { begin_row(); end_row(); });
  TTS(ReadDosCR,    Edos_cr,   ReadError, { error_message = "CR after CR"; });
  TTS(ReadDosCR,    Ewhitespace, ReadError, { error_message = "whitespace after CR"; });
  TTS(ReadDosCR,    Echar,     ReadError, { error_message = "char after CR"; });

  TTS(ReadSkipPre,   Eqchar,      ReadQuoted,   { active_qchar = value; drop_whitespace(); });   // we always want to forget whitespace before the quotes
  TTS(ReadSkipPre,   Esep,        ReadSkipPre,  { if (!collapse_separators) { next_cell(false); } });
  TTS(ReadSkipPre,   Enewline,    Start,        { next_cell(false); end_row(); });
  TTS(ReadSkipPre,   Edos_cr,     ReadDosCR,    { next_cell(false); end_row(); });  // same as newline, except we expect to see newline next
  TTS(ReadSkipPre,   Ewhitespace, ReadSkipPre,  { if (!trim_whitespace) { remember_whitespace(); } });  // we MAY want to remember this whitespace
  TTS(ReadSkipPre,   Echar,       ReadUnquoted, { add_whitespace(); add(); });   // we add whitespace IF any was recorded

  TTS(ReadQuoted,    Eqchar,      ReadQuotedCheckEscape, {});
  TTS(ReadQuoted,    Esep,        ReadQuoted,            { add(); });
  TTS(ReadQuoted,    Enewline,    ReadQuoted,            { add(); });
  TTS(ReadQuoted,    Edos_cr,     ReadQuoted,            { add(); });
  TTS(ReadQuoted,    Ewhitespace, ReadQuoted,            { add(); });
  TTS(ReadQuoted,    Echar,       ReadQuoted,            { add(); });

  TTS(ReadQuotedCheckEscape, Eqchar,      ReadQuoted,          { add(); });
  TTS(ReadQuotedCheckEscape, Esep,        ReadSkipPre,         { active_qchar = 0; next_cell(); });
  TTS(ReadQuotedCheckEscape, Enewline,    Start,               { active_qchar = 0; next_cell(); end_row(); });
  TTS(ReadQuotedCheckEscape, Edos_cr,     ReadDosCR,           { active_qchar = 0; next_cell(); end_row(); });
  TTS(ReadQuotedCheckEscape, Ewhitespace, ReadQuotedSkipPost,  { active_qchar = 0; });
  TTS(ReadQuotedCheckEscape, Echar,       ReadError,           { error_message = "char after possible endquote"; });

  TTS(ReadQuotedSkipPost, Eqchar,      ReadError,           { error_message = "quote after endquote"; });
  TTS(ReadQuotedSkipPost, Esep,        ReadSkipPre,         { next_cell(); });
  TTS(ReadQuotedSkipPost, Enewline,    Start,               { next_cell(); end_row(); });
  TTS(ReadQuotedSkipPost, Edos_cr,     ReadDosCR,           { next_cell(); end_row(); });
  TTS(ReadQuotedSkipPost, Ewhitespace, ReadQuotedSkipPost,  {});
  TTS(ReadQuotedSkipPost, Echar,       ReadError,           { error_message = "char after endquote"; });

  TTS(ReadUnquoted, Eqchar,      ReadError,              { error_message = "unexpected quote in unquoted string"; });
  TTS(ReadUnquoted, Esep,        ReadSkipPre,            { next_cell(); });
  TTS(ReadUnquoted, Enewline,    Start,                  { next_cell(); end_row(); });
  TTS(ReadUnquoted, Edos_cr,     ReadDosCR,              { next_cell(); end_row(); });
  TTS(ReadUnquoted, Ewhitespace, ReadUnquotedWhitespace, { remember_whitespace(); });  // must remember whitespace in case its in the middle of the cell
  TTS(ReadUnquoted, Echar,       ReadUnquoted,           { add_whitespace(); add(); });

  TTS(ReadUnquotedWhitespace, Eqchar,      ReadError,                { error_message = "unexpected quote after unquoted string"; });
  TTS(ReadUnquotedWhitespace, Esep,        ReadSkipPre,              { add_unquoted_post_whitespace(); next_cell(); });
  TTS(ReadUnquotedWhitespace, Enewline,    Start,                    { add_unquoted_post_whitespace(); next_cell(); end_row(); });
  TTS(ReadUnquotedWhitespace, Edos_cr,     ReadDosCR,                { add_unquoted_post_whitespace(); next_cell(); end_row(); });
  TTS(ReadUnquotedWhitespace, Ewhitespace, ReadUnquotedWhitespace,   { remember_whitespace(); });
  TTS(ReadUnquotedWhitespace, Echar,       ReadUnquoted,             { add_whitespace(); add(); });

  TTS(ReadError, Eqchar, ReadError,       { assert(error_message); });
  TTS(ReadError, Esep, ReadError,         { assert(error_message); });
  TTS(ReadError, Enewline, ReadError,     { assert(error_message); });
  TTS(ReadError, Edos_cr,  ReadError,     { assert(error_message); });
  TTS(ReadError, Ewhitespace, ReadError,  { assert(error_message); });
  TTS(ReadError, Echar, ReadError,        { assert(error_message); });

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
     out.begin_row();
  }


  // emits cell to builder and clears buffer
  void next_cell(bool has_content = true)
  {
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
  printf("%s %c\n",typeid(State).name(), t.value);
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
template <class QuoteChars = char, class Separators = char>
struct csvparser {

   // trim_whitespace: remove whitespace around unquoted cells
   // the standard says you should NOT trim, but many people would want to.
   // You can always quote the whitespace, and that will be kept

// default constructor, gives you a parser for standards-compliant CSV
csvparser(csv_builder &out)
 : errmsg(NULL),
   trans(out, false, false)
{
   add_char(qchar, '"');
   add_char(sep, ',');
}

csvparser(csv_builder &out, QuoteChars qchar, Separators sep, bool trim_whitespace = false, bool collapse_separators = false)
 : qchar(qchar),sep(sep),
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

    if (*buf == '\r') {
      state = next(csvFSM::Edos_cr());
    } else if (*buf == '\n') {
      state = next(csvFSM::Enewline());
    } else if ( is_quote_char(qchar, *buf) ) {
      state = next(csvFSM::Eqchar());
    } else if ( is_sep_char(sep, *buf) ) {
      state = next(csvFSM::Esep());
    } else if (*buf == ' ') { // TODO? more (but DO NOT collide with sep=='\t')
      state = next(csvFSM::Ewhitespace());
    } else {
      state = next(csvFSM::Echar());
    }
    if (trans.error_message) {
#ifdef DEBUG
       fprintf(stderr, "State index: %d\n", state.which());
       fprintf(stderr,"csv parse error: %s\n",err->type);
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
  const char *errmsg;

  csvFSM::States state;
  csvFSM::Trans trans;

  // support either single or multiple quote characters
  bool is_quote_char( char qchar, unsigned int val ) const
  {
    return qchar == val;
  }

  bool is_quote_char( std::string const& qchar, unsigned int val ) const
  {
    if (trans.active_qchar != 0)
       return trans.active_qchar == val;
    return qchar.find_first_of(val) != std::string::npos;
  }

  // support either a single sep or a string of seps
  static bool is_sep_char( char sep, unsigned char value ) {
     return sep == value;
  }

  static bool is_sep_char( std::string const& sep, unsigned char value ) {
    return sep.find_first_of(value) != std::string::npos;
  }


   static void add_char( char & target, char src ) {
      target = src;
   }

   static void add_char( std::string & target, char src ) {
      target.push_back(src);
   }


   // finds the next state, performs transition activity
   template <class Event>
   csvFSM::States next(Event event) {
     return boost::apply_visitor( csvFSM::NextState<Event>(trans), state);
   }
};


}
