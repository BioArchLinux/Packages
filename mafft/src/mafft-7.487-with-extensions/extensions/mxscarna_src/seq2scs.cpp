///////////////////////////////////////////////////////////////
// seq2scs.cpp
//
// make SCS(Stem Candidate Sequence) from the profile
//////////////////////////////////////////////////////////////

#include "scarna.hpp"
#include "SafeVector.h"
#include "StemCandidate.hpp"
#include "Sequence.h"
#include "MultiSequence.h"
#include "BPPMatrix.hpp"
#include "nrutil.h"
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <cstring>
#include <stdio.h>
#include <math.h>

using namespace std;
using namespace::MXSCARNA;

// for alipfold
/*
#include "utils.h"
#include "fold_vars.h"
#include "fold.h"
#include "part_func.h"
#include "inverse.h"
#include "RNAstruct.h"
#include "treedist.h"
#include "stringdist.h"
#include "profiledist.h"
#include "alifold.h"
#include "aln_util.h"
#include "dist_vars.h"
*/
double Stacking_Energy[36] ={
 -0.9,-2.1,-1.7,-0.5,-0.9,-1.0,
 -1.8,-2.9,-2.0,-1.2,-1.7,-1.9,
 -2.3,-3.4,-2.9,-1.4,-2.1,-2.1,
 -1.1,-2.1,-1.9,-0.4,-1.0,1.5,
 -1.1,-2.3,-1.8,-0.8,-0.9,-1.1,
 -0.8,-1.4,-1.2,-0.2,-0.5,-0.4 };

static Trimat<float>* makeProfileBPPMatrix(MultiSequence *Sequences, SafeVector<BPPMatrix*> &BPPMatrices);
static int countSCS(MultiSequence *Sequences, Trimat<float>* consBppMat, int BandWidth);
static std::vector<StemCandidate>* makeProfileScs(std::vector<StemCandidate> *pscs, MultiSequence *Sequences, Trimat<float>* consBppMat, int BandWidth);
static void printScs(std::vector<StemCandidate> *pscs);
static std::vector<StemCandidate>* doubleScs(std::vector<StemCandidate> *pscs);
static std::vector<StemCandidate>* findRelations(std::vector<StemCandidate> *pscs);
static std::vector<StemCandidate>* findCorresponding(std::vector<StemCandidate>* pscs);
static std::vector<StemCandidate>* calculateStackingEnergy(std::vector<StemCandidate>* pscs);

//float alipf_fold(char **sequences, char *structure, pair_info **pi);

struct SortCmp {
    bool operator()(const StemCandidate &sc1, const StemCandidate &sc2) const {
#if 0 
// ? Incompatible with Mac
	if      (sc1.GetPosition() > sc2.GetPosition()) return false;
	else if (sc1.GetPosition() < sc2.GetPosition()) return true;
	else if (sc1.GetDistance() > sc2.GetDistance()) return false;
	else return true;
// ? works correctly
	if      (sc1.GetPosition() < sc2.GetPosition()) return true;
	else if (sc1.GetPosition() > sc2.GetPosition()) return false;
	else if (sc1.GetDistance() < sc2.GetDistance()) return true;
	else return false;
// works correctly, but requires c++11
	return std::tie(sc1.GetPosition(),sc1.GetDistance())<std::tie(sc2.GetPosition(),sc2.GetDistance());
// works!
	if      (sc1.GetPosition() > sc2.GetPosition()) return false;
	else if (sc1.GetPosition() < sc2.GetPosition()) return true;
	else if (sc1.GetDistance() > sc2.GetDistance()) return false;
	else if (sc1.GetDistance() < sc2.GetDistance()) return true;
	else return false;
#else // by katoh
	return (sc1.GetPosition() < sc2.GetPosition()) || (sc1.GetPosition() == sc2.GetPosition() && sc1.GetDistance() < sc2.GetDistance() );
#endif
    }
};


