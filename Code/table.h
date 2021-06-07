#ifndef __PARSER_TABLE__
#define __PARSER_TABLE__

#include "stdio.h"
#include "stddef.h"
#include "string.h"
#include "stdbool.h"
#include "TreeNode.h"

#define TABLE_SIZE 0x3fff

typedef struct Type_* Type;
typedef struct Structure_* Structure;
typedef struct VarList_* VarList;
typedef struct FuncType_* FuncType;

struct Type_
{
	enum { BASIC, ARRAY, STRUCTURE, CONSTANT } type;
	union
	{
		// 基本类型信息
		int basic;
		// 数组类型信息: 元素类型与数组大小
		struct { Type element; int size; } array;
		// 结构体类型信息
		Structure structure;
	} type_info;
};

struct Structure_
{
	char *name;
	VarList varList;
};


struct VarList_
{
	char *name;
	Type type;
	// open hashing
	VarList next; // 哈希表同一表项中所构成的链表
	VarList next_param;
};

struct FuncType_
{
	char *name;
	bool isDefined;
	int row;
	Type returnType;
	VarList param; // 参数列表
	FuncType next;
};


#endif