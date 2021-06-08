#ifndef _TREE_NODE_H
#define _TREE_NODE_H
#define MAX_NAME 32
#define MAX_VALUE 32
#include <stdio.h> 
#include <stdlib.h>
#include <string.h> 
/*
 * some func about node
 * */
typedef struct TreeNode{ 
	int row; 
	char name[MAX_NAME];
	char value[MAX_VALUE]; 
	struct TreeNode *children; 
	struct TreeNode *next; 
}Node;

typedef Node* PtrToNode;

char* Mystrcpy(char* des,char* src,int maxsize);//有长度限制的strcpy

PtrToNode NewNode(char* name,char* value);//name是节点在语法分析中的名称，value用于语义分析

void addChild(PtrToNode f,PtrToNode c);//sibling式树。插入子节点

void printTree(PtrToNode r,int count);//打印整棵树

#endif
