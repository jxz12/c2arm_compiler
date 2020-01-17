%stype s_info

%token

INCLUDE TYPEDEF STRUCT SIZEOF
STRING CHARACTER
OPEN_ROUND CLOSE_ROUND OPEN_CURLY CLOSE_CURLY OPEN_SQUARE CLOSE_SQUARE
EQUALS PLUSPLUS PLUS MINUSMINUS MINUS STAR FSLASH
COMPARE
DELIM COMMA 
OROR ANDAND OR XOR AND
IF ELSE WHILE DO FOR RETURN
ID NUMBER

%start file

//%debug

%baseclass-preinclude "Util.h"

%%
								  //initialise
file 		           		:   { arm << ("\t.text\n.global main\n\n");
                                  current_scope = new scope;
                                  current_scope->scope_above = NULL; 
                                  current_scope->scope_no = scope_no;
                                  current_reg = new stack_track;
                                  current_reg->stack_above = NULL;
								  setRegUsedFalse();
                                  initialiseTypeMap(); }
							  program
							  	  //print labels at the end
                                { printList(word_list);
                                  arm << "\n\t.data\n";
                                  printList(data_list); }
        		            ;
        		            
program						: header body
							| body
							;

header	 	    	        : header include	
	     	    	        | include
	     	    	        ;
	     	    	        
include						: INCLUDE
								{ headers += lexer.YYText(); }
							;

body    	 		        : typedef DELIM body
                            | function body
	           		        | function_prototype body
	           		        | function
	           		        ;
	           		        
	           		        
/////////////////////////
//FUNCTION DECLARATIONS//
/////////////////////////	           		        

function_prototype       	: id variable_name OPEN_ROUND function_declaration_list CLOSE_ROUND DELIM
                                { storeFunction($2.value, $1.value, true);
							  	  temp_variable_map.clear();
							  	  temp_count = 0; }
							| id variable_name OPEN_ROUND CLOSE_ROUND DELIM
								{ storeFunction($2.value, $1.value, true); }
	          	            ;

function 		            : id variable_name OPEN_ROUND function_declaration_list CLOSE_ROUND 
							  	{ storeFunction($2.value, $1.value, false);
							  	  //clear the temporary map now that the declarations are stored
							  	  temp_variable_map.clear();
							  	  temp_count = 0; }
							  	  
							  open_scope 
								{ functionCodeStart($2.value); }
							  instruction_list 
							  	{ functionCodeEnd(); }
							  close_scope
							  
	 		                | id variable_name OPEN_ROUND CLOSE_ROUND
	 		                    { storeFunction($2.value, $1.value, false); }
	 		                    
	 		                  open_scope 
								{ functionCodeStart($2.value); }
							  instruction_list
							  	{ functionCodeEnd(); }
							  close_scope
	 		                ;

function_declaration_list   : function_declaration_list COMMA id variable_name 
								  //adds the function declarations to a list to store the functions
								{ addParameter($3.value, $4); }
							  
                            | id variable_name
								{ addParameter($1.value, $2); }
                            ;
                            
                            
/////////////////////
//INSTRUCTION TYPES//
/////////////////////

instruction_list	        : instruction_list instruction
     	   		            | instruction
     	   		            ;
										
                                  					  //remove any temporaries that are still hanging around just in case
instruction		            : declaration DELIM     { removeUsedTemp(); }
	    		            | assignment DELIM      { removeUsedTemp(); }
	    		            | function_call DELIM   { removeUsedTemp(); }
                            | statement             { removeUsedTemp(); }
	    		            ;

statement                   : if
							| else/*
                            | switch*/
                            | while
                            | dowhile
                            | for
                            | return/*
                            | break*/
			                ;

declaration_list            : declaration_list declaration DELIM
                            | declaration DELIM
                            ;
			                
declaration 		        : id initialisation_list
                                { addVariableType($1.value);
                                  temp_strings.clear(); }
	    		            ;

initialisation_list 	    : initialisation_list COMMA initialisation
			                | initialisation                               
			                ;

