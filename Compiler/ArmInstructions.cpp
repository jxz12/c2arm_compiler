#include "Parser.h"

using namespace std;


////////////////////
//ARM INSTRUCTIONS//
////////////////////

//clear registers_used for all registers past the variables stored
void Parser::removeUsedTemp(){
    for(int i = current_scope->variable_map.size() + 4; i < 13; i++){
        current_reg->registers_used[i] = false;
        
        //gets rid of any temporaries that shouldn't be in the pointer map
        if(pointer_map.find(i) != pointer_map.end()){
        	pointer_map.erase(i);
        }
    }
}

//remove a temporary's register if passed to this function
void Parser::removeUsedTemp(s_info token){
	if(token.type == "temporary"){
		current_reg->registers_used[convert<int>(token.value)] = false;
	}
}

//overload the function because only 2 temporaries will ever be used in an arm instruction
void Parser::removeUsedTemp(s_info token1, s_info token2){
	if(token1.type == "temporary"){
		current_reg->registers_used[convert<int>(token1.value)] = false;
	}
	if(token2.type == "temporary"){
		current_reg->registers_used[convert<int>(token2.value)] = false;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//these functions are all for creating arm instructions based on the structure used to pass tokens around//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

string Parser::MOV(s_info Rd, s_info Op2){

    string text;

	text = "\tMOV" + condition + " r" + getRegNo(Rd) + ", ";
	
	if(Op2.type == "number"){
		text += "#" + Op2.value; 
	}
	else{
		text += "r" + getRegNo(Op2);

        //if a pointer is in the register MOVed from, mark it as also in the place MOVed to
        if(pointer_map.find(convert<int>(getRegNo(Op2))) != pointer_map.end()){
            pointer_map[convert<int>(getRegNo(Rd))] = pointer_map[convert<int>(getRegNo(Op2))];
        }
	}
	text += "\n";
	
	removeUsedTemp(Op2);
    
    return text;
}

//moves the value in a token into a temporary register for when an instruction can't use #number in an argument 
void Parser::convertToTemporary(s_info &token){
    s_info temp = {"temporary", findTempReg()};
    arm << MOV(temp, token);
    token = temp;
}

string Parser::convertToTemporaryNoPrint(s_info &token){
	s_info temp = {"temporary", findTempReg()};
	string text = MOV(temp, token);
	token = temp;
	return text;
}

string Parser::ORR(s_info Rd, s_info Rn, s_info Op2){
    string text;
	text = "\tORR" + condition + " r" + getRegNo(Rd) + ", r" + getRegNo(Rn);
    if(Op2.type == "variable" || Op2.type == "temporary"){
        text += ", r" + getRegNo(Op2);
    }
    else if(Op2.type == "number"){
       text += ", #" + Op2.value;
    }
    text += "\n";
    removeUsedTemp(Rn, Op2);
    return text;
}

string Parser::XOR(s_info Rd, s_info Rn, s_info Op2){
    string text;
	text = "\tXOR" + condition + " r" + getRegNo(Rd) + ", r" + getRegNo(Rn);
    if(Op2.type == "variable" || Op2.type == "temporary"){
        text += ", r" + getRegNo(Op2);
    }
    else if(Op2.type == "number"){
       text += ", #" + Op2.value;
    }
    text += "\n";
    removeUsedTemp(Rn, Op2);
    return text;
}

string Parser::AND(s_info Rd, s_info Rn, s_info Op2){
    string text;
	text = "\tAND" + condition + " r" + getRegNo(Rd) + ", r" + getRegNo(Rn);
    if(Op2.type == "variable" || Op2.type == "temporary"){
        text += ", r" + getRegNo(Op2);
    }
    else if(Op2.type == "number"){
       text += ", #" + Op2.value;
    }
    text += "\n";
    removeUsedTemp(Rn, Op2);
    return text;
}

string Parser::ADD(s_info Rd, s_info Rn, s_info Op2){
    string text;
	text = "\tADD" + condition + " r" + getRegNo(Rd) + ", r" + getRegNo(Rn);
    if(Op2.type == "variable" || Op2.type == "temporary"){
        text += ", r" + getRegNo(Op2);
    }
    else if(Op2.type == "number"){
       text += ", #" + Op2.value;
    }
    text += "\n";
    removeUsedTemp(Rn, Op2);
    return text;
}

string Parser::ADD(s_info Rd, s_info Rn, s_info Op2, int LSL){
    string text;
	text = "\tADD" + condition + " r" + getRegNo(Rd) + ", r" + getRegNo(Rn)
                   + ", r" + getRegNo(Op2) + ", LSL #" + convert<string>(LSL)
                   + "\n";
    removeUsedTemp(Op2);
    return text;
}
    

string Parser::SUB(s_info Rd, s_info Rn, s_info Op2){
    string text;
	text = "\tSUB" + condition + " r" + getRegNo(Rd) + ", r" + getRegNo(Rn);
    if(Op2.type == "variable" || Op2.type == "temporary"){
        text += ", r" + getRegNo(Op2);
    }
    else if(Op2.type == "number"){
       text += ", #" + Op2.value;
    }
    text += "\n";
    removeUsedTemp(Rn, Op2);
    return text;
}

string Parser::MUL(s_info Rd, s_info Rn, s_info Rm){
    string text;
	text = "\tMUL" + condition + " r" + getRegNo(Rd) + ", r" + getRegNo(Rn)
	               + ", r" + getRegNo(Rm) + "\n";
	removeUsedTemp(Rn, Rm);
    return text;
}

string Parser::SDIV(s_info Rd, s_info Rn, s_info Rm){
    string text;
	text = "\tSDIV" + condition + " r" + getRegNo(Rd) + ", r" + getRegNo(Rn)
                    + ", r" + getRegNo(Rm) + "\n";
    removeUsedTemp(Rn, Rm);
    return text;
}

string Parser::CMP(s_info Rn, s_info Op2){
    string text;
    text = "\tCMP" + condition + " r" + getRegNo(Rn) + ", ";
	if(Op2.type == "number"){
		text += "#" + Op2.value;
	}
	else{
		text += "r" + getRegNo(Op2);
	}
	text += "\n";
    return text;
}

string Parser::B(string condition, string label){
    string text;
    text = "\tB" + condition + " " + label + "\n";
    return text;
}

string Parser::BL(string condition, string label){
    string text;
    text = "\tBL" + condition + " " + label + "\n";
    return text;
}

string Parser::PUSH(string reglist){
    string text;
    text = "\tPUSH" + condition + " {" + reglist + "}" + "\n";
    return text;
}

string Parser::POP(string reglist){
    string text;
    text = "\tPOP" + condition + " {" + reglist + "}" + "\n";
    return text;
}



string Parser::LDR(s_info R1, s_info R2, string name){
	string text;
	if(R2.type == "string"){
        //add to the map of registers pointing to registers
        if(R2.value.substr(0,5) != "print" && getVariableInfo(name).array_size == 0)
        	pointer_map[convert<int>(getRegNo(R1))] = getVariableInfo(name).reg;
        	

	    text = "\tLDR" + condition + " r" + getRegNo(R1) + ", " + R2.value + "\n";
	}
	else{
	    text = "\tLDR" + condition + " r" + getRegNo(R1) + ", [r" + getRegNo(R2) + "]\n";
	}
	return text;
}

//overload LDR for arrays to use an offset
string Parser::LDR(s_info R1, s_info R2, s_info offset){
    string text;
    if(offset.type == "number"){
        text = "\tLDR" + condition + " r" + getRegNo(R1) + ", [r" + getRegNo(R2)
                       + ", #" + convert<string>(convert<int>(offset.value) * type_map[current_scope->variable_map[R2.value].type].size) + "]\n";
    }
    else{
        text = "\tLDR" + condition + " r" + getRegNo(R1) + ", [r" + getRegNo(R2)
                       + ", r" + getRegNo(offset)
                                                     //take logs to find LSL value to get the correct byte offset
                       + ", LSL #" + convert<string>(log(type_map[getVariableInfo(R2.value).type].size)/log(2)) + "]\n";
    }
    return text;
}

string Parser::STR(s_info &R1, s_info R2){
    string text;
    if(R1.type == "number"){
        convertToTemporary(R1);
    }
        
    if(R2.type == "string"){
        text = "\tSTR" + condition + " r" + getRegNo(R1) + ", " + R2.value + "\n";
    }
    else{
        text = "\tSTR" + condition + " r" + getRegNo(R1) + ", [r" + getRegNo(R2) + "]\n";
    }
    return text;
}

string Parser::STR(s_info &R1, s_info R2, s_info offset){
    string text;
    
    if(R1.type == "number"){
        convertToTemporary(R1);
    }
    if(offset.type == "number"){
        text = "\tSTR" + condition + " r" + getRegNo(R1) + ", [r" + getRegNo(R2)
                       + ", #" + convert<string>(convert<int>(offset.value) * type_map[getVariableInfo(R2.value).type].size) + "]\n";
    }
    else{
        text = "\tSTR" + condition + " r" + getRegNo(R1) + ", [r" + getRegNo(R2)
                       + ", r" + getRegNo(offset) 
                                                     //take logs to find LSL value to get the correct byte offset
                       + ", LSL #" + convert<string>(log(type_map[getVariableInfo(R2.value).type].size)/log(2)) + "]\n";
    }
    return text;
}


void Parser::armPrintf(){
    if(temp_registers[0].type != "string"){ error_list.push_back("line " + convert<string>(lexer.lineno()) + ": first argument of printf is not a string"); return; }

    if(temp_registers.size() > 4){ error_list.push_back("line " + convert<string>(lexer.lineno()) + ": printf with more than 4 arguments is not supported"); return; }
    
    printf_no++;
    word_list.push_back("print_args" + convert<string>(printf_no) + ": .word print_args_str" + convert<string>(printf_no));
    data_list.push_back("print_args_str" + convert<string>(printf_no) + ": .asciz " + temp_registers[0].value);
    
    s_info temp = {"temporary", "0"};
    s_info temp2 = {"string", "print_args" + convert<string>(printf_no)};
    
    arm << LDR(temp, temp2, "");
    
    s_info dest_reg;
    dest_reg.type = "temporary";
    for(int i = 1; i < temp_registers.size(); i++){
        dest_reg.value = convert<string>(i);
        arm << MOV(dest_reg, temp_registers[i]);
    }
    
    arm << BL(condition, "printf");
}

void Parser::armMalloc(){
    if(temp_registers.size() > 1){ error_list.push_back("line " + convert<string>(lexer.lineno()) + ": malloc cannot have more than 1 argument"); return; }

    s_info temp = {"temporary", "0"};
    arm << MOV(temp, temp_registers[0]);

    arm << BL(condition, "malloc");
}


void Parser::armFree(){
    if(temp_registers.size() > 1){ error_list.push_back("line " + convert<string>(lexer.lineno()) + ": free cannot have more than 1 argument"); return; }

    s_info temp = {"temporary", "0"};
    arm << MOV(temp, temp_registers[0]);

    arm << BL(condition, "free");
}


