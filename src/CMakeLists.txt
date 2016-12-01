# CMakeLists.txt - dim cli source

cmake_minimum_required (VERSION 3.6)
project (dimcli)
set_property(SOURCE "pch.cpp" APPEND PROPERTY COMPILE_FLAGS "/Ycpch.h")

file(GLOB_RECURSE sources LIST_DIRECTORIES false *.h *.cpp ../include/*)
update_deps_file("${sources}")

add_library(dimcli ${sources})
source_group("" FILES ${sources})
source_group("compiler" FILES ../include/dim/compiler/visualc.h)
write_user_file(dimcli)

# Special handling required to get PDBs of static libraries installed
# I would hope that there's a better way to do this, but I don't know what 
# it may be...
function(install_archive_pdb tgt dstdir)
    foreach(cfg ${CMAKE_CONFIGURATION_TYPES})
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${tgt}.dir/${cfg}/${tgt}.pdb" 
            DESTINATION ${dstdir} 
            CONFIGURATIONS ${cfg})
    endforeach()
endfunction()

install(TARGETS dimcli
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin)
if(NOT BUILD_SHARED_LIBS)
    install_archive_pdb(dimcli lib)
endif()
install(DIRECTORY ../include DESTINATION include)
