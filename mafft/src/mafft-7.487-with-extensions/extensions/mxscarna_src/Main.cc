/////////////////////////////////////////////////////////////////
// Main.cc
//
// Main routines for MXSCARNA program.
/////////////////////////////////////////////////////////////////

#include "scarna.hpp"
#include "SafeVector.h"
#include "MultiSequence.h"
#include "Defaults.h"
#include "ScoreType.h"
#include "ProbabilisticModel.h"
#include "EvolutionaryTree.h"
#include "SparseMatrix.h"
#include "BPPMatrix.hpp"
#include "StemCandidate.hpp"
#include "Globaldp.hpp"
#include "nrutil.h"
#include "AlifoldMEA.h"
#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <set>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <iomanip>
#include <fstream>

//#include "RfoldWrapper.hpp"
//static RFOLD::Rfold folder;

using namespace::MXSCARNA;

string parametersInputFilename = "";
string parametersOutputFilename = "no training";
string annotationFilename = "";
string weboutputFileName = "";

ofstream *outputFile;

bool enableTraining = false;
bool enableVerbose = false;
bool enableAllPairs = false;
bool enableAnnotation = false;
bool enableViterbi = false;
bool enableClustalWOutput = false;
bool enableTrainEmissions = false;
bool enableAlignOrder = false;
bool enableWebOutput  = false;
bool enableStockholmOutput = false;
bool enableMXSCARNAOutput = false;
bool enableMcCaskillMEAMode = false;
char bppmode = 's'; // by katoh
int numConsistencyReps = 2;
int numPreTrainingReps = 0;
int numIterativeRefinementReps = 100;
int scsLength = SCSLENGTH;
float cutoff = 0;
float gapOpenPenalty = 0;
float gapContinuePenalty = 0;
float threshhold = 1.0;
float BaseProbThreshold = BASEPROBTHRESHOLD;
float BasePairConst = BASEPAIRCONST;
int   BandWidth = BANDWIDTH;

SafeVector<string> sequenceNames;

VF initDistrib (NumMatrixTypes);
VF gapOpen (2*NumInsertStates);
VF gapExtend (2*NumInsertStates);
VVF emitPairs (256, VF (256, 1e-10));
VF emitSingle (256, 1e-5);

string alphabet = alphabetDefault;

string *ssCons = NULL;

const int MIN_PRETRAINING_REPS = 0;
const int MAX_PRETRAINING_REPS = 20;
const int MIN_CONSISTENCY_REPS = 0;
const int MAX_CONSISTENCY_REPS = 5;
const int MIN_ITERATIVE_REFINEMENT_REPS = 0;
const int MAX_ITERATIVE_REFINEMENT_REPS = 1000;

/////////////////////////////////////////////////////////////////
// Function prototypes
/////////////////////////////////////////////////////////////////

void PrintHeading();
void PrintParameters (const char *message, const VF &initDistrib, const VF &gapOpen,
                      const VF &gapExtend, const VVF &emitPairs, const VF &emitSingle, const char *filename);
MultiSequence *DoAlign (MultiSequence *sequence, const ProbabilisticModel &model, VF &initDistrib, VF &gapOpen, VF &gapExtend, VVF &emitPairs, VF &emitSingle);
SafeVector<string> ParseParams (int argc, char **argv);
void ReadParameters ();
MultiSequence *ComputeFinalAlignment (const TreeNode *tree, MultiSequence *sequences,
                                      const SafeVector<SafeVector<SparseMatrix *> > &sparseMatrices,
                                      const ProbabilisticModel &model,
				      SafeVector<BPPMatrix*> &BPPMatrices);
MultiSequence *AlignAlignments (MultiSequence *align1, MultiSequence *align2,
                                const SafeVector<SafeVector<SparseMatrix *> > &sparseMatrices,
                                const ProbabilisticModel &model, SafeVector<BPPMatrix*> &BPPMatrices, float identity);
SafeVector<SafeVector<SparseMatrix *> > DoRelaxation (MultiSequence *sequences, 
						      SafeVector<SafeVector<SparseMatrix *> > &sparseMatrices);
void Relax (SparseMatrix *matXZ, SparseMatrix *matZY, VF &posterior);
void Relax1 (SparseMatrix *matXZ, SparseMatrix *matZY, VF &posterior);
void DoBasePairProbabilityRelaxation (MultiSequence *sequences, 
				      SafeVector<SafeVector<SparseMatrix *> > &sparseMatrices,
				      SafeVector<BPPMatrix*> &BPPMatrices);
set<int> GetSubtree (const TreeNode *tree);
void TreeBasedBiPartitioning (const SafeVector<SafeVector<SparseMatrix *> > &sparseMatrices,
                              const ProbabilisticModel &model, MultiSequence* &alignment,
                              const TreeNode *tree, SafeVector<BPPMatrix*> &BPPMatrices);
void DoIterativeRefinement (const SafeVector<SafeVector<SparseMatrix *> > &sparseMatrices,
                            const ProbabilisticModel &model, MultiSequence* &alignment);
void WriteAnnotation (MultiSequence *alignment,
		      const SafeVector<SafeVector<SparseMatrix *> > &sparseMatrices);
int ComputeScore (const SafeVector<pair<int, int> > &active, 
		  const SafeVector<SafeVector<SparseMatrix *> > &sparseMatrices);
std::vector<StemCandidate>* seq2scs(MultiSequence *Sequences, SafeVector<BPPMatrix*> &BPPMatrices, int BandWidth);
void removeConflicts(std::vector<StemCandidate> *pscs1, std::vector<StemCandidate> *pscs2, std::vector<int> *matchPSCS1, std::vector<int> *matchPSCS2);

struct prob {
    int i;
    int j;
    float p;
};

/////////////////////////////////////////////////////////////////
// main()
//
// Calls all initialization routines and runs the MXSCARNA
// aligner.
/////////////////////////////////////////////////////////////////


