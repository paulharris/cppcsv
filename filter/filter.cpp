#ifndef _MSC_VER
#  define _FILE_OFFSET_BITS 64
#endif

#include <cppcsv/csvparser.hpp>
#include <cppcsv/csvwriter.hpp>

#include <cassert>
#include <cerrno>
#include <cstdio>

#include <utility>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

using cppcsv::csv_parser;
using cppcsv::csv_writer;

using std::pair;
using std::vector;
using std::string;
using std::ostringstream;

using std::distance;
using std::find;
using std::reverse;

using std::logic_error;
using std::runtime_error;

using std::cerr;
using std::cout;
using std::endl;



#ifdef _MSC_VER  // Microsoft C++
#  define fseek64 _fseeki64
#  define ftell64 _ftelli64
#else
#  define fseek64 fseeko
#  define ftell64 ftello
#endif



static bool str_equal( const char* a, size_t a_len, const char* b, size_t b_len )
{
   return a_len == b_len && strncmp(a, b, a_len)==0;
}

static bool str_equal( const char* a, const char* b, size_t b_len )
{
   return str_equal( a, strlen(a), b, b_len );
}


static size_t column_ascii_2_index( const char* str, size_t len )
{
   if (len > 3)
      throw runtime_error("Invalid spreadsheet column " + string(str,len));
   size_t colidx = 0;
   for (size_t i = 0; i < len; ++i)
   {
      char c = str[i];
	  int cidx = static_cast<int>(c - 'A');
      if (cidx < 0 || cidx >= 26)
         throw runtime_error("Invalid spreadsheet column " + string(str,len));
      colidx = colidx*26 + static_cast<size_t>(cidx + 1);
   }
   --colidx;
   return colidx;
}



/*
static string index_2_column_ascii(size_t col)
{
   string colstr;
   ++col;
   do {
      colstr.push_back( static_cast<char>('A' + (col-1) % 26) );
      col = (col - ((col-1)%26)) / 26;
   } while (col);
   std::reverse(colstr.begin(),colstr.end());
   return colstr;
}
*/



class ConfigBuilder : public cppcsv::per_cell_tag
{
   enum State {
      Begin,
      AddFilenameToRow,
      FilesHaveHeader,
      InputHeader,
      ExcludeBlank,
      ExcludeText,
      FilterMin,
      FilterMax,
      ColumnOrder,
      OutputHeader,
      End
   };

   State state;
   size_t column;

public:
   ConfigBuilder() :
      state(Begin),
      column(0),
      files_have_header(false),
      add_filename_to_row(false),
      has_input_headers(false)
   {}

   typedef pair<size_t, double> FilterNumber;
   typedef pair<size_t, string> FilterText;

   bool files_have_header;
   bool add_filename_to_row;

   bool has_input_headers;
   vector<string> input_headers;

   vector<size_t> exclude_blanks;                  // remembers columns to exclude
   vector< pair<size_t, string> > exclude_texts;   // remembers column --> text
   vector< pair<size_t, double> > filter_mins;     // remembers column --> limit
   vector< pair<size_t, double> > filter_maxs;     // remembers column --> limit

   vector<size_t> output_order;  // as many as they like. -1 --> output a blank column
   vector<string> output_header; // as many as output_header


   void begin_row()
   {
      column = 0;
   }




