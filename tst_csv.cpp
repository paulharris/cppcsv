#include <stdio.h>
#include <assert.h>
#include <fstream>
#include "csvparser.hpp"
#include "csvwriter.h"
#include "simplecsv.h"

#define PRINT_OUT 2

using cppcsv::csv_builder;
using cppcsv::csvparser;
using cppcsv::csv_writer;

// defined in tst_csv_2.cpp
// I am doing this to test that we can use the csv parser in
// multiple compile units without problems.
void do_test_again();

class debug_builder : public csv_builder {
public:
  virtual void begin_row() {
    printf("begin_row\n");
  }
  virtual void cell(const char *buf,int len) {
    if (!buf) {
      printf("(null) ");
    } else {
      printf("[%.*s] ",len,buf);
    }
  }
  virtual void end_row() {
    printf("end_row\n");
  }
};

class null_builder : public csv_builder {
public:
  virtual void cell(const char *buf,int len) {}
};

struct file_out {
  file_out(FILE *f) : f(f) { assert(f); }

  void operator()(const char *buf,int len) {
    fwrite(buf,1,len,f);
  }
private:
  FILE *f;
};

int main(int argc,char **argv)
{
//  debug_builder dbg;
//  null_builder dbg;
//  csv_writer<file_out> dbg((file_out(stdout))); // CPP11: dbg{{stdout}}
//  csv_writer<file_out> dbg(file_out(stdout),'\'',',',true);


{
#if (PRINT_OUT==1)
  SimpleCSV::Table tbl;
  SimpleCSV::builder dbg(tbl);
#elif (PRINT_OUT==2)
  debug_builder dbg;
#else
   null_builder dbg;
#endif

  cppcsv::csvparser<char,char> cp(dbg,'\'',',');
  cp(
    "\n"
    "1, 's' , 3,4   a\n"
    ",1,2,3,4\n"
    " asdf, 'asd''df', s\n"
  );

#if (PRINT_OUT==1)
  csv_writer<file_out> dbg2(file_out(stdout),'\'',',',true);
  tbl.write(dbg2);
#endif
}

    printf("\n\n-- DOS Test (without trim) ---\n\n");

{
#if (PRINT_OUT==1)
  SimpleCSV::Table tbl;
  SimpleCSV::builder dbg(tbl);
#elif (PRINT_OUT==2)
  debug_builder dbg;
#else
   null_builder dbg;
#endif

  cppcsv::csvparser<char,char> cp(dbg,'"',',');
  std::ifstream in("test_dos.csv");

  in.seekg (0, in.end);
  int length = in.tellg();
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\n", cp.error());
  }

#if (PRINT_OUT==1)
  csv_writer<file_out> dbg2(file_out(stdout),'\'',',',true);
  tbl.write(dbg2);
#endif

  delete[] buffer;
}

    printf("\n\n-- Test WITHOUT Trim: John Smith should have lots of whitespace around him (and 'Address') ---\n\n");

{
#if (PRINT_OUT==1)
  SimpleCSV::Table tbl;
  SimpleCSV::builder dbg(tbl);
#elif (PRINT_OUT==2)
  debug_builder dbg;
#else
   null_builder dbg;
#endif

  cppcsv::csvparser<char,char> cp(dbg,'"',',');
  std::ifstream in("test.csv");

  in.seekg (0, in.end);
  int length = in.tellg();
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\n", cp.error());
  }

#if (PRINT_OUT==1)
  csv_writer<file_out> dbg2(file_out(stdout),'\'',',',true);
  tbl.write(dbg2);
#endif

  delete[] buffer;
}


    printf("\n\n-- Test with Trim: John Smith should have no whitespace around him (and 'Address') ---\n\n");

{
#if (PRINT_OUT==1)
  SimpleCSV::Table tbl;
  SimpleCSV::builder dbg(tbl);
#elif (PRINT_OUT==2)
  debug_builder dbg;
#else
   null_builder dbg;
#endif

  cppcsv::csvparser<char,char> cp(dbg,'"',',',true);
  std::ifstream in("test.csv");

  in.seekg (0, in.end);
  int length = in.tellg();
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\n", cp.error());
  }

#if (PRINT_OUT==1)
  csv_writer<file_out> dbg2(file_out(stdout),'\'',',',true);
  tbl.write(dbg2);
#endif

  delete[] buffer;
}

    printf("\n\n-- Bad Separator Test ---\n\n");

{
#if (PRINT_OUT==1)
  SimpleCSV::Table tbl;
  SimpleCSV::builder dbg(tbl);
#elif (PRINT_OUT==2)
  debug_builder dbg;
#else
   null_builder dbg;
#endif

  cppcsv::csvparser<char,char> cp(dbg,'"',',');
  std::ifstream in("test_bad_separator.csv");

  in.seekg (0, in.end);
  int length = in.tellg();
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\n", cp.error());
  }

