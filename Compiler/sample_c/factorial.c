#include <stdio.h>
#include <stdlib.h>

int myFactorial(int a);

int main(){
	int a = 11;
	a = myFactorial(a);
	
	printf("%d\n", a);
}

int myFactorial(int a){
	if(a == 0) return 1;
	
	return myFactorial(a-1) * a;
}












