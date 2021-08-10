#include "Globaldp.hpp"

namespace MXSCARNA {

double Globaldp::ribosum_matrix[16][16]
= { { -2.49,  -7.04,  -8.24,  -4.32,  -8.84,  -14.37,  -4.68, -12.64, -6.86, -5.03, -8.39,  -5.84, -4.01, -11.32, -6.16, -9.05 },
    { -7.04,  -2.11,  -8.89,  -2.04,  -9.37,  -9.08,   -5.86, -10.45, -9.73, -3.81, -11.05, -4.72, -5.33, -8.67,  -6.93, -7.83 },
    { -8.24,  -8.89,  -0.80,  -5.13,  -10.41, -14.53,  -4.57, -10.14, -8.61, -5.77, -5.38,  -6.60, -5.43, -8.87,  -5.94, -11.07 },
    { -4.32,  -2.04,  -5.13,   4.49,  -5.56,  -6.71,    1.67, -5.17,  -5.33,  2.70, -5.61,   0.59,  1.61, -4.81,  -0.51, -2.98 },
    { -8.84,  -9.37,  -10.41, -5.56,  -5.13,  -10.45,  -3.57, -8.49,  -7.98, -5.95, -11.36, -7.93, -2.42, -7.08,  -5.63, -8.39 },
    { -14.37, -9.08,  -14.53, -6.71,  -10.45, -3.59,   -5.71, -5.77, -12.43, -3.70, -12.58, -7.88, -6.88, -7.40,  -8.41, -5.41 },
    { -4.68,  -5.86,  -4.57,   1.67,  -3.57,  -5.71,    5.36, -4.96,  -6.00,  2.11, -4.66,  -0.27,  2.75, -4.91,   1.32, -3.67 },
    { -12.64, -10.45, -10.14, -5.17,  -8.49,  -5.77,   -4.96, -2.28,  -7.71, -5.84, -13.69, -5.61, -4.72, -3.83,  -7.36, -5.21 },
    { -6.86,  -9.73,  -8.61,  -5.33,  -7.98,  -12.43,  -6.00, -7.71,  -1.05, -4.88, -8.67,  -6.10, -5.85, -6.63,  -7.55, -11.54 },
    { -5.03,  -3.81,  -5.77,   2.70,  -5.95,  -3.70,    2.11, -5.84,  -4.88,  5.62, -4.13,   1.21,  1.60, -4.49,  -0.08, -3.90 },
    { -8.39,  -11.05, -5.38,  -5.61,  -11.36, -12.58,  -4.66, -13.69, -8.67, -4.13, -1.98,  -5.77, -5.75, -12.01, -4.27, -10.79 },
    { -5.84,  -4.72,  -6.60,   0.59,  -7.93,  -7.88,   -0.27, -5.61,  -6.10,  1.21, -5.77,   3.47, -0.57, -5.30,  -2.09, -4.45 },
    { -4.01,  -5.33,  -5.43,   1.61,  -2.42,  -6.88,    2.75, -4.72,  -5.85,  1.60, -5.75,  -0.57,  4.97, -2.98,   1.14, -3.39 },
    { -11.32, -8.67,  -8.87,  -4.81,  -7.08,  -7.40,   -4.91, -3.83,  -6.63, -4.49, -12.01, -5.30, -2.98, -3.21,  -4.76, -5.97 },
    { -6.16,  -6.93,  -5.94,  -0.51,  -5.63,  -8.41,    1.32, -7.36,  -7.55, -0.08, -4.27,  -2.09,  1.14, -4.76,   3.36, -4.28 },
    { -9.05,  -7.83,  -11.07, -2.98,  -8.39,  -5.41,   -3.67, -5.21, -11.54, -3.90, -10.79, -4.45, -3.39, -5.97,  -4.28, -0.02 }
};


Trimat<float>* 
Globaldp::
makeProfileBPPMatrix(const MultiSequence *Sequences)
{
    int length = Sequences->GetSequence(0)->GetLength();
    float thr  = THR;
    Trimat<float> *consBppMat = new Trimat<float>(length + 1);
    fill(consBppMat->begin(), consBppMat->end(), 0);

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

		    if(tmpProb >= thr) {
			consBppMat->ref(i, j) += tmpProb;
		    }
		}
	    }
	}
    }

	/* compute the mean of base pairing probability  */
    for(int i = 1; i <= length; i++) {
	for(int j = i; j <= length; j++) {
	    consBppMat->ref(i,j) = consBppMat->ref(i,j)/(float)number;
	}
    }

    return consBppMat;   
}

