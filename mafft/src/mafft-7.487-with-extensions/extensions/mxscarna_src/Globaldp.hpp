////////////////////////////////////////////////////////////////
// Globaldp.hpp
// Global Alignment of two profile computed by SCARNA algorithm
////////////////////////////////////////////////////////////////


#ifndef __GLOBALDP_HPP__
#define  __GLOBALDP_HPP__

#include <string>
#include <iostream>
#include "nrutil.h"
#include "Util.hpp"
#include "Beta.hpp"
#include "scarna.hpp"
#include "StemCandidate.hpp"
#include "MultiSequence.h"
#include "Sequence.h"
#include "BPPMatrix.hpp"

using namespace ProbCons;
using namespace std;
namespace MXSCARNA {

typedef std::vector<StemCandidate> stemcandidate;
class Globaldp {
private:
    static double ribosum_matrix[16][16];

    NRMat<float> VM, VG;
    NRMat<int>   traceMI, traceMJ, traceGI, traceGJ;

    const stemcandidate *pscs1, *pscs2;
    const MultiSequence *seqs1, *seqs2;
    std::vector<int> *matchPSCS1, *matchPSCS2;
    SafeVector<BPPMatrix*> &BPPMatrices;
    NRMat<float> posterior;
    NRMat3d<int> countConp1, countConp2;
    NRMat<int> countContConp1, countContConp2;
    Trimat<float> *profileBPPMat1;
    Trimat<float> *profileBPPMat2;
    float multiDeltaScore;
    float multiDeltaStacking;
    float multiScore;
    float multiStacking;
    float multiPenalty;
    
    float scorePSCPair(const StemCandidate &sc1, const StemCandidate &sc2);
    float wordSimilarity(const StemCandidate &sc1, const StemCandidate &sc2, int i, int j);
    int   returnBasePairType(char s, char r);
    float probabilityScore(const StemCandidate &sc1, const StemCandidate &sc2);
    float stackingScore(const StemCandidate &sc1, const StemCandidate &sc2);
    float distancePenalty(const StemCandidate &sc1, const StemCandidate &sc2);
    float incrementalScorePSCPair(const StemCandidate &sc1, const StemCandidate &sc2);
    float incrementalProbabilityScore(const StemCandidate &sc1, const StemCandidate &sc2);
    float incrementalStackingScore(const StemCandidate &sc1, const StemCandidate &sc2);
    float incrementalWordSimilarity(const StemCandidate &sc1, const StemCandidate &sc2, int i, int j);
    Trimat<float>* makeProfileBPPMatrix(const MultiSequence *Sequences);

    void Initialize();
    void DP();
    float traceBack();
    void printMatch();
public:

    Globaldp(const stemcandidate *mypscs1, const stemcandidate *mypscs2, 
	     const MultiSequence *myseqs1, const MultiSequence *myseqs2,
	     std::vector<int> *matchPSCS1, std::vector<int> *matchPSCS2,
	     VF *myPosterior, SafeVector<BPPMatrix*> &myBPPMatrices,
	     float myMultiDeltaScore = MULTIDELTASCORE, float myMultiDeltaStacking = MULTIDELTASTACKING,
	     float myMultiScore = MULTISCORE, float myMultiStacking = MULTISTACKING,
	     float myMultiPenalty = MULTIPENALTY) 
	: VM(mypscs1->size(), mypscs2->size()), 
	  VG(mypscs1->size(), mypscs2->size()),
	  traceMI(mypscs1->size(), mypscs2->size()),
	  traceMJ(mypscs1->size(), mypscs2->size()),
	  traceGI(mypscs1->size(), mypscs2->size()),
	  traceGJ(mypscs1->size(), mypscs2->size()),
	  pscs1(mypscs1), pscs2(mypscs2), 
	  seqs1(myseqs1), seqs2(myseqs2),
	  matchPSCS1(matchPSCS1), matchPSCS2(matchPSCS2), 
	  BPPMatrices(myBPPMatrices), posterior(myseqs1->GetSequence(0)->GetLength() + 1, myseqs2->GetSequence(0)->GetLength() + 1),
	  countConp1(WORDLENGTH, 17, mypscs1->size()+1), countConp2(WORDLENGTH, 17, mypscs2->size()+1),
	  countContConp1(0, 17, mypscs1->size()+1), countContConp2(0, 17, mypscs2->size()+1), 
	  multiDeltaScore(myMultiDeltaScore), multiDeltaStacking(myMultiDeltaStacking),
	  multiScore(myMultiScore), multiStacking(myMultiStacking), 
	  multiPenalty(myMultiPenalty) {
	
	VF::iterator myPosteriorPtr = myPosterior->begin();
	for (int i = 0; i <= seqs1->GetSequence(0)->GetLength(); i++) {
	    for (int j = 0; j <= seqs2->GetSequence(0)->GetLength(); j++) {
		posterior[i][j] = (*myPosteriorPtr)/(seqs1->GetSequence(0)->GetLength()*seqs2->GetSequence(0)->GetLength());
		myPosteriorPtr++;
	    }
	}
    }


							 
    float Run();

    void setMultiDeltaScore(float score) { multiDeltaScore = score; }
    void setMultiDeltaStacking(float score) { multiDeltaStacking = score; }
    void setMultiScore(float score) { multiScore = score; }
    void setMultiStacking(float score) { multiStacking = score; }
    void setMultiPenalty(float score) { multiPenalty = score; }
};
}
#endif // __GLOBALDP_HPP__
