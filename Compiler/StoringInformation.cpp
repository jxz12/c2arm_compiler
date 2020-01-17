#include "Parser.h"

using namespace std;




//creates labels with an incrementing suffix to disambiguate
string Parser::createLabel(){
    label_no++;
    return "label" + convert<string>(label_no);
}


///////////////
//STACK_TRACK//
///////////////

//sets the array of bools used for registers as false
void Parser::setRegUsedFalse(){
	for(int i = 0; i < 13; i++){
		current_reg->registers_used[i] = false;
	}
}

//finds a free register, looks from r1 and up to separate variables and temporaries
int Parser::findVarReg(){
	int i;
	for(i = 4; i < 13; i++){
		if(current_reg->registers_used[i] == false){
			current_reg->registers_used[i] = true;
			return i;
		}
	}
	error_list.push_back("line " + convert<string>(lexer.lineno()) + ": ran out of variable registers"); return -1; 
}

//finds a free register, looks from r12 and down to separate variables and temporaries
string Parser::findTempReg(){
	int j;
	for(j = 12; j > 3; j--){
		if(current_reg->registers_used[j] == false){
			current_reg->registers_used[j] = true;
			return convert<string>(j);
		}
	}
	error_list.push_back("line " + convert<string>(lexer.lineno()) + ": ran out of temporary registers"); return "-1";
}





/////////
//SCOPE//
/////////

//adds a variable to a vector without including it's type yet
void Parser::addVariableNoType(s_info token, int array_size){
    variable_info temp;

    //type is temporarily set to an empty string because type is only available further up the tree
    temp.type = "";
    temp.reg = findVarReg();
    temp.array_size = array_size;
    temp.scope_no = current_scope->scope_no;

    if(token.type == "pointer") temp.pointer = true;
    else temp.pointer = false;
    
    //check if name is available
    if(current_scope->variable_map.find(token.value) == current_scope->variable_map.end()){
        current_scope->variable_map[token.value] = temp;
    }
	else{
	    error_list.push_back("line " + convert<string>(lexer.lineno()) + ": variable name '" + token.value + "' already defined in current scope"); return;
	}
}

//checks whether or not the type exists in the type_map
bool Parser::checkType(string type){
    bool match = false;
    for(map<string, t_info>::const_iterator it = type_map.begin();
        it != type_map.end(); ++it){

        if(it->first == type){
            match = true;
            break;
        }
    }
    return match;
}

//adds the type to the variables declared
void Parser::addVariableType(string type){

    if(!checkType(type)){
    	error_list.push_back("line " + convert<string>(lexer.lineno()) + ": type '" + type + "' not found"); return;
    }

    //check if type is pointer already
    bool pointer = false;
    if(type_map[type].pointer){
        pointer = true;
    }
	for(int i = 0; i < temp_strings.size(); i++){
        if(current_scope->variable_map[temp_strings[i]].pointer && pointer){
            error_list.push_back("line " + convert<string>(lexer.lineno()) + ": type '" + type + "' is already a pointer, pointers to pointers are not supported"); return; 
        }
        current_scope->variable_map[temp_strings[i]].pointer = pointer || current_scope->variable_map[temp_strings[i]].pointer;
	    current_scope->variable_map[temp_strings[i]].type = type;
	}
}

//goes through the scope linked list to get in information on a variable
variable_info Parser::getVariableInfo(string name){
	
    scope* temp_scope = current_scope;

	//check if the name exists in the current scope
	do{
		if(temp_scope->variable_map.find(name) == temp_scope->variable_map.end()){
		
			//if not found, go up a scope
			temp_scope = temp_scope->scope_above;
		}
		else{
			return temp_scope->variable_map[name];
		}
	}
	//keep going up scopes until no above scope exists
	while(temp_scope->scope_above != NULL);
	
	//if not found, return error
	variable_info temp = {"", -1};
	error_list.push_back("line " + convert<string>(lexer.lineno()) + ": variable '" + name + "' not defined in current scope"); return temp;
}

