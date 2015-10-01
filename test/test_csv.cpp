#include <cppcsv/csvparser.hpp>
#include <cppcsv/csvwriter.hpp>
#include <cppcsv/simplecsv.hpp>

#include <cstdio>
#include <cassert>
#include <fstream>
#include <boost/array.hpp>

#define PRINT_OUT 2

using cppcsv::csv_parser;
using cppcsv::csv_writer;

// defined in tst_csv_2.cpp
// I am doing this to test that we can use the csv parser in
// multiple compile units without problems.
void do_test_again();

void open_check( const char* filename, std::ifstream & in )
{
   in.open(filename);
   if (!in)
   {
      printf("Could not open file %s\n", filename);
      exit(1);
   }
}


class debug_builder : public cppcsv::per_cell_tag {
public:
  void begin_row() {
    printf("begin_row\n");
  }
  void cell(const char *buf, size_t len) {
    if (!buf) {
      printf("(null) ");
    } else {
      printf("[%.*s] ",len,buf);
    }
  }
  void end_row() {
    printf("end_row\n");
  }
};

class null_builder : public cppcsv::per_cell_tag {
public:
  void begin_row() {}
  void cell(const char *buf, size_t len) {}
  void end_row() {}
};

struct file_out {
  file_out(FILE *f) : f(f) { assert(f); }

  void operator()(const char *buf, size_t len) {
    fwrite(buf,1,len,f);
    fflush(f); // immediate flush for help with debugging
  }
private:
  FILE *f;
};



// for testing function row interface
static void print_bulk_row( const char* buffer, size_t num_cells, const size_t * offsets, size_t file_row )
{
  printf("ROW %u (%u cells): ", file_row, num_cells);
  for (size_t i = 0; i != num_cells; ++i) {
    const char* cell_buffer = buffer + offsets[i];
    size_t len = (offsets[i+1] - offsets[i]);

    if (len == 0) {
      printf("(null) ");
    } else {
      printf("[%.*s] ",len, cell_buffer);
    }
  }
  printf("\n");
}



// for testing functor row interface
struct print_bulk_row_t : public cppcsv::per_row_tag
{
  void end_full_row( const char* buffer, size_t num_cells, const size_t * offsets, size_t file_row )
  {
    printf("ROW %u (%u cells): ", file_row, num_cells);
    for (size_t i = 0; i != num_cells; ++i) {
      const char* cell_buffer = buffer + offsets[i];
      size_t len = (offsets[i+1] - offsets[i]);

      if (len == 0) {
        printf("(null) ");
      } else {
        printf("[%.*s] ",len, cell_buffer);
      }
    }
    printf("\n");
  }
};



int main(int argc,char **argv)
{
//  debug_builder dbg;
//  null_builder dbg;
//  csv_writer<file_out> dbg((file_out(stdout))); // CPP11: dbg{{stdout}}
//  csv_writer<file_out> dbg(file_out(stdout),'\'',',',true);


  do_test_again();

{
#if (PRINT_OUT==1)
  SimpleCSV::Table tbl;
  SimpleCSV::builder dbg(tbl);
  typedef SimpleCSV::builder dbg_builder;
#elif (PRINT_OUT==2)
  typedef debug_builder dbg_builder;
  dbg_builder dbg;
#else
  typedef null_builder dbg_builder;
  dbg_builder dbg;
#endif

  cppcsv::csv_parser<dbg_builder,char,char> cp(dbg,'\'',',');
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
  typedef SimpleCSV::builder dbg_builder;
#elif (PRINT_OUT==2)
  typedef debug_builder dbg_builder;
  dbg_builder dbg;
#else
  typedef null_builder dbg_builder;
  dbg_builder dbg;
#endif

  cppcsv::csv_parser<dbg_builder,char,char> cp(dbg,'"',',');
  std::ifstream in;
  open_check("test_dos.csv", in);

  in.seekg (0, in.end);
  size_t length = static_cast<size_t>(in.tellg());
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\nContext:\n%s\n", cp.error(), cp.error_context().c_str());
  }

#if (PRINT_OUT==1)
  FILE * fdos = fopen("out_test_dos_as_simplecsv_dos.csv","wb");
  FILE * funix = fopen("out_test_dos_as_simplecsv_unix.csv","wb");
  csv_writer<file_out> dbg_unix(file_out(funix),'\'',',',true);
  csv_writer< cppcsv::add_dos_cr_out<file_out> > dbg_dos(file_out(fdos),'\'',',',true);
  tbl.write(dbg_unix);
  tbl.write(dbg_dos);
  fclose(funix);
  fclose(fdos);
#endif

  delete[] buffer;
}

    printf("\n\n-- DOS Test (without trim, write to out_test_dos_as_*.csv) ---\n\n");

