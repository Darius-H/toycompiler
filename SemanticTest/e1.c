//多个函数
struct Vector
{
	float x, y;
};
float dotMultiply(struct Vector pv1 );
float getX();
float dotMultiply(struct Vector v1, struct Vector v2)
{
	int a = 0, b = 1, c = 2;
	float d = v1.x*v2.y-v1.y;
	return d;
}
int main()
{
	float a[10], c[10], b[10], d[10];
	int i = 0;
	struct Vector v1;
	struct Vector v2;
	while(i<10)
	{
	v1.x = a[i];
	v1.y = b[i];
	v2.x = c[i];
	v2.y = d[i];
	i = i+1;
	}
	return 0;
}