//finds the register that a token's information is stored in
string Parser::getRegNo(s_info token){
	if(token.type == "variable"){
		return convert<string>(getVariableInfo(token.value).reg);
	}
	else if(token.type == "temporary"){
		return token.value;
	}
}

//prints the map of variables stored
void Parser::printVariables(map<string, variable_info> variable_map){
        
    for(map <string, variable_info>::const_iterator it = variable_map.begin();
        it != variable_map.end(); ++it){
        
        arm << "name:" << it->first << ", type:" << it->second.type <<  ", reg:" << it->second.reg << ", array_size:" << it->second.array_size;
        if(it->second.pointer) arm << ", pointer";
        arm << endl;
    }
}

//prints a vector of strings received
void Parser::printList(vector<string> list){
    for(int i = 0; i < list.size(); i++){
            arm << list[i] << endl;
    }
}









/////////
//TYPES//
/////////

void Parser::initialiseTypeMap(){
    t_info temp = {1, false};
    type_map["char"] = temp;
    temp.size = 4;
    type_map["int"] = temp;
}

//adds a type to type_map
void Parser::addType(string type, int size, bool pointer){
    if(type_map.find(type) == type_map.end()){
        if(pointer){
            t_info temp = {4, true};
            type_map[type] = temp;
        }
        else{
            t_info temp = {size, false};
            type_map[type] = temp;
        }
    }
    else{
        error_list.push_back("line " + convert<string>(lexer.lineno()) + ": type name '" + type + "' already defined"); return;
    }
}

//I didn't have time to implement structures so this is unfinished
/*
void Parser::createStruct(string name){
	map<string, t_info> temp_type_map;
	int size = 0;
	for(map<string, variable_info>::const_iterator it = current_scope->variable_map.begin(); it != current_scope->variable_map.end(); ++it){
		temp_type_map[it->first] = type_map[it->second.type];
		size += type_map[it->second.type].size;
	}
	t_info temp = {size, false, temp_type_map};
	type_map[name] = temp;
    return;
}
*/

string Parser::findSizeof(string type){
    return convert<string>(type_map[type].size);
}













/////////////////////
//STORING FUNCTIONS//
/////////////////////

//keeps track of the parameters in a function definition
void Parser::addParameter(string type, s_info token){
	if(temp_variable_map.find(token.value) == temp_variable_map.end()){
		if(!checkType(type)){
    		error_list.push_back("line " + convert<string>(lexer.lineno()) + ": type '" + type + "' not found"); return;
    	}
		variable_info temp;
		temp.type = type;
		temp.reg = temp_count++;
        temp.array_size = 0;
        
        //add 1 to scope_no because the curly bracket hasn't been seen yet
        temp.scope_no = current_scope->scope_no + 1;
        if(token.type == "pointer") temp.pointer = true;
        else temp.pointer = false;

		temp_variable_map[token.value] = temp;
	}
	else{
		error_list.push_back("line " + convert<string>(lexer.lineno()) + ": function parameter '" + token.value + "' already defined"); return;
	}
}

//stores the name, return type, parameters needed of a function in a map
void Parser::storeFunction(string name, string return_type, bool prototype){
	//if the name hasn't been seen before, add it to the map
	if(function_map.find(name) == function_map.end()){
		function_info temp;
		temp.return_type = return_type;
		temp.defined = !prototype;
		temp.parameter_map = temp_variable_map;
		function_map[name] = temp;
	}
	
	//if the definition has already been created and another is attempted
	else if(function_map[name].defined && !prototype){
		error_list.push_back("line " + convert<string>(lexer.lineno()) + ": function '" + name + "' already defined"); return;
	}
	
	//if a function of the same name as a protoype has been created, do nothing, but set the defined flag to true
	else if(function_map.find(name) != function_map.end() && !prototype){
		function_map[name].defined = true;
		return;
	}
	
}







