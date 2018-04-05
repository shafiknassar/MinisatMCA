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
echo "${uniq_algs[*]}"

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
rm -rf $MROOT/bench_output
mkdir $MROOT/bench_output
out_dir=$MROOT/bench_output


#Run the benchmarks for the wanted algorithms
for alg in "${algs[@]}"; do
    for file in $dir_name_cnf; do
        formula=$file
        assum="${file::-4}.assum"
        filename=$(basename "$file")
        echo $MROOT/mca/mca $formula $out_dir/${filename::-4}_alg$i.out -assum=$assum -alg=$alg
    done
done