vector<StemCandidate>*
seq2scs(MultiSequence *Sequences, SafeVector<BPPMatrix*> &BPPMatrices, int BandWidth)
{

    Trimat<float> *consBppMat = makeProfileBPPMatrix(Sequences, BPPMatrices);

    int numberScs = countSCS(Sequences, consBppMat, BandWidth);

    std::vector<StemCandidate> *pscs = new std::vector<StemCandidate>(); // Profile Stem Candidate Sequence
//    cout << "numberScs=" << numberScs << endl;
    pscs->resize(numberScs+1);

    pscs = makeProfileScs(pscs, Sequences, consBppMat, BandWidth);

    pscs = doubleScs(pscs);

    std::vector<StemCandidate>::iterator startIter = pscs->begin();
    std::vector<StemCandidate>::iterator endIter   = pscs->end();
    ++startIter;
    std::sort(startIter, endIter, SortCmp());
    
//    printScs(pscs);
    pscs = findRelations(pscs);

    pscs = findCorresponding(pscs);

    pscs = calculateStackingEnergy(pscs);

//    findStemRelation()
    
//    exit(1);
    delete consBppMat;

    return pscs;
}

static Trimat<float>*
makeProfileBPPMatrix(MultiSequence *Sequences, SafeVector<BPPMatrix*> &BPPMatrices)
{
    int length = Sequences->GetSequence(0)->GetLength();
//    float thr  = BaseProbThreshold;
    Trimat<float> *consBppMat = new Trimat<float>(length + 1);
    fill(consBppMat->begin(), consBppMat->end(), 0);

// gabage
//    for(int i = 0; i <= length; i++) 
//	for(int j = i; j <= length; j++) 
//	    cout << "i=" << i << " j=" << j << " " << consBppMat->ref(i,j) << endl;
//	    consBppMat->ref(i,j) = 0;


    int number = Sequences->GetNumSequences();
//    if( number == 1) {
    for(int seqNum = 0; seqNum < number; seqNum++) {
	SafeVector<int> *tmpMap = Sequences->GetSequence(seqNum)->GetMappingNumber();
	int label = Sequences->GetSequence(seqNum)->GetLabel();
	BPPMatrix *tmpBppMatrix = BPPMatrices[label];
	    
	for(int i = 1; i <= length ; i++) {
	    int originI = tmpMap->at(i);
	    for(int j = i + 3; j <= length; j++) {
		int originJ = tmpMap->at(j);
		if(originI != 0 && originJ != 0) {
		    float tmpProb = tmpBppMatrix->GetProb(originI, originJ);

//		    if(tmpProb >= thr) {
			consBppMat->ref(i, j) += tmpProb;
//			cout << i << " " << j << " " << consBppMat->ref(i,j) << endl;
//		    }
		}
	    }
	}
    }

	/* compute the mean of base pairing probability  */
    for(int i = 1; i <= length; i++) {
	for(int j = i + 3; j <= length; j++) {
	    consBppMat->ref(i,j) = consBppMat->ref(i,j)/(float)number;
	    //consBppMat->ref(i,j) = std::sqrt[number](consBppMat->ref(i,j));
	    //	    cout << i << " " << j <<  " " << consBppMat->ref(i,j) << endl;
	}
    }


/*
    else {
	char **Seqs;
	Seqs = (char **) malloc(sizeof(char*) * number);

	for(int i = 0; i < number; i++) {
	    Seqs[i] = (char *) malloc(sizeof(char) * (length + 1));
	    for(int j = 1; j <= length; j++) {
		Seqs[i][j-1] = Sequences->GetSequence(i)->GetPosition(j);
	    }
	    Seqs[i][length] = '\0';
	}

	
	char *structure = NULL;
	pair_info *pi;
	    
	alipf_fold(Seqs, structure, &pi, number);

	for(int iter = 0; iter < length; iter++) {
	    if(pi[iter].i == 0) break;
	    consBppMat->ref(pi[iter].i, pi[iter].j) = pi[iter].p;
	}

	for(int i = 0; i < number; i++) {
	    free (Seqs[i]);
	}
	free (Seqs);
	
//	free_alifold_arrays(void);
    }
*/  

    return consBppMat;
}
 
