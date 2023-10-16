
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
#define MSG_END 4
#define MSG_REPORT 5
#define MSG_ANSWER 6
#define SHIFT_ROW 0
#define SHIFT_COL 1
#define DISP 1
#define K 10

int master_io(MPI_Comm world_comm, MPI_Comm comm);
int slave_io(MPI_Comm world_comm, MPI_Comm comm);
void* ProcessFunc(void *pArg);
void* SimulatePortNumber(void *pArg);
void* ResponseToNeighbour(void *pArg);



pthread_mutex_t g_Mutex = PTHREAD_MUTEX_INITIALIZER;
int g_nslaves = 0;


int main(int argc, char **argv)
{
    int ndims=2, size, rank, reorder, my_cart_rank, ierr, nrows, ncols,provided;
    int dims[ndims],coord[ndims];
    int wrap_around[ndims];
    int nbr_i_lo, nbr_i_hi;
	int nbr_j_lo, nbr_j_hi;

    // MPI_Init(NULL, NULL);
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided );
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
	FILE *pFile;
	MPI_Status status;
	MPI_Comm_size(MPI_COMM_WORLD, &size );
	
	int* p = (int*)pArg;
	nslaves = *p;
	int reportNumber = 1;

	while (nslaves > 0 && reportNumber<10) {
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
			case MSG_REPORT:
				printf("Thread prints: %s", buf);
				fflush(stdout);

				int result[13];
				char delimiter = ',';
				char* token = strtok(buf, &delimiter);
				for(i=0;i<13;i++){
				
				// printf("%s\n", token);
				result[i]=atoi(token);
				token = strtok(NULL, &delimiter);
    			}
				// printf("%s",token);
				time_t currentTime;
				time(&currentTime);
				char timeString[100]; 
				struct tm *localTime = localtime(&currentTime);
				strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", localTime);

				int sender_rank = status.MPI_SOURCE;
				sprintf(buf, "log_%d.txt", reportNumber);
				pFile = fopen(buf, "w");
				fprintf(pFile, "Max port number %d\n",K);
				fprintf(pFile, "Report number %d\n",reportNumber);
				fprintf(pFile, "Alert reported time : %s \n",token);
				fprintf(pFile, "Logged time: %s \n",timeString);		 
				fprintf(pFile, "Reporting node     Coordinate     Prot Value     Availability to be considered full\n");
				fprintf(pFile, "%d                  (%d,%d)          %d              %d \n",result[0],result[1],result[2],result[3],K - result[3]);
				fprintf(pFile, "Adjacent node     Coordinate     Prot Value     Availability to be considered full\n");
				for(i=8;i<12;i++){
					if(result[i]>=0){
						fprintf(pFile, "%d                 (%d,%d)          %d              %d \n",result[i],result[i]/result[12],result[i]%result[12],result[i-4],K - result[i-4]);
					}
				}
				fprintf(pFile, "End of file \n");
				// printf("Rank: 0%d; Coord: (1%d, 2%d). Node's Load: 3%d; Value{ Recv Left: 4%d; Recv Right: 5%d; Recv Top: 6%d; Recv Bottom: 7%d;}Rank{ Left: 8%d. Right: 9%d. Top:10 %d. Bottom: 11%d}\n", my_cart_rank,coord[0], coord[1], num, recv_data[0], recv_data[1], recv_data[2], recv_data[3],nbr_j_lo, nbr_j_hi, nbr_i_lo, nbr_i_hi);
				fclose(pFile);
				reportNumber++;
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
	
	// pthread_t tid;
	// pthread_create(&tid, 0, ProcessFunc, &nslaves); // Create the thread
	// pthread_join(tid, NULL);
	int i = 0, firstmsg;
	char buf[256], buf2[256];
	FILE *pFile;
	MPI_Status status;
	// MPI_Comm_size(MPI_COMM_WORLD, &size );
	
	// int* p = (int*)pArg;
	// nslaves = *p;
	int reportNumber = 1;

	while (nslaves > 0 && reportNumber<10) {
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
			case MSG_REPORT:
				printf("Thread prints: %s", buf);
				fflush(stdout);

				int result[13];
				char delimiter = ',';
				char* token = strtok(buf, &delimiter);
				for(i=0;i<13;i++){
				
				// printf("%s\n", token);
				result[i]=atoi(token);
				token = strtok(NULL, &delimiter);
    			}
				// printf("%s",token);
				time_t currentTime;
				time(&currentTime);
				char timeString[100]; 
				struct tm *localTime = localtime(&currentTime);
				strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", localTime);

				int sender_rank = status.MPI_SOURCE;
				sprintf(buf, "log_%d.txt", reportNumber);
				pFile = fopen(buf, "w");
				fprintf(pFile, "Max port number %d\n",K);
				fprintf(pFile, "Report number %d\n",reportNumber);
				fprintf(pFile, "Alert reported time : %s \n",token);
				fprintf(pFile, "Logged time: %s \n",timeString);		 
				fprintf(pFile, "Reporting node     Coordinate     Prot Value     Availability to be considered full\n");
				fprintf(pFile, "%d                  (%d,%d)          %d              %d \n",result[0],result[1],result[2],result[3],K - result[3]);
				fprintf(pFile, "Adjacent node     Coordinate     Prot Value     Availability to be considered full\n");
				for(i=8;i<12;i++){
					if(result[i]>=0){
						fprintf(pFile, "%d                 (%d,%d)          %d              %d \n",result[i],result[i]/result[12],result[i]%result[12],result[i-4],K - result[i-4]);
					}
				}
				fprintf(pFile, "End of file \n");
				// printf("Rank: 0%d; Coord: (1%d, 2%d). Node's Load: 3%d; Value{ Recv Left: 4%d; Recv Right: 5%d; Recv Top: 6%d; Recv Bottom: 7%d;}Rank{ Left: 8%d. Right: 9%d. Top:10 %d. Bottom: 11%d}\n", my_cart_rank,coord[0], coord[1], num, recv_data[0], recv_data[1], recv_data[2], recv_data[3],nbr_j_lo, nbr_j_hi, nbr_i_lo, nbr_i_hi);
				fclose(pFile);
				reportNumber++;
				break;
		}
	}

	int termination_message = 6; 
	MPI_Request request;

	int k;
	// MPI_Send(&termination_message, 1, MPI_INT, 1, MSG_EXIT, MPI_COMM_WORLD);

	for(k=0;k<size-1;k++){
		printf("Stop signal send to rank: %d\n",k);
		fflush(stdout);
		MPI_Isend(&k, 1, MPI_INT, k, MSG_END, world_comm,&request);
	}
	printf("Boardcast finished\n");
	fflush(stdout);
    return 0;

}


