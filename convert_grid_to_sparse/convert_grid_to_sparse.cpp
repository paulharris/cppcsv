#include <cppcsv/csvparser.hpp>
#include <cppcsv/csvwriter.hpp>

#include <cstdio>
#include <iostream>
#include <stdexcept>

#include <boost/lexical_cast.hpp>

using std::string;
using std::cerr;
using std::endl;
using std::logic_error;
using std::runtime_error;
using boost::lexical_cast;


static void usage( char** argv )
{
   cerr << "USAGE: " << argv[0] << " input1.csv input2.csv input3.csv ..." << endl;
}


class FileCloser
{
   FILE* fp;
public:
   FileCloser(FILE * fp) : fp(fp) {}
   ~FileCloser() { fclose(fp); }
};


class OutputFile
{
   uint64_t pos;
   FILE * fp;

   // noncopyable
   OutputFile( OutputFile const& );
   OutputFile& operator=( OutputFile const& );

public:
   OutputFile( string const& fn ) :
      pos(0),
      fp(fopen(fn.c_str(), "wb"))
   {
      if (fp == NULL)
         throw runtime_error("Could not open output file " + fn);
   }

   ~OutputFile()
   {
      fclose(fp);
   }

   // uint64_t position() const
   // {
      // return pos;
   // }

   void write( const char * buffer, size_t num )
   {
      if (num != fwrite(buffer, 1, num, fp))
         throw runtime_error("Error writing to output file");
      pos += num;
   }

   // for csvwriter to use
   void operator()(const char *buf, size_t len)
   {
      write(buf, len);
   }
};


typedef cppcsv::csv_writer< cppcsv::add_dos_cr_out< cppcsv::OutputRef<OutputFile> > > CsvWriter;


class ConvertBuilder : public cppcsv::per_cell_tag
{
   size_t col, row;
   string filename;
   CsvWriter & writer;

public:
   ConvertBuilder( string const& fn, CsvWriter & writer ) : col(0), row(0), filename(fn), writer(writer) {}

   static const size_t first_row = 11;
   static const size_t first_col = 2;

   void begin_row()
   {
      col = 1;
      ++row;
   }

   void end_row() {}

   void cell(const char* buf, size_t len)
   {
      if (col >= first_col and row >= first_row and buf)
      {
         writer.begin_row();
         writer.cell(filename.c_str(), filename.size());
         string x = lexical_cast<string>(col-first_col+1);
         string y = lexical_cast<string>(row-first_row+1);
         writer.cell(x.c_str(), x.size());
         writer.cell(y.c_str(), y.size());
         writer.cell(buf,len);
         writer.end_row();
      }
      ++col;
   }
};



int main(int argc, char **argv)
{
   try {
      if (argc < 1)
      {
         usage(argv);
         return 1;
      }

      string output_filename = "combined.csv";
      OutputFile outfile(output_filename);

      CsvWriter writer(
               cppcsv::make_OutputRef(outfile),
               '"',
               ',',
               true  // do smart quoting
               );

      writer.begin_row();
      writer.cell("Filename", 8);
      writer.cell("X", 1);
      writer.cell("Y", 1);
      writer.cell("Value", 5);
      writer.end_row();

      const size_t bufsize = 64*1024;
      char buffer[bufsize];

      for (int arg = 1; arg < argc; ++arg)
      {
         string filename = argv[arg];
         FILE* fp = fopen(filename.c_str(), "rb");
         if (not fp)
            throw runtime_error("Could not open file " + filename);
         FileCloser closer(fp);

         ConvertBuilder builder(filename, writer);

         cppcsv::csv_parser<ConvertBuilder, char, char, char> parser(
               builder, // builder
               '"',     // quotes
               ',',     // delimiters
               true,    // always trim whitespace
               false,   // collapse_delimiters,
               0,       // comment
               true,    // comments must be at the start
               true     // always collect error context
               );

         while (not feof(fp))
         {
            size_t n = fread(buffer, 1, bufsize, fp);

            if (!feof(fp) && ferror(fp))
               throw runtime_error("Error reading from file " + filename);

            const char* cursor = buffer;
            if (parser.process(cursor, n))
               throw runtime_error(
                     string("Error reading CSV ") 
                     + filename
                     + "\n" 
                     + parser.error() 
                     + "\n"
                     + "Context:\n" 
                     + parser.error_context()
                     );
         }
         // note: no need for final flush, will be done when last read of zero
      }
   }
   catch (runtime_error & err) {
      cerr << "Error: " << err.what() << endl;
      return 1;
   }
   catch (logic_error & err) {
      cerr << "LOGIC Error (report to programmer): " << err.what() << endl;
      return 1;
   }

   return 0;
}
