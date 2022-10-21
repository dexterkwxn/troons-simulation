#!/bin/bash
#SBATCH --job-name=troons
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --mem=16gb
#SBATCH --output=troons_%j.log
#SBATCH --error=troons_%j.log
#SBATCH --partition="xs-4114"

echo "Running troons job"
echo "We are running on $(hostname)"

# Broadcast our executable to all allocated nodes
sbcast -f /nfs/home/$USER/troons /tmp/troons
sbcast -f /nfs/home/$USER/troons_bonus /tmp/troons_bonus
sbcast -f /nfs/home/$USER/troons_seq2 /tmp/troons_seq2
sbcast -f /nfs/home/$USER/test1.in /tmp/test1.in
sbcast -f /nfs/home/$USER/test2.in /tmp/test2.in
sbcast -f /nfs/home/$USER/test3.in /tmp/test3.in
sbcast -f /nfs/home/$USER/test4.in /tmp/test4.in
sbcast -f /nfs/home/$USER/test5.in /tmp/test5.in
sbcast -f /nfs/home/$USER/test6.in /tmp/test6.in

# Actual job
perf stat /tmp/troons /tmp/test1.in > /dev/null
perf stat /tmp/troons_bonus /tmp/test1.in > /dev/null
perf stat /tmp/troons_seq2 /tmp/test1.in > /dev/null
perf stat /tmp/troons /tmp/test2.in > /dev/null
perf stat /tmp/troons_bonus /tmp/test2.in > /dev/null
perf stat /tmp/troons_seq2 /tmp/test2.in > /dev/null
perf stat /tmp/troons /tmp/test3.in > /dev/null
perf stat /tmp/troons_bonus /tmp/test3.in > /dev/null
perf stat /tmp/troons_seq2 /tmp/test3.in > /dev/null
perf stat /tmp/troons /tmp/test4.in > /dev/null
perf stat /tmp/troons_bonus /tmp/test4.in > /dev/null
perf stat /tmp/troons_seq2 /tmp/test4.in > /dev/null
perf stat /tmp/troons /tmp/test5.in > /dev/null
perf stat /tmp/troons_bonus /tmp/test5.in > /dev/null
perf stat /tmp/troons_seq2 /tmp/test5.in > /dev/null
perf stat /tmp/troons /tmp/test6.in > /dev/null
perf stat /tmp/troons_bonus /tmp/test6.in > /dev/null
perf stat /tmp/troons_seq2 /tmp/test6.in > /dev/null

# Copy log file
cp "troons_$SLURM_JOB_ID.log" "/nfs/home/$USER/"
mv -f "/nfs/home/$USER/troons_$SLURM_JOB_ID.log" "/nfs/home/$USER/troons_perf.log" 
