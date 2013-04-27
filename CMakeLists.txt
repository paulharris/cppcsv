cmake_minimum_required(VERSION 2.8)

find_package (Boost)
include_directories (${Boost_INCLUDE_DIR})

if (WIN32)
   # lots of warnings please
   set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wmissing-declarations -Wwrite-strings -Wpointer-arith -Wcast-qual -Wcast-align")
   set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wmissing-declarations -Wwrite-strings -Wpointer-arith -Wcast-qual -Wcast-align")
else (WIN32)
endif (WIN32)

include_directories (include)

add_library (cppcsv src/simplecsv.cpp)

# Note: uses csvparser as a header-only library
add_executable(tst_csv test/tst_csv.cpp test/tst_csv_2.cpp)

target_link_libraries(tst_csv cppcsv)