   void cell(const char* buf, size_t len)
   {
      const char* begin = buf;
      const char* end = buf+len;

      // transition state based on first column's value
      if (column == 0)
      {
         switch (state)
         {
            case Begin:
               if (!str_equal("Add Filename To Row", buf, len))
                  throw runtime_error("Line should start with 'Add Filename To Row'");
               state = AddFilenameToRow;
               break;

            case AddFilenameToRow:
               if (!str_equal("Files Have Header", buf, len))
                  throw runtime_error("Line should start with 'Files Have Header'");
               state = FilesHaveHeader;
               break;

            case FilesHaveHeader:
               if (str_equal("Input Header", buf, len))
               {
                  state = InputHeader;
                  has_input_headers = true;
               }
               else if (str_equal("Exclude Blank", buf, len))
                  state = ExcludeBlank;
               else
                  throw runtime_error("Line should start with 'Input Header', or 'Exclude Blank'");
               break;

            case InputHeader:
               if (!str_equal("Exclude Blank", buf, len))
                  throw runtime_error("Line should start with 'Exclude Blank'");
               state = ExcludeBlank;
               break;

            case ExcludeBlank:
               if (!str_equal("Exclude Text", buf, len))
                  throw runtime_error("Line should start with 'Exclude Text'");
               state = ExcludeText;
               break;

            case ExcludeText:
               if (str_equal("Exclude Text", buf, len))
                  break;   // stay in this state
               if (!str_equal("Filter Min", buf, len))
                  throw runtime_error("Line should start with 'Filter Min'");
               state = FilterMin;
               break;

            case FilterMin:
               if (!str_equal("Filter Max", buf, len))
                  throw runtime_error("Line should start with 'Filter Max'");
               state = FilterMax;
               break;

            case FilterMax:
               if (!str_equal("Column Order", buf, len))
                  throw runtime_error("Line should start with 'Column Order'");
               state = ColumnOrder;
               break;

            case ColumnOrder:
               if (!str_equal("Output Header", buf, len))
                  throw runtime_error("Line should start with 'Output Header'");
               state = OutputHeader;
               break;

            case OutputHeader:
               throw logic_error("Should never start in OutputHeader state");

            case End:
               throw runtime_error("Found extra stuff after the last line");
         }
      }

      else
      {
         size_t header_column = column-1;

         switch (state)
         {
            case Begin:
               throw logic_error("bad state: Begin");

            case AddFilenameToRow:
               add_filename_to_row = str_equal("TRUE", buf, len);
               break;

            case FilesHaveHeader:
               files_have_header = str_equal("TRUE", buf, len);
               break;

            case InputHeader:
               input_headers.push_back( string(begin,len) );
               break;

            case ExcludeBlank:
               if (len > 0)
               {
                  if (has_input_headers && header_column >= input_headers.size())
                     throw runtime_error("Too many Exclude Blank entries");
                  if (str_equal("TRUE", buf, len))
                     exclude_blanks.push_back(header_column);
               }
               break;

            case ExcludeText:
               if (len > 0)
               {
                  if (has_input_headers && header_column >= input_headers.size())
                     throw runtime_error("Too many Exclude Text entries");
                  exclude_texts.push_back( FilterText(header_column, string(begin,len)) );
               }
               break;

            case FilterMin:
               if (len > 0)
               {
                  if (has_input_headers && header_column >= input_headers.size())
                     throw runtime_error("Too many FilterMin entries");

                  char* parsed = const_cast<char*>(end);
                  errno = 0;
                  filter_mins.push_back( FilterNumber(header_column, strtod(begin, &parsed)) );
                  if (parsed != end || errno)
                     throw runtime_error("Error parsing FilterMin number '" + string(begin,len) + "'");
               }
               break;

            case FilterMax:
               if (len > 0)
               {
                  if (has_input_headers && header_column >= input_headers.size())
                     throw runtime_error("Too many FilterMax entries");

                  char* parsed = const_cast<char*>(end);
                  errno = 0;
                  filter_maxs.push_back( FilterNumber(header_column, strtod(begin, &parsed)) );
                  if (parsed != end || errno)
                     throw runtime_error("Error parsing FilterMin number '" + string(begin,len) + "'");
               }
               break;

            case ColumnOrder:
               {
                  if (len == 0)
                     output_order.push_back(-1);
                  else if (!has_input_headers)
                  {
                     // if no InputHeaders at all, then use the spreadsheet name styling
                     output_order.push_back( column_ascii_2_index(begin, len) );
                  }
                  else
                  {
                     string h(begin, len);
                     vector<string>::iterator found = find(input_headers.begin(), input_headers.end(), h);
                     if (found == input_headers.end())
                        throw runtime_error("Could not find Input Header named '" + h + "'");
                     output_order.push_back( distance(input_headers.begin(), found) );
                  }
               }
               break;

            case OutputHeader:
               output_header.push_back( string(begin, len) );
               break;

            case End:
               throw runtime_error("Found extra stuff after the last line");
         }
      }

      ++column;
   }



