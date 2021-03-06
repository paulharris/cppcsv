#include <cstdio>
#include <cassert>
#include <fstream>
#include <cppcsv/csvparser.hpp>
#include <cppcsv/csvwriter.hpp>
#include <cppcsv/simplecsv.hpp>

#include "test_csv_2.hpp"

using cppcsv::csv_parser;
using cppcsv::csv_writer;

class null_builder2 : public cppcsv::per_cell_tag {
public:
  void begin_row() {}
  void cell(const char *buf, size_t len) {}
  void end_row() {}
};

void do_test_again()
{
   null_builder2 dbg;

  cppcsv::csv_parser<null_builder2,char,char> cp(dbg,'\'',',');
  cp(
    "\n"
    "1, 's' , 3,4   a\n"
    ",1,2,3,4\n"
    " asdf, 'asd''df', s\n"
  );
}