float 
Globaldp::
incrementalScorePSCPair(const StemCandidate &sc1, const StemCandidate &sc2)
{
    int wordLength = WORDLENGTH;

    int pos1, rvpos1;
    if (sc1.GetDistance() > 0) {
	pos1   = sc1.GetPosition();
	rvpos1 = sc1.GetRvposition();
    }
    else {
	pos1   = sc1.GetRvposition();
	rvpos1 = sc1.GetPosition();
    }

    int pos2, rvpos2;
    if (sc2.GetDistance() > 0) {
	pos2   = sc2.GetPosition();
	rvpos2 = sc2.GetRvposition();
    }
    else {
	pos2   = sc2.GetRvposition();
	rvpos2 = sc2.GetPosition();
    }
/*
    cout << "1:" << i << " " << j << " " << sc1.GetDistance() << " " << sc2.GetDistance() << endl;
    cout << sc1.GetPosition() << " " << sc1.GetRvposition() << " " << sc2.GetPosition() << " " << sc2.GetRvposition() << endl;
*/  
    float score = 
	  posterior[sc1.GetPosition() + wordLength - 1][sc2.GetPosition() + wordLength - 1]
//	* sc1.GetBaseScore(wordLength - 1)
	* profileBPPMat1->ref(pos1 + wordLength - 1, rvpos1)
	* posterior[sc1.GetRvposition()][sc2.GetRvposition()]
//	* sc2.GetBaseScore(wordLength - 1);
	* profileBPPMat2->ref(pos2 + wordLength - 1, rvpos2);
	
	
/*
	  incrementalWordSimilarity(sc1, sc2, i, j)
	+ incrementalProbabilityScore(sc1, sc2) * multiDeltaScore
	+ incrementalStackingScore(sc1, sc2) * multiDeltaStacking;
*/

    return score*sc1.GetNumSeq()*sc2.GetNumSeq();
}

float
Globaldp::
incrementalProbabilityScore(const StemCandidate &sc1, const StemCandidate &sc2)
{
    int wordLength = WORDLENGTH;
    return sc1.GetBaseScore(wordLength-1) + sc2.GetBaseScore(wordLength-1);
}

float
Globaldp::
incrementalStackingScore(const StemCandidate &sc1, const StemCandidate &sc2)
{
    return - (sc1.GetStacking() + sc2.GetStacking());
}

float
Globaldp::
incrementalWordSimilarity(const StemCandidate &sc1, const StemCandidate &sc2, int i, int j)
{
    int numSeq1 = sc1.GetNumSeq();
    int numSeq2 = sc2.GetNumSeq();

    float score = 0;

    for(int k = 0; k < 16; k++) {
	for(int l = 0; l < 16; l++) {
	    score += countContConp1[k][i]*countContConp2[l][j]*ribosum_matrix[k][l];
	}
    }

    return score/(numSeq1*numSeq2);
}

