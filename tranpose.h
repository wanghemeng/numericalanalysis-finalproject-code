#ifndef _TRANS_
#define _TRANS_

//#include "common.h"
#include "other.h"
// m:rows n:cols
void matrix_transposition( int         m,
                           int         n,
                           int         nnz,
                           int        *csrRowPtr,
                           int        *csrColIdx,
                           float      *csrVal,
                           int        *cscRowIdx,
                           int        *cscColPtr,
                           float      *cscVal)
{
    // histogram in column pointer
    memset (cscColPtr, 0, sizeof(int) * (n+1));
    for (int i = 0; i < nnz; i++)
    {
        cscColPtr[csrColIdx[i]]++;
    }

    // prefix-sum scan to get the column pointer
    exclusive_scan(cscColPtr, n + 1);

    int *cscColIncr = (int *)malloc(sizeof(int) * (n+1));
    memcpy (cscColIncr, cscColPtr, sizeof(int) * (n+1));

    // insert nnz to csc
    for (int row = 0; row < m; row++)
    {
        for (int j = csrRowPtr[row]; j < csrRowPtr[row+1]; j++)
        {
            int col = csrColIdx[j];

            cscRowIdx[cscColIncr[col]] = row;
            cscVal[cscColIncr[col]] = csrVal[j];
            cscColIncr[col]++;
        }
    }

    free (cscColIncr);
}

#endif
