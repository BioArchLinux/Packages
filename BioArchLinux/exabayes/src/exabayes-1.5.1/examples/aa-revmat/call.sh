#! /bin/bash

binname=yggdrasil

if [ $# != 1 ]; then
    echo -e  "Usage: Please enter path to $binname"
    echo -e  "Example: ./call.sh ../$binname"
    exit
fi

bin=$1

if [ ! -x $bin ]; then
    echo -e  "Error: $bin is not an executable file."
    exit
fi

# this is a pretty memory intensive dataset. Let's trade as much runtime for memory as possible:
cmd="$bin -f aln.phy -m PROT -c config.nex -n myRun -s 123 -M 3 -S"
echo -e   "\n\ncommandline:\n$cmd\n\n"
eval "$cmd"
