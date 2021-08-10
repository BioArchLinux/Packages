//////////////////////////////////////////////////////////////
// GlobalParameters.cpp
// This file include Global Parameters, command line options, etc.
//////////////////////////////////////////////////////////////

#include "scarna.hpp"

// These paramenters are defined in scarna.hpp

float RNA_Match_AA      = RNAMATCHAA;
float RNA_Match_AT      = RNAMATCHAT;
float RNA_Match_AG      = RNAMATCHAG;
float RNA_Match_AC      = RNAMATCHAC;
float RNA_Match_TT      = RNAMATCHTT;
float RNA_Match_TG      = RNAMATCHTG;
float RNA_Match_TC      = RNAMATCHTC;
float RNA_Match_GG      = RNAMATCHGG;
float RNA_Match_GC      = RNAMATCHGC;
float RNA_Match_CC      = RNAMATCHCC;
float RNA_Gap_Penalty   = GAPPENALTY;
float RNA_Gap_Extension = GAPEXTENTIONPENALTY;

int   numIterativeRefinementReps = REFINEMENTREPS;
bool  PostProcessAlignment = false;
int   scsLength = SCSLENGTH;
float BaseProbThreshold = BASEPROBTHRESHOLD;
float BasePairConst     = BASEPAIRCONST;
