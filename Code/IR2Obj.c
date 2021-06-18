#include "IR2Obj.h"

void Init(){//初始化寄存器
    int i;
    char tmp[3];
    for(i=0;i<REG_NUM;i++){
        regs[i].vname=NULL;
        regs[i].LRU_count=0;
    }
    for(i=0;i<10;i++){//t系列寄存器赋名称        
        sprintf(tmp,"t%d",i);
        regNamer(regs[i].name,tmp);
    }
    for(i=10;i<18;i++){//s系列寄存器赋名称        
        sprintf(tmp,"s%d",i-10);
        regNamer(regs[i].name,tmp);
    }
    regNamer(regs[18].name,"fp");
    regNamer(regs[19].name,"sp");
    regNamer(regs[20].name,"ra");
    regNamer(regs[21].name,"v0");
    for(i=22;i<26;i++){//a系列寄存器
        sprintf(tmp,"a%d",i-22);
        regNamer(regs[i].name,tmp);
    }
    FuncVarList=NULL;
    VarListTail=NULL;
    FrameOffset=0;
    ArgCount=0;
}

void regNamer(char name[],char* str){
    name[0]=str[0];
    name[1]=str[1];
}

void DelVarList(){//删除整个变量链表
    vnode* cur;
    while(FuncVarList!=NULL){
        cur=FuncVarList;
        FuncVarList=FuncVarList->next;
        free(cur);
    }
    VarListTail=NULL;
}

void addVar(vnode *v){//添加v到变量链表尾部
    if(v==NULL){
        printf("addVar() ERROR : insert NULL node\n");
        exit(1);
    }
    if(FuncVarList==NULL){
        FuncVarList=VarListTail=v;
    }
    else{
        VarListTail->next=v;
        VarListTail=v;
    }
}

void resetStRegs(){//reset所有t系列和s系列寄存器
    int i;
	for(i=0;i<17;i++)
	{
		regs[i].vname=NULL;
		regs[i].LRU_count=0;
	}  
}

