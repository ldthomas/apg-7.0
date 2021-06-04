#!/bin/bash

# run the release builds

APG=apg70
APG_DIR=apg
EXAMPLES=examples
BUILD=Release
NAMES=(ex-apgex ex-api ex-ast ex-basic ex-conv ex-format ex-json ex-lines ex-msgs ex-sip ex-trace ex-wide ex-xml)
NAMELEN=${#NAMES[@]}
HELP=--help

# display the targets and descriptions
targets(){
    echo '     target    - description'
    echo '     ------    - -----------'
    echo '     --help    - print this help screen'
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
    echo "     run.sh  - envoke the APG 7.0 parser generator and examples of it's use"
    echo ''
    echo 'SYNOPSIS'
    echo "     ./run.sh target"
    echo ''
    echo 'DESCRIPTION'
    echo '     Use this script to envoke the APG 7.0 parser generator or any of the supplied examples'
    echo "     of using APG and the supplied suite of accompanying libraries."
    echo ""
    echo '     The target application must have been previously built with the script'
    echo "     ./build.sh -r target"
    echo "     or it's equivalent."
    echo ""
    echo "     Note 1. When envoking ${APG} this script allows only 8 arguments. A better solution is"
    echo "             to copy ${APG} to a PATH directory and then envoke it directly from the command line."
    echo "             For example:"
    echo "             sudo cp ${BUILD}/${APG_DIR}/${APG} /usr/local/bin"
    echo ""
    echo "     Note 2. Running the example applications here may provide limited enlightenment."
    echo "             For the examples, it is probably better to use the script"
    echo "             ./build -d target"
    echo "             and run the examples in the Eclipse IDE with a debugger."
    echo ""
    targets
}

# check for no or --help target
if [ $# -eq 0 ] || [ $1 == ${HELP} ]; then
    help
    exit 0
fi

# check for apg special case 
if [ $1 == "apg" ]; then
echo "run.sh limits apg to 8 arguments"
echo "better to copy ${BUILD}/$1/${APG} to a PATH directory and run ${APG}"
    eval "${BUILD}/${APG_DIR}/${APG} $2 $3 $4 $5 $6 $7 $8 $9"
    exit 0
fi

# find the target and execute it
for (( i=0; i<${NAMELEN}; i++))
do
    if [ ${NAMES[$i]} == $1 ]; then
        eval "${BUILD}/$1/$1 $2"
        exit 0
    fi
done

# if we are here then the target name was bad
echo "unrecognized target: $1"
echo ""
targets
exit 1
