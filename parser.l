%{
	#include"node.h"
	#include"parser.tab.h"
	int errorFlag=1;
%}

space [ \t]+
/* identifier */
ID_character [_0-9a-zA-Z]
Pre_ID [_a-zA-Z]
id {Pre_ID}{ID_character}*
/*int constant and float constant*/
O   [0-7]
D   [0-9]
D_NOZERO [1-9]
E   ([Ee][+-]?{D}+)
H   [a-fA-F0-9]
HP  (0(x|X))
HEX_CONSTANT {HP}{H}+ 
OCT_CONSTANT 0{O}+
DEC_CONSTANT 0|{D_NOZERO}{D}*
float {D}*\.{D}+{E}?
integer {HEX_CONSTANT}|{OCT_CONSTANT}|{DEC_CONSTANT}

%option yylineno

%%
{space} 	{/*space*/}
{integer} 	{yylval.node=initNode("INT",yytext);return INT;}
{float} 	{/*printf("float:%s\n",yytext)*/yylval.node=initNode("FLOAT",yytext);return FLOAT;}
";" 		{yylval.node=initNode("SEMI",yytext);return SEMI;}
"," 		{yylval.node=initNode("COMMA",yytext);return COMMA;}
"=" 		{yylval.node=initNode("ASSIGNOP",yytext);return ASSIGNOP;}
">" 		{yylval.node=initNode("RELOP",yytext);return RELOP;}
"<" 		{yylval.node=initNode("RELOP",yytext);return RELOP;}
">=" 		{yylval.node=initNode("RELOP",yytext);return RELOP;}
"<=" 		{yylval.node=initNode("RELOP",yytext);return RELOP;}
"==" 		{yylval.node=initNode("RELOP",yytext);return RELOP;}
"!=" 		{yylval.node=initNode("RELOP",yytext);return RELOP;}
"+" 		{yylval.node=initNode("PLUS",yytext);return PLUS;}
"-" 		{yylval.node=initNode("MINUS",yytext);return MINUS;}
"*" 		{yylval.node=initNode("MUL",yytext);return MUL;}
"/" 		{yylval.node=initNode("DIV",yytext);return DIV;}
"&&" 		{yylval.node=initNode("AND",yytext);return AND;}
"||" 		{yylval.node=initNode("OR",yytext);return OR;}
"." 		{yylval.node=initNode("DOT",yytext);return DOT;}
"!" 		{yylval.node=initNode("NOT",yytext);return NOT;}
int|float 	{yylval.node=initNode("TYPE",yytext);return TYPE;}
"(" 		{yylval.node=initNode("LP",yytext);return LP;}
")" 		{yylval.node=initNode("RP",yytext);return RP;}
"[" 		{yylval.node=initNode("LB",yytext);return LB;}
"]" 		{yylval.node=initNode("RB",yytext);return RB;}
"{" 		{yylval.node=initNode("LC",yytext);return LC;}
"}" 		{yylval.node=initNode("RC",yytext);return RC;}
struct 		{yylval.node=initNode("STRUCT",yytext);return STRUCT;}
return 		{yylval.node=initNode("RETURN",yytext);return RETURN;}
if			{yylval.node=initNode("IF",yytext);return IF;}
else 		{yylval.node=initNode("ELSE",yytext);return ELSE;}
while 		{yylval.node=initNode("WHILE",yytext);return WHILE;}	
\n			{/*line end*/}
{id} 		{yylval.node=initNode("ID",yytext);return ID;}
.			{errorFlag=0;printf("Error type A at line %d: Mysterious character '%s'\n",yylineno,yytext);}
%%

int yywrap(){
   return 1;
}
