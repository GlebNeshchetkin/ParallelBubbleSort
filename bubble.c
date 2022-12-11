#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void swap(int* xp, int* yp)
{
    int temp = *xp;
    *xp = *yp;
    *yp = temp;
}

void bubbleSort(int* arr, int start, int end)
{
    int i, j;
    for (i = start; i < end - 1; i++)
        for (j = start; j < end - i - 1; j++)
            if (arr[j] > arr[j + 1])
                swap(&arr[j], &arr[j + 1]);
}

void exchange(int* first_arr, int size, int self_id, int rank1, int rank2) {
	int second_arr[size];
	MPI_Status status;
	MPI_Sendrecv(first_arr,size,MPI_INT,((self_id == rank1) ? rank2 : rank1),0,second_arr,size,MPI_INT,((self_id == rank1) ? rank2 : rank1),0,MPI_COMM_WORLD,&status);
	int i, j = 2*size;
	if(self_id == rank1) {
		i = size-1;
		for (j = 0; j < size; i--, j++) { 
			if (first_arr[i] > second_arr[j]) { 
				int temp = first_arr[i];
				first_arr[i] = second_arr[j];
				second_arr[j] = temp;
			}
		}
		bubbleSort(first_arr, 0, size);
		bubbleSort(second_arr, 0, size);
	} else {
		i = size-1;
		for (j = 0; j < size; i--, j++) { 
			if (second_arr[i] > first_arr[j]) {
				int temp = second_arr[i];
				second_arr[i] = first_arr[j];
				first_arr[j] = temp;
			}
		}
		bubbleSort(first_arr, 0, size);
		bubbleSort(second_arr, 0, size);
	}
}

int  main(int argc, char** argv) {
	int* arr;
	int *chunk;
	int num_proc, id, size, chunk_size;
	double time_taken;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &num_proc);
	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	if(id == 0) {
		FILE* fp = fopen(argv[1], "r");
		fscanf(fp, "%d", &size);
		chunk_size = size / num_proc;
		if(size % num_proc != 0) {chunk_size++;}
		MPI_Alloc_mem(chunk_size * num_proc * sizeof(int), MPI_INFO_NULL, &arr);
		int i;
		for(i = 0; i < size; i++) {
			fscanf(fp, "%d", &arr[i]);
			printf("%d ", arr[i]);
		}
		printf("\n");
		fclose(fp);
		for(int i = size; i < chunk_size * num_proc; i++) { arr[i] = __INT_MAX__; }
	}
	MPI_Barrier(MPI_COMM_WORLD);
	time_taken = MPI_Wtime();
	MPI_Bcast(&chunk_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Alloc_mem(chunk_size * sizeof(int), MPI_INFO_NULL, &chunk);
	MPI_Scatter(arr, chunk_size, MPI_INT, chunk, chunk_size, MPI_INT, 0, MPI_COMM_WORLD);
	if(id == 0) { MPI_Free_mem(arr); }
	bubbleSort(chunk, 0, chunk_size);
	MPI_Barrier(MPI_COMM_WORLD);
	int i = 1;
	for(i = 0; i < num_proc; i++) {
		if(i%2 == 0) { // even iteration
			if((id % 2 == 0) && (id < num_proc - 1)) {
				exchange(chunk, chunk_size, id, id, id+1);
			} else if (id % 2 == 1) {
				exchange(chunk, chunk_size, id, id-1, id);
			}
		} else { // odd iteration
			if((id % 2 == 1) && (id <= num_proc - 2)) {
				exchange(chunk, chunk_size, id, id, id+1);
			} else if((id %2 == 0) && (id > 0)) {
				exchange(chunk, chunk_size, id, id-1, id);
			}
		}
		MPI_Barrier(MPI_COMM_WORLD);
	}
	if(id == 0) {MPI_Alloc_mem(num_proc * chunk_size * sizeof(int), MPI_INFO_NULL, &arr);}
	int status = MPI_Gather(chunk, chunk_size, MPI_INT, arr, chunk_size, MPI_INT, 0, MPI_COMM_WORLD);
	time_taken = MPI_Wtime() - time_taken;
	MPI_Free_mem(chunk);
	if(id == 0) {
		for(i = 0; i < size; i++) {printf("%d ", arr[i]);}
		printf("\n");
		printf("Time taken for execution is %lf seconds\n", time_taken);
	}
	MPI_Finalize();
	return 0;
}