float 
Globaldp::
scorePSCPair(const StemCandidate &sc1, const StemCandidate &sc2)
{


    int wordLength = WORDLENGTH;
    float score = 0;
    
    int pos1, rvpos1;
    if (sc1.GetDistance() > 0) {
	pos1   = sc1.GetPosition();
	rvpos1 = sc1.GetRvposition();
    }
    else {
	pos1   = sc1.GetRvposition();
	rvpos1 = sc1.GetPosition();
    }

    int pos2, rvpos2;
    if (sc2.GetDistance() > 0) {
	pos2   = sc2.GetPosition();
	rvpos2 = sc2.GetRvposition();
    }
    else {
	pos2   = sc2.GetRvposition();
	rvpos2 = sc2.GetPosition();
    }
    
    for (int k = 0; k < wordLength; k++) {
	score +=
	      posterior[sc1.GetPosition() + k][sc2.GetPosition() + k]
	    * profileBPPMat1->ref(pos1 + k, rvpos1 + wordLength - 1 - k)
//	    * sc1.GetBaseScore(k)
	    * posterior[sc1.GetRvposition() + wordLength - 1 - k][sc2.GetRvposition() + wordLength - 1 - k]
//	    * sc2.GetBaseScore(k);
	    * profileBPPMat2->ref(pos2 + k, rvpos2 + wordLength - 1 - k);
    }
    // validation code
    /*
    if (profileBPPMat1->ref(pos1 , rvpos1 + wordLength - 1) != sc1.GetBaseScore(0)) {
	cout << "1 " << profileBPPMat1->ref(pos1, rvpos1 + wordLength - 1) << " " << sc1.GetBaseScore(0) << endl;
    }
    if (profileBPPMat2->ref(pos2, rvpos2 + wordLength - 1) != sc2.GetBaseScore(0)) {
	cout << profileBPPMat2->ref(pos2, rvpos2 + wordLength - 1) << " " << sc2.GetBaseScore(0) << endl;
    }
    if (profileBPPMat1->ref(pos1 + 1, rvpos1 + wordLength - 1 - 1) != sc1.GetBaseScore(1)) {
	cout << "1 " << profileBPPMat1->ref(pos1 + 1, rvpos1 + wordLength - 1 - 1) << " " << sc1.GetBaseScore(1) << endl;
    }
    if (profileBPPMat2->ref(pos2 + 1, rvpos2 + wordLength - 1 - 1) != sc2.GetBaseScore(1)) {
	cout << profileBPPMat2->ref(pos2 + 1, rvpos2 + wordLength - 1 - 1) << " " << sc2.GetBaseScore(1) << endl;
	}*/

//    cout << sc1.GetPosition() << " " << sc1.GetRvposition() << " " << sc1.GetDistance() << endl;
//    cout << sc2.GetPosition() << " " << sc2.GetRvposition() << " " << sc2.GetDistance() << endl;
/*
	  wordSimilarity(sc1, sc2, i, j)
	+ probabilityScore(sc1, sc2) * multiScore
	+ stackingScore(sc1, sc2) * multiStacking

*/ 
/*
    if (sc1.GetDistance() < 0 && sc2.GetDistance() < 0) {
	cout << "2:" << i << " " << j << " " << sc1.GetDistance() << " " << sc2.GetDistance() << endl;
	cout << sc1.GetPosition() << " " << sc1.GetRvposition() << " " << sc2.GetPosition() << " " << sc2.GetRvposition() << endl;
	cout << "2:score" << score << endl;

    }
*/
    return score*sc1.GetNumSeq()*sc2.GetNumSeq();
}

float
Globaldp::
wordSimilarity(const StemCandidate &sc1, const StemCandidate &sc2, int i, int j)
{
    int wordLength = WORDLENGTH;

    int numSeq1 = sc1.GetNumSeq();
    int numSeq2 = sc2.GetNumSeq();

    float score = 0;

    for(int k = 0; k < 16; k++) {
	for(int l = 0; l < 16; l++) {
	    for(int iter = 0; iter < wordLength; iter++) {
		score += countConp1[iter][k][i]*countConp2[iter][l][j]*ribosum_matrix[k][l];
	    }
	}
    }

    return score/(numSeq1*numSeq2);
}

int
Globaldp::
returnBasePairType(char s, char r)
{
    if  (s == 'A') {
	if      (r == 'A') return 0;
	else if (r == 'C') return 1;
	else if (r == 'G') return 2;
	else if (r == 'U') return 3;
    }
    else if (s == 'C') {
	if      (r == 'A') return 4;
	else if (r == 'C') return 5;
	else if (r == 'G') return 6;
	else if (r == 'U') return 7;
    }
    else if (s == 'G') {
	if      (r == 'A') return 8;
	else if (r == 'C') return 9;
	else if (r == 'G') return 10;
	else if (r == 'U') return 11;
    }
    else if (s == 'U') {
	if      (r == 'A') return 12;
	else if (r == 'C') return 13;
	else if (r == 'G') return 14;
	else if (r == 'U') return 15;
    }

    return 16;
}

float
Globaldp::
probabilityScore(const StemCandidate &sc1, const StemCandidate &sc2)
{
    return sc1.GetScore() + sc2.GetScore();
}

float
Globaldp::
stackingScore(const StemCandidate &sc1, const StemCandidate &sc2)
{
    return - (sc1.GetStemStacking() + sc2.GetStemStacking());
}

