# Context
This was part of a school assignment, with the objective of learning how to work with OpenMP. 
The code was written in C++. You can find more details below as well as the attached PDF for this assignment.
Pair work was allowed for this assignment. I did this with another friend of mine @zwliew.

## Getting Started

### Provided files

* `troons_seq`: an executable of a correct sequential implementation for you to test your program
* `Makefile`: makefile with basic recipes to build, build with debug symbols and clean
* `main.cc`: skeleton code with input logic
* `testcases/`: example testcases from the assignment brief to get you started. Inputs have the extension `.in`, expected outputs have the extension `.output`
* `.gitignore`: a sensible gitignore template for C++

### Building the project

This starter code comes with a Makefile and a few recipes that can be run from the command line.

```
make submission   # builds the project using g++ and C++17
make debug        # builds the project with debug symbols and a symbol `DEBUG` defined
make clean        # cleans your project directory
```

### Testing your code

After building, run the code with `./troons <testcase_file>`. Your program should output to `stdout` (the command line
usually), and you can then pipe the output to a file like so:
`./troons testcases/sample1.in > sample1.out`.

One way to test is as follows:

1. Run a testcase and save the output of your program to a file. E.g. `./troons testcases/sample1.in > sample1.out`
2. Run the correct implementation executable provided with the same test case and save the output of that to another
   file. E.g. `./troons_seq testcases/sample1.in > sample1-correct.out`
3. Diff the outputs of the two files. E.g. `diff sample1.out sample1-correct.out`

NOTE: `*.out` files are ignored by the `.gitignore` file so if you wish to push your output files to github, use another extension (e.g. `.output`).

## Submitting your assignment

1. Push your code **and report** to this repo
2. Get the commit hash. A commit hash should look something like `2957684c4985684abd8b0cdea1501a8bc8fd7bb1` (do not submit the shorter version). A commit's hash can be found either:
   1. From your github repo. Find the commit you want to submit and click into the commit details to see the hash.
   2. By running `git log` in a terminal. Each commit's hash should be fully displayed along with other info about the commit.
3. Submit this commit hash on Canvas via the Assignment 1 text entry box before the assignment deadline.

IMPORTANT: You can use C or C++ (C++ recommended), and you can modify any file in the starter code. **We only require
that when your code is cloned on a lab machine and `make submission` is run, an executable named `troons` is produced in
the same directory**. We will autograde your submissions, and reserve the right to deduct any number of marks if your
project does not follow this requirement.

## Github actions

When you push to the repo, a Github Action will run that calls `make submission` then checks if a file named `troons` has been produced. This is **just a preliminary check** that the recipe works: it does not test any functionality of the file produced, and does not guarantee that `make submission` will work on a lab machine because the Github Actions environment may be different from that of a lab machine environment. 

There is **no penalty for failing the Github Action**; but if it is failing then it may indicate that the build will also fail on a lab machine (which will be penalised).

We highly recommend that you clone your repository on a few different lab machines, build your program and test its correctness and/or performance.