static int 
countSCS(MultiSequence *Sequences, Trimat<float> *consBppMat, int BandWidth)
{

    int length = Sequences->GetSequence(0)->GetLength();
    int Word_Length = scsLength;

    int i, j, k, s;
    int count;
    int sum;

    sum = 0;
    for(k = 1; k <= length; k++) {
        count = 0;

        for(i = k, j = k; i >= 1 && j <= (k + BandWidth - 1) && j <= length; i--, j++) {
	    if(consBppMat->ref(i, j) >= BaseProbThreshold) {
	        count++;
	    }
	    else if(count >= Word_Length) {
	        for(s = 0; s <= count - Word_Length; s++) 
		  sum++;

		count = 0;
	    }
	    else {
	        count = 0;
	    }
	    if(consBppMat->ref(i, j) >= BaseProbThreshold && count >= Word_Length && (i == 1 || j == length || j == (k + BandWidth - 1))) {
	        for(s = 0; s <= count - Word_Length; s++) 
		    sum++;

		count = 0;
	    }
	}
    
	count = 0;
	for(i = k, j = k + 1; i >= 1 && j <= (k + BandWidth - 1) && j <= length; i--, j++) {
  	    if(consBppMat->ref(i, j) >= BaseProbThreshold) {
	        count++;
	    }
	    else if(count >= Word_Length) {
	        for(s = 0; s <= count - Word_Length; s++) sum++;

		count = 0;
	    }
	    else {
	        count = 0;
	    }

	    if(consBppMat->ref(i, j) >= BaseProbThreshold && count >= Word_Length && (i == 1 || j == length || j == (k + BandWidth - 1))) {
	        for(s = 0; s <= count - Word_Length; s++) sum++;

		count = 0;
	    }
	}
    }

    return 2 * sum;
}