{
  FILE * funix = fopen("out_test_dos_as_unix.csv","wb");
  FILE * fdos = fopen("out_test_dos_as_dos.csv","wb");

  typedef csv_writer<file_out> debug_unix;
  debug_unix dbg_unix(file_out(funix),'\'',',',true);

  typedef csv_writer< cppcsv::add_dos_cr_out<file_out> > debug_dos;
  debug_dos dbg_dos(file_out(fdos),'\'',',',true);

  cppcsv::csv_parser<debug_unix, char,char> cp_unix(dbg_unix,'"',',');
  cppcsv::csv_parser<debug_dos, char,char> cp_dos(dbg_dos,'"',',');

  std::ifstream in;
  open_check("test_dos.csv", in);

  in.seekg (0, in.end);
  size_t length = static_cast<size_t>(in.tellg());
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp_unix.process_chunk(cursor, length))
        printf("ERROR: %s\nContext:\n%s\n", cp_unix.error(), cp_unix.error_context().c_str());
     cursor = buffer;
     if (cp_dos.process_chunk(cursor, length))
        printf("ERROR: %s\nContext:\n%s\n", cp_dos.error(), cp_dos.error_context().c_str());

     cp_unix.flush();
     cp_dos.flush();
  }

  fclose(funix);
  fclose(fdos);

  delete[] buffer;
}


    printf("\n\n-- Test WITHOUT Trim: John Smith should have lots of whitespace around him (and 'Address') ---\n\n");

