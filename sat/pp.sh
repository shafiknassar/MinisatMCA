#################################################################################################################
# Input: the script expects a CNF formula with the format that minisat expects, and the name of the output file.
# pp.sh <path to input file> < path to output file>
#
# The output is two files:
# 1. The first file is is similar to the CNF formula provided, the diffirence is that we add a new variable
#    to each one of the clauses (represents the assumptions), and it's name is: <input-file>-w-assum.cnf.
# 2. The second one contains all the variables that were added to the CNF but negated.
#    The file's name is: <input-file>-assum.cnf
#################################################################################################################

#!/bin/bash

if [[ ! (-f $1) ]]; then echo file does not exist; exit 1; fi
num_of_vars=0
num_of_clauses=0


cat $1 | while read -a line; do
    if [[ "${line[0]}" == "c" ]]; then
        continue;
    fi
    if [[ "${line[0]}" == "p" ]]; then
        num_of_vars=${line[2]};
        num_of_clauses=${line[3]};
	echo shafik;
	echo "p cnf $num_of_vars $num_of_clauses" > $2
	echo "p assum $num_of_clauses" > tmp_assum.cnf
        continue;
    fi
    let num_of_vars++;
    let last_index=${#line[@]}-1;
    let line[$last_index]=$((num_of_vars));
    let line[$last_index+1]=0;
    echo ${line[*]} >> $2
    echo $((num_of_vars*(-1))) >> tmp_assum.cnf
done

