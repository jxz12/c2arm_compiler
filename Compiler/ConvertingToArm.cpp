#include "Parser.h"

using namespace std;


//////////////
//ARM MEMORY//
//////////////

string Parser::createWord(string name, int scope_no){
    return "addr_" + name + convert<string>(scope_no) + ": .word " + name + convert<string>(scope_no);
}

//creates a label that allocates memory for a variable
string Parser::createData(string name, int elements, int bytes, int scope_no){

    //if the variable isn't an array, treat it as if it is one with one element
    if(elements == 0){
    	return name + convert<string>(scope_no) + ": .skip " + convert<string>(bytes);
    }
    else{
    	return ".balign " + convert<string>(bytes) + "\n" + name + convert<string>(scope_no) + ": .skip " + convert<string>(elements*bytes);
    }
}

//creates a vector of strings filled with labels and directives to print at the end of the code
void Parser::addDefinitions(){

    string name;
    int bytes, elements;

    for(map <string, variable_info>::const_iterator it = current_scope->variable_map.begin();
        it != current_scope->variable_map.end(); ++it){
    
        name = it->first;
        elements = it->second.array_size;
        if(current_scope->variable_map[name].pointer){
            //pointers are always 4 byte addresses
            bytes = 4;
        }
        else{
            bytes = type_map[current_scope->variable_map[name].type].size;
        }

        data_list.push_back(createData(name, elements, bytes, current_scope->scope_no));
        word_list.push_back(createWord(name, current_scope->scope_no));
    }

}


//creates the label and directive at the end for a literal char
string Parser::loadChar(s_info temp, string character){
    string label = "char" + convert<string>(++char_no);
    data_list.push_back(label + ": .byte " + character);
    word_list.push_back("addr_" + label + ": .word " + label);
    s_info address = {"string", "addr_" + label};
    return "\tLDR r" + temp.value + ", addr_" + label + "\n\tLDR r" + temp.value + ", [r" + temp.value + "]\n";
}





/////////////////////
//CALLING FUNCTIONS//
/////////////////////

//creates arm code necessary for a procedure call
void Parser::procedureCall(string name){

    //temp_registers contains a list of the registers that contain the functions parameters
    //move the parameters used to the right place (registers 0 to 3)
    
    s_info dest_reg;
    dest_reg.type = "temporary";
    
    if(temp_registers.size() < 5){
        for(int i = 0; i < temp_registers.size(); i++){
            dest_reg.value = convert<string>(i);
            arm << MOV(dest_reg, temp_registers[i]);
        }
        arm << BL(condition, name);
    }
    
    //if there are more than 4 parameters then the stack is needed
    else if(temp_registers.size() == 5){
        arm << PUSH("r4");
        
        for(int i = 0; i < temp_registers.size(); i++){
            dest_reg.value = convert<string>(i);
            arm << MOV(dest_reg, temp_registers[i]);
        }
        arm << BL(condition, name);
        arm << POP("r4");
    }
    else if(temp_registers.size() > 5){
        arm << PUSH("r4-r" + convert<string>(temp_registers.size() - 1));
        
        for(int i = 0; i < 4; i++){
            dest_reg.value = convert<string>(i);
            arm << MOV(dest_reg, temp_registers[i]);
        }
        
        //move the rest into temporary registers first to prevent data hazards
        vector<s_info> temp_list;
        for(int i = 4; i < temp_registers.size(); i++){
            dest_reg.value = findTempReg();
            arm << MOV(dest_reg, temp_registers[i]);
            temp_list.push_back(dest_reg);
        }
        for(int i = 0; i < temp_list.size(); i++){
            dest_reg.value = convert<string>(i+4);
            arm << MOV(dest_reg, temp_list[i]);
        }
        arm << BL(condition, name);
        arm << POP("r4-r" + convert<string>(temp_registers.size() - 1));
    }
}



//acts as a control panel type function to deal with function calls
void Parser::functionCall(string name){

	//these if statements check for functions inside existing libraries
    if(name == "printf"){
		if(headers.find("stdio.h") == -1){
			error_list.push_back("line " + convert<string>(lexer.lineno()) + ": printf called but stdio.h not included"); return;
		}
        armPrintf();
    }
    else if(name == "malloc"){
		if(headers.find("stdlib.h") == -1){
			error_list.push_back("line " + convert<string>(lexer.lineno()) + ": malloc called but stdlib.h not included"); return;
		}
        armMalloc();
    }
    else if(name == "free"){
		if(headers.find("stdlib.h") == -1){
			error_list.push_back("line " + convert<string>(lexer.lineno()) + ": free called but stdlib.h not included"); return;
		}
        armFree();
    }
    
    //if it's a normal function call, then check to see if it's been defined or not
    else{
        //if the function hasn't been defined
    	if(function_map.find(name) == function_map.end()){
        	error_list.push_back("line " + convert<string>(lexer.lineno()) + ": function '" + name + "' not previously defined"); return;
    	}
    	//if the amount of parameters isn't correct
    	if(function_map[name].parameter_map.size() != temp_registers.size()){
    	    error_list.push_back("line " + convert<string>(lexer.lineno()) + ": incorrect number of parameters in function '" + name + "'"); return;
    	}
        procedureCall(name);
    }
}





