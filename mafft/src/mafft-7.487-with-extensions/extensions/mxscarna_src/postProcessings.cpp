//////////////////////////////////////////////////////////////////////////////////////
// postProcessings.cpp
// 
// several post process functions after aligning two profile stem candidate sequences
//////////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include "scarna.hpp"
#include "StemCandidate.hpp"
#include <vector>
#include <string>

using namespace::MXSCARNA;

void printStructure(std::vector<StemCandidate> *pscs1, std::vector<StemCandidate> *pscs2, std::vector<int> *matchPSCS1, std::vector<int> *matchPSCS2);

void removeConflicts(std::vector<StemCandidate> *pscs1, std::vector<StemCandidate> *pscs2, std::vector<int> *matchPSCS1, std::vector<int> *matchPSCS2)
{
    int size = matchPSCS1->size();
    
    std::vector<int> found(size, 0);
    for(int k = size - 1; k >= 0; k--) {
	int i    = matchPSCS1->at(k);
	int i_rv = pscs1->at(i).GetRvscnumber();
	int j    = matchPSCS2->at(k);
	int j_rv = pscs2->at(j).GetRvscnumber();
	
	found[k] = 0;
	
	for(int m = size - 1; m >= 0; m--) {
	    if ( (matchPSCS1->at(m) == i_rv) && (matchPSCS2->at(m) == j_rv) ) {
		found[k] = 1;
		break;
	    }
	}
    }

    int pt = 0;
    for(int k = 0; k < size; k++) {
	matchPSCS1->at(pt) = matchPSCS1->at(k);
	matchPSCS2->at(pt) = matchPSCS2->at(k);
	if(found[k] == 1) ++pt; 
    }

    matchPSCS1->resize(pt);
    matchPSCS2->resize(pt);
    
    //printStructure (pscs1, pscs2, matchPSCS1, matchPSCS2);
}

void printStructure(std::vector<StemCandidate> *pscs1, std::vector<StemCandidate> *pscs2, std::vector<int> *matchPSCS1, std::vector<int> *matchPSCS2)
{
    int size = matchPSCS1->size();
    int len  = WORDLENGTH;
    std::vector<char> structure1(100, '.');
    std::vector<char> structure2(100, '.');
    for(int k = 0; k < size; k++) {
	int i    = matchPSCS1->at(k);
	int pos1 = pscs1->at(i).GetPosition();
	int j    = matchPSCS2->at(k);
	int pos2 = pscs2->at(j).GetPosition();
	for(int l = 0; l < len; l++) {
	    if(pscs1->at(i).GetDistance() > 0) {
		structure1[pos1 + l] = '(';
	    }
	    else {
		structure1[pos1 + l] = ')';
	    }
	
	    if(pscs2->at(j).GetDistance() > 0) {
		structure2[pos2 + l] = '(';
	    }
	    else {
		structure2[pos2 + l] = ')';
	    }
	}
	/*
	std::cout << i << "\t" << pscs1->at(i).GetLength() << "\t" << pscs1->at(i).GetPosition() << "\t" << pscs1->at(i).GetRvposition() << "\t" << pscs1->at(i).GetDistance()  << "\t" << pscs1->at(i).GetContPos() << "\t" << pscs1->at(i).GetBeforePos() << "\t" << pscs1->at(i).GetRvscnumber() << endl;
	*/
    }
    size = structure1.size();
    for(int k = 0; k < size; k++) {
	std::cout << structure1[k];
    }
    std::cout << endl;
    for(int k = 0; k < size; k++) {
	std::cout << structure2[k];
    }
    std::cout << endl;
}

