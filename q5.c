#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>
#include <unistd.h>
#include <string.h>

#define NUM_RANGE 100
#define SHIFT_ROW 0
#define SHIFT_COL 1
#define SHIFT_DEP 2 // changing to 2 shifts
#define DISP 1

#define MAX_ITERATION 50
#define randomUB 50

//----------------------------------------------------------------------------------------------
/// Is Prime Function Definition
/// Input: integer number
/// Returns: 1 if input argument is a prime number, 0 otherwise
//-----------------------------------------------------------------------------------------------
int IsPrime(int input);
int CheckValue(int* recvValues, int val) ;

int main(int argc, char **argv)
{   
	int ndims=3, size, my_rank, reorder, my_cart_rank, ierr;
	int nrows, ncols, ndepts; 
	int nbr_i_lo, nbr_i_hi;
	int nbr_j_lo, nbr_j_hi;
	int nbr_k_lo, nbr_k_hi;
	MPI_Comm comm3D;
	int dims[ndims],coord[ndims];
	int wrap_around[ndims];
	char buf[256] = {0};
	FILE *pFile;
	
	unsigned int randomSeed; // random number generator seed
	int iteration_count = 1;
	int randVal;

	MPI_Request send_request[6];
	MPI_Request receive_request[6];
	MPI_Status send_status[6];
	MPI_Status receive_status[6];

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size); 
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank); 
	
	randomSeed = my_rank * time(NULL); // Initialize random seed for each process
	
	/* process command line arguments*/
	if (argc == 4) {
		nrows = atoi (argv[1]);
		ncols = atoi (argv[2]);
		ndepts = atoi (argv[3]);
		dims[0] = nrows; /* number of rows */
		dims[1] = ncols; /* number of columns */
		dims[2] = ndepts;
		if( (nrows*ncols*ndepts) != size) {
			if( my_rank ==0) printf("ERROR: nrows*ncols*ndepts)=%d * %d * %d = %d != %d\n", nrows, ncols, ndepts, nrows*ncols*ndepts,size);
			MPI_Finalize(); 
			return 0;
		}
	} else {
		nrows=ncols=(int)sqrt(size);
		dims[0]=dims[1]=dims[2]=0;
	}
	
	MPI_Dims_create(size, ndims, dims);
	if(my_rank==0)
		printf("Root Rank: %d. Comm Size: %d: Grid Dimension = [%d x %d x %d] \n",my_rank,size,dims[0],dims[1], dims[2]);

	/* create cartesian mapping */
	wrap_around[0] = 0;
	wrap_around[1] = 0; 
	wrap_around[2] = 0; /* periodic shift is .false. */
	reorder = 1;
	ierr = 0;
	ierr = MPI_Cart_create(MPI_COMM_WORLD, ndims, dims, wrap_around, reorder, &comm3D);
	if(ierr != 0) printf("ERROR[%d] creating CART\n",ierr);

	MPI_Cart_coords(comm3D, my_rank, ndims, coord); // coordinated is returned into the coord array
	MPI_Cart_rank(comm3D, coord, &my_cart_rank);

	MPI_Cart_shift( comm3D, SHIFT_ROW, DISP, &nbr_i_lo, &nbr_i_hi );
	MPI_Cart_shift( comm3D, SHIFT_COL, DISP, &nbr_j_lo, &nbr_j_hi );
	MPI_Cart_shift( comm3D, SHIFT_DEP, DISP, &nbr_k_lo, &nbr_k_hi );

	sprintf(buf, "log_%d.txt", my_rank);
	pFile = fopen(buf, "w");

	fprintf(pFile, "Global rank: %d. Cart rank: %d. Coord: (%d, %d, %d). Rear: %d. Front: %d. Top: %d. Bottom: %d. Left: %d. Right: %d. \n\n", my_rank, my_cart_rank, coord[0], coord[1], coord[2], nbr_j_lo, nbr_j_hi, nbr_i_lo, nbr_i_hi, nbr_k_lo, nbr_k_hi);

	
	while(iteration_count < MAX_ITERATION){
		
		while(true){
			randVal = rand_r(&randomSeed) % randomUB;
			if(IsPrime(randVal) == 0){
				break;
			}
		}
		
		int recvValues[6] = {-1, -1, -1, -1, -1, -1};
		int neighbors[6] = {nbr_j_lo, nbr_j_hi, nbr_i_lo, nbr_i_hi, nbr_k_lo, nbr_k_hi}; 

		for (int i = 0; i < 6; i++) {
			MPI_Isend(&randVal, 1, MPI_INT, neighbors[i], 0, comm3D, &send_request[i]);
			MPI_Irecv(&recvValues[i], 1, MPI_INT, neighbors[i], 0, comm3D, &receive_request[i]);
		}

		MPI_Waitall(6, send_request, send_status);
		MPI_Waitall(6, receive_request, receive_status);
		
		
		if(CheckValue(recvValues, randVal) >= 1){
			fprintf(pFile, "[ITERATION %d] Random value: %d; Recv Rear: %d; Recv Front: %d; Recv Top: %d; Recv Bottom: %d; Recv Left: %d; Recv Right: %d; Number of matches %d\n", iteration_count, randVal, recvValues[4], recvValues[5], recvValues[2], recvValues[3], recvValues[0], recvValues[1], CheckValue(recvValues, randVal));
		}
		

		iteration_count++; 
	}

	fclose(pFile);
	MPI_Comm_free( &comm3D );
	MPI_Finalize();
	return 0;
}

int CheckValue(int* recvValues, int val) {
	int retVal = 0;
	for (int i = 0; i < 6; i++) {
		if(recvValues[i] == val){
			retVal++;
		}
	}
	return retVal;
}

int IsPrime(int input)
{
	int j;
	int limit = (int)sqrt((double)input);

	for(j = 2; j <= limit; j++)
	{
		if (input % j == 0)
		{
			return 0;
		}
	}
	return 1;
}

