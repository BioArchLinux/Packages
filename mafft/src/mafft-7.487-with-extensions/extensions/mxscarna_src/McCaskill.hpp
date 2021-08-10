
#ifndef MCCAKILL_H
#define MCCAKILL_H

//#define NDEBUG

#include <string>
#include <iostream>
#include "nrutil.h"
#include <cassert>
#include "Util.hpp"
#include "Beta.hpp"

#include "energy_param.hpp" 
//#include "energy_param3.hpp"

#include "ScoreType.hpp"


using namespace std;
using namespace ProbCons;

namespace MXSCARNA {
class McCaskill {
    char *seq;
    int  *numSeq;
    int  n_seq;
    static energy_param  energyParam;

    Trimat<float> a, q1, ab, am, am1, p;
    TriVertMat<float> q1v, abv, amv, am1v, pv;
    Trimat<int> typeMat;
    
    void printExpEnergy();
    void initParameter();
    void Inside();
    void Outside();
    void convertProbability();

    inline float expHairpinEnergy(const int type, const int l, const int i, const int j);
    inline float expLoopEnergy(int u1, int u2, int type, int type2, 
			 int si1, int sj1, int sp1, int sq1);
    
    inline float compQb(int i, int j, int tmpType);
    inline float compQm1(int i, int j, int tmpType);
    inline float compQm(int i, int j);
    inline float compQ1(int i, int j, int tmpType);
    inline float compQ(int i, int j);
    inline float compP(int h, int l, int tmpType);
    inline float compPm(int i, int l);
    inline float compPm1(int i, int l);
    inline float beginStemScore(const int i, const int j) const;
    inline float endStemScore(const int i, const int j) const;
    
    static const float GASCONST;
    static const float T;
    static const int    MAXLOOP;
    static const int    TETRA_ENTH37;
    static const int    NBPAIRS;
    static const int    SCALE;
    static const int    TURN;
    static float *exphairpin;
    static float expninio[5][32];
    static float expdangle5[6][4];
    static float expdangle3[6][4];
    static float expinternalLoop[31];
    static float expbulge[31];
    static char   exptriLoop[2][6];
    static float exptriLoopEnergy[2];
    static char   exptetraLoop[30][7];
    static float exptetraLoopEnergy[30];
    static float expStack[6][6];
    static float expTstackH[6][16];
    static float expTstackI[6][16];
    static float expint11[6][6][16];
    static float expint21[6][6][16][4];
    static float expint22[6][6][16][16];
    static float expMLclosing;
    static float expMLintern[8];
    static float expTermAU;
    static float expMLbase[31];

 public:
    
