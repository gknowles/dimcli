# CMakeLists.txt - dim cli source

cmake_minimum_required (VERSION 3.6)
project (dimcli)
set_property(SOURCE "pch.cpp" APPEND PROPERTY COMPILE_FLAGS "/Ycpch.h")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG Debug)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO RelWithDebInfo)

file(GLOB_RECURSE sources LIST_DIRECTORIES false *.h *.cpp ../include/*)
update_deps_file("${sources}")

add_library(dimcli ${sources})
source_group("" FILES ${sources})
source_group("compiler" FILES ../include/dim/compiler/visualc.h)
write_user_file(dimcli)

install(TARGETS dimcli
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin)

# Special handling required to get PDBs of static libraries installed
# I would hope that there's a better way to do this, but I don't know what 
# it may be...
if(NOT BUILD_SHARED_LIBS)
    get_property(archout TARGET dimcli 
        PROPERTY ARCHIVE_OUTPUT_DIRECTORY_${CMAKE_BUILD_TYPE})
    set(archout "${CMAKE_CURRENT_BINARY_DIR}/${archout}")
    install(FILES "${archout}/dimcli.pdb" DESTINATION lib)
endif()

install(DIRECTORY ../include DESTINATION include)