int main (int argc, char **argv){

    // print MXSCARNA heading
    PrintHeading();
  
    // parse program parameters
    sequenceNames = ParseParams (argc, argv);
    ReadParameters();
    PrintParameters ("Using parameter set:", initDistrib, gapOpen, gapExtend, emitPairs, emitSingle, NULL);
  
  // now, we'll process all the files given as input.  If we are given
  // several filenames as input, then we'll load all of those sequences
  // simultaneously, as long as we're not training.  On the other hand,
  // if we are training, then we'll treat each file as a separate
  // training instance

    if (enableMcCaskillMEAMode) {
      MultiSequence *sequences = new MultiSequence(); assert (sequences);
      for (int i = 0; i < (int) sequenceNames.size(); i++){
	  cerr << "Loading sequence file: " << sequenceNames[i] << endl;
	  sequences->LoadMFA (sequenceNames[i], true);
      }

      const int numSeqs = sequences->GetNumSequences();
      SafeVector<BPPMatrix*> BPPMatrices;

      // compute the base pairing matrices for each sequences
      for(int i = 0; i < numSeqs; i++) {
	  Sequence *tmpSeq = sequences->GetSequence(i);
	  string   seq = tmpSeq->GetString();
	  int    n_seq = tmpSeq->GetLength();
	  BPPMatrix *bppmat = new BPPMatrix(bppmode, seq, n_seq); // modified by katoh
	  BPPMatrices.push_back(bppmat);
      }
      if (bppmode=='w') exit( 0 );

      AlifoldMEA alifold(sequences, BPPMatrices, BasePairConst);
      alifold.Run();
      ssCons = alifold.getSScons();

      if (enableStockholmOutput) {
	  sequences->WriteSTOCKHOLM (cout, ssCons);
      }
      else if (enableMXSCARNAOutput){
	  sequences->WriteMXSCARNA (cout, ssCons);
      }
      else {
	  sequences->WriteMFA (cout, ssCons);
      }

      delete sequences;
  }
  // if we are training
  else if (enableTraining){

    // build new model for aligning
    ProbabilisticModel model (initDistrib, gapOpen, gapExtend, emitPairs, emitSingle);

    // prepare to average parameters
    for (int i = 0; i < (int) initDistrib.size(); i++) initDistrib[i] = 0;
    for (int i = 0; i < (int) gapOpen.size(); i++) gapOpen[i] = 0;
    for (int i = 0; i < (int) gapExtend.size(); i++) gapExtend[i] = 0;
    if (enableTrainEmissions){
      for (int i = 0; i < (int) emitPairs.size(); i++)
	for (int j = 0; j < (int) emitPairs[i].size(); j++) emitPairs[i][j] = 0;
      for (int i = 0; i < (int) emitSingle.size(); i++) emitSingle[i] = 0;
    }
   
    // align each file individually
    for (int i = 0; i < (int) sequenceNames.size(); i++){

      VF thisInitDistrib (NumMatrixTypes);
      VF thisGapOpen (2*NumInsertStates);
      VF thisGapExtend (2*NumInsertStates);
      VVF thisEmitPairs (256, VF (256, 1e-10));
      VF thisEmitSingle (256, 1e-5);
      
      // load sequence file
      MultiSequence *sequences = new MultiSequence(); assert (sequences);
      cerr << "Loading sequence file: " << sequenceNames[i] << endl;
      sequences->LoadMFA (sequenceNames[i], true);

      // align sequences
      DoAlign (sequences, model, thisInitDistrib, thisGapOpen, thisGapExtend, thisEmitPairs, thisEmitSingle);

      // add in contribution of the derived parameters
      for (int i = 0; i < (int) initDistrib.size(); i++) initDistrib[i] += thisInitDistrib[i];
      for (int i = 0; i < (int) gapOpen.size(); i++) gapOpen[i] += thisGapOpen[i];
      for (int i = 0; i < (int) gapExtend.size(); i++) gapExtend[i] += thisGapExtend[i];
      if (enableTrainEmissions){
	for (int i = 0; i < (int) emitPairs.size(); i++) 
	  for (int j = 0; j < (int) emitPairs[i].size(); j++) emitPairs[i][j] += thisEmitPairs[i][j];
	for (int i = 0; i < (int) emitSingle.size(); i++) emitSingle[i] += thisEmitSingle[i];
      }

      delete sequences;
    }

    // compute new parameters and print them out
    for (int i = 0; i < (int) initDistrib.size(); i++) initDistrib[i] /= (int) sequenceNames.size();
    for (int i = 0; i < (int) gapOpen.size(); i++) gapOpen[i] /= (int) sequenceNames.size();
    for (int i = 0; i < (int) gapExtend.size(); i++) gapExtend[i] /= (int) sequenceNames.size();
    if (enableTrainEmissions){
      for (int i = 0; i < (int) emitPairs.size(); i++) 
	for (int j = 0; j < (int) emitPairs[i].size(); j++) emitPairs[i][j] /= (int) sequenceNames.size();
      for (int i = 0; i < (int) emitSingle.size(); i++) emitSingle[i] /= sequenceNames.size();
    }
    
    PrintParameters ("Trained parameter set:",
                     initDistrib, gapOpen, gapExtend, emitPairs, emitSingle,
                     parametersOutputFilename.c_str());
  }
  // pass
  // if we are not training, we must simply want to align some sequences
  else {
    // load all files together
    MultiSequence *sequences = new MultiSequence(); assert (sequences);
    for (int i = 0; i < (int) sequenceNames.size(); i++){
      cerr << "Loading sequence file: " << sequenceNames[i] << endl;

      sequences->LoadMFA (sequenceNames[i], true);
    }

    // do all "pre-training" repetitions first
    // NOT execute 
    for (int ct = 0; ct < numPreTrainingReps; ct++){
      enableTraining = true;

      // build new model for aligning
      ProbabilisticModel model (initDistrib, gapOpen, gapExtend, 
                                emitPairs, emitSingle);

      // do initial alignments
      DoAlign (sequences, model, initDistrib, gapOpen, gapExtend, emitPairs, emitSingle);

      // print new parameters
      PrintParameters ("Recomputed parameter set:", initDistrib, gapOpen, gapExtend, emitPairs, emitSingle, NULL);

      enableTraining = false;
    }

    // now, we can perform the alignments and write them out
    if (enableWebOutput) {
	outputFile = new ofstream(weboutputFileName.c_str());
	if (!outputFile) {
	    cerr << "cannot open output file." << weboutputFileName << endl;
	    exit(1);
	}
        *outputFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
	*outputFile << "<result>" << endl;
    }
	MultiSequence *alignment = DoAlign (sequences,
					    ProbabilisticModel (initDistrib, gapOpen, gapExtend,  emitPairs, emitSingle),
					    initDistrib, gapOpen, gapExtend, emitPairs, emitSingle);
    

    if (!enableAllPairs){
	if (enableClustalWOutput) {
	    alignment->WriteALN (cout);
	}
	else if (enableWebOutput) {
	    alignment->WriteWEB (*outputFile, ssCons);
//	    computeStructureWithAlifold ();
	}
	else if (enableStockholmOutput) {
	    alignment->WriteSTOCKHOLM (cout, ssCons);
	}
	else if (enableMXSCARNAOutput) {
	    alignment->WriteMXSCARNA (cout, ssCons);
	}
	else {
	    alignment->WriteMFA (cout, ssCons);
	}
    }

    if (enableWebOutput) {
	*outputFile << "</result>" << endl;
	delete outputFile;
    }
    
    delete ssCons;
    delete alignment;
    delete sequences;
   
  }
}

/////////////////////////////////////////////////////////////////
// PrintHeading()
//
// Prints heading for PROBCONS program.
/////////////////////////////////////////////////////////////////

void PrintHeading (){
  cerr << endl
       << "Multiplex SCARNA"<< endl
       << "version " << VERSION << " - align multiple RNA sequences and print to standard output" << endl
       << "Written by Yasuo Tabei" << endl
       << endl;
}

/////////////////////////////////////////////////////////////////
// PrintParameters()
//
// Prints PROBCONS parameters to STDERR.  If a filename is
// specified, then the parameters are also written to the file.
/////////////////////////////////////////////////////////////////

void PrintParameters (const char *message, const VF &initDistrib, const VF &gapOpen,
                      const VF &gapExtend, const VVF &emitPairs, const VF &emitSingle, const char *filename){

  // print parameters to the screen
  cerr << message << endl
       << "    initDistrib[] = { ";
  for (int i = 0; i < NumMatrixTypes; i++) cerr << setprecision (10) << initDistrib[i] << " ";
  cerr << "}" << endl
       << "        gapOpen[] = { ";
  for (int i = 0; i < NumInsertStates*2; i++) cerr << setprecision (10) << gapOpen[i] << " ";
  cerr << "}" << endl
       << "      gapExtend[] = { ";
  for (int i = 0; i < NumInsertStates*2; i++) cerr << setprecision (10) << gapExtend[i] << " ";
  cerr << "}" << endl
       << endl;

  /*
  for (int i = 0; i < 5; i++){
    for (int j = 0; j <= i; j++){
      cerr << emitPairs[(unsigned char) alphabet[i]][(unsigned char) alphabet[j]] << " ";
    }
    cerr << endl;
    }*/

  // if a file name is specified
  if (filename){

    // attempt to open the file for writing
    FILE *file = fopen (filename, "w");
    if (!file){
      cerr << "ERROR: Unable to write parameter file: " << filename << endl;
      exit (1);
    }

    // if successful, then write the parameters to the file
    for (int i = 0; i < NumMatrixTypes; i++) fprintf (file, "%.10f ", initDistrib[i]); fprintf (file, "\n");
    for (int i = 0; i < 2*NumInsertStates; i++) fprintf (file, "%.10f ", gapOpen[i]); fprintf (file, "\n");
    for (int i = 0; i < 2*NumInsertStates; i++) fprintf (file, "%.10f ", gapExtend[i]); fprintf (file, "\n");
    fprintf (file, "%s\n", alphabet.c_str());
    for (int i = 0; i < (int) alphabet.size(); i++){
      for (int j = 0; j <= i; j++)
	fprintf (file, "%.10f ", emitPairs[(unsigned char) alphabet[i]][(unsigned char) alphabet[j]]);
      fprintf (file, "\n");
    }
    for (int i = 0; i < (int) alphabet.size(); i++)
      fprintf (file, "%.10f ", emitSingle[(unsigned char) alphabet[i]]);
    fprintf (file, "\n");
    fclose (file);
  }
}

