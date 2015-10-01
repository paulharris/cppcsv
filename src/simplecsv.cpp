#include <cppcsv/simplecsv.hpp>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

namespace cppcsv {
namespace SimpleCSV {

const std::string Row::del;

Table empty_table;
const Row Table::empty_row(empty_table,-1);
const Value Table::empty_value(empty_row,-1,std::string());

// {{{ Value
Value::Value(const Row &parent, size_t cidx,const std::string &value)
  : parent(parent),
    cidx(cidx),
    value(value)
{
}

int Value::asInt() const
{
  return atoi(value.c_str());  // TODO? error handling
}

const char *Value::asCString() const
{
  return value.c_str();
}

const std::string &Value::asString() const
{
  return value;
}
// }}}

// {{{ Row
Row::Row(Table &parent, size_t ridx)
  : parent(parent),ridx(ridx)
{
}

Row::~Row()
{
  for (std::map<size_t,Value *>::iterator it=columns.begin(),end=columns.end();it!=end;++it) {
    delete it->second;
  }
}

// TODO
const Value &Row::operator[](const char *key) const
{
  int cidx=parent.find_column(key);
  if (cidx==-1) { // TODO?! throw   (or return none; [static const Value none;] ?)
    printf("Key \"%s\" not found\n",key);
  }
  return operator[](cidx);
}

const Value &Row::operator[](size_t cidx) const // {{{
{
  if (cidx>=size()) {
    return parent.empty_value;
  }
  std::map<size_t,Value *>::const_iterator it=columns.find(cidx);
  if (it==columns.end()) {
    return parent.empty_value;
  }
  assert(it->second);
  return *it->second;
}
// }}}

size_t Row::size() const
{
  if (!columns.empty()) {  // columns.(r)begin()!=columns.(r)end()
    return columns.rbegin()->first+1; // last existing key +1
    //    "columns.back().first+1"
  }
  return 0;
//  return parent.columnnames.size();
}

void Row::set(size_t cidx,const std::string &value)
{
  if (&value==&del) {
    columns.erase(cidx);
  }
  Value *val=new Value(*this,cidx,value);
  columns.insert(std::make_pair(cidx,val));
}

void Row::dump() const // {{{
{
  printf("[%lu]:",ridx);

//  for (std::map<size_t,Value *>::iterator it=columns.begin(),end=columns.end();it!=end;++it) {
  const size_t clen=size();
  for (size_t iA=0;iA<clen;iA++) {
    printf("%s;",operator[](iA).asCString());
  }
  printf("\n");
}
// }}}

void Row::write(csv_builder &out) const // {{{
{
  out.begin_row();
  const size_t clen=size();
  for (size_t iA=0;iA<clen;iA++) {
    const Value &val=operator[](iA);
    if (&val==&parent.empty_value) {
      out.cell(NULL,0);
    } else {
      const std::string &str=val.asString(); 
      out.cell(str.data(), str.size());
    }
  }
  out.end_row();
}
// }}}

// }}}

// {{{ Table
Table::~Table()
{
  const size_t len=rows.size();
  for (size_t iA=0;iA<len;iA++) {
    delete rows[iA];
  }
}

const Row &Table::operator[](size_t ridx) const
{
  if (ridx>=size()) {
    return empty_row;
  }
  return *rows[ridx];
}

size_t Table::size() const
{
  return rows.size();
}

void Table::dump() const // {{{
{
  const size_t clen=size();
  for (size_t iA=0;iA<clen;iA++) {
    printf("%s;",columnnames[iA]->c_str());
  }
  printf("\n---\n");

  const size_t len=size();
  for (size_t iA=0;iA<len;iA++) {
    (operator[])(iA).dump();
//    rows[iA]->dump();
  }
}
// }}}

int Table::find_column(const std::string &name) const // {{{
{
  std::multimap<std::string,size_t>::const_iterator it=rev_column.find(name);
  if (it==rev_column.end()) {
    return -1;
  }
  return static_cast<int>(it->second); // only first matching element
}
// }}}

// TODO? allow rows[]==NULL  for empty_row  (created by after-the-end insertRow)

Row &Table::IBuild::newRow(Table &csv) // {{{
{
  csv.rows.push_back(new Row(csv, csv.rows.size()));
  return *csv.rows.back();
}
// }}}

Row &Table::IBuild::insertRow(Table &csv, size_t at_ridx) // {{{
{
  if (at_ridx<csv.rows.size()) { // insert
    Row *ret=new Row(csv,at_ridx);
/* replace
    delete csv.rows[at_ridx];
    csv.rows[at_ridx]=ret;
*/
    std::vector<Row *>::iterator it=csv.rows.begin();
    std::advance(it,at_ridx);
    try {
      it=++csv.rows.insert(it,ret);
    } catch (...) {
      delete ret;
      throw;
    }

    // fix Row::ridx 
    std::vector<Row *>::iterator end=csv.rows.end();
    for (;it!=end;++it) {
      (*it)->ridx++;
    }
    return *ret;
  } else { // append
    csv.rows.reserve(at_ridx+1);
    for (size_t iA=csv.rows.size();iA<=at_ridx;iA++) {
      csv.rows.push_back(new Row(csv,iA));
    }
    return *csv.rows.back();
  }
}
// }}}

void Table::IBuild::deleteRow(Table &csv, size_t ridx) // {{{
{
  if (ridx>=csv.rows.size()) {
    return; // no-op   (TODO?)
  }
  delete csv.rows[ridx];
  std::vector<Row *>::iterator it=csv.rows.begin();
  std::advance(it,ridx);
  it=csv.rows.erase(it);

  // fix Row::ridx
  std::vector<Row *>::iterator end=csv.rows.end();
  for (;it!=end;++it) {
    (*it)->ridx--;
  }
}
// }}}

// TODO? throw instead at duplicate?
void Table::IBuild::setHeader(Table &csv,const std::vector<std::string>& names) // {{{
{
  csv.columnnames.clear();
  csv.rev_column.clear();

  const size_t len = names.size();
  csv.columnnames.reserve(len);
  for (size_t iA=0;iA<len;iA++) {
    std::multimap<std::string, size_t>::iterator it=csv.rev_column.insert(std::make_pair(names[iA],iA));
    csv.columnnames.push_back(&it->first);
  }
}
// }}}

void Table::write(csv_builder &out,bool with_header) const // {{{
{
  if (with_header) {
    out.begin_row();
    const size_t clen=size();
    for (size_t iA=0;iA<clen;iA++) {
      out.cell(columnnames[iA]->c_str(),
               columnnames[iA]->size());
    }
    out.end_row();
  }

  const size_t len=size();
  for (size_t iA=0;iA<len;iA++) {
    (operator[])(iA).write(out);
  }
}
// }}}

// }}}


builder::builder(Table &result,bool first_is_header) // {{{
  : result(result),
    row(NULL),
    cidx(-1),
    as_header(first_is_header)
{
}
// }}}

void builder::begin_row() // {{{
{
  if (as_header) {
    row=NULL;
    return;
  }
  row=&Table::IBuild::newRow(result);
  cidx=0;
}
// }}}

void builder::cell(const char *buf, size_t len) // {{{
{
  if (as_header) {
//    header.emplace_back(buf,len);
    header.push_back(std::string(buf,len));
    return;
  }
  assert(row);
  row->set(cidx++,std::string(buf,len));
}
// }}}

void builder::end_row() // {{{
{
  if (as_header) {
    Table::IBuild::setHeader(result,header);
    header.clear();
    as_header=false;
  }
}

} // namespace SimpleCSV
}
