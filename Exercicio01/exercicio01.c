#include <stdio.h>
#include <mpi.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    MPI_Init(NULL, NULL);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    const int M = 5 * world_size;
    float *numbers = NULL;
    float result_sum = 0.0;
    int random_amount;
    int task_count = 0;
    int received_count = 0;

    if (world_rank == 0)
    {
        srand(time(NULL));

        for (int j = 1; j < world_size; j++)
        {
            // Generate a random number between 1000 and 2000
            int min = 1000;
            int max = 2000;
            random_amount = min + rand() % (max - min + 1);

            // Allocate and generate random floating point numbers
            numbers = (float *)malloc(random_amount * sizeof(float));
            for (int i = 0; i < random_amount; i++)
            {
                numbers[i] = (float)rand() / RAND_MAX * 100.0; // Random float between 0 and 100
            }

            // Send the amount and data to the next available process
            MPI_Send(&random_amount, 1, MPI_INT, j, 0, MPI_COMM_WORLD);
            MPI_Send(numbers, random_amount, MPI_FLOAT, j, 0, MPI_COMM_WORLD);

            free(numbers);
            task_count++;
        }

        // Collect results from worker processes
        while (received_count < (world_size - 1))
        {
            float value;
            MPI_Status status;
            MPI_Recv(&value, 1, MPI_FLOAT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
            result_sum += value;
            int sender_rank = status.MPI_SOURCE;
            printf("Received %f from process %d\n", value, sender_rank);
            received_count++;
        }

        printf("Total sum of slaves: %f\n", result_sum);
    }
    else
    {
        // Worker processes
        while (task_count < M)
        {
            MPI_Status status;
            MPI_Probe(0, 0, MPI_COMM_WORLD, &status);

            int count;
            MPI_Get_count(&status, MPI_INT, &count);

            if (status.MPI_SOURCE == 0)
            {
                MPI_Recv(&random_amount, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                numbers = (float *)malloc(random_amount * sizeof(float));
                MPI_Recv(numbers, random_amount, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                // Compute the local sum
                float local_sum = 0.0;
                for (int i = 0; i < random_amount; i++)
                {
                    local_sum += numbers[i];
                }

                // Send the result sum back to the master
                MPI_Send(&local_sum, 1, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);

                printf("Process %d sent sum %f to the master\n", world_rank, local_sum);

                free(numbers);
                task_count++;
            }
        }
    }

    MPI_Finalize();
    return 0;
}