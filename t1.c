#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>

#define MSG_EXIT 1
#define MSG_PRINT_ORDERED 2
#define MSG_PRINT_UNORDERED 3

int master_io(MPI_Comm world_comm, MPI_Comm comm);
int node_io(MPI_Comm world_comm, MPI_Comm comm);

int main(int argc, char **argv)
{
    int rank, size;
    MPI_Comm new_comm;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
	//size - 1 master 
    MPI_Comm_split( MPI_COMM_WORLD,rank == size-1, 0, &new_comm); // color will either be 0 or 1 
    if (rank == size-1) 
	master_io( MPI_COMM_WORLD, new_comm );
    else
	node_io( MPI_COMM_WORLD, new_comm );
    MPI_Finalize();
    return 0;
}

/* This is the master */
int master_io(MPI_Comm world_comm, MPI_Comm comm)
{
	int        i, size, nslaves, firstmsg;
	char       buf[256], buf2[256];
	MPI_Status status;
	MPI_Comm_size(world_comm, &size );
	nslaves = size - 1;
	
	while (nslaves > 0) {
		MPI_Recv(buf, 256, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, world_comm, &status );
		switch (status.MPI_TAG) {
			case MSG_EXIT: nslaves--; break;
			case MSG_PRINT_UNORDERED:
				fputs( buf, stdout );
			break;
			case MSG_PRINT_ORDERED:
				firstmsg = status.MPI_SOURCE;
				for (i=0; i<nslaves; i++) {
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

/* This is the slave */
int node_io(MPI_Comm world_comm, MPI_Comm comm)
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

	/* create cartesian mapping */
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


	sprintf( buf, "Hello from slave %d at Coordinate: (%d, %d)\n", my_rank, coord[0], coord[1]);
	MPI_Send( buf, strlen(buf) + 1, MPI_CHAR, worldSize-1, MSG_PRINT_ORDERED, world_comm );

	sprintf( buf, "Goodbye from slave %d at Coordinate: (%d, %d)\n", my_rank, coord[0], coord[1]);
	MPI_Send(buf, strlen(buf) + 1, MPI_CHAR, worldSize-1, MSG_PRINT_ORDERED, world_comm);
	
	sprintf(buf, "Slave %d at Coordinate: (%d, %d) is exiting\n", my_rank, coord[0], coord[1]);
	MPI_Send(buf, strlen(buf) + 1, MPI_CHAR, worldSize-1, MSG_PRINT_ORDERED, world_comm);
	MPI_Send(buf, 0, MPI_CHAR, worldSize-1, MSG_EXIT, world_comm);

    	MPI_Comm_free( &comm2D );
	return 0;
}