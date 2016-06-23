#pragma once

#include "csvbase.hpp"
#include <cassert>

namespace cppcsv {

// note: methods here are all declared non-virtual,
// if you want to inherit from a virtual Base, then use
//
// csv_writer< Output, char, csv_builder_t<char> >
//
// else, use
// csv_writer< Output, char >
// which will inherit from EmptyBaseClass and have no virtual methods at all.
//
// NOTE: Prior to Oct 2015, this inherited from csv_builder_t by default,
//       now it inherits from EmptyBaseClass by default.

struct EmptyBaseClass {};

template <typename Output, typename Char = char, class BaseClass = EmptyBaseClass >
class csv_writer : public BaseClass, public per_cell_tag {
public:
  csv_writer(Char qchar=Char('"'),Char sep=Char(','),bool smart_quote=false, size_t min_columns = 0, bool quote_quotes = true)
    : qchar(qchar),sep(sep),
      smart_quote(smart_quote),
		quote_quotes(quote_quotes),
      col(0),
      min_columns(min_columns),
      pending_seps(0),
      row_is_open(false)
  {}
  csv_writer(Output out,Char qchar= Char('"'),Char sep=Char(','),bool smart_quote=false, size_t min_columns = 0, bool quote_quotes = true)
    : out(out),
      qchar(qchar),sep(sep),
      smart_quote(smart_quote),
		quote_quotes(quote_quotes),
      col(0),
      min_columns(min_columns),
      pending_seps(0),
      row_is_open(false),
      current_row(0)
  {}

  void begin_row() {
    assert(!row_is_open);
    row_is_open = true;
    col = 0;
    pending_seps = 0;
    ++current_row;
  }
  void cell(const Char *buf, size_t len) {
    assert(row_is_open);
    if (col != 0) {
      ++pending_seps;
    }
    ++col;
    if (!buf || len == 0) {
      return;
    }

    while (pending_seps > 0)
    {
      out(&sep,1);
      --pending_seps;
    }

    if ( (qchar == 0) || ((smart_quote)&&(!need_quote(buf,len)))) {
      out(buf,len);
    } else {
      out(&qchar,1);

      const Char *pos=buf;
      while (len>0) {
        if (*pos==qchar) {
          out(buf, pos-buf+1); // first qchar
          buf=pos; // qchar still there! (second one)
        }
        pos++;
        len--;
      }
      out(buf, pos-buf);

      out(&qchar,1);
    }
  }

  // skip_newline: provided for unusual situations, where if
  // eg only one cell were printed out, we don't want a newline at the end.
  void end_row(bool skip_newline = false) {
     assert(row_is_open);
     row_is_open = false;
     static const Char newline = Char('\n');

     assert(col >= pending_seps);
     col -= pending_seps;
     while (col < min_columns)
     {
       if (col != 0)
          out(&sep,1);
       ++col;
     }

     if (!skip_newline)
       out(&newline, 1);
  }

  bool is_row_open() const { return row_is_open; }

  // call this at the end, to check for correct usage
  void finish() {
     assert(!row_is_open);
  }

  size_t get_current_row() const
  {
     return current_row;
  }

private:
  bool need_quote(const Char *buf, size_t len) const {
     assert(qchar != 0);

     static const Char space = Char(' ');
     static const Char tab = Char('\t');
     static const Char newline = Char('\n');

    // check for leading/trailing whitespace
    if (len > 0) {
      if (   (*buf == space)
          || (*buf == tab)
          || (*(buf+len-1) == space)
          || (*(buf+len-1) == tab)) {
        return true;
      }
    }

    // we need a quote if we start with a quote,
	 // even if "quote_quotes" is disabled.
    if (len > 0 && *buf == qchar)
       return true;

    // Note: readers will accept quotes in the middle of unquoted cells
    // But smartquote will always quote cells with quotes in them.

    while (len>0) {
      if (  // Note: Quoting cells that have quotes in them is desired
            //   and recommended, but not required.  We do this if enabled.
            // If you want it to output CSV like Excel's clipboard, then
				// set quote_quotes=false, but we will still quote if a cell
				// STARTS with a quote character (is confusing for Excel / LibreCalc).
            (quote_quotes && *buf==qchar) ||
            (*buf==sep) ||
            (*buf==newline) )
      {
        return true;
      }
      buf++;
      len--;
    }
    return false;
  }

private:
  Output out;
  Char qchar;
  Char sep;
  bool smart_quote;
  // optional - do not quote the cell when there is a quote character inside the cell,
  // normally you should quote such cells for CSV,
  // but you should NOT quote these cells when we are generating for a clipboard TSV.
  // Except, we WILL quote when the cell starts with a quote.  This is a cause for 
  // confusion for Excel (it drops the quotes), and LibreOffice only partially pastes
  // the text.
  bool quote_quotes;


  size_t col;
  size_t min_columns;
  size_t pending_seps;  // saves up the separators for blank cells

  bool row_is_open;
  size_t current_row;
};



// note: the CSV standard says to use DOS-CR
// https://tools.ietf.org/html/rfc4180
template <class Output, typename Char = char>
struct add_dos_cr_out {
  add_dos_cr_out( Output out ) : out(out) {}

  void operator()(const Char *buf, size_t len) {
    bool last_was_cr = false;
    const Char *pos = buf;
    static const Char cr = Char('\r');

    while (len>0) {
      if ( (*pos == '\n') && !last_was_cr ) {
        out(buf, pos-buf); // print what came before newline
        out(&cr, 1);      // print CR
        buf = pos;         // start from the newline (it'll be printed later)
      }

      last_was_cr = (*pos == cr);

      pos++;
      len--;
    }

    out(buf, pos-buf);
  }

private:
  Output out;
};




// for non-copyable outputs
template <class Ref>
class OutputRef {
public:
  Ref & out;

  OutputRef(Ref & b) : out(b) {}

  template <class Char>
  void operator()(const Char *buf, size_t len) { out(buf,len); }
};

template <class Ref>
OutputRef<Ref> make_OutputRef( Ref & r )
{
   return OutputRef<Ref>(r);
}

}
