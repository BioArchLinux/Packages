#include "AlifoldMEA.h"

namespace MXSCARNA{

const int AlifoldMEA::TURN = 3;

void
AlifoldMEA::
Run()
{
    makeProfileBPPMatrix(alignment);
    Initialization();
    DP();
    TraceBack();
}

void
AlifoldMEA::
makeProfileBPPMatrix(const MultiSequence *Sequences)
{
    int length = Sequences->GetSequence(0)->GetLength();

    Trimat<float> *consBppMat = new Trimat<float>(length + 1);
    fill(consBppMat->begin(), consBppMat->end(), 0);

    for(int i = 1; i <= length; i++) 
	for (int j = i; j <= length; j++) 
	    bppMat.ref(i, j) = 0;


    int number = Sequences->GetNumSequences();
    for(int seqNum = 0; seqNum < number; seqNum++) {
	SafeVector<int> *tmpMap = Sequences->GetSequence(seqNum)->GetMappingNumber();
	int label = Sequences->GetSequence(seqNum)->GetLabel();
	BPPMatrix *tmpBppMatrix = BPPMatrices[label];
	
	for(int i = 1; i <= length ; i++) {
	    int originI = tmpMap->at(i);
	    for(int j = i; j <= length; j++) {
		int originJ = tmpMap->at(j);
		if(originI != 0 && originJ != 0) {
		    float tmpProb = tmpBppMatrix->GetProb(originI, originJ);
		    bppMat.ref(i, j) += tmpProb;
		}
	    }
	}
    }

	/* compute the mean of base pairing probability  */
    for(int i = 1; i <= length; i++) {
	for(int j = i; j <= length; j++) {
	    bppMat.ref(i,j) = bppMat.ref(i,j)/(float)number;
	}
    }

    for (int i = 1; i <= length; i++) {
	float sum = 0;
	for (int j = i; j <= length; j++) {
	    sum += bppMat.ref(i,j);
	}
	Qi[i] = 1 - sum;
    }

    for (int i = 1; i <= length; i++) {
	float sum = 0;
	for (int j = i; j >= 1; j--) {
	    sum += bppMat.ref(j, i);
	}
	Qj[i] = 1 - sum;
    }
}

void
AlifoldMEA::
Initialization()
{
    int length = alignment->GetSequence(0)->GetLength();

    for (int i = 1; i <= length; i++) {
	for (int j = i; j <= length; j++) {
	    M.ref(i,j) = 0;
	    traceI.ref(i,j) = 0;
	    traceJ.ref(i,j) = 0;
	}
    }

    for (int i = 1; i <= length; i++) {
	M.ref(i,i)   = Qi[i]; 
	traceI.ref(i,i) = 0;
	traceJ.ref(i,i) = 0;
    }

    for (int i = 1; i <= length - 1; i++) {
	M.ref(i, i+1) =  Qi[i+1];
	traceI.ref(i,i + 1) = 0;
	traceJ.ref(i,i + 1) = 0;
    }

    for (int i = 0; i <= length; i++) {
	ssCons[i] = '.';
    }
}

void
AlifoldMEA::
DP()
{
    float g    = BasePairConst; // see scarna.hpp
    int length = alignment->GetSequence(0)->GetLength();
    
    for (int i = length - 1; i >= 1; i--) {
	for (int j = i + TURN + 1; j <= length; j++) {
	    float qi       = Qi[i];
	    float qj       = Qj[j];
	    float p        = bppMat.ref(i,j);

	    
	    float maxScore = qi + M.ref(i+1, j);
	    int tmpI = i+1;
	    int tmpJ = j;
	    
	    float tmpScore = qj + M.ref(i, j-1);
	    if (tmpScore > maxScore) {
		maxScore = tmpScore;
		tmpI     = i;
		tmpJ     = j - 1;
	    }
	    
	    tmpScore = g*2*p + M.ref(i+1, j-1);
	    if (tmpScore > maxScore) {
		maxScore = tmpScore;
		tmpI     = i + 1;
		tmpJ     = j - 1;
	    }
	    
	    for (int k = i + 1; k < j - 1; k++) {
		tmpScore = M.ref(i,k) + M.ref(k+1,j);
		if (tmpScore > maxScore) {
		    maxScore = tmpScore;
		    tmpI = i;
		    tmpJ = j;
		}
	    }
	    M.ref(i,j)       = maxScore;
	    traceI.ref(i, j) = tmpI;
	    traceJ.ref(i, j) = tmpJ;
	}
    }
}

void
AlifoldMEA::
TraceBack()
{

    int length = alignment->GetSequence(0)->GetLength();
    SafeVector<int> stackI((length + 1)*(length+1));
    SafeVector<int> stackJ((length + 1)*(length+1));
    int pt = 0;

    stackI[pt] = traceI.ref(1, length);
    stackJ[pt] = traceJ.ref(1, length);
    ++pt;
    
    while(pt != 0) {
	--pt;
	int tmpI = stackI[pt];
	int tmpJ = stackJ[pt];
	int nextI = traceI.ref(tmpI, tmpJ);
	int nextJ = traceJ.ref(tmpI, tmpJ);

	if (tmpI < tmpJ) {
	    if (tmpI + 1  == nextI && tmpJ == nextJ) {
		stackI[pt] = nextI;
		stackJ[pt] = nextJ;
		++pt;
	    }
	    else if (tmpI == nextI && tmpJ - 1 == nextJ) {
		stackI[pt] = nextI;
		stackJ[pt] = nextJ;
		++pt;
	    }
	    else if (tmpI + 1 == nextI && tmpJ - 1== nextJ) {
		stackI[pt] = nextI;
		stackJ[pt] = nextJ;
		++pt;
		ssCons[tmpI] = '(';
		ssCons[tmpJ] = ')';
	    }
	    else if (tmpI == nextI && tmpJ == nextJ) {
		float maxScore = IMPOSSIBLE;
		int maxK = 0;

		for (int k = tmpI + 1; k < tmpJ - 1; k++) {
		    float tmpScore = M.ref(tmpI,k) + M.ref(k+1,tmpJ);
		    if (tmpScore > maxScore) {
			maxScore = tmpScore;
			maxK = k;
		    }
		}
		stackI[pt] = traceI.ref(tmpI, maxK);
		stackJ[pt] = traceJ.ref(tmpI, maxK);
		++pt;
		stackI[pt] = traceI.ref(maxK+1, tmpJ);
		stackJ[pt] = traceJ.ref(maxK+1, tmpJ);
		++pt;
	    }
	}
    }
}
}