struct ThreadArgs {
    int *result; 
	int rank;
};



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

	//t1
	MPI_Dims_create(size, ndims, dims);
	// if(my_rank==0)
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

	int nbr_i_lo,nbr_i_hi,nbr_j_lo,nbr_j_hi; 
	// Check free port received from neighbour 
	MPI_Cart_shift( comm2D, SHIFT_ROW, DISP, &nbr_i_lo, &nbr_i_hi );
	MPI_Cart_shift( comm2D, SHIFT_COL, DISP, &nbr_j_lo, &nbr_j_hi );
	
	//q2 send and receive value of each node
	int action_list[4] = {nbr_j_lo, nbr_j_hi, nbr_i_lo, nbr_i_hi};
	int recv_data[4] = {-1, -1, -1, -1};
	// neighbour's rank

	// Receive stop signal from base
	int termination_received = 0;
	MPI_Request termination_request;
	MPI_Status termination_status;
	int termination_message = -1;
	MPI_Irecv(&termination_message, 1, MPI_INT, MPI_ANY_SOURCE, MSG_END, world_comm,&termination_request);
	
	// Keep working until stop signal
	while (!termination_received) {
		
		// Simulate the port number in pthread, store the value in num
		pthread_t tid;
		struct ThreadArgs args;
		args.result = &num;
		args.rank = my_rank;
		pthread_create(&tid, NULL, SimulatePortNumber, (void *)&args);
		pthread_join(tid, NULL); 

		// int received_data;
		// MPI_Request receive_from_neighbour_status;
		// MPI_Status neighbour_status;
		// while(1){
		// 	MPI_Irecv(&received_data, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &receive_from_neighbour_status);
		// 	int received_flag = 0;
        // 	MPI_Test(&receive_from_neighbour_status, &received_flag, &neighbour_status);
		// 	if(received_flag){

		// 	}
		// 	MPI_Wait(&receive_from_neighbour_status, &neighbour_status);
		// 	int sender_rank = status.MPI_SOURCE;
		
		// }

		for (int i = 0; i < 4; i++){
			MPI_Isend(&num, 1, MPI_INT, action_list[i], 0, comm2D, &send_request[i]);
			MPI_Irecv(&recv_data[i], 1, MPI_INT, action_list[i], 0, comm2D, &receive_request[i]);
		}
		MPI_Waitall(4, send_request, send_status);
		MPI_Waitall(4, receive_request, receive_status);

		//print result
		// if(my_rank == 1){
			// Neighbour's rank : nbr_j_lo
			// printf("Cart rank: %d. Coord: (%d, %d). Rank{ Left: %d. Right: %d. Top: %d. Bottom: %d}\n",  my_cart_rank, coord[0], coord[1], nbr_j_lo, nbr_j_hi, nbr_i_lo, nbr_i_hi);
			// Self value + Neighbour's value : 
		// printf("Rank: %d; Coord: (%d, %d). Node's Load: %d; Value{ Recv Left: %d; Recv Right: %d; Recv Top: %d; Recv Bottom: %d;}Rank{ Left: %d. Right: %d. Top: %d. Bottom: %d}\n", my_cart_rank,coord[0], coord[1], num, recv_data[0], recv_data[1], recv_data[2], recv_data[3],nbr_j_lo, nbr_j_hi, nbr_i_lo, nbr_i_hi);
				// printf("Rank: %d; Coord: (%d, %d).   %d; %d;  %d; %d;}\n", my_cart_rank,coord[0], coord[1], recv_data[0], recv_data[1], recv_data[2], recv_data[3]);

			// Neighbour's value : recv_data[0-3]
			// printf("Cart rank: %d. Coord: (%d, %d). Value{ Recv Left: %d; Recv Right: %d; Recv Top: %d; Recv Bottom: %d;} \n",  my_cart_rank, coord[0], coord[1],recv_data[0], recv_data[1], recv_data[2], recv_data[3]);
		

		// This node is full
		if(num >= K - 2){
			int flag = 1;
			int freeNeighbour[4] = {0, 0, 0, 0};

			// if any neighbour has free port, flag = 0, and it will be recorded in freeNeighbour
			for (int i = 0; i < 4; i++) {
				// printf("%d ",recv_data[i]);
				// fflush(stdout);
        		if (recv_data[i] > -1 && recv_data[i] < K- 2){
				
					flag = 0;
					freeNeighbour[i] = 1;
					
       			}else{

				}
			}
			// printf("flag = %d \n",flag);
			// fflush(stdout);
			if(flag){
				time_t currentTime;
				time(&currentTime);
				char timeString[100]; 
				struct tm *localTime = localtime(&currentTime);
				strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", localTime);
				// printf("Current local time: %s\n", timeString);
				// // printf("report\n");
				sprintf(buf, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s\n",my_cart_rank,coord[0], coord[1], num, recv_data[0], recv_data[1], recv_data[2], recv_data[3],nbr_j_lo, nbr_j_hi, nbr_i_lo, nbr_i_hi,dims[0],timeString);
				// printf("Rank: %d; Coord: (%d, %d). Node's Load: %d; Value{ Recv Left: %d; Recv Right: %d; Recv Top: %d; Recv Bottom: %d;}Rank{ Left: %d. Right: %d. Top: %d. Bottom: %d}\n", my_cart_rank,coord[0], coord[1], num, recv_data[0], recv_data[1], recv_data[2], recv_data[3],nbr_j_lo, nbr_j_hi, nbr_i_lo, nbr_i_hi);

				MPI_Send(buf, strlen(buf) + 1, MPI_CHAR, worldSize -1 , MSG_REPORT, world_comm);

				// sprintf(buf, "Slave %d at Coordinate: (%d, %d) is exiting\n", my_rank, coord[0], coord[1]);
				// MPI_Send(buf, strlen(buf) + 1, MPI_CHAR, worldSize-1, MSG_PRINT_ORDERED, world_comm);

			}
		}








		sleep(1);
		// Check stop signal
		MPI_Test(&termination_request, &termination_received, &termination_status);
	}
	

	// // exit
    // sprintf(buf, "Exit notification from %d\n", my_rank);
	// MPI_Send(buf, strlen(buf) + 1, MPI_CHAR, worldSize-1, MSG_EXIT, world_comm);



    MPI_Comm_free( &comm2D );
    return 0;
}

