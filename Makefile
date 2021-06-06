all:
	bison -d parser.y
	flex parser.l
	gcc -g parser.tab.c TreeNode.c  -o parser.exe