int allocReg(Operand op,FILE*file){//给变量分配寄存器，若变量已经分配寄存器，则直接返回寄存器索引
	char* name=malloc(32);
	vnode *cur=FuncVarList;
	//确定变量的名称(临时变量变量名前加t)
	if(op->kind==TMP_VAR)//临时变量分配t系列寄存器
	{
		name[0]='t';
		sprintf(name+1,"%d",op->u.var_no);
	}
	else if(op->kind==VAR)//普通变量
	{
		strcpy(name,op->u.value);
	}
	//查找该变量在变量列表中的位置
	while(cur!=NULL){
		if(!strcmp(cur->vname,name))break;
		cur=cur->next;	
	}
	int flag=0;//flag==1则该变量在内存中没有值
	if(cur==NULL)	//在函数里第一次出现,必然未分配寄存器，且不在内存中
	{
		cur=malloc(sizeof(vnode));
		cur->vname=name;
		cur->reg=-1;        //未分配寄存器
		FrameOffset+=4;		//在栈中分配位置
		cur->offset=FrameOffset;
		flag=1;			    //未写入内存
		cur->next=NULL;
		add_var(cur);//将变量添加到变量链表尾部
	}
	if(cur->reg==-1)//若变量还未分配寄存器
	{
		int index=GetEmptyReg(file);//找一个空寄存器或溢出一个寄存器
		cur->reg=index;
		regs[index].vname=cur->vname;
		if(cur->offset>=0&&!flag)		//在内存中有值,则把值读进寄存器
		{ 
			fputs("subu $v1 ,$fp , ",file);//计算该变量在内存中的位置
			char tmp[32];
			sprintf(tmp,"%d",cur->offset);
			fputs(tmp,file);
			fputs("\nlw ",file);
			getReg(cur->reg,file);
			fputs(", 0($v1)\n",file);	
		}
	}
	regs[cur->reg].LRU_count=0;
	return cur->reg;   
}
/*在t系列和s系列寄存器中找空余寄存器，没有则选LRU_count值最大的溢出，溢出时将写入内存的代码写入file。返回空闲寄存器索引*/
int GetEmptyReg(FILE*file){
    int i;
    for(i=0;i<17;i++)if(regs[i].vname!=NULL)regs[i].LRU_count++;
    for(i=0;i<17;i++)if(regs[i].vname==NULL)return i;
    //所有寄存器都已被分配,找最近最少使用的寄存器，然后将对应的变量存入内存
    int LRU_max=0,max_index;
    for(i=0;i<17;i++){
        if(LRU_max<regs[i].LRU_count){
            LRU_max=regs[i].LRU_count;
            max_index=i;
        }
    }
    vnode* cur=FuncVarList;
    while(cur!=NULL){//找到寄存器对应的变量
        if(!strcmp(regs[max_index].vname,cur->vname))break;
        cur=cur->next;
    }
    if(cur!=NULL)//将变量存入内存
	{
		fputs("subu $v1, $fp , ",file);
		char temp[32];
		sprintf(temp,"%d",cur->offset);
		fputs(temp,file);
		fputs("\nsw ",file);
		PrintReg(max_index,file);
		fputs(", 0($v1)\n",file);
		cur->reg=-1;
	}
	regs[max_index].vname=NULL;
	regs[max_index].LRU_count=0;
	return max_index;
}
void PrintReg(int index,FILE*file){//打印寄存器名
    fputs("$",file);
	fputs(regs[index].name,file);
}
void printAllCode(char*fname){//向fname中打印所有目标代码
    FILE *file=fopen(fname,"w");
	if(file==NULL) 
	{
		printf("open file error\n");
		return;
	}
	/*这部分是对任何程序都一样的，代码最开始部分，以及定义read和write函数*/
    //.asciiz : 存储str于内存中，并以null结尾。
	fputs(".data\n_prompt: .asciiz \"Enter an integer:\"\n",file);//定义read的提示字符串
	fputs("_ret: .asciiz \"\\n\"\n.globl main\n.text\n",file);//write每写出一次，加上一次换行
	//read函数，无参数，返回值为int
	fputs("\nread:\n",file);
	//处理栈指针和帧指针、寄存器ra
	fputs("subu $sp, $sp, 4\n",file); 
	fputs("sw $ra, 0($sp)\n",file);
	fputs("subu $sp, $sp, 4\n",file);
	fputs("sw $fp, 0($sp)\n",file);
	fputs("addi $fp, $sp, 8\n",file);
	//这部分照抄实验指导
	fputs("li $v0, 4\n",file);//选择系统调用print_string
    fputs("la $a0, _prompt\n",file);//传入读取提示字符串
	fputs("syscall\n",file);//打印提示字符串
    fputs("li $v0, 5\n",file);//对应系统调用read_int
    fputs("syscall\n",file);
	//处理栈指针和帧指针、寄存器ra
	fputs("subu $sp, $fp, 8\n",file);
	fputs("lw $fp, 0($sp)\n",file);
	fputs("addi $sp, $sp, 4\n",file);
	fputs("lw $ra, 0($sp)\n",file);
	fputs("addi $sp, $sp, 4\n",file);
	fputs("jr $ra\n",file);
	//write函数,参数为int，返回值为int
	fputs("\nwrite:\n",file);
	fputs("subu $sp, $sp, 4\n",file); 
	fputs("sw $ra, 0($sp)\n",file);
	fputs("subu $sp, $sp, 4\n",file);
	fputs("sw $fp, 0($sp)\n",file);
	fputs("addi $fp, $sp, 8\n",file);
    //参见实验指导
	fputs("li $v0, 1\n",file);//选择系统调用print_int
    fputs("syscall\n",file);
    fputs("li $v0, 4\n",file);//选择系统调用print_string
	fputs("la $a0, _ret\n",file);
    fputs("syscall\n",file);
    fputs("move $v0, $0\n",file);
	fputs("subu $sp, $fp, 8\n",file);
	fputs("lw $fp, 0($sp)\n",file);
	fputs("addi $sp, $sp, 4\n",file);
	fputs("lw $ra, 0($sp)\n",file);
	fputs("addi $sp, $sp, 4\n",file);
	fputs("jr $ra\n",file);
	//写入自己的目标代码
	InterCode c=head;//遍历中间代码链表
	while(c!=NULL)
	{
		printObjCode(c,file);
		c=c->next;
	}
}
void printObjCode(InterCode ic,FILE* file){//将一份中间代码转成目标代码写到file中
    switch (ic->kind){
        case LABEL:
            TansLabel(ic,file);
            break;
        case FUNCTION:
            TransFunc(ic,file);
            break;
        case ASSIGN:
            TransAssign(ic,file);
            break;
        case ADD_KIND:
        case SUB_KIND:
        case MUL_KIND: 
        case DIV_KIND:
            TransBinaryAssign(ic,file);
            break;
        case RIGHTAT:
            TransRightAt(ic,file);
            break;
        case GOTO:
            TransGOTO(ic,file);
            break;
        case IFGOTO:
            TransIFGOTO(ic,file);
            break; 
        case RETURN_KIND:
            TrasnReturn(ic,file);
            break;
        case DEC:
            break;
        case ARG:
            break;
        case CALL: 
            break;
        case PARAM:
            break;
        case READ:
            break;
        case WRITE:
            break;
        default:
            printf("[internal Error]: Unknown InterCode in IR2Obj\n");
            break;
    }
}
void StoreVarList(FILE* file){
    vnode* cur=FuncVarList;		
    while(cur!=NULL)//把当前所有变量保存到内存中
    {
        if(cur->reg>=0)
        {
            fputs("subu  $v1 ,$fp , ",file);
            char temp[32];
            sprintf(temp,"%d",cur->offset);
            fputs(temp,file);

            fputs("\nsw ",file);
            PrintReg(cur->reg,file);
            fputs(", 0($v1)\n",file);	//fp偏移量计算
            regs[cur->reg].vname=NULL;
            regs[cur->reg].LRU_count=0;
            cur->reg=-1;
        }
        cur=cur->next;
    }
}

