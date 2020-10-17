/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
int sameRow(int j, int k, int rb) {
    return j % rb * rb + k / 8 == k % rb * rb + j / 8; 
}


void transpose_32(int M, int N, int A[N][M], int B[M][N])
{
    int a0, a1, a2, a3, a4, a5, a6, a7;
    for (int jj = 0; jj < M; jj += 8 ) { 
        for (int kk = 0; kk < M; kk += 8) {
            for (int j = jj, k = kk; j < jj + 8; ++j, ++k) {
                    a0 = A[j][kk];
                    a1 = A[j][kk + 1];
                    a2 = A[j][kk + 2];
                    a3 = A[j][kk + 3];
                    a4 = A[j][kk + 4];
                    a5 = A[j][kk + 5];
                    a6 = A[j][kk + 6];
                    a7 = A[j][kk + 7];
                    B[k][jj] = a0;
                    B[k][jj + 1] = a1;
                    B[k][jj + 2] = a2;
                    B[k][jj + 3] = a3;
                    B[k][jj + 4] = a4;
                    B[k][jj + 5] = a5;
                    B[k][jj + 6] = a6;
                    B[k][jj + 7] = a7;
            }
            for (int j = 0; j < 8; ++j) {
                for (int k = j + 1; k < 8; ++k) {
                    int temp = B[kk + j][jj + k];
                    B[kk + j][jj + k] = B[kk + k][jj + j];
                    B[kk + k][jj + j] = temp;
                }
            }
        }
    }
}


void transpose_64(int M, int N, int A[N][M], int B[M][N])
{
    int a0, a1, a2, a3, a4, a5, a6, a7, temp;
    for (int jj = 0; jj < M; jj += 8 ) { 
        for (int kk = 0; kk < N; kk += 8) {
            for (int k = 0; k < 4; ++k) {
                    a0 = A[jj + k][kk];
                    a1 = A[jj + k][kk + 1];
                    a2 = A[jj + k][kk + 2];
                    a3 = A[jj + k][kk + 3];
                    a4 = A[jj + k][kk + 4];
                    a5 = A[jj + k][kk + 5];
                    a6 = A[jj + k][kk + 6];
                    a7 = A[jj + k][kk + 7];
                    B[kk][jj + k] = a0;
                    B[kk + 1][jj + k] = a1;
                    B[kk + 2][jj + k] = a2;
                    B[kk + 3][jj + k] = a3;
                    B[kk][jj + k + 4] = a4;
                    B[kk + 1][jj + k + 4] = a5;
                    B[kk + 2][jj + k + 4] = a6;
                    B[kk + 3][jj + k + 4] = a7;
            }
            for (int k = 0; k < 4; ++k) {
                a0 = A[jj + 4][kk + k], a1 = A[jj + 5][kk + k];
                a2 = A[jj + 6][kk + k], a3 = A[jj + 7][kk + k];
                a4 = A[jj + 4][kk + k + 4], a5 = A[jj + 5][kk + k + 4];
                a6 = A[jj + 6][kk + k + 4], a7 = A[jj + 7][kk + k + 4];

                temp = B[kk + k][jj + 4], B[kk + k][jj + 4] = a0, a0 = temp; 
                temp = B[kk + k][jj + 5], B[kk + k][jj + 5] = a1, a1 = temp;
                temp = B[kk + k][jj + 6], B[kk + k][jj + 6] = a2, a2 = temp;
                temp = B[kk + k][jj + 7], B[kk + k][jj + 7] = a3, a3 = temp;

                B[kk + 4 + k][jj] = a0, B[kk + 4 + k][jj + 1] = a1;
                B[kk + 4 + k][jj + 2] = a2, B[kk + 4 + k][jj + 3] = a3;
                B[kk + 4 + k][jj + 4] = a4, B[kk + 4 + k][jj + 5] = a5;
                B[kk + 4 + k][jj + 6] = a6, B[kk + 4 + k][jj + 7] = a7;
            }
        }
    }
}

int min(int a, int b) {return a > b ? b : a;}

void transpose_61(int M, int N, int A[N][M], int B[M][N])
{
    int a0, a1, a2, a3, a4, a5, a6, a7, b = 16, c = 16;
    for (int jj = 0; jj < N; jj += b) { 
        for (int kk = 0; kk < M; kk += c) {
            if (jj + b <= N && kk + c <= M) {
             for (int k = jj; k < jj + b; ++k) {
                a0 = A[k][kk];
                a1 = A[k][kk + 1];
                a2 = A[k][kk + 2];
                a3 = A[k][kk + 3];
                a4 = A[k][kk + 4];
                a5 = A[k][kk + 5];
                a6 = A[k][kk + 6];
                a7 = A[k][kk + 7];
                B[kk][k] = a0;
                B[kk + 1][k] = a1;
                B[kk + 2][k] = a2;
                B[kk + 3][k] = a3;
                B[kk + 4][k] = a4;
                B[kk + 5][k] = a5;
                B[kk + 6][k] = a6;
                
                B[kk + 7][k] = a7;
                a0 = A[k][kk + 8];
                a1 = A[k][kk + 9];
                a2 = A[k][kk + 10];
                a3 = A[k][kk + 11];
                a4 = A[k][kk + 12];
                a5 = A[k][kk + 13];
                a6 = A[k][kk + 14];
                a7 = A[k][kk + 15];
                B[kk + 8][k] = a0;
                B[kk + 9][k] = a1;
                B[kk + 10][k] = a2;
                B[kk + 11][k] = a3;
                B[kk + 12][k] = a4;
                B[kk + 13][k] = a5;
                B[kk + 14][k] = a6;
                B[kk + 15][k] = a7;
             

}
           } else {
                for (int i = jj; i < min(jj + b,N); ++i) {
                    for (int j = kk; j < min(kk + c,M); ++j) B[j][i] = A[i][j];
                }
           }
         }
    }         
}

char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (M == 32) transpose_32(M, N, A, B);
    else if (M == 64) transpose_64(M, N, A, B);
    else transpose_61(M, N, A, B);

}


/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int c = 256 / M;
    for (int jj = 0; jj < M; jj += c ) { 
        for (int kk = 0; kk < M; kk += c) {
            for (int j = jj; j < jj + c; j++){ 
                int temp = 0, x = -1, y = -1;
                for (int k = kk; k < kk + c; k++){
                    if (j % c * c + k / 8 == k % c * c + j / 8) {
                        x = j;
                        y = k;
                        temp = A[j][k];
                        continue;
                    }
                    B[k][j] = A[j][k];
                }
                if (x != -1) B[y][x] = temp;
            }
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    //registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