//concatenates the values of a vector of temporary registers because I need to pass the value of multiple registers in one token
string Parser::concatenateRegisters(vector<s_info> temp_registers){
	string registers;
	for(int i = 0; i < temp_registers.size(); i++){
		registers += getRegNo(temp_registers[i]) + " ";
    }
    return registers;
}

//gets the vector of temporary registers back
vector<s_info> Parser::unconcatenateRegisters(string registers){
	vector<s_info> temp_registers;
	string temp;
	s_info reg;
	reg.type = "temporary";
	for(string::const_iterator it = registers.begin(); it != registers.end(); ++it){
		if(*it == ' '){
			reg.value = temp;
			temp_registers.push_back(reg);
			temp.clear();
		}
		else{
			temp += *it;
		}
	}
	return temp_registers;
}







/////////////////
//FUNCTION CODE//
/////////////////

//prints the start of a function
void Parser::functionCodeStart(string name){
	arm << name << ":" << endl;
	//sets the variable_map to the parameter_map
	current_scope->variable_map = function_map[name].parameter_map;
	current_function = name;
	
	//push the values needed after the function onto the stack
	arm << PUSH("r4-r12, lr");
	stack_track* temp = new stack_track;
	temp->stack_above = current_reg;
	current_reg = temp;
	
	int i = current_scope->variable_map.size() + 3;
	s_info parameter = {"variable", ""},
		   parameter_dest = {"temporary", ""},
		   temp_reg = {"temporary", findTempReg()};
		   
	//set up the state of the program
	for(map<string, variable_info>::iterator it = current_scope->variable_map.end();
		it != current_scope->variable_map.begin(); ){
		--it;
        
        //move the parameters (starting from r0) into r4 upwards so that they act like normal variables
		parameter.value = it->first;
		parameter_dest.value = convert<string>(i);
		arm << MOV(parameter_dest, parameter);
		it->second.reg = i;
		current_reg->registers_used[i] = true;
		i--;
		
		//STR all the variables in their memory locations (essentially copying when passed by value)
	    s_info variable = {"variable", it->first},
	           address = {"string", "addr_" + it->first + convert<string>(getVariableInfo(it->first).scope_no)};
	           
        arm << LDR(temp_reg, address, it->first) + STR(variable, temp_reg);
	}
	removeUsedTemp(temp_reg);
}

//prints the end of a function
void Parser::functionCodeEnd(){
	//just in case there's no return statement
	arm << POP("r4-r12, pc") << endl;
    current_function = "";
    
    //move the state of the popped stack back into current_reg
    stack_track* temp = current_reg;
    current_reg = current_reg->stack_above;
    delete temp;
}







/////////////
//OPERATORS//
/////////////

//returns the instruction for each math operator
string Parser::mathOp(string op, s_info &Rd, s_info &Rn, s_info &Op2){
	string text;

    //create a temporary register if a number is used in Rn
    if(Rn.type == "number") text += convertToTemporaryNoPrint(Rn);
           	
	//create a temporary register for the result
	Rd.value = findTempReg();
	Rd.type = "temporary"; 

    //bitwise
    if(op == "|") return text + ORR(Rd, Rn, Op2);
    if(op == "^") return text + XOR(Rd, Rn, Op2);
    if(op == "&") return text + AND(Rd, Rn, Op2);

    //math
    if(op == "+") return text + ADD(Rd, Rn, Op2);
    if(op == "-") return text + SUB(Rd, Rn, Op2);
    if(op == "*"){
        //create a temporary register if a number is used in Op2(Rm) for multiply and divide
        if(Op2.type == "number") text += convertToTemporaryNoPrint(Op2);
        return text + MUL(Rd, Rn, Op2);
    }
    if(op == "/"){
        if(Op2.type == "number") text += convertToTemporaryNoPrint(Op2);
        return text + SDIV(Rd, Rn, Op2);
    }

}





//finds the condition codes for each comparator
string Parser::findConditionCode(string op){
    if(op == "==") return "EQNE";
    if(op == "!=") return "NEEQ";
    if(op == "<")  return "LTGE";
    if(op == "<=") return "LEGT";
    if(op == ">")  return "GTLE";
    if(op == ">=") return "GELT";
}



string Parser::compareOp(string op, s_info &Rd, s_info &Rn, s_info Op2){
    string text;
    //create a temporary register if a number is used in Rn
    if(Rn.type == "number"){
       	text += convertToTemporaryNoPrint(Rn);
    }
	//create a temporary register for the result
	Rd.value = findTempReg();
	Rd.type = "temporary";

    text += CMP(Rn, Op2);
    condition = findConditionCode(op).substr(0,2);
    s_info temp = {"number", "1"};
    text += MOV(Rd, temp);
    condition = findConditionCode(op).substr(2,2);
    temp.value = "0";
    text += MOV(Rd, temp);
    condition.clear();
    return text;
}