void TransLable(InterCode ic,FILE* file){//先把当前函数中的变量全部写入内存，再打印label
    vnode* cur=FuncVarList;
    char tmp[32];
    while(cur!=NULL){//变量写入内存
        if(cur->reg>=0){
            fputs("subu  $v1 ,$fp , ",file);//减去偏移量
            sprintf(tmp,"%d",cur->offset);
            fputs(tmp,file);
            fputs("\nsw ",file);//存入内存
            PrintReg(cur->reg,file);
            fputs(", 0($v1)\n",file);	//fp偏移量计算
            regs[cur->reg].vname=NULL;
            regs[cur->reg].LRU_count=0;
            cur->reg=-1;
        }
        cur=cur->next;
    }
    printOperand(ic->u.unary.op,file);//打印label
	fputs(":\n",file);
}

void TransAssign(InterCode ic,FILE* file){
    Operand left,right;
    left=ic->u.assign.left;
    right=ic->u.assign.right;
    if((left->kind==TMP_VAR)||(left->kind==VAR)){
        switch (right->kind)
        {
            case CONSTANT_OP://常数用li指令
                fputs("li ",file);
                PrintReg(allocReg(left,file),file);
                fputs(" ,",file);
                fputs(right->u.value,file);
                fputs("\n",file);
                break;
            case VAR:case TMP_VAR://左右都是变量都要分配寄存器，用move指令
                fputs("move ",file);
                PrintReg(allocReg(left,file),file);
                fputs(" ,",file);
                PrintReg(allocReg(right,file),file);
                break;
            case TMP_VAR_ADDRESS:case VAR_ADDRESS://x := *y -> lw reg(x),0(reg(y))
                fputs("lw ",file);
                PrintReg(allocReg(left,file),file);
                fputs(",0(",file);
                PrintReg(allocReg(right->u.var,file),file);
                fputs(")",file);
                break;
            default:
                printf("[internal ERROR]:Illegal right type in TransAssign()");
                break;
        }
    }
    else if(left->kind==VAR_ADDRESS||left->kind==TMP_VAR_ADDRESS){
        switch(right->kind)
        {
            case CONSTANT_OP:   //*x := #k -> sw (tmpreg for #k),0(reg(x))
                Operand tmpreg=Const2Tmpvar(right,file);
                fputs("sw ",file);
                PrintReg(allocReg(tmpreg,file),file);
                fputs(",0(",file);
                PrintReg(allocReg(left->u.var,file),file);
                fputs(")",file);
                break;
            case TMP_VAR:
            case VAR:   //*x := y -> sw reg(y),0(reg(x))
                fputs("sw ",file);
                PrintReg(allocReg(right,file),file);
                fputs(",0(",file);
                PrintReg(allocReg(left->u.var,file),file);
                fputs(")",file);
                break;
            default:
                printf("[internal ERROR]:Illegal right type in TransAssign()");
                break;
        }
    }
    else{
        printf("[internal ERROR]:Illegal left type in TransAssign()");
    }
}