void* SimulatePortNumber(void *args) // Common function prototype
{
	struct ThreadArgs *threadArgs = (struct ThreadArgs *)args;
	int rank = threadArgs->rank;

	srand(time(NULL));
	unsigned int seed = time(NULL) * rank;
	int num = rand_r(&seed) % K;
	// printf("rank:%d, K:%d, randomNumber:%d \n",rank, K, num);
	// fflush(stdout);

    *(threadArgs->result) = num; 
    pthread_exit(NULL);
}



//mpicc a2.c -o a2 -lm
//mpirun -np 7 -oversubscribe t1 3 2





		//print result
		// if(my_rank == 1){
			// Neighbour's rank : nbr_j_lo
			// printf("Cart rank: %d. Coord: (%d, %d). Rank{ Left: %d. Right: %d. Top: %d. Bottom: %d}\n",  my_cart_rank, coord[0], coord[1], nbr_j_lo, nbr_j_hi, nbr_i_lo, nbr_i_hi);
			// Self value + Neighbour's value : 
		// printf("Rank: %d; Random value: %d; Value{ Recv Left: %d; Recv Right: %d; Recv Top: %d; Recv Bottom: %d;}\n", my_cart_rank, num, recv_data[0], recv_data[1], recv_data[2], recv_data[3]);
			// Neighbour's value : recv_data[0-3]
			// printf("Cart rank: %d. Coord: (%d, %d). Value{ Recv Left: %d; Recv Right: %d; Recv Top: %d; Recv Bottom: %d;} \n",  my_cart_rank, coord[0], coord[1],recv_data[0], recv_data[1], recv_data[2], recv_data[3]);
		