{
#if (PRINT_OUT==1)
  SimpleCSV::Table tbl;
  SimpleCSV::builder dbg(tbl);
  typedef SimpleCSV::builder dbg_builder;
#elif (PRINT_OUT==2)
  typedef debug_builder dbg_builder;
  dbg_builder dbg;
#else
  typedef null_builder dbg_builder;
  dbg_builder dbg;
#endif

  cppcsv::csv_parser<dbg_builder,char,char> cp(dbg,'"',',');
  std::ifstream in;
  open_check("test.csv", in);

  in.seekg (0, in.end);
  size_t length = static_cast<size_t>(in.tellg());
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\nContext:\n%s\n", cp.error(), cp.error_context().c_str());
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
  typedef SimpleCSV::builder dbg_builder;
#elif (PRINT_OUT==2)
  typedef debug_builder dbg_builder;
  dbg_builder dbg;
#else
  typedef null_builder dbg_builder;
  dbg_builder dbg;
#endif

  cppcsv::csv_parser<dbg_builder,char,char> cp(dbg,'"',',',true);
  std::ifstream in;
  open_check("test.csv", in);

  in.seekg (0, in.end);
  size_t length = static_cast<size_t>(in.tellg());
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\nContext:\n%s\n", cp.error(), cp.error_context().c_str());
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
  typedef SimpleCSV::builder dbg_builder;
#elif (PRINT_OUT==2)
  typedef debug_builder dbg_builder;
  dbg_builder dbg;
#else
  typedef null_builder dbg_builder;
  dbg_builder dbg;
#endif

  cppcsv::csv_parser<dbg_builder,char,char> cp(dbg,'"',',');
  std::ifstream in;
  open_check("test_bad_separator.csv", in);

  in.seekg (0, in.end);
  size_t length = static_cast<size_t>(in.tellg());
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\nContext:\n%s\n", cp.error(), cp.error_context().c_str());
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
  typedef SimpleCSV::builder dbg_builder;
#elif (PRINT_OUT==2)
  typedef debug_builder dbg_builder;
  dbg_builder dbg;
#else
  typedef null_builder dbg_builder;
  dbg_builder dbg;
#endif

  cppcsv::csv_parser<dbg_builder,char,char> cp(dbg,'"',',');
  std::ifstream in;
  open_check("test_bad_dos.csv", in);

  in.seekg (0, in.end);
  size_t length = static_cast<size_t>(in.tellg());
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\nContext:\n%s\n", cp.error(), cp.error_context().c_str());
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
  typedef SimpleCSV::builder dbg_builder;
#elif (PRINT_OUT==2)
  typedef debug_builder dbg_builder;
  dbg_builder dbg;
#else
  typedef null_builder dbg_builder;
  dbg_builder dbg;
#endif

   // this will fail because it only uses single quotes

  cppcsv::csv_parser<dbg_builder,char,char> cp(dbg,'"',',');
  std::ifstream in;
  open_check("test_multiple_quotes.csv", in);

  in.seekg (0, in.end);
  size_t length = static_cast<size_t>(in.tellg());
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\nContext:\n%s\n", cp.error(), cp.error_context().c_str());
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
  typedef SimpleCSV::builder dbg_builder;
#elif (PRINT_OUT==2)
  typedef debug_builder dbg_builder;
  dbg_builder dbg;
#else
  typedef null_builder dbg_builder;
  dbg_builder dbg;
#endif

  cppcsv::csv_parser<dbg_builder, std::string,std::string> cp(dbg,"\"'",";,");
  std::ifstream in;
  open_check("test_multiple_quotes.csv", in);

  in.seekg (0, in.end);
  size_t length = static_cast<size_t>(in.tellg());
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\nContext:\n%s\n", cp.error(), cp.error_context().c_str());
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
  typedef SimpleCSV::builder dbg_builder;
#elif (PRINT_OUT==2)
  typedef debug_builder dbg_builder;
  dbg_builder dbg;
#else
  typedef null_builder dbg_builder;
  dbg_builder dbg;
#endif

  cppcsv::csv_parser<dbg_builder, std::string,std::string> cp(dbg,"\"'",";,", false, false);
  std::ifstream in;
  open_check("test_collapse_separators.csv", in);

  in.seekg (0, in.end);
  size_t length = static_cast<size_t>(in.tellg());
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\nContext:\n%s\n", cp.error(), cp.error_context().c_str());
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
  typedef SimpleCSV::builder dbg_builder;
#elif (PRINT_OUT==2)
  typedef debug_builder dbg_builder;
  dbg_builder dbg;
#else
  typedef null_builder dbg_builder;
  dbg_builder dbg;
#endif

  cppcsv::csv_parser<dbg_builder, std::string,std::string> cp(dbg,"\"'",";,", false, true);
  std::ifstream in;
  open_check("test_collapse_separators.csv", in);

  in.seekg (0, in.end);
  size_t length = static_cast<size_t>(in.tellg());
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\nContext:\n%s\n", cp.error(), cp.error_context().c_str());
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
  typedef SimpleCSV::builder dbg_builder;
#elif (PRINT_OUT==2)
  typedef debug_builder dbg_builder;
  dbg_builder dbg;
#else
  typedef null_builder dbg_builder;
  dbg_builder dbg;
#endif

  cppcsv::csv_parser<dbg_builder,char,char> cp(dbg, '"', ',', false, true, '#', true);
  std::ifstream in;
  open_check("test_comment.csv", in);

  in.seekg (0, in.end);
  size_t length = static_cast<size_t>(in.tellg());
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\nContext:\n%s\n", cp.error(), cp.error_context().c_str());
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
  typedef SimpleCSV::builder dbg_builder;
#elif (PRINT_OUT==2)
  typedef debug_builder dbg_builder;
  dbg_builder dbg;
#else
  typedef null_builder dbg_builder;
  dbg_builder dbg;
#endif

  cppcsv::csv_parser<dbg_builder,char,char> cp(dbg, '"', ',', true, true, '#', true);
  std::ifstream in;
  open_check("test_comment.csv", in);

  in.seekg (0, in.end);
  size_t length = static_cast<size_t>(in.tellg());
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\nContext:\n%s\n", cp.error(), cp.error_context().c_str());
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
  typedef SimpleCSV::builder dbg_builder;
#elif (PRINT_OUT==2)
  typedef debug_builder dbg_builder;
  dbg_builder dbg;
#else
  typedef null_builder dbg_builder;
  dbg_builder dbg;
#endif

  cppcsv::csv_parser<dbg_builder,char,char> cp(dbg, '"', ',', false, true, '#', false);
  std::ifstream in;
  open_check("test_comment.csv", in);

  in.seekg (0, in.end);
  size_t length = static_cast<size_t>(in.tellg());
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\nContext:\n%s\n", cp.error(), cp.error_context().c_str());
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
  typedef SimpleCSV::builder dbg_builder;
#elif (PRINT_OUT==2)
  typedef debug_builder dbg_builder;
  dbg_builder dbg;
#else
  typedef null_builder dbg_builder;
  dbg_builder dbg;
#endif

  cppcsv::csv_parser<dbg_builder,char,char> cp(dbg, '"', ',', true, true, '#', false);
  std::ifstream in;
  open_check("test_comment.csv", in);

  in.seekg (0, in.end);
  size_t length = static_cast<size_t>(in.tellg());
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\nContext:\n%s\n", cp.error(), cp.error_context().c_str());
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
  typedef SimpleCSV::builder dbg_builder;
#elif (PRINT_OUT==2)
  typedef debug_builder dbg_builder;
  dbg_builder dbg;
#else
  typedef null_builder dbg_builder;
  dbg_builder dbg;
#endif

  cppcsv::csv_parser<dbg_builder,char,char,std::string> cp(dbg, '"', ',', true, true, "!#", false);
  std::ifstream in;
  open_check("test_comment.csv", in);

  in.seekg (0, in.end);
  size_t length = static_cast<size_t>(in.tellg());
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\nContext:\n%s\n", cp.error(), cp.error_context().c_str());
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
  typedef SimpleCSV::builder dbg_builder;
#elif (PRINT_OUT==2)
  typedef debug_builder dbg_builder;
  dbg_builder dbg;
#else
  typedef null_builder dbg_builder;
  dbg_builder dbg;
#endif

   // trim whitespace, Don't collapse separators
  cppcsv::csv_parser<dbg_builder,char,std::string> cp(dbg, '"', "\t ", true, false);
  std::ifstream in;
  open_check("test_whitespace_sep.csv", in);

  in.seekg (0, in.end);
  size_t length = static_cast<size_t>(in.tellg());
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\nContext:\n%s\n", cp.error(), cp.error_context().c_str());
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
  typedef SimpleCSV::builder dbg_builder;
#elif (PRINT_OUT==2)
  typedef debug_builder dbg_builder;
  dbg_builder dbg;
#else
  typedef null_builder dbg_builder;
  dbg_builder dbg;
#endif

   // trim whitespace, collapse separators
  cppcsv::csv_parser<dbg_builder,char,std::string> cp(dbg, '"', "\t ", true, true);
  std::ifstream in;
  open_check("test_whitespace_sep.csv", in);

  in.seekg (0, in.end);
  size_t length = static_cast<size_t>(in.tellg());
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\nContext:\n%s\n", cp.error(), cp.error_context().c_str());
  }

