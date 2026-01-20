#!/bin/bash

# Count how many arguments passed
ARG_COUNT=$#

if [ "$ARG_COUNT" -lt 2 ]; then
    echo "Error: Missing arguments."
    echo "Usage: $0 <writefile> <writestr>"
    exit 1
fi

# Assign the first argument which is a path to a variable
writefile=$1
# Assign the second argument which is a string which needs to be written to a filee 
writestr=$2

dirpath=""

dirpath=$(dirname "$writefile")

# Create the directory if it does not exist
mkdir -p "$dirpath"

# check to see if directory exists else echo error
if [ ! -d "$dirpath" ]; then
    echo "Error: Directory was not created: $dirpath"
    exit 1
fi

# Write the string to the file
echo "$writestr" > "$writefile"

# check if file exists else echo error
if [ ! -f "$writefile" ]; then
    echo "Error: File was not created: $writefile"
    exit 1
fi

# Exit
exit 0



#Acknowledgement
#Google search on finding the directory Exists or not 
#https://www.google.com/search?q=bash+how+to+check+if+directory+exists+%3F
