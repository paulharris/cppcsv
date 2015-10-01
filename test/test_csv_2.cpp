#include <cstdio>
#include <cassert>
#include <fstream>
#include <cppcsv/csvparser.hpp>
#include <cppcsv/csvwriter.hpp>
#include <cppcsv/simplecsv.hpp>

using cppcsv::csvparser;
using cppcsv::csv_writer;

class null_builder2 : public cppcsv::per_cell_tag {
public:
  void begin_row() {}
  void cell(const char *buf,int len) {}
  void end_row() {}
};

void do_test_again()
{
   null_builder2 dbg;

  cppcsv::csvparser<null_builder2,char,char> cp(dbg,'\'',',');
  cp(
    "\n"
    "1, 's' , 3,4   a\n"
    ",1,2,3,4\n"
    " asdf, 'asd''df', s\n"
  );
}
