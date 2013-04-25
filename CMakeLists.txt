cmake_minimum_required(VERSION 2.8)

find_package (Boost)
include_directories (${Boost_INCLUDE_DIR})

add_executable(tst_csv csvparser.cpp tst_csv.cpp simplecsv.cpp)
