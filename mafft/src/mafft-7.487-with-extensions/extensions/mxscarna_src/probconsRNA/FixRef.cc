/////////////////////////////////////////////////////////////////
// Main.cc
/////////////////////////////////////////////////////////////////

#include "SafeVector.h"
#include "MultiSequence.h"
#include "Defaults.h"
#include "ScoreType.h"
#include "ProbabilisticModel.h"
#include "EvolutionaryTree.h"
#include "SparseMatrix.h"
#include <string>
#include <iomanip>
#include <iostream>
#include <list>
#include <set>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <iomanip>

string matrixFilename = "";
string parametersInputFilename = "";
string parametersOutputFilename = "no training";

bool enableTraining = false;
bool enableVerbose = false;
int numConsistencyReps = 2;
int numPreTrainingReps = 0;
int numIterativeRefinementReps = 100;

float gapOpenPenalty = 0;
float gapContinuePenalty = 0;
VF initDistrib (NumMatrixTypes);
VF gapOpen (2*NumInsertStates);
VF gapExtend (2*NumInsertStates);
SafeVector<char> alphabet;
VVF emitPairs;
VF emitSingle;

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
                      const VF &gapExtend, const char *filename);
MultiSequence *DoAlign (MultiSequence *sequence, const ProbabilisticModel &model);
SafeVector<string> ParseParams (int argc, char **argv);
void ReadParameters ();
MultiSequence *ComputeFinalAlignment (const TreeNode *tree, MultiSequence *sequences,
                                      const SafeVector<SafeVector<SparseMatrix *> > &sparseMatrices,
                                      const ProbabilisticModel &model);
MultiSequence *AlignAlignments (MultiSequence *align1, MultiSequence *align2,
                                const SafeVector<SafeVector<SparseMatrix *> > &sparseMatrices,
                                const ProbabilisticModel &model);
void DoRelaxation (MultiSequence *sequences, SafeVector<SafeVector<SparseMatrix *> > &sparseMatrices);
void Relax (SparseMatrix *matXZ, SparseMatrix *matZY, VF &posterior);
void DoIterativeRefinement (const SafeVector<SafeVector<SparseMatrix *> > &sparseMatrices,
                            const ProbabilisticModel &model, MultiSequence* &alignment);
//float ScoreAlignment (MultiSequence *alignment, MultiSequence *sequences, SparseMatrix **sparseMatrices, const int numSeqs);

/////////////////////////////////////////////////////////////////
// main()
//
// Calls all initialization routines and runs the PROBCONS
// aligner.
/////////////////////////////////////////////////////////////////