/////////////////////////////////////////////////////////////////
// DoAlign()
//
// First computes all pairwise posterior probability matrices.
// Then, computes new parameters if training, or a final
// alignment, otherwise.
/////////////////////////////////////////////////////////////////
MultiSequence *DoAlign (MultiSequence *sequences, const ProbabilisticModel &model, VF &initDistrib, VF &gapOpen, VF &gapExtend, VVF &emitPairs, VF &emitSingle){


  assert (sequences);

  const int numSeqs = sequences->GetNumSequences();
  VVF distances (numSeqs, VF (numSeqs, 0));
  VVF identities (numSeqs, VF (numSeqs, 0));
  SafeVector<SafeVector<SparseMatrix *> > sparseMatrices (numSeqs, SafeVector<SparseMatrix *>(numSeqs, NULL));
  
  SafeVector<BPPMatrix*> BPPMatrices;
  
  for(int i = 0; i < numSeqs; i++) {
    Sequence *tmpSeq = sequences->GetSequence(i);
    string   seq = tmpSeq->GetString();
    int    n_seq = tmpSeq->GetLength();
    BPPMatrix *bppmat = new BPPMatrix(bppmode, seq, n_seq, BASEPROBTHRESHOLD); // modified by katoh
    BPPMatrices.push_back(bppmat);
  }
    
  if (enableTraining){
    // prepare to average parameters
    for (int i = 0; i < (int) initDistrib.size(); i++) initDistrib[i] = 0;
    for (int i = 0; i < (int) gapOpen.size(); i++) gapOpen[i] = 0;
    for (int i = 0; i < (int) gapExtend.size(); i++) gapExtend[i] = 0;
    if (enableTrainEmissions){
      for (int i = 0; i < (int) emitPairs.size(); i++)
	for (int j = 0; j < (int) emitPairs[i].size(); j++) emitPairs[i][j] = 0;
      for (int i = 0; i < (int) emitSingle.size(); i++) emitSingle[i] = 0;
    }
  }

  // skip posterior calculations if we just want to do Viterbi alignments
  if (!enableViterbi){

    // do all pairwise alignments for posterior probability matrices
    for (int a = 0; a < numSeqs-1; a++){
      for (int b = a+1; b < numSeqs; b++){
	Sequence *seq1 = sequences->GetSequence (a); 
	Sequence *seq2 = sequences->GetSequence (b);
	
	// verbose output
	if (enableVerbose)
	  cerr << "Computing posterior matrix: (" << a+1 << ") " << seq1->GetHeader() << " vs. "
	       << "(" << b+1 << ") " << seq2->GetHeader() << " -- ";
	
	// compute forward and backward probabilities
	VF *forward = model.ComputeForwardMatrix (seq1, seq2); assert (forward);
	VF *backward = model.ComputeBackwardMatrix (seq1, seq2); assert (backward);
	
	// if we are training, then we'll simply want to compute the
	// expected counts for each region within the matrix separately;
	// otherwise, we'll need to put all of the regions together and
	// assemble a posterior probability match matrix
	
	// so, if we're training
	if (enableTraining){
	  
	  // compute new parameters
	  VF thisInitDistrib (NumMatrixTypes);
	  VF thisGapOpen (2*NumInsertStates);
	  VF thisGapExtend (2*NumInsertStates);
	  VVF thisEmitPairs (256, VF (256, 1e-10));
	  VF thisEmitSingle (256, 1e-5);
	  
	  model.ComputeNewParameters (seq1, seq2, *forward, *backward, thisInitDistrib, thisGapOpen, thisGapExtend, thisEmitPairs, thisEmitSingle, enableTrainEmissions);

	  // add in contribution of the derived parameters
	  for (int i = 0; i < (int) initDistrib.size(); i++) initDistrib[i] += thisInitDistrib[i];
	  for (int i = 0; i < (int) gapOpen.size(); i++) gapOpen[i] += thisGapOpen[i];
	  for (int i = 0; i < (int) gapExtend.size(); i++) gapExtend[i] += thisGapExtend[i];
	  if (enableTrainEmissions){
	    for (int i = 0; i < (int) emitPairs.size(); i++) 
	      for (int j = 0; j < (int) emitPairs[i].size(); j++) emitPairs[i][j] += thisEmitPairs[i][j];
	    for (int i = 0; i < (int) emitSingle.size(); i++) emitSingle[i] += thisEmitSingle[i];
	  }

	  // let us know that we're done.
	  if (enableVerbose) cerr << "done." << endl;
	}
	// pass
	else {

	  // compute posterior probability matrix
	  VF *posterior = model.ComputePosteriorMatrix (seq1, seq2, *forward, *backward); assert (posterior);

	  // compute sparse representations
	  sparseMatrices[a][b] = new SparseMatrix (seq1->GetLength(), seq2->GetLength(), *posterior);
	  sparseMatrices[b][a] = NULL; 
	  
	  if (!enableAllPairs){
	    // perform the pairwise sequence alignment
	    pair<SafeVector<char> *, float> alignment = model.ComputeAlignment (seq1->GetLength(),
										seq2->GetLength(),
										*posterior);
	    
	    Sequence *tmpSeq1 = seq1->AddGaps (alignment.first, 'X');
	    Sequence *tmpSeq2 = seq2->AddGaps (alignment.first, 'Y');

	    // compute sequence identity for each pair of sequenceses
	    int length = tmpSeq1->GetLength();
	    int matchCount = 0;
	    int misMatchCount = 0;
	    for (int k = 1; k <= length; k++) {
		int ch1 = tmpSeq1->GetPosition(k);
		int ch2 = tmpSeq2->GetPosition(k);
		if (ch1 == ch2 && ch1 != '-' && ch2 != '-') { ++matchCount; }
		else if (ch1 != ch2 && ch1 != '-' && ch2 != '-') { ++misMatchCount; }
	    }

	    identities[a][b] = identities[b][a] = (float)matchCount/(float)(matchCount + misMatchCount);

	    // compute "expected accuracy" distance for evolutionary tree computation
	    float distance = alignment.second / min (seq1->GetLength(), seq2->GetLength());
	    distances[a][b] = distances[b][a] = distance;
	    
	    if (enableVerbose)
	      cerr << setprecision (10) << distance << endl;
	    
	      delete alignment.first;
	  }
	  else {
	    // let us know that we're done.
	    if (enableVerbose) cerr << "done." << endl;
	  }
	  
	  delete posterior;
	}
	
	delete forward;
	delete backward;
      }
    }
  }


  // now average out parameters derived
  if (enableTraining){
    // compute new parameters
    for (int i = 0; i < (int) initDistrib.size(); i++) initDistrib[i] /= numSeqs * (numSeqs - 1) / 2;
    for (int i = 0; i < (int) gapOpen.size(); i++) gapOpen[i] /= numSeqs * (numSeqs - 1) / 2;
    for (int i = 0; i < (int) gapExtend.size(); i++) gapExtend[i] /= numSeqs * (numSeqs - 1) / 2;

    if (enableTrainEmissions){
      for (int i = 0; i < (int) emitPairs.size(); i++)
	for (int j = 0; j < (int) emitPairs[i].size(); j++) emitPairs[i][j] /= numSeqs * (numSeqs - 1) / 2;
      for (int i = 0; i < (int) emitSingle.size(); i++) emitSingle[i] /= numSeqs * (numSeqs - 1) / 2;
    }
  }

  // see if we still want to do some alignments
  else {
      // pass
    if (!enableViterbi){

      // perform the consistency transformation the desired number of times
      for (int r = 0; r < numConsistencyReps; r++){
	SafeVector<SafeVector<SparseMatrix *> > newSparseMatrices = DoRelaxation (sequences, sparseMatrices);

	// now replace the old posterior matrices
	for (int i = 0; i < numSeqs; i++){
	  for (int j = 0; j < numSeqs; j++){
	    delete sparseMatrices[i][j];
	    sparseMatrices[i][j] = newSparseMatrices[i][j];
	  }
	}
      }
      //if (numSeqs > 8) {
      //  for (int r = 0; r < 1; r++) 
      //  DoBasePairProbabilityRelaxation(sequences, sparseMatrices, BPPMatrices);
      //}
    }

    MultiSequence *finalAlignment = NULL;

    if (enableAllPairs){
      for (int a = 0; a < numSeqs-1; a++){
	for (int b = a+1; b < numSeqs; b++){
	  Sequence *seq1 = sequences->GetSequence (a);
	  Sequence *seq2 = sequences->GetSequence (b);
	  
	  if (enableVerbose)
	    cerr << "Performing pairwise alignment: (" << a+1 << ") " << seq1->GetHeader() << " vs. "
		 << "(" << b+1 << ") " << seq2->GetHeader() << " -- ";

	  
	  // perform the pairwise sequence alignment
	  pair<SafeVector<char> *, float> alignment;
	  if (enableViterbi)
	    alignment = model.ComputeViterbiAlignment (seq1, seq2);
	  else {

	    // build posterior matrix
	    VF *posterior = sparseMatrices[a][b]->GetPosterior(); assert (posterior);
	    int length = (seq1->GetLength() + 1) * (seq2->GetLength() + 1);
	    for (int i = 0; i < length; i++) (*posterior)[i] -= cutoff;

	    alignment = model.ComputeAlignment (seq1->GetLength(), seq2->GetLength(), *posterior);
	    delete posterior;
	  }


	  // write pairwise alignments 
	  string name = seq1->GetHeader() + "-" + seq2->GetHeader() + (enableClustalWOutput ? ".aln" : ".fasta");
	  ofstream outfile (name.c_str());
	  
	  MultiSequence *result = new MultiSequence();
	  result->AddSequence (seq1->AddGaps(alignment.first, 'X'));
	  result->AddSequence (seq2->AddGaps(alignment.first, 'Y'));
	  result->WriteMFAseq (outfile); // by katoh
	  
	  outfile.close();
	  
	  delete alignment.first;
	}
      }
	exit( 0 );
    }
    
    // now if we still need to do a final multiple alignment
    else {
      
      if (enableVerbose)
	cerr << endl;
      
      // compute the evolutionary tree
      TreeNode *tree = TreeNode::ComputeTree (distances, identities);
      
      if (enableWebOutput) {
	  *outputFile << "<tree>" << endl;
	  tree->Print (*outputFile, sequences);
	  *outputFile << "</tree>" << endl;
      }
      else {
	  tree->Print (cerr, sequences);
	  cerr << endl;
      }
      // make the final alignment
      finalAlignment = ComputeFinalAlignment (tree, sequences, sparseMatrices, model, BPPMatrices);
      
      // build annotation
      if (enableAnnotation){
	WriteAnnotation (finalAlignment, sparseMatrices);
      }

      delete tree;
    }

    if (!enableViterbi){
      // delete sparse matrices
      for (int a = 0; a < numSeqs-1; a++){
	for (int b = a+1; b < numSeqs; b++){
	  delete sparseMatrices[a][b];
	  delete sparseMatrices[b][a];
	}
      }
    }

    //AlifoldMEA alifold(finalAlignment, BPPMatrices, BasePairConst);
    //alifold.Run();
    //ssCons = alifold.getSScons();

    return finalAlignment;

  }

  return NULL;
}

/////////////////////////////////////////////////////////////////
// GetInteger()
//
// Attempts to parse an integer from the character string given.
// Returns true only if no parsing error occurs.
/////////////////////////////////////////////////////////////////

bool GetInteger (char *data, int *val){
  char *endPtr;
  long int retVal;

  assert (val);

  errno = 0;
  retVal = strtol (data, &endPtr, 0);
  if (retVal == 0 && (errno != 0 || data == endPtr)) return false;
  if (errno != 0 && (retVal == LONG_MAX || retVal == LONG_MIN)) return false;
  if (retVal < (long) INT_MIN || retVal > (long) INT_MAX) return false;
  *val = (int) retVal;
  return true;
}

