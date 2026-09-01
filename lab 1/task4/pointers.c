#include<stdio.h>

int main(int argc, char* argv[]) {

  int a = 45; int b = 22;
  int *p = &a;
  int *q = NULL;

  printf("%d\n",*p);
 
  p = q;
  printf("%d\n",*p);
 
  p = &b;
  printf("%d\n",*p);

  return 0;
}