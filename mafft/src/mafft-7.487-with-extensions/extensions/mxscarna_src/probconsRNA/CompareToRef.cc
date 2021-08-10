/////////////////////////////////////////////////////////////////
// CompareToRef.cc
//
// Program for scoring alignments according to the SUM-OF-PAIRS
// or COLUMN score.
/////////////////////////////////////////////////////////////////

#include "SafeVector.h"
#include "MultiSequence.h"
#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <set>
#include <limits>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <iomanip>

const char CORE_BLOCK = 'h';
typedef pair<int,int> PII;
bool useCoreBlocks = false;
bool useColScore = false;
bool useCaps = false;
bool useBaliAnnot = false;
bool makeAnnot = false;

/////////////////////////////////////////////////////////////////
// Function prototypes
/////////////////////////////////////////////////////////////////

set<PII> ComputePairs (MultiSequence *align, bool isRef);
set<VI> ComputeColumns (MultiSequence *align, bool isRef);
string GetName (string s);
set<int> coreCols;

set<VI> refCols, testCols;
set<PII> refPairs, testPairs;
VI annotation;

/////////////////////////////////////////////////////////////////
// main()
//
// Main program.
/////////////////////////////////////////////////////////////////

int main (int argc, char **argv){

  // check arguments
  if (argc < 3){
    cerr << "Usage: score TEST_ALIGNMENT REFERENCE_ALIGNMENT [BALIBASE_ANNOT_FILE] [-col] [-core] [-caps] [-annot FILENAME]" << endl;
    exit (1);
  }

  // try opening file
  FileBuffer infile (argv[1]);

  MultiSequence *testAlign;
  if (infile.fail()){
    cerr << "ERROR: Could not open file '" << argv[1] << "' for reading." << endl;
    testAlign = NULL;
  }
  else {
    testAlign = new MultiSequence(); assert (testAlign);
    testAlign->LoadMFA (infile);
  }
  infile.close();

  MultiSequence *refAlign = new MultiSequence (string (argv[2])); assert (refAlign);
  
  string outFilename = "";

  for (int i = 3; i < argc; i++){
    if (strcmp (argv[i], "-core") == 0)
      useCoreBlocks = true;
    else if (strcmp (argv[i], "-col") == 0)
      useColScore = true;
    else if (strcmp (argv[i], "-caps") == 0)
      useCaps = true;
    else if (strcmp (argv[i], "-annot") == 0){
      makeAnnot = true;
      outFilename = string (argv[++i]);
    }
    else { // annotation file
      useBaliAnnot = true;

      ifstream annotFile (argv[i]);
      if (annotFile.fail()){
        cerr << "ERROR: Could not read BAliBASE annotation file." << endl;
        exit (1);
      }

      SafeVector<int> *indices = refAlign->GetSequence(0)->GetMapping();

      char buffer[10000];
      while (annotFile.getline (buffer, 10000)){
        istringstream ss;
        ss.str (string (buffer));

        string s;

        if ((ss >> s) && s == string ("BPOS")){
          while (ss >> s){
            int begin=-1, end=-1;
            if (sscanf (s.c_str(), "%d=%d", &begin, &end) == 2){
              for (int i = (*indices)[begin]; i <= (*indices)[end]; i++)
                coreCols.insert (i);
            }
          }
        }
      }

      delete indices;

      annotFile.close();
    }
  }

  if (useColScore) makeAnnot = false;

  if (testAlign){
    for (int i = 0; i < testAlign->GetNumSequences(); i++){
      bool found = false;
      
      for (int j = 0; !found && j < refAlign->GetNumSequences(); j++){
	if (testAlign->GetSequence(i)->GetHeader() == refAlign->GetSequence(j)->GetHeader())
	  found = true;
      }
      
      if (!found){
	testAlign->RemoveSequence (i);
	i--;
      }
    }
    
    for (int i = 0; i < refAlign->GetNumSequences(); i++){
      bool found = false;
      
      for (int j = 0; !found && j < testAlign->GetNumSequences(); j++){
	if (refAlign->GetSequence(i)->GetHeader() == testAlign->GetSequence(j)->GetHeader())
	  found = true;
      }
      
      if (!found){
	refAlign->RemoveSequence (i);
	i--;
      }
    }
    
    testAlign->SortByHeader();
    refAlign->SortByHeader();
  }

  int TP = 0;
  int TPFN = 0;
  int TPFP = 0;
  double FD, FM;
  if (useColScore){
    refCols = ComputeColumns (refAlign, true);
    if (testAlign) testCols = ComputeColumns (testAlign, false);
    set<VI> colIntersect;
    insert_iterator<set<VI> > colIntersectIter (colIntersect, colIntersect.begin());
    set_intersection (testCols.begin(), testCols.end(), refCols.begin(), refCols.end(), colIntersectIter);
    TP = (int) colIntersect.size();
    TPFN = (int) refCols.size();
    if (testAlign) TPFP = (int) testCols.size();
  }
  else {
    refPairs = ComputePairs (refAlign, true);
    if (testAlign) testPairs = ComputePairs (testAlign, false);
    set<PII> pairIntersect;

    insert_iterator<set<PII> > pairIntersectIter (pairIntersect, pairIntersect.begin());
    set_intersection (testPairs.begin(), testPairs.end(), refPairs.begin(), refPairs.end(), pairIntersectIter);
    TP = (int) pairIntersect.size();
    TPFN = (int) refPairs.size();
    if (testAlign) TPFP = (int) testPairs.size();
  }

  FD = (double) TP / TPFN;
  FM = (double) TP / TPFP;
  
  cout << GetName(string (argv[2])) << " " << TP << " " << TPFN << " " << TPFP << " " << FD << " " << FM << endl;

  if (makeAnnot){
    ofstream outfile (outFilename.c_str());
    for (int i = 0; i < (int) annotation.size(); i++){
      outfile << annotation[i] << endl;
    }
    outfile.close();
  }

  if (testAlign) delete testAlign;
  delete refAlign;
}