static std::vector<StemCandidate>* 
makeProfileScs(std::vector<StemCandidate> *pscs, MultiSequence *Sequences, Trimat<float>* consBppMat, int BandWidth)
{

    int length = Sequences->GetSequence(0)->GetLength();
    int Word_Length = scsLength;
    float Thr = BaseProbThreshold;
    int i, j, k, s, t, m, n, l;
    int count;
    int sum;
    
    sum = 0;
    for(k = 1; k <= length; k++) {
        count = 0;
        for(i = k, j = k; i >= 1 && j <= (k + BandWidth - 1) && j <= length; i--, j++) {
            if(consBppMat->ref(i,j) >= Thr) {
	        count++;
	    }
	    else if(count >= Word_Length) {
	        for(s = 0; s <= count- Word_Length; s++) {
		    sum++;
		    pscs->at(sum).SetLength(Word_Length);
		    pscs->at(sum).SetPosition(i+1+s);
		    pscs->at(sum).SetRvposition(j-count+(count-Word_Length-s));
		    pscs->at(sum).SetDistance((j-count+count-Word_Length-s) - (i+1+s+Word_Length));
		    pscs->at(sum).SetNumSeq(Sequences->GetNumSequences());
		    pscs->at(sum).SetNumSubstr(Sequences->GetNumSequences());
		    pscs->at(sum).SetNumRvstr(Sequences->GetNumSequences());
		    for(m = i + 1 + s, n = j - count + (count-Word_Length-s), l = j - 1 - s, t = 0; n < j-s; m++, n++, l--, t++) {
		        for(int num = 0; num < Sequences->GetNumSequences(); num++) {
			    Sequence *seq = Sequences->GetSequence(num);
//			    cout << num << "; " << m << ":" << seq->GetPosition(m) << " " << n << ":" << seq->GetPosition(n) << endl;
			    pscs->at(sum).AddSubstr(num, seq->GetPosition(m));
			    pscs->at(sum).AddRvstr(num, seq->GetPosition(n));
		        }
			//	    assert(pr[iindx[m]-l] > Thr);
//			cout << "prob=" << consBppMat->ref(m,l) << endl;
			pscs->at(sum).AddScore(consBppMat->ref(m,l));
		        pscs->at(sum).AddBaseScore(consBppMat->ref(m, l));
		    }
		    for(int num = 0; num < Sequences->GetNumSequences(); num++) {
		        pscs->at(sum).AddSubstr(num, '\0'); 
		        pscs->at(sum).AddRvstr(num, '\0');
		    }
		}
		count = 0;
	    }
	    else {
  	        count = 0;
	    }
	    if(consBppMat->ref(i,j) >= Thr && count >= Word_Length && (i == 1 || j == length || j == (k + BandWidth - 1))) {
	        for(s = 0; s <= count- Word_Length; s++) {
		    sum++;
		    pscs->at(sum).SetLength(Word_Length);
		    pscs->at(sum).SetPosition(i+s);
		    pscs->at(sum).SetRvposition(j-count+1+(count-Word_Length-s));
		    pscs->at(sum).SetDistance((j-count+1+count-Word_Length-s) - (i+s+Word_Length));
		    pscs->at(sum).SetNumSeq(Sequences->GetNumSequences());
		    pscs->at(sum).SetNumSubstr(Sequences->GetNumSequences());
		    pscs->at(sum).SetNumRvstr(Sequences->GetNumSequences());
		    for(m = i + s, n = j - count + 1 + (count-Word_Length-s), l = j - s, t = 0; n <= j-s; m++, n++, l--, t++) {
			for(int num = 0; num < Sequences->GetNumSequences(); num++) {
			    Sequence *seq = Sequences->GetSequence(num);
			    pscs->at(sum).AddSubstr(num, seq->GetPosition(m));
			    pscs->at(sum).AddRvstr(num, seq->GetPosition(n));
		        }

			pscs->at(sum).AddScore(consBppMat->ref(m,l));
		        pscs->at(sum).AddBaseScore(consBppMat->ref(m, l));
		    }
		    for(int num = 0; num < Sequences->GetNumSequences(); num++) {
		        pscs->at(sum).AddSubstr(num, '\0'); 
		        pscs->at(sum).AddRvstr(num, '\0');
		    }
		}
		count = 0;
	    }
	}
	count = 0;
	for(i = k, j = k + 1; i >= 1 && j <= (k + BandWidth - 1) && j <= length; i--, j++) {
	    if(consBppMat->ref(i,j) >= Thr) {
	        count++;
	    }
	    else if(count >= Word_Length) {
	        for(s = 0; s <= count- Word_Length; s++) {
		    sum++;
		    pscs->at(sum).SetLength(Word_Length);
		    pscs->at(sum).SetPosition(i+1+s);
		    pscs->at(sum).SetRvposition( j-count+(count-Word_Length-s));
		    pscs->at(sum).SetDistance((j-count+count-Word_Length-s) - (i+1+s+Word_Length));
		    pscs->at(sum).SetNumSeq(Sequences->GetNumSequences());
		    pscs->at(sum).SetNumSubstr(Sequences->GetNumSequences());
		    pscs->at(sum).SetNumRvstr(Sequences->GetNumSequences());
		    for(m = i + 1 + s, n = j - count + (count-Word_Length-s), l = j - 1 - s, t = 0; n < j-s; m++, n++, l--, t++) {
			for(int num = 0; num < Sequences->GetNumSequences(); num++) {
			    Sequence *seq = Sequences->GetSequence(num);
			    pscs->at(sum).AddSubstr(num, seq->GetPosition(m));
			    pscs->at(sum).AddRvstr(num, seq->GetPosition(n));
		        }

			pscs->at(sum).AddScore(consBppMat->ref(m,l));
		        pscs->at(sum).AddBaseScore(consBppMat->ref(m, l));
		    }
		    for(int num = 0; num < Sequences->GetNumSequences(); num++) {
		        pscs->at(sum).AddSubstr(num, '\0'); 
	   	        pscs->at(sum).AddRvstr(num, '\0');
		    }
		}
		count = 0;
	    }
	    else {
	        count = 0;
	    }
	    if(consBppMat->ref(i,j) >= Thr && count >= Word_Length && (i == 1 || j == length || j == (k + BandWidth - 1))) {
	        for(s = 0; s <= count - Word_Length; s++) {
		    sum++;
		    pscs->at(sum).SetLength(Word_Length);
		    pscs->at(sum).SetPosition(i+s);
		    pscs->at(sum).SetRvposition(j-count+1+(count-Word_Length-s));
		    pscs->at(sum).SetDistance((j-count+1+count-Word_Length-s) - (i+s+Word_Length));
//		    pscs->at(sum).SetDistance((j-count+count-Word_Length-s) - (i+1+s+Word_Length));
		    pscs->at(sum).SetNumSeq(Sequences->GetNumSequences());
		    pscs->at(sum).SetNumSubstr(Sequences->GetNumSequences());
		    pscs->at(sum).SetNumRvstr(Sequences->GetNumSequences());
		    for(m = i + s, n = j - count + 1 + (count-Word_Length-s), l = j - s, t=0; n <= j-s; m++, n++, l--, t++) {
		        for(int num = 0; num < Sequences->GetNumSequences(); num++) {
			    Sequence *seq = Sequences->GetSequence(num);
			    pscs->at(sum).AddSubstr(num, seq->GetPosition(m));
			    pscs->at(sum).AddRvstr(num, seq->GetPosition(n));
			}

			pscs->at(sum).AddScore(consBppMat->ref(m,l));
			pscs->at(sum).AddBaseScore(consBppMat->ref(m, l));
		    }
		    for(int num = 0; num < Sequences->GetNumSequences(); num++) {
		        pscs->at(sum).AddSubstr(num, '\0'); 
			pscs->at(sum).AddRvstr(num, '\0');
		    }
		}
		count = 0;
	    }
	}
    }

    return pscs;
}

