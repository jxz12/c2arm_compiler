//Sample 8
#include <stdio.h>
#include <stdlib.h>


int myDiv(int top, int bottom){
  int i = 0;
  while(top >= bottom){
    top = top - bottom;
    i++;
  }
  return i;
}

int main(){
  int pi = 0;
  int million = 100 * 100 * 100;
  
  int i, j = 4;
  for(i = 1; i < million; i++){
    pi = pi + j*myDiv(100*million, (2*i-1));
    j = 0-j;
  }

  printf("%d\n", pi);

}
