#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#define MSG_EXIT 1
#define MSG_INITIALIZE 2
#define MSG_PRINT_UNORDERED 3
#define MSG_END 4
#define MSG_REPORT 5
#define MSG_ANSWER 6
#define SHIFT_ROW 0
#define SHIFT_COL 1
#define DISP 1
#define K 10

int base_io(MPI_Comm world_comm);
int node_io(MPI_Comm world_comm, MPI_Comm comm);
void* MsgSendReceive(void *pArg);
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
	base_io( MPI_COMM_WORLD);
    else
	node_io( MPI_COMM_WORLD, new_comm);
    MPI_Finalize();
    return 0;
}

struct ThreadData {
    MPI_Comm world_comm;
};


/* This is the base */
int base_io(MPI_Comm world_comm) 
{
	int size, nNodes,i,j,k,flag=1;
	MPI_Comm_size(world_comm, &size);
	nNodes = size - 1;
	pthread_mutex_t g_Mutex = PTHREAD_MUTEX_INITIALIZER;
	struct ThreadData data;
	data.world_comm = world_comm;

	pthread_t tid;
	pthread_create(&tid, 0, MsgSendReceive, (void *)&data); // Create the thread
	pthread_join(tid, NULL);

	printf("master finished\n");
	fflush(stdout);
	pthread_mutex_destroy(&g_Mutex);
    return 0;

}

