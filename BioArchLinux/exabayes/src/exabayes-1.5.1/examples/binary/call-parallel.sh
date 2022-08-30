#! /bin/bash

binname=exabayes

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
cmd="$mpi -np 8  $bin -R 2 -C 2  -f aln.phy -q aln.part -c config.nex -n myRun -s 123"
echo -e   "\n\ncommandline:\n$cmd\n\n"
eval "$cmd"
