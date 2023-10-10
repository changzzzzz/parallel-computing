
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
#define randomUB 10

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

void* ProcessFunc(void *pArg) // Common function prototype
{
	int i = 0, size, nslaves, firstmsg;
	char buf[256], buf2[256];
	MPI_Status status;
	MPI_Comm_size(MPI_COMM_WORLD, &size );
	
	int* p = (int*)pArg;
	nslaves = *p;

	while (nslaves > 0) {
		MPI_Recv(buf, 256, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status );
		switch (status.MPI_TAG) {
			case MSG_EXIT: nslaves--; break;
			case MSG_PRINT_UNORDERED:
				printf("Thread prints: %s", buf);
				fflush(stdout);
			break;
			case MSG_PRINT_ORDERED:
				firstmsg = status.MPI_SOURCE;
				for (i=0; i<size-1; i++) {
					if (i == firstmsg){
						printf("Thread prints: %s", buf);
						fflush(stdout);
					}else {
						MPI_Recv( buf2, 256, MPI_CHAR, i, MSG_PRINT_ORDERED, MPI_COMM_WORLD, &status );
						printf("Thread prints: %s", buf2);
						fflush(stdout);
					}
				}
			break;
		}
	}
	

	return 0;
}


/* This is the base */
int master_io(MPI_Comm world_comm, MPI_Comm comm) 
{
	int size, nslaves;
	MPI_Comm_size(world_comm, &size );
	nslaves = size - 1;
	
	pthread_t tid;
	pthread_create(&tid, 0, ProcessFunc, &nslaves); // Create the thread
	pthread_join(tid, NULL); 
    // int i,j, size, numNode,firstmsg;

    // char buf[256],buf2[256];
    // int rank;

    // MPI_Status status;
    // MPI_Comm_size( world_comm, &size );
    // numNode = size - 1;
    // printf("start to receive, numNode%d\n",numNode);

    // //receive data from node
	// while (numNode > 0) {
	// 	MPI_Recv(buf, 256, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, world_comm, &status );
	// 	switch (status.MPI_TAG) {
	// 		case MSG_EXIT: numNode--; break;
	// 		case MSG_PRINT_UNORDERED:
	// 			fputs( buf, stdout );
	// 		break;
	// 		case MSG_PRINT_ORDERED:
	// 			firstmsg = status.MPI_SOURCE;
	// 			for (i=0; i<numNode; i++) {
	// 				if (i == firstmsg) 
	// 					fputs( buf, stdout );
	// 				else {
	// 					MPI_Recv( buf2, 256, MPI_CHAR, i, MSG_PRINT_ORDERED, world_comm, &status );
	// 					fputs( buf2, stdout );
	// 				}
	// 			}
	// 		break;
	// 	}
	// }
    // return 0;

}



