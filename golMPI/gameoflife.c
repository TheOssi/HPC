#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>
#include <mpi.h>

#define calcIndex(width, x,y)  ((y)*(width) + (x))

long TimeSteps = 100;
long WidthHeight = 30;
long SplitIndex = 10;

void fill(double* currentfield, int x, int y) {
	currentfield[calcIndex(WidthHeight, x, y)] = 1;
}

void unfill(double* currentfield, int x, int y) {
	currentfield[calcIndex(WidthHeight, x, y)] = 0;
}

bool isFilled(double* currentfield, int x, int y) {
	if (x < 0) {
		x = WidthHeight - 1;
	}
	if (x > (WidthHeight - 1)) {
		x = 0;
	}
	if (y < 0) {
		y = WidthHeight - 1;
	}
	if (y > (WidthHeight - 1)) {
		y = 0;
	}
	return currentfield[calcIndex(WidthHeight, x, y)] == 1 ? true : false;
}

int getNeighbourhood(double* currentfield, int x, int y) {
	int counter = 0;
	for (int i = -1; i <= 1; i++) {
		for (int j = -1; j <= 1; j++) {
			if (isFilled(currentfield, x + i, y + j) && (i != 0 || j != 0)) {
				counter++;
			}
		}
	}
	return counter;
}

void writeVTK2(long timestep, double *data, char prefix[1024], long w, long h) {
	char filename[2048];
	int x, y;

	long offsetX = 0;
	long offsetY = 0;
	float deltax = 1.0;
	float deltay = 1.0;
	long nxy = w * h * sizeof(float);

	snprintf(filename, sizeof(filename), "%s-%05ld%s", prefix, timestep,
			".vti");
	FILE* fp = fopen(filename, "w");

	fprintf(fp, "<?xml version=\"1.0\"?>\n");
	fprintf(fp,
			"<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n");
	fprintf(fp,
			"<ImageData WholeExtent=\"%d %d %d %d %d %d\" Origin=\"0 0 0\" Spacing=\"%le %le %le\">\n",
			offsetX, offsetX + w - 1, offsetY, offsetY + h - 1, 0, 0, deltax,
			deltax, 0.0);
	fprintf(fp, "<CellData Scalars=\"%s\">\n", prefix);
	fprintf(fp,
			"<DataArray type=\"Float32\" Name=\"%s\" format=\"appended\" offset=\"0\"/>\n",
			prefix);
	fprintf(fp, "</CellData>\n");
	fprintf(fp, "</ImageData>\n");
	fprintf(fp, "<AppendedData encoding=\"raw\">\n");
	fprintf(fp, "_");
	fwrite((unsigned char*) &nxy, sizeof(long), 1, fp);

	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			float value = data[calcIndex(h, x, y)];
			fwrite((unsigned char*) &value, sizeof(float), 1, fp);
		}
	}

	fprintf(fp, "\n</AppendedData>\n");
	fprintf(fp, "</VTKFile>\n");
	fclose(fp);
}

void show(double* currentfield, int w, int h) {
	printf("\033[H");
	int x, y;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++)
			printf(
					currentfield[calcIndex(w, x, y)] ?
							"\033[07m  \033[m" : "  ");
		//printf("\033[E");
		printf("\n");
	}
	fflush(stdout);
}

void evolve(double* currentfield, double* newfield, int w, int h) {
	int x, y;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {

			int neighbourhood = getNeighbourhood(currentfield, x, y);
			bool currentIsFilled = isFilled(currentfield, x, y);

			if (!currentIsFilled) {
				if (neighbourhood == 3) {
					fill(newfield, x, y);
				} else {
					unfill(newfield, x, y);
				}
			} else {

				if (neighbourhood < 2 || neighbourhood > 3) {
					unfill(newfield, x, y);
				} else {
					fill(newfield, x, y);
				}

			}

		}
	}
}

void filling(double* currentfield, int w, int h) {
	int i;
	int fieldCounter = 0;

	FILE *f1 = fopen("field.txt", "r");
	fseek(f1, 0, SEEK_END);
	int inputsize = ftell(f1);
	char *input1 = calloc(inputsize, sizeof(char));
	fseek(f1, 0, SEEK_SET);
	for (i = 0; i < inputsize; i++) {
		fscanf(f1, "%c", &input1[i]);
		if (input1[i] == '0') {
			currentfield[fieldCounter] = 0;
			fieldCounter++;
		}
		if (input1[i] == '1') {
			currentfield[fieldCounter] = 1;
			fieldCounter++;
		}
	}
}

void game(int w, int h) {
	double *currentfield = calloc(w * h, sizeof(double));
	double *newfield = calloc(w * h, sizeof(double));
		filling(currentfield, w, h);
		long t;
		for (t = 0; t < TimeSteps; t++) {
			//show(currentfield, w, h);
			break;
			evolve(currentfield, newfield, w, h);

			printf("%ld timestep\n", t);
			writeVTK2(t, currentfield, "gol", w, h);

			usleep(200000);

			//SWAP
			double *temp = currentfield;
			currentfield = newfield;
			newfield = temp;
		}


	//free(currentfield);
	//free(newfield);

}

int main(int c, char **v) {
	MPI_Init(&c, &v);
	int rank, size, source, dest;
	MPI_Comm comm;
	int dim[2], period[2], reorder;
	int coord[2];
	coord[1] = 2;

	MPI_Status status;

	MPI_Comm_size(MPI_COMM_WORLD, &size);
	dim[0] = size;

	period[0] = 1;
	reorder = 1;
	if ((WidthHeight * WidthHeight) % size == 0) {

		MPI_Cart_create(MPI_COMM_WORLD, 1, dim, period, reorder, &comm);

		MPI_Comm_rank(MPI_COMM_WORLD, &rank);

		MPI_Cart_coords(comm, rank, 2, coord);
		printf("Rank %d coordinates are %d %d\n", rank, coord[0], coord[1]);

		MPI_Cart_shift(comm, 1, 1, &source, &dest);
		if (rank % 2 == 0) {
			MPI_Send(showField, w, MPI_DOUBLE,rankRight, 0, comm);
			MPI_Recv(rightField, w, MPI_DOUBLE,rankRight, 0, comm, &status);
			MPI_Send(showField, w, MPI_DOUBLE,rankLeft, 0, comm);
			MPI_Recv(leftField, w, MPI_DOUBLE,rankLeft, 0, comm, &status);
		}else{

		}

		printf("%d | %d | %d\n", source, rank, dest);
		int w = 0, h = 0;
			if (c > 1)
				w = atoi(v[1]); ///< read width
			if (c > 2)
				h = atoi(v[2]); ///< read height
			if (w <= 0)
				w = WidthHeight; ///< default width
			if (h <= 0)
				h = WidthHeight; ///< default height
			MPI_Finalize();
			game(w, h);

	}else{
		printf("Size % Threads != 0");
	}

}