/////////////////////////////////////////////////////////////////
// GetFloat()
//
// Attempts to parse a float from the character string given.
// Returns true only if no parsing error occurs.
/////////////////////////////////////////////////////////////////

bool GetFloat (char *data, float *val){
  char *endPtr;
  double retVal;

  assert (val);

  errno = 0;
  retVal = strtod (data, &endPtr);
  if (retVal == 0 && (errno != 0 || data == endPtr)) return false;
  if (errno != 0 && (retVal >= 1000000.0 || retVal <= -1000000.0)) return false;
  *val = (float) retVal;
  return true;
}

/////////////////////////////////////////////////////////////////
// ParseParams()
//
// Parse all command-line options.
/////////////////////////////////////////////////////////////////

SafeVector<string> ParseParams (int argc, char **argv){

  if (argc < 2){

    cerr << "MXSCARNA comes with ABSOLUTELY NO WARRANTY.  This is free software, and" << endl
         << "you are welcome to redistribute it under certain conditions.  See the" << endl
         << "file COPYING.txt for details." << endl
         << endl
         << "Usage:" << endl
         << "       mxscarna [OPTION]... [MFAFILE]..." << endl
         << endl
         << "Description:" << endl
         << "       Align sequences in MFAFILE(s) and print result to standard output" << endl
         << endl
         << "       -clustalw" << endl
         << "              use CLUSTALW output format instead of MFA" << endl
	 << endl
	 << "       -stockholm" << endl
         << "              use STOCKHOLM output format instead of MFA" << endl
	 << endl
	 << "       -mxscarna" << endl
         << "              use MXSCARNA output format instead of MFA" << endl
	 << endl
	 << "       -weboutput /<output_path>/<outputfilename>" << endl
	 << "        use web output format" << endl
	 << endl
         << "       -c, --consistency REPS" << endl
         << "              use " << MIN_CONSISTENCY_REPS << " <= REPS <= " << MAX_CONSISTENCY_REPS
         << " (default: " << numConsistencyReps << ") passes of consistency transformation" << endl
         << endl
         << "       -ir, --iterative-refinement REPS" << endl
         << "              use " << MIN_ITERATIVE_REFINEMENT_REPS << " <= REPS <= " << MAX_ITERATIVE_REFINEMENT_REPS
         << " (default: " << numIterativeRefinementReps << ") passes of iterative-refinement" << endl
         << endl
	 << "       -pre, --pre-training REPS" << endl
	 << "              use " << MIN_PRETRAINING_REPS << " <= REPS <= " << MAX_PRETRAINING_REPS
	 << " (default: " << numPreTrainingReps << ") rounds of pretraining" << endl
	 << endl
	 << "       -pairs" << endl
         << "              generate all-pairs pairwise alignments" << endl
         << endl
	 << "       -viterbi" << endl
	 << "              use Viterbi algorithm to generate all pairs (automatically enables -pairs)" << endl
	 << endl
         << "       -v, --verbose" << endl
         << "              report progress while aligning (default: " << (enableVerbose ? "on" : "off") << ")" << endl
         << endl
         << "       -annot FILENAME" << endl
         << "              write annotation for multiple alignment to FILENAME" << endl
         << endl
         << "       -t, --train FILENAME" << endl
         << "              compute EM transition probabilities, store in FILENAME (default: "
         << parametersOutputFilename << ")" << endl
         << endl
         << "       -e, --emissions" << endl
         << "              also reestimate emission probabilities (default: "
         << (enableTrainEmissions ? "on" : "off") << ")" << endl
         << endl
	 << "       -p, --paramfile FILENAME" << endl
	 << "              read parameters from FILENAME (default: "
	 << parametersInputFilename << ")" << endl
	 << endl
	 << "       -a, --alignment-order" << endl
	 << "              print sequences in alignment order rather than input order (default: "
	 << (enableAlignOrder ? "on" : "off") << ")" << endl
	 << endl
	 << "       -s THRESHOLD" << endl
	 << "               the threshold of SCS alignment" << endl
	 << endl
	 << "               In default, for less than " << threshhold << ", the SCS aligment is applied. " << endl
	 << "       -l SCSLENGTH" << endl
	 << "               the length of stem candidates " << SCSLENGTH << endl
	 << endl
	 << "       -b BASEPROBTRHESHHOLD" << endl
	 << "               the threshold of base pairing probability " << BASEPROBTHRESHOLD << endl
	 << endl
	 << "       -m, --mccaskillmea" << endl
	 << "               McCaskill MEA MODE: input the clustalw format file and output the secondary structure predicted by McCaskill MEA" << endl
	 << endl
	 << "       -g BASEPAIRSCORECONST" << endl
	 << "               the control parameter of the prediction of base pairs, default:" << BasePairConst << endl
         << endl
	 << "       -w BANDWIDTH" << endl
	 << "               the control parameter of the distance of stem candidates, default:" << BANDWIDTH << endl
	 << endl; 


    //     	 << "       -go, --gap-open VALUE" << endl
    //     	 << "              gap opening penalty of VALUE <= 0 (default: " << gapOpenPenalty << ")" << endl
    //	 << endl
    //	 << "       -ge, --gap-extension VALUE" << endl
    //	 << "              gap extension penalty of VALUE <= 0 (default: " << gapContinuePenalty << ")" << endl
    //	 << endl
    //         << "       -co, --cutoff CUTOFF" << endl
    //         << "              subtract 0 <= CUTOFF <= 1 (default: " << cutoff << ") from all posterior values before final alignment" << endl
    //         << endl
    
    exit (1);
  }

  SafeVector<string> sequenceNames;
  int tempInt;
  float tempFloat;

  for (int i = 1; i < argc; i++){
    if (argv[i][0] == '-'){

      // training
      if (!strcmp (argv[i], "-t") || !strcmp (argv[i], "--train")){
        enableTraining = true;
        if (i < argc - 1)
          parametersOutputFilename = string (argv[++i]);
        else {
          cerr << "ERROR: Filename expected for option " << argv[i] << endl;
          exit (1);
        }
      }
      
      // emission training
      else if (!strcmp (argv[i], "-e") || !strcmp (argv[i], "--emissions")){
        enableTrainEmissions = true;
      }

      // parameter file
      else if (!strcmp (argv[i], "-p") || !strcmp (argv[i], "--paramfile")){
        if (i < argc - 1)
          parametersInputFilename = string (argv[++i]);
        else {
          cerr << "ERROR: Filename expected for option " << argv[i] << endl;
          exit (1);
        }
      }
      else if (! strcmp (argv[i], "-s")) {
	if (i < argc - 1){
	  if (!GetFloat (argv[++i], &tempFloat)){
            cerr << "ERROR: Invalid floating-point value following option " << argv[i-1] << ": " << argv[i] << endl;
            exit (1);
          }
          else {
            if (tempFloat < 0){
              cerr << "ERROR: For option " << argv[i-1] << ", floating-point value must not be nagative." << endl;
              exit (1);
            }
            else
              threshhold = tempFloat;
          }
        }
        else {
          cerr << "ERROR: Floating-point value expected for option " << argv[i] << endl;
          exit (1);
        }
      }
      
      else if (! strcmp (argv[i], "-l")) {
	  if (i < argc - 1) {
	      if (!GetInteger (argv[++i], &tempInt)){
		  cerr << "ERROR: Invalid integer following option " << argv[i-1] << ": " << argv[i] << endl;
		  exit (1);
	      }
	      else {
		  if (tempInt <= 0 || 6 <= tempInt) {
		      cerr << "ERROR: For option " << argv[i-1] << ", integer must be between "
			   << "1 and 6" << "." << endl;
		      exit (1);
		  }
		  else
		      scsLength = tempInt;
	      }
	  }
      }
      else if (! strcmp (argv[i], "-b")) {
	  if (i < argc - 1) {
	      if (!GetFloat (argv[++i], &tempFloat)){
		  cerr << "ERROR: Invalid floating-point value following option " << argv[i-1] << ": " << argv[i] << endl;
		  exit (1);
	      }
	      else {
		  if (tempFloat < 0 && 1 < tempFloat) {
		      cerr << "ERROR: For option " << argv[i-1] << ", floating-point value must not be nagative." << endl;
		      exit (1);
		  }
		  else
		      BaseProbThreshold = tempFloat;
	      }
	  }
      }
      else if (! strcmp (argv[i], "-g")) {
	  if (i < argc - 1) {
	      if (!GetFloat (argv[++i], &tempFloat)){
		  cerr << "ERROR: Invalid floating-point value following option " << argv[i-1] << ": " << argv[i] << endl;
		  exit (1);
	      }
	      else {
		  if (tempFloat < 0 && 1 < tempFloat) {
		      cerr << "ERROR: For option " << argv[i-1] << ", floating-point value must not be nagative." << endl;
		      exit (1);
		  }
		  else
		      BasePairConst = tempFloat;
	      }
	  }
      }
      
      // number of consistency transformations
      else if (!strcmp (argv[i], "-c") || !strcmp (argv[i], "--consistency")){
        if (i < argc - 1){
          if (!GetInteger (argv[++i], &tempInt)){
            cerr << "ERROR: Invalid integer following option " << argv[i-1] << ": " << argv[i] << endl;
            exit (1);
          }
          else {
            if (tempInt < MIN_CONSISTENCY_REPS || tempInt > MAX_CONSISTENCY_REPS){
              cerr << "ERROR: For option " << argv[i-1] << ", integer must be between "
                   << MIN_CONSISTENCY_REPS << " and " << MAX_CONSISTENCY_REPS << "." << endl;
              exit (1);
            }
            else
              numConsistencyReps = tempInt;
          }
        }
        else {
          cerr << "ERROR: Integer expected for option " << argv[i] << endl;
          exit (1);
        }
      }

      // number of randomized partitioning iterative refinement passes
      else if (!strcmp (argv[i], "-ir") || !strcmp (argv[i], "--iterative-refinement")){
        if (i < argc - 1){
          if (!GetInteger (argv[++i], &tempInt)){
            cerr << "ERROR: Invalid integer following option " << argv[i-1] << ": " << argv[i] << endl;
            exit (1);
          }
          else {
            if (tempInt < MIN_ITERATIVE_REFINEMENT_REPS || tempInt > MAX_ITERATIVE_REFINEMENT_REPS){
              cerr << "ERROR: For option " << argv[i-1] << ", integer must be between "
                   << MIN_ITERATIVE_REFINEMENT_REPS << " and " << MAX_ITERATIVE_REFINEMENT_REPS << "." << endl;
              exit (1);
            }
            else
              numIterativeRefinementReps = tempInt;
          }
        }
        else {
          cerr << "ERROR: Integer expected for option " << argv[i] << endl;
          exit (1);
        }
      }
      // number of EM pre-training rounds
      else if (!strcmp (argv[i], "-pre") || !strcmp (argv[i], "--pre-training")){
        if (i < argc - 1){
          if (!GetInteger (argv[++i], &tempInt)){
            cerr << "ERROR: Invalid integer following option " << argv[i-1] << ": " << argv[i] << endl;
            exit (1);
          }
          else {
            if (tempInt < MIN_PRETRAINING_REPS || tempInt > MAX_PRETRAINING_REPS){
              cerr << "ERROR: For option " << argv[i-1] << ", integer must be between "
                   << MIN_PRETRAINING_REPS << " and " << MAX_PRETRAINING_REPS << "." << endl;
              exit (1);
            }
            else
              numPreTrainingReps = tempInt;
          }
        }
        else {
          cerr << "ERROR: Integer expected for option " << argv[i] << endl;
          exit (1);
        }
      }

      // the distance of stem candidate
      else if (!strcmp (argv[i], "-w")){
        if (i < argc - 1){
          if (!GetInteger (argv[++i], &tempInt)){
            cerr << "ERROR: Invalid integer following option " << argv[i-1] << ": " << argv[i] << endl;
            exit (1);
          }
          else {
              BandWidth = tempInt;
          }
        }
        else {
          cerr << "ERROR: Integer expected for option " << argv[i] << endl;
          exit (1);
        }
      }

      // gap open penalty
      else if (!strcmp (argv[i], "-go") || !strcmp (argv[i], "--gap-open")){
        if (i < argc - 1){
          if (!GetFloat (argv[++i], &tempFloat)){
            cerr << "ERROR: Invalid floating-point value following option " << argv[i-1] << ": " << argv[i] << endl;
            exit (1);
          }
          else {
            if (tempFloat > 0){
              cerr << "ERROR: For option " << argv[i-1] << ", floating-point value must not be positive." << endl;
              exit (1);
            }
            else
              gapOpenPenalty = tempFloat;
          }
        }
        else {
          cerr << "ERROR: Floating-point value expected for option " << argv[i] << endl;
          exit (1);
        }
      }

      // gap extension penalty
      else if (!strcmp (argv[i], "-ge") || !strcmp (argv[i], "--gap-extension")){
        if (i < argc - 1){
          if (!GetFloat (argv[++i], &tempFloat)){
            cerr << "ERROR: Invalid floating-point value following option " << argv[i-1] << ": " << argv[i] << endl;
            exit (1);
          }
          else {
            if (tempFloat > 0){
              cerr << "ERROR: For option " << argv[i-1] << ", floating-point value must not be positive." << endl;
              exit (1);
            }
            else
              gapContinuePenalty = tempFloat;
          }
        }
        else {
          cerr << "ERROR: Floating-point value expected for option " << argv[i] << endl;
          exit (1);
        }
      }

      // all-pairs pairwise alignments
      else if (!strcmp (argv[i], "-pairs")){
        enableAllPairs = true;
      }

      // all-pairs pairwise Viterbi alignments
      else if (!strcmp (argv[i], "-viterbi")){
        enableAllPairs = true;
	enableViterbi = true;
      }

      // read base-pairing probability from the '_bpp' file, by katoh
      else if (!strcmp (argv[i], "-readbpp")){
        bppmode = 'r';
      }

      // write base-pairing probability to stdout, by katoh
      else if (!strcmp (argv[i], "-writebpp")){
        bppmode = 'w';
      }

      // annotation files
      else if (!strcmp (argv[i], "-annot")){
        enableAnnotation = true;
        if (i < argc - 1)
	  annotationFilename = argv[++i];
        else {
          cerr << "ERROR: FILENAME expected for option " << argv[i] << endl;
          exit (1);
        }
      }

      // clustalw output format
      else if (!strcmp (argv[i], "-clustalw")){
	enableClustalWOutput = true;
      }
      // mxscarna output format
      else if (!strcmp (argv[i], "-mxscarna")) {
	  enableMXSCARNAOutput = true;
      }
      // stockholm output format
      else if (!strcmp (argv[i], "-stockholm")) {
	  enableStockholmOutput = true;
      }
      // web output format
      else if (!strcmp (argv[i], "-weboutput")) {
	  if (i < argc - 1) {
	      weboutputFileName = string(argv[++i]);
	  }
	  else {
	      cerr << "ERROR: Invalid following option " << argv[i-1] << ": " << argv[i] << endl;
	      exit (1);
	  }

	  enableWebOutput = true;
      }

      // cutoff
      else if (!strcmp (argv[i], "-co") || !strcmp (argv[i], "--cutoff")){
        if (i < argc - 1){
          if (!GetFloat (argv[++i], &tempFloat)){
            cerr << "ERROR: Invalid floating-point value following option " << argv[i-1] << ": " << argv[i] << endl;
            exit (1);
          }
          else {
            if (tempFloat < 0 || tempFloat > 1){
              cerr << "ERROR: For option " << argv[i-1] << ", floating-point value must be between 0 and 1." << endl;
              exit (1);
            }
            else
              cutoff = tempFloat;
          }
        }
        else {
          cerr << "ERROR: Floating-point value expected for option " << argv[i] << endl;
          exit (1);
        }
      }

      // verbose reporting
      else if (!strcmp (argv[i], "-v") || !strcmp (argv[i], "--verbose")){
        enableVerbose = true;
      }

      // alignment order
      else if (!strcmp (argv[i], "-a") || !strcmp (argv[i], "--alignment-order")){
	enableAlignOrder = true;
      }
      // McCaskill MEA MODE
      else if (!strcmp (argv[i], "-m") || !strcmp (argv[i], "--mccaskillmea")){
        enableMcCaskillMEAMode = true;
      }
      // bad arguments
      else {
        cerr << "ERROR: Unrecognized option: " << argv[i] << endl;
        exit (1);
      }
    }
    else {
      sequenceNames.push_back (string (argv[i]));
    }
  }

  if (enableTrainEmissions && !enableTraining){
    cerr << "ERROR: Training emissions (-e) requires training (-t)" << endl;
    exit (1);
  }

  return sequenceNames;
}

