cmake_minimum_required (VERSION 3.20.3)
project (CppPapersImpl CXX)

# enable MSVC specific multi-processor compilation option

if (MSVC)
   set(CMAKE_CXX_STANDARD 23)
   # prevent MSVC/clang incompatible MP flag
   add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)
else()
   set(CMAKE_CXX_STANDARD 23)
   set(CXX_STANDARD_REQUIRED TRUE)
   add_compile_options(-Wall -Wextra -Werror -stdlib=libc++)
   set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")

endif()

# git clone Catch2, cmake & install to $INSTALL.
# set Catch2_DIR in this project to $INSTALL/lib/cmake/Catch2
find_package(Catch2 3 REQUIRED)

# git clone range-v3, cmake & build (or install, but no need to)
# add the build root path (where there is a range-v3-config.cmake file) to CMAKE_MODULE_PATH 
find_package(range-v3 REQUIRED)


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
target_link_libraries(concat-test PRIVATE range-v3)
