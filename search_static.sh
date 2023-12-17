#!/bin/sh

directory="code"
keyword="static"

# Check if the directory exists
if [ ! -d "$directory" ]; then
    echo "Directory '$directory' does not exist."
    exit 1
fi

echo
echo STATIC!!!
echo ----------
grep -n "$keyword" "$directory"/*."c"
grep -n "$keyword" "$directory"/*."h"

echo
echo LOCAL PERSIST!!!
echo ----------
grep -n "local_persist" "$directory"/*."c"
grep -n "local_persist" "$directory"/*."h"
echo

echo
echo GLOBAL VARIABLE!!!
echo ----------
grep -n "global_variable" "$directory"/*."c"
grep -n "global_variable" "$directory"/*."h"

echo