//for each initialisation, store the name and the value into a vector to store with the type in the declaration
initialisation	    	    : variable_name
                                { addVariableNoType($1, 0); 
                                  temp_strings.push_back($1.value); }
                                
			                | variable_name 
			                    { addVariableNoType($1, 0); 
                                  temp_strings.push_back($1.value); }
                              EQUALS expression
                                { if($1.type == "pointer") $1.type = "variable";
                                  arm << createAssignment($1, $4); }
                                
			                | variable_name OPEN_SQUARE number CLOSE_SQUARE	
                                { addVariableNoType($1, convert<int>($3.value)); 
                                  temp_strings.push_back($1.value);
                                  $1.type = "variable";
                                  getVariableInfo($1.value);
    							  s_info temp = {"string", "addr_" + $1.value + convert<string>(getVariableInfo($1.value).scope_no)};
  								  arm << LDR($1, temp, $1.value); }
		 	                ;


assignment					: parse_assignment
								{ arm << temp_assignment;
								  temp_assignment.clear();
								  removeUsedTemp(); }
							;
								  
stored_assignment			: parse_assignment
								{ $$.value = temp_assignment;
								  temp_assignment.clear();
								  removeUsedTemp(); }
							;
		 	                
parse_assignment            : variable_name EQUALS expression
								{ temp_assignment += createAssignment($1, $3); }

			                | variable_name PLUSPLUS
			                	{ temp_assignment += createAssignment($1, "++"); }
			                	  
			                | variable_name MINUSMINUS
			                	{ temp_assignment += createAssignment($1, "--"); }
			                	  
			                | variable_name OPEN_SQUARE expression CLOSE_SQUARE EQUALS expression
			                    { temp_assignment += createAssignment($1, $6, $3);}
	   		                ;
	   		                
	   		                
	   		                
	   		                

function_call 		        : id OPEN_ROUND expression_list CLOSE_ROUND 
								  //move the parameters into the right place and clear the temporary list
                                { functionCall($1.value); 
                                  temp_registers.clear(); }
                                  
                            | id OPEN_ROUND CLOSE_ROUND
                            	{ functionCall($1.value); }
	      		            ;


//////////////
//STATEMENTS//
//////////////

//I considered using conditional execution instead of branches, but the code got way too complicated
if                          : IF OPEN_ROUND stored_expression CLOSE_ROUND
                                  assign_label
                                { arm << $3.value << B("EQ", $5.value); }
                              instruction
                                { arm << $5.value << ":\n"; }

                            | IF OPEN_ROUND stored_expression 
                              CLOSE_ROUND open_scope 
                              	  assign_label
                                { arm << $3.value << B("EQ", $6.value); }
                              instruction_list
                                { arm << $6.value << ":\n"; }
                              close_scope
                            ;
                            
else						: ELSE
							;
                            
                            
//turns the while into an if then a dowhile for efficiency
while                       : WHILE
                              OPEN_ROUND stored_expression CLOSE_ROUND
                                  assign_label
                                { arm << $3.value << B("EQ", $5.value); }
                                  print_label
                              instruction
                                { arm << $3.value << B("NE", $7.value) << $5.value << ":\n"; }
                              
                            | WHILE 
                              OPEN_ROUND stored_expression CLOSE_ROUND
                              open_scope
                                  assign_label
                                { arm << $3.value << B("EQ", $6.value); }
                              	  print_label
                              instruction_list
                              	{ arm << $3.value << B("NE", $8.value) << $6.value << ":\n"; }
                              close_scope
                            ;
                            
                            
dowhile                     : DO 
                                  print_label
                              instruction WHILE OPEN_ROUND stored_expression
                                { arm << $6.value << B("NE", $2.value); }
                              CLOSE_ROUND DELIM
                              
                            | DO
                                  print_label
                              open_scope instruction_list close_scope WHILE OPEN_ROUND stored_expression
                                { arm << $8.value << B("NE", $2.value); }
                              CLOSE_ROUND DELIM
                            ;
                            
for                         : FOR OPEN_ROUND assignment DELIM
                                print_label
                              stored_expression DELIM stored_assignment CLOSE_ROUND instruction 
                                { arm << $8.value << $6.value << B("NE", $5.value); }
                                
                            | FOR OPEN_ROUND assignment DELIM
                                print_label
                              stored_expression DELIM stored_assignment CLOSE_ROUND open_scope instruction_list
                                { arm << $8.value << $6.value << B("NE", $5.value); }
                              close_scope
                            ;

                            
                            
