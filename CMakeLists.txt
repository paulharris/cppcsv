cmake_minimum_required(VERSION 2.8)

find_package (Boost)
include_directories (${Boost_INCLUDE_DIR})

# Note: uses csvparser as a header-only library
add_executable(tst_csv tst_csv.cpp tst_csv_2.cpp)