/////////////////////////////////////////////////////////////////
// ReadParameters()
//
// Read initial distribution, transition, and emission
// parameters from a file.
/////////////////////////////////////////////////////////////////

void ReadParameters (){

  ifstream data;

  emitPairs = VVF (256, VF (256, 1e-10));
  emitSingle = VF (256, 1e-5);

  // read initial state distribution and transition parameters
  // pass
  if (parametersInputFilename == string ("")){
    if (NumInsertStates == 1){
      for (int i = 0; i < NumMatrixTypes; i++) initDistrib[i] = initDistrib1Default[i];
      for (int i = 0; i < 2*NumInsertStates; i++) gapOpen[i] = gapOpen1Default[i];
      for (int i = 0; i < 2*NumInsertStates; i++) gapExtend[i] = gapExtend1Default[i];
    }
    else if (NumInsertStates == 2){
      for (int i = 0; i < NumMatrixTypes; i++) initDistrib[i] = initDistrib2Default[i];
      for (int i = 0; i < 2*NumInsertStates; i++) gapOpen[i] = gapOpen2Default[i];
      for (int i = 0; i < 2*NumInsertStates; i++) gapExtend[i] = gapExtend2Default[i];
    }
    else {
      cerr << "ERROR: No default initial distribution/parameter settings exist" << endl
           << "       for " << NumInsertStates << " pairs of insert states.  Use --paramfile." << endl;
      exit (1);
    }

    alphabet = alphabetDefault;

    for (int i = 0; i < (int) alphabet.length(); i++){
      emitSingle[(unsigned char) tolower(alphabet[i])] = emitSingleDefault[i];
      emitSingle[(unsigned char) toupper(alphabet[i])] = emitSingleDefault[i];
      for (int j = 0; j <= i; j++){
	emitPairs[(unsigned char) tolower(alphabet[i])][(unsigned char) tolower(alphabet[j])] = emitPairsDefault[i][j];
	emitPairs[(unsigned char) tolower(alphabet[i])][(unsigned char) toupper(alphabet[j])] = emitPairsDefault[i][j];
	emitPairs[(unsigned char) toupper(alphabet[i])][(unsigned char) tolower(alphabet[j])] = emitPairsDefault[i][j];
	emitPairs[(unsigned char) toupper(alphabet[i])][(unsigned char) toupper(alphabet[j])] = emitPairsDefault[i][j];
	emitPairs[(unsigned char) tolower(alphabet[j])][(unsigned char) tolower(alphabet[i])] = emitPairsDefault[i][j];
	emitPairs[(unsigned char) tolower(alphabet[j])][(unsigned char) toupper(alphabet[i])] = emitPairsDefault[i][j];
	emitPairs[(unsigned char) toupper(alphabet[j])][(unsigned char) tolower(alphabet[i])] = emitPairsDefault[i][j];
	emitPairs[(unsigned char) toupper(alphabet[j])][(unsigned char) toupper(alphabet[i])] = emitPairsDefault[i][j];
      }
    }
  }
  else {
    data.open (parametersInputFilename.c_str());
    if (data.fail()){
      cerr << "ERROR: Unable to read parameter file: " << parametersInputFilename << endl;
      exit (1);
    }
    
    string line[3];
    for (int i = 0; i < 3; i++){
      if (!getline (data, line[i])){
	cerr << "ERROR: Unable to read transition parameters from parameter file: " << parametersInputFilename << endl;
	exit (1);
      }
    }
    istringstream data2;
    data2.clear(); data2.str (line[0]); for (int i = 0; i < NumMatrixTypes; i++) data2 >> initDistrib[i];
    data2.clear(); data2.str (line[1]); for (int i = 0; i < 2*NumInsertStates; i++) data2 >> gapOpen[i];
    data2.clear(); data2.str (line[2]); for (int i = 0; i < 2*NumInsertStates; i++) data2 >> gapExtend[i];

    if (!getline (data, line[0])){
      cerr << "ERROR: Unable to read alphabet from scoring matrix file: " << parametersInputFilename << endl;
      exit (1);
    }
    
    // read alphabet as concatenation of all characters on alphabet line
    alphabet = "";
    string token;
    data2.clear(); data2.str (line[0]); while (data2 >> token) alphabet += token;

    for (int i = 0; i < (int) alphabet.size(); i++){
      for (int j = 0; j <= i; j++){
	float val;
        data >> val;
	emitPairs[(unsigned char) tolower(alphabet[i])][(unsigned char) tolower(alphabet[j])] = val;
	emitPairs[(unsigned char) tolower(alphabet[i])][(unsigned char) toupper(alphabet[j])] = val;
	emitPairs[(unsigned char) toupper(alphabet[i])][(unsigned char) tolower(alphabet[j])] = val;
	emitPairs[(unsigned char) toupper(alphabet[i])][(unsigned char) toupper(alphabet[j])] = val;
	emitPairs[(unsigned char) tolower(alphabet[j])][(unsigned char) tolower(alphabet[i])] = val;
	emitPairs[(unsigned char) tolower(alphabet[j])][(unsigned char) toupper(alphabet[i])] = val;
	emitPairs[(unsigned char) toupper(alphabet[j])][(unsigned char) tolower(alphabet[i])] = val;
	emitPairs[(unsigned char) toupper(alphabet[j])][(unsigned char) toupper(alphabet[i])] = val;
      }
    }

    for (int i = 0; i < (int) alphabet.size(); i++){
      float val;
      data >> val;
      emitSingle[(unsigned char) tolower(alphabet[i])] = val;
      emitSingle[(unsigned char) toupper(alphabet[i])] = val;
    }
    data.close();
  }
}

