#ifndef EXE_H_
#define EXE_H_
#define MATRIX_SIZE 26
char* createDynStr();
void Mutanat_Squence(char* str , int k , int size_str);
int Sqn_Main_str(char** new_str, const char* first, int sqn, int length);
int readMatrixFromFile(const char* filename, int matrix[MATRIX_SIZE][MATRIX_SIZE]);

#endif