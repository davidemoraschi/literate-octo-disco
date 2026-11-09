cmake_minimum_required(VERSION 3.20)
project(sqlbot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SQLBOT_ENABLE_SQLITE3 "Enable SQLite3 native connector" OFF)

add_executable(sqlbot
    src/main.cpp
    src/app.cpp
    src/mcp_client.cpp
    src/db_connector/sqlite_connector.cpp
    src/db_connector/sqlserver_connector.cpp
    src/db_connector/bigquery_connector.cpp
    src/utils/config.cpp
    src/utils/string_utils.cpp
)

target_include_directories(sqlbot PRIVATE src)

target_compile_definitions(sqlbot PRIVATE _HAS_CXX23=1)

if (MSVC)
        # Optimization flags are release-only: /O2 conflicts with the Debug
    # defaults (/Od /RTC1) that CMake adds, so they must not be unconditional.
    target_compile_options(sqlbot PRIVATE
        /W4
        $<$<CONFIG:Release,RelWithDebInfo,MinSizeRel>:/O2 /GL /Gy /Gw>)
    target_link_options(sqlbot PRIVATE
        $<$<CONFIG:Release,RelWithDebInfo,MinSizeRel>:/LTCG /OPT:REF /OPT:ICF>)
    set_property(TARGET sqlbot PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
else()
    target_compile_options(sqlbot PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(sqlbot PRIVATE
        $<$<CONFIG:Release,RelWithDebInfo,MinSizeRel>:-O2>)
endif()

if (WIN32)
    target_link_libraries(sqlbot PRIVATE ws2_32 advapi32 odbc32)
endif()

if (SQLBOT_ENABLE_SQLITE3)
    find_path(SQLITE3_INCLUDE_DIR sqlite3.h)
    find_library(SQLITE3_LIBRARY sqlite3)
    if (NOT SQLITE3_INCLUDE_DIR OR NOT SQLITE3_LIBRARY)
        message(FATAL_ERROR "SQLBOT_ENABLE_SQLITE3=ON but sqlite3 was not found")
    endif()
    target_compile_definitions(sqlbot PRIVATE SQLBOT_ENABLE_SQLITE3=1)
    target_include_directories(sqlbot PRIVATE ${SQLITE3_INCLUDE_DIR})
    target_link_libraries(sqlbot PRIVATE ${SQLITE3_LIBRARY})
endif()

if (WIN32)
    target_compile_definitions(sqlbot PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
endif()
