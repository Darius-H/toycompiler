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
	enum { BASIC, ARRAY, STRUCTURE } type;
	union
	{
		// 基本类型信息
		enum { INT, FLOAT } basic;
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
	VarList next_field; // 结构体中连接所有成员变量的链表
};

struct FuncType_
{
	char *name;//函数名
	bool isDefined;//函数可只声明不定义
	int row;//函数最初被识别时在编辑器中的行，可能是声明也可能是定义
	Type returnType;
	VarList param; // 参数列表
	FuncType next;// 哈希表同一表项中所构成的链表
};


#endif