#include "table.h"

VarList varTable[TABLE_SIZE] = {0};
FuncType funcTable[TABLE_SIZE] = {0};

// hash function by P.J.Weinberger
unsigned int hash_pjw(char *name) {
	unsigned int val = 0, i;
	for(; *name; ++name) {
		val = (val << 2) + *name;
		if(i = val & ~TABLE_SIZE) val = (val ^ (i >> 12)) & TABLE_SIZE;
	}
	return val;
}

// insertVar: 将变量符号插入符号哈希表
// return: -1: 不插入, 1: 该符号已存在, 0: 插入成功
int insertVar(VarList vl) {
	if(vl->name == NULL)
		return -1;
	
	unsigned int index = hash_pjw(vl->name);
	if(varTable[index] == NULL) {
		varTable[index] = vl;
	} else {
		VarList cur = varTable[index], pre;
		while(cur) {
			if(!strcmp(cur->name, vl->name))
				return 1;
			pre = cur;
			cur = cur->next;
		}
		pre->next = vl;
	}
	return 0;
}

// insertFunc: 将函数插入函数哈希表
// return:
int insertFunc(FuncType f) {
	if(f->name == NULL)
		return -1;
	
	unsigned int index = hash_pjw(f->name);
	if(funcTable[index] == NULL) {
		funcTable[index] = f;
		insertParams(f);
	} else {
		FuncType cur = funcTable[index], pre;
		while(cur) {
			if(!strcmp(cur->name, f->name)) {
				// 如果现在要插入一个定义
				if(f->isDefined == true) {
					// 如果该函数已经存在定义
					if(cur->isDefined) {
						return 1;
					// 如果没有定义,则已经声明,且参数或返回值不同
					} else if(!isTypeEqual(cur->returnType, f->returnType) || !isParamEqual(cur->param, f->param)) {
						return 2;
					} else {
						cur->isDefined = true;
						return 0;
					}
				// 如果现在要插入一个声明
				} else {
					// 如果插入的声明和之前已经存在的参数或返回值不同
					if(!isTypeEqual(cur->returnType, f->returnType) || !isParamEqual(cur->param, f->param)) {
						// 声明和定义不同
						if(cur->isDefined)
							return 3;
						// 声明和声明不同
						else
							return 4;
					} else {
						return 0;
					}
				}
			}
			pre = cur;
			cur = cur->next;
		}
		pre->next = f;
		insertParam(f);
	}
	return 0;
}

void insertParam(FuncType f) {
	VarList param = f->param;
	int ret_code = 0; // 返回状态码
	while(param) {
		ret_code = insertVar(param);
		if(ret_code == 1) {
			// Error type 3
			// 没有实现作用域
			printf("error type 3 at line %d: redefinition of variable '%s'\n", f->row, param->name);
		}
		param = param->next_param; // ?
	}
}

bool isTypeEqual(Type t1, Type t2) {
	if(t1->type == BASIC && t2->type == CONSTANT || t1->type == CONSTANT && t2->type == BASIC) {
		if(t1->type_info.basic != t2->type_info.basic) {
			return false;
		}
	} else if(t1->type != t2->type) {
		return false;
	} else {
		if(t1->type == BASIC) {
			if(t1->type_info.basic != t2->type_info.basic)
				return false;
		} else if(t1->type == ARRAY) {
			return isTypeEqual(t1->type_info.array.element, t2->type_info.array.element);
		} else if(t1->type == STRUCTURE) {
			// 存在无名结构体
			if(t1->type_info.structure->name == NULL || t2->type_info.structure->name == NULL) {
				return isParamEqual(t1->type_info.structure->varList, t2->type_info.structure->varList);
			}
			// 结构体不同名
			if(strcmp(t1->type_info.structure->name, t2->type_info.structure->name))
				return false;
			// 结构体同名
			else {
				return isParamEqual(t1->type_info.structure->varList, t2->type_info.structure->varList);
			}
		}
	}
	return true;
}

bool isParamEqual(VarList v1, VarList v2) {
	if(v1 == NULL && v2 == NULL)
		return true;
	if(v1 == NULL || v2 == NULL)
		return false;
	if(isTypeEqual(v1->type, v2->type))
		return isParamEqual(v1->next_param, v2->next_param);
	else
		return false;
}

VarList findSymbol(char *name) {
	unsigned int index = hash_pjw(name);
	if(varTable[index] == NULL)
		return NULL;
	VarList cur = varTable[index];
	while(cur) {
		if(!strcmp(cur->name, name))
			return cur;
		cur = cur->next;
	}
	return NULL;
	// ? next_param是干什么用的
}

FuncType findFunc(char *name) {
	unsigned int index = hash_pjw(name);
	if(funcTable[index] == NULL)
		return NULL;
	FuncType cur = funcTable[index];
	while(cur) {
		if(!strcmp(cur->name, name))
			return cur;
		cur = cur->next;
	}
	return NULL;
}

// 根据创建好的语法树，构建语义分析

// Program -> ExtDefList
void Program(Node *n) {
	ExtDefList(n->children);
}

// ExtDefList -> ExtDef ExtDefList | ε
void ExtDefList(Node *n) {
	Node *child = n->children;
	if(child) {
		ExtDef(child);
		ExtDefList(child->next);
	}
}

// ExtDef -> Specifier ExtDecList SEMI | Specifier SEMI
// 		   | Specifier FunDec CompSt | Specifier FunDec SEMI
void ExtDef(Node *n) {
	Node *child = n->children; // child: Specifier
	Type t = Specifier(child);
	if(!strcmp(child->next->name, "ExtDecList")) {
		ExtDecList(child->next, t);
		return;
	} else if(!strcmp(child->next->name, "SEMI")) {
		return;
	} else if(!strcmp(child->next->name, "FunDec")) {
		FuncType f = FunDec(child->next, t);
		// 此时f已经获得了返回值和参数类型
		if(f) {
			if(!strcmp(child->next->next->name, "CompSt")) { // 函数定义
				f->isDefined = true;
				int ret_code = insertFunc(f);
				if(ret_code == 1) {
					// 函数重定义
					printf("error type 4 at line %d: redefinition of function '%s'\n", f->row, f->name);
				} else if(ret_code == 2) {
					// 当前定义和先前的声明不同
					printf("error type 19 at line %d: definition of function '%s' is different from the previous declaration\n", f->row, f->name);
				}
				return;
			} else if(!strcmp(child->next->next->name, "SEMI")) { // 函数声明
				f->isDefined = false;
				// 此时已知f的返回类型、参数类型且是声明
				int ret_code = insertFunc(f);
				if(ret_code == 3) {
					// 当前声明和先前的定义不同
					printf("error type 19 at line %d: declaration of function '%s' is different from the previous definition\n", f->row, f->name);
				} else if(ret_code == 4) {
					// 当前声明和先前的声明不同
					printf("error type 19 at line %d: declaration of function '%s' is different from the previous declaration\n", f->row, f->name);
				}
				return;
			}
		}
	}
	printf("[Internal Error] error in semantic analysis in ExtDef()\n");
	return;
}

// ExtDecList -> VarDec | VarDec COMMA ExtDecList
void ExtDecList(Node *n, Type t) {
	Node *child = n->children;
	VarList v = VarDec(child);
	if(!strcmp(child->next->name, "COMMA") && !strcmp(child->next->next->name, "ExtDecList")) {
		ExtDecList(child->next->next, t);
	}
	printf("[Internal Error] error in semantic analysis in ExtDecList()\n");
	return;
}

Type Specifier(Node *n) {
	
}



