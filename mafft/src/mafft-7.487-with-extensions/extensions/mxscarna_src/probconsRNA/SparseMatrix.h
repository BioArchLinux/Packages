/////////////////////////////////////////////////////////////////
// SparseMatrix.h
//
// Sparse matrix computations
/////////////////////////////////////////////////////////////////

#ifndef SPARSEMATRIX_H
#define SPARSEMATRIX_H

#include <iostream>
#include "SafeVector.h"
#include "nrutil.h"

using namespace std;

const float POSTERIOR_CUTOFF = 0.01;         // minimum posterior probability
                                             // value that is maintained in the
                                             // sparse matrix representation

typedef pair<int,float> PIF;                 // Sparse matrix entry type
                                             //   first --> column
                                             //   second --> value

namespace MXSCARNA {
struct PIF2 {        // Sparse matrix entry type
    int i;
    int j;
    float prob;
};
}

/////////////////////////////////////////////////////////////////
// SparseMatrix
//
// Class for sparse matrix computations
/////////////////////////////////////////////////////////////////
namespace MXSCARNA {
class SparseMatrix {

  int seq1Length, seq2Length;                     // dimensions of matrix
  VI rowSize;                                     // rowSize[i] = # of cells in row i
  SafeVector<PIF> data;                           // data values
  SafeVector<SafeVector<PIF>::iterator> rowPtrs;  // pointers to the beginning of each row

 public:
  SafeVector<PIF2> data2;
  /////////////////////////////////////////////////////////////////
  // SparseMatrix::SparseMatrix()
  //
  // Private constructor.1
  /////////////////////////////////////////////////////////////////
  SparseMatrix() { }

  /////////////////////////////////////////////////////////////////
  // SparseMatrix::SparseMatrix()
  //
  // Constructor.  Builds a sparse matrix from a posterior matrix.
  // Note that the expected format for the posterior matrix is as
  // a (seq1Length+1) x (seq2Length+1) matrix where the 0th row
  // and 0th column are ignored (they should contain all zeroes).
  /////////////////////////////////////////////////////////////////

  SparseMatrix (int seq1Length, int seq2Length, const VF &posterior) :
    seq1Length (seq1Length), seq2Length (seq2Length) {

    int numCells = 0;

    assert (seq1Length > 0);
    assert (seq2Length > 0);

    // calculate memory required; count the number of cells in the
    // posterior matrix above the threshold
    VF::const_iterator postPtr = posterior.begin();
    for (int i = 0; i <= seq1Length; i++){
      for (int j = 0; j <= seq2Length; j++){
        if (*(postPtr++) >= POSTERIOR_CUTOFF){
          assert (i != 0 && j != 0);
          numCells++;
        }
      }
    }
    
    // allocate memory
    data.resize(numCells);
    rowSize.resize (seq1Length + 1); rowSize[0] = -1;
    rowPtrs.resize (seq1Length + 1); rowPtrs[0] = data.end();

    // build sparse matrix
    postPtr = posterior.begin() + seq2Length + 1;           // note that we're skipping the first row here
    SafeVector<PIF>::iterator dataPtr = data.begin();
    for (int i = 1; i <= seq1Length; i++){
      postPtr++;                                            // and skipping the first column of each row
      rowPtrs[i] = dataPtr;
      for (int j = 1; j <= seq2Length; j++){
        if (*postPtr >= POSTERIOR_CUTOFF){
          dataPtr->first = j;
          dataPtr->second = *postPtr;
          dataPtr++;
        }
        postPtr++;
      }
      rowSize[i] = dataPtr - rowPtrs[i];
    }
  }

  //////////////////////////////////////////////////////////////////////////
  // SparseMatrix::SetSparseMatrix()
  // 
  // Constructor. 
  //////////////////////////////////////////////////////////////////////////
  void SetSparseMatrix(int inseq1Length, int inseq2Length, const Trimat<float> &bppMat, float cutoff = 0.01) {
      seq1Length = inseq1Length;
      seq2Length = inseq2Length;

      int numCells = 0;

      assert (seq1Length > 0);
      assert (seq2Length > 0);

      data.clear();
      rowSize.clear();
      rowPtrs.clear();
      for (int i = 1; i <= seq1Length; i++) {
	  for (int j = i; j <= seq2Length; j++) {
	      if (bppMat.ref(i, j) >= cutoff ) {
		  numCells++;
	      }
	  }
      }

      // allocate memory
      data.resize(numCells);
      for (int i = 0; i < numCells; i++) {
	  data[i].first  = 0;
	  data[i].second = 0;
      }
      rowSize.resize (seq1Length + 1); rowSize[0] = -1;
      rowPtrs.resize (seq1Length + 1); rowPtrs[0] = data.end();

      SafeVector<PIF>::iterator dataPtr = data.begin();
      for (int i = 1; i <= seq1Length; i++) {
	  rowPtrs[i] = dataPtr;
	  for (int j = i; j <= seq2Length; j++) {
	      if ( bppMat.ref(i, j) >= cutoff ) {
		  dataPtr->first = j;
		  dataPtr->second = bppMat.ref(i, j);
		  dataPtr++;
	      }
	  }
	  rowSize[i] = dataPtr - rowPtrs[i];
      }

      float tmp;
      for(int k = 1; k <= seq1Length; k++) {
	  for(int m = k, n = k; n <= k + 300 && m >= 1 && n <= seq2Length; m--, n++) {
	      if ((tmp = GetValue(m, n)) > 0) {
		  PIF2 p;
		  p.i    = m;
		  p.j    = n;
		  p.prob = tmp;
		  data2.push_back(p);
	      }
	  }
    
	  for(int m = k, n = k + 1; n <= k + 300 && m >= 1 && n <= seq2Length; m--, n++) {
	      if ((tmp = GetValue(m, n)) > 0) {
		  PIF2 p;
		  p.i    = m;
		  p.j    = n;
		  p.prob = tmp;
		  data2.push_back(p);
	      }
	  }
      }
  }