int main (int argc, char **argv){

  if (argc != 3){
    cerr << "Usage: FixRef inputfile reffile" << endl;
    exit (1);
  }

  string inputFilename = string (argv[1]);
  string refFilename = string (argv[2]);

  ReadParameters();

  // build new model for aligning
  ProbabilisticModel model (initDistrib, gapOpen, gapExtend, 
                            alphabet, emitPairs, emitSingle);

  MultiSequence *inputSeq = new MultiSequence(); inputSeq->LoadMFA (inputFilename);
  MultiSequence *refSeq = new MultiSequence(); refSeq->LoadMFA (refFilename);

  SafeVector<char> *ali = new SafeVector<char>;

  if (refSeq->GetNumSequences() != 2){
    cerr << "ERROR: Expected two sequences in reference alignment." << endl;
    exit (1);
  }
  set<int> s; s.insert (0);
  MultiSequence *ref1 = refSeq->Project (s);
  s.clear(); s.insert (1);
  MultiSequence *ref2 = refSeq->Project (s);

  for (int i = 0; i < inputSeq->GetNumSequences(); i++){
    if (inputSeq->GetSequence(i)->GetHeader() == ref1->GetSequence(0)->GetHeader()){
      ref1->AddSequence (inputSeq->GetSequence(i)->Clone());
    }
    if (inputSeq->GetSequence(i)->GetHeader() == ref2->GetSequence(0)->GetHeader())
      ref2->AddSequence (inputSeq->GetSequence(i)->Clone());
  }
  if (ref1->GetNumSequences() != 2){
    cerr << "ERROR: Expected two sequences in reference1 alignment." << endl;
    exit (1);
  }
  if (ref2->GetNumSequences() != 2){
    cerr << "ERROR: Expected two sequences in reference2 alignment." << endl;
    exit (1);
  }

  ref1->GetSequence(0)->SetLabel(0);
  ref2->GetSequence(0)->SetLabel(0);
  ref1->GetSequence(1)->SetLabel(1);
  ref2->GetSequence(1)->SetLabel(1);

  //  cerr << "Aligning..." << endl;

  // now, we can perform the alignments and write them out
  MultiSequence *alignment1 = DoAlign (ref1,
                                       ProbabilisticModel (initDistrib, gapOpen, gapExtend, 
                                                           alphabet, emitPairs, emitSingle));

  //cerr << "Aligning second..." << endl;
  MultiSequence *alignment2 = DoAlign (ref2,
                                       ProbabilisticModel (initDistrib, gapOpen, gapExtend, 
                                                           alphabet, emitPairs, emitSingle));

  SafeVector<char>::iterator iter1 = alignment1->GetSequence(0)->GetDataPtr();
  SafeVector<char>::iterator iter2 = alignment1->GetSequence(1)->GetDataPtr();
  for (int i = 1; i <= alignment1->GetSequence(0)->GetLength(); i++){
    if (islower(iter1[i])) iter2[i] = tolower(iter2[i]);
    if (isupper(iter1[i])) iter2[i] = toupper(iter2[i]);
  }
  iter1 = alignment2->GetSequence(0)->GetDataPtr();
  iter2 = alignment2->GetSequence(1)->GetDataPtr();
  for (int i = 1; i <= alignment2->GetSequence(0)->GetLength(); i++){
    if (islower(iter1[i])) iter2[i] = tolower(iter2[i]);
    if (isupper(iter1[i])) iter2[i] = toupper(iter2[i]);
  }
  //alignment1->WriteMFA (cout);
  //alignment2->WriteMFA (cout);

  int a1 = 0, a = 0;
  int b1 = 0, b = 0;

  for (int i = 1; i <= refSeq->GetSequence(0)->GetLength(); i++){

    // catch up in filler sequences
    if (refSeq->GetSequence(0)->GetPosition(i) != '-'){
      while (true){
        a++;
        if (alignment1->GetSequence(0)->GetPosition(a) != '-') break;
        ali->push_back ('X');
      }
    }
    if (refSeq->GetSequence(1)->GetPosition(i) != '-'){
      while (true){
        b++;
        if (alignment2->GetSequence(0)->GetPosition(b) != '-') break;
        ali->push_back ('Y');
      }
    }

    if (refSeq->GetSequence(0)->GetPosition(i) != '-' &&
        refSeq->GetSequence(1)->GetPosition(i) != '-'){
      //cerr << "M: " << refSeq->GetSequence(0)->GetPosition(i) << refSeq->GetSequence(1)->GetPosition(i) << endl;
      ali->push_back ('B');
    }
    else if (refSeq->GetSequence(0)->GetPosition(i) != '-'){
      //cerr << "X" << endl;
      ali->push_back ('X');
    }
    else if (refSeq->GetSequence(1)->GetPosition(i) != '-'){
      //cerr << "Y" << endl;
      ali->push_back ('Y');
    }
  }

  while (a < alignment1->GetSequence(0)->GetLength()){
    a++;
    ali->push_back ('X');
    if (alignment1->GetSequence(0)->GetPosition(a) != '-') a1++;
  }
  while (b < alignment2->GetSequence(0)->GetLength()){
    b++;
    ali->push_back ('Y');
    if (alignment2->GetSequence(0)->GetPosition(b) != '-') b1++;
  }

  Sequence *seq1 = alignment1->GetSequence(1)->AddGaps (ali, 'X');
  Sequence *seq2 = alignment2->GetSequence(1)->AddGaps (ali, 'Y');
  seq1->WriteMFA (cout, 60);
  seq2->WriteMFA (cout, 60);

  delete seq1;
  delete seq2;

  delete ali;
  delete alignment1;
  delete alignment2;
  delete inputSeq;
  delete refSeq;
}

/////////////////////////////////////////////////////////////////
// PrintHeading()
//
// Prints heading for PROBCONS program.
/////////////////////////////////////////////////////////////////

void PrintHeading (){
  cerr << endl
       << "PROBCONS version 1.02 - align multiple protein sequences and print to standard output" << endl
       << "Copyright (C) 2004  Chuong Ba Do" << endl
       << endl;
}

