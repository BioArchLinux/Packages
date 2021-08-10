/////////////////////////////////////////////////////////////////
// ProbabilisticModel.h
//
// Routines for (1) posterior probability computations
//              (2) chained anchoring
//              (3) maximum weight trace alignment
/////////////////////////////////////////////////////////////////

#ifndef PROBABILISTICMODEL_H
#define PROBABILISTICMODEL_H

#include <list>
#include <cmath>
#include <cstdio>
#include "SafeVector.h"
#include "ScoreType.h"
#include "SparseMatrix.h"
#include "MultiSequence.h"
#include "StemCandidate.hpp"
#include "scarna.hpp"
#include "nrutil.h"
#include <vector>

using namespace std;

const int NumMatchStates = 1;                                    // note that in this version the number
                                                                 // of match states is fixed at 1...will
                                                                 // change in future versions
const int NumMatrixTypes = NumMatchStates + NumInsertStates * 2;

/////////////////////////////////////////////////////////////////
// ProbabilisticModel
//
// Class for storing the parameters of a probabilistic model and
// performing different computations based on those parameters.
// In particular, this class handles the computation of
// posterior probabilities that may be used in alignment.
/////////////////////////////////////////////////////////////////
namespace MXSCARNA {
class ProbabilisticModel {

  float initialDistribution[NumMatrixTypes];               // holds the initial probabilities for each state
  float transProb[NumMatrixTypes][NumMatrixTypes];         // holds all state-to-state transition probabilities
  float matchProb[256][256];                               // emission probabilities for match states
  float insProb[256][NumMatrixTypes];                      // emission probabilities for insert states
  NRMat<float> WM;

 public:

  /////////////////////////////////////////////////////////////////
  // ProbabilisticModel::ProbabilisticModel()
  //
  // Constructor.  Builds a new probabilistic model using the
  // given parameters.
  /////////////////////////////////////////////////////////////////

  ProbabilisticModel (const VF &initDistribMat, const VF &gapOpen, const VF &gapExtend,
                      const VVF &emitPairs, const VF &emitSingle){

    // build transition matrix
    VVF transMat (NumMatrixTypes, VF (NumMatrixTypes, 0.0f));
    transMat[0][0] = 1;
    for (int i = 0; i < NumInsertStates; i++){
      transMat[0][2*i+1] = gapOpen[2*i];
      transMat[0][2*i+2] = gapOpen[2*i+1];
      transMat[0][0] -= (gapOpen[2*i] + gapOpen[2*i+1]);
      assert (transMat[0][0] > 0);
      transMat[2*i+1][2*i+1] = gapExtend[2*i];
      transMat[2*i+2][2*i+2] = gapExtend[2*i+1];
      transMat[2*i+1][2*i+2] = 0;
      transMat[2*i+2][2*i+1] = 0;
      transMat[2*i+1][0] = 1 - gapExtend[2*i];
      transMat[2*i+2][0] = 1 - gapExtend[2*i+1];
    }

    // create initial and transition probability matrices
    for (int i = 0; i < NumMatrixTypes; i++){
      initialDistribution[i] = LOG (initDistribMat[i]);
      for (int j = 0; j < NumMatrixTypes; j++)
        transProb[i][j] = LOG (transMat[i][j]);
    }

    // create insertion and match probability matrices
    for (int i = 0; i < 256; i++){
      for (int j = 0; j < NumMatrixTypes; j++)
        insProb[i][j] = LOG (emitSingle[i]);
      for (int j = 0; j < 256; j++)
        matchProb[i][j] = LOG (emitPairs[i][j]);
    }
  }

  NRMat<float> weightMatchScore(std::vector<StemCandidate> *pscs1, std::vector<StemCandidate> *pscs2,
			std::vector<int> *matchPSCS1, std::vector<int> *matchPSCS2, NRMat<float> WM) {
      int len      = WORDLENGTH;
      int size     = matchPSCS1->size();
      float weight = 1000;

      for(int iter = 0; iter < size; iter++) {
	  int i = matchPSCS1->at(iter);
	  int j = matchPSCS2->at(iter);

	  const StemCandidate &sc1 = pscs1->at(i);
	  const StemCandidate &sc2 = pscs2->at(j);
	
	  for(int k = 0; k < len; k++) {
	      WM[sc1.GetPosition() + k][sc2.GetPosition() + k] += weight;
//	      sumWeight += weight;
	  }
      }
      return WM;
  }

  /////////////////////////////////////////////////////////////////
  // ProbabilisticModel::ComputeForwardMatrix()
  //
  // Computes a set of forward probability matrices for aligning
  // seq1 and seq2.
  //
  // For efficiency reasons, a single-dimensional floating-point
  // array is used here, with the following indexing scheme:
  //
  //    forward[i + NumMatrixTypes * (j * (seq2Length+1) + k)]
  //    refers to the probability of aligning through j characters
  //    of the first sequence, k characters of the second sequence,
  //    and ending in state i.
  /////////////////////////////////////////////////////////////////

