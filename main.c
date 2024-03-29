#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#include "mmio_highlevel.h"

#define VALUE_Y 1

void printmat(float *A, int m, int n)
{
    for (int i = 0; i < m; i++)
    {
        for (int j = 0; j < n; j++)
            printf("%4.2f ", A[i * n + j]);
        printf("\n");
    }
}

void printvec(float *x, int n)
{
    for (int i = 0; i < n; i++)
        printf("%4.2f\n", x[i]);
}

void matvec(float *A, float *x, float *y, int m, int n)
{
    for (int i = 0; i < m; i++)
    {
        y[i] = 0;
        for (int j = 0; j < n; j++)
            y[i] += A[i * n + j] * x[j];
    }
}

void matmat(float *C, float *A, float *B, int m, int k, int n)
{
    memset(C, 0, sizeof(float) * m * n);
    for (int i = 0; i < m; i++)
        for (int j = 0; j < n; j++)
            for (int kk = 0; kk < k; kk++)
                C[i * n + j] += A[i * k + kk] * B[kk * n + j];
}

void matmat_transB(float *C, float *A, float *BT, int m, int k, int n)
{
    memset(C, 0, sizeof(float) * m * n);
    for (int i = 0; i < m; i++)
        for (int j = 0; j < n; j++)
            for (int kk = 0; kk < k; kk++)
                C[i * n + j] += A[i * k + kk] * BT[j * k + kk];
}

float dotproduct(float *vec1, float *vec2, int n)
{
    float result = 0;
    for (int i = 0; i < n; i++)
        result += vec1[i] * vec2[i];
    return result;
}

// A is m x n, AT is n x m
void transpose(float *AT, float *A, int m, int n)
{
    for (int i = 0; i < m; i++)
        for (int j = 0; j < n; j++)
            AT[j * m + i] = A[i * n + j];
}

float vec2norm(float *x, int n)
{
    float sum = 0;
    for (int i = 0; i < n; i++)
        sum += x[i] * x[i];
    return sqrt(sum);
}

void cg(float *A, float *x, float *b, int n, int *iter, int maxiter, float threshold)
{
    memset(x, 0, sizeof(float) * n);
    float *residual = (float *)malloc(sizeof(float) * n);
    float *y = (float *)malloc(sizeof(float) * n);
    float *p = (float *)malloc(sizeof(float) * n);
    float *q = (float *)malloc(sizeof(float) * n);
    *iter = 0;
    float norm = 0;
    float rho = 0;
    float rho_1 = 0;

    // p0 = r0 = b - Ax0
    matvec(A, x, y, n, n);
    for (int i = 0; i < n; i++)
        residual[i] = b[i] - y[i];
    //printvec(residual, n);

    do
    {
        //printf("\ncg iter = %i\n", *iter);
        rho = dotproduct(residual, residual, n);
        if (*iter == 0)
        {
            for (int i = 0; i < n; i++)
                p[i] = residual[i];
        }
        else
        {
            float beta = rho / rho_1;
            for (int i = 0; i < n; i++)
                p[i] = residual[i] + beta * p[i];
        }

        matvec(A, p, q, n, n);
        float alpha = rho / dotproduct(p, q, n);
        //printf("alpha = %f\n", alpha);
        for (int i = 0; i < n; i++)
            x[i] += alpha * p[i];
        for (int i = 0; i < n; i++)
            residual[i] += - alpha * q[i];

        rho_1 = rho;
        float error = vec2norm(residual, n) / vec2norm(b, n);

        //printvec(x, n);
        *iter += 1;

        if (error < threshold)
            break;
    }
    while (*iter < maxiter);

    free(residual);
    free(y);
    free(p);
    free(q);
}

void updateX(float *R, float *X, float *Y,
             int m, int n, int f, float lamda)
{
    // create YT, and transpose Y
    float *YT = (float *)malloc(sizeof(float) * n * f);
    transpose(YT, Y, n, f);

    // create smat = YT*Y
    float *smat = (float *)malloc(sizeof(float) * f * f);

    // multiply YT and Y to smat
    matmat(smat, YT, Y, f, n, f);

    // smat plus lamda*I
    for (int i = 0; i < f; i++)
        smat[i * f + i] += lamda;

    // create svec
    float *svec = (float *)malloc(sizeof(float) * f);

    // loop for all rows of X, from 0 to m-1
    for (int u = 0; u < m; u++)
    {
        printf("\n u = %i", u);
        // reference the uth row of X
        float *xu = &X[u * f];

        // compute svec by multiplying YT and the uth row of R
        matvec(YT, &R[u * n], svec, f, n);

        // solve the Ax=b system (A is smat, b is svec, x is xu (uth row of X))
        int cgiter = 0;
        cg(smat, xu, svec, f, &cgiter, 100, 0.00001);

        //printf("\nsmat = \n");
        //printmat(smat, f, f);

        //printf("\nsvec = \n");
        //printvec(svec, f);

        //printf("\nxu = \n");
        //printvec(xu, f);
    }

    free(smat);
    free(svec);
    free(YT);
}

