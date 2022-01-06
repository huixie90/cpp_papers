cmake_minimum_required (VERSION 3.20.3)
project (CppPapersImpl CXX)

# enable MSVC specific multi-processor compilation option

if (MSVC)
   set(CMAKE_CXX_STANDARD 23)
   add_compile_options(/MP)
else()
   set(CXX_STANDARD_REQUIRED TRUE)
endif()

# git clone Catch2, cmake & install to $INSTALL.
# set Catch2_DIR in this project to $INSTALL/lib/cmake/Catch2
find_package(Catch2 3 REQUIRED)



#  +----------+
#  |  CONCAT  |
#  +----------+
# for now, all in one file. ideally if things grow, consider splitting CMake logic to individual directories, via
#    add_subdirectory(concat) etc.
file(GLOB_RECURSE concat_hdr  RELATIVE ${CMAKE_SOURCE_DIR} concat/*.hpp)
add_library(concat INTERFACE) # header-only library
target_sources(concat INTERFACE ${concat_hdr})
#target_compile_features(cppcoro PUBLIC cxx_std_20)


#  +---------------+
#  |  CONCAT-TEST  |
#  +---------------+

file(GLOB_RECURSE concat_test_src RELATIVE ${CMAKE_SOURCE_DIR} CONFIGURE_DEPENDS  concat/test/*.cpp)
add_executable(concat-test)
target_sources(concat-test PRIVATE ${concat_test_src})
target_link_libraries(concat-test PRIVATE Catch2::Catch2WithMain)
target_include_directories(concat-test PRIVATE concat)
