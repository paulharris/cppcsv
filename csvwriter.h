#pragma once

#include "csvbase.h"

namespace cppcsv {

template <typename Output>  // ("asdf",4)
class csv_writer : public csv_builder { // {{{
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
  bool need_quote(const char *buf,int len) const { // {{{
    while (len>0) {
      if ( (*buf==qchar)||(*buf==sep)||(*buf=='\n') ) {
        return true;
      }
      buf++;
      len--;
    }
    return false;
  }
  // }}}
private:
  Output out;
  char qchar;
  char sep;
  bool smart_quote;

  bool first;
};
// }}}

}