/////////////////////////////////////////////////////////////////
// ProcessTree()
//
// Process the tree recursively.  Returns the aligned sequences
// corresponding to a node or leaf of the tree.
/////////////////////////////////////////////////////////////////
float ide;
MultiSequence *ProcessTree (const TreeNode *tree, MultiSequence *sequences,
                            const SafeVector<SafeVector<SparseMatrix *> > &sparseMatrices,
                            const ProbabilisticModel &model, SafeVector<BPPMatrix*> &BPPMatrices) {
  MultiSequence *result;

  // check if this is a node of the alignment tree
  if (tree->GetSequenceLabel() == -1){
    MultiSequence *alignLeft = ProcessTree (tree->GetLeftChild(), sequences, sparseMatrices, model, BPPMatrices);
    MultiSequence *alignRight = ProcessTree (tree->GetRightChild(), sequences, sparseMatrices, model, BPPMatrices);

    assert (alignLeft);
    assert (alignRight);
    
    result = AlignAlignments (alignLeft, alignRight, sparseMatrices, model, BPPMatrices, tree->GetIdentity());
    assert (result);

    delete alignLeft;
    delete alignRight;
  }

  // otherwise, this is a leaf of the alignment tree
  else {
    result = new MultiSequence(); assert (result);
    result->AddSequence (sequences->GetSequence(tree->GetSequenceLabel())->Clone());
  }

  return result;
}

/////////////////////////////////////////////////////////////////
// ComputeFinalAlignment()
//
// Compute the final alignment by calling ProcessTree(), then
// performing iterative refinement as needed.
/////////////////////////////////////////////////////////////////

MultiSequence *ComputeFinalAlignment (const TreeNode *tree, MultiSequence *sequences,
                                      const SafeVector<SafeVector<SparseMatrix *> > &sparseMatrices,
                                      const ProbabilisticModel &model, 
				      SafeVector<BPPMatrix*> &BPPMatrices) { 

  MultiSequence *alignment = ProcessTree (tree, sequences, sparseMatrices, model, BPPMatrices);

  if (enableAlignOrder){
    alignment->SaveOrdering();
    enableAlignOrder = false;
  }

  // tree-based refinement 
  // if you use the function, you can degrade the quality of the software.
  // TreeBasedBiPartitioning (sparseMatrices, model, alignment, tree, BPPMatrices);

  // iterative refinement
/*
  for (int i = 0; i < numIterativeRefinementReps; i++)
      DoIterativeRefinement (sparseMatrices, model, alignment);

      cerr << endl;
*/
  // return final alignment
  return alignment;
}

/////////////////////////////////////////////////////////////////
// AlignAlignments()
//
// Returns the alignment of two MultiSequence objects.
/////////////////////////////////////////////////////////////////

MultiSequence *AlignAlignments (MultiSequence *align1, MultiSequence *align2,
                                const SafeVector<SafeVector<SparseMatrix *> > &sparseMatrices,
                                const ProbabilisticModel &model, 
				SafeVector<BPPMatrix*> &BPPMatrices, float identity){

  // print some info about the alignment
  if (enableVerbose){
    for (int i = 0; i < align1->GetNumSequences(); i++)
      cerr << ((i==0) ? "[" : ",") << align1->GetSequence(i)->GetLabel();
    cerr << "] vs. ";
    for (int i = 0; i < align2->GetNumSequences(); i++)
      cerr << ((i==0) ? "[" : ",") << align2->GetSequence(i)->GetLabel();
    cerr << "]: ";
  }

  VF *posterior = model.BuildPosterior (align1, align2, sparseMatrices, cutoff);

  pair<SafeVector<char> *, float> alignment;
  // choose the alignment routine depending on the "cosmetic" gap penalties used
  if (gapOpenPenalty == 0 && gapContinuePenalty == 0) {

    if(identity <= threshhold) {
	std::vector<StemCandidate> *pscs1, *pscs2;
	pscs1 = seq2scs(align1, BPPMatrices, BandWidth);
	pscs2 = seq2scs(align2, BPPMatrices, BandWidth);
	std::vector<int> *matchPSCS1 = new std::vector<int>;
	std::vector<int> *matchPSCS2 = new std::vector<int>;

	Globaldp globaldp(pscs1, pscs2, align1, align2, matchPSCS1, matchPSCS2, posterior, BPPMatrices);
	//float scsScore = globaldp.Run();

	globaldp.Run();

	removeConflicts(pscs1, pscs2, matchPSCS1, matchPSCS2);

	alignment = model.ComputeAlignment2 (align1->GetSequence(0)->GetLength(), align2->GetSequence(0)->GetLength(), *posterior, pscs1, pscs2, matchPSCS1, matchPSCS2);
	delete matchPSCS1;
	delete matchPSCS2;
    } else {
	alignment = model.ComputeAlignment (align1->GetSequence(0)->GetLength(), align2->GetSequence(0)->GetLength(), *posterior);
    }
  }
  else {
    alignment = model.ComputeAlignmentWithGapPenalties (align1, align2,
                                                        *posterior, align1->GetNumSequences(), align2->GetNumSequences(),
                                                        gapOpenPenalty, gapContinuePenalty);
  }

  delete posterior;

  if (enableVerbose){

    // compute total length of sequences
    int totLength = 0;
    for (int i = 0; i < align1->GetNumSequences(); i++)
      for (int j = 0; j < align2->GetNumSequences(); j++)
        totLength += min (align1->GetSequence(i)->GetLength(), align2->GetSequence(j)->GetLength());

    // give an "accuracy" measure for the alignment
    cerr << alignment.second / totLength << endl;
  }

  // now build final alignment
  MultiSequence *result = new MultiSequence();
  for (int i = 0; i < align1->GetNumSequences(); i++)
    result->AddSequence (align1->GetSequence(i)->AddGaps(alignment.first, 'X'));
  for (int i = 0; i < align2->GetNumSequences(); i++)
    result->AddSequence (align2->GetSequence(i)->AddGaps(alignment.first, 'Y'));
  if (!enableAlignOrder)
    result->SortByLabel();

  // free temporary alignment
  delete alignment.first;

  return result;
}

