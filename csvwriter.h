#pragma once

#include "csvbase.h"

namespace cppcsv {

template <typename Output>
class csv_writer : public csv_builder {
public:
   // note: the CSV standard says to use DOS-CR
   // https://tools.ietf.org/html/rfc4180
  csv_writer(char qchar='"',char sep=',',bool smart_quote=false, bool dos_cr=true)
    : qchar(qchar),sep(sep),
      smart_quote(smart_quote),
      dos_cr(dos_cr),
      first(true)
  {}
  csv_writer(Output out,char qchar='"',char sep=',',bool smart_quote=false, bool dos_cr=true)
    : out(out),
      qchar(qchar),sep(sep),
      smart_quote(smart_quote),
      dos_cr(dos_cr),
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
     if (dos_cr)
       out("\r\n",2);
     else
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
  bool dos_cr;

  bool first;
};

}