void TransBinaryAssign(InterCode ic,FILE* file){
    Operand op1,op2,result;
    op1=ic->u.binop.op1;
    op2=ic->u.binop.op2;
    result=ic->u.binop.result;
    if(result->kind==VAR||result->kind==TMP_VAR){
        if((op1->kind==TMP_VAR||op1->kind==VAR)&&(op2->kind==VAR||op2->kind==TMP_VAR)){
            if(ic->kind==ADD_KIND)fputs("add ",file);
            else if(ic->kind==SUB_KIND)fputs("sub ",file);
            else if(ic->kind==MUL_KIND)fputs("mul ",file);
            else if(ic->kind==DIV_KIND)fputs("div ",file);
            fputs(allocReg(result,file),file);
            fputs(", ",file);
            fputs(allocReg(op1,file),file);
            fputs(", ",file);
            fputs(allocReg(op2,file),file);
        }
        else if((op1->kind==TMP_VAR||op1->kind==VAR)&&op2->kind==CONSTANT_OP){//可以用addi
            if(ic->kind==ADD_KIND||ic->kind==SUB_KIND){
                fputs("addi ",file);
                fputs(allocReg(result,file),file);
                fputs(", ",file);
                fputs(allocReg(op1,file),file);
                fputs(", ",file);
                if(ic->kind==SUB_KIND)fputs("-",file);//减法取负数
                fputs(op2->u.value,file);
            }
            else if(ic->kind==MUL_KIND||ic->kind==DIV_KIND){//需要把常数转寄存器
                if(ic->kind==MUL_KIND)fputs("mul ",file);
                else if(ic->kind==DIV_KIND)fputs("div ",file);
                fputs(allocReg(result,file),file);
                fputs(", ",file);
                fputs(allocReg(op1,file),file);
                fputs(", ",file);
                fputs(allocReg(Const2Tmpvar(op2,file),file),file);//常数转寄存器
            }
        }
        else if((op2->kind==TMP_VAR||op2->kind==VAR)&&op1->kind==CONSTANT_OP){
            if(ic->kind==ADD_KIND){
                fputs("addi ",file);
                fputs(allocReg(result,file),file);
                fputs(", ",file);
                fputs(allocReg(op2,file),file);
                fputs(", ",file);
                fputs(op1->u.value,file);
            }
            else if(ic->kind==SUB_KIND||ic->kind==MUL_KIND||ic->kind==DIV_KIND){//需要把常数转寄存器   
                if(ic->kind==SUB_KIND)fputs("sub ",file);
                else if(ic->kind==MUL_KIND)fputs("mul ",file);
                else if(ic->kind==DIV_KIND)fputs("div ",file);
                fputs(allocReg(result,file),file);
                fputs(", ",file);
                fputs(allocReg(Const2Tmpvar(op1,file),file),file);
                fputs(", ",file);
                fputs(allocReg(op2,file),file);//常数转寄存器
            }
        }
        else if((op1->kind==VAR_ADDRESS||op1->kind==TMP_VAR_ADDRESS)&&(op2->kind==VAR||op2->kind==TMP_VAR)){// result=*op1 + op2
            Operand t1=Addr2Tmpvar(op1,file);
            InterCode c1=malloc(sizeof(struct InterCode_));
            c1->kind=ic->kind;
            c1->u.binop.result=result;
            c1->u.binop.op1=t1;
            c1->u.binop.op2=op2;
            printObjCode(c1,file);
        }
        else if((op2->kind==VAR_ADDRESS||op2->kind==TMP_VAR_ADDRESS)&&(op1->kind==VAR||op1->kind==TMP_VAR)){// result=op1 + *op2
            Operand t1=assCode(op2,file);
            InterCode c1=malloc(sizeof(struct InterCode_));
            c1->kind=ic->kind;
            c1->u.binop.result=result;
            c1->u.binop.op1=op1;
            c1->u.binop.op2=t1;
            printObjCode(c1,file);
        }
        else if((op1->kind!=VAR&&op1->kind!=TMP_VAR)&&(op2->kind!=VAR&&op2->kind!=TMP_VAR))
        {
            Operand t1=Addr2Tmpvar(op1,file);//constant和addr处理方式相同
            Operand t2=Addr2Tmpvar(op2,file);
            InterCode c1=malloc(sizeof(struct InterCode_));
            c1->kind=ic->kind;
            c1->u.binop.result=result;
            c1->u.binop.op1=t1;
            c1->u.binop.op2=t2;
            printObjCode(c1,file);
        }
    }
    else if(result->kind==VAR_ADDRESS||result->kind==TMP_VAR_ADDRESS){//*result = op1 + op2
        //先用一个操作数tmp_op，存放op1和op2的计算结果，再使用assign语句*result=tmpop
        Operand tmp_op=malloc(sizeof(struct Operand_));
        tmp_op->kind=TMP_VAR;
        tmp_op->u.var_no=varNo++;
        tmp_op->next=NULL;
        InterCode tmp_ic=malloc(sizeof(struct InterCode_));
        tmp_ic->kind=ic->kind;
        tmp_ic->next=tmp_ic->prev=NULL;
        tmp_ic->u.binop.result=tmp_op;
        tmp_ic->u.binop.op1=op1;
        tmp_ic->u.binop.op2=op2;
        printObjCode(tmp_ic,file);
        tmp_ic->kind=ASSIGN;//赋值语句
        tmp_ic->u.assign.left=result;
        tmp_ic->u.assign.right=tmp_op;
        printObjCode(tmp_ic,file);
    }
    else{
        printf("[internal ERROR]: Illeagal Binary Inter Code in TransBinaryAssign()\n");
    }
}