  VF *ComputeForwardMatrix (Sequence *seq1, Sequence *seq2) const {

    assert (seq1);
    assert (seq2);

    const int seq1Length = seq1->GetLength();
    const int seq2Length = seq2->GetLength();

    // retrieve the points to the beginning of each sequence
    SafeVector<char>::iterator iter1 = seq1->GetDataPtr();
    SafeVector<char>::iterator iter2 = seq2->GetDataPtr();

    // create matrix
    VF *forwardPtr = new VF (NumMatrixTypes * (seq1Length+1) * (seq2Length+1), LOG_ZERO);
    assert (forwardPtr);
    VF &forward = *forwardPtr;

    // initialization condition
    forward[0 + NumMatrixTypes * (1 * (seq2Length+1) + 1)] = 
      initialDistribution[0] + matchProb[(unsigned char) iter1[1]][(unsigned char) iter2[1]];
   
    for (int k = 0; k < NumInsertStates; k++){
      forward[2*k+1 + NumMatrixTypes * (1 * (seq2Length+1) + 0)] = 
	initialDistribution[2*k+1] + insProb[(unsigned char) iter1[1]][k];
      forward[2*k+2 + NumMatrixTypes * (0 * (seq2Length+1) + 1)] = 
	initialDistribution[2*k+2] + insProb[(unsigned char) iter2[1]][k]; 
    }
    
    // remember offset for each index combination
    int ij = 0;
    int i1j = -seq2Length - 1;
    int ij1 = -1;
    int i1j1 = -seq2Length - 2;

    ij *= NumMatrixTypes;
    i1j *= NumMatrixTypes;
    ij1 *= NumMatrixTypes;
    i1j1 *= NumMatrixTypes;

    // compute forward scores
    for (int i = 0; i <= seq1Length; i++){
      unsigned char c1 = (i == 0) ? '~' : (unsigned char) iter1[i];
      for (int j = 0; j <= seq2Length; j++){
        unsigned char c2 = (j == 0) ? '~' : (unsigned char) iter2[j];

	if (i > 1 || j > 1){
	  if (i > 0 && j > 0){
	    forward[0 + ij] = forward[0 + i1j1] + transProb[0][0];
	    for (int k = 1; k < NumMatrixTypes; k++)
	      LOG_PLUS_EQUALS (forward[0 + ij], forward[k + i1j1] + transProb[k][0]);
	    forward[0 + ij] += matchProb[c1][c2];
	  }
	  if (i > 0){
	    for (int k = 0; k < NumInsertStates; k++)
	      forward[2*k+1 + ij] = insProb[c1][k] +
		LOG_ADD (forward[0 + i1j] + transProb[0][2*k+1],
			 forward[2*k+1 + i1j] + transProb[2*k+1][2*k+1]);
	  }
	  if (j > 0){
	    for (int k = 0; k < NumInsertStates; k++)
	      forward[2*k+2 + ij] = insProb[c2][k] +
		LOG_ADD (forward[0 + ij1] + transProb[0][2*k+2],
			 forward[2*k+2 + ij1] + transProb[2*k+2][2*k+2]);
	  }
	}

        ij += NumMatrixTypes;
        i1j += NumMatrixTypes;
        ij1 += NumMatrixTypes;
        i1j1 += NumMatrixTypes;
      }
    }

    return forwardPtr;
  }

  /////////////////////////////////////////////////////////////////
  // ProbabilisticModel::ComputeBackwardMatrix()
  //
  // Computes a set of backward probability matrices for aligning
  // seq1 and seq2.
  //
  // For efficiency reasons, a single-dimensional floating-point
  // array is used here, with the following indexing scheme:
  //
  //    backward[i + NumMatrixTypes * (j * (seq2Length+1) + k)]
  //    refers to the probability of starting in state i and
  //    aligning from character j+1 to the end of the first
  //    sequence and from character k+1 to the end of the second
  //    sequence.
  /////////////////////////////////////////////////////////////////

  VF *ComputeBackwardMatrix (Sequence *seq1, Sequence *seq2) const {

    assert (seq1);
    assert (seq2);

    const int seq1Length = seq1->GetLength();
    const int seq2Length = seq2->GetLength();
    SafeVector<char>::iterator iter1 = seq1->GetDataPtr();
    SafeVector<char>::iterator iter2 = seq2->GetDataPtr();

    // create matrix
    VF *backwardPtr = new VF (NumMatrixTypes * (seq1Length+1) * (seq2Length+1), LOG_ZERO);
    assert (backwardPtr);
    VF &backward = *backwardPtr;

    // initialization condition
    for (int k = 0; k < NumMatrixTypes; k++)
      backward[NumMatrixTypes * ((seq1Length+1) * (seq2Length+1) - 1) + k] = initialDistribution[k];

    // remember offset for each index combination
    int ij = (seq1Length+1) * (seq2Length+1) - 1;
    int i1j = ij + seq2Length + 1;
    int ij1 = ij + 1;
    int i1j1 = ij + seq2Length + 2;

    ij *= NumMatrixTypes;
    i1j *= NumMatrixTypes;
    ij1 *= NumMatrixTypes;
    i1j1 *= NumMatrixTypes;

    // compute backward scores
    for (int i = seq1Length; i >= 0; i--){
      unsigned char c1 = (i == seq1Length) ? '~' : (unsigned char) iter1[i+1];
      for (int j = seq2Length; j >= 0; j--){
        unsigned char c2 = (j == seq2Length) ? '~' : (unsigned char) iter2[j+1];

        if (i < seq1Length && j < seq2Length){
          const float ProbXY = backward[0 + i1j1] + matchProb[c1][c2];
          for (int k = 0; k < NumMatrixTypes; k++)
            LOG_PLUS_EQUALS (backward[k + ij], ProbXY + transProb[k][0]);
        }
        if (i < seq1Length){
          for (int k = 0; k < NumInsertStates; k++){
            LOG_PLUS_EQUALS (backward[0 + ij], backward[2*k+1 + i1j] + insProb[c1][k] + transProb[0][2*k+1]);
            LOG_PLUS_EQUALS (backward[2*k+1 + ij], backward[2*k+1 + i1j] + insProb[c1][k] + transProb[2*k+1][2*k+1]);
          }
        }
        if (j < seq2Length){
          for (int k = 0; k < NumInsertStates; k++){
            LOG_PLUS_EQUALS (backward[0 + ij], backward[2*k+2 + ij1] + insProb[c2][k] + transProb[0][2*k+2]);
            LOG_PLUS_EQUALS (backward[2*k+2 + ij], backward[2*k+2 + ij1] + insProb[c2][k] + transProb[2*k+2][2*k+2]);
          }
        }

        ij -= NumMatrixTypes;
        i1j -= NumMatrixTypes;
        ij1 -= NumMatrixTypes;
        i1j1 -= NumMatrixTypes;
      }
    }

    return backwardPtr;
  }

  /////////////////////////////////////////////////////////////////
  // ProbabilisticModel::ComputeTotalProbability()
  //
  // Computes the total probability of an alignment given
  // the forward and backward matrices.
  /////////////////////////////////////////////////////////////////

  float ComputeTotalProbability (int seq1Length, int seq2Length,
                                 const VF &forward, const VF &backward) const {

    // compute total probability
    float totalForwardProb = LOG_ZERO;
    float totalBackwardProb = LOG_ZERO;
    for (int k = 0; k < NumMatrixTypes; k++){
      LOG_PLUS_EQUALS (totalForwardProb,
                       forward[k + NumMatrixTypes * ((seq1Length+1) * (seq2Length+1) - 1)] + 
		       backward[k + NumMatrixTypes * ((seq1Length+1) * (seq2Length+1) - 1)]);
    }

    totalBackwardProb = 
      forward[0 + NumMatrixTypes * (1 * (seq2Length+1) + 1)] +
      backward[0 + NumMatrixTypes * (1 * (seq2Length+1) + 1)];

    for (int k = 0; k < NumInsertStates; k++){
      LOG_PLUS_EQUALS (totalBackwardProb,
		       forward[2*k+1 + NumMatrixTypes * (1 * (seq2Length+1) + 0)] +
		       backward[2*k+1 + NumMatrixTypes * (1 * (seq2Length+1) + 0)]);
      LOG_PLUS_EQUALS (totalBackwardProb,
		       forward[2*k+2 + NumMatrixTypes * (0 * (seq2Length+1) + 1)] +
		       backward[2*k+2 + NumMatrixTypes * (0 * (seq2Length+1) + 1)]);
    }

    //    cerr << totalForwardProb << " " << totalBackwardProb << endl;
    
    return (totalForwardProb + totalBackwardProb) / 2;
  }

