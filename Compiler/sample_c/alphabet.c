#include <stdio.h>
#include <stdlib.h>

int main(){
	char a = 'a';
	int i = 0;
	
	do{
		printf("%c", a + i);
		i++;
	}
	while(i < 26);
	
	printf("\n");
}














