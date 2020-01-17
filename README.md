#A compiler for translating C into ARM assembly

##Installation:
requires bisonc++ V2.4.8, flex 2.5.35
under the 'Compiler' directory
'make' creates the executable
'make clean' removes the executable
'make all' creates the executable and runs it on test.c 

##Operating instructions:
create a c program e.g. 'program.c'
use the command 'cat program.c | ./output'
the output file will be called test.s

##Programmer:
Jonathan Zheng, jxz12

##Known bugs:
Function Prototypes do not work well in conjunction with pointers
Arrays of pointers can sometimes cause problems
