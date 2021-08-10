//////////////////////////////////////////////////////////////
// BPPMatrix.hpp
//
// save the Base Pairing Probability Matrix for each sequences
//////////////////////////////////////////////////////////////

#ifndef __BBPMatrix_HPP__
#define __BBPMatrix_HPP__

#include <iostream>
#include <string>
#include "SparseMatrix.h"
#include "McCaskill.hpp"
#include "nrutil.h"



using namespace std;

namespace MXSCARNA{
class BPPMatrix {
private:

    int seqLength;       // sequence length;
    float cutOff;        // the threshold of probability 

    BPPMatrix() {}
public:
    SparseMatrix bppMat; // base pairing probability matrix 1-origin(1..seqLength)
    BPPMatrix(int bppmode, const string &seq, int seqLength, float inCutOff = 0.0001) : seqLength (seqLength), cutOff(inCutOff) {
      if( bppmode == 'r' ) // by katoh
        readBPPMatrix(seq);
      else if( bppmode == 'w' )
        setandwriteBPPMatrix(seq);
      else
        setBPPMatrix(seq);
    }
    BPPMatrix(int seqLength, float inCutOff, const Trimat<float> & tmpBppMat) : seqLength(seqLength), cutOff(inCutOff) {
      bppMat.SetSparseMatrix(seqLength, seqLength, tmpBppMat, cutOff);
    }

      
    void setBPPMatrix(const string &seq) {
	const char *tmpSeq = seq.c_str();
	McCaskill mc(seqLength, &tmpSeq[1]);
	mc.calcPartitionFunction();
	Trimat<float> tmpBppMat(seqLength + 1);

	for (int i = 0; i < seqLength; i++) {
	  for(int j = i; j < seqLength; j++) {
	    tmpBppMat.ref(i+1, j+1) = mc.getProb(i,j);
	  }
	}
	bppMat.SetSparseMatrix(seqLength, seqLength, tmpBppMat, cutOff);
    }

    void setandwriteBPPMatrix(const string &seq) { // by katoh
        const char *tmpSeq = seq.c_str();
	McCaskill mc(seqLength, &tmpSeq[1]);
	mc.calcPartitionFunction();
	Trimat<float> tmpBppMat(seqLength + 1);
        float tmpbp;

        fprintf( stdout, ">\n" );
	for (int i = 0; i < seqLength; i++) {
	  for(int j = i; j < seqLength; j++) {
        tmpbp = mc.getProb(i,j);
        if (tmpbp > 0.0001)
          fprintf( stdout, "%d %d %50.40f\n", i, j, tmpbp );
	    tmpBppMat.ref(i+1, j+1) = tmpbp;
	  }
	}
	bppMat.SetSparseMatrix(seqLength, seqLength, tmpBppMat, cutOff);
    }

    void readBPPMatrix(const string &seq) { // by katoh
	const char *tmpSeq = seq.c_str();
	char oneline[1000];
	int onechar;
	float prob;
	int posi, posj;
	static FILE *bppfp = NULL;


	if( bppfp == NULL )
	{
		bppfp = fopen( "_bpp", "r" );
		if( bppfp == NULL )
		{
			fprintf( stderr, "Cannot open _bpp.\n" );
			exit( 1 );
		}
	}

	Trimat<float> tmpBppMat(seqLength + 1);
	fgets( oneline, 999, bppfp );
	if( oneline[0] != '>' )
	{
		fprintf( stderr, "Format error\n" );
		exit( 1 );
	}
	while( 1 )
	{
		onechar = getc( bppfp ); 
		ungetc( onechar, bppfp );
		if( onechar == '>' || onechar == EOF ) break;

		fgets( oneline, 999, bppfp );
		sscanf( oneline, "%d %d %f", &posi, &posj, &prob );
//		fprintf( stderr, "%d %d -> %f\n", posi, posj, prob );
		tmpBppMat.ref(posi+1, posj+1) = prob;
	}

	bppMat.SetSparseMatrix(seqLength, seqLength, tmpBppMat, cutOff);
    }

    float GetProb(int i, int j) {
	return bppMat.GetValue(i,j);
    }

    float GetLength() const {
	return seqLength;
    }
    
    void updateBPPMatrix(const Trimat<float> &inbppMat) {
	bppMat.SetSparseMatrix(seqLength, seqLength, inbppMat, cutOff);
    }
};
}
#endif // __BPPMatrix_HPP__