  /////////////////////////////////////////////////////////////////
  // SparseMatrix::GetRowPtr()
  //
  // Returns the pointer to a particular row in the sparse matrix.
  /////////////////////////////////////////////////////////////////

  SafeVector<PIF>::iterator GetRowPtr (int row) const {
    assert (row >= 1 && row <= seq1Length);
    return rowPtrs[row];
  }

  /////////////////////////////////////////////////////////////////
  // SparseMatrix::GetValue()
  //
  // Returns value at a particular row, column.
  /////////////////////////////////////////////////////////////////

  float GetValue (int row, int col){
    assert (row >= 1 && row <= seq1Length);
    assert (col >= 1 && col <= seq2Length);
    for (int i = 0; i < rowSize[row]; i++){
      if (rowPtrs[row][i].first == col) return rowPtrs[row][i].second;
    }
    return 0;
  }

  void SetValue(int row, int col, float value) {
    assert (row >= 1 && row <= seq1Length);
    assert (col >= 1 && col <= seq2Length);
    for (int i = 0; i < rowSize[row]; i++){
      if (rowPtrs[row][i].first == col) rowPtrs[row][i].second = value;
    }
  }

  /////////////////////////////////////////////////////////////////
  // SparseMatrix::GetRowSize()
  //
  // Returns the number of entries in a particular row.
  /////////////////////////////////////////////////////////////////

  int GetRowSize (int row) const {
    assert (row >= 1 && row <= seq1Length);
    return rowSize[row];
  }

  /////////////////////////////////////////////////////////////////
  // SparseMatrix::GetSeq1Length()
  //
  // Returns the first dimension of the matrix.
  /////////////////////////////////////////////////////////////////

  int GetSeq1Length () const {
    return seq1Length;
  }

  /////////////////////////////////////////////////////////////////
  // SparseMatrix::GetSeq2Length()
  //
  // Returns the second dimension of the matrix.
  /////////////////////////////////////////////////////////////////

  int GetSeq2Length () const {
    return seq2Length;
  }

  /////////////////////////////////////////////////////////////////
  // SparseMatrix::GetRowPtr
  //
  // Returns the pointer to a particular row in the sparse matrix.
  /////////////////////////////////////////////////////////////////

  int GetNumCells () const {
    return data.size();
  }

  /////////////////////////////////////////////////////////////////
  // SparseMatrix::Print()
  //
  // Prints out a sparse matrix.
  /////////////////////////////////////////////////////////////////

  void Print (ostream &outfile) const {
    outfile << "Sparse Matrix:" << endl;
    for (int i = 1; i <= seq1Length; i++){
      outfile << "  " << i << ":";
      for (int j = 0; j < rowSize[i]; j++){
        outfile << " (" << rowPtrs[i][j].first << "," << rowPtrs[i][j].second << ")";
      }
      outfile << endl;
    }
  }

  /////////////////////////////////////////////////////////////////
  // SparseMatrix::ComputeTranspose()
  //
  // Returns a new sparse matrix containing the transpose of the
  // current matrix.
  /////////////////////////////////////////////////////////////////

  SparseMatrix *ComputeTranspose () const {

    // create a new sparse matrix
    SparseMatrix *ret = new SparseMatrix();
    int numCells = data.size();

    ret->seq1Length = seq2Length;
    ret->seq2Length = seq1Length;

    // allocate memory
    ret->data.resize (numCells);
    ret->rowSize.resize (seq2Length + 1); ret->rowSize[0] = -1;
    ret->rowPtrs.resize (seq2Length + 1); ret->rowPtrs[0] = ret->data.end();

    // compute row sizes
    for (int i = 1; i <= seq2Length; i++) ret->rowSize[i] = 0;
    for (int i = 0; i < numCells; i++)
      ret->rowSize[data[i].first]++;

    // compute row ptrs
    for (int i = 1; i <= seq2Length; i++){
      ret->rowPtrs[i] = (i == 1) ? ret->data.begin() : ret->rowPtrs[i-1] + ret->rowSize[i-1];
    }

    // now fill in data
    SafeVector<SafeVector<PIF>::iterator> currPtrs = ret->rowPtrs;

    for (int i = 1; i <= seq1Length; i++){
      SafeVector<PIF>::iterator row = rowPtrs[i];
      for (int j = 0; j < rowSize[i]; j++){
        currPtrs[row[j].first]->first = i;
        currPtrs[row[j].first]->second = row[j].second;
        currPtrs[row[j].first]++;
      }
    }

    return ret;
  }

  /////////////////////////////////////////////////////////////////
  // SparseMatrix::GetPosterior()
  //
  // Return the posterior representation of the sparse matrix.
  /////////////////////////////////////////////////////////////////

  VF *GetPosterior () const {

    // create a new posterior matrix
    VF *posteriorPtr = new VF((seq1Length+1) * (seq2Length+1)); assert (posteriorPtr);
    VF &posterior = *posteriorPtr;

    // build the posterior matrix
    for (int i = 0; i < (seq1Length+1) * (seq2Length+1); i++) posterior[i] = 0;
    for (int i = 1; i <= seq1Length; i++){
      VF::iterator postPtr = posterior.begin() + i * (seq2Length+1);
      for (int j = 0; j < rowSize[i]; j++){
        postPtr[rowPtrs[i][j].first] = rowPtrs[i][j].second;
      }
    }

    return posteriorPtr;
  }

};
}
#endif
