output: parse.o ConvertingToArm.o ArmInstructions.o StoringInformation.o lex.yy.o output.o
	g++ -o output output.o parse.o lex.yy.o ConvertingToArm.o ArmInstructions.o StoringInformation.o
	
parse.o: parse.cc Parser.h
	 g++ -c parse.cc

lex.yy.o: lex.yy.cc Parserbase.h
	  g++ -c lex.yy.cc

output.o: output.cpp Parser.h Util.h
	  g++ -c output.cpp

ConvertingToArm.o:  ConvertingToArm.cpp
	g++ -c ConvertingToArm.cpp 
	
ArmInstructions.o:  ArmInstructions.cpp
	g++ -c ArmInstructions.cpp

StoringInformation.o: StoringInformation.cpp
	g++ -c StoringInformation.cpp

Parserbase.h: parse.cc

parse.cc: Compiler.y
	  bisonc++ Compiler.y

lex.yy.cc: Compiler.l
	   flex++ Compiler.l

clean:
	rm -f lex.yy.cc parse.cc Parserbase.h *.o output

make all: clean output
	cat test.c | ./output
