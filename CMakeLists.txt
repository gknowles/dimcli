# Copyright Glen Knowles 2016 - 2022.
# Distributed under the Boost Software License, Version 1.0.
#
# CMakeLists.txt - general project generator
#
# Use this one file *unmodified* for all projects that follow the convections.
# All the careful tweaking is for Visual Studio, but it works on linux with
# both gcc and clang (at least well enough to do travis builds :P).
#
# Project layout:
#   /<project_name>
#   /<project_name>/CMakeLists.txt - this file
#   /<project_name>/CMakeDeps.cmake - made by this file (source controlled)
#   /<project_name>/conf/**
#   /<project_name>/conf/**/<basename>.sample - causes .../<basename> to be 
#       excluded from dependencies
#   /<project_name>/docs/**
#   /<project_name>/libs/<lib_project_name>/**
#   /<project_name>/libs/${PROJECT_NAME}/config.h - may be modified by the
#       install task, see "Installing" below.
#   /<project_name>/testlibs/<test_lib_project_name>/**
#   /<project_name>/tests/<test_project_name>/**
#   /<project_name>/tools/<tool_project_name>/**
#   /<project_name>/vendor/<vendor_project_name>/ - only if
#       /<project_name>/vendor/<vendor_project_name>/CMakeLists.txt exists
#
# Code files:
# Source files are expected to be "*.cpp" and "*.h".
#
# Precompiled headers:
# If a lib, test, or tool has a "pch.h" it should also have a "pch.cpp", and
# it's cpp files will (under msvc) have "/Yupch.h" - except pch.cpp which gets
# "/Ycpch.h".
#
# Building libraries:
# If a library has no cpp files a non-building custom target is made for it
# instead of a lib target (good for header only libs).
# For library builds the ${PROJECT_NAME}_LIB_SOURCE macro is defined, used in
# combination with ${PROJECT_NAME}_LIB_DYN_LINK to help mark up the interface
# with the correct dllexport/dllimport attributes.
#
# Testing:
# CTest will run all tests, and pass them the "--test" argument. The testlibs
# contain compile time tests that are never run or linked to anything else.
#
# Installing:
# By default nothing is installed. Installation of libs, tools, and tests
# are enabled using the INSTALL_LIBS, INSTALL_TOOLS, and INSTALL_TESTS options
# respectively. Install creates the following (vcpkg conventions):
#   include - subdir for each lib containing all of its headers except those
#             like "*int.h"
#   include/${PROJECT_NAME}/config.h - if it (and they) exist, the following
#           lines are commented/uncommented according to the selected options.
#       "#define ${PROJECT_NAME}_LIB_DYN_LINK" - BUILD_SHARED_LIBS
#       "#define ${PROJECT_NAME}_LIB_WINAPI_FAMILY_APP - WINDOWS_STORE
#   bin - only release build targets
#       libs with BUILD_SHARED_LIBS: *.dll and *.pdb
#       libs otherwise: no files
#       tools and tests: *.exe and *.pdb
#   lib - only release build targets
#       libs with BUILD_SHARED_LIBS: *.lib
#       libs otherwise: *.lib and *.pdb
#   debug/bin - like bin above, but debug targets
#   debug/lib - like lib above, but debug targets
#
# Vendor submodules:
# add_subdirectory is called for every vendor project directory that has
# a CMakeLists.txt file. It is expected to be a compatible version of this
# file, and only it's lib projects will be included in the root solution.

cmake_minimum_required(VERSION 3.6)
option(BUILD_PROJECT_NAME "Override name of project to build" OFF)
option(LINK_STATIC_RUNTIME "Link with static c++ runtime" ON)
option(BUILD_SHARED_LIBS "Build DLLs instead of static libs" OFF)
option(BUILD_UNICODE "Build with Unicode charset" OFF)
option(BUILD_TESTING "Enable test generation for ctest" ON)
option(BUILD_COVERAGE "Enable coverage profiling (only with GCC)" OFF)
option(INSTALL_LIBS "Install includes libs" OFF)
option(INSTALL_TOOLS "Install includes tool binaries" OFF)
option(INSTALL_TESTS "Install includes test binaries" OFF)

# Reference to suppress "not used" warning when manually specified
set(tmp "${BUILD_COVERAGE}")        # only used by clang and gcc builds
set(tmp "${CMAKE_BUILD_TYPE}")      # not used (vcpkg sets it)
set(tmp "${LINK_STATIC_RUNTIME}")   # not used by clang or gcc builds


#############################################################################
#
#   Compiler and general project settings
#
####

