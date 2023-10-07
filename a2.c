
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#define MSG_EXIT 1
#define MSG_PRINT_UNORDERED 2

int master_io(MPI_Comm world_comm, MPI_Comm comm);
int slave_io(MPI_Comm world_comm, MPI_Comm comm);
void* ProcessFunc(void *pArg);

pthread_mutex_t g_Mutex = PTHREAD_MUTEX_INITIALIZER;
int g_nslaves = 0;


int main(int argc, char **argv)
{
    int ndims=2, size, my_rank, reorder, my_cart_rank, ierr, nrows, ncols, nbr_i_lo, nbr_i_hi;
    int dims[ndims],coord[ndims];

    MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if (argc == 3) {
    nrows = atoi (argv[1]);
    ncols = atoi (argv[2]);
    dims[0] = nrows; /* number of rows */
    dims[1] = ncols; /* number of columns */
    if( (nrows*ncols) != size-1) {
        if( my_rank == 0) printf("ERROR: nrows * ncols + 1 != size: %d * %d + 1= %d != %d\n", nrows, ncols, nrows*ncols+1,size);
        MPI_Finalize(); 
        return 0;
		}
	}else{
        if( my_rank == 0) printf("nrows %d, ncols %d, cores: %d\n", nrows, ncols ,size);

    }

    MPI_Comm new_comm;
    MPI_Comm_split( MPI_COMM_WORLD,my_rank == size-1, 0, &new_comm);

    if (my_rank == 0) 
	master_io( MPI_COMM_WORLD, new_comm );
    else
	slave_io( MPI_COMM_WORLD, new_comm );
    MPI_Finalize();
    return 0;




}

int master_io(MPI_Comm master_comm, MPI_Comm comm) 
{
    int i,j, size;
    char buf[256];
    MPI_Status status;
    MPI_Comm_size( master_comm, &size );
    for (j=1; j<=2; j++) {
	for (i=1; i<size; i++) {
    		MPI_Recv( buf, 256, MPI_CHAR, i, 0, master_comm, &status );
    		fputs( buf, stdout );
	}
    }
    return 0;
}

/* This is the slave */
int slave_io(MPI_Comm master_comm, MPI_Comm comm)
{
    char buf[256];
    int  rank;
    
    MPI_Comm_rank(comm, &rank);
    sprintf(buf, "Hello from slave %d\n", rank);
    MPI_Send( buf, strlen(buf) + 1, MPI_CHAR, 0, 0, master_comm ); // master_comm = MPI_COMM_WORLD
    
    sprintf( buf, "Goodbye from slave %d\n", rank );
    MPI_Send( buf, strlen(buf) + 1, MPI_CHAR, 0, 0, master_comm ); // master_comm = MPI_COMM_WORLD
    return 0;
}
