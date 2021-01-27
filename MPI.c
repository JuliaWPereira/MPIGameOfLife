#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <sys/time.h>

#define N 2048 /* grid dimension */
#define SRAND_VALUE 1985 /* seed to generate pseudo-random numbers to fill the first grid*/
#define MAX_IT 2000

/* Auxiliar function to print the matrix */
void PrintMatrix(int **grid, int slice){
    for(int i = 0;i<slice+2;i++){
        for(int j = 0; j < N+2; j++){
            printf("%3d", grid[i][j]);
        }
        printf("\n");
    }
}

int getNeighbors(int i, int j, int **grid)
{
   /* as we consider alive as 1 and dead as 0, we can sum their to get the number of alive neighbors */
    return grid[i-1][j-1] + grid[i-1][j] + grid[i-1][j+1] + grid[i][j-1] + grid[i][j+1] + grid[i+1][j-1] + grid[i+1][j] + grid[i+1][j+1];
}

void SetNextState(int i, int j, int **grid, int **newGrid)
{
    int qty = getNeighbors(i,j,grid); /* get number of alive neighbors */
    /* If their is less than 2 alive neighbors, then the cell dies */
    if(qty < 2)
        newGrid[i][j] = 0;
    /* An alive cell still alive if it has 2 or 3 alive neighbors */
    else if(grid[i][j] == 1 && (qty == 2|| qty == 3))
        newGrid[i][j] = 1;
    /* An alive cell dies if it has more than 3 neighbors */
    else if(qty >= 4)
        newGrid[i][j] = 0;
    /* A dead cell become alive if it has 3 alive neighbors */
    else if(grid[i][j] == 0 && qty == 3)
        newGrid[i][j] = 1;
    else{
        newGrid[i][j] = 0;
    }
}       


int main(int argc, char * argv[]) {

    /* definition of the random value with defined SEED */
    
    int processId; /* rank dos processos */
    int noProcesses; /* Número de processos */
    int nameSize; /* Tamanho do nome */
    int start = 0, end = 0;
    char computerName[MPI_MAX_PROCESSOR_NAME];

    int **grid; /* principal matrix */
    int **newGrid; /* auxiliar matrix */

    int recA[N], recB[N];
    int sendA[N], sendB[N];
    int before, after;

    double time1, time2,duration,global;

    MPI_Status status;

    

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &noProcesses);
    MPI_Comm_rank(MPI_COMM_WORLD, &processId);
    MPI_Get_processor_name(computerName, &nameSize);

    time1 = MPI_Wtime();

    int slice = (int)ceil(((double)N/(double)noProcesses));
    int qtyAlive = 0;

    start = processId*slice;
    slice = start + slice < N ? slice : N-start+1;

    before = ((processId+noProcesses-1)%noProcesses);
    after = ((processId+1)%noProcesses);

    grid = (int **)malloc(sizeof(int *) * (slice+2));
    newGrid = (int **)malloc(sizeof(int *) * (slice+2));
    for(int i = 0;i<slice+2;i++){
        grid[i] = (int*)malloc(sizeof(int) * N+2);
        newGrid[i] = (int*)malloc(sizeof(int) * N+2);
    }
    srand(SRAND_VALUE);
    for(int i = 0; i<processId*slice*N;i++){
        rand();
    }

    for(int i = 1;i<slice+1;i++){
        for(int j = 1; j < N+1; j++){
            grid[i][j] = rand()%2; /* 0 or 1 */
        }
    }
    
    /* Iterate for MAX_IT generations */
    for(int k = 0; k < MAX_IT; k++){
        //fill the buffers to send to neighbors
        for(int j = 0; j < N;j++){
            sendA[j] = grid[1][j+1];
            sendB[j] = grid[slice][j+1];
        }

        MPI_Sendrecv (sendB, N, MPI_INTEGER, after, 1,
                    recA, N, MPI_INTEGER, before, 1,
                    MPI_COMM_WORLD, &status);

        MPI_Sendrecv (sendA, N, MPI_INTEGER, before, 1,
                    recB,  N, MPI_INTEGER, after, 1,
                    MPI_COMM_WORLD, &status);
        

        for(int j = 0; j < N;j++){
            grid[0][j+1] = recA[j];
            grid[slice+1][j+1] = recB[j];
        }

        for(int i = 0; i < slice+2; i++){
            grid[i][N+1] = grid[i][1];
            grid[i][0] = grid[i][N];
        }


        /* Get the next generation of Game of Life */
        for(int i = 1; i < slice+1; i++){
            for(int j = 1; j < N+1;j++)
            {
                SetNextState(i,j, grid, newGrid);
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);
        /* get quantity of alive cells and copy newGrid to grid */
        qtyAlive = 0;
        for(int i = 1; i < slice+1; i++){
            for(int j = 1; j < N+1;j++)
            {
                grid[i][j] = newGrid[i][j];
                qtyAlive += newGrid[i][j];
            }
        } 
    }

    if(processId == 0)
    {
        int results[noProcesses], sum = 0;
        results[0] = qtyAlive;
        
        for(int i = 1; i < noProcesses; i++){
            MPI_Recv(&results[i], 1, MPI_INTEGER,i,10, MPI_COMM_WORLD, &status);
        }

        for(int i = 0; i < noProcesses; i++){
            sum += results[i];
        }
        printf("\nTotal: %d\n",sum);
    }
    else{
        MPI_Send(&qtyAlive, 1, MPI_INTEGER,0,10, MPI_COMM_WORLD); 
    }

    duration = time2 - time1;

    if(processId == 0) {
        time2 = MPI_Wtime();
        duration = time2 - time1;
        printf("Runtime at %d is %f \n", processId,duration);
    }
    MPI_Finalize();

    return 0;
}
