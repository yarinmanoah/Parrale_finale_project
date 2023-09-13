#include <omp.h>
#include <stdlib.h>
#include <ctype.h>
#include "mpi.h"
#include "cFunctions.h"
#include <cstring>

#define MATRIX_SIZE 26
#define ROOT 0
#define MAX_STRING_SIZE 1000
enum matrix_score
{
    THERE_IS_MATRIX_SCORE ,
    NO_MATRIX_SCORE
}how_to_caculate;

enum tags
{
    WORK,
    STOP,
    DONE
};
struct score_alignment
{
   char str [MAX_STRING_SIZE];
   int sqn;
   int MS;
   int score;
};

int lenght_first_str;
int number_strings;
char* first_str;
int matrix [MATRIX_SIZE][MATRIX_SIZE];
int readMatrixFromFile(const char* filename, int matrix[MATRIX_SIZE][MATRIX_SIZE]);
void init(int argc, char **argv); 
extern int computeOnGPU(const char  *s1, const char *s2);
extern int computeOnGPUWithMatrix(const char  *s1, const char *s2 ,const int matrix[MATRIX_SIZE][MATRIX_SIZE]);

int main(int argc, char *argv[]) 
{

    int my_rank, num_procs;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    //making new type-struct score_alignment
    MPI_Datatype mpi_score_alignment_type;
    MPI_Datatype types[4] = {MPI_CHAR, MPI_INT, MPI_INT, MPI_INT};
    int block_lengths[4] = { MAX_STRING_SIZE ,1 , 1,1}; // Initialized to zeros
    MPI_Aint displacements[4];
    struct score_alignment temp; // Used to calculate displacements

    // Calculate displacements
    MPI_Get_address(&temp.str, &displacements[0]);
    MPI_Get_address(&temp.sqn, &displacements[1]);
    MPI_Get_address(&temp.MS, &displacements[2]);
    MPI_Get_address(&temp.score, &displacements[3]);

    for (int i = 3; i > 0; i--) {
        displacements[i] -= displacements[0];
    }
    displacements[0] = 0;

    // Create the custom data type
    MPI_Type_create_struct(4, block_lengths, displacements, types, &mpi_score_alignment_type);
    MPI_Type_commit(&mpi_score_alignment_type);

    if (my_rank==0)
    {
      init(argc, argv);

      int int_enum = (int)how_to_caculate;
      MPI_Bcast(&int_enum , 1 , MPI_INT , ROOT , MPI_COMM_WORLD);
      if (how_to_caculate==THERE_IS_MATRIX_SCORE)
           MPI_Bcast(matrix , MATRIX_SIZE*MATRIX_SIZE , MPI_INT , ROOT , MPI_COMM_WORLD);
      MPI_Status status;
      double t_start = MPI_Wtime();
      MPI_Bcast(&lenght_first_str , 1 , MPI_INT , ROOT , MPI_COMM_WORLD);
      MPI_Bcast(first_str ,lenght_first_str * sizeof(char) , MPI_CHAR , ROOT , MPI_COMM_WORLD);
      char* str_to_send;
      int str_length;
      for (int worker_rank = 1; worker_rank < num_procs; worker_rank++)
        {
            //MPI_Send(&int_enum , 1 , MPI_INT , ROOT , MPI_ANY_TAG,MPI_COMM_WORLD);
            // MPI_Send(&lenght_first_str , 1 , MPI_INT , ROOT , MPI_ANY_TAG,MPI_COMM_WORLD);
            // MPI_Send(first_str ,lenght_first_str * sizeof(char) , MPI_CHAR , ROOT ,MPI_ANY_TAG , MPI_COMM_WORLD);
            str_to_send = createDynStr();
            str_length = strlen(str_to_send);
            #ifdef DEBUG
                printf("send to rank %d -%s\n",worker_rank , str_to_send);

            #endif
            MPI_Send(&str_length , 1 , MPI_INT , worker_rank  , WORK , MPI_COMM_WORLD);
            MPI_Send(str_to_send, (str_length+1)*sizeof(char) , MPI_CHAR , worker_rank, WORK, MPI_COMM_WORLD);
        }
        int str_send = num_procs-1; 
        int tasks = number_strings;
        for (int tasks_done = 0; tasks_done<number_strings; tasks_done++)
        {
             struct  score_alignment localMax;
            localMax.MS =0;
            localMax.score =0;
            localMax.MS =0;
            
            MPI_Recv(&localMax, 1, mpi_score_alignment_type, MPI_ANY_SOURCE,
                            DONE, MPI_COMM_WORLD, &status);
            printf("\nfor the string %s \n, we found that the max score alignment %d is from MS  - %d and sqn - %d  \n",
            localMax.str , localMax.score , localMax.MS , localMax.sqn);
            int tasks_not_sent_yet = tasks - str_send;
            if (tasks_not_sent_yet > 0) {
                    
                    str_to_send = createDynStr();
                    str_length = strlen(str_to_send);
                    MPI_Send(&str_length , 1 , MPI_INT, status.MPI_SOURCE  , WORK , MPI_COMM_WORLD);
                    MPI_Send(str_to_send, (str_length+1)*sizeof(char) , MPI_CHAR , status.MPI_SOURCE, WORK, MPI_COMM_WORLD);
                    str_send++;
                }
            else {
                    /* send STOP message. message has no data */
                    int dummy;
                    MPI_Send(&dummy,1, MPI_INT, status.MPI_SOURCE,
                            STOP, MPI_COMM_WORLD);
                }
        
        }
        fprintf(stderr,"sequential time: %f secs\n", MPI_Wtime() - t_start);

    }
    else
    {
        int size_str_to_check ,enumGet;
        char str_to_check[MAX_STRING_SIZE];
        //MPI_Recv(&enumGet , 1 , MPI_INT , ROOT ,MPI_ANY_TAG, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        MPI_Bcast(&enumGet , 1 , MPI_INT , ROOT , MPI_COMM_WORLD);
        how_to_caculate = (enum matrix_score) enumGet;
        if (how_to_caculate==THERE_IS_MATRIX_SCORE)
           MPI_Bcast(matrix  , MATRIX_SIZE*MATRIX_SIZE , MPI_INT , ROOT , MPI_COMM_WORLD);
        MPI_Bcast(&lenght_first_str , 1 , MPI_INT , ROOT , MPI_COMM_WORLD);
        //MPI_Recv(&lenght_first_str , 1 , MPI_INT, ROOT ,MPI_ANY_TAG, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        first_str = (char*)malloc(lenght_first_str*sizeof(char));
        MPI_Bcast(first_str , lenght_first_str*sizeof(char) , MPI_CHAR , ROOT , MPI_COMM_WORLD);
        //MPI_Recv(first_str , lenght_first_str*sizeof(char) , MPI_CHAR  , ROOT , MPI_ANY_TAG , MPI_COMM_WORLD , MPI_STATUS_IGNORE);
        MPI_Status status;
        int tag,sqn_taries;
        do
        {

            MPI_Recv(&size_str_to_check , 1 , MPI_INT , 
            ROOT,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
            tag = status.MPI_TAG;
            struct score_alignment temp_Max , AS_max;
            if (tag==WORK)
            {
                MPI_Recv(str_to_check, (size_str_to_check+1) * sizeof(char), 
                MPI_CHAR , ROOT,MPI_ANY_TAG,MPI_COMM_WORLD,MPI_STATUS_IGNORE);

                // #ifdef DEBUG
                //     printf("rank = %d tag = %d\n",my_rank,status.MPI_TAG);
                //     printf("got str:%s \n",str_to_check);

                // #endif

                #pragma omp declare reduction(AS_max_func : struct score_alignment : \
                        omp_out = (omp_out.score > omp_in.score ? omp_out : omp_in)) \
                        initializer(omp_priv = omp_orig)
                
                AS_max.sqn = 0;
                AS_max.MS = 0;
                AS_max.score = -1;
                temp_Max.sqn =0;
                temp_Max.MS =0;
                temp_Max.score =0;
                sqn_taries = (size_str_to_check<lenght_first_str)? (lenght_first_str-size_str_to_check)
                : (size_str_to_check-lenght_first_str);
                char str_for_sqn [size_str_to_check];
                // char* str_for_sqn;
                #pragma omp parallel firstprivate(temp_Max)
                for (int off_set = 0; off_set <= sqn_taries; off_set++)
                {
                    temp_Max.sqn = off_set;

                    for (int j = 0; j <=size_str_to_check; j++)
                    {
                        str_for_sqn[j] = *(first_str+j+off_set);
                        if (j==size_str_to_check)
                            str_for_sqn[j] ='\0';
                    }
                    // #ifdef DEBUG
                    //     printf(" %s before -  %s , sqn_number = %d \n" ,temp_first_str, str_to_check  , i);
                    // #endif
                    //str_for_sqn = first_str+off_set;
                    #pragma omp for reduction(AS_max_func :  AS_max)
                    for (int k = 0; k < size_str_to_check; k++)
                    {
                        temp_Max.MS = k;

                        strncpy(temp_Max.str , str_to_check ,MAX_STRING_SIZE-1);
                        temp_Max.str[MAX_STRING_SIZE] = '\0';   
                        Mutanat_Squence(temp_Max.str , k,size_str_to_check);
                        // #ifdef DEBUG
                        // printf("old str  - %s  str %s , <MS> = %d \n", str_to_check, temp_Max.str  , d);
                        // #endif
                        //caculate result
                        if (how_to_caculate==NO_MATRIX_SCORE)
                            temp_Max.score = computeOnGPU(str_for_sqn , temp_Max.str);
                        else
                            temp_Max.score = computeOnGPUWithMatrix(str_for_sqn , temp_Max.str , matrix);
                        // #ifdef DEBUG
                        // printf("temp.result = %d\n",temp_Max.score);
                        // #endif
                        if (AS_max.score <temp_Max.score)
                        {
                            AS_max.MS = temp_Max.MS;
                            AS_max.sqn = temp_Max.sqn   ;
                            AS_max.score = temp_Max.score;
                        }

                    }    
                    str_for_sqn[0] = '\0';
                }
                
            
            strncpy(AS_max.str , str_to_check , MAX_STRING_SIZE-1);
            AS_max.str [MAX_STRING_SIZE] = '\0';
            MPI_Send(&AS_max , 1  , mpi_score_alignment_type , ROOT , DONE , MPI_COMM_WORLD);
            }

        } while (tag != STOP);
       
    }
    MPI_Barrier(MPI_COMM_WORLD);
    free(first_str);
    MPI_Type_free(&mpi_score_alignment_type);
    MPI_Finalize();
    return EXIT_SUCCESS;

}


void init(int argc, char **argv )
{
    first_str = createDynStr();
    if (first_str==NULL)
    {
      fprintf(stderr,"line 1 should be the first string\n");
        exit(1);
    }
    lenght_first_str = strlen(first_str);
    if (!(scanf("%d",&number_strings)))
    {
            fprintf(stderr,"line 2 should be the numebr of strings to check\n");
            exit(1);
    }
    how_to_caculate = NO_MATRIX_SCORE;
    if (argc==2)
    {
        how_to_caculate = THERE_IS_MATRIX_SCORE;
        if (readMatrixFromFile(argv[1], matrix) == 0) 
            printf("Matrix read successfully!\n");
        else 
            printf("There was an error reading the matrix.\n");
    

    }
    
}


int readMatrixFromFile(const char* filename, int matrix[MATRIX_SIZE][MATRIX_SIZE]) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return -1; // Indicate failure
    }

    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            if (fscanf(file, "%d", &matrix[i][j]) != 1) {  // assuming integers in the matrix
                perror("Error reading matrix values");
                fclose(file);
                return -2;  // Indicate reading error
            }
        }
    }

    fclose(file);
    return 0;  // Success
}
