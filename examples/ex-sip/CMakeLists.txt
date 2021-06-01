# https://github.com/ldthomas/apg-7.0

# required versions
cmake_minimum_required(VERSION 3.20)
set(CMAKE_C_STANDARD 11)

# set the project name
project(EX-SIP)

# pass the source directory to the application
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
configure_file(source.h.in source.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# gcc compile-time macros (#define s)
add_compile_definitions(APG_TRACE APG_STATS)

# include the api library
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/../../json DIR_JSON)
add_library(json STATIC ${DIR_JSON})

# include the parser's library
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/../../library DIR_LIBRARY)
add_library(library STATIC ${DIR_LIBRARY})

# include the library of utilities
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/../../utilities DIR_UTILITIES)
add_library(utilities STATIC ${DIR_UTILITIES})

# define the executable source code
add_executable(ex-sip ${CMAKE_CURRENT_SOURCE_DIR}/main.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/sip-0.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sip-1.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/udtlib.c)

# include the libraries' source code
target_link_libraries(ex-sip
  json
  library
  utilities
)
