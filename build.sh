#!/bin/bash

# default variables
APG_DIR=apg
EXAMPLES_DIR=examples
BUILD_DIR_DEBUG=Debug
BUILD_DIR_RELEASE=Release
TYPE_DEBUG=-DCMAKE_BUILD_TYPE=Debug
TYPE_RELEASE=-DCMAKE_BUILD_TYPE=Release
NAMES=(ex-apgex ex-api ex-ast ex-basic ex-conv ex-format ex-json ex-lines ex-msgs ex-sip ex-trace ex-wide ex-xml)
NAMELEN=${#NAMES[@]}
FLAG_DEBUG=-d
FLAG_RELEASE=-r
HELP="--help"
SOURCE=
BUILD=
TARGET=

# modify these for different IDE or build system
IDE_DEBUG='-G "Eclipse CDT4 - Unix Makefiles"'
IDE_VERSION_DEBUG=4.16
IDE_RELEASE='Unix Makefiles'
IDE_VERSION_RELEASE=''

# release version is default
BUILD_DIR=${BUILD_DIR_RELEASE}
IDE=${IDE_RELEASE}
IDE_VERSION=${IDE_VERSION_RELEASE}
TYPE=${TYPE_RELEASE}

# display the targets and descriptions
targets(){
    echo '     target    - description'
    echo '     ------    - -----------'
    echo '     all       - build all targets'
    echo '     apg       - build apg70, the APG Version 7.0 parser generator'
    echo '     ex-apgex  - examples of using the SABNF pattern-matching engine, apgex library'
    echo "     ex-api    - examples of using the parser generator's Application Programming Interface (API) library"
    echo '     ex-ast    - examples of using the Abstract Syntax Tree (AST)'
    echo '     ex-basic  - illustrate the basics of creating and using a parser'
    echo '     ex-conv   - illustrate the use of the data conversion library'
    echo '     ex-format - illustrate the use of the data (hexdump-like) foramatting library'
    echo '     ex-json   - illustrate the use of the JSON parser library'
    echo '     ex-lines  - illustrate the lines parsing library'
    echo '     ex-msgs   - illustrate the use of the message logging library'
    echo '     ex-sip    - parsing and time test for the Session Initiation Protocol (SIP) "torture tests"'
    echo '     ex-trace  - illustrate parser tracing and parser, memory and vector statistics'
    echo '     ex-wide   - illustrate parsing of wide (32-bit) characters'
}
# display the help screen
help(){
    echo 'NAME'
    echo "     build.sh  - create executables and/or Eclipse projects for the"
    echo "                 APG 7.0 parser generator and examples of it's use"
    echo ''
    echo 'SYNOPSIS'
    echo "     ./build.sh [-r | -d |--help] target"
    echo ''
    echo 'DESCRIPTION'
    echo '     This script will generate make files for the APG 7.0 parser generator and any'
    echo "     or all of the supplied example applications."
    echo ""
    echo "     In release mode (-r), Unix Makefiles are generated and optimized executables are built."
    echo "     For a different build system, modify IDE_RELEASE and IDE_VERSION_RELEASE."
    echo ""
    echo "     In debug mode (-d) Eclipse, version 4.16, project files are generated."
    echo "     Open Eclipse and select"
    echo "         Project->import->General->Existing Projects into Workspace"
    echo "     and browse to the ./Debug directory. At this point you should be able to"
    echo "     import all of the built projects into Eclipse."
    echo "     For a different build system, modify IDE_DEBUG and IDE_VERSION_DEBUG."
    echo ""
    echo "     -r (default) - generate a release build and compile the executables"
    echo "     -d           - generate Eclipse project files for a debug build"
    echo '     --help       - print this help screen'
    echo ""
    targets
}

# generate and Eclipse target
build(){
    eval "cmake ${IDE} ${IDE_VERSION} ${TYPE} -S $1 -B $2"
    if [ ${TYPE} == ${TYPE_RELEASE} ];then
        eval "cmake --build $2"
    fi
}

# check for an argument
if [ $# -eq 0 ] || [ $1 == ${HELP} ]; then
    help
    exit 0
fi

# set up for release or debug build
if [ $1 == ${FLAG_DEBUG} ]; then
    BUILD_DIR=${BUILD_DIR_DEBUG}
    IDE=${IDE_DEBUG}
    IDE_VERSION=${IDE_VERSION_DEBUG}
    TYPE=${TYPE_DEBUG}
    TARGET=$2
    echo "Building DEBUG target ${TARGET}"
    elif [ $1 == ${FLAG_RELEASE} ]; then
    TARGET=$2
    echo "Building RELEASE target ${TARGET}"
    else
    TARGET=$1
    echo "Building RELEASE target ${TARGET}"
fi    

# the examples require an output directory
if [ ! -d ${EXAMPLES_DIR}/output ]; then
    echo "No example output directory - creating"
    echo "${EXAMPLES_DIR}/output"
    eval "mkdir ${EXAMPLES_DIR}/output"
fi


# generate Makefiles and build all example executables
if [ ${TARGET} == "all" ]; then
    build apg ${BUILD_DIR}/apg
    for (( i=0; i<${NAMELEN}; i++))
    do
        build ${EXAMPLES_DIR}/${NAMES[$i]} ${BUILD_DIR}/${NAMES[$i]}
    done
    exit 0
fi

# generate and build special case of apg70 executable
if [ ${TARGET} == "apg" ]; then
    build apg ${BUILD_DIR}/${APG_DIR}
    exit 0
fi    

# search the list of example names
# generate and build the example if found
for (( i=0; i<${NAMELEN}; i++))
do
    if [ ${NAMES[$i]} == ${TARGET} ]; then
        build ${EXAMPLES_DIR}/${TARGET} ${BUILD_DIR}/${TARGET}
        exit 0
    fi
done

# error on unrecognized argument
echo "Unrecognized target: ${TARGET}"
help
exit 1
