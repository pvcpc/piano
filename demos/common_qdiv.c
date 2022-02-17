#include <stdio.h>

#include <common.h>


int
demo()
{
	printf(" 5 /  4 = %d:%d\n",  5 /  4,  5 %  4);
	printf("-5 /  4 = %d:%d\n", -5 /  4, -5 %  4);
	printf(" 5 / -4 = %d:%d\n",  5 / -4,  5 % -4);
	printf("-5 / -4 = %d:%d\n", -5 / -4, -5 % -4);

	s32 tmp;
	printf(" 5 /  4 = %d:%d\n", tmp, qdiv(&tmp,  5,  4));
	printf("-5 /  4 = %d:%d\n", tmp, qdiv(&tmp, -5,  4));
	printf(" 5 / -4 = %d:%d\n", tmp, qdiv(&tmp,  5, -4));
	printf("-5 / -4 = %d:%d\n", tmp, qdiv(&tmp, -5, -4));
	return 0;
}
