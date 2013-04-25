#include <assert.h>
#include <string.h>
#include "csvbase.h"

#include <boost/variant.hpp>

//#define DEBUG

#ifdef DEBUG
  #include <stdio.h>
  #include <typeinfo>
#endif

namespace cppcsv {
namespace csvFSM {

// States
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

#define TTS(S,E,Snew,code) void on(States &self, S &s, const E &e) { code; self = Snew; }
struct Trans {
  Trans(csv_builder &out) : error_message(NULL), out(out) {}

  const char* error_message;
  std::string whitespace_state;

  //  State  Event        Next_State      Transition_Action
  TTS(Start, Eqchar,      ReadQuoted(),   { out.begin_row(); });
  TTS(Start, Esep,        ReadSkipPre(),  { out.begin_row(); next_cell(false); });
  TTS(Start, Enewline,    self,           { out.begin_row(); out.end_row(); });
  TTS(Start, Ewhitespace, ReadSkipPre(),  { out.begin_row(); });
  TTS(Start, Echar,       ReadUnquoted(), { out.begin_row(); add(e); });

  TTS(ReadSkipPre, Eqchar,      ReadQuoted(),   {});
  TTS(ReadSkipPre, Esep,        self,           { next_cell(false); });
  TTS(ReadSkipPre, Enewline,    Start(),        { next_cell(false); out.end_row(); });
  TTS(ReadSkipPre, Ewhitespace, self,           {});
  TTS(ReadSkipPre, Echar,       ReadUnquoted(), { add(e); });

  TTS(ReadQuoted, Eqchar,      ReadQuotedCheckEscape(), {});
  TTS(ReadQuoted, Esep,        self, { add(e); });
  TTS(ReadQuoted, Enewline,    self, { add(e); });
  TTS(ReadQuoted, Ewhitespace, self, { add(e); });
  TTS(ReadQuoted, Echar,       self, { add(e); });

  TTS(ReadQuotedCheckEscape, Eqchar,      ReadQuoted(),  { add(e); });
  TTS(ReadQuotedCheckEscape, Esep,        ReadSkipPre(), { next_cell(); });
  TTS(ReadQuotedCheckEscape, Enewline,    Start(),       { next_cell(); out.end_row(); });
  TTS(ReadQuotedCheckEscape, Ewhitespace, ReadQuotedSkipPost(), {});
  TTS(ReadQuotedCheckEscape, Echar,       ReadError(),   { error_message = "char after possible endquote"; });

  TTS(ReadQuotedSkipPost, Eqchar,      ReadError(),   { error_message = "quote after endquote"; });
  TTS(ReadQuotedSkipPost, Esep,        ReadSkipPre(), { next_cell(); });
  TTS(ReadQuotedSkipPost, Enewline,    Start(),       { next_cell(); out.end_row(); });
  TTS(ReadQuotedSkipPost, Ewhitespace, self,          {});
  TTS(ReadQuotedSkipPost, Echar,       ReadError(),   { error_message = "char after endquote"; });

  TTS(ReadUnquoted, Eqchar,      ReadError(),   { error_message = "unexpected quote in unquoted string"; });
  TTS(ReadUnquoted, Esep,        ReadSkipPre(), { next_cell(); });
  TTS(ReadUnquoted, Enewline,    Start(),       { next_cell(); out.end_row(); });
  TTS(ReadUnquoted, Ewhitespace, ReadUnquotedWhitespace(), { whitespace_state.push_back(e.value); });
  TTS(ReadUnquoted, Echar,       self,          { add(e); });

  TTS(ReadUnquotedWhitespace, Eqchar,      ReadError(),   { error_message = "unexpected quote after unquoted string"; });
  TTS(ReadUnquotedWhitespace, Esep,        ReadSkipPre(),  { cell.append(whitespace_state); whitespace_state.clear(); next_cell(); });
  TTS(ReadUnquotedWhitespace, Enewline,    Start(),        { cell.append(whitespace_state); whitespace_state.clear(); next_cell(); out.end_row(); });
  TTS(ReadUnquotedWhitespace, Ewhitespace, self,           { whitespace_state.push_back(e.value); });
  TTS(ReadUnquotedWhitespace, Echar,       ReadUnquoted(), { cell.append(whitespace_state); whitespace_state.clear(); add(e); });

  TTS(ReadError, Eqchar, self, { assert(error_message); });
  TTS(ReadError, Esep, self, { assert(error_message); });
  TTS(ReadError, Enewline, self, { assert(error_message); });
  TTS(ReadError, Ewhitespace, self, { assert(error_message); });
  TTS(ReadError, Echar, self, { assert(error_message); });

#undef TTS

private:
  template <class Event>
  void add(const Event &e) { cell.push_back(e.value); }

  inline void next_cell(bool has_content=true) {
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

namespace detail {
  template <class StateVariant,class Event,class Transitions>
  struct NextState : boost::static_visitor<void> {
    NextState(StateVariant &v,const Event &e,Transitions &t) : v(v),e(e),t(t) {}

    template <class State>
    void operator()(State &s) const {
#ifdef DEBUG
  printf("%s %c\n",typeid(State).name(),e.value);
#endif
      t.on(v,s,e);
    }

  private:
    StateVariant &v;
    const Event &e;
    Transitions &t;
  };
} // namespace detail

#ifdef CPP11
template <class Transitions=Trans,class StateVariant,class Event>
void next(StateVariant &state,Event &&event,Transitions &&trans=Transitions()) {
  boost::apply_visitor(detail::NextState<StateVariant,Event,Transitions>(state,std::forward<Event>(event),std::forward<Transitions>(trans)),state);
}
#else
template <class Transitions,class StateVariant,class Event>
void next(StateVariant &state,const Event &event,const Transitions &trans=Transitions()) {
  boost::apply_visitor(detail::NextState<StateVariant,Event,Transitions>(state,event,trans),state);
}
template <class Transitions,class StateVariant,class Event>
void next(StateVariant &state,const Event &event,Transitions &trans) {
  boost::apply_visitor(detail::NextState<StateVariant,Event,Transitions>(state,event,trans),state);
}
#endif

} // namespace csvFSM


struct csvparser {
csvparser(csv_builder &out,char qchar='"',char sep=',')
 : out(out),
   qchar(qchar),sep(sep),
   errmsg(NULL)
{} 
  
  // NOTE: returns true on error
bool operator()(const std::string &line) // not required to be linewise
{
  const char *buf=line.c_str();
  return (operator())(buf,line.size());
}

bool operator()(const char *&buf,int len)
{
  csvFSM::States state;
  csvFSM::Trans trans(out);
  while (len > 0) {
    if (*buf == qchar) {
      csvFSM::next(state,csvFSM::Eqchar(*buf),trans);
    } else if (*buf == sep) {
      csvFSM::next(state,csvFSM::Esep(*buf),trans);
    } else if (*buf == ' ') { // TODO? more (but DO NOT collide with sep=='\t')
      csvFSM::next(state,csvFSM::Ewhitespace(*buf),trans);
    } else if (*buf == '\n') {
      csvFSM::next(state,csvFSM::Enewline(*buf),trans);
    } else {
      csvFSM::next(state,csvFSM::Echar(*buf),trans);
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
  csv_builder &out;
  char qchar;
  char sep;
  const char *errmsg;
};


}