return                      : RETURN DELIM
								{ arm << POP("r4-r12, pc"); }
                            | RETURN expression DELIM
                                { if(function_map[current_function].return_type == "void")
                                	  error_list.push_back("line " + convert<string>(lexer.lineno()) + ": warning: return has a value but function returns void"); 
                                  s_info temp = {"temporary", "0"};
                                  //set the return register to r0
                                  arm << MOV(temp, $2);
                                  arm << POP("r4-r12, pc"); }
                            ;
                            
                          
                            
                            
//these two replace {} instructions because they sometimes break bisonc++
//this just assigns a label
assign_label				: { $$.value = createLabel(); }
						    ;
						 
//this assigns a label and prints it						 
print_label                 : { $$.value = createLabel();
								arm << ($$.value + ":\n"); }
                            ;

      


   
                                 
///////////	            
//ALGEBRA//
///////////

//TODO: The problem is that the register that gets returned from one nested function gets push_back'ed into the same vector as the registers used by the next nested function. The solution is probably to create a new temp_registers for every function call

expression_list      	 	: expression_list COMMA expression
                                { if($3.type == "pointer"){
                                      $$ = loadDereference($3);
                                      temp_registers.push_back($$); 
                                  }
                                  else temp_registers.push_back($3); }
                            | expression
                                { if($1.type == "pointer"){
                                      $$ = loadDereference($1);
                                      temp_registers.push_back($$); 
                                  }
                                  else temp_registers.push_back($1); }
                            ;


expression					: parse_expression 
								{ arm << temp_expression;
                                  temp_expression.clear(); }
                            ;

stored_expression			: parse_expression
								{ s_info temp = {"number", "0"};
							      $$.value = temp_expression + CMP($1, temp);
								  temp_expression.clear(); }
							;

                            
parse_expression            : parse_expression OROR logical_and
                                { temp_expression += logicalOp("||", $$, $1, $3); }
                            | logical_and

                            | CHARACTER						
			                    { $$.type = "temporary";
			                      $$.value = findTempReg();
			                      temp_expression += loadChar($$, lexer.YYText()); }
			                | STRING						
			                    { $$.value = lexer.YYText(); $$.type = "string"; }
       			            ;

logical_and                 : logical_and ANDAND bitwise_or
                                { temp_expression += logicalOp("&&", $$, $1, $3); }
                            | bitwise_or
                            ;

bitwise_or                  : bitwise_or OR bitwise_xor
								{ temp_expression += mathOp("|", $$, $1, $3); }
                            | bitwise_xor
                            ;

bitwise_xor                 : bitwise_xor XOR bitwise_and
								{ temp_expression += mathOp("^", $$, $1, $3); }
                            | bitwise_and
                            ;

bitwise_and                 : bitwise_and AND comparison
								{ temp_expression += mathOp("&", $$, $1, $3); }
                            | comparison
                            ;

comparison                  : comparison compare sum
                                { temp_expression += compareOp($2.value, $$, $1, $3); }
                            | sum
                            ;

sum                         : sum PLUS term
								{ temp_expression += mathOp("+", $$, $1, $3); }
                            | sum MINUS term
						        { temp_expression += mathOp("-", $$, $1, $3); }
                            | term
                            ;

term   			            : term STAR factor
								{ temp_expression += mathOp("*", $$, $1, $3); }
                            | term FSLASH factor
								{ temp_expression += mathOp("/", $$, $1, $3); }
       			            | factor
       			            ;

factor 			            : OPEN_ROUND parse_expression CLOSE_ROUND
								{ $$ = $2; }
       			            | parameter
       			            ;




////////////////////////////
//PARAMETERS AND VARIABLES//
////////////////////////////