  /////////////////////////////////////////////////////////////////
  // ProbabilisticModel::ComputePosteriorMatrix()
  //
  // Computes the posterior probability matrix based on
  // the forward and backward matrices.
  /////////////////////////////////////////////////////////////////

  VF *ComputePosteriorMatrix (Sequence *seq1, Sequence *seq2,
                              const VF &forward, const VF &backward) const {

    assert (seq1);
    assert (seq2);

    const int seq1Length = seq1->GetLength();
    const int seq2Length = seq2->GetLength();

    float totalProb = ComputeTotalProbability (seq1Length, seq2Length,
                                               forward, backward);

    // compute posterior matrices
    VF *posteriorPtr = new VF((seq1Length+1) * (seq2Length+1)); assert (posteriorPtr);
    VF &posterior = *posteriorPtr;

    int ij = 0;
    VF::iterator ptr = posterior.begin();

    for (int i = 0; i <= seq1Length; i++){
      for (int j = 0; j <= seq2Length; j++){
        *(ptr++) = EXP (min (LOG_ONE, forward[ij] + backward[ij] - totalProb));
        ij += NumMatrixTypes;
      }
    }

    posterior[0] = 0;

    return posteriorPtr;
  }

  /*
  /////////////////////////////////////////////////////////////////
  // ProbabilisticModel::ComputeExpectedCounts()
  //
  // Computes the expected counts for the various transitions.
  /////////////////////////////////////////////////////////////////

  VVF *ComputeExpectedCounts () const {

    assert (seq1);
    assert (seq2);

    const int seq1Length = seq1->GetLength();
    const int seq2Length = seq2->GetLength();
    SafeVector<char>::iterator iter1 = seq1->GetDataPtr();
    SafeVector<char>::iterator iter2 = seq2->GetDataPtr();

    // compute total probability
    float totalProb = ComputeTotalProbability (seq1Length, seq2Length,
                                               forward, backward);

    // initialize expected counts
    VVF *countsPtr = new VVF(NumMatrixTypes + 1, VF(NumMatrixTypes, LOG_ZERO)); assert (countsPtr);
    VVF &counts = *countsPtr;

    // remember offset for each index combination
    int ij = 0;
    int i1j = -seq2Length - 1;
    int ij1 = -1;
    int i1j1 = -seq2Length - 2;

    ij *= NumMatrixTypes;
    i1j *= NumMatrixTypes;
    ij1 *= NumMatrixTypes;
    i1j1 *= NumMatrixTypes;

    // compute expected counts
    for (int i = 0; i <= seq1Length; i++){
      unsigned char c1 = (i == 0) ? '~' : (unsigned char) iter1[i];
      for (int j = 0; j <= seq2Length; j++){
        unsigned char c2 = (j == 0) ? '~' : (unsigned char) iter2[j];

        if (i > 0 && j > 0){
          for (int k = 0; k < NumMatrixTypes; k++)
            LOG_PLUS_EQUALS (counts[k][0],
                             forward[k + i1j1] + transProb[k][0] +
                             matchProb[c1][c2] + backward[0 + ij]);
        }
        if (i > 0){
          for (int k = 0; k < NumInsertStates; k++){
            LOG_PLUS_EQUALS (counts[0][2*k+1],
                             forward[0 + i1j] + transProb[0][2*k+1] +
                             insProb[c1][k] + backward[2*k+1 + ij]);
            LOG_PLUS_EQUALS (counts[2*k+1][2*k+1],
                             forward[2*k+1 + i1j] + transProb[2*k+1][2*k+1] +
                             insProb[c1][k] + backward[2*k+1 + ij]);
          }
        }
        if (j > 0){
          for (int k = 0; k < NumInsertStates; k++){
            LOG_PLUS_EQUALS (counts[0][2*k+2],
                             forward[0 + ij1] + transProb[0][2*k+2] +
                             insProb[c2][k] + backward[2*k+2 + ij]);
            LOG_PLUS_EQUALS (counts[2*k+2][2*k+2],
                             forward[2*k+2 + ij1] + transProb[2*k+2][2*k+2] +
                             insProb[c2][k] + backward[2*k+2 + ij]);
          }
        }

        ij += NumMatrixTypes;
        i1j += NumMatrixTypes;
        ij1 += NumMatrixTypes;
        i1j1 += NumMatrixTypes;
      }
    }

    // scale all expected counts appropriately
    for (int i = 0; i < NumMatrixTypes; i++)
      for (int j = 0; j < NumMatrixTypes; j++)
        counts[i][j] -= totalProb;

  }
  */

  /////////////////////////////////////////////////////////////////
  // ProbabilisticModel::ComputeNewParameters()
  //
  // Computes a new parameter set based on the expected counts
  // given.
  /////////////////////////////////////////////////////////////////