#if (PRINT_OUT==1)
  csv_writer<file_out> dbg2(file_out(stdout),'\'',',',true);
  tbl.write(dbg2);
#endif

  delete[] buffer;
}

    printf("\n\n-- DOS Test (bad CR) ---\n\n");

{
#if (PRINT_OUT==1)
  SimpleCSV::Table tbl;
  SimpleCSV::builder dbg(tbl);
#elif (PRINT_OUT==2)
  debug_builder dbg;
#else
   null_builder dbg;
#endif

  cppcsv::csvparser<char,char> cp(dbg,'"',',');
  std::ifstream in("test_bad_dos.csv");

  in.seekg (0, in.end);
  int length = in.tellg();
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\n", cp.error());
  }

#if (PRINT_OUT==1)
  csv_writer<file_out> dbg2(file_out(stdout),'\'',',',true);
  tbl.write(dbg2);
#endif

  delete[] buffer;
}

    printf("\n\n-- Test multiple quotes (FAIL) ---\n\n");

{
#if (PRINT_OUT==1)
  SimpleCSV::Table tbl;
  SimpleCSV::builder dbg(tbl);
#elif (PRINT_OUT==2)
  debug_builder dbg;
#else
   null_builder dbg;
#endif

   // this will fail because it only uses single quotes

  cppcsv::csvparser<char,char> cp(dbg,'"',',');
  std::ifstream in("test_multiple_quotes.csv");

  in.seekg (0, in.end);
  int length = in.tellg();
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\n", cp.error());
  }

#if (PRINT_OUT==1)
  csv_writer<file_out> dbg2(file_out(stdout),'\'',',',true);
  tbl.write(dbg2);
#endif

  delete[] buffer;
}

    printf("\n\n-- Test multiple quotes (SUCCESS) ---\n\n");

{
#if (PRINT_OUT==1)
  SimpleCSV::Table tbl;
  SimpleCSV::builder dbg(tbl);
#elif (PRINT_OUT==2)
  debug_builder dbg;
#else
   null_builder dbg;
#endif

  cppcsv::csvparser<std::string,std::string> cp(dbg,"\"'",";,");
  std::ifstream in("test_multiple_quotes.csv");

  in.seekg (0, in.end);
  int length = in.tellg();
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\n", cp.error());
  }

#if (PRINT_OUT==1)
  csv_writer<file_out> dbg2(file_out(stdout),'\'',',',true);
  tbl.write(dbg2);
#endif

  delete[] buffer;
}

    printf("\n\n-- Test collapse separators (dont collapse) ---\n\n");

{
#if (PRINT_OUT==1)
  SimpleCSV::Table tbl;
  SimpleCSV::builder dbg(tbl);
#elif (PRINT_OUT==2)
  debug_builder dbg;
#else
   null_builder dbg;
#endif

  cppcsv::csvparser<std::string,std::string> cp(dbg,"\"'",";,", false, false);
  std::ifstream in("test_collapse_separators.csv");

  in.seekg (0, in.end);
  int length = in.tellg();
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\n", cp.error());
  }

#if (PRINT_OUT==1)
  csv_writer<file_out> dbg2(file_out(stdout),'\'',',',true);
  tbl.write(dbg2);
#endif

  delete[] buffer;
}

    printf("\n\n-- Test collapse separators (do collapse) ---\n\n");

{
#if (PRINT_OUT==1)
  SimpleCSV::Table tbl;
  SimpleCSV::builder dbg(tbl);
#elif (PRINT_OUT==2)
  debug_builder dbg;
#else
   null_builder dbg;
#endif

  cppcsv::csvparser<std::string,std::string> cp(dbg,"\"'",";,", false, true);
  std::ifstream in("test_collapse_separators.csv");

  in.seekg (0, in.end);
  int length = in.tellg();
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\n", cp.error());
  }

#if (PRINT_OUT==1)
  csv_writer<file_out> dbg2(file_out(stdout),'\'',',',true);
  tbl.write(dbg2);
#endif

  delete[] buffer;
}



    printf("\n\n-- Test comment (start of line only, no trim) ---\n\n");

{
#if (PRINT_OUT==1)
  SimpleCSV::Table tbl;
  SimpleCSV::builder dbg(tbl);
#elif (PRINT_OUT==2)
  debug_builder dbg;
#else
   null_builder dbg;
#endif

  cppcsv::csvparser<char,char> cp(dbg, '"', ',', false, true, '#', true);
  std::ifstream in("test_comment.csv");

  in.seekg (0, in.end);
  int length = in.tellg();
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\n", cp.error());
  }

#if (PRINT_OUT==1)
  csv_writer<file_out> dbg2(file_out(stdout),'\'',',',true);
  tbl.write(dbg2);
