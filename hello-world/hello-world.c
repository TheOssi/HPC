#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>

int main(int argc, char **argv) {
#pragma omp parallel num_threads(4)
{
printf("Hello World from Thread %d \n", omp_get_thread_num() +1);
}
  return 0;
}
