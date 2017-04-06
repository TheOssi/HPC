#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>
#include "/usr/lib/openmpi/include/mpi.h"
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>

#define calcIndex(width, x,y)  ((y)*(width) + (x))

long TimeSteps = 10;

void filling(double* currentfield, int w, int h) {
  int i;
  for (i = 0; i < h*w; i++) {
    currentfield[i] = i; //(rand() < RAND_MAX / 10) ? 1 : 0; ///< init domain randomly
  }
}

int coutLivingN(double* currentfield, int w, int x, int y) {
	int living = 0;
	for(int i = -1; i <= 1; i++) {
		for(int j = -1; j <= 1; j++) {
			living += currentfield[calcIndex(w, x-i, y-j)];
		}
	}
	//substract current cell
	living -= currentfield[calcIndex(w,x,y)];
	return living;
}

void calcBorders(double* currentFiled, double* leftField, double* rightField, double* showFiled, int w, int h) {
	int wI = w+2;

	//left upper corner
	currentFiled[0] = leftField[w-1];
	//upper border
	for(int i = 1; i <= w; i++) {
		currentFiled[i] = leftField[i-1];
	}
	//right upper corner
	currentFiled[w+1] = leftField[0];
	//left border
	currentFiled[calcIndex(wI, 0, 1)] = showFiled[w-1];
	//center
	for(int i = 0; i < w; i++) { //index of sendField
		currentFiled[calcIndex(wI,i+1,1)] = showFiled[i];

	}
	//right border
	currentFiled[calcIndex(wI, wI-1, 1)] = showFiled[0];
	//left bottom corner
	currentFiled[calcIndex(wI,0,2)] = rightField[w-1];
	//bottom border
	for(int i = 1; i <= w; i++) {
		currentFiled[calcIndex(wI, i, 2)] = rightField[i-1];
	}
	//right bottom border
	currentFiled[calcIndex(wI,wI-1,2)] = rightField[0];
}

void show(double* currentfield, int w, int h) {
  printf("\033[H");
  int x,y;
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) printf(currentfield[calcIndex(w, x,y)] ? "\033[07m  \033[m" : "  ");
    //printf("\033[E");
    printf("\n");
  }
  fflush(stdout);
}

void evolve(double* currentfield, double* newfield, int w, int h) {
	int x,y;
	for (y = 1; y < h-1; y++) {
		for (x = 1; x < w-1; x++) {
			int isLiving = currentfield[calcIndex(w, x,y)];
			int living = coutLivingN(currentfield, w, x,y);
			if(isLiving == 0) {
				if(living == 3) {
					newfield[calcIndex(w, x,y)] = 1;
				}else {
					newfield[calcIndex(w, x,y)] = 0;
				}
			} else {
				if(living < 2 || living > 3) {
					newfield[calcIndex(w, x,y)] = 0;
				} else {
					newfield[calcIndex(w, x,y)] = 1;
				}
			}
		}
	}
}

int main(int c, char **v) {
	MPI_Init(&c, &v);

	int w = 5, h = 5, n = 5, rankLeft, rankRight, rank;
	long t;

	MPI_Status status;
	MPI_Comm comm;
	int dims[] = {n};
	int periods[] = {1};

	double *showField = calloc(w, sizeof(double));
	double *tempShowField = calloc(w, sizeof(double));
	double *leftField = calloc(w, sizeof(double));
	double *rightField = calloc(w, sizeof(double));
	double *currentField = calloc((w+2)*3, sizeof(double));
	double *newField = calloc((w+2)*3, sizeof(double));
	int *doNextLoop = calloc(1, sizeof(int));

	MPI_Cart_create(MPI_COMM_WORLD, 1, &dims, &periods, 1, &comm);
	MPI_Cart_shift(comm, 1, 1, &rankLeft, &rankRight);
	MPI_Comm_rank( comm, &rank );

	filling(showField, w, 1);
	doNextLoop[0] = 1;

	for (t = 0; t < TimeSteps && doNextLoop[0] == 1;t++) {
		if (rank % 2 == 0) {
				MPI_Send(showField, w, MPI_DOUBLE,rankRight, 0, comm);
				MPI_Recv(rightField, w, MPI_DOUBLE,rankRight, 0, comm, &status);
				MPI_Send(showField, w, MPI_DOUBLE,rankLeft, 0, comm);
				MPI_Recv(leftField, w, MPI_DOUBLE,rankLeft, 0, comm, &status);
		 } else {
				 MPI_Recv(leftField, w, MPI_DOUBLE,rankLeft, 0, comm, &status);
				 MPI_Send(showField, w, MPI_DOUBLE,rankLeft, 0, comm);
				 MPI_Recv(rightField, w, MPI_DOUBLE,rankRight, 0, comm, &status);
				 MPI_Send(showField, w, MPI_DOUBLE,rankRight, 0, comm);
		 }

		calcBorders(currentField, leftField, rightField, showField, w, h);
	    evolve(currentField, newField, w, h);

	    usleep(200000);

	    //SWAP
	    double *temp = currentField;
	    currentField = newField;
	    newField = temp;

	    //fill the new content to the tempShowField
		for(int i = 0; i < w; i++) {
			tempShowField[i] = currentField[calcIndex(w+2,i+1,1)];
		}

		//calc changes
		int *change = calloc(1, sizeof(int));
		for(int i = 0; i < w; i++) {
			if(tempShowField[i] != showField[i]) {
				change[0] = 1;
			}
		}

		showField = tempShowField;

		//collect change parameters
		int *changes = calloc(n-1, sizeof(int));
		MPI_Gather(change, 1, MPI_INT, changes, n-1, MPI_INT, 0, comm);

		//calc if somebody has changes
		if(rank == 0) {
			doNextLoop[0] = change; //add own change value
			for(int i = 0; i < n; i++) {
				if(changes[i] == 1) {
					doNextLoop[0] = 1;
				}
			}
		}

		//send to all if the loop should be continued
		MPI_Bcast(doNextLoop,1, MPI_INT, 0, comm);

		//show
//		double *completeField = calloc(w*h, sizeof(double)); //TODO own field
//		MPI_Gather(completeField, w*h, MPI_DOUBLE, showField, w, MPI_DOUBLE, 0, comm);
////	    printf("%ld timestep\n", t);
//		if(rank == 0) {
//		    printf("%ld timestep\n", t);
//		    show(completeField, w, h);
//		}

	    //TODO show
	  }

	MPI_Finalize();
	return 0;
}