  void ComputeNewParameters (Sequence *seq1, Sequence *seq2,
			     const VF &forward, const VF &backward,
                             VF &initDistribMat, VF &gapOpen,
                             VF &gapExtend, VVF &emitPairs, VF &emitSingle, bool enableTrainEmissions) const {
    
    assert (seq1);
    assert (seq2);

    const int seq1Length = seq1->GetLength();
    const int seq2Length = seq2->GetLength();
    SafeVector<char>::iterator iter1 = seq1->GetDataPtr();
    SafeVector<char>::iterator iter2 = seq2->GetDataPtr();

    // compute total probability
    float totalProb = ComputeTotalProbability (seq1Length, seq2Length,
                                               forward, backward);
    
    // initialize expected counts
    VVF transCounts (NumMatrixTypes, VF (NumMatrixTypes, LOG_ZERO));
    VF initCounts (NumMatrixTypes, LOG_ZERO);
    VVF pairCounts (256, VF (256, LOG_ZERO));
    VF singleCounts (256, LOG_ZERO);
    
    // remember offset for each index combination
    int ij = 0;
    int i1j = -seq2Length - 1;
    int ij1 = -1;
    int i1j1 = -seq2Length - 2;

    ij *= NumMatrixTypes;
    i1j *= NumMatrixTypes;
    ij1 *= NumMatrixTypes;
    i1j1 *= NumMatrixTypes;

    // compute initial distribution posteriors
    initCounts[0] = LOG_ADD (forward[0 + NumMatrixTypes * (1 * (seq2Length+1) + 1)] +
			     backward[0 + NumMatrixTypes * (1 * (seq2Length+1) + 1)],
			     forward[0 + NumMatrixTypes * ((seq1Length+1) * (seq2Length+1) - 1)] + 
			     backward[0 + NumMatrixTypes * ((seq1Length+1) * (seq2Length+1) - 1)]);
    for (int k = 0; k < NumInsertStates; k++){
      initCounts[2*k+1] = LOG_ADD (forward[2*k+1 + NumMatrixTypes * (1 * (seq2Length+1) + 0)] +
				   backward[2*k+1 + NumMatrixTypes * (1 * (seq2Length+1) + 0)],
				   forward[2*k+1 + NumMatrixTypes * ((seq1Length+1) * (seq2Length+1) - 1)] + 
				   backward[2*k+1 + NumMatrixTypes * ((seq1Length+1) * (seq2Length+1) - 1)]);
      initCounts[2*k+2] = LOG_ADD (forward[2*k+2 + NumMatrixTypes * (0 * (seq2Length+1) + 1)] +
				   backward[2*k+2 + NumMatrixTypes * (0 * (seq2Length+1) + 1)],
				   forward[2*k+2 + NumMatrixTypes * ((seq1Length+1) * (seq2Length+1) - 1)] + 
				   backward[2*k+2 + NumMatrixTypes * ((seq1Length+1) * (seq2Length+1) - 1)]);
    }

    // compute expected counts
    for (int i = 0; i <= seq1Length; i++){
      unsigned char c1 = (i == 0) ? '~' : (unsigned char) toupper(iter1[i]);
      for (int j = 0; j <= seq2Length; j++){
        unsigned char c2 = (j == 0) ? '~' : (unsigned char) toupper(iter2[j]);

	if (i > 0 && j > 0){
	  if (enableTrainEmissions && i == 1 && j == 1){
	    LOG_PLUS_EQUALS (pairCounts[c1][c2],
			     initialDistribution[0] + matchProb[c1][c2] + backward[0 + ij]);
	    LOG_PLUS_EQUALS (pairCounts[c2][c1],
			     initialDistribution[0] + matchProb[c2][c1] + backward[0 + ij]);
	  }

	  for (int k = 0; k < NumMatrixTypes; k++){
	    LOG_PLUS_EQUALS (transCounts[k][0],
			     forward[k + i1j1] + transProb[k][0] +
			     matchProb[c1][c2] + backward[0 + ij]);
	    if (enableTrainEmissions && i != 1 || j != 1){
	      LOG_PLUS_EQUALS (pairCounts[c1][c2],
			       forward[k + i1j1] + transProb[k][0] +
			       matchProb[c1][c2] + backward[0 + ij]);
	      LOG_PLUS_EQUALS (pairCounts[c2][c1],
			       forward[k + i1j1] + transProb[k][0] +
			       matchProb[c2][c1] + backward[0 + ij]);
	    }
	  }
	}
	if (i > 0){
	  for (int k = 0; k < NumInsertStates; k++){
	    LOG_PLUS_EQUALS (transCounts[0][2*k+1],
			     forward[0 + i1j] + transProb[0][2*k+1] +
			     insProb[c1][k] + backward[2*k+1 + ij]);
	    LOG_PLUS_EQUALS (transCounts[2*k+1][2*k+1],
			     forward[2*k+1 + i1j] + transProb[2*k+1][2*k+1] +
			     insProb[c1][k] + backward[2*k+1 + ij]);
	    if (enableTrainEmissions){
	      if (i == 1 && j == 0){
		LOG_PLUS_EQUALS (singleCounts[c1],
				 initialDistribution[2*k+1] + insProb[c1][k] + backward[2*k+1 + ij]);
	      }
	      else {
		LOG_PLUS_EQUALS (singleCounts[c1],
				 forward[0 + i1j] + transProb[0][2*k+1] +
				 insProb[c1][k] + backward[2*k+1 + ij]);
		LOG_PLUS_EQUALS (singleCounts[c1],
				 forward[2*k+1 + i1j] + transProb[2*k+1][2*k+1] +
				 insProb[c1][k] + backward[2*k+1 + ij]);
	      }
	    }
	  }
	}
	if (j > 0){
	  for (int k = 0; k < NumInsertStates; k++){
	    LOG_PLUS_EQUALS (transCounts[0][2*k+2],
			     forward[0 + ij1] + transProb[0][2*k+2] +
			     insProb[c2][k] + backward[2*k+2 + ij]);
	    LOG_PLUS_EQUALS (transCounts[2*k+2][2*k+2],
			     forward[2*k+2 + ij1] + transProb[2*k+2][2*k+2] +
			     insProb[c2][k] + backward[2*k+2 + ij]);
	    if (enableTrainEmissions){
	      if (i == 0 && j == 1){
		LOG_PLUS_EQUALS (singleCounts[c2],
				 initialDistribution[2*k+2] + insProb[c2][k] + backward[2*k+2 + ij]);
	      }
	      else {
		LOG_PLUS_EQUALS (singleCounts[c2],
				 forward[0 + ij1] + transProb[0][2*k+2] +
				 insProb[c2][k] + backward[2*k+2 + ij]);
		LOG_PLUS_EQUALS (singleCounts[c2],
				 forward[2*k+2 + ij1] + transProb[2*k+2][2*k+2] +
				 insProb[c2][k] + backward[2*k+2 + ij]);
	      }
	    }
	  }
	}
      
        ij += NumMatrixTypes;
        i1j += NumMatrixTypes;
        ij1 += NumMatrixTypes;
        i1j1 += NumMatrixTypes;
      }
    }

    // scale all expected counts appropriately
    for (int i = 0; i < NumMatrixTypes; i++){
      initCounts[i] -= totalProb;
      for (int j = 0; j < NumMatrixTypes; j++)
        transCounts[i][j] -= totalProb;
    }
    if (enableTrainEmissions){
      for (int i = 0; i < 256; i++){
	for (int j = 0; j < 256; j++)
	  pairCounts[i][j] -= totalProb;
	singleCounts[i] -= totalProb;
      }
    }

    // compute new initial distribution
    float totalInitDistribCounts = 0;
    for (int i = 0; i < NumMatrixTypes; i++)
      totalInitDistribCounts += exp (initCounts[i]); // should be 2
    initDistribMat[0] = min (1.0f, max (0.0f, (float) exp (initCounts[0]) / totalInitDistribCounts));
    for (int k = 0; k < NumInsertStates; k++){
      float val = (exp (initCounts[2*k+1]) + exp (initCounts[2*k+2])) / 2;
      initDistribMat[2*k+1] = initDistribMat[2*k+2] = min (1.0f, max (0.0f, val / totalInitDistribCounts));
    }

    // compute total counts for match state
    float inMatchStateCounts = 0;
    for (int i = 0; i < NumMatrixTypes; i++)
      inMatchStateCounts += exp (transCounts[0][i]);
    for (int i = 0; i < NumInsertStates; i++){

      // compute total counts for gap state
      float inGapStateCounts =
        exp (transCounts[2*i+1][0]) +
        exp (transCounts[2*i+1][2*i+1]) +
        exp (transCounts[2*i+2][0]) +
        exp (transCounts[2*i+2][2*i+2]);

      gapOpen[2*i] = gapOpen[2*i+1] =
        (exp (transCounts[0][2*i+1]) +
         exp (transCounts[0][2*i+2])) /
        (2 * inMatchStateCounts);

      gapExtend[2*i] = gapExtend[2*i+1] =
        (exp (transCounts[2*i+1][2*i+1]) +
         exp (transCounts[2*i+2][2*i+2])) /
        inGapStateCounts;
    }

    if (enableTrainEmissions){
      float totalPairCounts = 0;
      float totalSingleCounts = 0;
      for (int i = 0; i < 256; i++){
	for (int j = 0; j <= i; j++)
	  totalPairCounts += exp (pairCounts[j][i]);
	totalSingleCounts += exp (singleCounts[i]);
      }
      
      for (int i = 0; i < 256; i++) if (!islower ((char) i)){
	int li = (int)((unsigned char) tolower ((char) i));
	for (int j = 0; j <= i; j++) if (!islower ((char) j)){
	  int lj = (int)((unsigned char) tolower ((char) j));
	  emitPairs[i][j] = emitPairs[i][lj] = emitPairs[li][j] = emitPairs[li][lj] = 
	    emitPairs[j][i] = emitPairs[j][li] = emitPairs[lj][i] = emitPairs[lj][li] = exp(pairCounts[j][i]) / totalPairCounts;
	}
	emitSingle[i] = emitSingle[li] = exp(singleCounts[i]) / totalSingleCounts;
      }
    }
  }
    
