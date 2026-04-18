cmake_minimum_required(VERSION 3.16)
project(tsk_wrapper LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_WRAPPER "Build libtusk libraries" ON)
option(BUILD_STATIC_WRAPPER "Build static libtusk library" ON)
option(BUILD_SHARED_WRAPPER "Build shared libtusk library" ON)
option(BUILD_TOOL "Build standalone binary" ON)

# Custom static toolchain location
set(FOR_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(FOR_INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

include_directories(${FOR_INC_DIR})
link_directories(${FOR_LIB_DIR})

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(LIBTUSK_STATIC_DEPS
        ${FOR_LIB_DIR}/libtsk_win.a
        ${FOR_LIB_DIR}/libz_win.a
    )
else()
    set(LIBTUSK_STATIC_DEPS
        ${FOR_LIB_DIR}/libtsk_lnx.a
        ${FOR_LIB_DIR}/libz_lnx.a
        pthread
    )
endif()

if (BUILD_SHARED_WRAPPER AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message(STATUS "BUILD_SHARED_WRAPPER disabled for Windows cross-compile")
    set(BUILD_SHARED_WRAPPER OFF)
endif()

if (BUILD_SHARED_WRAPPER)
    unset(LIBTUSK_SHARED_TSK CACHE)
    unset(LIBTUSK_SHARED_Z CACHE)
    set(LIBTUSK_ORIGINAL_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(CMAKE_FIND_LIBRARY_SUFFIXES .so)
    find_library(LIBTUSK_SHARED_TSK NAMES tsk)
    find_library(LIBTUSK_SHARED_Z NAMES z)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${LIBTUSK_ORIGINAL_FIND_LIBRARY_SUFFIXES})

    if (NOT LIBTUSK_SHARED_TSK)
        message(FATAL_ERROR "BUILD_SHARED_WRAPPER=ON requires a shared libtsk on the system")
    endif()

    if (NOT LIBTUSK_SHARED_Z)
        message(FATAL_ERROR "BUILD_SHARED_WRAPPER=ON requires a shared zlib on the system")
    endif()

    set(LIBTUSK_SHARED_DEPS
        ${LIBTUSK_SHARED_TSK}
        ${LIBTUSK_SHARED_Z}
        pthread
    )
endif()

function(configure_libtusk_target target_name)
    set(options)
    set(one_value_args)
    set(multi_value_args LINK_LIBS)
    cmake_parse_arguments(LIBTUSK "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    target_include_directories(${target_name}
        PUBLIC
            ${FOR_INC_DIR}
    )

    target_link_libraries(${target_name}
        PRIVATE
            ${LIBTUSK_LINK_LIBS}
    )
endfunction()

# -----------------------------
# Build wrapper library
# -----------------------------
if (BUILD_WRAPPER)
    if (BUILD_STATIC_WRAPPER)
        add_library(tusk_static STATIC
            libtusk.cpp
        )

        configure_libtusk_target(tusk_static LINK_LIBS ${LIBTUSK_STATIC_DEPS})

        set_target_properties(tusk_static PROPERTIES
            OUTPUT_NAME tusk
        )

        if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
            # Build a fat archive: merge libtusk wrapper + libtsk_win.a + libz_win.a
            # into one self-contained archive using ar MRI script mode.
            set(FAT_ARCHIVE "${CMAKE_CURRENT_BINARY_DIR}/libtusk_fat.a")
            set(FAT_MRI     "${CMAKE_CURRENT_BINARY_DIR}/libtusk_fat.mri")

            file(GENERATE OUTPUT "${FAT_MRI}" CONTENT
"CREATE ${FAT_ARCHIVE}
ADDLIB $<TARGET_FILE:tusk_static>
ADDLIB ${FOR_LIB_DIR}/libtsk_win.a
ADDLIB ${FOR_LIB_DIR}/libz_win.a
SAVE
END
")

            add_custom_command(TARGET tusk_static POST_BUILD
                COMMAND ${CMAKE_AR} -M < "${FAT_MRI}"
                COMMENT "Creating fat archive libtusk_fat.a"
                VERBATIM
            )
        else()
            # Build a fat archive for Linux: merge libtusk wrapper + libtsk_lnx.a + libz_lnx.a
            # into one self-contained archive using ar MRI script mode.
            set(FAT_ARCHIVE "${CMAKE_CURRENT_BINARY_DIR}/libtusk_fat.a")
            set(FAT_MRI     "${CMAKE_CURRENT_BINARY_DIR}/libtusk_fat.mri")

            file(GENERATE OUTPUT "${FAT_MRI}" CONTENT
"CREATE ${FAT_ARCHIVE}
ADDLIB $<TARGET_FILE:tusk_static>
ADDLIB ${FOR_LIB_DIR}/libtsk_lnx.a
ADDLIB ${FOR_LIB_DIR}/libz_lnx.a
SAVE
END
")

            add_custom_command(TARGET tusk_static POST_BUILD
                COMMAND ${CMAKE_AR} -M < "${FAT_MRI}"
                COMMENT "Creating fat archive libtusk_fat.a (Linux)"
                VERBATIM
            )
        endif()
    endif()

    if (BUILD_SHARED_WRAPPER)
        add_library(tusk_shared SHARED
            libtusk.cpp
        )

        configure_libtusk_target(tusk_shared LINK_LIBS ${LIBTUSK_SHARED_DEPS})
        target_compile_definitions(tusk_shared PRIVATE LIBTUSK_BUILD_SHARED)
        set_target_properties(tusk_shared PROPERTIES
            CXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN YES
        )
        target_link_options(tusk_shared PRIVATE
            LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/libtusk.map
        )

        set_target_properties(tusk_shared PROPERTIES
            OUTPUT_NAME tusk
            VERSION 0.1
            SOVERSION 1
        )
    endif()
endif()

# -----------------------------
# Build standalone binary
# -----------------------------
if (BUILD_TOOL AND NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_executable(tsktool
        tusk.cpp
    )

    if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
        target_link_libraries(tsktool
            ${FOR_LIB_DIR}/libtsk_win.a
            ${FOR_LIB_DIR}/libz_win.a
        )
        target_link_options(tsktool PRIVATE
            -static
        )
    else()
        target_link_libraries(tsktool
            ${FOR_LIB_DIR}/libtsk_lnx.a
            ${FOR_LIB_DIR}/libz_lnx.a
            pthread
        )
        target_link_options(tsktool PRIVATE
            -static
            -static-libgcc
            -static-libstdc++
        )
    endif()
endif()