/////////////////////////////////////////////////////////////////
// DoRelaxation()
//
// Performs one round of the consistency transformation.  The
// formula used is:
//                     1
//    P'(x[i]-y[j]) = ---  sum   sum P(x[i]-z[k]) P(z[k]-y[j])
//                    |S| z in S  k
//
// where S = {x, y, all other sequences...}
//
/////////////////////////////////////////////////////////////////

SafeVector<SafeVector<SparseMatrix *> > DoRelaxation (MultiSequence *sequences, 
						      SafeVector<SafeVector<SparseMatrix *> > &sparseMatrices){
  const int numSeqs = sequences->GetNumSequences();

  SafeVector<SafeVector<SparseMatrix *> > newSparseMatrices (numSeqs, SafeVector<SparseMatrix *>(numSeqs, NULL));

  // for every pair of sequences
  for (int i = 0; i < numSeqs; i++){
    for (int j = i+1; j < numSeqs; j++){
      Sequence *seq1 = sequences->GetSequence (i);
      Sequence *seq2 = sequences->GetSequence (j);
      
      if (enableVerbose)
        cerr << "Relaxing (" << i+1 << ") " << seq1->GetHeader() << " vs. "
             << "(" << j+1 << ") " << seq2->GetHeader() << ": ";

      // get the original posterior matrix
      VF *posteriorPtr = sparseMatrices[i][j]->GetPosterior(); assert (posteriorPtr);
      VF &posterior = *posteriorPtr;

      const int seq1Length = seq1->GetLength();
      const int seq2Length = seq2->GetLength();

      // contribution from the summation where z = x and z = y
      for (int k = 0; k < (seq1Length+1) * (seq2Length+1); k++) posterior[k] += posterior[k];

      if (enableVerbose)
        cerr << sparseMatrices[i][j]->GetNumCells() << " --> ";

      // contribution from all other sequences
      for (int k = 0; k < numSeqs; k++) if (k != i && k != j){
	if (k < i)
	  Relax1 (sparseMatrices[k][i], sparseMatrices[k][j], posterior);
	else if (k > i && k < j)
	  Relax (sparseMatrices[i][k], sparseMatrices[k][j], posterior);
	else {
	  SparseMatrix *temp = sparseMatrices[j][k]->ComputeTranspose();
	  Relax (sparseMatrices[i][k], temp, posterior);
	  delete temp;
	}
      }

      // now renormalization
      for (int k = 0; k < (seq1Length+1) * (seq2Length+1); k++) posterior[k] /= numSeqs;

      // mask out positions not originally in the posterior matrix
      SparseMatrix *matXY = sparseMatrices[i][j];
      for (int y = 0; y <= seq2Length; y++) posterior[y] = 0;
      for (int x = 1; x <= seq1Length; x++){
	SafeVector<PIF>::iterator XYptr = matXY->GetRowPtr(x);
	SafeVector<PIF>::iterator XYend = XYptr + matXY->GetRowSize(x);
	VF::iterator base = posterior.begin() + x * (seq2Length + 1);
	int curr = 0;
	while (XYptr != XYend){

	  // zero out all cells until the first filled column
	  while (curr < XYptr->first){
	    base[curr] = 0;
	    curr++;
	  }

	  // now, skip over this column
	  curr++;
	  ++XYptr;
	}
	
	// zero out cells after last column
	while (curr <= seq2Length){
	  base[curr] = 0;
	  curr++;
	}
      }

      // save the new posterior matrix
      newSparseMatrices[i][j] = new SparseMatrix (seq1->GetLength(), seq2->GetLength(), posterior);
      newSparseMatrices[j][i] = NULL;

      if (enableVerbose)
        cerr << newSparseMatrices[i][j]->GetNumCells() << " -- ";

      delete posteriorPtr;

      if (enableVerbose)
        cerr << "done." << endl;
    }
  }
  
  return newSparseMatrices;
}

/////////////////////////////////////////////////////////////////
// Relax()
//
// Computes the consistency transformation for a single sequence
// z, and adds the transformed matrix to "posterior".
/////////////////////////////////////////////////////////////////

void Relax (SparseMatrix *matXZ, SparseMatrix *matZY, VF &posterior){

  assert (matXZ);
  assert (matZY);

  int lengthX = matXZ->GetSeq1Length();
  int lengthY = matZY->GetSeq2Length();
  assert (matXZ->GetSeq2Length() == matZY->GetSeq1Length());

  // for every x[i]
  for (int i = 1; i <= lengthX; i++){
    SafeVector<PIF>::iterator XZptr = matXZ->GetRowPtr(i);
    SafeVector<PIF>::iterator XZend = XZptr + matXZ->GetRowSize(i);

    VF::iterator base = posterior.begin() + i * (lengthY + 1);

    // iterate through all x[i]-z[k]
    while (XZptr != XZend){
      SafeVector<PIF>::iterator ZYptr = matZY->GetRowPtr(XZptr->first);
      SafeVector<PIF>::iterator ZYend = ZYptr + matZY->GetRowSize(XZptr->first);
      const float XZval = XZptr->second;

      // iterate through all z[k]-y[j]
      while (ZYptr != ZYend){
        base[ZYptr->first] += XZval * ZYptr->second;
       ZYptr++;
      }
      XZptr++;
    }
  }
}

/////////////////////////////////////////////////////////////////
// Relax1()
//
// Computes the consistency transformation for a single sequence
// z, and adds the transformed matrix to "posterior".
/////////////////////////////////////////////////////////////////

void Relax1 (SparseMatrix *matZX, SparseMatrix *matZY, VF &posterior){

  assert (matZX);
  assert (matZY);

  int lengthZ = matZX->GetSeq1Length();
  int lengthY = matZY->GetSeq2Length();

  // for every z[k]
  for (int k = 1; k <= lengthZ; k++){
    SafeVector<PIF>::iterator ZXptr = matZX->GetRowPtr(k);
    SafeVector<PIF>::iterator ZXend = ZXptr + matZX->GetRowSize(k);

    // iterate through all z[k]-x[i]
    while (ZXptr != ZXend){
      SafeVector<PIF>::iterator ZYptr = matZY->GetRowPtr(k);
      SafeVector<PIF>::iterator ZYend = ZYptr + matZY->GetRowSize(k);
      const float ZXval = ZXptr->second;
      VF::iterator base = posterior.begin() + ZXptr->first * (lengthY + 1);

      // iterate through all z[k]-y[j]
      while (ZYptr != ZYend){
        base[ZYptr->first] += ZXval * ZYptr->second;
        ZYptr++;
      }
      ZXptr++;
    }
  }
}