get_property(parent DIRECTORY PROPERTY PARENT_DIRECTORY)
if("${parent}" STREQUAL "")
    set(ROOT_PROJECT true)
else()
    set(ROOT_PROJECT false)
endif()

if(BUILD_PROJECT_NAME AND ROOT_PROJECT)
    project(${BUILD_PROJECT_NAME})
else()
    # Get name of project from containing directory. Strip version suffixes
    # such as "-1.2.3-2" from the name.
    get_filename_component(prjname "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
    string(REGEX REPLACE "(-[\.0-9\-]*)$" "" prjname ${prjname})
    project(${prjname})
endif()
string(TOUPPER "${PROJECT_NAME}" DEFINE_PREFIX)

# OUT_OF_TREE is 0 if binary dir is a subdirectory of source dir, else
# non-zero.
string(FIND "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/"
    OUT_OF_TREE)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER cmake)

set(CMAKE_CONFIGURATION_TYPES Debug Release)
if(NOT OUT_OF_TREE)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)
endif()

if(MSVC)
    # "/EHsc" - exceptions from c++ only (not from structured or "C")
    # "/GF" - string pooling
    # "/GR-" (UNUSED) - disable RTTI
    # "/guard:cf" (UNUSED) - control flow guard
    # "/nologo" - suppress startup banner and informational messages
    # "/std:c++latest" - allow constructs still pending standardization
    # "/utf-8" - sets source code character set to utf-8
    # "/W4" - warning level 4
    # "/WX" - treat warnings as errors
    # "/Zc:inline" - all inline functions must have definition available
    # "/Zc:rvalueCast" - enforce type conversion rules
    # "/Zc:strictStrings" - disable string literal type conversion to non-const
    # "/Fa[dir]" - assembly listing location, the CMake default puts it right
    #    under the solution tree, and we want it in under /build
    set(CMAKE_CXX_FLAGS "\
        /EHsc /GF /nologo /std:c++latest /utf-8 /W4 /WX \
        /Zc:inline /Zc:rvalueCast /Zc:strictStrings \
        /Fa$(IntDir)")
    if(NOT MSVC_VERSION LESS 1910)
        # /permissive-  // reject non-conforming backward compatibility-isms
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive-")
    endif()

    # "/Od" - disables optimization
    set(CMAKE_CXX_FLAGS_DEBUG "/Od")
    if(FALSE AND NOT MSVC_VERSION LESS 1928)
        # "/fsanitize=address" - enables ASAN memory error detector
        # "/Zi" - debug info (not present would be no info)
        set(CMAKE_CXX_FLAGS_DEBUG
            "${CMAKE_CXX_FLAGS_DEBUG} /fsanitize=address /Zi")
    else()
        # "/RTC1" - runtime checks (stack frame over/under run and uninit use)
        # "/ZI" - debug with edit and continue (disables some optimizations)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /RTC1 /ZI")
    endif()
    if(LINK_STATIC_RUNTIME)
        # "/MTd" - multithread debug static runtime
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    else()
        # "/MDd" - multithread debug dll runtime
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
    endif()

    # "/O2" - maximize speed
    # "/Zi" - debug info format (not present would be no info)
    set(CMAKE_CXX_FLAGS_RELEASE "/DNDEBUG /O2 /Zi")
    if(LINK_STATIC_RUNTIME)
        # "/MT" - multithread release static runtime
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    else()
        # "/MD" - multithread release dll runtime
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
    endif()
elseif(CMAKE_COMPILER_IS_GNUCXX)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "8")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")
    elseif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "11")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2b")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "7")
        # The libstdc++6 library, as of 2021-06-15, no longer works with
        # Clang++-6 in c++2a mode, failing with a reference to the unknown
        # std::is_constant_evaluated().
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")
    elseif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "12")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2b")
    endif()
endif()
if(BUILD_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID STREQUAL GNU
            OR CMAKE_CXX_COMPILER_ID STREQUAL Clang)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
            -fprofile-arcs -ftest-coverage -fsanitize=address")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} \
            -fprofile-arcs")
    endif()
endif()

if(BUILD_UNICODE)
    add_definitions(/D_UNICODE)
endif()
if(BUILD_COVERAGE)
    add_definitions(/D${DEFINE_PREFIX}_LIB_BUILD_COVERAGE)
endif()

# set list of defines that need to go in config.h
if(BUILD_SHARED_LIBS)
    list(APPEND config_defines ${DEFINE_PREFIX}_LIB_DYN_LINK)
endif()
if(WINDOWS_STORE)
    list(APPEND config_defines ${DEFINE_PREFIX}_LIB_WINAPI_FAMILY_APP)
