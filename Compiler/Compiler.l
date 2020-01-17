%{
#include "Parserbase.h"
%}

%option noyywrap
%option yylineno

ID		    [a-zA-Z_]+
NUMBER		[0-9]+

%%

"//".*			              //ignore comments by not returning
"/*"(("*"[^/])|[^*])*"*/"
" "
"\n"            
"\t"

"#include".*		{return ParserBase::INCLUDE;}

"typedef"       	{return ParserBase::TYPEDEF;}
"struct"            {return ParserBase::STRUCT;}
"sizeof"        	{return ParserBase::SIZEOF;}

"\"".*"\""			{return ParserBase::STRING;}
'(.|"\\n"|"\\t")'	{return ParserBase::CHARACTER;}

"\("		   		{return ParserBase::OPEN_ROUND;}
"\)"	    		{return ParserBase::CLOSE_ROUND;}
"{"			    	{return ParserBase::OPEN_CURLY;}
"}"			    	{return ParserBase::CLOSE_CURLY;}
"["			    	{return ParserBase::OPEN_SQUARE;}
"]"			    	{return ParserBase::CLOSE_SQUARE;}
    
"="			    	{return ParserBase::EQUALS;}
"++"            	{return ParserBase::PLUSPLUS;}
"+"			    	{return ParserBase::PLUS;}
"--"            	{return ParserBase::MINUSMINUS;}
"-"             	{return ParserBase::MINUS;}
"*"			    	{return ParserBase::STAR;}
"/"             	{return ParserBase::FSLASH;}

"=="|"!="|"<"|"<="|">"|">=" {return ParserBase::COMPARE;}
    
","			    	{return ParserBase::COMMA;}
";"			    	{return ParserBase::DELIM;}

"||"            	{return ParserBase::OROR;}
"&&"            	{return ParserBase::ANDAND;}
"|"             	{return ParserBase::OR;}
"^"             	{return ParserBase::XOR;}
"&"			    	{return ParserBase::AND;}
"if"            	{return ParserBase::IF;}
"else"          	{return ParserBase::ELSE;}
"while"         	{return ParserBase::WHILE;}
"do"            	{return ParserBase::DO;}
"for"           	{return ParserBase::FOR;}

"return"        	{return ParserBase::RETURN;}

{ID}				{return ParserBase::ID;}
{NUMBER}			{return ParserBase::NUMBER;}

