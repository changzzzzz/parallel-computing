//---------------------------------------------------------------------------------------------------------------------
// MPI_Scatter_Gather.c
//
// Scatter and Gather test
//
// http://www.mpi-forum.org/docs/mpi-11-html/node72.html
// http://www.mcs.anl.gov/research/projects/mpi/mpi-standard/mpi-report-1.1/node70.htm#Node70
//
// Last updated by: Vishnu Monn
// Last updated date: 19th September 2021
//---------------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"

#define BUFF_ELEMENTS 12

int main(int argc, char* argv[])
{
	int my_rank;
	int p;
	int tag = 0;
	int n = 0, i = 0;
	int *pSendBuffer;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &p);

	pSendBuffer = (int*)malloc(BUFF_ELEMENTS * sizeof(int));
	if(my_rank == 0){
		for(int i = 0; i < BUFF_ELEMENTS; i++){
			pSendBuffer[i] = rand() % 1500;
		}
	}

	int *pReceiveBuffer = (int*)malloc(BUFF_ELEMENTS * sizeof(int));
	memset(pReceiveBuffer, 0, BUFF_ELEMENTS * sizeof(int));

	// Scatter the buffer to all processors
	int elementsPerProcess = BUFF_ELEMENTS / p; // e.g., BUFF_ELEMENTS = 12, p  = 3;
	MPI_Scatter(pSendBuffer, elementsPerProcess, MPI_INT, pReceiveBuffer, elementsPerProcess, MPI_INT, 0, MPI_COMM_WORLD);

	char fileName[32] = {0};
	sprintf(fileName, "DataFile_Scatter_%d.txt", my_rank);
	FILE *pFile = fopen(fileName, "w");
	for(int i = 0; i < elementsPerProcess; i++)
	{
		fprintf(pFile, "%d\n", pReceiveBuffer[i]);
	}
	fclose(pFile);

	// Do something to the received buffer
	for(int i = 0; i < elementsPerProcess; i++)
	{
		pReceiveBuffer[i] = pReceiveBuffer[i] * pReceiveBuffer[i];
	}
	
	// Gather the receive buffer data into send buffer.
	memset(pSendBuffer, NULL, (sizeof(int) * BUFF_ELEMENTS));
	MPI_Gather(pReceiveBuffer, elementsPerProcess, MPI_INT, pSendBuffer, elementsPerProcess, MPI_INT, 0, MPI_COMM_WORLD);

	if(my_rank == 0)
	{
		pFile = fopen("DataFile_Gather_0.txt", "w");
		for(int i = 0; i < BUFF_ELEMENTS; i++)
		{
			fprintf(pFile, "%d\n", pSendBuffer[i]);
		}
		fclose(pFile);
	}

	free(pSendBuffer);
	free(pReceiveBuffer);
	MPI_Finalize();
	return 0;
}