void DoBasePairProbabilityRelaxation (MultiSequence *sequences, 
				      SafeVector<SafeVector<SparseMatrix *> > &sparseMatrices,
				      SafeVector<BPPMatrix*> &BPPMatrices) {
    const int numSeqs = sequences->GetNumSequences();

    for (int i = 0; i < numSeqs; i++) {
	Sequence *seq1 = sequences->GetSequence (i);
	BPPMatrix *seq1BppMatrix = BPPMatrices[seq1->GetLabel()];
	Trimat<float> consBppMat(seq1->GetLength() + 1);
	int seq1Length = seq1->GetLength();

	for (int k = 1; k <= seq1Length; k++) {
	    for (int l = k; l <= seq1Length; l++) {
		consBppMat.ref(k, l) = seq1BppMatrix->GetProb(k, l);
	    }
	}

	for (int j = i + 1; j < numSeqs; j++) {

   //		VF *posteriorPtr = sparseMatrices[i][j]->GetPosterior()
	  Sequence *seq2 = sequences->GetSequence (j);
	  BPPMatrix *seq2BppMatrix = BPPMatrices[seq2->GetLabel()];
//	  int seq2Length = seq2->GetLength();
	  SparseMatrix *matchProb = sparseMatrices[i][j];

//	  vector<PIF2> &probs1 = seq1BppMatrix->bppMat.data2;
	  for(int k = 1; k <= seq1Length; k++) {
	      for(int m = k, n = k; n <= k + 200 && m >= 1 && n <= seq1Length; m--, n++) {
		  
//	  for (int k = 0; k < (int)probs1.size(); k++) {
//	      float tmpProb1 = probs1[k].prob;
//	      int   tmp1I    = probs1[k].i;
//	      int   tmp1J    = probs1[k].j;

		  float sumProb = 0;
		  vector<PIF2> &probs2 = seq2BppMatrix->bppMat.data2;
		  for(int l = 0; l < (int)probs2.size(); l++) {
		      float tmpProb2 = probs2[l].prob;
		      int   tmp2I    = probs2[l].i;
		      int   tmp2J    = probs2[l].j;
		      sumProb += matchProb->GetValue(m, tmp2I)*matchProb->GetValue(n, tmp2J)*tmpProb2;
		  }

		  consBppMat.ref(m, n) += sumProb;
	      }

	      for(int m = k, n = k + 1; n <= k + 200 && m >= 1 && n <= seq1Length; m--, n++) {
		  
//	  for (int k = 0; k < (int)probs1.size(); k++) {
//	      float tmpProb1 = probs1[k].prob;
//	      int   tmp1I    = probs1[k].i;
//	      int   tmp1J    = probs1[k].j;

		  float sumProb = 0;
		  vector<PIF2> &probs2 = seq2BppMatrix->bppMat.data2;
		  for(int l = 0; l < (int)probs2.size(); l++) {
		      float tmpProb2 = probs2[l].prob;
		      int   tmp2I    = probs2[l].i;
		      int   tmp2J    = probs2[l].j;
		      sumProb += matchProb->GetValue(m, tmp2I)*matchProb->GetValue(n, tmp2J)*tmpProb2;
		  }

		  consBppMat.ref(m, n) += sumProb;
	      }
	  }
	}

/*
	  for(int k = 1; k <= seq1Length; k++) {
	      for(int m = k, n = k; n <= k + 30 && m >= 1 && n <= seq1Length; m--, n++) {
		  float tmpProb = seq1BppMatrix->GetProb(m, n);
		  for(int l = 1; l <= seq2Length; l++) {
		      for(int s = l, t = l; t <= l + 30 && s >= 1 && t <= seq2Length; s--, t++) {
			  tmpProb += matchProb->GetValue(m,s)*matchProb->GetValue(n,t)*seq2BppMatrix->GetProb(s,t);
		      }
		      for(int s = l, t = l + 1; t <= l + 31 && s >= 1 && t <= seq2Length; s--, t++) {
			  tmpProb += matchProb->GetValue(m,s)*matchProb->GetValue(n,t)*seq2BppMatrix->GetProb(s,t);
		      }
		  }
		  consBppMat.ref(m, n) += tmpProb;
	      }
    
	      for(int m = k, n = k + 1; n <= k + 31 && m >= 1 && n <= seq1Length; m--, n++) {
		  float tmpProb = seq1BppMatrix->GetProb(m, n);
		  for(int l = 1; l <= seq2Length; l++) {
		      for(int s = l, t = l; t <= l + 30 && s >= 1 && t <= seq2Length; s--, t++) {
			  tmpProb += matchProb->GetValue(m,s)*matchProb->GetValue(n,t)*seq2BppMatrix->GetProb(s,t);
		      }
		      for(int s = l, t = l + 1; t <= l + 31 && s >= 1 && t <= seq2Length; s--, t++) {
			  tmpProb += matchProb->GetValue(m,s)*matchProb->GetValue(n,t)*seq2BppMatrix->GetProb(s,t);
		      }
		  }
		  consBppMat.ref(m,n) += tmpProb;
	      }
	  }
	}
*/
	for (int m = 1; m <= seq1Length; m++) {
	    for (int n = m + 4; n <= seq1Length; n++) {
		consBppMat.ref(m,n) = consBppMat.ref(m,n)/(float)numSeqs;
	    }
	}
	seq1BppMatrix->updateBPPMatrix(consBppMat);
    }
}

/////////////////////////////////////////////////////////////////
// GetSubtree
//
// Returns set containing all leaf labels of the current subtree.
/////////////////////////////////////////////////////////////////

set<int> GetSubtree (const TreeNode *tree){
  set<int> s;

  if (tree->GetSequenceLabel() == -1){
    s = GetSubtree (tree->GetLeftChild());
    set<int> t = GetSubtree (tree->GetRightChild());

    for (set<int>::iterator iter = t.begin(); iter != t.end(); ++iter)
      s.insert (*iter);
  }
  else {
    s.insert (tree->GetSequenceLabel());
  }

  return s;
}

/////////////////////////////////////////////////////////////////
// TreeBasedBiPartitioning
//
// Uses the iterative refinement scheme from MUSCLE.
/////////////////////////////////////////////////////////////////

void TreeBasedBiPartitioning (const SafeVector<SafeVector<SparseMatrix *> > &sparseMatrices,
                              const ProbabilisticModel &model, MultiSequence* &alignment,
                              const TreeNode *tree, SafeVector<BPPMatrix*> &BPPMatrices){
  // check if this is a node of the alignment tree
  if (tree->GetSequenceLabel() == -1){
    TreeBasedBiPartitioning (sparseMatrices, model, alignment, tree->GetLeftChild(), BPPMatrices);
    TreeBasedBiPartitioning (sparseMatrices, model, alignment, tree->GetRightChild(), BPPMatrices);

    set<int> leftSubtree = GetSubtree (tree->GetLeftChild());
    set<int> rightSubtree = GetSubtree (tree->GetRightChild());
    set<int> leftSubtreeComplement, rightSubtreeComplement;

    // calculate complement of each subtree
    for (int i = 0; i < alignment->GetNumSequences(); i++){
      if (leftSubtree.find(i) == leftSubtree.end()) leftSubtreeComplement.insert (i);
      if (rightSubtree.find(i) == rightSubtree.end()) rightSubtreeComplement.insert (i);
    }

    // perform realignments for edge to left child
    if (!leftSubtree.empty() && !leftSubtreeComplement.empty()){
      MultiSequence *groupOneSeqs = alignment->Project (leftSubtree); assert (groupOneSeqs);
      MultiSequence *groupTwoSeqs = alignment->Project (leftSubtreeComplement); assert (groupTwoSeqs);
      delete alignment;
      alignment = AlignAlignments (groupOneSeqs, groupTwoSeqs, sparseMatrices, model, BPPMatrices, tree->GetLeftChild()->GetIdentity());
    }

    // perform realignments for edge to right child
    if (!rightSubtree.empty() && !rightSubtreeComplement.empty()){
      MultiSequence *groupOneSeqs = alignment->Project (rightSubtree); assert (groupOneSeqs);
      MultiSequence *groupTwoSeqs = alignment->Project (rightSubtreeComplement); assert (groupTwoSeqs);
      delete alignment;
      alignment = AlignAlignments (groupOneSeqs, groupTwoSeqs, sparseMatrices, model, BPPMatrices, tree->GetRightChild()->GetIdentity());
    }
  }
}

/////////////////////////////////////////////////////////////////
// DoterativeRefinement()
//
// Performs a single round of randomized partionining iterative
// refinement.
/////////////////////////////////////////////////////////////////
/*
void DoIterativeRefinement (const SafeVector<SafeVector<SparseMatrix *> > &sparseMatrices,
                            const ProbabilisticModel &model, MultiSequence* &alignment){
  set<int> groupOne, groupTwo;

  // create two separate groups
  for (int i = 0; i < alignment->GetNumSequences(); i++){
    if (rand() % 2)
      groupOne.insert (i);
    else
      groupTwo.insert (i);
  }

  if (groupOne.empty() || groupTwo.empty()) return;

  // project into the two groups
  MultiSequence *groupOneSeqs = alignment->Project (groupOne); assert (groupOneSeqs);
  MultiSequence *groupTwoSeqs = alignment->Project (groupTwo); assert (groupTwoSeqs);
  delete alignment;

  // realign
  alignment = AlignAlignments (groupOneSeqs, groupTwoSeqs, sparseMatrices, model);

  delete groupOneSeqs;
  delete groupTwoSeqs;
}
*/

/////////////////////////////////////////////////////////////////
// WriteAnnotation()
//
// Computes annotation for multiple alignment and write values
// to a file.
/////////////////////////////////////////////////////////////////

void WriteAnnotation (MultiSequence *alignment, 
		      const SafeVector<SafeVector<SparseMatrix *> > &sparseMatrices){
  ofstream outfile (annotationFilename.c_str());
  
  if (outfile.fail()){
    cerr << "ERROR: Unable to write annotation file." << endl;
    exit (1);
  }

  const int alignLength = alignment->GetSequence(0)->GetLength();
  const int numSeqs = alignment->GetNumSequences();
  
  SafeVector<int> position (numSeqs, 0);
  SafeVector<SafeVector<char>::iterator> seqs (numSeqs);
  for (int i = 0; i < numSeqs; i++) seqs[i] = alignment->GetSequence(i)->GetDataPtr();
  SafeVector<pair<int,int> > active;
  active.reserve (numSeqs);
  
  // for every column
  for (int i = 1; i <= alignLength; i++){
    
    // find all aligned residues in this particular column
    active.clear();
    for (int j = 0; j < numSeqs; j++){
      if (seqs[j][i] != '-'){
	active.push_back (make_pair(j, ++position[j]));
      }
    }
    
    outfile << setw(4) << ComputeScore (active, sparseMatrices) << endl;
  }
  
  outfile.close();
}

/////////////////////////////////////////////////////////////////
// ComputeScore()
//
// Computes the annotation score for a particular column.
/////////////////////////////////////////////////////////////////

int ComputeScore (const SafeVector<pair<int, int> > &active, 
		  const SafeVector<SafeVector<SparseMatrix *> > &sparseMatrices){

  if (active.size() <= 1) return 0;
  
  // ALTERNATIVE #1: Compute the average alignment score.

  float val = 0;
  for (int i = 0; i < (int) active.size(); i++){
    for (int j = i+1; j < (int) active.size(); j++){
      val += sparseMatrices[active[i].first][active[j].first]->GetValue(active[i].second, active[j].second);
    }
  }

  return (int) (200 * val / ((int) active.size() * ((int) active.size() - 1)));
  
}