  /////////////////////////////////////////////////////////////////
  // ProbabilisticModel::ComputeAlignment()
  //
  // Computes an alignment based on the given posterior matrix.
  // This is done by finding the maximum summing path (or
  // maximum weight trace) through the posterior matrix.  The
  // final alignment is returned as a pair consisting of:
  //    (1) a string (e.g., XXXBBXXXBBBBBBYYYYBBB) where X's and
  //        denote insertions in one of the two sequences and
  //        B's denote that both sequences are present (i.e.
  //        matches).
  //    (2) a float indicating the sum achieved
  /////////////////////////////////////////////////////////////////

  pair<SafeVector<char> *, float> ComputeAlignment (int seq1Length, int seq2Length, const VF &posterior) const {

    float *twoRows = new float[(seq2Length+1)*2]; assert (twoRows);
    float *oldRow = twoRows;
    float *newRow = twoRows + seq2Length + 1;

    char *tracebackMatrix = new char[(seq1Length+1)*(seq2Length+1)]; assert (tracebackMatrix);
    char *tracebackPtr = tracebackMatrix;

    VF::const_iterator posteriorPtr = posterior.begin() + seq2Length + 1;

    // initialization
    for (int i = 0; i <= seq2Length; i++){
      oldRow[i] = 0;
      *(tracebackPtr++) = 'L';
    }

    // fill in matrix
    for (int i = 1; i <= seq1Length; i++){

      // initialize left column
      newRow[0] = 0;
      posteriorPtr++;
      *(tracebackPtr++) = 'U';

      // fill in rest of row
      for (int j = 1; j <= seq2Length; j++){
        ChooseBestOfThree (*(posteriorPtr++) + oldRow[j-1], newRow[j-1], oldRow[j],
                           'D', 'L', 'U', &newRow[j], tracebackPtr++); // Match, insert, delete
      }

      // swap rows
      float *temp = oldRow;
      oldRow = newRow;
      newRow = temp;
    }

    // store best score
    float total = oldRow[seq2Length];
    delete [] twoRows;

    // compute traceback
    SafeVector<char> *alignment = new SafeVector<char>; assert (alignment);
    int r = seq1Length, c = seq2Length;
    while (r != 0 || c != 0){
      char ch = tracebackMatrix[r*(seq2Length+1) + c];
      switch (ch){
      case 'L': c--; alignment->push_back ('Y'); break;
      case 'U': r--; alignment->push_back ('X'); break;
      case 'D': c--; r--; alignment->push_back ('B'); break;
      default: assert (false);
      }
    }

    delete [] tracebackMatrix;

    reverse (alignment->begin(), alignment->end());

    return make_pair(alignment, total);
  }

  /////////////////////////////////////////////////////////////////
  // ProbabilisticModel::ComputeAlignment2()
  //
  // Computes an alignment based on the given posterior matrix.
  // This is done by finding the maximum summing path (or
  // maximum weight trace) through the posterior matrix.  The
  // final alignment is returned as a pair consisting of:
  //    (1) a string (e.g., XXXBBXXXBBBBBBYYYYBBB) where X's and
  //        denote insertions in one of the two sequences and
  //        B's denote that both sequences are present (i.e.
  //        matches).
  //    (2) a float indicating the sum achieved
  /////////////////////////////////////////////////////////////////