static std::vector<StemCandidate>* 
doubleScs(std::vector<StemCandidate> *pscs)
{
    int num = pscs->size()/2;
    
    for(int i = 1; i <= num; i++) {
	int latter = num + i;
	//cout << i << " " << latter << endl;
	StemCandidate &tmpScs = pscs->at(i);
	pscs->at(latter).SetLength(tmpScs.GetLength());
	pscs->at(latter).SetPosition(tmpScs.GetRvposition());
	pscs->at(latter).SetRvposition(tmpScs.GetPosition());
	pscs->at(latter).SetDistance(-tmpScs.GetDistance());
	pscs->at(latter).SetNumSeq(tmpScs.GetNumSeq());
	pscs->at(latter).SetNumSubstr(tmpScs.GetNumSeq());
	pscs->at(latter).SetNumRvstr(tmpScs.GetNumSeq());

	pscs->at(latter).SetScore(tmpScs.GetScore());
	for(int num = 0; num < tmpScs.GetNumSeq(); num++) {
	    string tmpSubstr = tmpScs.GetSubstr(num);
	    string tmpRvstr  = tmpScs.GetRvstr(num);

	    for(int k = 0; k < tmpScs.GetLength(); k++) {
		pscs->at(latter).AddSubstr(num, tmpSubstr[k]);
		pscs->at(latter).AddRvstr(num, tmpRvstr[k]);
	    }
	}
	for(int k = 0; k < tmpScs.GetLength(); k++) {
	    pscs->at(latter).AddBaseScore(tmpScs.GetBaseScore(k));
	}
    }
    return pscs;
}


static void 
printScs(std::vector<StemCandidate> *pscs)
{
    int num = pscs->size();
//    std::cout << "size = " << num << endl;
    for(int i = 1; i < num; i++) {
        StemCandidate &sc = pscs->at(i);
	
	std::cout << i << "\t" << sc.GetLength() << "\t" << sc.GetPosition() << "\t" << 
	    sc.GetRvposition() << "\t" << sc.GetDistance() << "\t" << sc.GetNumSeq() << 
	    "\t" << sc.GetScore() << "\t" << sc.GetContPos() << "\t" << sc.GetBeforePos() << 
	    "\t" << sc.GetRvscnumber() << "\t" << sc.GetStacking() << "\t" << sc.GetStemStacking() << 
	    "\t" << sc.GetBaseScore(0) << "\t" << sc.GetBaseScore(1) << endl;
	cout << "substr:" << endl;
	for(int k = 0; k < sc.GetNumSeq(); k++) {
	    cout << k << "\t" << sc.GetSubstr(k) << "\t" << sc.GetRvstr(k) << "\t" << endl;
	}
	
    }

}

static std::vector<StemCandidate>*
findRelations(std::vector<StemCandidate> *pscs)
{
    int num = pscs->size();
    
    for(int i = 1; i < num; i++) {
	int pt = i-1; 
	StemCandidate &sc = pscs->at(i);
	sc.SetContPos(-1);
	while(sc.GetPosition() == pscs->at(pt).GetPosition()) { pt--; }
	
	while((sc.GetPosition() == pscs->at(pt).GetPosition() + 1) && (pt > 0)) {
	    if(sc.GetRvposition() == pscs->at(pt).GetRvposition() - 1) {
		sc.SetContPos(pt);
		break;
	    }
	    --pt;
	}
	while((sc.GetPosition() < pscs->at(pt).GetPosition() + pscs->at(pt).GetLength())&&(pt > 0)) { pt--; }
	sc.SetBeforePos(pt);
    }
    
    return pscs;
}

static std::vector<StemCandidate>*
findCorresponding(std::vector<StemCandidate>* pscs)
{
    int num = pscs->size();
    
    for(int i = 1; i < num; i++) { pscs->at(i).SetRvscnumber(0); }
    
    for(int i = 1; i < num; i++) {
	if(pscs->at(i).GetDistance() > 0) {
	    for(int j = i + 1; j < num; j++) {
		if ( (pscs->at(j).GetPosition() == pscs->at(i).GetRvposition())
		     && (pscs->at(i).GetPosition() == pscs->at(j).GetRvposition()) ) {
		    pscs->at(i).SetRvscnumber(j);
		    pscs->at(j).SetRvscnumber(i);
		    break;
		}
	    }
	}
	if(pscs->at(i).GetRvscnumber() == 0) {
	    std::cerr << "error in findCorresponding" << " i=" << i << endl;
//	    exit(1);
	}
    }

    return pscs;
}