float
Globaldp::
distancePenalty(const StemCandidate &sc1, const StemCandidate &sc2)
{
    return std::sqrt((float)std::abs(sc1.GetDistance() - sc2.GetDistance()));
}

float
Globaldp::
Run()
{
    Initialize();
    DP();
    float score = traceBack();
    
    // printMatch();
    //cout << "score=" << score << endl;
    return score;
}

void
Globaldp::
Initialize()
{
    int nPscs1 = pscs1->size();
    int nPscs2 = pscs2->size();


    for(int i = 0; i < nPscs1; i++) {
	for(int j = 0; j < nPscs2; j++) {
	    VM[i][j] = 0;
	    VG[i][j] = 0;
	}
    }

    VM[0][0] = 0;
    VG[0][0] = IMPOSSIBLE;

    for (int i = 1; i < nPscs1; i++) {
	VM[i][0] = IMPOSSIBLE; VG[i][0] = 0;
    }
    for (int j = 1; j < nPscs2; j++) {
	VM[0][j] = IMPOSSIBLE; VG[0][j] = 0;
    }

    for (int i = 0; i < nPscs1; i++) {
	for (int j = 0; j < nPscs2; j++) {
	    traceMI[i][j] = 0; traceMJ[i][j] = 0;
	    traceGI[i][j] = 0; traceGJ[i][j] = 0;
	}
    }

    int wordLength = WORDLENGTH;
    int size1   = pscs1->size();
    int size2   = pscs2->size();

    for(int i = 0; i < wordLength; i++) {
	for(int j = 0; j < 17; j++) {
	    for(int k = 0; k < (int)pscs1->size(); k++) {
		countConp1[i][j][k] = 0;
	    }
	}
    }
    for(int i = 0; i < wordLength; i++) {
	for(int j = 0; j < 17; j++) {
	    for(int k = 0; k < (int)pscs2->size(); k++) {
		countConp2[i][j][k] = 0;
	    }
	}
    }
    for(int i = 0; i < 17; i++) {
	for(int j = 0; j < (int)pscs1->size()+1; j++) {
	    countContConp1[i][j] = 0;
	}
    }
    for(int i = 0; i < 17; i++) {
	for(int j = 0; j < (int)pscs2->size()+1; j++) {
	    countContConp2[i][j] = 0;
	}
    }

    for(int iter = 1; iter < size1; iter++) {

	const StemCandidate &sc1 = pscs1->at(iter);
	int numSeq1 = sc1.GetNumSeq();
	for (int i = 0; i < wordLength; i++) {
	
	    for(int k = 0; k < numSeq1; k++) {
//		const Sequence *seq = seqs1->GetSequence(k);
		string substr = sc1.GetSubstr(k);
		string rvstr  = sc1.GetRvstr(k);
	    
		char sChar = substr[i];
		char rChar = rvstr[wordLength - 1 -i];
	    
		int type = returnBasePairType(sChar, rChar);
		++countConp1[i][type][iter];
	    }
	}
	for(int k = 0; k < numSeq1; k++) {
//	    const Sequence *seq = seqs1->GetSequence(k);
	    string substr = sc1.GetSubstr(k);
	    string rvstr  = sc1.GetRvstr(k);

	    char sChar = substr[wordLength-1];
	    char rChar = rvstr[0];
	    
	    int type = returnBasePairType(sChar, rChar);
	    ++countContConp1[type][iter];

	}
    }
    for(int iter = 1; iter < size2; iter++) {
	const StemCandidate &sc2 = pscs2->at(iter);
	int numSeq2 = sc2.GetNumSeq();
	for (int i = 0; i < wordLength; i++) {
	
	    for(int k = 0; k < numSeq2; k++) {
//		const Sequence *seq = seqs2->GetSequence(k);
		string substr = sc2.GetSubstr(k);
		string rvstr  = sc2.GetRvstr(k);
	    
		char sChar = substr[i];
		char rChar = rvstr[wordLength - 1 -i];
	    
		int type = returnBasePairType(sChar, rChar);
		++countConp2[i][type][iter];
	    }
	}
	for(int k = 0; k < numSeq2; k++) {
//	    const Sequence *seq = seqs2->GetSequence(k);
	    string substr = sc2.GetSubstr(k);
	    string rvstr  = sc2.GetRvstr(k);

	    char sChar = substr[wordLength-1];
	    char rChar = rvstr[0];
	    
	    int type = returnBasePairType(sChar, rChar);
	    ++countContConp2[type][iter];

	}
    }
    profileBPPMat1 = makeProfileBPPMatrix(seqs1);
    profileBPPMat2 = makeProfileBPPMatrix(seqs2);
}

