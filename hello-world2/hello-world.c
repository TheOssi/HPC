#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>

int main(int argc, char **argv) {
#pragma omp parallel sections num_threads(4)
{
	#pragma omp section
	{
	printf("Hello World from Thread %d \n", omp_get_thread_num());
	}
	#pragma omp section
	{
	printf("Hallo Welt from Thread %d \n", omp_get_thread_num());
	}
	#pragma omp section
	{
	printf("Spaghetti from Thread %d \n", omp_get_thread_num());
	}
	#pragma omp section
	{
	printf("Baguette from Thread %d \n", omp_get_thread_num());
	}
	#pragma omp section
	{
	printf("Taco from Thread %d \n", omp_get_thread_num());
	}

}
  return 0;
}
