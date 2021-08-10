#include <iostream>
#include "McCaskill.hpp"
//#include "energy_param3.hpp"
#include "Util.hpp"
#include <cstring>

namespace MXSCARNA {
energy_param  McCaskill::energyParam;

float *McCaskill::exphairpin;
float McCaskill::expninio[5][32];
float McCaskill::expdangle5[6][4];
float McCaskill::expdangle3[6][4];
float McCaskill::expinternalLoop[31];
float McCaskill::expbulge[31];
char   McCaskill::exptriLoop[2][6];
float McCaskill::exptriLoopEnergy[2];
char   McCaskill::exptetraLoop[30][7];
float McCaskill::exptetraLoopEnergy[30];
float McCaskill::expStack[6][6];
float McCaskill::expTstackH[6][16];
float McCaskill::expTstackI[6][16];
float McCaskill::expint11[6][6][16];
float McCaskill::expint21[6][6][16][4];
float McCaskill::expint22[6][6][16][16];
float McCaskill::expMLclosing;
float McCaskill::expMLintern[8];
float McCaskill::expTermAU;
float McCaskill::expMLbase[31];

const int McCaskill::TURN         = 3;
const float McCaskill::GASCONST   = 1.98717;
const float McCaskill::T          = 37 + 273.15;
const int McCaskill::MAXLOOP      = 30;
const int McCaskill::TETRA_ENTH37 = -400;
const int McCaskill::NBPAIRS      = 7;
const int McCaskill::SCALE        = 10;


void 
McCaskill::
calcPartitionFunction()
{ 
    initParameter();
    Inside();
    Outside();
    convertProbability();

/*
    for(int i = 0; i < n_seq; i++) {
	for(int j = i; j < n_seq; j++) {
	    cout << getProb(i, j) << " ";
	}
	cout << endl;
    }
*/
}

void
McCaskill::
convertProbability()
{
    float *pPointer  = p.getPointer(0, 0);
    float *abPointer = ab.getPointer(0,0);
    for(int i = 0; i < n_seq; i++) {
	for(int j = i; j < n_seq; j++) {
	    *pPointer += *abPointer++;
#if 0
	    *pPointer++ = EXP(*pPointer);
#else
	    *pPointer = EXP(*pPointer); // by katoh
	    pPointer++;
#endif
	}
    }
}

void 
McCaskill::
Inside()
{

    for (int i = n_seq - TURN - 2; i >= 0; i--) {
	float *abPointer   = ab.getPointer(i, i + TURN + 1);
	float *am1Pointer  = am1.getPointer(i, i + TURN + 1);
	float *amPointer   = am.getPointer(i, i + TURN + 1);
	float *q1Pointer   = q1.getPointer(i, i + TURN + 1);
	float *aPointer    = a.getPointer(i, i + TURN + 1);
	int   *typePointer = typeMat.getPointer(i, i+TURN+1);
	for (int j = i + TURN + 1; j < n_seq; j++) {
	    int tmpType   = *typePointer++;
	                    *abPointer++  = compQb(i, j, tmpType);
	    am1v.ref(i,j) = *am1Pointer++ = compQm1(i,j, tmpType);
	    amv.ref(i,j)  = *amPointer++  = compQm(i,j);
	    q1v.ref(i,j)  = *q1Pointer++  = compQ1(i,j, tmpType);
	                    *aPointer++   = compQ(i,j);   
	}
    }
}

inline float
McCaskill::
compQb(int i, int j, int tmpType) 
{

    float tmpAb;
    int type  = tmpType;
    int u     = j - i - 1;
 
    if(Beta::isCanonicalReducedPairCode(type)) {
	// hairpin energy 
	assert(u >= 3);
	tmpAb = expHairpinEnergy(type, u, i + 1, j - 1);
		
	// base pair, bulge, interior loop energy
	for(int h = i + 1; h <= MIN(i + MAXLOOP + 1, j - TURN - 2); h++) {
	    int u1 = h-i-1;
	    int max = MAX(h + TURN + 1, j-1-MAXLOOP+u1);
	    float *abPointer     = ab.getPointer(h, max - 1);
	    const int *typeMatPointer = typeMat.getPointer(h, max);

	    for(int l = max; l < j; l++) {
		int type2 = *typeMatPointer++;
		abPointer++;
		if(!Beta::isCanonicalReducedPairCode(type2)) continue;
		
		assert(h >= 0 && h < n_seq && l >= 0 && l < n_seq);
		type2 = Beta::flipReducedPairCode(type2);
		assert(h-i-1 >= 0); assert(j-l-1 >= 0);
		float loopE = *abPointer;
		loopE += expLoopEnergy(u1, j-l-1, tmpType, type2, i+1, j-1, h-1, l+1); 
		tmpAb = LOG_ADD(tmpAb, loopE);
	    }
	}

	// multi loop
	float tmpQm = IMPOSSIBLE;
	float *amPointer  = am.getPointer(i + 1, j - TURN - 3);
	float *am1Pointer = am1v.getPointer(j-TURN-2, j-1);
	for(int h = j - TURN - 2; h >= i + TURN + 3; h--) {
	    assert(h >= 0 && h < n_seq);
	    float tmpScore = *amPointer--;
	    tmpScore += *am1Pointer--;
	    tmpQm = LOG_ADD(tmpQm, tmpScore);
	}
	tmpQm += expMLclosing + expMLintern[type];
	tmpQm += endStemScore(i, j);
	tmpAb = LOG_ADD(tmpAb, tmpQm);
    }
    else {
	tmpAb = IMPOSSIBLE;
    }
    return tmpAb;
}

//F = a:ML_closing + b:ML_intern*k + c:ML_BASE*u

inline float
McCaskill::
compQm1(int i, int j, int tmpType)
{
    float tmpQm1 = IMPOSSIBLE;

    int l = j;
    if (i + TURN + 1 <= l) {
	int type = typeMat.ref(i,l);
	if(Beta::isCanonicalReducedPairCode(type)) {
	    float tmpScore = ab.ref(i,l);
	    tmpScore += beginStemScore(i, l);
	    //tmpScore += expMLbase[1]*(j-l) + expMLintern[type];
	    tmpScore += expMLintern[tmpType];
	    tmpQm1 = LOG_ADD(tmpQm1, tmpScore);
	}
    }
    if(i < j) {
	tmpQm1 = LOG_ADD(tmpQm1, am1.ref(i,j-1));
    }

    return tmpQm1;
}

inline float
McCaskill::
compQm(int i, int j)
{
    float tmpQm = IMPOSSIBLE;
    float *amPointer  = am.getPointer(i,j-TURN-2);
    float *am1Pointer = am1v.getPointer(j-TURN-1, j);
    for(int h = j - TURN - 1; h >= i ; h--) {
	float tmpScore = 0;
	float tmpAm1 = *am1Pointer--;
	
	tmpScore += tmpAm1;
	tmpQm = LOG_ADD(tmpQm, tmpScore);
	tmpScore = *amPointer--;
	tmpScore += tmpAm1;
	tmpQm = LOG_ADD(tmpQm, tmpScore);
    }

    return tmpQm;
}

inline float
McCaskill::
compQ1(int i, int j, int tmpType)
{
    float tmpQ1 = IMPOSSIBLE;

    if(Beta::isCanonicalReducedPairCode(tmpType)) {
	float tmpScore = ab.ref(i, j);
	tmpScore += beginStemScore(i, j);
	tmpQ1 = LOG_ADD(tmpQ1, tmpScore);
    }
    tmpQ1 = LOG_ADD(tmpQ1, q1.ref(i, j - 1));

    return tmpQ1;
}

inline float
McCaskill::
compQ(int i, int j)
{
    float tmpQ = 0;
    tmpQ = LOG_ADD(tmpQ, q1.ref(i,j));
    
    float *aPointer  = a.getPointer(i,j-TURN-2);
    float *q1Pointer = q1v.getPointer(j-TURN-1, j);
    for(int h = j - TURN - 1; h >= i + 1; h--) {
	float tmpScore  = *aPointer--;
	tmpScore       += *q1Pointer--;
	tmpQ            = LOG_ADD(tmpQ, tmpScore);
    }

    return tmpQ;
}

inline float
McCaskill::
beginStemScore(const int i, const int j) const
{
    float temp = 0;
    int type = typeMat.ref(i,j);

    if(0 < i)                   { temp += expdangle5[type][numSeq[i-1]]; }
    if(j < n_seq-1)             { temp += expdangle3[type][numSeq[j+1]]; }
    if(type != Beta::REDUCED_CG_CODE && type != Beta::REDUCED_GC_CODE)  { temp += expTermAU; }
    return temp;
}

inline float
McCaskill::
endStemScore(const int i, const int j) const
{
    float temp = 0;
    int type = typeMat.ref(i,j);

    type = Beta::flipReducedPairCode(type);

    if(i < n_seq-1)             { temp += expdangle3[type][numSeq[i+1]]; }
    if(j > 0)                   { temp += expdangle5[type][numSeq[j-1]]; }
    if(type != Beta::REDUCED_CG_CODE && type != Beta::REDUCED_GC_CODE)  { temp += expTermAU; }
    return temp;
}

inline float
McCaskill::
compP(int h, int l, int tmpType)
{
    float prob = IMPOSSIBLE;
	    
    int type = tmpType;
    if(Beta::isCanonicalReducedPairCode(type)) {
		
	/* exterior loop */
	float tmp_p = 0;
	tmp_p -= a.ref(0,n_seq-1);
	if(0 < h) { 
	    tmp_p += a.ref(0,h-1);
	}
	if(l < n_seq-1) {
	    tmp_p += a.ref(l+1, n_seq-1);
	}
	tmp_p += beginStemScore(h, l);
	prob = LOG_ADD(prob, tmp_p);

	assert(IMPOSSIBLE <= prob && prob <= 0);

	/* internal loop */
	tmp_p = IMPOSSIBLE;
	int tt = Beta::flipReducedPairCode(tmpType);
	int max = MAX(0,h-MAXLOOP-1);
	for(int i = max; i <= h - 1; i++) {
	    float min = MIN(l+MAXLOOP-h+i+2, n_seq-1);
	    int   *typeMatPointer = typeMat.getPointer(i,l+1);
	    float *pPointer       = p.getPointer(i,l);
	    for(int j = l + 1; j <= min; j++) {
		int type2    = *typeMatPointer++;
		pPointer++;
		if(!Beta::isCanonicalReducedPairCode(type2)) continue;
		assert(i >= 0 && i < n_seq && j >= 0 && j < n_seq);

		float tmpScore  = *pPointer;
		tmpScore       += expLoopEnergy(h-i-1, j-l-1, type2, tt, i+1, j-1, h-1, l+1);
		tmp_p = LOG_ADD(tmp_p, tmpScore);
	    }
	}
	prob = LOG_ADD(prob, tmp_p); 
	assert(IMPOSSIBLE <= prob && prob <= 0);

	/* multi loop */
	tmp_p = IMPOSSIBLE;
	float tmp_begin   = beginStemScore(h, l);
	float *q1Pointer  = q1v.getPointer(0, l);
	float *am1Pointer = am1v.getPointer(0, l);
	float *amPointer  = amv.getPointer(1,h-1);
	for(int i = 0; i <= h-TURN-1; i++) {
	    float tmpq1    = *q1Pointer++;
	    float tmpam    = *amPointer++;
	    float tmpScore = *am1Pointer++;

	    tmpScore += tmpam;
	    tmpScore += tmp_begin;
	    tmpScore += expMLclosing + expMLintern[tt];
	    tmp_p = LOG_ADD(tmp_p, tmpScore);

	    tmpScore  = tmpq1;
	    tmpScore += tmpam;
	    tmpScore += tmp_begin;
	    tmpScore += expMLclosing + expMLintern[tt];
	    tmp_p = LOG_ADD(tmp_p, tmpScore);

	    tmpScore  = tmpq1;
	    tmpScore += tmp_begin;
	    tmpScore += expMLclosing + expMLintern[tt];
	    tmp_p = LOG_ADD(tmp_p, tmpScore);
	}
		
	assert(IMPOSSIBLE <= tmp_p && tmp_p <= 0);
	prob = LOG_ADD(prob, tmp_p); 
	
	tmp_p = IMPOSSIBLE;
	for(int i = h-TURN; i <= h-1; i++) {
	    if(i >= 0) {
		float tmpScore  = q1.ref(i,l);
		tmpScore       += tmp_begin;
		tmpScore       += expMLclosing + expMLintern[tt];
		tmp_p           = LOG_ADD(tmp_p, tmpScore);
	    }
	}
	assert(IMPOSSIBLE <= tmp_p && tmp_p <= 0); 
	prob = LOG_ADD(prob, tmp_p);
    }
    else {
	prob = IMPOSSIBLE;
    }

    return prob;
}

inline float
McCaskill::
compPm(int i, int l)
{
  float tmpPm  = IMPOSSIBLE;

  int *typeMatPointer   = typeMat.getPointer(i,n_seq-1);
  float *pPointer       = p.getPointer(i,n_seq);
  float *amPointer      = am.getPointer(l+1,n_seq-1);
  float *abPointer      = ab.getPointer(i, n_seq);
  for(int j = n_seq - 1; j >= l + TURN + 1; j--) {
      int type = *typeMatPointer--;
      pPointer--;
      amPointer--;
      abPointer--;
      if(Beta::isCanonicalReducedPairCode(type)) {
	  float tmp  = *pPointer;
	  tmp       += *amPointer;
	  tmp       += endStemScore(i, j);
	  tmpPm = LOG_ADD(tmpPm, tmp);
      }
  }
  tmpPm += expMLintern[1];

  return tmpPm;
}

inline float
McCaskill::
compPm1(int i, int l)
{
  float tmpPm1 = IMPOSSIBLE;

  int j =  l + 1;
  if(j <= n_seq-1) {
      int type = typeMat.ref(i,j);
      if(Beta::isCanonicalReducedPairCode(type)) {
	  float tmp = p.ref(i,j);
	  tmp += endStemScore(i, j);
	  tmpPm1 = tmp;
      }
      tmpPm1 += expMLintern[1];
  }
  if(l+1 <= n_seq - 1) {
      tmpPm1 = LOG_ADD(tmpPm1, am1.ref(i, l+1));
  }

  return tmpPm1;
}

void 
McCaskill::
Outside()
{
    for(int h = 0; h <= n_seq - TURN - 2; h++) {
	float *pPointer    = p.getPointer(h, n_seq-1);
	float *q1Pointer   = q1.getPointer(h, n_seq-1);
	float *am1Pointer  = am1.getPointer(h, n_seq-1);
	int   *typePointer = typeMat.getPointer(h, n_seq-1);
	for(int l = n_seq-1; l >= h + TURN + 1; l--) {
	    int tmpType    = *typePointer--;
	    pv.ref(h,l)    = *pPointer--   = compP(h,l,tmpType);
	    q1v.ref(h,l)   = *q1Pointer--  = compPm(h,l);
	    am1v.ref(h,l)  = *am1Pointer-- = compPm1(h,l);

	    assert(p.ref(h,l) <= 0);
	}
    }
}

void
McCaskill::
printProbMat()
{
    int m = 0;
    for(int i = 0; i < n_seq; i++) cout << " " << seq[i];
    cout << endl;
    for(int i = 0; i < n_seq; i++) {
	if(m < n_seq) {
	    cout << seq[m];
	}
	for(int j = 0; j <= i-1; j++) {
	    if(j != i-1) cout << "  ";
	    else         cout << " ";
	}
	if(i != 0 && i != n_seq-1) {
	    cout << "\\";
	}

	for(int j = i; j < n_seq; j++) {
	    if(p.ref(i,j) > 0.01) {

		int type = Beta::getReducedPairCode(numSeq[i], numSeq[j]);
		
		if(!Beta::isCanonicalReducedPairCode(type)) {
		    cout << "\n" << seq[i] << " " << seq[j] << " " << exp(p.ref(i,j)) << endl;
		}

		if(j != n_seq-1) {
		    cout << "* ";
		}
		else {
		    cout << "*";
		}

	    }
	    else {

		if(j != n_seq-1) {
		    cout << "  ";
		}
		else {
		    cout << " ";
		}

	    }

	}
	if(m < n_seq) {
	    cout << seq[m++] << endl;
	}
	if(i == n_seq - 1) cout << endl;
    }
    for(int i = 0; i < n_seq; i++) cout << " " << seq[i];
    cout << endl;
}

void
McCaskill::
initParameter()
{
    float GT;
    float RT_KCAL_MOL = McCaskill::T*McCaskill::GASCONST;
    int len = 31;

    for(int i = 0; i < len; i++) {
	GT = energyParam.getHairpin(i);
	exphairpin[i] = -GT*10/RT_KCAL_MOL;
    }
    
    for (int i = 0; i < len; i++) {
	GT = energyParam.getBulge(i);
	expbulge[i] = -GT*10/RT_KCAL_MOL;
	GT = energyParam.getInternalLoop(i);
	expinternalLoop[i] = -GT*10/RT_KCAL_MOL;
    }
    expinternalLoop[2] = -80*10/RT_KCAL_MOL; /* special case of size 2 interior loops (single mismatch) */
    
    // float lxc = energy_param3::lxc37;
    for (int i = 31; i < n_seq; i++) {
	GT = energyParam.getHairpin(30) + (107.856*LOG((float)i/30));
	exphairpin[i] = -GT*10/RT_KCAL_MOL;
    }

    for(int i = 0; i < 5; i++) {
	GT = energyParam.getNinio(i);
	for(int j = 0; j <= MAXLOOP; j++) {
	    expninio[i][j] = -MIN(energyParam.getMaxNinio(), j*GT)*10/RT_KCAL_MOL;
	}
    }

    for(int i = 0; i < 30; i++) {
      GT = energyParam.getTetraLoopEnergy(i);
      exptetraLoopEnergy[i] = -GT*10/RT_KCAL_MOL;
    }
    
    /*no parameters for Triloop*/
    for(int i = 0; i < 2; i++) {
	GT = 0;
	exptriLoopEnergy[i] = -GT*10/RT_KCAL_MOL;
    }

    GT = energyParam.getMLclosing();
    expMLclosing = -GT*10/RT_KCAL_MOL;

    for(int i = 0; i <= NBPAIRS; i++) {
      GT = energyParam.getMLintern();
      expMLintern[i] = -GT*10/RT_KCAL_MOL;
    }

    expTermAU = -energyParam.getTerminalAU()*10/RT_KCAL_MOL;
    
    GT = energyParam.getMLBase();
    for(int i = 0; i < len; i++) {
	expMLbase[i] = -GT*10*(float)i/RT_KCAL_MOL;
    }

    /*
       if danlges = 0 just set their energy to 0,
       don't let dangle energyies become > 0 (at large temps)
       but make sure go smoothly to 0
    */
    for(int i = 0; i < 6; i++) {
	for(int j =0; j < 4; j++) {
	    GT = energyParam.getDangle5(i,j);
	    expdangle5[i][j] = -GT*10/RT_KCAL_MOL;
	    GT = energyParam.getDangle3(i,j);
	    expdangle3[i][j] = -GT*10/RT_KCAL_MOL;
	}
    }

    /* stacking energies  */
    for(int i = 0; i < 6; i++) {
	for(int j = 0; j < 6; j++) {
	    GT = energyParam.getStack(i,j);
	    expStack[i][j] = -GT*10/RT_KCAL_MOL;
	}
    }

    /* mismatch energies */
    for (int i = 0; i < 6; i++) {
	for (int j = 0; j < 16; j++) {
	  GT = energyParam.getTstackI(i, j);
	  //  cout << i << " " << " " << j << " " << GT << endl;
	  expTstackI[i][j] = -GT*10/RT_KCAL_MOL;
	  GT = energyParam.getTstackH(i, j);
	  expTstackH[i][j] = -GT*10/RT_KCAL_MOL;
	}
    }
    
    /* interior loops of length 2*/
    for(int i = 0; i < 6; i++) {
	for(int j = 0; j < 6; j++) {
	    for(int k = 0; k < 16; k++) {
	      GT = energyParam.getInt11(i, j, k);
	      expint11[i][j][k] = -GT*10/RT_KCAL_MOL;
	    }
	}
    }

    /* interior 2*1 loops */
    for(int i = 0; i < 6; i++) {
	for(int j =0; j < 6; j++) {
	    for(int k = 0; k < 16; k++) {
		for(int l = 0; l < 4; l++) {
		  GT = energyParam.getInt21(i,j,k,l);
		  expint21[i][j][k][l] = -GT*10/RT_KCAL_MOL;
		}
	    }
	}
    }

    /* interior 2*2 loops */
    for (int i = 0; i < 6; i++) {
	for(int j = 0; j < 6; j++) {
	    for(int k = 0; k < 16; k++) {
		for(int l = 0; l < 16; l++) {
		  GT = energyParam.getInt22(i,j,k,l);
		  expint22[i][j][k][l] = -GT*10/RT_KCAL_MOL;
		}
	    }
	}
    }
}


inline float 
McCaskill::
expHairpinEnergy(const int type, const int l, const int i, const int j)
{
  float q;
  int    k;
  
//  assert(l >= 0);
  q = exphairpin[l];

  if(l == 4) {
      char temp_seq[7];

      for(int iter = i - 1; iter < i + 5; iter++) {
	temp_seq[iter - (i-1)] = seq[iter];
      }
      temp_seq[6] = '\0';

      for(k = 0; k < 30; k++) {
	if(strcmp(temp_seq, energyParam.getTetraLoop(k)) == 0) break;
      }
      if(k != 30) {
	q += exptetraLoopEnergy[k];
      }
  }
  if(l == 3) {

      /* no triloop bonus
    char temp_seq[6];
    
    for(int iter = i - 1; iter < i + 4; iter++) {
	temp_seq[iter - (i-1)] = seq[iter];
    }
    temp_seq[6] = '\0';
    for(k = 0; k < 2; k++) {
      if(strcmp(temp_seq, energyParam.getTriLoop(k)) == 0) break;
    }
    if(k != 2) {
      q *= exptriLoopEnergy[k];
    }
      */

    if(type != Beta::REDUCED_CG_CODE && type != Beta::REDUCED_GC_CODE) q += expTermAU;
  }
  else {
      int type2 = Beta::getPairCode(numSeq[i], numSeq[j]);
      q += expTstackH[type][type2];
  }

  return q;
}


inline float 
McCaskill::
expLoopEnergy(int u1, int u2, int type, int type2, 
	      int si1, int sj1, int sp1, int sq1)
{
  float z = 0;

  if((u1 == 0) && (u2 == 0)) { z = expStack[type][type2]; }
  else {
    if((u1 == 0) || (u2 == 0)) {
      int u;
      if(u1 == 0) { u = u2; }
      else        { u = u1; }
      z = expbulge[u];
      if(u1 + u2 == 1) z += expStack[type][type2];
      else {
	if (type  != Beta::REDUCED_CG_CODE && type  != Beta::REDUCED_GC_CODE) z += expTermAU;
	if (type2 != Beta::REDUCED_CG_CODE && type2 != Beta::REDUCED_GC_CODE) z += expTermAU;
      }
    }
    else {
      if(u1 + u2 == 2) {
	  z = expint11[type][type2][Beta::getPairCode(numSeq[si1], numSeq[sj1])];
      }
      else if((u1 == 1) && (u2 == 2)) {
	  z = expint21[type][type2][Beta::getPairCode(numSeq[si1], numSeq[sj1])][numSeq[sq1]];
      }
      else if((u1 == 2) && (u2 == 1)) {
	  z = expint21[type2][type][Beta::getPairCode(numSeq[sq1], numSeq[sp1])][numSeq[si1]];
      }
      else if((u1 == 2) && (u2 == 2)) {
	z = expint22[type][type2][Beta::getPairCode(numSeq[si1], numSeq[sj1])][Beta::getPairCode(numSeq[sp1], numSeq[sq1])];
      }
      else {
	z = expinternalLoop[u1 + u2] +
	    expTstackI[type][Beta::getPairCode(numSeq[si1], numSeq[sj1])]
	     + expTstackI[type2][Beta::getPairCode(numSeq[sq1], numSeq[sp1])];
	z += expninio[2][abs(u1-u2)];
      }
    }
  }

  return z;
}

void
McCaskill::
printExpEnergy()
{
    cout << "exphairpin:" << endl;
    for(int i = 0; i < 31; i++) {
	cout << exphairpin[i] << endl;
    }
    
    cout << "expninio[5][32]:" << endl;
    for(int i = 0; i < 5; i++) {
	for(int j = 0; j < 32; j++) {
	    cout << expninio[i][j] << " ";
	}
	cout << endl;
    }

    cout << "expdangle5[6][4]:" << endl;
    for(int i = 0; i < 6; i++) {
	for(int j = 0; j < 4; j++) {
	    cout << expdangle5[i][j] << " ";
	}
	cout << endl;
    }

    cout << "expdangle3[6][4]:" << endl;
    for(int i = 0; i < 6; i++) {
	for(int j = 0; j < 4; j++) {
	    cout << expdangle3[i][j] << " ";
	}
	cout << endl;
    }

    cout << "expinternalLoop[31]:" << endl;
    for(int i = 0; i < 31; i++) {
	cout << i << ":" << expinternalLoop[i] << endl;
    }
    cout << "expbulge[31]:" << endl;
    for(int i = 0; i < 31; i++) {
	cout << i << ":" << expbulge[i] << endl;
    }
    
    cout << "exptriLoopEnergy[2]:" << endl;
    for(int i = 0; i < 2; i++) {
	cout << i << ":" << exptriLoopEnergy[i] << endl;
    }

    cout << "exptetraLoopEnergy[15]" << endl;
    for(int i = 0; i < 15; i++) {
	cout << i << ":" << exptetraLoopEnergy[i] << endl;
    }
    
    cout << "expStack[6][6]:" << endl;
    for(int i = 0; i < 6; i++) {
	for(int j = 0; j < 6; j++) {
	    cout << expStack[i][j] << " ";
	}
	cout << endl;
    }

    cout << "expTstackH[6][16]:" << endl;
    for(int i = 0; i < 6; i++) {
	for(int j = 0; j < 16; j++) {
	    cout << expTstackH[i][j] << " ";
	}
	cout << endl;
    }

    cout << "expTstackI[6][16]:" << endl;
    for(int i = 0; i < 6; i++) {
	for(int j = 0; j < 16; j++) {
	    cout << expTstackI[i][j] << " ";
	}
	cout << endl;
    }
    
    cout << "expMLclosing=" << expMLclosing << endl;
    cout << "expMLintern:" << endl;
    for(int i = 0; i < 8; i++) {
	cout << expMLintern[i] << " ";
    }
    cout << endl;

    cout << "expMLbase[31]:";
    for(int i = 0; i < 31; i++) {
	cout << i << ":" << expMLbase[i] << endl;
    }

    cout << "expint11[6][6][16]:";
    for(int i = 0; i < 6; i++) {
	for(int j = 0; j < 6; j++) {
	    for(int k = 0; k < 16; k++) {
		cout << expint11[i][j][k] << " ";
	    }
	    cout << endl;
	}
	cout << endl;
    }

    cout << "expint21[6][6][16][4]:" << endl;
    for(int i = 0; i < 6; i++) {
	for(int j = 0; j < 6; j++) {
	    for(int k = 0; k < 16; k++) {
		for(int l = 0; l < 4; l++) {
		    cout << expint21[i][j][k][l] << " ";
		}
		cout << endl;
	    }
	    cout << endl;
	}
	cout << endl;
    }

    
    cout << "expint22[6][6][16][16]:" << endl;
    for(int i = 0; i < 6; i++) {
	for(int j = 0; j < 6; j++) {
	    for(int k = 0; k < 16; k++) {
		for(int l = 0; l < 16; l++) {
		    cout << expint22[i][j][k][l] << " ";
		}
		cout << endl;
	    }
	    cout << endl;
	}
	cout << endl;
    }

}

}
