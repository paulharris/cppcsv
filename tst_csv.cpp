#include <stdio.h>
#include <assert.h>
#include <fstream>
#include "csvparser.h"
#include "csvwriter.h"
#include "simplecsv.h"

#if !defined(__GXX_EXPERIMENTAL_CXX0X__)&&(__cplusplus<201103L)
  #define override
#endif

#define PRINT_OUT 2

class debug_builder : public csv_builder {
public:
  void begin_row() override { 
    printf("begin_row\n");
  }
  void cell(const char *buf,int len) override {
    if (!buf) {
      printf("(null) ");
    } else {
      printf("\"%.*s\" ",len,buf);
    }
  }
  void end_row() override {
    printf("end_row\n");
  }
};

class null_builder : public csv_builder {
public:
  void cell(const char *buf,int len) override {}
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

  csvparser cp(dbg,'\'');
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

    printf("\n\n--------------------\n\n");

{
#if (PRINT_OUT==1)
  SimpleCSV::Table tbl;
  SimpleCSV::builder dbg(tbl);
#elif (PRINT_OUT==2)
  debug_builder dbg;
#else
   null_builder dbg;
#endif

  csvparser cp(dbg,'"');
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

  csvparser cp(dbg,'"');
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
}

  return 0;
}

