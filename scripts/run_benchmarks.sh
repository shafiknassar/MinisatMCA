#!/bin/bash

dir_name=$1;
if [ ! -d "${dir_name}" ]; then
    echo "$dir_name is not a directory";
    exit 2;
fi
dir_name_cnf=$dir_name"*.cnf";

#get the algorithms
if (( $# < 2 )); then
    echo no algorithm is given
elif (( $# == 2 )); then
    algs=($2)
elif (( $# == 3 )); then
    algs=($2 $3)
elif (( $# == 4 )); then
    algs=($2 $3 $4)
elif (( $# == 5 )); then
    algs=($2 $3 $4 $5)
elif (( $# >= 5 )); then
    echo only four algorithms are available!
    exit 2;
fi

#remove repetetion in algs
uniq_algs=($(echo "${algs[@]}" | tr ' ' '\n' | sort -u | tr '\n' ' '))

#Compile mca
cd $MROOT/mca
rm -f mca
make &> /dev/null 
if [[ ! -f $MROOT/mca/mca ]]; then
    cd - > /dev/null 
    echo
    echo Program complitaion failed
    exit 2
fi
cd - > /dev/null

#Create out dir
rm -rf $MROOT/benchmarks_output
mkdir $MROOT/benchmarks_output
out_dir=$MROOT/benchmarks_output

rot_arr=('Iterative Deletion' 'Iterative Deletion Algorithm with MiniSAT minimization' 'Iterative Insertion' 'Rotation')

#Run the benchmarks for the wanted algorithms
for alg in "${algs[@]}"; do
    for file in $dir_name_cnf; do
        formula=$file
        assum="${file::-4}.assum"
        filename=$(basename "$file")
        query_num=${filename::-4}
        echo Running $query_num with ${rot_arr[${alg}-1]} algorithm > $out_dir/${query_num}_alg${alg}.out
        echo >> $out_dir/${query_num}_alg${alg}.out
        $MROOT/mca/mca $formula $out_dir/${query_num}_alg${alg}.assum -assum=$assum -alg=$alg >> $out_dir/${query_num}_alg${alg}.out
    done
done