string Parser::logicalOp(string op, s_info &Rd, s_info Rn, s_info Op2){

    s_info temp = {"number", "0"};
    string text = CMP(Rn, temp);
    if(op == "&&"){
        //compare if the left expression does return true, so it has to go through both
        condition = "NE";
        text += CMP(Op2, temp);
    }
    if(op == "||"){
        //only compare if the left expression one doesn't return true, so it skips the right if the left is already true
        condition = "EQ";
        text += CMP(Op2, temp);
    }
    condition = "NE";
    temp.value = "1";
    return text + MOV(Rd, temp);
}











///////////////
//ASSIGNMENTS//
///////////////


//retrieves the register that the variable a pointer is pointing at is stored in so that it can be updated
s_info Parser::getPointedReg(s_info token){
    s_info temp = {"temporary", convert<string>(pointer_map[getVariableInfo(token.value).reg])};
    return temp;
}

s_info Parser::getArrayPointedReg(string mem_location){
    s_info temp = {"temporary", convert<string>(array_pointer_map[mem_location])};
    return temp;
}

//dereferences a pointer
s_info Parser::loadDereference(s_info token){
    s_info temp;
    temp.type = "temporary";
    temp.value = findTempReg();
    token.type = "variable";
    arm << LDR(temp, token, token.value);
    return temp;
}


string Parser::createAssignment(s_info left, s_info &right){

    //if the left hand side is a derefenced pointer
    if(left.type == "pointer"){
        if(!getVariableInfo(left.value).pointer){
            error_list.push_back("line " + convert<string>(lexer.lineno()) + ": variable '" + left.value + "' dereferenced but is not a pointer"); return "";
        }
        if(right.type == "number" || right.type == "char"){
            convertToTemporary(right);
        }
        s_info temp = {"variable", left.value};
        if(pointer_map.find(getVariableInfo(left.value).reg) == pointer_map.end()) return STR(right, temp);
        else return STR(right, temp) + MOV(getPointedReg(left), right);
    }
    else{
        s_info address = {"string", "addr_" + left.value + convert<string>(getVariableInfo(left.value).scope_no)};
        s_info temp = {"temporary", findTempReg()};
        return MOV(left, right) + LDR(temp, address, left.value) + STR(left, temp);
    }
}


//overload createAssignment for arrays
string Parser::createAssignment(s_info left, s_info &right, s_info offset){

	if(/*getVariableInfo(left.value).pointer == false ||*/ getVariableInfo(left.value).array_size == 0)
		error_list.push_back("line " + convert<string>(lexer.lineno()) + ": variable '" + left.value + "' indexed with [] but is not a pointer or array");
		
    if(right.type == "number" || right.type == "char"){
        convertToTemporary(right);
    }
    //if the left hand side is a derefenced pointer
    if(left.type == "pointer"){
        if(!getVariableInfo(left.value).pointer){
            error_list.push_back("line " + convert<string>(lexer.lineno()) + ": variable '" + left.value + "'dereferenced but is not a pointer"); return "";
        }
        s_info temp = {"temporary", findTempReg()};
        left.type = "variable";

        string mem_location = getRegNo(left) + offset.value;
        if(array_pointer_map.find(mem_location) == array_pointer_map.end()) return LDR(temp, left, offset) + STR(right, temp);
        else return LDR(temp, left, offset) + STR(right, temp) + MOV(getArrayPointedReg(mem_location), right);
    }
    else{
        //if a pointer is in the register STRed from, mark it as also in the place MOVed to
        if(pointer_map.find(convert<int>(getRegNo(right))) != pointer_map.end()){
            array_pointer_map[getRegNo(left) + offset.value] = pointer_map[convert<int>(getRegNo(right))];
        }
        return STR(right, left, offset);
    }
}



//overload createAssignment for ++ and --
string Parser::createAssignment(s_info left, string op){

	string instruction;
    s_info one = {"number", "1"};
	
	if(left.type == "pointer"){
        if(!getVariableInfo(left.value).pointer){
            error_list.push_back("line " + convert<string>(lexer.lineno()) + ": variable '" + left.value + "'dereferenced but is not a pointer"); return "";
        }
	    s_info temp = {"temporary", findTempReg()};
	    instruction += LDR(temp, left, "");
        
        if(op == "++"){
        	instruction += ADD(temp, temp, one);
        }
        if(op == "--"){
        	instruction += SUB(temp, temp, one);
        }
        s_info temp2 = {"variable", left.value};
        if(pointer_map.find(getVariableInfo(left.value).reg) == pointer_map.end()) return instruction + STR(temp, temp2);
        else return instruction + STR(temp, temp2) + MOV(getPointedReg(left), temp);
    }
    else{
    	if(op == "++"){
        	instruction += ADD(left, left, one);
        }
        if(op == "--"){
        	instruction += SUB(left, left, one);
        }
        
        s_info address = {"string", "addr_" + left.value + convert<string>(getVariableInfo(left.value).scope_no)};
        s_info temp = {"temporary", findTempReg()};
        return instruction + LDR(temp, address, left.value) + STR(left, temp);
    }
}