  pair<SafeVector<char> *, float> ComputeAlignment2 (int seq1Length, int seq2Length,
						     const VF &posterior, std::vector<StemCandidate> *pscs1, std::vector<StemCandidate> *pscs2,
						     std::vector<int> *matchPSCS1, std::vector<int> *matchPSCS2) const {
    NRMat<float> WM(seq1Length + 1, seq2Length + 1);
    for (int i = 0; i <= seq1Length; i++) {
	for (int j = 0; j <= seq2Length; j++) {
	    WM[i][j] = 0;
	}
    }

    int len      = WORDLENGTH;
    int size     = matchPSCS1->size();
    float weight = 1000;

    for(int iter = 0; iter < size; iter++) {
	int i = matchPSCS1->at(iter);
	int j = matchPSCS2->at(iter);

	const StemCandidate &sc1 = pscs1->at(i);
	const StemCandidate &sc2 = pscs2->at(j);
	for(int k = 0; k < len; k++) {
	    WM[sc1.GetPosition() + k][sc2.GetPosition() + k] += weight;
	}
    }
    float *twoRows = new float[(seq2Length+1)*2]; assert (twoRows);
    float *oldRow = twoRows;
    float *newRow = twoRows + seq2Length + 1;

    char *tracebackMatrix = new char[(seq1Length+1)*(seq2Length+1)]; assert (tracebackMatrix);
    char *tracebackPtr = tracebackMatrix;

    VF::const_iterator posteriorPtr = posterior.begin() + seq2Length + 1;

    // initialization
    for (int i = 0; i <= seq2Length; i++){
      oldRow[i] = 0;
      *(tracebackPtr++) = 'L';
    }

    // fill in matrix
    for (int i = 1; i <= seq1Length; i++){

      // initialize left column
      newRow[0] = 0;
      posteriorPtr++;
      *(tracebackPtr++) = 'U';

      // fill in rest of row
      for (int j = 1; j <= seq2Length; j++){
        ChooseBestOfThree (*(posteriorPtr++) + oldRow[j-1] + WM[i][j], newRow[j-1], oldRow[j],
                           'D', 'L', 'U', &newRow[j], tracebackPtr++);
      }

      // swap rows
      float *temp = oldRow;
      oldRow = newRow;
      newRow = temp;
    }

    // store best score
    float total = oldRow[seq2Length];
    delete [] twoRows;

    // compute traceback
    SafeVector<char> *alignment = new SafeVector<char>; assert (alignment);
    int r = seq1Length, c = seq2Length;
    while (r != 0 || c != 0){
      char ch = tracebackMatrix[r*(seq2Length+1) + c];
      switch (ch){
      case 'L': c--; alignment->push_back ('Y'); break;
      case 'U': r--; alignment->push_back ('X'); break;
      case 'D': c--; r--; alignment->push_back ('B'); break;
      default: assert (false);
      }
    }

    delete [] tracebackMatrix;

    reverse (alignment->begin(), alignment->end());

    return make_pair(alignment, total);
  }

  /////////////////////////////////////////////////////////////////
  // ProbabilisticModel::ComputeAlignmentWithGapPenalties()
  //
  // Similar to ComputeAlignment() except with gap penalties.
  /////////////////////////////////////////////////////////////////