void
Globaldp::
DP()
{
    int nPscs1 = pscs1->size();
    int nPscs2 = pscs2->size();
    
    for(int i = 1; i < nPscs1; i++) {
	const StemCandidate &sc1 = pscs1->at(i);

	for(int j = 1; j < nPscs2; j++) {
	    const StemCandidate &sc2 = pscs2->at(j);
	    
	    float max = IMPOSSIBLE;
	    int traceI = 0; 
	    int traceJ = 0;
	    int alpha = sc1.GetContPos(), beta = sc2.GetContPos();
	    if( (alpha > 0) && (beta > 0) ) {
		max = VM[alpha][beta] + incrementalScorePSCPair(sc1, sc2);
		traceI = alpha;
		traceJ = beta;
	    }
	    
	    float similarity = scorePSCPair(sc1, sc2);
	    int p = sc1.GetBeforePos(), q = sc2.GetBeforePos();
	    float tmpScore = VM[p][q] + similarity;
//	    cout << i << " " << j << " " << p << " " << q << " " << tmpScore << " " << VM[p][q] << " " << similarity << " " << endl;
		
	    if(max <= tmpScore) {
		max = tmpScore;
		traceI = p;
		traceJ = q;
	    }

	    tmpScore = VG[p][q] + similarity;
	    if(max <= tmpScore) {
		max = tmpScore;
		traceI = traceGI[p][q];
		traceJ = traceGJ[p][q];
	    }
	    
	    VM[i][j]      = max;
	    traceMI[i][j] = traceI;
	    traceMJ[i][j] = traceJ;
	    
	    max = VM[i][j-1];
	    traceI   = i;
	    traceJ   = j-1;

	    tmpScore = VM[i-1][j];
	    if(max <= tmpScore) {
		max    = tmpScore;
		traceI = i-1;
		traceJ = j;
	    }

	    tmpScore = VG[i-1][j];
	    if(max <= tmpScore) {
		max    = tmpScore;
		traceI = traceGI[i-1][j];
		traceJ = traceGJ[i-1][j];
	    }
	    
	    tmpScore = VG[i][j-1];
	    if(max <= tmpScore) {
		max = tmpScore;
		traceI = traceGI[i][j-1];
		traceJ = traceGJ[i][j-1];
	    }

	    VG[i][j]      = max;
	    traceGI[i][j] = traceI;
	    traceGJ[i][j] = traceJ;
	}
    }
}

float
Globaldp::
traceBack()
{
    int nPscs1 = pscs1->size();
    int nPscs2 = pscs2->size();
    
    float max = IMPOSSIBLE;
    int traceI, traceJ;
    for (int i = 1; i < nPscs1; i++) {
	for (int j = 1; j < nPscs2; j++) {
	    if(max < VM[i][j]) {
		max = VM[i][j];
		traceI = i;
		traceJ = j;
	    }
	}
    }

    while ( (traceI != 0) &&  (traceJ != 0) ) {
	matchPSCS1->push_back(traceI); matchPSCS2->push_back(traceJ);
	int tmpI = traceMI[traceI][traceJ];
	int tmpJ = traceMJ[traceI][traceJ];
	traceI = tmpI; traceJ = tmpJ;
    }
    
    if(traceI != 0 && traceJ != 0) {
	std::cout << "error in trace back of pscs:" << traceI << " " << traceJ << endl;
    }

    return max;
}

void
Globaldp::
printMatch()
{
    int size = matchPSCS1->size();
    for(int iter = 0; iter < size; iter++) {
	int i = matchPSCS1->at(iter);
	int j = matchPSCS2->at(iter);
	
	const StemCandidate &sc1 = pscs1->at(i);
	const StemCandidate &sc2 = pscs2->at(j);

	std::cout << iter << "\t" << sc1.GetPosition()  << "\t" << sc1.GetRvposition() << "\t" << sc2.GetPosition() << "\t" << sc2.GetRvposition() << endl;
    }

}
}
