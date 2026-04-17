cmake_minimum_required(VERSION 3.16)
project(tusk LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set(INC "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(TSK_INC "O:\\tmp\\sleuthkit")
set(LIB "${CMAKE_CURRENT_SOURCE_DIR}/lib")

if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()

add_library(libtusk STATIC libtusk.cpp)
target_include_directories(libtusk PRIVATE
    ${INC} ${TSK_INC}
    $<$<CXX_COMPILER_ID:MSVC>:${CMAKE_CURRENT_SOURCE_DIR}/include/msvc_compat>
)
target_link_libraries(libtusk PRIVATE ${LIB}/libtsk.lib ${LIB}/zlib.lib)

add_executable(tsktool tusk.cpp)
target_include_directories(tsktool PRIVATE
    ${INC} ${TSK_INC}
    $<$<CXX_COMPILER_ID:MSVC>:${CMAKE_CURRENT_SOURCE_DIR}/include/msvc_compat>
)
target_link_libraries(tsktool PRIVATE ${LIB}/libtsk.lib ${LIB}/zlib.lib)