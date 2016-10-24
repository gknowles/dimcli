cmake_minimum_required (VERSION 3.6)
project (test)
set_property(SOURCE "pch.cpp" APPEND PROPERTY COMPILE_FLAGS "/Ycpch.h")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ../bin)

file(GLOB_RECURSE sources LIST_DIRECTORIES false *.h *.cpp)
update_deps_file("${sources}")

add_executable(clitest ${sources})
target_link_libraries(clitest dimcli)
source_group("" FILES ${sources})
write_user_file(clitest)

add_test(NAME clitest COMMAND clitest)