  pair<SafeVector<char> *, float> ComputeAlignmentWithGapPenalties (MultiSequence *align1,
                                                                    MultiSequence *align2,
                                                                    const VF &posterior, int numSeqs1,
                                                                    int numSeqs2,
                                                                    float gapOpenPenalty,
                                                                    float gapContinuePenalty) const {
    int seq1Length = align1->GetSequence(0)->GetLength();
    int seq2Length = align2->GetSequence(0)->GetLength();
    SafeVector<SafeVector<char>::iterator > dataPtrs1 (align1->GetNumSequences());
    SafeVector<SafeVector<char>::iterator > dataPtrs2 (align2->GetNumSequences());

    // grab character data
    for (int i = 0; i < align1->GetNumSequences(); i++)
      dataPtrs1[i] = align1->GetSequence(i)->GetDataPtr();
    for (int i = 0; i < align2->GetNumSequences(); i++)
      dataPtrs2[i] = align2->GetSequence(i)->GetDataPtr();

    // the number of active sequences at any given column is defined to be the
    // number of non-gap characters in that column; the number of gap opens at
    // any given column is defined to be the number of gap characters in that
    // column where the previous character in the respective sequence was not
    // a gap
    SafeVector<int> numActive1 (seq1Length+1), numGapOpens1 (seq1Length+1);
    SafeVector<int> numActive2 (seq2Length+1), numGapOpens2 (seq2Length+1);

    // compute number of active sequences and gap opens for each group
    for (int i = 0; i < align1->GetNumSequences(); i++){
      SafeVector<char>::iterator dataPtr = align1->GetSequence(i)->GetDataPtr();
      numActive1[0] = numGapOpens1[0] = 0;
      for (int j = 1; j <= seq1Length; j++){
        if (dataPtr[j] != '-'){
          numActive1[j]++;
          numGapOpens1[j] += (j != 1 && dataPtr[j-1] != '-');
        }
      }
    }
    for (int i = 0; i < align2->GetNumSequences(); i++){
      SafeVector<char>::iterator dataPtr = align2->GetSequence(i)->GetDataPtr();
      numActive2[0] = numGapOpens2[0] = 0;
      for (int j = 1; j <= seq2Length; j++){
        if (dataPtr[j] != '-'){
          numActive2[j]++;
          numGapOpens2[j] += (j != 1 && dataPtr[j-1] != '-');
        }
      }
    }

    VVF openingPenalty1 (numSeqs1+1, VF (numSeqs2+1));
    VF continuingPenalty1 (numSeqs1+1);
    VVF openingPenalty2 (numSeqs1+1, VF (numSeqs2+1));
    VF continuingPenalty2 (numSeqs2+1);

    // precompute penalties
    for (int i = 0; i <= numSeqs1; i++)
      for (int j = 0; j <= numSeqs2; j++)
        openingPenalty1[i][j] = i * (gapOpenPenalty * j + gapContinuePenalty * (numSeqs2 - j));
    for (int i = 0; i <= numSeqs1; i++)
      continuingPenalty1[i] = i * gapContinuePenalty * numSeqs2;
    for (int i = 0; i <= numSeqs2; i++)
      for (int j = 0; j <= numSeqs1; j++)
        openingPenalty2[i][j] = i * (gapOpenPenalty * j + gapContinuePenalty * (numSeqs1 - j));
    for (int i = 0; i <= numSeqs2; i++)
      continuingPenalty2[i] = i * gapContinuePenalty * numSeqs1;

    float *twoRows = new float[6*(seq2Length+1)]; assert (twoRows);
    float *oldRowMatch = twoRows;
    float *newRowMatch = twoRows + (seq2Length+1);
    float *oldRowInsertX = twoRows + 2*(seq2Length+1);
    float *newRowInsertX = twoRows + 3*(seq2Length+1);
    float *oldRowInsertY = twoRows + 4*(seq2Length+1);
    float *newRowInsertY = twoRows + 5*(seq2Length+1);

    char *tracebackMatrix = new char[3*(seq1Length+1)*(seq2Length+1)]; assert (tracebackMatrix);
    char *tracebackPtr = tracebackMatrix;

    VF::const_iterator posteriorPtr = posterior.begin() + seq2Length + 1;

    // initialization
    for (int i = 0; i <= seq2Length; i++){
      oldRowMatch[i] = oldRowInsertX[i] = (i == 0) ? 0 : LOG_ZERO;
      oldRowInsertY[i] = (i == 0) ? 0 : oldRowInsertY[i-1] + continuingPenalty2[numActive2[i]];
      *(tracebackPtr) = *(tracebackPtr+1) = *(tracebackPtr+2) = 'Y';
      tracebackPtr += 3;
    }

    // fill in matrix
    for (int i = 1; i <= seq1Length; i++){

      // initialize left column
      newRowMatch[0] = newRowInsertY[0] = LOG_ZERO;
      newRowInsertX[0] = oldRowInsertX[0] + continuingPenalty1[numActive1[i]];
      posteriorPtr++;
      *(tracebackPtr) = *(tracebackPtr+1) = *(tracebackPtr+2) = 'X';
      tracebackPtr += 3;

      // fill in rest of row
      for (int j = 1; j <= seq2Length; j++){

        // going to MATCH state
        ChooseBestOfThree (oldRowMatch[j-1],
                           oldRowInsertX[j-1],
                           oldRowInsertY[j-1],
                           'M', 'X', 'Y', &newRowMatch[j], tracebackPtr++);
        newRowMatch[j] += *(posteriorPtr++);

        // going to INSERT X state
        ChooseBestOfThree (oldRowMatch[j] + openingPenalty1[numActive1[i]][numGapOpens2[j]],
                           oldRowInsertX[j] + continuingPenalty1[numActive1[i]],
                           oldRowInsertY[j] + openingPenalty1[numActive1[i]][numGapOpens2[j]],
                           'M', 'X', 'Y', &newRowInsertX[j], tracebackPtr++);

        // going to INSERT Y state
        ChooseBestOfThree (newRowMatch[j-1] + openingPenalty2[numActive2[j]][numGapOpens1[i]],
                           newRowInsertX[j-1] + openingPenalty2[numActive2[j]][numGapOpens1[i]],
                           newRowInsertY[j-1] + continuingPenalty2[numActive2[j]],
                           'M', 'X', 'Y', &newRowInsertY[j], tracebackPtr++);
      }

      // swap rows
      float *temp;
      temp = oldRowMatch; oldRowMatch = newRowMatch; newRowMatch = temp;
      temp = oldRowInsertX; oldRowInsertX = newRowInsertX; newRowInsertX = temp;
      temp = oldRowInsertY; oldRowInsertY = newRowInsertY; newRowInsertY = temp;
    }

    // store best score
    float total;
    char matrix;
    ChooseBestOfThree (oldRowMatch[seq2Length], oldRowInsertX[seq2Length], oldRowInsertY[seq2Length],
                       'M', 'X', 'Y', &total, &matrix);

    delete [] twoRows;

    // compute traceback
    SafeVector<char> *alignment = new SafeVector<char>; assert (alignment);
    int r = seq1Length, c = seq2Length;
    while (r != 0 || c != 0){

      int offset = (matrix == 'M') ? 0 : (matrix == 'X') ? 1 : 2;
      char ch = tracebackMatrix[(r*(seq2Length+1) + c) * 3 + offset];
      switch (matrix){
      case 'Y': c--; alignment->push_back ('Y'); break;
      case 'X': r--; alignment->push_back ('X'); break;
      case 'M': c--; r--; alignment->push_back ('B'); break;
      default: assert (false);
      }
      matrix = ch;
    }

    delete [] tracebackMatrix;

    reverse (alignment->begin(), alignment->end());

    return make_pair(alignment, 1.0f);
  }


  /////////////////////////////////////////////////////////////////
  // ProbabilisticModel::ComputeViterbiAlignment()
  //
  // Computes the highest probability pairwise alignment using the
  // probabilistic model.  The final alignment is returned as a
  //  pair consisting of:
  //    (1) a string (e.g., XXXBBXXXBBBBBBYYYYBBB) where X's and
  //        denote insertions in one of the two sequences and
  //        B's denote that both sequences are present (i.e.
  //        matches).
  //    (2) a float containing the log probability of the best
  //        alignment (not used)
  /////////////////////////////////////////////////////////////////

