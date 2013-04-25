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
struct Echar {
  Echar(unsigned char value) : value(value) {}
  unsigned char value;
};
struct Ewhitespace { 
  Ewhitespace(unsigned char value) : value(value) {}
  unsigned char value;
};
struct Eqchar {
  Eqchar(unsigned char value) : value(value) {}
  unsigned char value;
};
struct Esep {
  Esep(unsigned char value) : value(value) {}
  unsigned char value;
};
struct Enewline {
  Enewline(unsigned char value) : value(value) {}
  unsigned char value;
};

struct Trans {
  Trans(csv_builder &out) : error_message(NULL), out(out) {}

  const char* error_message;
  std::string whitespace_state;

    // note: states are just empty structs, so pass by copy should be faster
#define TTS(State, E, Snew, code) States on(State, E const& e) { code; return Snew(); }
  //  State          Event        Next_State    Transition_Action
  TTS(Start,         Eqchar,      ReadQuoted,   { out.begin_row(); });
  TTS(Start,         Esep,        ReadSkipPre,  { out.begin_row(); next_cell(false); });
  TTS(Start,         Enewline,    Start,        { out.begin_row(); out.end_row(); });
  TTS(Start,         Ewhitespace, ReadSkipPre,  { out.begin_row(); });
  TTS(Start,         Echar,       ReadUnquoted, { out.begin_row(); add(e.value); });

  TTS(ReadSkipPre,   Eqchar,      ReadQuoted,   {});
  TTS(ReadSkipPre,   Esep,        ReadSkipPre,  { next_cell(false); });
  TTS(ReadSkipPre,   Enewline,    Start,        { next_cell(false); out.end_row(); });
  TTS(ReadSkipPre,   Ewhitespace, ReadSkipPre,  {});
  TTS(ReadSkipPre,   Echar,       ReadUnquoted, { add(e.value); });

  TTS(ReadQuoted,    Eqchar,      ReadQuotedCheckEscape, {});
  TTS(ReadQuoted,    Esep,        ReadQuoted,            { add(e.value); });
  TTS(ReadQuoted,    Enewline,    ReadQuoted,            { add(e.value); });
  TTS(ReadQuoted,    Ewhitespace, ReadQuoted,            { add(e.value); });
  TTS(ReadQuoted,    Echar,       ReadQuoted,            { add(e.value); });

  TTS(ReadQuotedCheckEscape, Eqchar,      ReadQuoted,          { add(e.value); });
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
  TTS(ReadUnquoted, Ewhitespace, ReadUnquotedWhitespace, { remember_whitespace(e.value); });
  TTS(ReadUnquoted, Echar,       ReadUnquoted,           { add(e.value); });

  TTS(ReadUnquotedWhitespace, Eqchar,      ReadError,                { error_message = "unexpected quote after unquoted string"; });
  TTS(ReadUnquotedWhitespace, Esep,        ReadSkipPre,              { add_whitespace(); next_cell(); });
  TTS(ReadUnquotedWhitespace, Enewline,    Start,                    { add_whitespace(); next_cell(); out.end_row(); });
  TTS(ReadUnquotedWhitespace, Ewhitespace, ReadUnquotedWhitespace,   { remember_whitespace(e.value); });
  TTS(ReadUnquotedWhitespace, Echar,       ReadUnquoted,             { add_whitespace(); add(e.value); });

  TTS(ReadError, Eqchar, ReadError,       { assert(error_message); });
  TTS(ReadError, Esep, ReadError,         { assert(error_message); });
  TTS(ReadError, Enewline, ReadError,     { assert(error_message); });
  TTS(ReadError, Ewhitespace, ReadError,  { assert(error_message); });
  TTS(ReadError, Echar, ReadError,        { assert(error_message); });

#undef TTS

private:
  void add(unsigned char value)
  {
     cell.push_back(value);
  }

  // whitespace is remembered until we know if we need to add to the output or forget
  void remember_whitespace(unsigned char value)
  {
     whitespace_state.push_back(value);
  }

  void add_whitespace()
  {
     cell.append(whitespace_state);
     whitespace_state.clear(); 
  }

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
private:
  csv_builder &out;
  std::string cell;
};

  template <class Event>
  struct NextState : boost::static_visitor<States> {
    NextState(const Event &e, Trans &t) : e(e),t(t) {}

    // note: states are just empty structs, so pass by copy should be faster
    template <class State>
    States operator()(State s) const {
#ifdef DEBUG
  printf("%s %c\n",typeid(State).name(),e.value);
#endif
      return t.on(s,e);
    }

  private:
    const Event &e;
    Trans &t;
  };


} // namespace csvFSM


struct csvparser {
csvparser(csv_builder &out,char qchar='"',char sep=',')
 : qchar(qchar),sep(sep),
   errmsg(NULL),
   trans(out)
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
    if (*buf == qchar) {
      state = next(csvFSM::Eqchar(*buf));
    } else if (*buf == sep) {
      state = next(csvFSM::Esep(*buf));
    } else if (*buf == ' ') { // TODO? more (but DO NOT collide with sep=='\t')
      state = next(csvFSM::Ewhitespace(*buf));
    } else if (*buf == '\n') {
      state = next(csvFSM::Enewline(*buf));
    } else {
      state = next(csvFSM::Echar(*buf));
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
   csvFSM::States next(const Event & event) {
     return boost::apply_visitor( csvFSM::NextState<Event>(event,trans), state);
   }
};


}