   void end_row()
   {
      // special move: transition to End after OutputHeader
      if (state == OutputHeader)
         state = End;
   }



   void ensure_loaded()
   {
      assert(has_input_headers || input_headers.empty());

      if (state != End)
         throw runtime_error("Config file did not have all the fields");
   }
};










/*
class FixedString
{
public:
   const char* begin;
   const char* end;
   FixedString( const char* s ) : begin(s), end(str+strlen(s)) {}
};


// for csv_parser to work with FixedString
static const char* begin( FixedString const& s )
{
   return s.begin;
}

static const char* end( FixedString const& s )
{
   return s.end;
}
*/


class InputFile
{
   uint64_t pos;
   FILE * fp;

   // noncopyable
   InputFile( InputFile const& );
   InputFile& operator=( InputFile const& );

public:
   InputFile( const char* fn ) :
      pos(0),
      fp( fopen(fn, "rb") )
   {
      if (fp == NULL)
         throw runtime_error("Could not open input file " + string(fn));
   }

   ~InputFile()
   {
      fclose(fp);
   }

   uint64_t position() const
   {
      return pos;
   }

   uint64_t size()
   {
      int64_t pos = ftell64(fp);
      fseek64(fp, 0, SEEK_END);
      int64_t s = ftell64(fp);
      fseek64(fp, pos, SEEK_SET);
      return s;
   }

   size_t read( char * buffer, size_t num )
   {
      size_t n = fread(buffer, 1, num, fp);
      pos += n;

      if (n > 0)
         return n;

      if (!feof(fp) && ferror(fp))
      {
         ostringstream err;
         err << "Error reading from file at position " << pos;
         throw runtime_error(err.str());
      }

	 return 0;
   }
};




class OutputFile
{
   uint64_t pos;
   FILE * fp;

   // noncopyable
   OutputFile( OutputFile const& );
   OutputFile& operator=( OutputFile const& );

public:
   OutputFile( const char* fn ) :
      pos(0),
      fp(NULL)
   {
      if (FILE * test = fopen(fn, "rb"))
      {
         fclose(test);
         throw runtime_error("Output file already exists, will not overwrite for safety. Aborting.");
      }

      fp = fopen(fn, "wb");
      if (fp == NULL)
         throw runtime_error("Could not open output file " + string(fn));
   }

   ~OutputFile()
   {
      fclose(fp);
   }

   uint64_t position() const
   {
      return pos;
   }

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




typedef csv_writer< cppcsv::add_dos_cr_out< cppcsv::OutputRef<OutputFile> > > CsvWriter;


// note: this is NOT derived from csv_builder, so we skip all the virtual calls entirely
class FilterBuilder : public cppcsv::per_row_tag
{
   ConfigBuilder const& config;
   CsvWriter & out;
   bool first_row;
   vector<size_t> map_header_to_file;
   string filename;

public:
   FilterBuilder( ConfigBuilder const& config, CsvWriter & out, string const& filename ) :
      config(config),
      out(out),
      first_row(true),
      filename(filename)
   {
   }


