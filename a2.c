
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

    // Create topology
    MPI_Comm comm2D;        
    wrap_around[0]=0;
    wrap_around[1]=0;
    reorder = 0;
    ierr = 0;
	ierr = MPI_Cart_create(MPI_COMM_WORLD, ndims, dims, wrap_around, reorder, &comm2D);
	if(ierr != 0) printf("ERROR[%d] creating CART\n",ierr);

    if (rank<6){
    MPI_Cart_coords(comm2D, rank, ndims, coord); // coordinated is returned into the coord array
	MPI_Cart_rank(comm2D, coord, &my_cart_rank);

    
	MPI_Cart_shift( comm2D, SHIFT_ROW, DISP, &nbr_i_lo, &nbr_i_hi );
	MPI_Cart_shift( comm2D, SHIFT_COL, DISP, &nbr_j_lo, &nbr_j_hi );
	printf("Global rank: %d.Cart rank: %d.Coord:(%d, %d).Top: %d.Bottom: %d.Left: %d.Right: %d.\n\n",rank, my_cart_rank, coord[0], coord[1], nbr_j_lo, nbr_j_hi, nbr_i_lo, nbr_i_hi);
    }


    // Create base and node
    MPI_Comm new_comm;
    MPI_Comm_split( MPI_COMM_WORLD, rank == size - 1 , 0, &new_comm);
    if (rank == 0) 
	master_io( MPI_COMM_WORLD, new_comm );
    else
	slave_io( MPI_COMM_WORLD, new_comm );
    MPI_Finalize();






    return 0;
}

/* This is the base */
int master_io(MPI_Comm master_comm, MPI_Comm comm) 
{
    int i,j, size, numNode,firstmsg;
    int nbr_i_lo, nbr_i_hi;
	int nbr_j_lo, nbr_j_hi;
    char buf[256],buf2[256];
    int rank;

    MPI_Status status;
    MPI_Comm_size( master_comm, &size );
    numNode = size - 1;
    printf("start to receive, numNode%d\n",numNode);


    while(numNode>0){

        MPI_Recv( buf, 256, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, master_comm, &status );
        
        
        switch (status.MPI_TAG) {
			case MSG_EXIT: 
				// fputs( buf, stdout ); // not requires to print the exit notification message
				numNode--; 
				break;
			case MSG_PRINT_UNORDERED:
				fputs( buf, stdout );
			break;
			case MSG_PRINT_ORDERED:
				firstmsg = status.MPI_SOURCE;
				for (i=1; i<size; i++) {
					if (i == firstmsg) 
						fputs( buf, stdout );
					else {
						MPI_Recv( buf2, 256, MPI_CHAR, i, MSG_PRINT_ORDERED, 
						master_comm, &status );
						fputs( buf2, stdout );
					}
				}
			break;
		}
    }
    printf("end of receive\n");
    MPI_Request send_request[4];
	MPI_Request receive_request[4];
	MPI_Status send_status[4];
	MPI_Status receive_status[4];

    int recvValues[4] = {-1, -1, -1, -1};
    int neighbors[4] = {nbr_j_lo, nbr_j_hi, nbr_i_lo, nbr_i_hi}; 

    for (int i = 0; i < 6; i++) {
    MPI_Isend(567, 1, MPI_INT, neighbors[i], 0, comm, &send_request[i]);
    MPI_Irecv(&recvValues[i], 1, MPI_INT, neighbors[i], 0, comm, &receive_request[i]);
    }



    // for (j=1; j<=2; j++) {
	// for (i=1; i<size; i++) {
    // 		MPI_Recv( buf, 256, MPI_CHAR, i, 0, master_comm, &status );
    // 		fputs( buf, stdout );
	// }
    // }


    return 0;
}

/* This is the node */
int slave_io(MPI_Comm master_comm, MPI_Comm comm)
{
    char buf[256];
    int  rank;
    
    MPI_Comm_rank(master_comm, &rank);
    // sprintf(buf, "Hello from slave %d\n", rank);
    // MPI_Send( buf, strlen(buf) + 1, MPI_CHAR, 0, 0, master_comm ); // master_comm = MPI_COMM_WORLD
    
    // sprintf( buf, "Goodbye from slave %d\n", rank );
    // MPI_Send( buf, strlen(buf) + 1, MPI_CHAR, 0, 0, master_comm ); // master_comm = MPI_COMM_WORLD
    
    


    sprintf( buf, "Hello from slave %d\n", rank );
	MPI_Send( buf, strlen(buf) + 1, MPI_CHAR, 0, MSG_PRINT_ORDERED, master_comm );

	sprintf(buf, "Goodbye from slave %d\n", rank);
	MPI_Send(buf, strlen(buf) + 1, MPI_CHAR, 0, MSG_PRINT_ORDERED, master_comm);
	
        	//can use sleep(...) or usleep(...) to delay the process of sending unordered output message for a random timing to simulate the processes sends the message in different timing
	sprintf(buf, "I'm exiting (%d)\n", rank);
	MPI_Send(buf, strlen(buf) + 1, MPI_CHAR, 0, MSG_PRINT_UNORDERED, master_comm);
	
	sprintf(buf, "Exit notification from %d\n", rank);
	MPI_Send(buf, strlen(buf) + 1, MPI_CHAR, 0, MSG_EXIT, master_comm);

    
    return 0;
}