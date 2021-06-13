//重定义函数错误,不支持先定义结构体名再根据结构体名定义变量
struct Position
{
	int  x,y;
};
struct Position s;
int func(int a);
int func(struct Position p);
int main()
{
	func(s.x);
}
