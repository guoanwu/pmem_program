for program in lock_*
do
    numactl -N 1 perf stat ./$program
done
