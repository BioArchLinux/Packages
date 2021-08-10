/////////////////////////////////////////////////////////////////
// ProjectPairwise
//
// Program for projecting multiple alignments to all pairwise
// alignments.
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

bool compressGaps = true;

/////////////////////////////////////////////////////////////////
// main()
//
// Main program.
/////////////////////////////////////////////////////////////////

int main (int argc, char **argv){

  // check arguments
  if (argc < 2){
    cerr << "Usage: project ALIGNMENT [-nocompressgaps]" << endl;
    exit (1);
  }

  for (int i = 2; i < argc; i++){
    if (strcmp (argv[i], "-nocompressgaps") == 0)
      compressGaps = false;
    else {
      cerr << "Unrecognized option: " << argv[i] << endl;
      exit (1);
    }
  }

  MultiSequence *align = new MultiSequence (string (argv[1])); assert (align);

  int N = align->GetNumSequences();
  for (int i = 0; i < N; i++){
    for (int j = i+1; j < N; j++){
      string name = align->GetSequence(i)->GetHeader() + "-" + align->GetSequence(j)->GetHeader() + ".fasta";
      ofstream outfile (name.c_str());

      if (compressGaps){
	set<int> s;
	s.insert (i); s.insert (j);
	MultiSequence *proj = align->Project (s);
	proj->WriteMFA (outfile);
	delete proj;
      }
      else {
	align->GetSequence(i)->WriteMFA (outfile, 60);
	align->GetSequence(j)->WriteMFA (outfile, 60);
      }
      outfile.close();
    }
  }

  delete align;
}
