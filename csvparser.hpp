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
struct ReadUnquoted {};
struct ReadUnquotedWhitespace {};
struct ReadError {};

typedef boost::variant<Start,
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


struct Trans {
  Trans(csv_builder &out, bool trim_whitespace) : value(0), error_message(NULL), out(out), trim_whitespace(trim_whitespace) {}

  // this is set before the state change is called,
  // that way the Events do not need to carry their state with them.
  unsigned char value;

  const char* error_message;

    // note: states and events are just empty structs, so pass by copy should be faster
#define TTS(State, Event, NextState, code) States on(State, Event) { code; return NextState(); }
  //  State          Event        Next_State    Transition_Action
  TTS(Start,         Eqchar,      ReadQuoted,   { out.begin_row(); });
  TTS(Start,         Esep,        ReadSkipPre,  { out.begin_row(); next_cell(false); });
  TTS(Start,         Enewline,    Start,        { out.begin_row(); out.end_row(); });
  TTS(Start,         Ewhitespace, ReadSkipPre,  { if (!trim_whitespace) { remember_whitespace(); } out.begin_row(); });   // we MAY want to remember this whitespace
  TTS(Start,         Echar,       ReadUnquoted, { out.begin_row(); add(); });

  TTS(ReadSkipPre,   Eqchar,      ReadQuoted,   { drop_whitespace(); });   // we always want to forget whitespace before the quotes
  TTS(ReadSkipPre,   Esep,        ReadSkipPre,  { next_cell(false); });
  TTS(ReadSkipPre,   Enewline,    Start,        { next_cell(false); out.end_row(); });
  TTS(ReadSkipPre,   Ewhitespace, ReadSkipPre,  { if (!trim_whitespace) { remember_whitespace(); } });  // we MAY want to remember this whitespace
  TTS(ReadSkipPre,   Echar,       ReadUnquoted, { add_whitespace(); add(); });   // we add whitespace IF any was recorded

  TTS(ReadQuoted,    Eqchar,      ReadQuotedCheckEscape, {});
  TTS(ReadQuoted,    Esep,        ReadQuoted,            { add(); });
  TTS(ReadQuoted,    Enewline,    ReadQuoted,            { add(); });
  TTS(ReadQuoted,    Ewhitespace, ReadQuoted,            { add(); });
  TTS(ReadQuoted,    Echar,       ReadQuoted,            { add(); });

  TTS(ReadQuotedCheckEscape, Eqchar,      ReadQuoted,          { add(); });
  TTS(ReadQuotedCheckEscape, Esep,        ReadSkipPre,         { next_cell(); });
  TTS(ReadQuotedCheckEscape, Enewline,    Start,               { next_cell(); out.end_row(); });
  TTS(ReadQuotedCheckEscape, Ewhitespace, ReadQuotedSkipPost,  {});
  TTS(ReadQuotedCheckEscape, Echar,       ReadError,           { error_message = "char after possible endquote"; });

  TTS(ReadQuotedSkipPost, Eqchar,      ReadError,           { error_message = "quote after endquote"; });
  TTS(ReadQuotedSkipPost, Esep,        ReadSkipPre,         { next_cell(); });
  TTS(ReadQuotedSkipPost, Enewline,    Start,               { next_cell(); out.end_row(); });
  TTS(ReadQuotedSkipPost, Ewhitespace, ReadQuotedSkipPost,  {});
  TTS(ReadQuotedSkipPost, Echar,       ReadError,           { error_message = "char after endquote"; });

  TTS(ReadUnquoted, Eqchar,      ReadError,              { error_message = "unexpected quote in unquoted string"; });
  TTS(ReadUnquoted, Esep,        ReadSkipPre,            { next_cell(); });
  TTS(ReadUnquoted, Enewline,    Start,                  { next_cell(); out.end_row(); });
  TTS(ReadUnquoted, Ewhitespace, ReadUnquotedWhitespace, { remember_whitespace(); });  // must remember whitespace in case its in the middle of the cell
  TTS(ReadUnquoted, Echar,       ReadUnquoted,           { add_whitespace(); add(); });

  TTS(ReadUnquotedWhitespace, Eqchar,      ReadError,                { error_message = "unexpected quote after unquoted string"; });
  TTS(ReadUnquotedWhitespace, Esep,        ReadSkipPre,              { add_unquoted_post_whitespace(); next_cell(); });
  TTS(ReadUnquotedWhitespace, Enewline,    Start,                    { add_unquoted_post_whitespace(); next_cell(); out.end_row(); });
  TTS(ReadUnquotedWhitespace, Ewhitespace, ReadUnquotedWhitespace,   { remember_whitespace(); });
  TTS(ReadUnquotedWhitespace, Echar,       ReadUnquoted,             { add_whitespace(); add(); });

  TTS(ReadError, Eqchar, ReadError,       { assert(error_message); });
  TTS(ReadError, Esep, ReadError,         { assert(error_message); });
  TTS(ReadError, Enewline, ReadError,     { assert(error_message); });
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

  csv_builder &out;
  std::string cell;
  std::string whitespace_state;
  bool trim_whitespace;
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


struct csvparser {

   // trim_whitespace: remove whitespace around unquoted cells
   // the standard says you should NOT trim, but many people would want to.
   // You can always quote the whitespace, and that will be kept

csvparser(csv_builder &out,char qchar='"',char sep=',', bool trim_whitespace = false)
 : qchar(qchar),sep(sep),
   errmsg(NULL),
   trans(out, trim_whitespace)
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

    if (*buf == qchar) {
      state = next(csvFSM::Eqchar());
    } else if (*buf == sep) {
      state = next(csvFSM::Esep());
    } else if (*buf == ' ') { // TODO? more (but DO NOT collide with sep=='\t')
      state = next(csvFSM::Ewhitespace());
    } else if (*buf == '\n') {
      state = next(csvFSM::Enewline());
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
  char qchar;
  char sep;
  const char *errmsg;

  csvFSM::States state;
  csvFSM::Trans trans;

   template <class Event>
   csvFSM::States next(Event event) {
     return boost::apply_visitor( csvFSM::NextState<Event>(trans), state);
   }
};


}