    McCaskill(int n, const char *mySeq) {
	seq    = new char[n + 1];
	numSeq = new int[n + 1];
	n_seq = 0;

       
	for(int i = 0; i < n; i++) {
	    if     (mySeq[i] == 'a' || mySeq[i] == 'A') { seq[n_seq] = 'A'; numSeq[n_seq] = Beta::A_CODE; n_seq++; }
	    else if(mySeq[i] == 't' || mySeq[i] == 'T' ||
		    mySeq[i] == 'u' || mySeq[i] == 'U') { seq[n_seq] = 'U'; numSeq[n_seq] = Beta::U_CODE; n_seq++; }
	    else if(mySeq[i] == 'g' || mySeq[i] == 'G') { seq[n_seq] = 'G'; numSeq[n_seq] = Beta::G_CODE; n_seq++; }
	    else if(mySeq[i] == 'c' || mySeq[i] == 'C') { seq[n_seq] = 'C'; numSeq[n_seq] = Beta::C_CODE; n_seq++; }
	    else if(mySeq[i] == 'n' || mySeq[i] == 'N') { seq[n_seq] = 'N'; numSeq[n_seq] = Beta::N_CODE; n_seq++; } 
	    else if(mySeq[i] == '.' || mySeq[i] == '-') { seq[n_seq] = '-'; numSeq[n_seq] = Beta::GAP_CODE; n_seq++; }
	    else { seq[n_seq] = mySeq[i]; numSeq[n_seq] = Beta::INVALID_CODE; n_seq++; }
	}
	seq[n_seq] = '\0'; 
	a.Allocator(n_seq);
	q1.Allocator(n_seq);
	ab.Allocator(n_seq);
	am.Allocator(n_seq);
	am1.Allocator(n_seq);
	p.Allocator(n_seq);
	q1v.Allocator(n_seq);
	abv.Allocator(n_seq);
	amv.Allocator(n_seq);
	am1v.Allocator(n_seq);
	pv.Allocator(n_seq);
	typeMat.Allocator(n_seq);

	if(n_seq > 31) {
	  exphairpin = new float[n_seq + 1];
	}
	else {
	  exphairpin = new float[31];
	}

	for(int i = 0; i < n_seq; i++) {
	    for(int j = i; j < n_seq; j++) {
		a.ref(i,j)  = q1.ref(i,j) = IMPOSSIBLE;
		q1v.ref(i,j) = IMPOSSIBLE;
	    }
	}
       
	for(int i = 0; i < n_seq; i++) {
	    a.ref(i,i)   = 0.0; 
	    q1.ref(i,i)  = IMPOSSIBLE;
	    q1v.ref(i,i) = IMPOSSIBLE;
	}

	for(int i = 0; i < n_seq-1; i++) {
	    a.ref(i,i+1)   = 0.0;
	    q1.ref(i,i+1)  = IMPOSSIBLE;
	    q1v.ref(i,i+1) = IMPOSSIBLE;
	}

	for(int i = 0; i < n_seq-2; i++) {
	    a.ref(i,i+2)   = 0.0;
	    q1.ref(i,i+2)  = IMPOSSIBLE;
	    q1v.ref(i,i+2) = IMPOSSIBLE;
	}

	for(int i = 0; i < n_seq-3; i++) {
	    a.ref(i,i+3)   = 0.0;
	    q1.ref(i,i+3)  = IMPOSSIBLE;   
	    q1v.ref(i,i+3) = IMPOSSIBLE;
	    
	}

	for(int i = 0; i < n_seq; i++) {
	    for(int j = i; j < n_seq; j++) {
		ab.ref(i,j)  = am.ref(i,j)  = am1.ref(i,j)  = p.ref(i,j)  = IMPOSSIBLE;
		abv.ref(i,j) = amv.ref(i,j) = am1v.ref(i,j) = pv.ref(i,j) = IMPOSSIBLE;
	    }
	}

	/* the type of base pair */
	/* C <-> G : type 1      */
	/* G <-> C : type 2      */
	/* G <-> U : type 3      */
	/* U <-> G : type 5      */
	/* A <-> U : type 0      */
	/* U <-> A : type 4      */
	/* ? <-> ? : type 6      */
	for(int i = 0; i < n_seq; i++) {
	    for(int j = i; j < n_seq; j++) {
		typeMat.ref(i,j) = Beta::getReducedPairCode(numSeq[i], numSeq[j]);
	    }
	}

    }

    /*------------------------------------------------------------------------*/
    /* dangling ends should never be destabilizing, i.e. expdangle>=1         */
    /* specific heat needs smooth function (2nd derivative)                   */
    /* we use a*(sin(x+b)+1)^2, with a=2/(3*sqrt(3)), b=Pi/6-sqrt(3)/2,       */
    /* in the interval b<x<sqrt(3)/2                                          */
    float SMOOTH(float X) { 
      return ((X)/SCALE<-1.2283697)?0:(((X)/SCALE>0.8660254)?(X):
				  SCALE*0.38490018*(sin((X)/SCALE-0.34242663)+1)*(sin((X)/SCALE-0.34242663)+1));
    }

    ~McCaskill() {
	delete[] seq;
	delete[] numSeq;
	delete[] exphairpin;
    }

    void calcPartitionFunction();
    void printProbMat();

    inline float getProb(const int i, const int j) const { 
	// 0 origin : 0..(n-1)
	return p.ref(i, j);
    }
};
}
#endif // MCCASKILL_H