// base station receive and send messages from the EV charging nodes
void* MsgSendReceive(void *pArg) 
{

	struct ThreadData *data = (struct ThreadData *)pArg;
	MPI_Comm world_comm = data->world_comm;

	int size, nNodes,i=0,j,k,flag=1,firstmsg;
	char buf[256], buf2[256];
	FILE *pFile;
	MPI_Status status;
	MPI_Comm_size(world_comm, &size);
	nNodes = size - 1;
	pthread_mutex_t g_Mutex = PTHREAD_MUTEX_INITIALIZER;

	/**/
	//topology store the neighbour of each node
	int topology[nNodes][4];
	int queue[3][nNodes];
	int queuePointer=0;

	// Initialize the array 
	for(i=0;i<3;i++){
		for(j=0;j<nNodes;j++){
			queue[i][j]=-1;
		}
	}

	// Send initalize msg to node
	MPI_Request init_request;
	for(i=0;i<nNodes;i++){
		MPI_Isend(&flag, 1, MPI_INT, i, MSG_INITIALIZE, world_comm,&init_request);
	}


	int reportNumber = 1;
	char delimiter = ',';	
	int tempNumber,nodeNumber;

	// The base will top after receiving 10 reports 
	while (nNodes > 0 && reportNumber<11) {
		MPI_Recv(buf, 256, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		switch (status.MPI_TAG) {
			case MSG_EXIT: nNodes--; break;

			
			case MSG_INITIALIZE:
				// Recevie each node's neighbour information 
				pthread_mutex_lock(&g_Mutex);

				// data received can be split by ','
				char* token2 = strtok(buf, &delimiter);
				nodeNumber = atoi(token2);
				printf("Show topology of ");
				printf("node number: %d, neighbours: ", nodeNumber);
				fflush(stdout);
				for (i=0;i<4;i++) {
					token2 = strtok(NULL, &delimiter);
					printf("%s ", token2);
					fflush(stdout);
					//store the neighbours to array called topology
					// example: topology[0] = {-2,1,-2,3} (-2 means none. 1,3 are node 0 neighbour)
					topology[nodeNumber][i] = atoi(token2);
				}
				printf("\n");
				fflush(stdout);
				pthread_mutex_unlock(&g_Mutex);
				break;

			case MSG_REPORT:
				// Receive the report data when reporting node and neighbour are full


				// data received can be split by ','
				// Rank: %d; 
				// Coord: (%d, %d). 
				// Node's Load: %d; 
				// Value{ Recv Left: %d; Recv Right: %d; Recv Top: %d; Recv Bottom: %d;}
				// Rank{ Left: %d. Right: %d. Top: %d. Bottom: %d}

				int result[13];
				char* token = strtok(buf, &delimiter);
				for(i=0;i<13;i++){
				result[i]=atoi(token);
				token = strtok(NULL, &delimiter);
    			}
				time_t currentTime;
				time(&currentTime);
				char timeString[100]; 
				struct tm *localTime = localtime(&currentTime);
				strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", localTime);

				int sender_rank = status.MPI_SOURCE;

				// Create report file
				sprintf(buf, "log_%d.txt", reportNumber);
				pFile = fopen(buf, "w");
				fprintf(pFile, "Max port number %d\n",K);
				fprintf(pFile, "Report number %d\n",reportNumber);
				fprintf(pFile, "Alert reported time : %s \n",token);
				fprintf(pFile, "Logged time: %s \n",timeString);		 
				fprintf(pFile, "Reporting node     Coordinate     Prot Value     Availability to be considered full\n");
				fprintf(pFile, "%d                  (%d,%d)          %d              %d \n",result[0],result[1],result[2],result[3],K - result[3]);
				fprintf(pFile, "Adjacent node      Coordinate     Prot Value     Availability to be considered full\n");
				for(i=8;i<12;i++){
					if(result[i]>=0){
						fprintf(pFile, "%d                  (%d,%d)          %d              %d \n",result[i],result[i]/result[12],result[i]%result[12],result[i-4],K - result[i-4]);
						
					}
				}

				// freeNode store the nearyby neighbours
				int freeNode[nNodes];
				for(i=0;i<nNodes;i++){
					freeNode[i] = -1;
				}
				printf("Report number: %d Node: %d reports full\n",reportNumber,result[0]);
				queue[queuePointer][result[0]]=1;

				for(i=0;i<4;i++){
					if(result[8+i]>=0){
						queue[queuePointer][result[8+i]]=1;
						for(j=0;j<4;j++){
							if(topology[result[8+i]][j]>=0){
								freeNode[topology[result[8+i]][j]]=1;
							}
						}
					}
				}
				fprintf(pFile, "Nearby Nodes      Coordinate\n");

				// print nearby node with free charging port
				for(i=0;i<nNodes;i++){
					if(freeNode[i]==1 && i != result[0]){
						fprintf(pFile, "%d                 (%d,%d)\n",i,i/result[12],i%result[12]);
					}
				}

				fprintf(pFile, "Available station nearby (no report received in last 3 iteration): ");

				// find the nearby node didn't show in reports in the last 3 iteration and print in report
				int flag=1;
				for(i=0;i<nNodes;i++){
					if(freeNode[i]==1){
						for(j=0;j<3;j++){
							if(queue[j][i]==1){
								flag = 0;
							}
						}
						if(flag == 1){
							fprintf(pFile, "%d ",i);

						}
						flag=1;

					}
				}
				fprintf(pFile, "\n");
				fprintf(pFile, "End of file \n");
				fclose(pFile);
				reportNumber++;

				// queue first in first out loop
				queuePointer = (queuePointer + 1) % 3;
				break;
		}
	}
	// setting termination_message as flag send to node
	int termination_message = 1; 
	MPI_Request request;

	// send stop signal to node
	for(k=0;k<nNodes;k++){
		printf("Stop signal send to node: %d\n",k);
		fflush(stdout);
		MPI_Isend(&k, 1, MPI_INT, k, MSG_END, world_comm,&request);
	}
	printf("Boardcast finished\n");
	fflush(stdout);

	pthread_mutex_destroy(&g_Mutex);
    return 0;
	
}



struct ThreadArgs {
    int *result; 
	int rank;
};



/* This is the node */
// Generate random values and send to base once a second
int node_io(MPI_Comm world_comm, MPI_Comm comm)
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

	MPI_Dims_create(size, ndims, dims);
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
	
	// send and receive value of each node
	int action_list[4] = {nbr_j_lo, nbr_j_hi, nbr_i_lo, nbr_i_hi};
	// neighbour's rank
	int recv_data[4] = {-1, -1, -1, -1};
	


	// Receive stop signal from base
	int termination_received = 0;
	MPI_Request termination_request;
	MPI_Status termination_status;
	int termination_message = -1;
	MPI_Irecv(&termination_message, 1, MPI_INT, MPI_ANY_SOURCE, MSG_END, world_comm,&termination_request);
	
	// Keep working until receive stop signal
	while (!termination_received) {
		
		// Simulate the port number in pthread, store the value in num
		pthread_t tid;
		struct ThreadArgs args;
		args.result = &num;
		args.rank = my_rank;
		pthread_create(&tid, NULL, SimulatePortNumber, (void *)&args);
		pthread_join(tid, NULL); 


		// send and receive free port value with neighbour
		for (int i = 0; i < 4; i++){
			MPI_Isend(&num, 1, MPI_INT, action_list[i], 0, comm2D, &send_request[i]);
			MPI_Irecv(&recv_data[i], 1, MPI_INT, action_list[i], 0, comm2D, &receive_request[i]);
		}
		MPI_Waitall(4, send_request, send_status);
		MPI_Waitall(4, receive_request, receive_status);

		
		// send topology to base at beginning
		int initialize_received = 0;
		MPI_Request initialize_request;
		MPI_Status tinitialize_status;
		int initialize_message = -1;
		MPI_Irecv(&initialize_message, 1, MPI_INT, MPI_ANY_SOURCE, MSG_INITIALIZE, world_comm,&initialize_request);
		if(initialize_message >= 0){
			sprintf(buf, "%d,%d,%d,%d,%d,%d\n",my_cart_rank,nbr_j_lo, nbr_j_hi, nbr_i_lo, nbr_i_hi,dims[0]);
			MPI_Isend(&buf, 20, MPI_CHAR, worldSize - 1, MSG_INITIALIZE, world_comm, &initialize_request);
		}


		// printf("Rank: %d; Coord: (%d, %d). Node's Load: %d; Value{ Recv Left: %d; Recv Right: %d; Recv Top: %d; Recv Bottom: %d;}Rank{ Left: %d. Right: %d. Top: %d. Bottom: %d}\n", my_cart_rank,coord[0], coord[1], num, recv_data[0], recv_data[1], recv_data[2], recv_data[3],nbr_j_lo, nbr_j_hi, nbr_i_lo, nbr_i_hi);

		// When node is full 8/10, K =10
		if(num >= K - 2){
			int flag = 1;
			int freeNeighbour[4] = {0, 0, 0, 0};

			// if any neighbour has free port, flag = 0, and it will be recorded in freeNeighbour
			for (int i = 0; i < 4; i++) {

        		if (recv_data[i] > -1 && recv_data[i] < K- 2){
				
					flag = 0;
					freeNeighbour[i] = 1;
					
       			}else{

				}
			}

			// if all neighbour are nearly full (>=80%)
			if(flag){
				time_t currentTime;
				time(&currentTime);
				char timeString[100]; 
				struct tm *localTime = localtime(&currentTime);
				strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", localTime);
				if(my_cart_rank==0){
					printf("%s",timeString);
				}

				// send string to base, data is splited by ','
				// Rank: %d; 
				// Coord: (%d, %d). 
				// Node's Load: %d; 
				// Value{ Recv Left: %d; Recv Right: %d; Recv Top: %d; Recv Bottom: %d;}
				// Rank{ Left: %d. Right: %d. Top: %d. Bottom: %d}
				sprintf(buf, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s\n",my_cart_rank,coord[0], coord[1], num, recv_data[0], recv_data[1], recv_data[2], recv_data[3],nbr_j_lo, nbr_j_hi, nbr_i_lo, nbr_i_hi,dims[0],timeString);

				MPI_Send(buf, strlen(buf) + 1, MPI_CHAR, worldSize -1 , MSG_REPORT, world_comm);

			}
		}

		sleep(1);
		// Check stop signal
		MPI_Test(&termination_request, &termination_received, &termination_status);
	}
	

    MPI_Comm_free( &comm2D );
    return 0;
}

void* SimulatePortNumber(void *args)
// simulate the number of port being used
{
	struct ThreadArgs *threadArgs = (struct ThreadArgs *)args;
	int rank = threadArgs->rank;

	srand(time(NULL));
	unsigned int seed = time(NULL) * rank;
	int num = rand_r(&seed) % K;

    *(threadArgs->result) = num; 
    pthread_exit(NULL);
}



//mpicc a2.c -o a2 -lm
//mpirun -np 10 -oversubscribe a2 3 3


