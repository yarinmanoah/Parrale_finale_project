#include <omp.h>
#include <stdlib.h>
#include <ctype.h>
#include <cstring>
#include "mpi.h"
#include "struct.h"  //  "struct.h" contains the definition of the 'struct score_alignment'

#define MATRIX_SIZE 26
#define ROOT 0
#define MAX_STRING_SIZE 3000

// Function to calculate the maximum score without using a grade table
void caculate_max_score_no_grade_table(char *str_to_check, char *first_str, struct score_alignment *AS_ptr)
{
    int lenght_first_str = strlen(first_str);
    int size_str_to_check = strlen(str_to_check);
    int sqn_taries = (size_str_to_check < lenght_first_str) ? (lenght_first_str - size_str_to_check)  
                                                            : (size_str_to_check - lenght_first_str);
    int off_set, max_score = 0;
    int k, max_k, score = 0;
    for (off_set = 0; off_set <= sqn_taries; off_set++)
    {
        fprintf(stderr, "off_set %d thread num = %d num threads = %d\n", off_set, omp_get_thread_num(), omp_get_num_threads());
        for (k = 0; k < size_str_to_check; k++)
        {
            score = 0;
            // OpenMP parallelization with reduction to calculate the score
            #pragma omp parallel for reduction(+ : score)
            for (int i = 0; i < size_str_to_check; i++)
            {
                if (i >= k)
                    score = (*((first_str + i) + off_set) == toupper(*((str_to_check + i)) + 1));
                else
                {
                    if (*((first_str + i) + off_set) == str_to_check[i])
                        score++;
                }
            }

            if (max_score <= score)
            {
                AS_ptr->K = k;
                AS_ptr->off_set = off_set;
                AS_ptr->score = score;
                max_score = score;
            }
        }
    }
}

// Function to calculate the maximum score using a grade table
void caculate_max_score_grade_table(char *str_to_check, char *first_str, int matrix[MATRIX_SIZE][MATRIX_SIZE], struct score_alignment *AS_ptr)
{
    int lenght_first_str = strlen(first_str);
    int size_str_to_check = strlen(str_to_check);
    int sqn_taries = (size_str_to_check < lenght_first_str) ? (lenght_first_str - size_str_to_check)
                                                            : (size_str_to_check - lenght_first_str);
    int off_set = 0, max_off_set, max_score = 0;
    int k = 0, max_k, score;
    for (off_set = 0; off_set <= sqn_taries; off_set++)
    {
        for (k = 0; k < size_str_to_check; k++)
        {
            score = 0;
            // OpenMP parallelization with reduction to calculate the score
            #pragma omp parallel for reduction(+ : score)
            for (int i = 0; i < size_str_to_check; i++)
            {
                char c1 = *(first_str + i + off_set);
                char c2 = *(str_to_check + i);
                c1 = toupper(c1);
                c2 = toupper(c2);
                int x = c1 - 'A';
                int y = c2 - 'A';

                if (i >= k)
                {
                    y++;
                    if (c2 == 'Z')
                        y = 0;
                }

                if (x >= 0 && x < MATRIX_SIZE && y >= 0 && y < MATRIX_SIZE) // Check bounds
                {
                    score += matrix[x][y];
                }
            }

            if (max_score <= score)
            {
                AS_ptr->K = k;
                AS_ptr->off_set = off_set;
                AS_ptr->score = score;
                max_score = score;
            }
        }
    }
}

// Function to create a custom MPI data type for 'struct score_alignment'
void make_datatype(MPI_Datatype *mpi_score_alignment_type)
{
    // Making a new type for 'struct score_alignment'
    MPI_Datatype types[4] = {MPI_CHAR, MPI_INT, MPI_INT, MPI_INT};
    int block_lengths[4] = {MAX_STRING_SIZE, 1, 1, 1}; // Initialized to zeros
    MPI_Aint displacements[4];
    struct score_alignment temp; // Used to calculate displacements

    // Calculate displacements
    MPI_Get_address(&temp.str, &displacements[0]);
    MPI_Get_address(&temp.K, &displacements[1]);
    MPI_Get_address(&temp.score, &displacements[2]);
    MPI_Get_address(&temp.off_set, &displacements[3]);

    for (int i = 3; i > 0; i--)
    {
        displacements[i] -= displacements[0];
    }
    displacements[0] = 0;

    // Create the custom data type
    MPI_Type_create_struct(4, block_lengths, displacements, types, mpi_score_alignment_type);
    MPI_Type_commit(mpi_score_alignment_type);
}

