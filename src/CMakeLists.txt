cmake_minimum_required (VERSION 3.6)
project (dimcli)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ../lib)

file(GLOB_RECURSE sources LIST_DIRECTORIES false *.h *.cpp ../include/*)
update_deps_file("${sources}")

add_library(dimcli ${sources})
source_group("" FILES ${sources})
source_group("compiler" FILES ../include/dim/compiler/visualc.h)
write_user_file(dimcli)
