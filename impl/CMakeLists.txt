cmake_minimum_required (VERSION 3.20.3)
project (CppPapersImpl CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# enable MSVC specific multi-processor compilation option

if (MSVC)
   set(CMAKE_CXX_STANDARD 23)
   # prevent MSVC/clang incompatible MP flag
   add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
   set(CXX_STANDARD_REQUIRED TRUE)
	add_compile_options(-Wall -Wextra -Werror -stdlib=libc++ -std=c++2c)
	#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
   set(CXX_STANDARD_REQUIRED TRUE)
	add_compile_options(-Wall -Wextra -Werror -std=c++2b)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
endif()

#set(range-v3_DIR /Users/huixie/repos/range-v3/build)
#set(Catch2_DIR /Users/huixie/.local/lib/cmake/Catch2)

# git clone Catch2, cmake & install to $INSTALL.
# set Catch2_DIR in this project to $INSTALL/lib/cmake/Catch2
find_package(Catch2 3 REQUIRED)

# git clone range-v3, cmake & build (or install, but no need to)
# add the build root path (where there is a range-v3-config.cmake file) to CMAKE_MODULE_PATH 
find_package(range-v3 REQUIRED)
find_package(benchmark REQUIRED)


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
#  |  REF-WRAPPER  |
#  +---------------+
# for now, all in one file. ideally if things grow, consider splitting CMake logic to individual directories, via
#    add_subdirectory(ref_wrapper) etc.
file(GLOB_RECURSE ref_wrapper_hdr  RELATIVE ${CMAKE_SOURCE_DIR} ref_wrapper/*.hpp)
add_library(ref_wrapper INTERFACE) # header-only library
target_sources(ref_wrapper INTERFACE ${ref_wrapper_hdr})

#  +----------+
#  |  ANY_VIEW  |
#  +----------+
# for now, all in one file. ideally if things grow, consider splitting CMake logic to individual directories, via
#    add_subdirectory(any_view) etc.
file(GLOB_RECURSE any_view_hdr  RELATIVE ${CMAKE_SOURCE_DIR} any_view/*.hpp)
add_library(any_view INTERFACE) # header-only library
target_sources(any_view INTERFACE ${any_view_hdr})


#  +---------------+
#  |  CONCAT-TEST  |
#  +---------------+

file(GLOB_RECURSE concat_test_src RELATIVE ${CMAKE_SOURCE_DIR} CONFIGURE_DEPENDS  concat/test/*.cpp)
add_executable(concat-test)
target_sources(concat-test PRIVATE ${concat_test_src})
target_link_libraries(concat-test PRIVATE Catch2::Catch2WithMain)
target_include_directories(concat-test PRIVATE concat ref_wrapper)
target_link_libraries(concat-test PRIVATE range-v3)





#  +--------------------+
#  |  REF-WRAPPER-TEST  |
#  +--------------------+

#file(GLOB_RECURSE ref_wrapper_test_src RELATIVE ${CMAKE_SOURCE_DIR} CONFIGURE_DEPENDS  ref_wrapper/test/*.cpp)
#add_executable(ref_wrapper-test)
#target_sources(ref_wrapper-test PRIVATE ${ref_wrapper_test_src})
#target_link_libraries(ref_wrapper-test PRIVATE Catch2::Catch2WithMain)
#target_include_directories(ref_wrapper-test PRIVATE ref_wrapper)
#target_link_libraries(ref_wrapper-test PRIVATE range-v3)


#  +---------------+
#  |  ANY-VIEW-TEST  |
#  +---------------+

file(GLOB_RECURSE any_view_test_src RELATIVE ${CMAKE_SOURCE_DIR} CONFIGURE_DEPENDS  any_view/test/*.cpp)
#file(GLOB_RECURSE any_view_test_src RELATIVE ${CMAKE_SOURCE_DIR} CONFIGURE_DEPENDS  any_view/test/view/input.cpp)
add_executable(any_view-test)
target_sources(any_view-test PRIVATE ${any_view_test_src})
target_link_libraries(any_view-test PRIVATE Catch2::Catch2WithMain)
target_include_directories(any_view-test PRIVATE any_view ref_wrapper)
target_link_libraries(any_view-test PRIVATE range-v3)

#  +----------------------+
#  |  ANY-VIEW-BENCHMARK  |
#  +----------------------+
file(GLOB_RECURSE any_view_bench_src RELATIVE ${CMAKE_SOURCE_DIR} CONFIGURE_DEPENDS  any_view/benchmark/*.cpp)
add_executable(any_view-bench)
target_sources(any_view-bench PRIVATE ${any_view_bench_src})
target_include_directories(any_view-bench PRIVATE any_view )
target_link_libraries(any_view-bench PRIVATE benchmark::benchmark)
target_link_libraries(any_view-bench PRIVATE benchmark::benchmark_main)
target_compile_options(any_view-bench PRIVATE -O0)
