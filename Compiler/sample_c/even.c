#include <stdio.h>
#include <stdlib.h>

typedef int *int_pointer;

int main(){
	int_pointer a[10];
	
	int i;
	for(i = 0; i < 10; i++){
		a[i] = malloc(sizeof(int));
		*a[i] = i * 2;
		printf("%d ", *a[i]);
	}
}