parameter		            : variable_name
                                { if($1.type == "pointer"){
                                      if(getVariableInfo($1.value).pointer == false) 
                                          error_list.push_back("line " + convert<string>(lexer.lineno()) + ": parameter '" + $1.value + "' dereferenced but is not a pointer");
                                      $$ = loadDereference($1);
                                  } }
			                      
			                | AND id
			                      //get the address of an array by LDRing "addr_name"
			                    { $$.type = "temporary";
			                      $$.value = findTempReg();
                                  if(getVariableInfo($2.value).array_size == 0){
                                      getVariableInfo($2.value);
			                          s_info temp = {"string", "addr_" + $2.value + convert<string>(getVariableInfo($2.value).scope_no)};
			                          arm << LDR($$, temp, $2.value);
                                  }
                                  else{ 
                                      $2.type = "variable";
                                      arm << MOV($$, $2);
                                  } }
			                    
			                | variable_name OPEN_SQUARE expression CLOSE_SQUARE
			                    { $$.type = "temporary";
			                      $$.value = findTempReg();
                                  s_info temp = {"variable", $1.value};
			                      arm << LDR($$, temp, $3);
			                      
			                      if(/*getVariableInfo($1.value).pointer == false ||*/ getVariableInfo($1.value).array_size == 0)
			                          error_list.push_back("line " + convert<string>(lexer.lineno()) + ": parameter '" + $1.value + "' indexed with [] but is not a pointer or array");
			                      
                                  if($1.type == "pointer"){
                                      if(getVariableInfo($1.value).pointer == false)
                                          error_list.push_back("line " + convert<string>(lexer.lineno()) + ": parameter '" + $1.value + "' dereferenced but is not a pointer");
                                      arm << LDR($$, $$, $1.value);
                                  } }
			                      
			                | AND id OPEN_SQUARE expression CLOSE_SQUARE
			                      //get the address of an array by LDRing the address of the first value and ADDing log2(size)
			                    { if(getVariableInfo($2.value).pointer == false || getVariableInfo($2.value).array_size == 0)
			                          error_list.push_back("line " + convert<string>(lexer.lineno()) + ": parameter '" + $1.value + "' indexed with [] but is not a pointer or array");

                                  $$.type = "temporary";
			                      $$.value = findTempReg();
                                  $2.type = "variable";

			                      if($4.type == "number") convertToTemporary($4);
			                      arm << MOV($$, $2) << ADD($$, $$, $4, log(type_map[getVariableInfo($2.value).type].size)/log(2)); }
			                
			                | number

			                | function_call
			                      //use r0 to get the value returned
			                    { $1.type = "temporary";
			                      $1.value = "0";
       			            	  $$.type = "temporary";
       			            	  $$.value = findTempReg();
       			            	  arm << MOV($$, $1); }
			                ;
			                

variable_name               : id
			                    { $$.type = "variable"; }
			                | STAR id
                                { $$.type = "pointer";
                                  $$.value = $2.value; }
			                ;

id			                : ID
                                { $$.value = lexer.YYText(); }
     			            ;

number                      : NUMBER 
                                { $$.type = "number";
                                  $$.value = lexer.YYText(); }
                            | SIZEOF OPEN_ROUND id CLOSE_ROUND
                                  //evaluate the sizeof() before runtime
                                { $$.type = "number";
                                  $$.value = findSizeof($3.value); }
                            ;

compare                     : COMPARE
                                { $$.value = lexer.YYText(); }
                            ;

/////////
//SCOPE//
/////////

open_scope                  : OPEN_CURLY
                                { scope* temp = new scope;
                                  temp->scope_above = current_scope;
                                  current_scope = temp;
                                  current_scope->scope_no = ++scope_no; }
                            ;
                            
close_scope                 : CLOSE_CURLY
			                    //{ printVariables(current_scope->variable_map); }
                                { addDefinitions();
                                  scope* temp = current_scope;
                                  current_scope = current_scope->scope_above;
                                  delete temp; }
                            ;
                            
                            


/////////
//TYPES//
/////////

typedef                     : TYPEDEF id variable_name
                                { if(!checkType($2.value)){
    							      error_list.push_back("line " + convert<string>(lexer.lineno()) + ": type '" + $2.value + "' not found"); return;
    							  }
                                  bool pointer = false;
                                  if($3.type == "pointer") pointer = true;
                                  addType($3.value, type_map[$2.value].size, pointer); }
                            | TYPEDEF struct variable_name
                                //{ createStruct($2.value); }
                            ;

struct                      : STRUCT id open_scope declaration_list close_scope
								{ $$ = $2; }
                            ;











