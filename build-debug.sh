#!/bin/bash

# define some variables

#cmake -DCMAKE_BUILD_TYPE=Release -S ex-basic -B builds/ex-basic-release
APG_DIR=apg
EXAMPLES_DIR=examples
BUILD_DIR=Debug
TYPE=-DCMAKE_BUILD_TYPE=Debug
NAMES=(ex-apgex ex-api ex-ast ex-basic ex-conv ex-format ex-json ex-lines ex-msgs ex-sip ex-trace ex-wide ex-xml)
NAMELEN=${#NAMES[@]}

# modify these for other IDE or version
IDE='-G "Eclipse CDT4 - Unix Makefiles"'
IDE_VERSION=-DCMAKE_ECLIPSE_VERSION=4.16

# display the targets and descriptions
targets(){
    echo '     target    - description'
    echo '     ------    - -----------'
    echo '     --help    - print this help screen'
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
    echo "     build-debug.sh  - create Eclipse projects for the APG 7.0 parser generator and the examples of it's use"
    echo ''
    echo 'SYNOPSIS'
    echo "     ./build-debug.sh target"
    echo ''
    echo 'DESCRIPTION'
    echo '     Use this script to generate Eclipse projects for the APG 7.0 parser generator and any of the'
    echo "     supplied example applications. After running this script, in the Eclipse IDE select"
    echo "     Project->import->General->Existing Projects into Workspace"
    echo "     and browse to the ./Debug directory. At this point you should be able to import all of the"
    echo "     built projects into Eclipse."
    echo ""
    echo "     For a different IDE or version adjust the variables, IDE and/or IDE_VERSION,"
    echo "     in this script accordingly"
    echo ""
    targets
}

# generate and Eclipse target
build(){
    eval "cmake ${IDE} ${IDE_VERSION} ${TYPE} -S $1 -B $2"
}

# check for an argument
if [ $# -eq 0 ] || [ $1 == ${HELP} ]; then
    help
    exit 0
fi

# check for an output directory
if [ ! -d ${EXAMPLES_DIR}/output ]; then
    echo "No example output directory - creating"
    echo "${EXAMPLES_DIR}/output"
    eval "mkdir ${EXAMPLES_DIR}/output"
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
help
exit 1
