#include <stdio.h>
#include <assert.h>
#include <fstream>
#include "csvparser.hpp"
#include "csvwriter.h"
#include "simplecsv.h"

using cppcsv::csv_builder;
using cppcsv::csvparser;
using cppcsv::csv_writer;

class null_builder2 : public csv_builder {
public:
  virtual void cell(const char *buf,int len) {}
};

void do_test_again()
{
   null_builder2 dbg;

  cppcsv::csvparser cp(dbg,'\'');
  cp(
    "\n"
    "1, 's' , 3,4   a\n"
    ",1,2,3,4\n"
    " asdf, 'asd''df', s\n"
  );
}
