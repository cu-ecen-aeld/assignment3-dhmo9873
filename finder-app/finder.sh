#!/bin/sh

# First, check how many arguments were provided
ARG_COUNT=$#

# If fewer than 2 arguments are given, echo error
if [ "$ARG_COUNT" -lt 2 ]; then
    echo "Error: Missing arguments."
    echo "Usage: $0 <filesdir> <searchstr>"
    exit 1
fi

# Assign the first argument which is a path to a variable 
filesdir=$1

# Assign the second argument which is a search string to variable
searchstr=$2

# Check if the directory exists if not echo and exit
if [ -d "$filesdir" ]; then
    echo "Directory exists: $filesdir"
else
    echo "Error: $filesdir is not a valid directory."
    exit 1
fi

X=0

# find command to list all files and read them one by one
file_list=$(find "$filesdir" -type f)

for file in $file_list
do
    X=$((X + 1))
done

Y=0

# Search for the string in each file
for file in $file_list
do
    # Count matching lines in the current file
    matches_in_file=$(grep "$searchstr" "$file" | wc -l)

    # Add the matches to the total
    Y=$((Y + matches_in_file))
done

# Print the final result
echo "The number of files are $X and the number of matching lines are $Y"

# Exit
exit 0





#Acknowledgement
#Google search on finding the directory Exists or not 
#https://www.google.com/search?q=bash+how+to+check+if+directory+exists+%3F
#Google search on finding the number of lines found by grep
#https://unix.stackexchange.com/questions/291225/count-the-number-of-lines-found-by-grep
