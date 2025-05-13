#include <stdio.h>
#include <stdlib.h>

#include "include/wav reader.h"

int main(int argc, char *argv[]) {
  FILE *fptr;
  char *reader = malloc(8 * sizeof(char));
  char currentDataChunk[1000000];

  fptr = fopen("Test Files/16-Test.wav", "r");
  fgets(currentDataChunk, 8, fptr);

  //printf("%s", myString);
  //printf("str_buf = %.4s\n", myString);
  
  printf("%.8s\n", currentDataChunk);

  fclose(fptr);
}
