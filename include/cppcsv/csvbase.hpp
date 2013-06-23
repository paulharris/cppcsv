#pragma once

namespace cppcsv {

// basic virtual interface for catching the begin/cell/end emissions.
// You can avoid the virtual calls by templating csvparser on your own builder.
template <class Char>
class csv_builder_t { // abstract base
public:
  virtual ~csv_builder_t() {}

  virtual void begin_row() {}
  virtual void cell( const Char *buf, int len ) =0;  // buf can be NULL
  virtual void end_row() {}

  // DO NOT OVERRIDE, this is an
  // Optional interface that csvparser supports,
  // but other existing csv_builder clients do not.
  void end_full_row(
        Char* buffer,
        unsigned int num_cells,
        const unsigned int * offsets,  // array[num_cells+1]
        unsigned int file_row          // row where these cells started
        )
  {
     this->end_row();
  }
};

typedef csv_builder_t<char> csv_builder;


// This adaptor allows you to use csvparser on a emit-per-row basis
// The function attached will be called once per row.
// This is NOT designed to be inherited from.
//
// Note, the cell() and end_full_row() accepts non-const buffer :)

template <class Function>
class csv_builder_bulk {
public:
  Function function;

  csv_builder_bulk(Function func) : function(func) {}

  void begin_row() {}   // does nothing
  void cell( char *buf, int len ) {}   // does nothing

  // this calls the attached function
  // note that there is (num_cells+1) offsets,
  // the last offset is the one-past-the-end index of the last cell
  void end_full_row(
        char* buffer,
        unsigned int num_cells,
        const unsigned int * offsets, // array[num_cells+1]
        unsigned int file_row          // row where these cells started
        )
  {
    function(buffer, num_cells, offsets, file_row);
  }
};

}
