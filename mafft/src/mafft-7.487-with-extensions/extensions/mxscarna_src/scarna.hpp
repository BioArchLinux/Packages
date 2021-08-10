
#ifndef __SCARNA_HPP__
#define __SCARNA_HPP__

#include <iostream>
using namespace std;

#define WORDLENGTH 2	     /* default word length in stem candidates (SCs) */

#define THR 0.01

// the parameter trained by maximazing the sps of tRNA
//#define MULTISCORE    9.870619  //multiple score with the constant.
//#define MULTIRIBOSUM  0.452626  //multiple ribosum_score with the constant
//#define MULTIPENALTY  24.263776 //multiple penalty with the constatnt
//#define MULTISTACKING 45.208927 //multiple stacking energy with the constatnt

/*
#define MULTIRIBOSUM       1
#define MULTIPENALTY       3.1
#define MULTISCORE         3.7
#define MULTISTACKING      0.1
#define MULTIDELTASCORE    9.4
#define MULTIDELTASTACKING 8.6
*/
/*
#define MULTIRIBOSUM       1
#define MULTIPENALTY       1.1162
#define MULTISCORE         0.53299
#define MULTISTACKING      4.25669
#define MULTIDELTASCORE    1.17805
#define MULTIDELTASTACKING 4.2016
*/
// new CRF DATA 550 sigma 1

#define MULTIRIBOSUM       1
#define MULTIPENALTY       1.82294
#define MULTISCORE         0.250631
#define MULTISTACKING      2.35517
#define MULTIDELTASCORE    1.1781
#define MULTIDELTASTACKING 2.45417

/*
#define MULTIRIBOSUM       1
#define MULTIPENALTY       0.478054
#define MULTISCORE         1.36322
#define MULTISTACKING      4.96635
#define MULTIDELTASCORE    1.14239
#define MULTIDELTASTACKING 7.32992
*/
// CRF DATA 900 sigma 1
/*
#define  MULTIRIBOSUM       1
#define  MULTIPENALTY       2.28364
#define  MULTISCORE         0.00945681
#define  MULTISTACKING      2.25357
#define  MULTIDELTASCORE    1.02201
#define  MULTIDELTASTACKING 2.21293
*/

/*
#define RNAMATCHAA    506159
#define RNAMATCHAT    359916
#define RNAMATCHAG    451319
#define RNAMATCHAC    390720
#define RNAMATCHTT    398658
#define RNAMATCHTG    377069
#define RNAMATCHTC    378456
#define RNAMATCHGG    554695
#define RNAMATCHGC    419950
#define RNAMATCHCC    479030
#define GAPPENALTY    190947
#define GAPEXTENTIONPENALTY -118817
*/
// RIBOSUM 
#define RNAMATCHAA    2.22
#define RNAMATCHAT    -1.39
#define RNAMATCHAG    -1.46
#define RNAMATCHAC    -1.86
#define RNAMATCHTT     1.65
#define RNAMATCHTG    -1.74
#define RNAMATCHTC    -1.05
#define RNAMATCHGG     1.03
#define RNAMATCHGC    -2.48
#define RNAMATCHCC     1.16
//#define GAPPENALTY     9.42   // 3default linear gap penalry in RNA sequence alignment
//#define GAPPENALTY    9.18743
//#define GAPPENALTY     5.00
//#define GAPEXTENTIONPENALTY 7.15
//#define GAPEXTENTIONPENALTY 9.62003

#define GAPPENALTY 5
#define GAPEXTENTIONPENALTY 2.5
//#define GAPPENALTY 8.08875
//#define GAPEXTENTIONPENALTY 3.89655

#define REFINEMENTREPS 0
#define SCSLENGTH 2
#define BASEPROBTHRESHOLD 0.01
#define BASEPAIRCONST 6
#define BANDWIDTH 500
#define USERFOLD false

extern float  RNA_Match_AA;
extern float  RNA_Match_AT;
extern float  RNA_Match_AG;
extern float  RNA_Match_AC;
extern float  RNA_Match_TT;
extern float  RNA_Match_TG;
extern float  RNA_Match_TC;
extern float  RNA_Match_GG;
extern float  RNA_Match_GC;
extern float  RNA_Match_CC;
extern float  RNA_Gap_Penalty;
extern float  RNA_Gap_Extension;

extern int numIterativeRefinementReps;
extern bool PostProcessAlignment;
extern int scsLength;
extern float BaseProbThreshold;
extern float BasePairConst;

#endif /*__SCARNA_HPP__*/