   void end_full_row( char* buffer, size_t num_cells, const size_t * offsets, size_t file_row )
   {
      if (first_row && config.files_have_header)
      {
         // index what we see from the file
         vector<string> cells;
         for (size_t i = 0; i != num_cells; ++i)
            cells.push_back( string( buffer+offsets[i], offsets[i+1]-offsets[i] ) );

		 size_t num_exp = config.input_headers.size();

         // match the file header to the expected headers
         // for each expected header, store the column index from the file
         map_header_to_file.resize(num_exp);

         // find matching header
         for (size_t i = 0; i != num_exp; ++i)
         {
            string const& header = config.input_headers[i];

            size_t j = 0;
            for (; j != cells.size(); ++j)
               if (header == cells[j])
                  break;

            if (j == cells.size())
            {
               ostringstream err;
               err << "Could not find header '" << header << "' in file which has headers:" << endl;
               for (size_t k = 0; k != cells.size(); ++k)
                  err << cells[k] << endl;

               throw runtime_error(err.str());
            }

            map_header_to_file[i] = j;
         }
      }

      // ignore empty rows
      else if (num_cells > 0)
      {
         // does this line pass the filter?
         bool pass = true;

         // exclude blanks...
         for (size_t i = 0; pass && i != config.exclude_blanks.size(); ++i)
         {
            size_t col = config.exclude_blanks[i];
            if (config.files_have_header)
               col = map_header_to_file[col];
            pass = col < num_cells && (offsets[col+1]-offsets[col]) > 0;
         }

         // exclude texts...
         for (size_t i = 0; pass && i != config.exclude_texts.size(); ++i)
         {
            size_t col = config.exclude_texts[i].first;
            if (config.files_have_header)
               col = map_header_to_file[col];

            if (col < num_cells)
            {
               size_t len = offsets[col+1]-offsets[col];
               string const& ex = config.exclude_texts[i].second;
               pass = !str_equal(buffer+offsets[col], len, ex.c_str(), ex.size());
            }
         }

         // filter min...
         for (size_t i = 0; pass && i != config.filter_mins.size(); ++i)
         {
            size_t col = config.filter_mins[i].first;
            if (config.files_have_header)
               col = map_header_to_file[col];

            if (col < num_cells)
            {
               size_t len = offsets[col+1]-offsets[col];
               double thresh = config.filter_mins[i].second;

               const char* begin = buffer+offsets[col];
               const char* end = begin+len;

               char* parsed = const_cast<char*>(end);
               errno = 0;
               double val = strtod(begin, &parsed);
               if (parsed != end || errno)
                  throw runtime_error("Error parsing number '" + string(begin,len) + "'");

               pass = (thresh <= val);
            }
         }

         // filter max...
         for (size_t i = 0; pass && i != config.filter_maxs.size(); ++i)
         {
            size_t col = config.filter_maxs[i].first;
            if (config.files_have_header)
               col = map_header_to_file[col];

            if (col < num_cells)
            {
               size_t len = offsets[col+1]-offsets[col];
               double thresh = config.filter_maxs[i].second;

               const char* begin = buffer+offsets[col];
               const char* end = begin+len;

               char* parsed = const_cast<char*>(end);
               errno = 0;
               double val = strtod(begin, &parsed);
               if (parsed != end || errno)
                  throw runtime_error("Error parsing number '" + string(begin,len) + "'");

               pass = (val <= thresh);
            }
         }


         if (pass)
         {
            out.begin_row();

            if (config.add_filename_to_row)
               out.cell( filename.c_str(), filename.size() );

            size_t i = 0;
            for (; i != config.output_order.size(); ++i)
            {
               // which input column do we want to output
               size_t j = config.output_order[i];

               // if we are matching headers, then figure out which FILE header we want
               if (config.files_have_header)
                  j = map_header_to_file[j];

               if (j >= 0 && j < num_cells)
                  out.cell( buffer+offsets[j], offsets[j+1]-offsets[j] );
               else
                  out.cell( NULL, 0 ); // be correct and specify ALL the columns
            }

            for (; i < config.output_header.size(); ++i)
               out.cell( NULL, 0 ); // be correct and specify ALL the columns

            out.end_row();
         }
      }

      first_row = false;
   }
};



class Output_to_Stdout
{
public:
   void operator()( const char* str, size_t len )
   {
      cout.write(str, len);
   }
};



typedef cppcsv::csv_writer< Output_to_Stdout, char > Csv_to_Stdout;


class ExtractHeaderBuilder : public cppcsv::per_cell_tag
{
   Csv_to_Stdout & out;

public:
   struct done_exception {};  // thrown when header seen - for early cancelling

   ExtractHeaderBuilder( Csv_to_Stdout & out ) : out(out) {}

   void begin_row()
   {
      out.begin_row();
   }

   void cell( const char* str, size_t len )
   {
      out.cell(str,len);
   }

