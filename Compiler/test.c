#include <stdio.h>
#include <stdlib.h>

int main(){
	int *a[3];
	int b = 2;
	
	a[2] = &b;
	*a[2] = 4;
	
	printf("%d\n", b);
}
