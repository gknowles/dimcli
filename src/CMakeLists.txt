# CMakeLists.txt - dim cli source

cmake_minimum_required (VERSION 3.6)
project (dimcli)
set_property(SOURCE "pch.cpp" APPEND PROPERTY COMPILE_FLAGS "/Ycpch.h")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "../lib/$(Configuration)")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "../lib/$(Configuration)")

file(GLOB_RECURSE sources LIST_DIRECTORIES false *.h *.cpp ../include/*)
update_deps_file("${sources}")

add_library(dimcli STATIC ${sources})
source_group("" FILES ${sources})
source_group("compiler" FILES ../include/dim/compiler/visualc.h)
write_user_file(dimcli)