/////////////////////////////////////////////////////////////////
// PrintParameters()
//
// Prints PROBCONS parameters to STDERR.  If a filename is
// specified, then the parameters are also written to the file.
/////////////////////////////////////////////////////////////////

void PrintParameters (const char *message, const VF &initDistrib, const VF &gapOpen,
                      const VF &gapExtend, const char *filename){

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

MultiSequence *DoAlign (MultiSequence *sequences, const ProbabilisticModel &model){

  assert (sequences);

  const int numSeqs = sequences->GetNumSequences();
  VVF distances (numSeqs, VF (numSeqs, 0));
  SafeVector<SafeVector<SparseMatrix *> > sparseMatrices (numSeqs, SafeVector<SparseMatrix *>(numSeqs, NULL));

  // do all pairwise alignments
  for (int a = 0; a < numSeqs-1; a++){
    for (int b = a+1; b < numSeqs; b++){
      Sequence *seq1 = sequences->GetSequence (a);
      Sequence *seq2 = sequences->GetSequence (b);

      // verbose output
      if (enableVerbose)
        cerr << "(" << a+1 << ") " << seq1->GetHeader() << " vs. "
             << "(" << b+1 << ") " << seq2->GetHeader() << ": ";

      // compute forward and backward probabilities
      VF *forward = model.ComputeForwardMatrix (seq1, seq2); assert (forward);
      VF *backward = model.ComputeBackwardMatrix (seq1, seq2); assert (backward);

      // if we are training, then we'll simply want to compute the
      // expected counts for each region within the matrix separately;
      // otherwise, we'll need to put all of the regions together and
      // assemble a posterior probability match matrix

      // compute posterior probability matrix
      VF *posterior = model.ComputePosteriorMatrix (seq1, seq2, *forward, *backward); assert (posterior);

      // compute "expected accuracy" distance for evolutionary tree computation
      pair<SafeVector<char> *, float> alignment = model.ComputeAlignment (seq1->GetLength(),
                                                                          seq2->GetLength(),
                                                                          *posterior);

      float distance = alignment.second / min (seq1->GetLength(), seq2->GetLength());

      if (enableVerbose)
        cerr << setprecision (10) << distance << endl;

      // save posterior probability matrices in sparse format
      distances[a][b] = distances[b][a] = distance;
      sparseMatrices[a][b] = new SparseMatrix (seq1->GetLength(), seq2->GetLength(), *posterior);
      sparseMatrices[b][a] = sparseMatrices[a][b]->ComputeTranspose();

      delete alignment.first;
      delete posterior;

      delete forward;
      delete backward;
    }
  }

  if (!enableTraining){
    if (enableVerbose)
      cerr << endl;

    // now, perform the consistency transformation the desired number of times
    for (int i = 0; i < numConsistencyReps; i++)
      DoRelaxation (sequences, sparseMatrices);

    // compute the evolutionary tree
    TreeNode *tree = TreeNode::ComputeTree (distances);

    //tree->Print (cerr, sequences);
    //cerr << endl;

    // make the final alignment
    MultiSequence *alignment = ComputeFinalAlignment (tree, sequences, sparseMatrices, model);
    delete tree;

    return alignment;
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

    cerr << "PROBCONS comes with ABSOLUTELY NO WARRANTY.  This is free software, and" << endl
         << "you are welcome to redistribute it under certain conditions.  See the" << endl
         << "file COPYING.txt for details." << endl
         << endl
         << "Usage:" << endl
         << "       probcons [OPTION]... [MFAFILE]..." << endl
         << endl
         << "Description:" << endl
         << "       Align sequences in MFAFILE(s) and print result to standard output" << endl
         << endl
         << "       -t, --train FILENAME" << endl
         << "              compute EM transition probabilities, store in FILENAME (default: "
         << parametersOutputFilename << ")" << endl
         << endl
         << "       -m, --matrixfile FILENAME" << endl
         << "              read transition parameters from FILENAME (default: "
         << matrixFilename << ")" << endl
         << endl
         << "       -p, --paramfile FILENAME" << endl
         << "              read scoring matrix probabilities from FILENAME (default: "
         << parametersInputFilename << ")" << endl
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
         << "       -go, --gap-open VALUE" << endl
         << "              gap opening penalty of VALUE <= 0 (default: " << gapOpenPenalty << ")" << endl
         << endl
         << "       -ge, --gap-extension VALUE" << endl
         << "              gap extension penalty of VALUE <= 0 (default: " << gapContinuePenalty << ")" << endl
         << endl
         << "       -v, --verbose" << endl
         << "              report progress while aligning (default: " << (enableVerbose ? "on" : "off") << ")" << endl
         << endl;

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

      // scoring matrix file
      else if (!strcmp (argv[i], "-m") || !strcmp (argv[i], "--matrixfile")){
        if (i < argc - 1)
          matrixFilename = string (argv[++i]);
        else {
          cerr << "ERROR: Filename expected for option " << argv[i] << endl;
          exit (1);
        }
      }

      // transition/initial distribution parameter file
      else if (!strcmp (argv[i], "-p") || !strcmp (argv[i], "--paramfile")){
        if (i < argc - 1)
          parametersInputFilename = string (argv[++i]);
        else {
          cerr << "ERROR: Filename expected for option " << argv[i] << endl;
          exit (1);
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

      // verbose reporting
      else if (!strcmp (argv[i], "-v") || !strcmp (argv[i], "--verbose")){
        enableVerbose = true;
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

  // read initial state distribution and transition parameters
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
  }
  else {
    data.open (parametersInputFilename.c_str());
    if (data.fail()){
      cerr << "ERROR: Unable to read parameter file: " << parametersInputFilename << endl;
      exit (1);
    }
    for (int i = 0; i < NumMatrixTypes; i++) data >> initDistrib[i];
    for (int i = 0; i < 2*NumInsertStates; i++) data >> gapOpen[i];
    for (int i = 0; i < 2*NumInsertStates; i++) data >> gapExtend[i];
    data.close();
  }

  // read emission parameters
  int alphabetSize = 20;

  // allocate memory
  alphabet = SafeVector<char>(alphabetSize);
  emitPairs = VVF (alphabetSize, VF (alphabetSize, 0));
  emitSingle = VF (alphabetSize);

  if (matrixFilename == string ("")){
    for (int i = 0; i < alphabetSize; i++) alphabet[i] = alphabetDefault[i];
    for (int i = 0; i < alphabetSize; i++){
      emitSingle[i] = emitSingleDefault[i];
      for (int j = 0; j <= i; j++){
        emitPairs[i][j] = emitPairs[j][i] = (i == j);
      }
    }
  }
  else {
    data.open (matrixFilename.c_str());
    if (data.fail()){
      cerr << "ERROR: Unable to read scoring matrix file: " << matrixFilename << endl;
      exit (1);
    }

    for (int i = 0; i < alphabetSize; i++) data >> alphabet[i];
    for (int i = 0; i < alphabetSize; i++){
      for (int j = 0; j <= i; j++){
        data >> emitPairs[i][j];
        emitPairs[j][i] = emitPairs[i][j];
      }
    }
    for (int i = 0; i < alphabetSize; i++){
      char ch;
      data >> ch;
      assert (ch == alphabet[i]);
    }
    for (int i = 0; i < alphabetSize; i++) data >> emitSingle[i];
    data.close();
  }
}

/////////////////////////////////////////////////////////////////
// ProcessTree()
//
// Process the tree recursively.  Returns the aligned sequences
// corresponding to a node or leaf of the tree.
/////////////////////////////////////////////////////////////////

MultiSequence *ProcessTree (const TreeNode *tree, MultiSequence *sequences,
                            const SafeVector<SafeVector<SparseMatrix *> > &sparseMatrices,
                            const ProbabilisticModel &model){
  MultiSequence *result;

  // check if this is a node of the alignment tree
  if (tree->GetSequenceLabel() == -1){
    MultiSequence *alignLeft = ProcessTree (tree->GetLeftChild(), sequences, sparseMatrices, model);
    MultiSequence *alignRight = ProcessTree (tree->GetRightChild(), sequences, sparseMatrices, model);

    assert (alignLeft);
    assert (alignRight);

    result = AlignAlignments (alignLeft, alignRight, sparseMatrices, model);
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
                                      const ProbabilisticModel &model){

  MultiSequence *alignment = ProcessTree (tree, sequences, sparseMatrices, model);

  // iterative refinement
  for (int i = 0; i < numIterativeRefinementReps; i++)
    DoIterativeRefinement (sparseMatrices, model, alignment);

  cerr << endl;

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
                                const ProbabilisticModel &model){

  // print some info about the alignment
  if (enableVerbose){
    for (int i = 0; i < align1->GetNumSequences(); i++)
      cerr << ((i==0) ? "[" : ",") << align1->GetSequence(i)->GetLabel();
    cerr << "] vs. ";
    for (int i = 0; i < align2->GetNumSequences(); i++)
      cerr << ((i==0) ? "[" : ",") << align2->GetSequence(i)->GetLabel();
    cerr << "]: ";
  }

  VF *posterior = model.BuildPosterior (align1, align2, sparseMatrices);
  pair<SafeVector<char> *, float> alignment;

  // choose the alignment routine depending on the "cosmetic" gap penalties used
  if (gapOpenPenalty == 0 && gapContinuePenalty == 0)
    alignment = model.ComputeAlignment (align1->GetSequence(0)->GetLength(), align2->GetSequence(0)->GetLength(), *posterior);
  else
    alignment = model.ComputeAlignmentWithGapPenalties (align1, align2,
                                                        *posterior, align1->GetNumSequences(), align2->GetNumSequences(),
                                                        gapOpenPenalty, gapContinuePenalty);

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

void DoRelaxation (MultiSequence *sequences, SafeVector<SafeVector<SparseMatrix *> > &sparseMatrices){
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
        Relax (sparseMatrices[i][k], sparseMatrices[k][j], posterior);
      }

      // now renormalization
      for (int k = 0; k < (seq1Length+1) * (seq2Length+1); k++) posterior[k] /= numSeqs;

      // save the new posterior matrix
      newSparseMatrices[i][j] = new SparseMatrix (seq1->GetLength(), seq2->GetLength(), posterior);
      newSparseMatrices[j][i] = newSparseMatrices[i][j]->ComputeTranspose();

      if (enableVerbose)
        cerr << newSparseMatrices[i][j]->GetNumCells() << " -- ";

      delete posteriorPtr;

      if (enableVerbose)
        cerr << "done." << endl;
    }
  }

  // now replace the old posterior matrices
  for (int i = 0; i < numSeqs; i++){
    for (int j = 0; j < numSeqs; j++){
      delete sparseMatrices[i][j];
      sparseMatrices[i][j] = newSparseMatrices[i][j];
    }
  }
}

/////////////////////////////////////////////////////////////////
// DoRelaxation()
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
        base[ZYptr->first] += XZval * ZYptr->second;;
        ZYptr++;
      }
      XZptr++;
    }
  }
}

