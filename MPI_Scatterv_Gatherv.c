//---------------------------------------------------------------------------------------------------------------------
// MPI_Scatterv_Gatherv.c
//
// Scatterv and Gatherv test for unequal data distribution
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

#define BUFF_ELEMENTS 1201

int main(int argc, char* argv[])
{
	int my_rank;
	int p;
	int tag = 0;
	int n = 0;
	MPI_Status status;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &p);

	int *pSendBuffer = (int*)malloc(BUFF_ELEMENTS * sizeof(int));
	memset(pSendBuffer, 0, BUFF_ELEMENTS * sizeof(int));
	if(my_rank == 0)
	{
		for(int i = 0; i < BUFF_ELEMENTS; i++)
		{
			pSendBuffer[i] = rand() % 1500;
		}
	}
	int *pReceiveBuffer = (int*)malloc(BUFF_ELEMENTS * sizeof(int));
	memset(pReceiveBuffer, 0, BUFF_ELEMENTS * sizeof(int));

	int elementsPerProcess = BUFF_ELEMENTS / p; // e.g, BUFF_ELEMENTS = 1201, p = 3, elementsPerProcess = 400
	int elementsPerProcessRemain = BUFF_ELEMENTS % p; // elementsPerProcessRemain  = 1

	int elementsToScatterGather = 0;
	if(my_rank == (p-1))
		elementsToScatterGather = elementsPerProcess + elementsPerProcessRemain;
	else
		elementsToScatterGather = elementsPerProcess;

	int *pSendCounts = (int*)malloc(p * sizeof(int)); // Heap array for number of processes
	int *pDispls = (int*)malloc(p * sizeof(int)); // Heap array to keep an offset (displacement)
	for(int i = 0; i < p; i++)
	{
		if(i == (p-1))
		{
			pSendCounts[i] = elementsPerProcess + elementsPerProcessRemain;
			pDispls[i] = i * elementsPerProcess;
		}
		else
		{
			pSendCounts[i] = elementsPerProcess;
			pDispls[i] = i * elementsPerProcess; // start point for each process
		}
	}

	// Scatter the buffer to all processors
	MPI_Scatterv(pSendBuffer, pSendCounts, pDispls, MPI_INT, pReceiveBuffer, elementsToScatterGather, MPI_INT, 0, MPI_COMM_WORLD);

	char fileName[32] = {0};
	sprintf(fileName, "DataFile_Scatter_%d.txt", my_rank);
	FILE *pFile = fopen(fileName, "w");
	for(int i = 0; i < elementsPerProcess; i++)
	{
		fprintf(pFile, "%d\n", pReceiveBuffer[i]);
	}

	if(my_rank == (p-1))
	{
		for(int i = elementsPerProcess; i < (elementsPerProcess + elementsPerProcessRemain); i++)
			fprintf(pFile, "%d\n", pReceiveBuffer[i]);
	}
	fclose(pFile);

	// Do something to the received buffer (i.e., find the square root of each element in buffer)
	for(int i = 0; i < elementsPerProcess; i++)
	{
		pReceiveBuffer[i] = pReceiveBuffer[i] * pReceiveBuffer[i];
	}

	memset(pSendBuffer, NULL, (sizeof(int) * BUFF_ELEMENTS));
	MPI_Gatherv(pReceiveBuffer, elementsToScatterGather, MPI_INT, pSendBuffer, pSendCounts, pDispls, MPI_INT, 0, MPI_COMM_WORLD);
	
	if(my_rank == 0)
	{
		pFile = fopen("DataFile_Gather_0.txt", "w");
		for(int i = 0; i < BUFF_ELEMENTS; i++)
		{
			fprintf(pFile, "%d\n", pSendBuffer[i]);
		}
		fclose(pFile);
	}

	//// Gather the receive buffer data into send buffer.
	//memset(pSendBuffer, NULL, (sizeof(int) * BUFF_ELEMENTS));
	//MPI_Gather(pReceiveBuffer, elementsPerProcess, MPI_INT, pSendBuffer, elementsPerProcess, MPI_INT, 0, MPI_COMM_WORLD);

	//// Receive the remaining data from the last node.
	//if(my_rank == (p-1))
	//{
	//	MPI_Send((int*)pReceiveBuffer + elementsPerProcess, elementsPerProcessRemain, MPI_INT, 0, 99, MPI_COMM_WORLD);
	//}
	//else if(my_rank == 0)
	//{
	//	int offset = elementsPerProcess * p;
	//	MPI_Recv((int*)pSendBuffer + offset, elementsPerProcessRemain, MPI_INT, (p-1), 99, MPI_COMM_WORLD, &status);
	//}

	free(pSendCounts);
	free(pDispls);
	free(pSendBuffer);
	free(pReceiveBuffer);
	
	MPI_Finalize();
	return 0;
}