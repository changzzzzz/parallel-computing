#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include "mpi.h"

#define NBRILO 10
#define NBRIHI 11
#define NBRJLO 12
#define NBRJHI 13
#define SHIFT_ROW 0
#define SHIFT_COL 1
#define DISP 1

#define MAX_ITERATION 50
#define randomUB 50

//----------------------------------------------------------------------------------------------
/// Is Prime Function Definition
/// Input: integer number
/// Returns: 1 if input argument is a prime number, 0 otherwise
//-----------------------------------------------------------------------------------------------
int IsPrime(int input);

int main(int argc, char *argv[]) {

	int ndims=2, size, my_rank, reorder, my_cart_rank, ierr, nrows, ncols, nbr_i_lo, nbr_i_hi;
	int nbr_j_lo, nbr_j_hi;
	MPI_Comm comm2D;
	int dims[ndims],coord[ndims];
	int wrap_around[ndims];
	int i;
	char buf[256] = {0};
	FILE *pFile;
	
	int iteration_count = 1;
	int num;
	int matching_count = 0;
	
        MPI_Request send_request[4];
        MPI_Request receive_request[4];
        MPI_Status send_status[4];
        MPI_Status receive_status[4];
	
	/* start up initial MPI environment */
	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	
	unsigned int seed = time(NULL) * my_rank;
	
	
	
	/* process command line arguments*/
	if (argc == 3) {
		nrows = atoi (argv[1]);
		ncols = atoi (argv[2]);
		dims[0] = nrows; /* number of rows */
		dims[1] = ncols; /* number of columns */
		if( (nrows*ncols) != size) {
			if( my_rank ==0) printf("ERROR: nrows*ncols)=%d * %d = %d != %d\n", nrows, ncols, nrows*ncols,size);
			MPI_Finalize(); 
			return 0;
		}
	} else {
		nrows=ncols=(int)sqrt(size);
		dims[0]=dims[1]=0;
	}
	
	/*************************************************************/
	/* create cartesian topology for processes */
	/*************************************************************/
	MPI_Dims_create(size, ndims, dims);
	if(my_rank==0)
		printf("PW[%d], CommSz[%d]: PEdims = [%d x %d] \n",my_rank,size,dims[0],dims[1]);
	
	/* create cartesian mapping */
	wrap_around[0] = wrap_around[1] = 0; /* periodic shift is .false. */
	reorder = 1;
	ierr =0;
	ierr = MPI_Cart_create(MPI_COMM_WORLD, ndims, dims, wrap_around, reorder, &comm2D);
	if(ierr != 0) printf("ERROR[%d] creating CART\n",ierr);
	
	/* find my coordinates in the cartesian communicator group */
	MPI_Cart_coords(comm2D, my_rank, ndims, coord);
	/* use my cartesian coordinates to find my rank in cartesian group*/
	MPI_Cart_rank(comm2D, coord, &my_cart_rank);
	/* get my neighbors; axis is coordinate dimension of shift */
	/* axis=0 ==> shift along the rows: P[my_row-1]: P[me] : P[my_row+1] */
	/* axis=1 ==> shift along the columns P[my_col-1]: P[me] : P[my_col+1] */
	
	MPI_Cart_shift( comm2D, SHIFT_ROW, DISP, &nbr_i_lo, &nbr_i_hi );
	MPI_Cart_shift( comm2D, SHIFT_COL, DISP, &nbr_j_lo, &nbr_j_hi );
	int action_list[4] = {nbr_j_lo, nbr_j_hi, nbr_i_lo, nbr_i_hi};
	int recv_data[4] = {-1, -1, -1, -1};

	sprintf(buf, "log_%d.txt", my_rank);
	pFile = fopen(buf, "w");
	
	fprintf(pFile, "Global rank: %d. Cart rank: %d. Coord: (%d, %d). Left: %d. Right: %d. Top: %d. Bottom: %d\n\n", my_rank, my_cart_rank, coord[0], coord[1], nbr_j_lo, nbr_j_hi, nbr_i_lo, nbr_i_hi);

	
	
	while(iteration_count < MAX_ITERATION){
			
		matching_count = 0; //reset the matching count
			
		while(true){
			num = rand_r(&seed) % randomUB;
			if(IsPrime(num) == 0){
				break;
			}
		}
		
		
		for (i = 0; i < 4; i++){
		   MPI_Isend(&num, 1, MPI_INT, action_list[i], 0, comm2D, &send_request[i]);
		   MPI_Irecv(&recv_data[i], 1, MPI_INT, action_list[i], 0, comm2D, &receive_request[i]);
		}
		MPI_Waitall(4, send_request, send_status);
		MPI_Waitall(4, receive_request, receive_status);
		
		// printf("Global rank: %d. Cart rank: %d. Coord: (%d, %d). Rand: %d. Recv Vals - ", my_rank, my_cart_rank, coord[0], coord[1], num);
		for (i = 0; i < 4; i++){
			//printf("recv_data[%d] = %d\t", i, recv_data[i]);
			if(num==recv_data[i]){
				matching_count++;
			}
		}
		
		// record the informatioo if there is any matches
		if(matching_count>=1){
			fprintf(pFile, "[ITERATION %d] Random value: %d; Recv Left: %d; Recv Right: %d; Recv Top: %d; Recv Bottom: %d; Num of matches:%d\n", iteration_count, num, recv_data[0], recv_data[1], recv_data[2], recv_data[3], matching_count);
		}
		
		iteration_count++;
	}
	
	fclose(pFile);
	MPI_Comm_free(&comm2D);
	MPI_Finalize();
	return 0;
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
