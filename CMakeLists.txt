cmake_minimum_required (VERSION 3.6)
project (dimcli)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CONFIGURATION_TYPES Debug RelWithDebInfo)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO bin)

include_directories(include)
# add_compile_options(/std:c++latest)

function(write_user_file prjname)
    set(user_file "${CMAKE_CURRENT_BINARY_DIR}/${prjname}.vcxproj.user")
    file(WRITE ${user_file} [=[
<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="4.0" 
        xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
]=])
    foreach(cfgname ${CMAKE_CONFIGURATION_TYPES})
        file(APPEND ${user_file} [=[
    <PropertyGroup Condition="'$(Configuration)|$(Platform)'==']=] "${cfgname}" [=[|x64'">
        <LocalDebuggerWorkingDirectory>$(TargetDir)</LocalDebuggerWorkingDirectory>
        <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
    </PropertyGroup>
]=])
    endforeach(cfgname)
    file(APPEND ${user_file} [=[
</Project>
]=])
endfunction(write_user_file)

set(meta_sources 
    .clang-format
    appveyor.yml
    LICENSE
    README.md
    docs/README.md)
add_custom_target("_meta" SOURCES ${meta_sources})
source_group("docs" FILES docs/README.md)

set(dimcli_sources 
    include/cli.h include/util.h
    include/config.h include/compiler/visualc.h
    src/cli.cpp src/pch.cpp src/pch.h)
add_library(dimcli ${dimcli_sources})
source_group("" FILES ${dimcli_sources})
source_group("compiler" FILES include/compiler/visualc.h)
write_user_file(dimcli)

set(clitest_sources test/clitest.cpp test/pch.cpp test/pch.h)
add_executable(clitest ${clitest_sources})
target_link_libraries(clitest dimcli)
source_group("" FILES ${clitest_sources})
write_user_file(clitest)

# The solution file isn't generated until after this script finishes, 
# which means that:
#   - it might not exist (if this is the first run)
#   - you need to run cmake twice to ensure any new solution was copied
set(sln_binpath ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.sln)
if(EXISTS ${sln_binpath})
    # Load solution file from bin-dir and change the relative references to 
    # project files so that the in memory copy is as if it had been built in 
    # the source dir.
    file(RELATIVE_PATH prefix 
        ${CMAKE_CURRENT_SOURCE_DIR} 
        ${CMAKE_CURRENT_BINARY_DIR})
    file(READ ${sln_binpath} sln_content)
    string(REGEX REPLACE 
        "\"([^\"]+).vcxproj\""
        "\"${prefix}/\\1.vcxproj\"" 
        sln_content
        "${sln_content}")

    # Compare the updated contents with the existing source path sln, if it
    # exists and is the same we don't want to disturb VS by touching it.
    set(sln_srcpath ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.sln)
    set(old_content "")
    if(EXISTS ${sln_srcpath})
        file(READ ${sln_srcpath} old_content)
    endif()
    if(NOT old_content STREQUAL sln_content)
        file(WRITE ${sln_srcpath} ${sln_content})
    endif()
endif()
