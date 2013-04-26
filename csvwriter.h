#pragma once

#include "csvbase.h"

namespace cppcsv {

template <typename Output>
class csv_writer : public csv_builder {
public:
  csv_writer(char qchar='"',char sep=',',bool smart_quote=false)
    : qchar(qchar),sep(sep),
      smart_quote(smart_quote),
      first(true)
  {}
  csv_writer(Output out,char qchar='"',char sep=',',bool smart_quote=false)
    : out(out),
      qchar(qchar),sep(sep),
      smart_quote(smart_quote),
      first(true)
  {}

  virtual void begin_row() {
    first=true;
  }
  virtual void cell(const char *buf,int len) {
    if (!first) {
      out(&sep,1);
    } else {
      first=false;
    }
    if (!buf) {
      return;
    }
    if ( (smart_quote)&&(!need_quote(buf,len)) ) {
      out(buf,len);
    } else {
      out(&qchar,1);

      const char *pos=buf;
      while (len>0) {
        if (*pos==qchar) {
          out(buf,pos-buf+1); // first qchar
          buf=pos; // qchar still there! (second one)
        }
        pos++;
        len--;
      }
      out(buf,pos-buf);

      out(&qchar,1);
    }
  }
  virtual void end_row() {
    out("\n",1);
  }
private:
  bool need_quote(const char *buf,int len) const {
    while (len>0) {
      if ( (*buf==qchar)||(*buf==sep)||(*buf=='\n') ) {
        return true;
      }
      buf++;
      len--;
    }
    return false;
  }

private:
  Output out;
  char qchar;
  char sep;
  bool smart_quote;

  bool first;
};



// note: the CSV standard says to use DOS-CR
// https://tools.ietf.org/html/rfc4180
template <class Output>
struct add_dos_cr_out {
  add_dos_cr_out( Output out ) : out(out) {}

  void operator()(const char *buf,int len) {
    bool last_was_cr = false;
    const char *pos = buf;

    while (len>0) {
      if ( (*pos == '\n') && !last_was_cr ) {
        out(buf, pos-buf); // print what came before newline
        out("\r", 1);      // print CR
        buf = pos;         // start from the newline (it'll be printed later)
      }

      last_was_cr = (*pos == '\r');

      pos++;
      len--;
    }

    out(buf,pos-buf);
  }

private:
  Output out;
};

}