#if (PRINT_OUT==1)
  csv_writer<file_out> dbg2(file_out(stdout),'\'',',',true);
  tbl.write(dbg2);
#endif

  delete[] buffer;
}



    printf("\n\n-- Test csv file no newline at the end of the file (no flush) ---\n\n");

{
#if (PRINT_OUT==1)
  SimpleCSV::Table tbl;
  SimpleCSV::builder dbg(tbl);
  typedef SimpleCSV::builder dbg_builder;
#elif (PRINT_OUT==2)
  typedef debug_builder dbg_builder;
  dbg_builder dbg;
#else
  typedef null_builder dbg_builder;
  dbg_builder dbg;
#endif

   // trim whitespace, collapse separators
  cppcsv::csv_parser<dbg_builder, char, char> cp(dbg, '"', ',', false, false);
  std::ifstream in;
  open_check("test_no_last_newline.csv", in);

  in.seekg (0, in.end);
  size_t length = static_cast<size_t>(in.tellg());
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp.process_chunk(cursor, length))
        printf("ERROR: %s\nContext:\n%s\n", cp.error(), cp.error_context().c_str());
  }

#if (PRINT_OUT==1)
  csv_writer<file_out> dbg2(file_out(stdout),'\'',',',true);
  tbl.write(dbg2);
#endif

  delete[] buffer;
}



    printf("\n\n-- Test csv file no newline at the end of the file (with flush) ---\n\n");

