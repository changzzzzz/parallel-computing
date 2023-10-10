
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#define MSG_EXIT 1
#define MSG_PRINT_ORDERED 2
#define MSG_PRINT_UNORDERED 3
#define SHIFT_ROW 0
#define SHIFT_COL 1
#define DISP 1


int master_io(MPI_Comm world_comm, MPI_Comm comm);
int slave_io(MPI_Comm world_comm, MPI_Comm comm);
void* ProcessFunc(void *pArg);

pthread_mutex_t g_Mutex = PTHREAD_MUTEX_INITIALIZER;
int g_nslaves = 0;


int main(int argc, char **argv)
{
    int ndims=2, size, rank, reorder, my_cart_rank, ierr, nrows, ncols;
    int dims[ndims],coord[ndims];
    int wrap_around[ndims];
    int nbr_i_lo, nbr_i_hi;
	int nbr_j_lo, nbr_j_hi;

    MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    // Check user input
    if (argc == 3) {
    nrows = atoi (argv[1]);
    ncols = atoi (argv[2]);
    dims[0] = nrows; /* number of rows */
    dims[1] = ncols; /* number of columns */
    if( (nrows*ncols) != size-1) {
        if( rank == 0) printf("ERROR: nrows * ncols + 1 != size: %d * %d + 1= %d != %d\n", nrows, ncols, nrows*ncols+1,size);
        MPI_Finalize(); 
        return 0;
		}
	}else{
        if( rank == 0) printf("nrows %d, ncols %d, cores: %d\n", nrows, ncols ,size);

    }


    // Create base and node
    MPI_Comm new_comm;
    MPI_Comm_split( MPI_COMM_WORLD, rank == size - 1 , 0, &new_comm);
    if (rank == size -1 ) 
	master_io( MPI_COMM_WORLD, new_comm);
    else
	slave_io( MPI_COMM_WORLD, new_comm);
    MPI_Finalize();


 



    return 0;
}

/* This is the base */
int master_io(MPI_Comm world_comm, MPI_Comm comm) 
{
    int i,j, size, numNode,firstmsg;

    char buf[256],buf2[256];
    int rank;

    MPI_Status status;
    MPI_Comm_size( world_comm, &size );
    numNode = size - 1;
    printf("start to receive, numNode%d\n",numNode);

    //receive data from node
	while (numNode > 0) {
		MPI_Recv(buf, 256, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, world_comm, &status );
		switch (status.MPI_TAG) {
			case MSG_EXIT: numNode--; break;
			case MSG_PRINT_UNORDERED:
				fputs( buf, stdout );
			break;
			case MSG_PRINT_ORDERED:
				firstmsg = status.MPI_SOURCE;
				for (i=0; i<numNode; i++) {
					if (i == firstmsg) 
						fputs( buf, stdout );
					else {
						MPI_Recv( buf2, 256, MPI_CHAR, i, MSG_PRINT_ORDERED, world_comm, &status );
						fputs( buf2, stdout );
					}
				}
			break;
		}
	}
    return 0;

}



/* This is the node */
int slave_io(MPI_Comm world_comm, MPI_Comm comm)
{ 
    int ndims=2, size, my_rank, reorder, my_cart_rank, ierr, worldSize;
	MPI_Comm comm2D;
	int dims[ndims],coord[ndims];
	int wrap_around[ndims];
	char buf[256];

    MPI_Comm_size(world_comm, &worldSize); // size of the world communicator
  	MPI_Comm_size(comm, &size); // size of the slave communicator
	MPI_Comm_rank(comm, &my_rank);  // rank of the slave communicator
	dims[0]=dims[1]=0;

    MPI_Dims_create(size, ndims, dims);
	if(my_rank==0)
	printf("Slave Rank: %d. Comm Size: %d: Grid Dimension = [%d x %d] \n",my_rank,size,dims[0],dims[1]);

	wrap_around[0] = 0;
	wrap_around[1] = 0; /* periodic shift is .false. */
	reorder = 0;
	ierr =0;
	ierr = MPI_Cart_create(comm, ndims, dims, wrap_around, reorder, &comm2D);
	if(ierr != 0) printf("ERROR[%d] creating CART\n",ierr);

	/* find my coordinates in the cartesian communicator group */
	MPI_Cart_coords(comm2D, my_rank, ndims, coord); // coordinated is returned into the coord array
	/* use my cartesian coordinates to find my rank in cartesian group*/
	MPI_Cart_rank(comm2D, coord, &my_cart_rank);



    
	printf("Global rank (within slave comm): %d. Cart rank: %d. Coord: (%d, %d).\n", my_rank, my_cart_rank, coord[0], coord[1]);
	fflush(stdout);


    ///////////
	sprintf( buf, "Hello from slave %d at Coordinate: (%d, %d)\n", my_rank, coord[0], coord[1]);
	MPI_Send( buf, strlen(buf) + 1, MPI_CHAR, worldSize-1, MSG_PRINT_ORDERED, world_comm );

	sprintf( buf, "Goodbye from slave %d at Coordinate: (%d, %d)\n", my_rank, coord[0], coord[1]);
	MPI_Send(buf, strlen(buf) + 1, MPI_CHAR, worldSize-1, MSG_PRINT_ORDERED, world_comm);

    MPI_Send(buf, 0, MPI_CHAR, worldSize-1, MSG_EXIT, world_comm);

    MPI_Comm_free( &comm2D );

    
    return 0;
}