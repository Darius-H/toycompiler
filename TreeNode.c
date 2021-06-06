#include"TreeNode.h"
extern int yylineno;
/*init a Node and allocate space for it*/
PtrToNode NewNode(char *name,char *value)
{
	PtrToNode node= malloc(sizeof(Node));
	strcpy(node->name,name);
	strcpy(node->value,value);
	node->children=NULL;
	node->next=NULL;
    node->row=yylineno;
	return node;
}

char* Mystrcpy(char* des,char* src,int maxsize)
{
    if(des==NULL||src==NULL){
        printf("Mystrcpy ERROR:des or src is NULL\n");
        return des;
    }
    int i=0;
    while((*(des++)=*(src++))&&i<maxsize)i++;
	des[i-1]='\0';
	return des;
}


/*give father a child*/
void addChild(PtrToNode f,PtrToNode c)
{
	if(f!=NULL&&c!=NULL)
	{
		c->next=f->children;
		f->children=c;
		f->row=c->row;		//词法分析会给终结符node的行赋值，默认非终结符的行值等于终结符的行值(含有语句块的可能不符合该条件)
	}
}

/*print the whole tree*/
void printTree(PtrToNode r,int count)
{
	if(r==NULL)return;
	if(r->children==NULL)		//it's a token
	{	
		int i=0;		
		for(;i<count;i++)	// retract 
		{
			printf("  ");
		}
		//not all nodes need to print value
		if(strcmp(r->name,"TYPE")==0||strcmp(r->name,"INT")==0||strcmp(r->name,"FLOAT")==0||strcmp(r->name,"ID")==0)
		{
			if(strcmp(r->name,"INT")==0)
				printf("%s: %d\n",r->name,atoi(r->value));
			else if(strcmp(r->name,"FLOAT")==0)
				printf("%s: %f\n",r->name,atof(r->value));
			else
				printf("%s: %s\n",r->name,r->value);

		}
		else
		{
			printf("%s\n",r->name);
		}
	}
	else 				//non-terminal
	{
		int i=0;
		for(;i<count;i++)
		{
			printf("  ");
		}
		printf("%s(%d)\n",r->name,r->row);
		Node *p=r->children;
		//traverse the child nodes
		while(p!=NULL){
			printTree(p,count+1);
			//PtrToNode temp=p;
			p=p->next;
			//free(temp);
			//temp=NULL;
		}
	}
	return;
}
