#ifndef UTIL_H
#define UTIL_H

#include <sstream>
#include <vector>
#include <map>
#include <cmath>

template<typename T, typename T2>
T convert(const T2& in)
{
    std::stringstream buf;
    buf << in;
    T result;
    buf >> result;
    return result;
}

//a structure to store the attributes of a token
//type stores whether a variable is temporary or declared or just data
//for temporary variables, value stores the register it exists in
//for declared variables, value stores the name
//for data e.g. a number, value stores the actual value
struct s_info{
	std::string type;
	std::string value;
};

//a structure to store the type of each variable and which register it's assigned to
struct variable_info{
	std::string type;
	int reg;
    int array_size;
    bool pointer;
    int scope_no;
};

//arrays of bools to keep track of which registers have been stored to the stack
struct stack_track{
    stack_track* stack_above;
    bool registers_used[13];
};

//a structure to separate scopes created by {}, with a pointer to allow it to access any scopes within it's scope e.g. loops
struct scope{
    scope* scope_above;
    std::map <std::string, variable_info> variable_map;
    int scope_no;
};

struct function_info{
	std::string return_type;
	bool defined;
	std::map<std::string, variable_info> parameter_map;
};

struct t_info{
    int size;
    bool pointer;
};

struct struct_info{
	std::map<std::string, t_info> members;
};

#endif // UTIL_H