/////////////////////////////////////////////////////////////////
// DoIterativeRefinement()
//
// Performs a single round of randomized partionining iterative
// refinement.
/////////////////////////////////////////////////////////////////

void DoIterativeRefinement (const SafeVector<SafeVector<SparseMatrix *> > &sparseMatrices,
                            const ProbabilisticModel &model, MultiSequence* &alignment){
  set<int> groupOne, groupTwo;

  // create two separate groups
  for (int i = 0; i < alignment->GetNumSequences(); i++){
    if (random() % 2)
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
}

/*
float ScoreAlignment (MultiSequence *alignment, MultiSequence *sequences, SparseMatrix **sparseMatrices, const int numSeqs){
  int totLength = 0;
  float score = 0;

  for (int a = 0; a < alignment->GetNumSequences(); a++){
    for (int b = a+1; b < alignment->GetNumSequences(); b++){
      Sequence *seq1 = alignment->GetSequence(a);
      Sequence *seq2 = alignment->GetSequence(b);

      const int seq1Length = sequences->GetSequence(seq1->GetLabel())->GetLength();
      const int seq2Length = sequences->GetSequence(seq2->GetLabel())->GetLength();

      totLength += min (seq1Length, seq2Length);

      int pos1 = 0, pos2 = 0;
      for (int i = 1; i <= seq1->GetLength(); i++){
        char ch1 = seq1->GetPosition(i);
        char ch2 = seq2->GetPosition(i);

        if (ch1 != '-') pos1++;
        if (ch2 != '-') pos2++;
        if (ch1 != '-' && ch2 != '-'){
          score += sparseMatrices[a * numSeqs + b]->GetValue (pos1, pos2);
        }
      }
    }
  }

  return score / totLength;
}
*/