endif()
foreach(var ${config_defines})
    add_definitions(/D${var})
endforeach()

# Setting source_group to a single space is weird and relies on Visual Studio
# treating it as no group, ideally we'd set it to the empty string to mean
# no group, but - just for regexes (bug?) - that causes CMake to use defaults.
source_group(" " REGULAR_EXPRESSION ".*")
source_group("ci" REGULAR_EXPRESSION "\.github/.*\.yml")
source_group("ci" FILES appveyor.yml codecov.yml .travis.yml)

if(ROOT_PROJECT)
    include_directories(libs)
endif()


#############################################################################
#
#   Vendor submodules
#
####

if(ROOT_PROJECT)
    file(GLOB allnames vendor/*)
    foreach(var ${allnames})
        if(IS_DIRECTORY "${var}")
            list(APPEND deps "${var}")
            get_filename_component(prjname "${var}" NAME)
            if(EXISTS "${var}/CMakeLists.txt")
                list(APPEND submodules "${var}")
                include_directories(${var}/libs)
            endif()
        endif()
    endforeach()
    foreach(var ${submodules})
        add_subdirectory("${var}")
    endforeach()
endif()


#############################################################################
#
#   Functions for generating local projects
#
#   Because function definitions are global the ones defined for use in this
#   file (to generate local projects) are defined after all vendor subprojects
#   have been included, otherwise they could be replaced by same named 
#   functions defined by the vendor cmake scripts.
#
####

# Compare the new contents with the existing file, if it exists and is the
# same we don't want to trigger a make by changing its timestamp.
function(update_file path content)
    set(old_content "")
    if(EXISTS "${path}")
        file(READ "${path}" old_content)
    endif()
    if(NOT old_content STREQUAL content)
        file(WRITE "${path}" "${content}")
    endif()
endfunction()

# Creates a file called CMakeDeps.cmake next to the CMakeLists.txt with
# the list of dependencies in it - this file should be treated as part of
# CMakeLists.txt (source controlled, etc.).
function(update_deps_file)
    set(deps_file "CMakeDeps.cmake")
    # Normalize the list so it's the same on every machine.
    foreach(dep ${ARGV})
        get_filename_component(dep "${dep}" ABSOLUTE)
        file(RELATIVE_PATH rel_dep ${CMAKE_CURRENT_SOURCE_DIR} ${dep})
        list(APPEND all_rel_deps "${rel_dep}")
    endforeach()
    list(REMOVE_DUPLICATES all_rel_deps)
    set(rel_deps ${all_rel_deps})
    foreach(dep ${all_rel_deps})
        # Remove files where the same file exists but with an additional
        # ".sample" extension.
        get_filename_component(match "${dep}" LAST_EXT)
        if(match STREQUAL ".sample")
            get_filename_component(dir "${dep}" DIRECTORY)
            get_filename_component(name ${dep} NAME_WLE)
            if (dir)
                set(name "${dir}/${name}")
            endif()
            message(STATUS "${name} dependency skipped because ${dep} exists.")
            list(REMOVE_ITEM rel_deps "${name}")
        endif()
    endforeach()
    list(SORT rel_deps)
    # Build the content
    set(content "# Generated by make process\n")
    foreach(dep IN LISTS rel_deps)
        string(APPEND content "# ${dep}\n")
    endforeach()
    # Update the deps file
    update_file("${deps_file}" "${content}")
    # Include the file so it's tracked as a generation dependency; we don't
    # need the content.
    include(${deps_file})
endfunction()

# Available options:
#   "debug" - include default debugger settings
#   "props" - import global and project specific *.user.props files
function(write_user_file prjname opts)
    if(NOT MSVC)
        return()
    endif()
    set(user_file "${CMAKE_CURRENT_BINARY_DIR}/${prjname}.vcxproj.user")
    file(WRITE ${user_file} [=[
<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="4.0"
        xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
]=])
    list(FIND opts debug hasOpt)
    if(NOT ${hasOpt} EQUAL -1)
        foreach(cfgname ${CMAKE_CONFIGURATION_TYPES})
            if(${cfgname} STREQUAL Release)
                set(libpath "lib")
            else()
                set(libpath "${cfgname}/lib")
            endif()
            set(cfgplat "${cfgname}|${MSVC_CXX_ARCHITECTURE_ID}")
            file(APPEND ${user_file} [=[
    <PropertyGroup Condition="'$(Configuration)|$(Platform)'==']=] "${cfgplat}" [=['">
        <LocalDebuggerWorkingDirectory>$(TargetDir)</LocalDebuggerWorkingDirectory>
        <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
        <VcpkgLibPath>]=] "${libpath}" [=[</VcpkgLibPath>
    </PropertyGroup>
]=])
        endforeach()
    endif()
    list(FIND opts props hasOpt)
    if(NOT ${hasOpt} EQUAL -1)
        if(TARGET ${prjname})
            get_target_property(srcdir ${prjname} SOURCE_DIR)
            file(GLOB props LIST_DIRECTORIES false
                "${srcdir}/*.user.props"
                "${CMAKE_SOURCE_DIR}/*.user.props")
        else()
            file(GLOB props LIST_DIRECTORIES false
                "${CMAKE_SOURCE_DIR}/*.user.props")
        endif()
        foreach(propfile ${props})
            file(APPEND ${user_file} [=[
    <Import Project="]=] "${propfile}" [=["/>
]=])
        endforeach()
    endif()
    file(APPEND ${user_file} [=[
</Project>
]=])
endfunction()

function(install_archive_pdb tgt dstdir)
    if(NOT MSVC)
        # do nothing
    elseif(BUILD_SHARED_LIBS)
        install(FILES "$<TARGET_PDB_FILE:${tgt}>" DESTINATION bin)
    else()
        # Special handling required to get PDBs of static libraries installed.
        # I would hope that there's a better way to do this, but I don't know
        # what it may be...
        if(MSVC_VERSION LESS 1920)
            set(src "${CMAKE_CURRENT_BINARY_DIR}/${tgt}.dir")
        else()
            set(src "${CMAKE_CURRENT_BINARY_DIR}")
        endif()
        foreach(cfg ${CMAKE_CONFIGURATION_TYPES})
            install(
                FILES "${src}/${cfg}/${tgt}.pdb"
                DESTINATION ${dstdir}
                CONFIGURATIONS ${cfg})
        endforeach()
    endif()
endfunction()

function(set_pch tgt srcdir)
    if(MSVC AND EXISTS "${srcdir}/pch.h")
        # use pch.cpp to create precompiled header
        set_property(SOURCE "${srcdir}/pch.cpp"
            APPEND PROPERTY COMPILE_FLAGS "/Ycpch.h")
        # use pch.h as precompiled header
        set_property(TARGET ${tgt} APPEND PROPERTY COMPILE_FLAGS "/Yupch.h")
    endif()
endfunction()

function(add_exec_project tgt srcdir)
    file(GLOB_RECURSE sources LIST_DIRECTORIES false ${srcdir}/*)
    list(FILTER sources EXCLUDE REGEX ".*\.aps$")
    list(APPEND deps ${sources})
    set(deps ${deps} PARENT_SCOPE)
    add_executable(${tgt} ${sources})
    set_pch(${tgt} ${srcdir})
    if(MSVC)
        set_target_properties(${tgt} PROPERTIES LINK_FLAGS /Debug)
    endif()
    foreach(var ${libnames})
        get_filename_component(libname "${var}" NAME)
        target_link_libraries(${tgt} ${libname})
    endforeach()
    if(CMAKE_COMPILER_IS_GNUCXX 
            AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9")
        # required for #include <experimental/filesystem>
        target_link_libraries(${tgt} "stdc++fs")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL Clang
            AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9")
        target_link_libraries(${tgt} "stdc++fs")
    endif()
    write_user_file(${tgt} "debug;props")
endfunction()

function(add_lib_project tgt srcdir)
    file(GLOB_RECURSE sources LIST_DIRECTORIES false ${srcdir}/*)
    list(FILTER sources EXCLUDE REGEX ".*\.aps$")
    list(APPEND deps ${sources})
    set(deps ${deps} PARENT_SCOPE)
    set(cpps ${sources})
    list(FILTER cpps INCLUDE REGEX ".*\.cpp$")
    list(LENGTH cpps cpps)
    if(cpps)
        add_library(${tgt} ${sources})
        target_compile_definitions(${tgt} PRIVATE ${DEFINE_PREFIX}_LIB_SOURCE)
        list(APPEND libnames ${tgt})
        set(libnames ${libnames} PARENT_SCOPE)

        if(INSTALL_LIBS)
            install(TARGETS ${tgt}
                ARCHIVE DESTINATION lib
                RUNTIME DESTINATION bin)
            install_archive_pdb(${tgt} lib)
        endif()
    else()
        add_custom_target(${tgt} SOURCES ${sources})
        set_target_properties(${tgt} PROPERTIES EXCLUDE_FROM_ALL false)
    endif()
    set_target_properties(${tgt} PROPERTIES FOLDER libs)
    set_pch(${tgt} ${srcdir})
    if(INSTALL_LIBS)
        file(GLOB incls LIST_DIRECTORIES false
            RELATIVE "${srcdir}" "${srcdir}/*.h")
        foreach(var ${incls})
            get_filename_component(name "${var}" NAME)
            if(NOT name MATCHES ".*int\.h")
                install(FILES "${srcdir}/${var}" DESTINATION include/${tgt})
            endif()
        endforeach()
    endif()
    write_user_file(${tgt} "debug;props")
endfunction()

function(add_file_project tgt)
    set(sources ${ARGN})
    list(LENGTH sources num)
    if(num)
        list(APPEND deps ${sources})
        set(deps ${deps} PARENT_SCOPE)
        add_custom_target(${tgt} SOURCES ${sources})
        set_target_properties(${tgt} PROPERTIES
            FOLDER files
            EXCLUDE_FROM_ALL false)
    endif()
endfunction()


#############################################################################
#
#   Local projects
#
####

# lib targets
file(GLOB allnames libs/*)
foreach(var ${allnames})
    if(IS_DIRECTORY "${var}")
        get_filename_component(prjname "${var}" NAME)
        if(NOT prjname STREQUAL ${PROJECT_NAME})
            set(prjname ${PROJECT_NAME}-${prjname})
        endif()
        add_lib_project(${prjname} "${var}")
    endif()
endforeach()

if(NOT ROOT_PROJECT)
    set(deps ${deps} PARENT_SCOPE)
    set(libnames ${libnames} PARENT_SCOPE)
    return()
endif()

# file targets
file(GLOB sources
    LICENSE
    .clang-format
    configure.bat
    *.adoc
    *.md
    *.props
    *.rst
    *.yml
)
file(GLOB_RECURSE allnames .github/*.yml)
list(APPEND sources ${allnames})
if(sources)
    add_file_project(about ${sources})
endif()

file(GLOB_RECURSE sources conf/*)
if(sources)
    add_file_project(conf ${sources})
endif()

file(GLOB_RECURSE sources docs/*)
if(sources)
    add_file_project(docs ${sources})
endif()

# tool targets
file(GLOB allnames tools/*)
foreach(var ${allnames})
    if(IS_DIRECTORY "${var}")
        get_filename_component(prjname "${var}" NAME)
        add_exec_project(${prjname} "${var}")
        set_target_properties(${prjname} PROPERTIES FOLDER tools)
        if(INSTALL_TOOLS)
            install(TARGETS ${prjname} RUNTIME DESTINATION bin)
        endif()
    endif()
endforeach()

# test targets
if(BUILD_TESTING)
    enable_testing()
endif()
file(GLOB allnames tests/*)
foreach(var ${allnames})
    if(IS_DIRECTORY "${var}")
        get_filename_component(prjname "${var}" NAME)
        add_exec_project(${prjname} "${var}")
        target_compile_definitions(${prjname}
            PRIVATE ${DEFINE_PREFIX}_LIB_KEEP_MACROS)
        set_target_properties(${prjname} PROPERTIES FOLDER tests)
        add_test(NAME ${prjname} COMMAND ${prjname} --test)
        if(INSTALL_TESTS)
            install(TARGETS ${prjname} RUNTIME DESTINATION bin)
        endif()
    endif()
endforeach()

# test lib targets - use for compile only tests, never linked
file(GLOB allnames testlibs/*)
foreach(var ${allnames})
    if(IS_DIRECTORY "${var}")
        get_filename_component(prjname "${var}" NAME)
        add_lib_project(${prjname} "${var}")
        set_target_properties(${prjname} PROPERTIES FOLDER testlibs)
    endif()
endforeach()

# update deps file
update_deps_file(${deps})

# Create modified config.h to reflect build defines
set(cfgname "libs/${PROJECT_NAME}/config.h")
get_filename_component(path ${cfgname} ABSOLUTE)
if(EXISTS "${path}")
    file(READ "${path}" content)
    foreach(val ${config_defines})
        string(REGEX REPLACE
            "\n//#define ${val}" "\n#define ${val}"
            content "${content}")
    endforeach()
    update_file("${CMAKE_CURRENT_BINARY_DIR}/${cfgname}" "${content}")

    if(INSTALL_LIBS)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${cfgname}"
            DESTINATION include/${PROJECT_NAME})
    endif()
endif()

# Add settings for predefined cmake targets
foreach(tgt ALL_BUILD PACKAGE ZERO_CHECK INSTALL RUN_TESTS)
    write_user_file("${tgt}" "props")
endforeach()

include(CPack)