  pair<SafeVector<char> *, float> ComputeViterbiAlignment (Sequence *seq1, Sequence *seq2) const {
    
    assert (seq1);
    assert (seq2);
    
    const int seq1Length = seq1->GetLength();
    const int seq2Length = seq2->GetLength();
    
    // retrieve the points to the beginning of each sequence
    SafeVector<char>::iterator iter1 = seq1->GetDataPtr();
    SafeVector<char>::iterator iter2 = seq2->GetDataPtr();
    
    // create viterbi matrix
    VF *viterbiPtr = new VF (NumMatrixTypes * (seq1Length+1) * (seq2Length+1), LOG_ZERO);
    assert (viterbiPtr);
    VF &viterbi = *viterbiPtr;

    // create traceback matrix
    VI *tracebackPtr = new VI (NumMatrixTypes * (seq1Length+1) * (seq2Length+1), -1);
    assert (tracebackPtr);
    VI &traceback = *tracebackPtr;

    // initialization condition
    for (int k = 0; k < NumMatrixTypes; k++)
      viterbi[k] = initialDistribution[k];

    // remember offset for each index combination
    int ij = 0;
    int i1j = -seq2Length - 1;
    int ij1 = -1;
    int i1j1 = -seq2Length - 2;

    ij *= NumMatrixTypes;
    i1j *= NumMatrixTypes;
    ij1 *= NumMatrixTypes;
    i1j1 *= NumMatrixTypes;

    // compute viterbi scores
    for (int i = 0; i <= seq1Length; i++){
      unsigned char c1 = (i == 0) ? '~' : (unsigned char) iter1[i];
      for (int j = 0; j <= seq2Length; j++){
        unsigned char c2 = (j == 0) ? '~' : (unsigned char) iter2[j];

        if (i > 0 && j > 0){
          for (int k = 0; k < NumMatrixTypes; k++){
	    float newVal = viterbi[k + i1j1] + transProb[k][0] + matchProb[c1][c2];
	    if (viterbi[0 + ij] < newVal){
	      viterbi[0 + ij] = newVal;
	      traceback[0 + ij] = k;
	    }
	  }
        }
        if (i > 0){
          for (int k = 0; k < NumInsertStates; k++){
	    float valFromMatch = insProb[c1][k] + viterbi[0 + i1j] + transProb[0][2*k+1];
	    float valFromIns = insProb[c1][k] + viterbi[2*k+1 + i1j] + transProb[2*k+1][2*k+1];
	    if (valFromMatch >= valFromIns){
	      viterbi[2*k+1 + ij] = valFromMatch;
	      traceback[2*k+1 + ij] = 0;
	    }
	    else {
	      viterbi[2*k+1 + ij] = valFromIns;
	      traceback[2*k+1 + ij] = 2*k+1;
	    }
	  }
	}
        if (j > 0){
          for (int k = 0; k < NumInsertStates; k++){
	    float valFromMatch = insProb[c2][k] + viterbi[0 + ij1] + transProb[0][2*k+2];
	    float valFromIns = insProb[c2][k] + viterbi[2*k+2 + ij1] + transProb[2*k+2][2*k+2];
	    if (valFromMatch >= valFromIns){
	      viterbi[2*k+2 + ij] = valFromMatch;
	      traceback[2*k+2 + ij] = 0;
	    }
	    else {
	      viterbi[2*k+2 + ij] = valFromIns;
	      traceback[2*k+2 + ij] = 2*k+2;
	    }
	  }
        }

        ij += NumMatrixTypes;
        i1j += NumMatrixTypes;
        ij1 += NumMatrixTypes;
        i1j1 += NumMatrixTypes;
      }
    }

    // figure out best terminating cell
    float bestProb = LOG_ZERO;
    int state = -1;
    for (int k = 0; k < NumMatrixTypes; k++){
      float thisProb = viterbi[k + NumMatrixTypes * ((seq1Length+1)*(seq2Length+1) - 1)] + initialDistribution[k];
      if (bestProb < thisProb){
	bestProb = thisProb;
	state = k;
      }
    }
    assert (state != -1);

    delete viterbiPtr;

    // compute traceback
    SafeVector<char> *alignment = new SafeVector<char>; assert (alignment);
    int r = seq1Length, c = seq2Length;
    while (r != 0 || c != 0){
      int newState = traceback[state + NumMatrixTypes * (r * (seq2Length+1) + c)];
      
      if (state == 0){ c--; r--; alignment->push_back ('B'); }
      else if (state % 2 == 1){ r--; alignment->push_back ('X'); }
      else { c--; alignment->push_back ('Y'); }
      
      state = newState;
    }

    delete tracebackPtr;

    reverse (alignment->begin(), alignment->end());
    
    return make_pair(alignment, bestProb);
  }

  /////////////////////////////////////////////////////////////////
  // ProbabilisticModel::BuildPosterior()
  //
  // Builds a posterior probability matrix needed to align a pair
  // of alignments.  Mathematically, the returned matrix M is
  // defined as follows:
  //    M[i,j] =     sum          sum      f(s,t,i,j)
  //             s in align1  t in align2
  // where
  //                  [  P(s[i'] <--> t[j'])
  //                  [       if s[i'] is a letter in the ith column of align1 and
  //                  [          t[j'] it a letter in the jth column of align2
  //    f(s,t,i,j) =  [
  //                  [  0    otherwise
  //
  /////////////////////////////////////////////////////////////////

  VF *BuildPosterior (MultiSequence *align1, MultiSequence *align2,
                      const SafeVector<SafeVector<SparseMatrix *> > &sparseMatrices,
		      float cutoff = 0.0f) const {
    const int seq1Length = align1->GetSequence(0)->GetLength();
    const int seq2Length = align2->GetSequence(0)->GetLength();

    VF *posteriorPtr = new VF((seq1Length+1) * (seq2Length+1), 0); assert (posteriorPtr);
    VF &posterior = *posteriorPtr;
    VF::iterator postPtr = posterior.begin();

    // for each s in align1
    for (int i = 0; i < align1->GetNumSequences(); i++){
      int first = align1->GetSequence(i)->GetLabel();
      SafeVector<int> *mapping1 = align1->GetSequence(i)->GetMapping();

      // for each t in align2
      for (int j = 0; j < align2->GetNumSequences(); j++){
        int second = align2->GetSequence(j)->GetLabel();
        SafeVector<int> *mapping2 = align2->GetSequence(j)->GetMapping();
	if (first < second){

	  // get the associated sparse matrix
	  SparseMatrix *matrix = sparseMatrices[first][second];
	  
	  for (int ii = 1; ii <= matrix->GetSeq1Length(); ii++){
	    SafeVector<PIF>::iterator row = matrix->GetRowPtr(ii);
	    int base = (*mapping1)[ii] * (seq2Length+1);
	    int rowSize = matrix->GetRowSize(ii);
	    // add in all relevant values
	    for (int jj = 0; jj < rowSize; jj++) 
	      posterior[base + (*mapping2)[row[jj].first]] += row[jj].second;

	    // subtract cutoff 
	    for (int jj = 0; jj < matrix->GetSeq2Length(); jj++) {
	      posterior[base + (*mapping2)[jj]] -= cutoff;
	    }

	  }

	} else {
	  // get the associated sparse matrix
	  SparseMatrix *matrix = sparseMatrices[second][first];
	  
	  for (int jj = 1; jj <= matrix->GetSeq1Length(); jj++){
	    SafeVector<PIF>::iterator row = matrix->GetRowPtr(jj);
	    int base = (*mapping2)[jj];
	    int rowSize = matrix->GetRowSize(jj);
	    
	    // add in all relevant values
	    for (int ii = 0; ii < rowSize; ii++)
	      posterior[base + (*mapping1)[row[ii].first] * (seq2Length + 1)] += row[ii].second;
	    
	    // subtract cutoff 
	    for (int ii = 0; ii < matrix->GetSeq2Length(); ii++)
	      posterior[base + (*mapping1)[ii] * (seq2Length + 1)] -= cutoff;
	  }

	}
	

        delete mapping2;
      }

      delete mapping1;
    }

    return posteriorPtr;
  }
};
}
#endif