void updateY(float *R, float *X, float *Y,
             int m, int n, int f, float lamda)
{
    float *XT = (float *)malloc(sizeof(float) * m * f);
    transpose(XT, X, m, f);

    float *smat = (float *)malloc(sizeof(float) * f * f);
    matmat(smat, XT, X, f, m, f);
    for (int i = 0; i < f; i++)
        smat[i * f + i] += lamda;

    float *svec = (float *)malloc(sizeof(float) * f);
    float *ri = (float *)malloc(sizeof(float) * m);

    for (int i = 0; i < n; i++)
    {
        printf("\n i = %i", i);
        float *yi = &Y[i * f];

        for (int k = 0; k < m; k++)
            ri[k] = R[k * n + i];

        matvec(XT, ri, svec, f, m);

        int cgiter = 0;
        cg(smat, yi, svec, f, &cgiter, 100, 0.00001);

        printf("\nsmat = \n");
        printmat(smat, f, f);

        printf("\nsvec = \n");
        printvec(svec, f);

        printf("\nyi = \n");
        printvec(yi, f);
    }

    free(smat);
    free(svec);
    free(ri);
    free(XT);
}

// ALS for matrix factorization
void als(float *R, float *X, float *Y,
         int m, int n, int f, float lamda)
{
    // create YT
    float *YT = (float *)malloc(sizeof(float) * f * n);

    // create R (result)
    float *Rp = (float *)malloc(sizeof(float) * m * n);

    int iter = 0;
    float error = 0.0;
    float error_old = 0.0;
    float error_new = 0.0;
    do
    {
        // step 1. update X
        updateX(R, X, Y, m, n, f, lamda);

        // step 2. update Y
        updateY(R, X, Y, m, n, f, lamda);

        // step 3. validate R, by multiplying X and YT

        // step 3-1. matrix multiplication with transposed Y
        matmat_transB(Rp, X, Y, m, f, n);

        // step 3-2. calculate error
        error_new = 0.0;
        for (int i = 0; i < m * n; i++)
            if (R[i] != 0) // only compare nonzero entries in R
                error_new += fabs(Rp[i] - R[i]) * fabs(Rp[i] - R[i]);
        error_new = sqrt(error_new/(m * n));

        error = fabs(error_new - error_old) / error_new;
        error_old = error_new;
        printf("iter = %i, error = %f\n", iter, error);

        iter++;
    }
    while(iter < 10 && error > 0.0001);

    printf("\nR = \n");
    printmat(R, m, n);

    printf("\nRp = \n");
    printmat(Rp, m, n);

    free(Rp);
    free(YT);
}

