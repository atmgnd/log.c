cmake_minimum_required(VERSION 3.1)

# ref: stackoverflow(How to set Visual Studio Filters for nested sub directory using cmake)
function(assign_source_group)
    foreach(_source IN ITEMS ${ARGN})
        if (IS_ABSOLUTE "${_source}")
            file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
        else()
            set(_source_rel "${_source}")
        endif()
        get_filename_component(_source_path "${_source_rel}" PATH)
        string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
        source_group("${_source_path_msvc}" FILES "${_source}")
    endforeach()
endfunction(assign_source_group)

set (XXX_PROJECT_NAME logc)

project(${XXX_PROJECT_NAME})
set (CMAKE_BUILD_TYPE Debug)
if(ANDROID)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/asr/lib/${ANDROID_ABI})
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/asr/lib/${ANDROID_ABI})
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/asr/bin/${ANDROID_ABI})
endif()

# CMAKE_<LANG>_COMPILER_ID
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # using Clang
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_C_FLAGS "-std=c99 -Wall  -D_GNU_SOURCE ${CMAKE_CXX_FLAGS}")
    link_libraries(pthread)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# target_
# include_directories
# link_directories

# add_subdirectory()

# RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
file (GLOB_RECURSE logcsource
    src/log.c
)

file (GLOB_RECURSE logcheaders
    src/log.h
)

file (GLOB_RECURSE demosource
    src/demo.c
)

set (XXX_PROJECT_LOGC logc)
add_library(${XXX_PROJECT_LOGC} STATIC ${logcsource} ${logcheaders})

set (XXX_PROJECT_DEMO demo)
add_executable(${XXX_PROJECT_DEMO} ${demosource} ${logcheaders})
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    target_link_libraries(${XXX_PROJECT_DEMO} PUBLIC logc ws2_32)
else()
    target_link_libraries(${XXX_PROJECT_DEMO} PUBLIC logc)
endif()


assign_source_group(${logcsource})
assign_source_group(${logcheaders})
assign_source_group(${demosource})

# include_directories("${PROJECT_BINARY_DIR}")
# cmake -G"Visual Studio 14"
# msbuild /P:Configuration=Release INSTALL.vcxproj