/*这个函数还没写完，没搞明白*/
void TransFunc(InterCode ic,FILE* file){
    printOperand(ic->u.unary.op,file);//打印函数标签
    fputs(":\n",file);
    DelVarList();	//因为在CALL_K处已经把所有寄存器写入内存了，所以直接删除上一函数的调用列表
	resetST();		//重置t系列寄存器和s系列寄存器。
    //存放fp和ra寄存器
    FrameOffset=FRAME_OFFSET;
    fputs("subu $sp, $sp, 4\n",file); 
    fputs("sw $ra, 0($sp)\n",file);
    fputs("subu $sp, $sp, 4\n",file);
    fputs("sw $fp, 0($sp)\n",file);
    fputs("addi $fp, $sp, 8\n",file);
    int memSize;
    char tmp[32];
    memSize=countVar(ic);			//预分配内存空间
    fputs("subu $sp,$sp,",file);
    sprintf(tmp,"%d",memSize*4);
    fputs(tmp,file);
    fputs("\n",file);
}

/*Dec语句后一定是x := &y*/
void TransRightAt(InterCode ic,FILE* file){//x := &y -> 
    char tmp[32];
    InterCode icpre=ic->prev;
    if(icpre->kind!=DEC)
        printf("DEC MISS\n");
    sprintf(tmp,"%d",icpre->u.dec.size);
    fputs("subu $sp, $sp, ",file);
    fputs(tmp,file);
    fputs("\n",file);
    FrameOffset+=icpre->u.dec.size;
    fputs("move ",file);
    PrintReg(allocReg(ic->u.assign.left,file),file);
    fputs(", $sp",file);
}

