project (cppcsv CXX)
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

   set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS")

   # lots of link-time goodness
   set (CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE} /GL")
   set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL")

   set (CMAKE_C_FLAGS_RELWITHDEBINFO   "${CMAKE_C_FLAGS_RELWITHDEBINFO} /GL")
   set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /GL")

   set (EXTRA_LINK_FLAGS "/LTCG")
   
   set (CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} ${EXTRA_LINK_FLAGS}")
   set (CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} ${EXTRA_LINK_FLAGS}")
   set (STATIC_LIBRARY_FLAGS_RELEASE "${STATIC_LIBRARY_FLAGS_RELEASE} ${EXTRA_LINK_FLAGS}")

   set (CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO} ${EXTRA_LINK_FLAGS}")
   set (CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} ${EXTRA_LINK_FLAGS}")
   set (STATIC_LIBRARY_FLAGS_RELWITHDEBINFO "${STATIC_LIBRARY_FLAGS_RELWITHDEBINFO} ${EXTRA_LINK_FLAGS}")

else (WIN32)

   # lots of warnings please
   set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wmissing-declarations -Wwrite-strings -Wpointer-arith -Wcast-qual -Wcast-align")
   set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wmissing-declarations -Wwrite-strings -Wpointer-arith -Wcast-qual -Wcast-align")

endif (WIN32)

# basic install for headers
set (HEADERS
   include/cppcsv/csvbase.hpp
   include/cppcsv/csvparser.hpp
   include/cppcsv/csvwriter.hpp
   include/cppcsv/nocase.hpp
   include/cppcsv/simplecsv.hpp)

install (FILES ${HEADERS}
   DESTINATION include/cppcsv)

# build the library
include_directories (include)

# static library
# if (CPPCSV_STATIC)
add_library (cppcsv STATIC src/simplecsv.cpp ${HEaDERS})

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


# A CSV filter/combiner program
add_executable(csv_filter filter/filter.cpp ${HEADERS})
target_link_libraries(csv_filter cppcsv)
install (TARGETS csv_filter
   ARCHIVE DESTINATION lib
   LIBRARY DESTINATION lib
   RUNTIME DESTINATION bin
   )


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
