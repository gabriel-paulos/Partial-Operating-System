#include "common.h"
#include <unistd.h>
#include <string.h>

int factorial(int num){
  if(num == 1) return num;

  else return num*factorial(num-1);
}

int main(int argc, char** argv){
  if(argc < 2){
    printf("Huh?\n");
    return 0;
  }
  char dec = '.';
  char* str = strrchr(argv[1],dec);
  
  if(str != NULL){
    printf("Huh?\n");
    return 0;
  }

  else{
    int num = atoi(argv[1]);
    if(num < 1) printf("Huh?\n");
    
    else if(num > 12) printf("Overflow\n");

    else{
      int factor = factorial(num);
      printf("%d\n", factor);
    }
  } 
  
  return 0;
}
