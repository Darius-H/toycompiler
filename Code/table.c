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
// return: -1: 为空,不插入, 1: 该符号已存在, 0: 插入成功
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

// insertFunc: 将函数插入函数哈希表。注意c语言不支持重载
/* return: -1: 不插入(函数名为NULL), 1: 函数已被定义, 2:插入定义时定义和声明不匹配 
3:插入声明时定义和声明不匹配 4:重复的声明参数或返回值不相同 0: 插入成功 */
int insertFunc(FuncType f) {
	if(f->name == NULL)
		return -1;
	
	unsigned int index = hash_pjw(f->name);
	if(funcTable[index] == NULL) {
		funcTable[index] = f;
		insertParam(f);
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
				// 如果现在要插入一个声明,注意重复的声明不会报错
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
			printf("error type 3 at line %d: redefinition of variable '%s'\n", f->row, param->name);
		}
		param = param->next_field;
	}
}

//类型检查
bool isTypeEqual(Type t1, Type t2) {
	if(t1->type != t2->type) {
		return false;
	} else {//变量类型相同
		if(t1->type == BASIC) {
			if(t1->type_info.basic != t2->type_info.basic)
				return false;
		} else if(t1->type == ARRAY) {
			return isTypeEqual(t1->type_info.array.element, t2->type_info.array.element);
		} else if(t1->type == STRUCTURE) {
			// 存在无名结构体（匿名结构体和其它结构体都不等价,gcc是如此）
			if(t1->type_info.structure->name == NULL || t2->type_info.structure->name == NULL) {
				return false;
			}
			// 结构体不同名（结构体不同名即不等价,gcc是如此）
			if(strcmp(t1->type_info.structure->name, t2->type_info.structure->name))
				return false;
		}
	}
	return true;
}
//检查参数列表是否相同(只检查参数类型，允许参数名不同)
bool isParamEqual(VarList v1, VarList v2) {
	if(v1 == NULL && v2 == NULL)
		return true;
	if(v1 == NULL || v2 == NULL)
		return false;
	if(isTypeEqual(v1->type, v2->type))
		return isParamEqual(v1->next_field, v2->next_field);
	else
		return false;
}
//检查变量表中是否存在该变量，不存在则返回null，存在则返回地址
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
}
//检查函数表中是否存在该函数，不存在则返回null，存在则返回地址
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

// Specifier -> TYPE | StructSpecifier
Type Specifier(Node *n) {
	Node *child = n->children;
	if(!strcmp(child->name, "TYPE")) {
		Type t = malloc(sizeof(struct Type_));
		t->type = BASIC;
		if(!strcmp(child->value, "int")) {
			t->type_info.basic = INT;
		} else if(!strcmp(child->value, "float")) {
			t->type_info.basic = FLOAT;
		} else {
			t = NULL;
		}
		return t;
	} else if(!strcmp(child->name, "StructSpecifier")) {
		Type t = StructSpecifier(child);
		return t;
	}
}

// StructSpecifier -> STRUCT OptTag LC DefList RC
// 					| STRUCT Tag
Type StructSpecifier(Node *n) {
	Node *child = n->children;
	if(!strcmp(child->name, "STRUCT")) {
		if(!strcmp(child->next->name, "OptTag")) {
			Type t = malloc(sizeof(struct Type_));
			t->type = STRUCTURE;
			t->type_info.structure = malloc(sizeof(struct Structure_));
			t->type_info.structure->name = NULL;
			t->type_info.structure->varList = NULL;

			Node *OptTag_node = child->next;
			// 如果OptTag有孩子,说明是有名结构定义
			if(child->children) {
				t->type_info.structure->name = malloc(sizeof(OptTag_node->children->value)+1);
				strcpy(t->type_info.structure->name, OptTag_node->children->value);
			} // 否则OptTag无孩子,说明是无名结构定义,什么也不做
			Node *tmp = OptTag_node->next;
			if(!strcmp(tmp->name, "LC")) {
				Node *DefList_node = tmp->next;
				if(!strcmp(DefList_node->name, "DefList")) {
					VarList v = DefList(DefList_node);
					t->type_info.structure->varList = v;
					// 无名结构体,无需插入至哈希表
					if(t->type_info.structure->name == NULL) {
						return t;
					}
					// 将定义好的结构体本身插入哈希表
					// 结构体内部的变量不在此处插入
					VarList tmp = malloc(sizeof(struct VarList_));
					tmp->type = t;
					tmp->name = malloc(sizeof(t->type_info.structure->name));
					strcpy(tmp->name, t->type_info.structure->name);
					int ret_code = insertVar(tmp);
					// 结构体的名字和定义过的结构体或变量的名字重复
					if(ret_code == 1) {
						printf("error type 16 at line %d: the name of struct '%s' duplicates name of another variable or struct", DefList_node->row, t->type_info.structure->name);
						return NULL;
					}
					return t;
				}
			}
		// 根据已有的结构定义新的结构变量
		} else if(!strcmp(child->next->name, "Tag")) {
			// 查找变量符号表中是否已经定义了该结构
			VarList tmp = findSymbol(child->next->children->value);
			if(!tmp || tmp->type->type != STRUCTURE || strcmp(tmp->name, tmp->type->type_info.structure->name)) {
				// 结构体未定义
				printf("error type 17 at line %d: undefined struct type '%s'\n", child->next->row, child->next->children->value);
				return NULL;
			} else {
				return tmp->type;
			}
		}
	}
	printf("[Internal Error] error in semantic analysis in StructSpecifier()\n");
	return NULL;
}

VarList DefList(Node *n) {

}
