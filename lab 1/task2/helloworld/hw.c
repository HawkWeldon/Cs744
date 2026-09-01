#include <stdio.h>
#include <unistd.h>
#include "helloworld.h"

int main(int argc, char **argv) {

  // call a function
  helloworld();

  // wait for a character input to hold program from exiting
  getchar();

  return 0;
}
