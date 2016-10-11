cmake_minimum_required (VERSION 3.6)
project (test)

file(GLOB_RECURSE sources LIST_DIRECTORIES false *.h *.cpp)
update_deps_file("${sources}")

add_executable(clitest ${sources})
target_link_libraries(clitest dimcli)
source_group("" FILES ${sources})
write_user_file(clitest)