#endif

  delete[] buffer;
}



    printf("\n\n-- Test comment (start of line only, with trim) ---\n\n");

{
#if (PRINT_OUT==1)
  SimpleCSV::Table tbl;
  SimpleCSV::builder dbg(tbl);
#elif (PRINT_OUT==2)
  debug_builder dbg;
#else
   null_builder dbg;
#endif

  cppcsv::csvparser<char,char> cp(dbg, '"', ',', true, true, '#', true);
  std::ifstream in("test_comment.csv");

  in.seekg (0, in.end);
  int length = in.tellg();
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\n", cp.error());
  }

#if (PRINT_OUT==1)
  csv_writer<file_out> dbg2(file_out(stdout),'\'',',',true);
  tbl.write(dbg2);
#endif

  delete[] buffer;
}



    printf("\n\n-- Test comment (any location, no trim) ---\n\n");

{
#if (PRINT_OUT==1)
  SimpleCSV::Table tbl;
  SimpleCSV::builder dbg(tbl);
#elif (PRINT_OUT==2)
  debug_builder dbg;
#else
   null_builder dbg;
#endif

  cppcsv::csvparser<char,char> cp(dbg, '"', ',', false, true, '#', false);
  std::ifstream in("test_comment.csv");

  in.seekg (0, in.end);
  int length = in.tellg();
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\n", cp.error());
  }

#if (PRINT_OUT==1)
  csv_writer<file_out> dbg2(file_out(stdout),'\'',',',true);
  tbl.write(dbg2);
#endif

  delete[] buffer;
}



    printf("\n\n-- Test comment (any location, with trim) ---\n\n");

{
#if (PRINT_OUT==1)
  SimpleCSV::Table tbl;
  SimpleCSV::builder dbg(tbl);
#elif (PRINT_OUT==2)
  debug_builder dbg;
#else
   null_builder dbg;
#endif

  cppcsv::csvparser<char,char> cp(dbg, '"', ',', true, true, '#', false);
  std::ifstream in("test_comment.csv");

  in.seekg (0, in.end);
  int length = in.tellg();
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\n", cp.error());
  }

#if (PRINT_OUT==1)
  csv_writer<file_out> dbg2(file_out(stdout),'\'',',',true);
  tbl.write(dbg2);
#endif

  delete[] buffer;
}



    printf("\n\n-- Test comment (alternative comment chars, any location, with trim) ---\n\n");

{
#if (PRINT_OUT==1)
  SimpleCSV::Table tbl;
  SimpleCSV::builder dbg(tbl);
#elif (PRINT_OUT==2)
  debug_builder dbg;
#else
   null_builder dbg;
#endif

  cppcsv::csvparser<char,char,std::string> cp(dbg, '"', ',', true, true, "!#", false);
  std::ifstream in("test_comment.csv");

  in.seekg (0, in.end);
  int length = in.tellg();
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\n", cp.error());
  }

#if (PRINT_OUT==1)
  csv_writer<file_out> dbg2(file_out(stdout),'\'',',',true);
  tbl.write(dbg2);
#endif

  delete[] buffer;
}



    printf("\n\n-- Test csv file with multiple tabs and spaces as separators, WITHOUT collapse ---\n\n");

{
#if (PRINT_OUT==1)
  SimpleCSV::Table tbl;
  SimpleCSV::builder dbg(tbl);
#elif (PRINT_OUT==2)
  debug_builder dbg;
#else
   null_builder dbg;
#endif

   // trim whitespace, Don't collapse separators
  cppcsv::csvparser<char,std::string> cp(dbg, '"', "\t ", true, false);
  std::ifstream in("test_whitespace_sep.csv");

  in.seekg (0, in.end);
  int length = in.tellg();
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\n", cp.error());
  }

#if (PRINT_OUT==1)
  csv_writer<file_out> dbg2(file_out(stdout),'\'',',',true);
  tbl.write(dbg2);
#endif

  delete[] buffer;
}



    printf("\n\n-- Test csv file with multiple tabs and spaces as separators ---\n\n");

{
#if (PRINT_OUT==1)
  SimpleCSV::Table tbl;
  SimpleCSV::builder dbg(tbl);
#elif (PRINT_OUT==2)
  debug_builder dbg;
#else
   null_builder dbg;
#endif

   // trim whitespace, collapse separators
  cppcsv::csvparser<char,std::string> cp(dbg, '"', "\t ", true, true);
  std::ifstream in("test_whitespace_sep.csv");

  in.seekg (0, in.end);
  int length = in.tellg();
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\n", cp.error());
  }

#if (PRINT_OUT==1)
  csv_writer<file_out> dbg2(file_out(stdout),'\'',',',true);
  tbl.write(dbg2);
#endif

  delete[] buffer;
}

  do_test_again();

  return 0;
}