void TransGOTO(InterCode ic,FILE* file){//GOTO x->j x;
    StoreVarList(file);
    fputs("j ",file);//跳转
    printOperand(ic->u.unary.op,file);
}

void TransIFGOTO(InterCode ic,FILE* file){//IF x relop y GOTO z -> bxx reg(x), reg(y), z
    StoreVarList(file);
    Operand t1,t2,label;
    t1=ic->u.ifgoto.t1;
    t2=ic->u.ifgoto.t2;
    label=ic->u.ifgoto.label;
    //比较需要是寄存器比较，立即数不可比较
    if(t1->kind==CONSTANT_OP)t1=Const2Tmpvar(t1,file);
    if(t2->kind==CONSTANT_OP)t2=Const2Tmpvar(t2,file);
    char* relop=ic->u.ifgoto.op;
    if(!strcmp(relop,"=="))
        fputs("beq ",file);
    else if(!strcmp(relop,"!="))
        fputs("bne ",file);
    else if(!strcmp(relop,">"))
        fputs("bgt ",file);
    else if(!strcmp(relop,"<"))
        fputs("blt ",file);
    else if(!strcmp(relop,">="))
        fputs("bge ",file);
    else if(!strcmp(relop,"<="))
        fputs("ble ",file);
    PrintReg(allocReg(t1,file),file);
    fputs(", ",file);
    PrintReg(allocReg(t2,file),file);
    fputs(", ",file);
	printOperand(label,file);
}

void TransReturn(InterCode ic,FILE* file){//RETURN x -> move $v0, reg(x); jr $ra;
    int index;
    if(ic->u.unary.op->kind==CONSTANT_OP){
        index=allocReg(Const2Tmpvar(ic->u.unary.op,file),file);
    }
    else index=allocReg(ic->u.unary.op,file);
    fputs("subu $sp, $fp, 8\n",file);//从内存读出ra和fp
    fputs("lw $fp, 0($sp)\n",file);
    fputs("addi $sp, $sp, 4\n",file);
    fputs("lw $ra, 0($sp)\n",file);
    fputs("addi $sp, $sp, 4\n",file);
    fputs("move $v0,",file);//设置返回值
    PrintReg(index,file);
    fputs("\n",file);
    fputs("jr $ra",file);//跳回调用点
}

//有些指令不能直接用立即数计算，所以需要先把立即数加载进寄存器.返回值为新分配的寄存器的操作数
Operand Const2Tmpvar(Operand cst,FILE* file){  //#k -> t_varNo++ := k;
    Operand tmpvar=malloc(sizeof(struct Operand_));
    tmpvar->next=NULL;
    tmpvar->kind=TMP_VAR;
    tmpvar->u.var_no=varNo++;
    InterCode ic=malloc(sizeof(struct InterCode_));
    ic->kind=ASSIGN;
    ic->u.assign.left=tmpvar;
    ic->u.assign.right=cst;
    ic->next=NULL;
    ic->prev=NULL;
    printObjCode(ic,file);//打印 tmp
    return tmpvar;
}

//地址也没分配寄存器，计算时需要用寄存器存储
Operand Addr2Tmpvar(Operand addr,FILE* file){// *x -> t_varNo++ := *x;
    return Const2Tmpvar(addr,file);//处理方式和立即数相同
}