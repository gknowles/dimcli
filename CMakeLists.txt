cmake_minimum_required (VERSION 3.6)
project (dimcli)
include_directories(include)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO bin)
set(CMAKE_CONFIGURATION_TYPES Debug RelWithDebInfo)

function(write_user_file prjname)
    set(user_file "${CMAKE_CURRENT_BINARY_DIR}/${prjname}.vcxproj.user")
    file(WRITE ${user_file} [=[
<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
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

set(sln_path ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.sln)
file(RELATIVE_PATH prefix ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
file(READ ${sln_path} sln_content)
string(REGEX REPLACE 
    "\"([^\"\.]+).vcxproj\""
    "\"${prefix}/\\1.vcxproj\"" 
    sln_content
    ${sln_content})
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.sln ${sln_content})

