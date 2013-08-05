int main(int argc, char **argv)
{
	typedef int x;
// 	int x;
// 	float x;
	x B;

	{
		typedef int x;
		x a;
	}

	return B + B;
}
