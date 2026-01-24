//@	{"target":{"name": "main.o"}}

#include <cstdio>

int main(int argc, char**)
{
	if(argc != 2)
	{
		perror("Missing startup configuration from command line");
		return -1;
	}
}