#!/bin/bash

# define some variables

#cmake -DCMAKE_BUILD_TYPE=Release -S ex-basic -B builds/ex-basic-release
APG_DIR=apg
APG=apg70
EXAMPLES_DIR=examples
BUILD_DIR=Release
TYPE=-DCMAKE_BUILD_TYPE=Release
NAMES=(ex-apgex ex-api ex-ast ex-basic ex-conv ex-format ex-json ex-lines ex-msgs ex-sip ex-trace ex-wide ex-xml)
NAMELEN=${#NAMES[@]}
CLEAN_FLAG=0
HELP=--help
CLEAN=--clean

# display the targets and descriptions
targets(){
    echo '     target    - description'
    echo '     ------    - -----------'
    echo '     --help    - print this help screen'
    echo '     all       - build all targets'
    echo '     apg       - build apg70, the APG Version 7.0 parser generator'
    echo '     ex-apgex  - illustrates the use of the SABNF pattern-matching engine, apgex library'
    echo "     ex-api    - illustrates the use of the parser generator's Application Programming Interface (API) library"
    echo '     ex-ast    - illustrates the use of the Abstract Syntax Tree (AST)'
    echo '     ex-basic  - illustrates the basics of creating and using a parser'
    echo '     ex-conv   - illustrates the use of the data conversion library'
    echo '     ex-format - illustrates the use of the data (hexdump-like) foramatting library'
    echo '     ex-json   - illustrates the use of the JSON parser library'
    echo '     ex-lines  - illustrates the lines parsing library'
    echo '     ex-msgs   - illustrates the use of the message logging library'
    echo '     ex-sip    - parsing and time test for the Session Initiation Protocol (SIP) "torture tests"'
    echo '     ex-trace  - illustrates parser tracing and parser, memory and vector statistics'
    echo '     ex-wide   - illustrates parsing of wide (32-bit) characters'
}
# display the help screen
help(){
    echo 'NAME'
    echo "     build-release.sh  - build executables for the APG 7.0 parser generator and the examples of it's use"
    echo ''
    echo 'SYNOPSIS'
    echo "     ./build-release.sh target [--clean]"
    echo ''
    echo 'DESCRIPTION'
    echo '     Use this script to build executables for the APG 7.0 parser generator and any of the'
    echo "     supplied example applications. After running this script for any target,"
    echo "     that target may be invoked with"
    echo ""
    echo "     ${BUILD_DIR}/target/target"
    echo ""
    echo "     or with"
    echo ""
    echo "     ./run.sh target"
    echo ""
    echo "     Note: For ${APG} a better solution is to copy the executable to a PATH directory"
    echo "           and then invoke it directly from the command line. e.g."
    echo "           sudo cp ${BUILD_DIR}/${APG_DIR}/${APG} /usr/local/bin"
    echo "           ${APG} --version"
    echo ""
    echo "     --clean   - if present, the target will be cleaned rather than built."
    echo ""
    targets
}

# generate a Makefile and build the target
build(){
    if [ ${CLEAN_FLAG} -eq 1 ]; then
        eval "cmake --build $2 --target clean" 
    else
        eval "cmake ${TYPE} -S $1 -B $2"
        eval "cmake --build $2"
    fi
}

# check for an argument
if [ $# -eq 0 ] || [ $1 == ${HELP} ]; then
    help
    exit 0
fi

# check for an output directory
if [ ! -d ${EXAMPLES_DIR}/output ]; then
    echo "No example output directory. Creating the following relative to the repository directory."
    echo "${EXAMPLES_DIR}/output"
    eval "mkdir ${EXAMPLES_DIR}/output"
fi

# build or clean
if [ $# -gt 1 ] && [ $2 == ${CLEAN} ]; then
    CLEAN_FLAG=1; else
    CLEAN_FLAG=0
fi

# generate Makefiles and build all example executables
if [ $1 == "all" ]; then
    build apg ${BUILD_DIR}/apg
    for (( i=0; i<${NAMELEN}; i++))
    do
        build ${EXAMPLES_DIR}/${NAMES[$i]} ${BUILD_DIR}/${NAMES[$i]}
    done
    exit 0
fi

# generate and build special case if apg70 executable
if [ $1 == "apg" ]; then
    build apg ${BUILD_DIR}/${APG_DIR}
    exit 0
fi    

# search the list of example names
# generate and build the example if found
for (( i=0; i<${NAMELEN}; i++))
do
    if [ ${NAMES[$i]} == $1 ]; then
        build ${EXAMPLES_DIR}/$1 ${BUILD_DIR}/$1
        exit 0
    fi
done

# error on unrecognized argument
echo "unrecognized target: $1"
echo ""
targets
exit 1