int GetOffset (Sequence *testSeq, Sequence *refSeq){
  string test = testSeq->GetString();
  string ref = refSeq->GetString();

  for (int i = 0; i < (int) test.length(); i++) test[i] = toupper(test[i]);
  for (int i = 0; i < (int) ref.length(); i++) ref[i] = toupper(ref[i]);

  size_t offset = test.find (ref, 0);
  if (offset == string::npos){
    cerr << "ERROR: Reference string not found in original sequence!" << endl;
    cerr << "       test = " << test << endl;
    cerr << "       ref = " << ref << endl;
    exit (1);
  }

  cerr << "Offset found: " << offset << endl;

  return (int) offset;
}

string GetName (string s){

  size_t index1 = s.rfind ('/');
  size_t index2 = s.rfind ('.');

  if (index1 == string::npos) index1 = 0; else index1++;
  if (index2 == string::npos) index2 = s.length();

  if (index2 < index1) index2 = s.length();

  return s.substr (index1, index2 - index1);
}

bool isCore (char ch, int col){
  if (ch == '-') return false;
  if (useBaliAnnot){
    return coreCols.find (col) != coreCols.end();
  }
  if (useCaps){
    return ch >= 'A' && ch <= 'Z';
  }
  return ch == CORE_BLOCK;
}

/////////////////////////////////////////////////////////////////
// ComputePairs
//
// Returns the set of all matching pairs.
/////////////////////////////////////////////////////////////////

set<PII> ComputePairs (MultiSequence *align, bool isRef){
  int N = align->GetNumSequences();
  int L = align->GetSequence(0)->GetLength();

  // retrieve all sequence data pointers
  SafeVector<SafeVector<char>::iterator> seqs (N);
  for (int i = 0; i < N; i++){
    seqs[i] = align->GetSequence(i)->GetDataPtr();
    assert (align->GetSequence(i)->GetLength() == L);
  }

  set<PII> ret;
  VI ctr(N);

  // compute pairs
  for (int i = 1; i <= L; i++){

    // ctr keeps track of the current position in each sequence
    for (int j = 0; j < N; j++){
      ctr[j] += (seqs[j][i] != '-');
    }

    int good = 0;
    int ct = 0;

    // check for all matching pairs
    for (int j = 0; j < N - 1; j++){
      for (int k = j + 1; k < N; k++){
	
	// skip if one of the sequences is gapped
	if (seqs[j][i] == '-' || seqs[k][i] == '-') continue;

	// check for core blocks in the reference sequence
	if (isRef && useCoreBlocks)
	  if (!isCore (seqs[j][i], i) || !isCore (seqs[k][i], i)) continue;
      
	// if all ok, then add pair to list of pairs
	pair<int,int> p (10000 * j + ctr[j], 10000 * k + ctr[k]);

	// if we're making an annotation, compute annotation statistics
	if (makeAnnot && !isRef){
	  ct++;
	  if (refPairs.find (p) != refPairs.end()) good++;
	}
        ret.insert (p);
      }
    }

    // build annotation
    if (makeAnnot && !isRef){
      annotation.push_back ((ct == 0) ? 0 : 100 * good / ct);
    }

  }

  return ret;
}

/////////////////////////////////////////////////////////////////
// ComputeColumns
//
// Returns the set of all columns.
/////////////////////////////////////////////////////////////////

set<VI> ComputeColumns (MultiSequence *align,  bool isRef){
  int N = align->GetNumSequences();
  int L = align->GetSequence(0)->GetLength();

  // retrieve all sequence data pointers
  SafeVector<SafeVector<char>::iterator> seqs (N);
  for (int i = 0; i < N; i++){
    seqs[i] = align->GetSequence(i)->GetDataPtr();
  }

  set<VI> ret;
  VI ctr(N);

  // compute pairs
  for (int i = 1; i <= L; i++){

    // ctr keeps track of the current position in each sequence
    for (int j = 0; j < N; j++){
      ctr[j] += (seqs[j][i] != '-');
    }

    // add column, pick only positions that are matched
    SafeVector<int> column (N);
    bool useThisColumn = !useCoreBlocks;

    for (int j = 0; j < N; j++){
      if (isCore (seqs[j][i], i)) useThisColumn = true;
      column[j] = (seqs[j][i] == '-') ? -1 : ctr[j];
    }

    if (useThisColumn || !isRef)
      ret.insert (column);
  }

  return ret;
}
