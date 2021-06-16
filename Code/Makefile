all:
	bison -d -v syntax.y
	flex lexical.l
	gcc -Wall -g syntax.tab.c TreeNode.c InterCode.c table.c -o parser.exe

clean:
	del -fR syntax.output syntax.tab.c syntax.tab.h lex.yy.c parser.exe