void updateX_recsys(float *R, float *X, float *Y,
                    int m, int n, int f, float lamda,
                    double *time_prepareA, double *time_prepareb, double *time_solver)
{
    struct timeval t1, t2;

    // malloc smat (A) and svec (b)
    float *smat = (float *)malloc(sizeof(float) * f * f);
    float *svec = (float *)malloc(sizeof(float) * f);

    for (int u = 0; u < m; u++)
    {
        gettimeofday(&t1, NULL);
        //printf("\n u = %i", u);
        float *xu = &X[u * f];

        // find nzr (i.e., #nonzeros in the uth row of R)
        int nzr = 0;
        for (int k = 0; k < n; k++)
            nzr = R[u * n + k] == 0 ? nzr : nzr + 1;

        // malloc ru (i.e., uth row of R) and insert entries into it
        float *ru = (float *)malloc(sizeof(float) * nzr);
        int count = 0;
        for (int k = 0; k < n; k++)
        {
            if (R[u * n + k] != 0)
            {
                ru[count] = R[u * n + k];
                count++;
            }
        }
        //printf("\n nzr = %i, ru = \n", nzr);
        //printvec(ru, nzr);

        // create sY and sYT (i.e., the zero-free version of Y and YT)
        float *sY = (float *)malloc(sizeof(float) * nzr * f);
        float *sYT = (float *)malloc(sizeof(float) * nzr * f);

        // fill sY, according to the sparsity of the uth row of R
        count = 0;
        for (int k = 0; k < n; k++)
        {
            if (R[u * n + k] != 0)
            {
                memcpy(&sY[count * f], &Y[k * f], sizeof(float) * f);
                count++;
            }
        }
        //printf("\n sY = \n");
        //printmat(sY, nzr, f);

        // transpose sY to sYT
        transpose(sYT, sY, nzr, f);

        // multiply sYT and sY, and plus lamda * I
        matmat(smat, sYT, sY, f, nzr, f);
        for (int i = 0; i < f; i++)
            smat[i * f + i] += lamda;

        gettimeofday(&t2, NULL);
        *time_prepareA += (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;

        // compute b (i.e., svec) by multiplying sYT and the uth row of R
        gettimeofday(&t1, NULL);
        matvec(sYT, ru, svec, f, nzr);
        gettimeofday(&t2, NULL);
        *time_prepareb += (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;

        // solve the system of Ax=b, and get x = the uth row of X
        gettimeofday(&t1, NULL);
        int cgiter = 0;
        cg(smat, xu, svec, f, &cgiter, 100, 0.00001);
        gettimeofday(&t2, NULL);
        *time_solver += (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;

        //printf("\nsmat = \n");
        //printmat(smat, f, f);

        //printf("\nsvec = \n");
        //printvec(svec, f);

        //printf("\nxu = \n");
        //printvec(xu, f);

        free(ru);
        free(sY);
        free(sYT);
    }

    free(smat);
    free(svec);
    //free(YT);
}

void updateY_recsys(float *R, float *X, float *Y,
                    int m, int n, int f, float lamda,
                    double *time_prepareA, double *time_prepareb, double *time_solver)
{
    struct timeval t1, t2;

    float *smat = (float *)malloc(sizeof(float) * f * f);
    float *svec = (float *)malloc(sizeof(float) * f);

    for (int i = 0; i < n; i++)
    {
        gettimeofday(&t1, NULL);
        //printf("\n i = %i", i);
        float *yi = &Y[i * f];

        int nzc = 0;
        for (int k = 0; k < m; k++)
            nzc = R[k * n + i] == 0 ? nzc : nzc + 1;

        float *ri = (float *)malloc(sizeof(float) * nzc);
        int count = 0;
        for (int k = 0; k < m; k++)
        {
            if (R[k * n + i] != 0)
            {
                ri[count] = R[k * n + i];
                count++;
            }
        }
        //printf("\n nzc = %i, ri = \n", nzc);
        //printvec(ri, nzc);

        float *sX = (float *)malloc(sizeof(float) * nzc * f);
        float *sXT = (float *)malloc(sizeof(float) * nzc * f);
        count = 0;
        for (int k = 0; k < m; k++)
        {
            if (R[k * n + i] != 0)
            {
                memcpy(&sX[count * f], &X[k * f], sizeof(float) * f);
                count++;
            }
        }
        //printf("\n sX = \n");
        //printmat(sX, nzc, f);

        transpose(sXT, sX, nzc, f);
        matmat(smat, sXT, sX, f, nzc, f);
        for (int i = 0; i < f; i++)
            smat[i * f + i] += lamda;

        gettimeofday(&t2, NULL);
        *time_prepareA += (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;

        gettimeofday(&t1, NULL);
        matvec(sXT, ri, svec, f, nzc);
        gettimeofday(&t2, NULL);
        *time_prepareb += (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;

        gettimeofday(&t1, NULL);
        int cgiter = 0;
        cg(smat, yi, svec, f, &cgiter, 100, 0.00001);
        gettimeofday(&t2, NULL);
        *time_solver += (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;

        //printf("\nsmat = \n");
        //printmat(smat, f, f);

        //printf("\nsvec = \n");
        //printvec(svec, f);

        //printf("\nyi = \n");
        //printvec(yi, f);

        free(ri);
        free(sX);
        free(sXT);
    }

    free(smat);
    free(svec);
}


void als_recsys(float *R, float *X, float *Y,
         int m, int n, int f, float lamda)
{
    // create YT and Rp
    float *YT = (float *)malloc(sizeof(float) * f * n);
    float *Rp = (float *)malloc(sizeof(float) * m * n);

    int iter = 0;
    float error = 0.0;
    float error_old = 0.0;
    float error_new = 0.0;
    struct timeval t1, t2;

    double time_updatex_prepareA = 0;
    double time_updatex_prepareb = 0;
    double time_updatex_solver = 0;

    double time_updatey_prepareA = 0;
    double time_updatey_prepareb = 0;
    double time_updatey_solver = 0;

    double time_updatex = 0;
    double time_updatey = 0;
    double time_validate = 0;

    do
    {
        // step 1. update X
        gettimeofday(&t1, NULL);
        updateX_recsys(R, X, Y, m, n, f, lamda,
                       &time_updatex_prepareA, &time_updatex_prepareb, &time_updatex_solver);
        gettimeofday(&t2, NULL);
        time_updatex += (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;

        // step 2. update Y
        gettimeofday(&t1, NULL);
        updateY_recsys(R, X, Y, m, n, f, lamda,
                       &time_updatey_prepareA, &time_updatey_prepareb, &time_updatey_solver);
        gettimeofday(&t2, NULL);
        time_updatey += (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;

        // step 3. validate
        // step 3-1. matrix multiplication
        gettimeofday(&t1, NULL);
        matmat_transB(Rp, X, Y, m, f, n);

        // step 3-2. calculate error
        error_new = 0.0;
        int nnz = 0;
        for (int i = 0; i < m * n; i++)
        {
            if (R[i] != 0)
            {
                error_new += fabs(Rp[i] - R[i]) * fabs(Rp[i] - R[i]);
                nnz++;
            }
        }
        error_new = sqrt(error_new/nnz);
        gettimeofday(&t2, NULL);
        time_validate += (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;

        error = fabs(error_new - error_old) / error_new;
        error_old = error_new;
        printf("iter = %i, error = %f\n", iter, error);

        iter++;
    }
    while(iter < 1000 && error > 0.0001);

    //printf("\nR = \n");
    //printmat(R, m, n);

    //printf("\nRp = \n");
    //printmat(Rp, m, n);

    printf("\nUpdate X %4.2f ms (prepare A %4.2f ms, prepare b %4.2f ms, solver %4.2f ms)\n",
           time_updatex, time_updatex_prepareA, time_updatex_prepareb, time_updatex_solver);
    printf("Update Y %4.2f ms (prepare A %4.2f ms, prepare b %4.2f ms, solver %4.2f ms)\n",
           time_updatey, time_updatey_prepareA, time_updatey_prepareb, time_updatey_solver);
    printf("Validate %4.2f ms\n", time_validate);

    free(Rp);
    free(YT);
}

#include "recsyscsr.h"

int main(int argc, char ** argv)
{
    // parameters
    int f = 0;

    // method
    char *method = argv[1];
    printf("\n");

    char *filename = argv[2];
    printf ("filename = %s\n", filename);

    int m, n, nnzR, isSymmetricR;

    struct timeval t1, t2;
    gettimeofday(&t1, NULL);

    mmio_info(&m, &n, &nnzR, &isSymmetricR, filename);
    // m = 480189; n = 17770; nnzR = 99072112;
    // 206 5575
    int *csrRowPtrR = (int *)malloc((m+1) * sizeof(int));
    int *csrColIdxR = (int *)malloc(nnzR * sizeof(int));
    float *csrValR    = (float *)malloc(nnzR * sizeof(float));
    mmio_data(csrRowPtrR, csrColIdxR, csrValR, filename);
    gettimeofday(&t2, NULL);
    double time_input = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
    printf("input time is %f\n", time_input);

    //FILE *file;
    //file = fopen(filename, "r+");
    //fscanf(file, "%i", &m);
    //fscanf(file, "%i", &n);
    printf("The order of the rating matrix R is %i by %i, #nonzeros = %i\n",
           m, n, nnzR);

    // create R
    // float *R = (float *)malloc(sizeof(float) * m * n);
    // memset(R, 0, sizeof(float) * m * n);

    // put nonzeros into the dense form of R (the array R)
    // for (int rowidx = 0; rowidx < m; rowidx++)
    // {
    //     for (int j = csrRowPtrR[rowidx]; j < csrRowPtrR[rowidx + 1]; j++)
    //     {
    //         int colidx = csrColIdxR[j];
    //         float val = csrValR[j];
    //         // R[rowidx * n + colidx] = val;
    //     }
    // }

    // read R
    //for (int i = 0; i < m; i++)
    //    for (int j = 0; j < n; j++)
    //        fscanf(file, "%f", &R[i * n + j]);

    //printf("\nR = \n");
    //printmat(R, m, n);

    f = atoi(argv[3]);
    printf("The latent feature is %i \n", f);

    // create X
    float *X = (float *)malloc(sizeof(float) * m * f);
    memset(X, 0, sizeof(float) * m * f);

    // create Y
    float *Y = (float *)malloc(sizeof(float) * n * f);
    for (int i = 0; i < n * f; i++)
        Y[i] = VALUE_Y;

    // lamda parameter
    float lamda = 0.1;

    // call function
    if (strcmp(method, "als") == 0)
    {
        // als(R, X, Y, m, n, f, lamda);
    }
    else if (strcmp(method, "als_recsys") == 0)
    {
        // als_recsys(R, X, Y, m, n, f, lamda);
    }
    else if (strcmp(method, "als_recsys_csr") == 0)
    {
        als_recsys_csr( csrRowPtrR, csrColIdxR, csrValR, X, Y,
                        m, n, f, lamda);
    }

    // free(R);
    free(X);
    free(Y);
}

