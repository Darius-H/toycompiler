#include "table.h"

VarType varTable[TABLE_SIZE] = {0};
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

void printVarTable(void) {
	int i;
	for(i = 0; i < TABLE_SIZE; i++) {
		if(varTable[i]) {
			VarType cur = varTable[i];
			printf("index: %d\n", i);
			while(cur) {
				printf("  name: %s, type: %s\n", cur->name, Type2String(cur->type));
				cur = cur->next;
			}
		}
	}
}

void printFuncTable(void) {
	int i;
	for(i = 0; i < TABLE_SIZE; i++) {
		if(funcTable[i]) {
			FuncType cur = funcTable[i];
			printf("index: %d\n", i);
			while(cur) {
				printf("  name: %s\n", cur->name);
				printf("    isDefined: %d\n", cur->isDefined);
				printf("    row: %d\n", cur->row);
				printf("    returnType: %s\n", Type2String(cur->returnType)); 
				printf("    param:\n");
				VarType tmp = cur->param;
				while(tmp) {
					printf("      name: %s, type: %s\n", tmp->name, Type2String(tmp->type));
					tmp = tmp->next_field;
				}
				cur = cur->next;
			}
		}
	}
}

void initHashTable(void) {
	// 需要在符号表中预先添加read和write,因为这两个函数中间代码的生成和普通函数是不同的
	// int read(void); 返回值为读入的整数值
	FuncType read = malloc(sizeof(struct FuncType_));
	read->name = malloc(5);
	strcpy(read->name, "read");
	read->isDefined = true;
	read->row = 0;
	read->returnType = malloc(sizeof(struct Type_));
	read->returnType->type = BASIC;
	read->returnType->type_info.basic = INT_TYPE;
	read->param = NULL;
	read->next = NULL;
	insertFunc(read);

	// int write(int); 参数为要输出的整数值,返回值固定为0
	FuncType write = malloc(sizeof(struct FuncType_));
	write->name = malloc(6);
	strcpy(write->name, "write");
	write->isDefined = true;
	write->row = 0;
	write->returnType = malloc(sizeof(struct Type_));
	write->returnType->type = BASIC;
	write->returnType->type_info.basic = INT_TYPE;
	write->param = malloc(sizeof(struct VarType_));
	write->param->name = malloc(20);
	strcpy(write->param->name, "write_param");
	write->param->type = write->returnType;
	write->param->next_field = NULL;
	write->next = NULL;
	insertFunc(write);
	
}

// insertVar: 将变量符号插入符号哈希表
// return: -1: 为空,不插入, 1: 该符号已存在, 0: 插入成功
int insertVar(VarType vl) {
	if(vl->name == NULL)
		return -1;
	
	unsigned int index = hash_pjw(vl->name);
	//printf("inserVar:%s %d\n",vl->name,index);
	if(varTable[index] == NULL) {
		varTable[index] = vl;
	} else {
		VarType cur = varTable[index], pre;
		while(cur) {
			if(!strcmp(cur->name, vl->name))//重名变量
				return REDEFINE_ERROR;
			pre = cur;
			cur = cur->next;
		}
		pre->next = vl;
	}
	return INSERT_SUCCESS;//成功插入
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
						return REDEFINE_ERROR;
					// 如果没有定义,则已经声明,且参数或返回值不同
					} else if(!isTypeEqual(cur->returnType, f->returnType) || !isParamEqual(cur->param, f->param)) {
						return DEF_MISMATCH_DEC;
					} else {
						cur->isDefined = true;
						return INSERT_SUCCESS;
					}
				// 如果现在要插入一个声明,注意重复的声明不会报错
				} else {
					// 如果插入的声明和之前已经存在的参数或返回值不同
					if(!isTypeEqual(cur->returnType, f->returnType) || !isParamEqual(cur->param, f->param)) {
						// 声明和定义不同
						if(cur->isDefined)
							return DEC_MISMATCH_DEF;
						// 声明和声明不同
						else
							return DEC_MISMATCH_DEC;
					} else {
						return INSERT_SUCCESS;
					}
				}
			}
			pre = cur;
			cur = cur->next;
		}
		pre->next = f;
		insertParam(f);
	}
	return INSERT_SUCCESS;
}