/* This is the node */
int slave_io(MPI_Comm world_comm, MPI_Comm comm)
{ 
    int ndims=2, size, my_rank, reorder, my_cart_rank, ierr, worldSize,num;
	int dims[ndims],coord[ndims];
	int wrap_around[ndims];
	char buf[256];

	MPI_Comm comm2D;
	MPI_Request send_request[4];
	MPI_Request receive_request[4];
	MPI_Status send_status[4];
	MPI_Status receive_status[4];
	

    MPI_Comm_size(world_comm, &worldSize); // size of the world communicator
  	MPI_Comm_size(comm, &size); // size of the slave communicator
	MPI_Comm_rank(comm, &my_rank);  // rank of the slave communicator
	dims[0]=dims[1]=0;

	// generate random number as free port
	unsigned int seed = time(NULL) * my_rank;
	num = rand_r(&seed) % randomUB;

	//t1
    MPI_Dims_create(size, ndims, dims);
	if(my_rank==0)
	printf("Slave Rank: %d. Comm Size: %d: Grid Dimension = [%d x %d] \n",my_rank,size,dims[0],dims[1]);


	//q2
	wrap_around[0] = 0;
	wrap_around[1] = 0; 
	reorder = 0;
	ierr =0;
	ierr = MPI_Cart_create(comm, ndims, dims, wrap_around, reorder, &comm2D);
	if(ierr != 0) printf("ERROR[%d] creating CART\n",ierr);

	MPI_Cart_coords(comm2D, my_rank, ndims, coord); 
	MPI_Cart_rank(comm2D, coord, &my_cart_rank);

	int nbr_i_lo,nbr_i_hi,nbr_j_lo,nbr_j_hi; // neighbour's rank
	MPI_Cart_shift( comm2D, SHIFT_ROW, DISP, &nbr_i_lo, &nbr_i_hi );
	MPI_Cart_shift( comm2D, SHIFT_COL, DISP, &nbr_j_lo, &nbr_j_hi );
	/*
		MPI_Isend(&num, 1, MPI_INT, action_list[i], 0, comm2D, &send_request[i]);
		MPI_Irecv(&recv_data[i], 1, MPI_INT, action_list[i], 0, comm2D, &receive_request[i]);
	*/
	int action_list[4] = {nbr_j_lo, nbr_j_hi, nbr_i_lo, nbr_i_hi};
	int recv_data[4] = {-1, -1, -1, -1};



	// Check current node is full or not
	if(num == randomUB-1){
		printf("No free port, need to check neighbour's value.\n");
	}
	// Check free port received from neighbour 



	//q2 send and receive value of each node
	for (int i = 0; i < 4; i++){
		MPI_Isend(&num, 1, MPI_INT, action_list[i], 0, comm2D, &send_request[i]);
		MPI_Irecv(&recv_data[i], 1, MPI_INT, action_list[i], 0, comm2D, &receive_request[i]);
	}
	MPI_Waitall(4, send_request, send_status);
	MPI_Waitall(4, receive_request, receive_status);

	//print result
	// printf("Cart rank: %d. Coord: (%d, %d). Rank{ Left: %d. Right: %d. Top: %d. Bottom: %d}\n",  my_cart_rank, coord[0], coord[1], nbr_j_lo, nbr_j_hi, nbr_i_lo, nbr_i_hi);
	// printf("Random value: %d; Value{ Recv Left: %d; Recv Right: %d; Recv Top: %d; Recv Bottom: %d;}\n", num, recv_data[0], recv_data[1], recv_data[2], recv_data[3]);
	// printf("Cart rank: %d. Coord: (%d, %d). Value{ Recv Left: %d; Recv Right: %d; Recv Top: %d; Recv Bottom: %d;} \n",  my_cart_rank, coord[0], coord[1],recv_data[0], recv_data[1], recv_data[2], recv_data[3]);



    //t1
	// printf("Global rank (within slave comm): %d. Cart rank: %d. Coord: (%d, %d).\n", my_rank, my_cart_rank, coord[0], coord[1]);
	// fflush(stdout);


    ///////////
	// sprintf( buf, "Hello from slave %d at Coordinate: (%d, %d)\n", my_rank, coord[0], coord[1]);
	// MPI_Send( buf, strlen(buf) + 1, MPI_CHAR, worldSize-1, MSG_PRINT_ORDERED, world_comm );

	// sprintf( buf, "Goodbye from slave %d at Coordinate: (%d, %d)\n", my_rank, coord[0], coord[1]);
	// MPI_Send(buf, strlen(buf) + 1, MPI_CHAR, worldSize-1, MSG_PRINT_ORDERED, world_comm);
    while (1) {
        unsigned int seed = time(NULL) * my_rank;
        int num = rand_r(&seed) % randomUB; // 生成随机数
		sprintf(buf, "%d\n", num);
		// sprintf( buf, "haha\n");
        // 发送随机数给master
        MPI_Send(buf, strlen(buf) + 1, MPI_CHAR, worldSize - 1, MSG_PRINT_ORDERED, world_comm);
		// MPI_Send(&num, 0, MPI_CHAR, worldSize-1, MSG_EXIT, world_comm);
        // 等待五秒
        sleep(5);
    }
    // MPI_Send(buf, 0, MPI_CHAR, worldSize-1, MSG_EXIT, world_comm);

    MPI_Comm_free( &comm2D );



    return 0;
}
//mpicc a2.c -o a2 -lm
//mpirun -np 7 -oversubscribe t1 3 2