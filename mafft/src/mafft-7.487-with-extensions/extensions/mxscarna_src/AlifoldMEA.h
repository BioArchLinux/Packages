#ifndef _ALIFOLDMEA_H_
#define _ALIFOLDMEA_H_

#include <iostream>
#include <string>
#include <cassert>
#include "scarna.hpp"
#include "nrutil.h"
#include "Util.hpp"
#include "Beta.hpp"
#include "BPPMatrix.hpp"
#include "MultiSequence.h"
#include "Sequence.h"
#include "SafeVector.h"

using namespace std;

namespace MXSCARNA {
class AlifoldMEA {
    MultiSequence *alignment;
    SafeVector<BPPMatrix*> BPPMatrices;
    float BasePairConst;
    Trimat<float> M;
    Trimat<int> traceI;
    Trimat<int> traceJ;
    Trimat<float> bppMat;
    NRVec<float>  Qi;
    NRVec<float>  Qj;
    NRVec<char> ssCons;
    
    static const int TURN;

    void Initialization();
    void makeProfileBPPMatrix(const MultiSequence *Sequences);
    void DP();
    void TraceBack();
 public:
    AlifoldMEA(MultiSequence *inalignment, SafeVector<BPPMatrix*> &inBPPMatrices, float inBasePairConst = 6) :
	alignment(inalignment), BPPMatrices(inBPPMatrices),
	BasePairConst(inBasePairConst),
	M(inalignment->GetSequence(0)->GetLength()+1), 
	traceI(inalignment->GetSequence(0)->GetLength()+1),
	traceJ(inalignment->GetSequence(0)->GetLength()+1),
	bppMat(inalignment->GetSequence(0)->GetLength() + 1), 
	Qi(inalignment->GetSequence(0)->GetLength() + 1), 
	Qj(inalignment->GetSequence(0)->GetLength() + 1), 
	ssCons(inalignment->GetSequence(0)->GetLength() + 1){}

    void Run();

    string *getSScons() {
	string *sscons = new string();
	sscons->push_back(' ');
	int length = alignment->GetSequence(0)->GetLength();
	for (int i = 1; i <= length; i++) {
	    sscons->push_back(ssCons[i]);
	}
	
	return sscons;
    }
};
}
#endif // _ALIFOLDMEA_H_
