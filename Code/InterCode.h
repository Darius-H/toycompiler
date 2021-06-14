#ifndef __INTER_CODE__
#define __INTER_CODE__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Operand_* Operand;
typedef struct InterCode_* InterCode;

struct Operand_ {
	enum {
		VARIABLE, CONSTANT, ADDRESS,

			

	} kind;
	union {
		int var_no;
		char* value; //?
		Operand name; //?
	} u;
	Operand next; //?
};

struct InterCode_ {
	enum {
		LABEL, FUNCTION, ASSIGN, 
		ADD, SUB, MUL, DIV, RIGHTAT,
		GOTO, IFGOTO, RETURN, DEC,
		ARG, CALL, PARAM,
		READ, WRITE
	} kind;

	union {
		// LABEL, FUNCTION, GOTO, RETURN, ARG, PARAM, READ, WRITE
		struct { Operand op; } unary;
		// ASSIGN, RIGHTAT, CALL
		struct { Operand left, right; } assign;
		// ADD, SUB, MUL, DIV
		struct { Operand result, op1, op2; } binop;
		// IFGOTO
		struct { Operand t1; char* op; Operand t2, label; } ifgoto;
		// DEC
		struct { Operand op; int size; } dec;
	} u;
	InterCode prev, next;
};

#endif