   void end_row()
   {
      out.end_row();
      throw done_exception();
   }
};



template <class Parser>
void ensure_csv_ok( const char* filename, Parser & parser )
{
   if (parser.error())
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



// out is used for printing output file position
template <class Builder>
void parse_csv_file( const char* filename, Builder & builder, OutputFile const* out )
{
   cppcsv::csv_parser<Builder, char, char, char> parser(
         builder, // builder
         '"',     // quotes
         ',',     // delimiters
         true,    // always trim whitespace
         false,   // collapse_delimiters,
         '#', // comment
         true,    // comments must be at the start
         true
         );   // always collect error context

   static const size_t buffer_size = 64*1024;   // 64k buffer
   char buffer[buffer_size];

   InputFile in(filename);
   uint64_t in_size = in.size();
   if (out)
      cout << filename << "  " << (in_size/1024/1024) << " MB" << endl;

   if (in_size > 0)
   {
      try {
         uint64_t print_pos = 0;
         while (true)
         {
            size_t num_read = in.read(buffer, buffer_size);

            uint64_t mb_pos = in.position() / (1024*1024); // integer truncation

            if (out && (print_pos < mb_pos || num_read == 0))  // every megabyte
            {
               print_pos = mb_pos;
               cout << "\r" << static_cast<int>((100.0*in.position())/in_size) << " %    "
                  << mb_pos << " MB"
                  << " --> " << (out->position()/1024/1024) << " MB";
               cout.flush();
            }

            const char* cursor = buffer;
            parser.process(cursor, num_read);
            ensure_csv_ok( filename, parser );

            if (num_read == 0)
               break;
         }

         if (out)
         {
            cout << "\r                                          \r";
            cout.flush();
         }
      }
      catch (...) {
         // ensure the last progress printout can still be seen
         if (out)
            cout << endl;
         throw;
      }
   }
}



struct usage_exception {};

static void usage( char** argv )
{
   cerr << "USAGE: " << argv[0] << " FILTER config_file output_file input_file1 input_file2 input_file3 ..." << endl;
   cerr << endl;
   cerr << "or USAGE: " << argv[0] << " HEADERS input_file1 input_file2 input_file3 ..." << endl;
}


int main(int argc, char **argv)
{
   try {
      if (argc < 2)
      {
         usage(argv);
         return 1;
      }


      if (string("HEADERS") == argv[1])
      {
         cout << "Printing headers from CSV files" << endl;

         Csv_to_Stdout out('"', ',', true);
         ExtractHeaderBuilder printer(out);

         for (int arg = 2; arg < argc; ++arg)
         {
            try {
               cout << argv[arg] << endl;
               parse_csv_file( argv[arg], printer, NULL );
            }
            catch (ExtractHeaderBuilder::done_exception &) {
               // use exceptions to escape early from the parsing
            }
         }

         return 0;
      }


      else if (string("FILTER") == argv[1])
      {
         if (argc < 5)
            throw usage_exception();

         // load config
         ConfigBuilder config;
         cout << "Loading config file" << endl;
         parse_csv_file( argv[2], config, NULL );
         config.ensure_loaded();

         cout << "Opening output file " << argv[3] << endl;

         OutputFile outfile(argv[3]);

         CsvWriter outcsv(
               cppcsv::make_OutputRef(outfile),
               '"',
               ',',
               true  // do smart quoting
               );

         // write the header, IF there is any Output Headers specified
         if (!config.output_header.empty())
         {
            outcsv.begin_row();

            if (config.add_filename_to_row)
               outcsv.cell( "Filename", 8 );

            size_t i = 0;
            for (; i != config.output_header.size(); ++i)
               outcsv.cell( config.output_header[i].c_str(), config.output_header[i].size() );
            for (; i < config.output_order.size(); ++i)
               outcsv.cell(NULL, 0);

            outcsv.end_row();
         }

         // begin the stream
         for (int arg = 4; arg < argc; ++arg)
         {
            FilterBuilder filter( config, outcsv, argv[arg] );
            parse_csv_file( argv[arg], filter, &outfile );
         }

         cout << "Wrote " << outfile.position()/1024/1024 << " MB" << endl;

         return 0;
      }

      else
      {
         throw usage_exception();
      }
   }
   catch (usage_exception &) {
      usage(argv);
      return 1;
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
