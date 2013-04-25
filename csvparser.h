#ifndef _CSVPARSER_H
#define _CSVPARSER_H

#include <string>

class csv_builder;  // csvbase.h
struct csvparser {
  csvparser(csv_builder &out,char qchar='"',char sep=',');
  
  // NOTE: returns true on error
  bool operator()(const std::string &line); // not required to be linewise
  bool operator()(const char *&buf,int len);

  const char *error() const;

private:
  csv_builder &out;
  char qchar;
  char sep;
  const char *errmsg;
};

#endif