void insertParam(FuncType f) {
	VarType param = f->param;
	int ret_code = 0; // 返回状态码
	while(param) {
		ret_code = insertVar(param);
		if(ret_code == REDEFINE_ERROR) {
			// Error type 3
			printf("Error type 3 at line %d: Redefinition of variable '%s'\n", f->row, param->name);
		}
		param = param->next_field;
	}
}

//类型检查
bool isTypeEqual(Type t1, Type t2) {
	if(t1==NULL||t2==NULL){
		printf("ERROR: Type param is NULL in isTypeEqual function!\n");
		return false;
	}
	if((t1->type==BASIC&&t2->type==CONSTANT)||(t1->type==CONSTANT&&t2->type==BASIC)){//变量和常数
		return t1->type_info.basic==t2->type_info.basic;
	}
	else if(t1->type != t2->type) {
		return false;
	} else {//变量类型相同
		if(t1->type == BASIC) {//变量和变量
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
bool isParamEqual(VarType v1, VarType v2) {
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
VarType findSymbol(char *name) {
	unsigned int index = hash_pjw(name);
	if(varTable[index] == NULL)
		return NULL;
	VarType cur = varTable[index];
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
	printf("ExtDefList\n");
	if(n) {
		Node *child = n->children;
		if(child) {
			ExtDef(child);
			if(child->next)
				ExtDefList(child->next);
		}
	}
}

// ExtDef -> Specifier ExtDecList SEMI | Specifier SEMI
// 		   | Specifier FunDec CompSt | Specifier FunDec SEMI
void ExtDef(Node *n) {
	printf("ExtDef\n");
	Node *child = n->children; // child: Specifier
	if(!child)
		return;
	Type t = Specifier(child);
	if(!strcmp(child->next->name, "ExtDecList")) {//variable
		ExtDecList(child->next, t);
		return;
	} else if(!strcmp(child->next->name, "SEMI")) {//STRUCT declare
		return;
	} else if(!strcmp(child->next->name, "FunDec")) {
		FuncType f = FunDec(child->next, t);//t作为返回值类型传入
		// 此时f已经获得了返回值和参数类型
		if(f) {
			if(!strcmp(child->next->next->name, "CompSt")) { // 函数定义
				f->isDefined = true;
				int ret_code = insertFunc(f);
				if(ret_code == REDEFINE_ERROR) {
					// 函数重定义
					printf("Error type 4 at line %d: Redefinition of function '%s'\n", f->row, f->name);
				} else if(ret_code == DEF_MISMATCH_DEC) {
					// 当前定义和先前的声明不同
					printf("Error type 19 at line %d: Definition of function '%s' is different from the previous declaration\n", f->row, f->name);
				}
				CompSt(child->next->next,t);
				return;
			} else if(!strcmp(child->next->next->name, "SEMI")) { // 函数声明
				f->isDefined = false;
				// 此时已知f的返回类型、参数类型且是声明
				int ret_code = insertFunc(f);
				if(ret_code == 3) {
					// 当前声明和先前的定义不同
					printf("Error type 19 at line %d: Declaration of function '%s' is different from the previous definition\n", f->row, f->name);
				} else if(ret_code == 4) {
					// 当前声明和先前的声明不同
					printf("Error type 19 at line %d: Declaration of function '%s' is different from the previous declaration\n", f->row, f->name);
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
	if(!child)
		return;
	VarType v = VarDec(child, t, FROM_GLOBAL);
	if(child->next) {
		if(!strcmp(child->next->name, "COMMA") && !strcmp(child->next->next->name, "ExtDecList")) {
			ExtDecList(child->next->next, t);
		}	
	} else {
		return;
	}
}

// Specifier -> TYPE | StructSpecifier
Type Specifier(Node *n) {
	printf("Specifier\n");
	Node *child = n->children;
	if(!child)
		return NULL;
	if(!strcmp(child->name, "TYPE")) {
		Type t = (Type)malloc(sizeof(struct Type_));
		if(!t)
			return NULL;
		t->type = BASIC;
		if(!strcmp(child->value, "int")) {
			t->type_info.basic = INT_TYPE;
		} else if(!strcmp(child->value, "float")) {
			t->type_info.basic = FLOAT_TYPE;
		}
		return t;
	} else if(!strcmp(child->name, "StructSpecifier")) {
		Type t = StructSpecifier(child);
		return t;
	}
	return NULL;
}

// StructSpecifier -> STRUCT OptTag LC DefList RC
// 					| STRUCT Tag
//OptTag和Tag不需要定义函数，直接在StructSpecifier中就解决了
Type StructSpecifier(Node *n) {
	Node *child = n->children;
	if(!strcmp(child->name, "STRUCT")) {
		if(!strcmp(child->next->name, "OptTag")) {//首次定义结构体变量
			Type t = malloc(sizeof(struct Type_));
			t->type = STRUCTURE;
			t->type_info.structure = malloc(sizeof(struct Structure_));
			t->type_info.structure->name = NULL;
			t->type_info.structure->varList = NULL;

			Node *OptTag_node = child->next;
			// 如果OptTag有孩子,说明是有名结构定义(OptTag->ID)
			if(OptTag_node->children) {
				t->type_info.structure->name = malloc(sizeof(OptTag_node->children->value)+1);//ID是终结符，value就是ID名
				strcpy(t->type_info.structure->name, OptTag_node->children->value);
			} // 否则OptTag无孩子(OpttTag->e),说明是无名结构定义,什么也不做
			Node *tmp = OptTag_node->next;
			if(!strcmp(tmp->name, "LC")) {//如果是首次定义结构体，定义内部结构
				Node *DefList_node = tmp->next;
				if(!strcmp(DefList_node->name, "DefList")) {
					VarType v = DefList(DefList_node,FROM_STRUCT);
					t->type_info.structure->varList = v;
					// 无名结构体,无需插入至哈希表
					if(t->type_info.structure->name == NULL) {
						printf("why\n");
						return t;
					}
					// 将定义好的结构体本身插入哈希表
					// 结构体内部的变量不在此处插入
					VarType tmp = malloc(sizeof(struct VarType_));
					tmp->type = t;
					tmp->name = malloc(sizeof(t->type_info.structure->name)+1);
					strcpy(tmp->name, t->type_info.structure->name);
					int ret_code = insertVar(tmp);
					// 结构体的名字和定义过的结构体或变量的名字重复
					if(ret_code == 1) {
						printf("Error type 16 at line %d: The name of struct '%s' duplicates name of another variable or struct", DefList_node->row, t->type_info.structure->name);
						return NULL;
					}
					return t;
				}
			}
		// 根据已有的结构定义新的结构变量
		} else if(!strcmp(child->next->name, "Tag")) {
			// 查找变量符号表中是否已经定义了该结构
			VarType tmp = findSymbol(child->next->children->value);
			if(!tmp || tmp->type->type != STRUCTURE || strcmp(tmp->name, tmp->type->type_info.structure->name)) {
				// 结构体未定义
				printf("Error type 17 at line %d: Undefined struct type '%s'\n", child->next->row, child->next->children->value);
				return NULL;
			} else {
				return tmp->type;
			}
		}
	}
	printf("[Internal Error] error in semantic analysis in StructSpecifier()\n");
	return NULL;
}

// DefList-> Def DefList
//			| e
//变量定义和声明：place是该非终结符被定义的地方。
VarType DefList(Node *n,int place) {//Struct中的DefList不允许声明时赋值，CompoundStatement中的允许赋值
	Node *child = n->children;
	if(child){	//存在声明
		VarType v;
		v=Def(child,place);
		child=child->next;//DefList
		VarType tmp=v;
		if(v!=NULL){			//Def可能定义了多个变量
			while(tmp->next_field!=NULL)tmp=tmp->next_field;
			tmp->next_field=DefList(child,place);
		}
		else v=DefList(child,place);
		return v;
	}
	else return NULL;
}

//Def -> Specifier DecList SEMI
VarType Def(Node* n,int place){
	//printf("Def\n");
	Node* child=n->children;
	VarType v;
	Type type=Specifier(child);
	//printf("%d\n",type->type);
	child=child->next;
	v=DecList(child,type,place);	
	return v;
}

//DecList -> Dec
//		   | Dec COMMA DecList
VarType DecList(Node* n,Type type,int place){
	Node* child=n->children;
	VarType v=Dec(child,type,place);
	if(child->next!=NULL){
		child=child->next->next;//DecList
		if(v!=NULL)
		{
			VarType tmp=v;
			while(tmp->next_field!=NULL)tmp=tmp->next_field;
			tmp->next_field=DecList(child,type,place);
		}
		else v=DecList(child,type,place);
	}
	return v;
}

//Dec -> VarDec
//	   | VarDec ASSIGNOP Exp
//FROM_STRUCT:不允许使用Dec -> VarDec ASSIGNOP Exp的规则，FROM_COUMPOUND:两个规则都可以
VarType Dec(Node* n,Type type,int place){
	Node* child=n->children;
	VarType v=VarDec(child,type,place);
	if(child->next!=NULL){		
		if(place==FROM_STRUCT){	
			printf("Error type 15 at line %d:variable initialized in struct is not allowed.In sturct %s\n",child->row,v->name);
			return v;
		}
		child=child->next->next;//Exp
		Type t=Exp(child);
		if(!isTypeEqual(type,t)){
			printf("Error type 5 at line %d: The type mismatched\n",child->row);
		}
	}
	return v;
}

//FunDec-> ID LP VarList RP
//		|  ID LP RP
//		|  error RP (语法错误，不在此处处理)
FuncType FunDec(Node* n,Type return_type)
{
	//printf("FunDec\n");
	Node* child=n->children;
	FuncType func=malloc(sizeof(struct FuncType_));
	func->isDefined=false;
	func->name=malloc(sizeof(child->value)+1);
	strcpy(func->name,child->value);
	func->returnType=return_type;
	func->row=child->row;
	Node* child3=child->next->next;//右括号或VarList
	if(!strcmp(child3->name,"VarList"))func->param=VarList(child3);
	else func->param=NULL;
}


//VarList -> ParamDec COMMA VarList
//		   | ParamDec
VarType VarList(Node* n){//返回参数列表
	Node* child=n->children;
	VarType v;
	v=ParamDec(child);
	if(child->next!=NULL)
	{
		child=child->next->next;//VarList
		VarType tmp=v;
		if(v!=NULL){
			while(tmp->next_field!=NULL)tmp=tmp->next_field;
			tmp->next_field=VarList(child);
		}
		else v=VarList(child);
	}
	return v;
}

//ParamDec -> Specifier VarDec
VarType ParamDec(Node*n){
	Node *child=n->children;
	Type type=Specifier(child);
	VarType v=VarDec(child->next,type,FROM_PARAM);
	return v;
}

//VarDec -> ID
// 		  | VarDec LB INT RB
VarType VarDec(Node* n,Type type,int place){//将定义的变量插入变量表
	Node* child=n->children;
	//printf("VARDEC\n");
	if(!strcmp(child->name,"ID")){
		VarType v=malloc(sizeof(struct VarType_));
		v->name=malloc(sizeof(child->value)+1);//变量名
		strcpy(v->name,child->value);
		v->type=type;
		//printf("%d\n",type->type);
		v->next=NULL;
		v->next_field=NULL;
		if(place==FROM_PARAM)return v;//在函数参数列表中定义的变量，不需要插入变量表，只需要赋值给func->param
		//printf("insert Start\n");
		if(insertVar(v)==REDEFINE_ERROR){
			if(place==FROM_GLOBAL||place==FROM_COMPOUND)//暂时不区分全局变量和域变量
				printf("Error type 3 at line %d: Redefined global variable'%s'\n",child->row,v->name);	
			else //来自结构体
				printf("Error type 15 at line %d: Redefined field variable '%s'\n",child->row,v->name);
			return NULL;
		}
		return v;
	}
	/*他这种写法很怪异
	else if(!strcmp(child->name,"VarDec")){//数组
		VarType v=VarDec(child,type,place);//先调用，定义type，然后接下来再改。
		if(v==NULL)return NULL;//重名了
		else{
			child=child->next->next;	//INT
			Type t=malloc(sizeof(struct Type_));
			t->type=ARRAY;
			t->type_info.array.size=atoi(child->value);
			t->type_info.array.element=type;//确定数组中的元素类型
			Type tmp=v->type;
			if(tmp->type!=ARRAY){//以int a[8];为例。此时a的type是int，此处要改为array
				v->type=t;
				return v;
			}
			while(tmp->type_info.array.element->type==ARRAY){	//找到数组的最外层，定义其类型为基本类型。
				tmp=tmp->type_info.array.element;
			}
			tmp->type_info.array.element=t;
			//printtype(f->type);
			return v;
		}
	}
	*/
	else if(!strcmp(child->name,"VarDec")){//数组
		Type NewType=malloc(sizeof(struct Type_));
		NewType->type=ARRAY;
		NewType->type_info.array.size=atoi(child->next->next->value);
		NewType->type_info.array.element=type;
		VarType v=VarDec(child,NewType,place);
		if(v==NULL)return NULL;
		return v;
	}
	else {
		printf("CODE ERROR in VarDec function\n");
	}
}

//CompSt -> LC DefList StmtList RC  这个CoumpSt仅用于函数体，不用于结构体，故有返回值
void CompSt(Node * n,Type return_type){
	printf("CompSt\n");
	Node* child=n->children->next;//DefList
	DefList(child,FROM_COMPOUND);
	child=child->next;//StmtList
	StmtList(child,return_type);
}

//StmtList -> Stmt StmtList
//			| e
void StmtList(Node *n,Type return_type){
	Node* child=n->children;
	if(child){
		Stmt(child,return_type);
		child=child->next;
		StmtList(child,return_type);
	}
	return;
}

//Stmt -> Exp SEMI		
//     |  CompSt			
//	   |  RETURN Exp SEMI		
//	   |  IF LP Exp RP Stmt	
//	   |  IF LP Exp RP Stmt ELSE Stmt
//	   |  WHILE LP Exp RP Stmt
void Stmt(Node *n,Type return_type)		//error type 8	return  //return type mismatched	
{
	Node*child=n->children;
	if(!strcmp(child->name,"Exp")){
		Exp(child);
		return;
	}	
	else if(!strcmp(child->name,"CompSt")){
		CompSt(child,return_type);
		return;
	}
	else if(!strcmp(child->name,"RETURN")){
		child=child->next;
		Type t=Exp(child);
		if(return_type==NULL||t==NULL)return;
		if(!isTypeEqual(return_type,t))
		{
			printf("Error type 8 at line %d: The return type mismatched\n",child->row);
		}
		return;
	}
	else if(!strcmp(child->name,"IF")){
		//还需改动,实现跳转功能
		child=child->next->next;//Exp
		Type t=Exp(child);
		if(t!=NULL&&!((t->type==BASIC||t->type==CONSTANT)&&t->type_info.basic!=INT_TYPE)){//t==NULL的话说明在Exp函数中已经报错了
			printf("Error at line %d: type %s is not allowed for if condition",child->row,Type2String(t));
		}
		child=child->next->next;//Stmt
		Stmt(child,return_type);
		child=child->next;//else或NULL
		if(child){
			child=child->next;//Stmt
			Stmt(child,return_type);
		}
	}
	else if(!strcmp(child->name,"WHILE")){
		//还需改动,实现停止循环功能
		child=child->next->next;//Exp
		Type t=Exp(child);
		if(t!=NULL&&!((t->type==BASIC||t->type==CONSTANT)&&t->type_info.basic!=INT_TYPE)){//t==NULL的话说明在Exp函数中已经报错了
			printf("Error at line %d: type %s is not allowed for while condition",child->row,Type2String(t));
		}
		child=child->next->next;//Stmt
		Stmt(child,return_type);
	}
	else {
		printf("CODE ERROR: in Stmt function\n");
	}
}

/*Exp ->Exp ASSIGNOP Exp	
    |	Exp AND Exp		
	| 	Exp OR Exp		
	|	Exp RELOP Exp		
	|	Exp PLUS Exp		
	|	Exp MINUS Exp		
	|	Exp MUL Exp		
	|	Exp DIV Exp		
	| 	LP Exp RP		
	|	MINUS Exp		
	|	NOT Exp			
	|	ID LP Args RP		
	|	ID LP RP		
	|	Exp LB Exp RB		
	|	Exp DOT ID		
	|	ID			
	|	INT			
	|	FLOAT			
	;*/
//返回值要说明Exp的值的类型
Type Exp(Node *n){
	Node *child=n->children;
	if(!strcmp(child->name,"Exp")){
		if(!strcmp(child->name,"LB")){//array
			Type t1=Exp(child);
			if(t1==NULL)return NULL;
			if(t1->type!=ARRAY)
			{
				printf("Error type 10 at line %d: '%s' must be an array\n",child->row,child->value);
				return NULL;
			}
			child=child->next->next;//第二个exp
			Type t2=Exp(child);
			if(t2==NULL)return NULL;  
			if(!((t2->type==BASIC||t2->type==CONSTANT)&&t2->type_info.basic==INT_TYPE))
			{
				printf("Error type 12 at line %d: Operands type mistaken\n",child->row);
				return NULL;
			}
			return t1->type_info.array.element;
		}
		else if(!strcmp(child->name,"DOT")){//struct
			Type t1=Exp(child);
			if(t1==NULL)return NULL;
			if(t1->type!=STRUCTURE)
			{
				printf("Error type 13 at line %d: Illegal use of '.'\n",child->row);
				return NULL;
			}
			VarType v=t1->type_info.structure->varList;
			child=child->next->next;
			while(v!=NULL)
			{
				if(!strcmp(v->name,child->value))
					return v->type;
				v=v->next_field;
			}
			printf("Error type 14 at line %d: Un-existed field '%s‘\n",child->row,child->value);
			return NULL;
		}
		else{//binary exp
			return BinaryExp(child,child->next,child->next->next);
		}
	}
	else if(!strcmp(child->name,"LP")){
		return Exp(child->next);
	}
	else if(!strcmp(child->name,"MINUS")){
		Type t=Exp(child->next);
		if(t==NULL)return NULL;
		if(t->type==BASIC)return t;
		printf("Error type 7 at line %d: Operands type mismatched\n",child->row);
		return NULL;
	}
	else if(!strcmp(child->name,"NOT")){
		Type t=Exp(child->next);
		if(t==NULL)return NULL;
		if(t->type==BASIC&&t->type_info.basic==INT_TYPE)return t;
		printf("Error type 7 at line %d: Operands type mismatched\n",child->row);
		return NULL;
	}
	else if(!strcmp(child->name,"ID")){
		if(child->next!=NULL){//function call
			VarType v=findSymbol(child->value);
			FuncType f=findFunc(child->value);
			if(v!=NULL&&f==NULL)
			{
				printf("Error type 11 at line %d: '%s' must be a function\n",child->row,child->value);
				return NULL;
			}
			if(f==NULL||!f->isDefined){
				printf("Error type 2 at line %d: Undefined function '%s'\n",child->row,child->value);
				return NULL;
			}
			VarType param=f->param;
			child=child->next->next;
			if(strcmp(child->name,"RP")==0)//无参
			{
				if(param!=NULL)
				{
					printf("Error type 9 at line : The method '%s(",f->name);
					printparam(param);
					printf(")'is not applicable for the arguments '()'\n");
				}
			}
			else
			{
				if(!Args(child,param)){
					printf("Error type 9 at line : The method '%s(",f->name);
					printparam(param);
					printf(")'is not applicable for the arguments '(");
					printargs(child);
					printf(")'\n");
				}
			}
			return f->returnType;
		}
		else{//variable identifier
			VarType v=findSymbol(child->value);
			if(v==NULL)
			{
				printf("Error type 1 at line %d: Undefined variable '%s'\n",child->row,child->value);	
				return NULL;
			}
			return v->type;
		}
	}
	else if(!strcmp(child->name,"INT")){
		Type t=malloc(sizeof(struct Type_));
		t->type=CONSTANT;
		t->type_info.basic=INT_TYPE;
		return t;
	}
	else if(!strcmp(child->name,"FLOAT")){
		Type t=malloc(sizeof(struct Type_));
		t->type=CONSTANT;
		t->type_info.basic=FLOAT_TYPE;
		return t;
	}
	else{
		printf("CODE ERROR:in function exp");
	}
}
Type BinaryExp(Node* left,Node* op,Node* right){
	if(!strcmp(op->name,"ASSIGNOP")){
		Type leftType=Exp(left);
		if(leftType->type!=BASIC){
			printf("Error type 6 at line %d: The left-hand side of an assignment must be a variable\n",left->row);
		}
		Type rightType=Exp(right);
		if(leftType==NULL||rightType==NULL)return NULL;
		if(isTypeEqual(leftType,rightType))return leftType;
		else
		{
			printf("Error type 5 at line %d: Type mismatched\n",left->row);
			return NULL;
		}
	}
	else if(!strcmp(op->name,"PLUS")||!strcmp(op->name,"MINUS")||!strcmp(op->name,"MUL")||!strcmp(op->name,"DIV")||!strcmp(op->name,"RELOP")){
		Type leftType=Exp(left);
		Type rightType=Exp(right);
		if(leftType==NULL||rightType==NULL)return NULL;
		else if((leftType->type==BASIC||leftType->type==CONSTANT)&&(rightType->type==BASIC||rightType->type==CONSTANT)
			&&leftType->type_info.basic==rightType->type_info.basic)return leftType;
		else
		{
			printf("Error type 7 at line %d: Operands type mismatched\n",left->row);
			return NULL;
		}
	}
	else{
		printf("ERROR:unkonwn operand %s at line %d\n",op->value,op->row);
	}
}
//Args -> Exp COMMA Args
//		| Exp
//返回值用于判断实际参数和形式参数是否匹配，VarType v是函数的形式参数列表
bool Args(Node* n,VarType v){
	if(n==NULL&&v==NULL)return true;//匹配结束
	else if(n==NULL||v==NULL)return false;//数量不匹配
	Node *child=n->children;
	Type t=Exp(child);
	if(t==NULL)return true;		//如果exp返回NULL说明出现了错误，此处不用重复报错，直接返回
	if(!isTypeEqual(t,v->type))return false;
	//不直接调用Args(child->next->next,v->next_field);是因为child->next==NULL时child->next->next会出现指针错误
	if(child->next==NULL&&v->next_field==NULL)return true;
	else if(child->next==NULL||v->next_field==NULL)return false;
	return Args(child->next->next,v->next_field);
}

char* Type2String(Type t){
	if(t==NULL)return "NULL";
	switch (t->type)
	{
		case BASIC: case CONSTANT:
			if(t->type_info.basic==INT_TYPE) return "int";
			else return "float";
		case ARRAY:return "ARRAY";
		case STRUCTURE:	return "STRUCTURE";
		default: return "unknown TYPE";
	}
}

int String2Int(char *s){//允许16进制和8进制字符串转整数
	if(s==NULL){
		printf("ERROR:NULL pointer in String2Int function\n");
		return 0;
	}
	if(s[0]=='-'){
		return -String2Int(s+1);
	}
	int len=strlen(s);
	int n=0,tmp,i;
	if(s[0]=='0'){
		if(len==1)return 0;
		else if(s[1]=='x'||s[1]=='X'){//16进制
			for(i=2;i<len;i++){
				if(s[i]>='A'&&s[i]<='F')//十六进制还要判断他是不是在A-F或者a-f之间a=10。。
				tmp=s[i]-'A'+10;
				else if(s[i]>='a'&&s[i]<='f')
				tmp=s[i]-'a'+10;
				else tmp=s[i]-'0';
				n=n*16+tmp;
			}
			return n;
		}
		else{//8进制
			for(i=1;i<len;i++){
				tmp=s[i]-'0';
				n=n*8+tmp;
			}
			return n;
		}
	}
	else return atoi(s);
}
//Test Function
void printparam(VarType v){
	while(v!=NULL){
		printtype(v->type);
		v=v->next_field;
	}
}

void printargs(Node *n){
	Node *child=n->children;
	Type t=Exp(child);
	if(t==NULL)return;
	printtype(t);
	child=child->next;
	if(child==NULL)return;
	child=child->next;
	printargs(child);
}

void printtype(Type t){
	if((t->type==BASIC||t->type==CONSTANT)&&t->type_info.basic==INT_TYPE)
		printf(" int ");
	else if((t->type==BASIC||t->type==CONSTANT)&&t->type_info.basic==FLOAT_TYPE)
		printf(" float ");
	else if(t->type==STRUCTURE)
		printf("struct %s ",t->type_info.structure->name);
	else if(t->type==ARRAY) {
		printtype(t->type_info.array.element);
		printf("[]");
	}
}

void printNode(Node *n){
	Node *child=n->children;
	if(child==NULL) {
		printf(" %s",n->value);
		return;
	}
	while(child!=NULL) {
		printNode(child);
		child=child->next;
	}
}

