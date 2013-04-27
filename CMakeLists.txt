cmake_minimum_required(VERSION 2.8)

message (STATUS "Note: this is mostly a header-only library, you probably don't need to build it.")

# NOTE: Only static library is built at this time,
# because dll export options have not been setup for SimpleCSV
# And if you just want csvparser or csvwriter, you do not need to build this.

# some build options
# option(CPPCSV_SHARED "Build shared lib" YES)
# option(CPPCSV_STATIC "Build static lib" YES)
option(CPPCSV_TESTS "Build tests" YES)

# find boost
find_package (Boost)
include_directories (${Boost_INCLUDE_DIR})

# some compiler options
if (WIN32)
   # lots of warnings please
   set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wmissing-declarations -Wwrite-strings -Wpointer-arith -Wcast-qual -Wcast-align")
   set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wmissing-declarations -Wwrite-strings -Wpointer-arith -Wcast-qual -Wcast-align")
else (WIN32)
endif (WIN32)

# basic install for headers
install (FILES
   include/cppcsv/csvbase.hpp
   include/cppcsv/csvparser.hpp
   include/cppcsv/csvwriter.hpp
   include/cppcsv/nocase.hpp
   include/cppcsv/simplecsv.hpp
   DESTINATION include/cppcsv)

# build the library
include_directories (include)

# static library
# if (CPPCSV_STATIC)
   add_library (cppcsv STATIC src/simplecsv.cpp)

   install (TARGETS cppcsv
      ARCHIVE DESTINATION lib
      LIBRARY DESTINATION lib
      RUNTIME DESTINATION bin
      )
# endif (CPPCSV_STATIC)


# shared library
# if (CPPCSV_SHARED)
#    add_library (cppcsv src/simplecsv.cpp)
#
#    install (TARGETS cppcsv
#       ARCHIVE DESTINATION lib
#       LIBRARY DESTINATION lib
#       RUNTIME DESTINATION bin
#       )
# endif (CPPCSV_SHARED)


# tests
if (CPPCSV_TESTS)
   add_executable(test_csv test/test_csv.cpp test/test_csv_2.cpp)

   target_link_libraries(test_csv cppcsv)

   install (TARGETS test_csv
      ARCHIVE DESTINATION lib
      LIBRARY DESTINATION lib
      RUNTIME DESTINATION bin
      )
endif (CPPCSV_TESTS)
