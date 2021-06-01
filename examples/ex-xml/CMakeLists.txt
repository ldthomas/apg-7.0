# https://github.com/ldthomas/apg-7.0

# required versions
cmake_minimum_required(VERSION 3.20)
set(CMAKE_C_STANDARD 11)

# set the project name
project(EX-XML)

# pass the source directory to the application
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
configure_file(source.h.in source.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# include the api library
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/../../xml DIR_XML)
add_library(xml STATIC ${DIR_XML})

# include the parser's library
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/../../library DIR_LIBRARY)
add_library(library STATIC ${DIR_LIBRARY})

# include the library of utilities
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/../../utilities DIR_UTILITIES)
add_library(utilities STATIC ${DIR_UTILITIES})

# define the executable source code
add_executable(ex-xml ${CMAKE_CURRENT_SOURCE_DIR}/main.c)

# include the libraries' source code
target_link_libraries(ex-xml
  xml
  library
  utilities
)
