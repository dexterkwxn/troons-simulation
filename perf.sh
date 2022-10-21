#!/bin/bash


# prepare files
make submission
make bonus

echo "copying files..."

cp troons /nfs/home/$USER/
cp troons_bonus /nfs/home/$USER/
cp troons_seq2 /nfs/home/$USER/
cp test1.in /nfs/home/$USER/
cp test2.in /nfs/home/$USER/
cp test3.in /nfs/home/$USER/
cp test4.in /nfs/home/$USER/
cp test5.in /nfs/home/$USER/
cp test6.in /nfs/home/$USER/

echo "files copied, executing slurm job now..."

# submit slurm job
sbatch ./slurm.sh

