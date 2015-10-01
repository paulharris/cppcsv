#pragma once

#include <map>
#include <vector>
#include "nocase.hpp"
#include "csvbase.hpp"

namespace cppcsv {
namespace SimpleCSV {


   // interface that SimpleCSV expects to use
class csv_builder : public per_cell_tag { // abstract base
public:
  virtual ~csv_builder() {}

  virtual void begin_row() = 0;
  virtual void cell( const char *buf, size_t len ) = 0;  // buf can be NULL
  virtual void end_row() = 0;
};




class Table;
class Row;
class Value {
public:
  int asInt() const;
  const char *asCString() const;
  const std::string &asString() const;

private:
  friend class Table;
  friend class Row;
  Value(const Row &parent, size_t cidx,const std::string &value);

  Value(const Value&); // = delete
  Value &operator=(const Value &);
private:
  // parent, cidx  are currently unused
  const Row &parent;
  size_t cidx;

  std::string value;
};

class Row {
  Row(const Row&); // = delete
  Row &operator=(const Row &);
public:
  ~Row();

  const Value &operator[](const char *key) const;
  const Value &operator[](size_t cidx) const;

  size_t size() const;

  void dump() const;
public:
  // special case: &value==&del
  void set(size_t cidx, const std::string &value);  // non-const !

  static const std::string del; // sentinel
private:
  friend class Table;
  Row(Table &parent, size_t ridx);
  void write(csv_builder &out) const;
private:
  Table &parent;
  size_t ridx;

  std::map<size_t,Value *> columns;  // TODO: not by *? (need move?)
};

class Table {
  Table(const Table&); // = delete
  Table &operator=(const Table &);
public:
  Table() {} // = default
  ~Table();

  const Row &operator[](size_t ridx) const;
  size_t size() const;

  void dump() const;
public:
  class IBuild {
  public:
    static Row &newRow(Table &csv);
    static Row &insertRow(Table &csv, size_t at_ridx); // 0<at_ridx<=size();   // replaces! (TODO? real insert?)
    static void deleteRow(Table &csv, size_t ridx);  // (moves elements!)

    static void setHeader(Table &csv,const std::vector<std::string>& names);
  };
  friend class IBuild;

  void write(csv_builder &out,bool with_header=false) const; // TODO header_if_not_empty?
private:
  friend class Row;
  int find_column(const std::string &name) const;

  static const Row empty_row;      // sentinel
  static const Value empty_value;  // sentinel
private:
  std::multimap<std::string, size_t,lt_nocase_str> rev_column;
  std::vector<const std::string *> columnnames;
  std::vector<Row *> rows;  // CPP11:  std::unique_ptr!   OR  by-value(Row moveable)?
};

class builder : public per_cell_tag {
public:
  builder(Table &result,bool first_is_header=false);
  void begin_row();
  void cell(const char *buf, size_t len);
  void end_row();
private:
  Table &result;
  Row *row;
  size_t cidx;
  bool as_header;
  std::vector<std::string> header;
};

} // namespace SimpleCSV
}