{
#if (PRINT_OUT==1)
  SimpleCSV::Table tbl;
  SimpleCSV::builder dbg(tbl);
  typedef SimpleCSV::builder dbg_builder;
#elif (PRINT_OUT==2)
  typedef debug_builder dbg_builder;
  dbg_builder dbg;
#else
  typedef null_builder dbg_builder;
  dbg_builder dbg;
#endif

   // trim whitespace, collapse separators
  cppcsv::csv_parser<dbg_builder, char, char> cp(dbg, '"', ',', false, false);
  std::ifstream in;
  open_check( "test_no_last_newline.csv", in);

  in.seekg (0, in.end);
  size_t length = static_cast<size_t>(in.tellg());
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp.process_chunk(cursor, length))
        printf("ERROR: %s\nContext:\n%s\n", cp.error(), cp.error_context().c_str());
     cp.flush();
  }

#if (PRINT_OUT==1)
  csv_writer<file_out> dbg2(file_out(stdout),'\'',',',true);
  tbl.write(dbg2);
#endif

  delete[] buffer;
}


    printf("\n\n-- Test fixed-number-of-multiple quotes (SUCCESS) ---\n\n");

{
#if (PRINT_OUT==1)
  SimpleCSV::Table tbl;
  SimpleCSV::builder dbg(tbl);
  typedef SimpleCSV::builder dbg_builder;
#elif (PRINT_OUT==2)
  typedef debug_builder dbg_builder;
  dbg_builder dbg;
#else
  typedef null_builder dbg_builder;
  dbg_builder dbg;
#endif

  // use a boost array instead of a string
  // this should allow the compiler to unroll all of match_char loops completely
  typedef boost::array<char,2> Arr;
  Arr quotes = { '"', '\'' };
  Arr seps   = { ';', ',' };

  cppcsv::csv_parser<dbg_builder, Arr,Arr> cp(dbg, quotes, seps);
  std::ifstream in;
  open_check( "test_multiple_quotes.csv", in);

  in.seekg (0, in.end);
  size_t length = static_cast<size_t>(in.tellg());
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\nContext:\n%s\n", cp.error(), cp.error_context().c_str());
  }

#if (PRINT_OUT==1)
  csv_writer<file_out> dbg2(file_out(stdout),'\'',',',true);
  tbl.write(dbg2);
#endif

  delete[] buffer;
}


    printf("\n\n-- Test per-row bulk interface, with function ---\n\n");

{
  // use a boost array instead of a string
  // this should allow the compiler to unroll all of match_char loops completely
  typedef boost::array<char,2> Arr;
  Arr quotes = { '"', '\'' };
  Arr seps   = { ';', ',' };

  typedef cppcsv::csv_builder_bulk<void (*)(const char*, size_t, const size_t*, size_t)> Builder;
  Builder builder(print_bulk_row);
  cppcsv::csv_parser<Builder,Arr,Arr,char> cp(builder, quotes, seps);
  std::ifstream in;
  open_check( "test_multiple_quotes.csv", in);

  in.seekg (0, in.end);
  size_t length = static_cast<size_t>(in.tellg());
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\nContext:\n%s\n", cp.error(), cp.error_context().c_str());
  }

  delete[] buffer;
}


    printf("\n\n-- Test per-row bulk interface, with functor ---\n\n");

{
  // use a boost array instead of a string
  // this should allow the compiler to unroll all of match_char loops completely
  typedef boost::array<char,2> Arr;
  Arr quotes = { '"', '\'' };
  Arr seps   = { ';', ',' };

  print_bulk_row_t builder;
  cppcsv::csv_parser<print_bulk_row_t,Arr,Arr,char> cp(builder, quotes, seps);
  std::ifstream in;
  open_check( "test_multiple_quotes.csv", in);

  in.seekg (0, in.end);
  size_t length = static_cast<size_t>(in.tellg());
  in.seekg (0, in.beg);
  char * buffer = new char[length];
  in.read(buffer, length);
  if (in)
  {
     const char* cursor = buffer;
     if (cp(cursor, length))
        printf("ERROR: %s\nContext:\n%s\n", cp.error(), cp.error_context().c_str());
  }

  delete[] buffer;
}
  return 0;
}

