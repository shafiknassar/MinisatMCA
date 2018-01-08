#################################################################################################################
# Input: the script expects a CNF formula with the format that minisat expects, and the name of the output file.
# pp.sh <path to input file> <path to the cnf output> <path to the assumption output>
# If there is no output files provided, default is: "cnf_pp.cnf" for the formula and "assum_pp.cnf" for the assumption.
#
# The output is two files:
# 1. The first file is is similar to the CNF formula provided, the diffirence is that we add a new negated 
# literal to each one of the clauses (represents the assumptions).
# 2. The second one contains all the literals that were added to the CNF (not negated).
#################################################################################################################

#!/bin/bash

#output files handling
if (( $# == 3 )); then
    cnf_out=$2
    assum_out=$3
elif (( $# <= 2 )); then
    assum_out="assum_pp.cnf"
    [[ $# = 2 ]] && cnf_out="$2" || cnf_out="cnf_pp.cnf"
fi
echo -n | tee $cnf_out $assum_out

if [[ ! (-f $1) ]]; then echo "file does not exist"; exit 1; fi

num_of_vars=0
num_of_clauses=0
init=0

cat $1 | while read -a line; do
    if [[ "${line[0]}" == "c" ]]; then
        echo ${line[0]} >> $cnf_out
        continue;
    fi

    #INVARIANT: This should only appear at the beginning of the file, meaning no other line should begin with "p"
    if [[ "${line[0]}" == "p" ]]; then
        if [[ $init == 1 ]]; then echo "only one initialization line allowed, please fix!"; exit 2; fi
        num_of_vars=${line[2]}
        num_of_clauses=${line[3]}
	echo "p cnf $((num_of_vars+num_of_clauses)) $num_of_clauses" >> $cnf_out
	echo "p assumptions $num_of_clauses" >> $assum_out
        init=1
        continue;
    fi
    if (( $init == 0 )); then echo "no initialization line before the cnf, usage: p cnf <num of vars> <num of clauses>"; exit 3; fi

    let num_of_vars++;
    let last_index=${#line[@]}-1
    let line[$last_index]=$((num_of_vars*(-1)))
    let line[$last_index+1]=0
    echo ${line[*]} >> $cnf_out
    echo $((num_of_vars)) >> $assum_out
done