static std::vector<StemCandidate>*
calculateStackingEnergy(std::vector<StemCandidate>* pscs)
{
    int num = pscs->size();
    int wordLength = scsLength;

    for(int i = 1; i < num; i++) {
	StemCandidate &scI = pscs->at(i);
	int j = pscs->at(i).GetContPos();
	
	if(j > 0) {

	    StemCandidate &scJ = pscs->at(j);
	    float stacking = 0;
	    int profNum =  scJ.GetNumSeq();
	    for(int k = 0; k < profNum; k++) {
		string substr = scJ.GetSubstr(k);
		string rvstr  = scJ.GetRvstr(k);
		int index = 0;
		switch(substr[wordLength - 1]) {
		        case 'A': if(     rvstr[0]=='U' ) {index += 0;}
				  else{ index = -1000; }
				  break;
			case 'C': if(     rvstr[0]=='G' ) {index += 6;}
				  else{ index = -1000; }
				  break;
			case 'G': if(     rvstr[0]=='C'){index += 12;}
				  else if(rvstr[0]=='U'){index += 18;}
				  else{ index = -1000; }
				  break;
			case 'U': if(     rvstr[0]=='A'){index += 24;}
				  else if(rvstr[0]=='G'){index += 30;}
				  else{ index = - 1000; }
				  break;
		}
		substr = scI.GetSubstr(k);
		rvstr  = scI.GetRvstr(k);
		switch(substr[wordLength - 1]){
			case 'A': if(     rvstr[0]=='U'){index += 0;}
				  else{ index = -1000; }
				  break;
			case 'C': if(     rvstr[0]=='G'){index += 1;}
				  else{ index = -1000; }
				  break;
			case 'G': if(     rvstr[0]=='C'){index += 2;}
				  else if(rvstr[0]=='U'){index += 3;}
				  else{ index = -1000; }
				  break;
			case 'U': if(     rvstr[0]=='A'){index += 4;}
				  else if(rvstr[0]=='G'){index += 5;}
				  else{ index = -1000; }
				  break;
		}
		if(index > 0) {
		    stacking += Stacking_Energy[index];
		}
	    }
	    scI.SetStacking(stacking/(float)profNum);
	}
	else {
	    scI.SetStacking(1000);
	}
    }

    for(int i = 1; i < num; i++) {
	StemCandidate &sc = pscs->at(i);
	float stemStacking = 0;
	int profNum = sc.GetNumSeq();
	for(int k = 0; k < profNum; k++) {
	    string substr = sc.GetSubstr(k);
	    string rvstr  = sc.GetRvstr(k);
	    for(int j = 0; j < wordLength-1; j++) {
		int index = 0;

		switch(substr[j]) {
		    case 'A': if(   rvstr[wordLength-1-j]=='U'){index += 0;}
		              else{ index = -1000; }
			      break;
		    case 'C': if(   rvstr[wordLength-1-j]=='G'){index += 6;}
		              else{ index = -1000; }
			      break;
		    case 'G': if(   rvstr[wordLength-1-j]=='C'){index += 12;}
                              else if(rvstr[wordLength-1-j]=='U'){index += 18;}
		              else{ index = -1000; }
			      break;
		    case 'U': if(   rvstr[wordLength-1-j]=='A'){index += 24;}
		              else if(rvstr[wordLength-1-j]=='G'){index += 30;}
		              else{ index = -1000; }
			      break;
		}
		switch(substr[j+1]){
		    case 'A': if(   rvstr[wordLength-1-(j+1)]=='U'){index += 0;}
			      else{ index = -1000; }
			      break;
		    case 'C': if(   rvstr[wordLength-1-(j+1)]=='G'){index += 1;}
		              else{ index = -1000; }
			      break;
		    case 'G': if(     rvstr[wordLength-1-(j+1)]=='C'){index += 2;}
		              else if(rvstr[wordLength-1-(j+1)]=='U'){index += 3;}
		              else{ index = -1000; }
			      break;
		    case 'U': if(     rvstr[wordLength-1-(j+1)]=='A'){index += 4;}
		              else if(rvstr[wordLength-1-(j+1)]=='G'){index += 5;}
		              else{ index = -1000; }
			      break;
		}
		if(index > 0) {
		    stemStacking += Stacking_Energy[index];
		}
	    }
	    sc.SetStemStacking(stemStacking/(float)profNum);
	}
    }
    return pscs;
}

