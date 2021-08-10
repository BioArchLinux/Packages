////////////////////////////////////////////////////////////////
// MultiSequence.h
//
// Utilities for reading/writing multiple sequence data.
/////////////////////////////////////////////////////////////////

#ifndef MULTISEQUENCE_H
#define MULTISEQUENCE_H

#include <cctype>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <set>
#include "SafeVector.h"
#include "Sequence.h"
#include "FileBuffer.h"

/////////////////////////////////////////////////////////////////
// MultiSequence
//
// Class for multiple sequence alignment input/output.
/////////////////////////////////////////////////////////////////

namespace MXSCARNA {
class MultiSequence {

  SafeVector<Sequence *> *sequences;

 public:

  /////////////////////////////////////////////////////////////////
  // MultiSequence::MultiSequence()
  //
  // Default constructor.
  /////////////////////////////////////////////////////////////////

  MultiSequence () : sequences (NULL) {}

  /////////////////////////////////////////////////////////////////
  // MultiSequence::MultiSequence()
  //
  // Constructor.  Load MFA from a FileBuffer object.
  /////////////////////////////////////////////////////////////////

  MultiSequence (FileBuffer &infile) : sequences (NULL) {
    LoadMFA (infile);
  }

  /////////////////////////////////////////////////////////////////
  // MultiSequence::MultiSequence()
  //
  // Constructor.  Load MFA from a filename.
  /////////////////////////////////////////////////////////////////

  MultiSequence (const string &filename) : sequences (NULL){
    LoadMFA (filename);
  }

  /////////////////////////////////////////////////////////////////
  // MultiSequence::~MultiSequence()
  //
  // Destructor.  Gets rid of sequence objects contained in the
  // multiple alignment.
  /////////////////////////////////////////////////////////////////

  ~MultiSequence(){

    // if sequences allocated
    if (sequences){

      // free all sequences
      for (SafeVector<Sequence *>::iterator iter = sequences->begin(); iter != sequences->end(); ++iter){
        assert (*iter);
        delete *iter;
        *iter = NULL;
      }

      // free sequence vector
      delete sequences;
      sequences = NULL;
    }
  }

  /////////////////////////////////////////////////////////////////
  // MultiSequence::LoadMFA()
  //
  // Load MFA from a filename.
  /////////////////////////////////////////////////////////////////

  void LoadMFA (const string &filename, bool stripGaps = false){

    // try opening file
    FileBuffer infile (filename.c_str());

    if (infile.fail()){
      cerr << "ERROR: Could not open file '" << filename << "' for reading." << endl;
      exit (1);
    }

    // if successful, then load using other LoadMFA() routine
    LoadMFA (infile, stripGaps);

    infile.close();
  }

  /////////////////////////////////////////////////////////////////
  // MultiSequence::LoadMFA()
  //
  // Load MSF from a FileBuffer object.
  /////////////////////////////////////////////////////////////////

  void ParseMSF (FileBuffer &infile, string header, bool stripGaps = false){

    SafeVector<SafeVector<char> *> seqData;
    SafeVector<string> seqNames;
    SafeVector<int> seqLengths;

    istringstream in;
    bool valid = true;
    bool missingHeader = false;
    bool clustalW = false;

    // read until data starts
    while (!infile.eof() && header.find ("..", 0) == string::npos){
      if (header.find ("CLUSTAL", 0) == 0 || header.find ("PROBCONS", 0) == 0){
	clustalW = true;
	break;
      }
      infile.GetLine (header);
      if (header.find ("//", 0) != string::npos){
        missingHeader = true;
        break;
      }
    }

    // read until end-of-file
    while (valid){
      infile.GetLine (header);
      if (infile.eof()) break;

      string word;
      in.clear();
      in.str(header);

      // check if there's anything on this line
      if (in >> word){

	// clustalw name parsing
	if (clustalW){
	  if (!isspace(header[0]) && find (seqNames.begin(), seqNames.end(), word) == seqNames.end()){
	    seqNames.push_back (word);
	    seqData.push_back (new SafeVector<char>());
	    seqLengths.push_back (0);
	    seqData[(int) seqData.size() - 1]->push_back ('@');
	  }	  
	}

        // look for new sequence label
        if (word == string ("Name:")){
          if (in >> word){
            seqNames.push_back (word);
            seqData.push_back (new SafeVector<char>());
            seqLengths.push_back (0);
            seqData[(int) seqData.size() - 1]->push_back ('@');
          }
          else
            valid = false;
        }

        // check if this is sequence data
        else if (find (seqNames.begin(), seqNames.end(), word) != seqNames.end()){
          int index = find (seqNames.begin(), seqNames.end(), word) - seqNames.begin();

          // read all remaining characters on the line
          char ch;
          while (in >> ch){
            if (isspace (ch)) continue;
//            if (ch >= 'a' && ch <= 'z') ch = ch - 'a' + 'A';
            if (ch == '.') ch = '-';
	    if (stripGaps && ch == '-') continue;
/*
            if (!((ch >= 'A' && ch <= 'Z') || ch == '*' || ch == '-')){
              cerr << "ERROR: Unknown character encountered: " << ch << endl;
              exit (1);
	    }
*/
	    if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '*' || ch == '-')){
              cerr << "ERROR: Unknown character encountered: " << ch << endl;
              exit (1);
            }

            // everything's ok so far, so just store this character.
            seqData[index]->push_back (ch);
            seqLengths[index]++;
          }
        }
        else if (missingHeader){
          seqNames.push_back (word);
          seqData.push_back (new SafeVector<char>());
          seqLengths.push_back (0);
          seqData[(int) seqData.size() - 1]->push_back ('@');

          int index = (int) seqNames.size() - 1;

          // read all remaining characters on the line
          char ch;
          while (in >> ch){
            if (isspace (ch)) continue;
//            if (ch >= 'a' && ch <= 'z') ch = ch - 'a' + 'A';
            if (ch == '.') ch = '-';
	    if (stripGaps && ch == '-') continue;

            if (!((ch >= 'A' && ch <= 'Z') || ch == '*' || ch == '-')){
              cerr << "ERROR: Unknown character encountered: " << ch << endl;
              exit (1);
            }
/*
	    if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '*' || ch == '-')){
              cerr << "ERROR: Unknown character encountered: " << ch << endl;
              exit (1);
            }
*/
            // everything's ok so far, so just store this character.
            seqData[index]->push_back (ch);
            seqLengths[index]++;
          }
        }
      }
    }

    // check for errorsq
    if (seqNames.size() == 0){
      cerr << "ERROR: No sequences read!" << endl;
      exit (1);
    }

    assert (!sequences);
    sequences = new SafeVector<Sequence *>;
    for (int i = 0; i < (int) seqNames.size(); i++){
      if (seqLengths[i] == 0){
        cerr << "ERROR: Sequence of zero length!" << endl;
        exit (1);
      }
      Sequence *seq = new Sequence (seqData[i], seqNames[i], seqLengths[i], i, i);
      sequences->push_back (seq);
    }
  }

  /////////////////////////////////////////////////////////////////
  // MultiSequence::LoadMFA()
  //
  // Load MFA from a FileBuffer object.
  /////////////////////////////////////////////////////////////////

  void LoadMFA (FileBuffer &infile, bool stripGaps = false){

    // check to make sure that file reading is ok
    if (infile.fail()){
      cerr << "ERROR: Error reading file." << endl;
      exit (1);
    }

    // read all sequences
    while (true){

      // get the sequence label as being the current # of sequences
      // NOTE: sequence labels here are zero-based
      int index = (!sequences) ? 0 : sequences->size();

      // read the sequence
      Sequence *seq = new Sequence (infile, stripGaps);
      if (seq->Fail()){

        // check if alternative file format (i.e. not MFA)
        if (index == 0){
          string header = seq->GetHeader();
          if (header.length() > 0 && header[0] != '>'){
            // try MSF format
            ParseMSF (infile, header);
            break;
          }
        }

        delete seq;
        break;
      }
      seq->SetLabel (index);

      // add the sequence to the list of current sequences
      if (!sequences) sequences = new SafeVector<Sequence *>;
      sequences->push_back (seq);
    }

    // make sure at least one sequence was read
    if (!sequences){
      cerr << "ERROR: No sequences read." << endl;
      exit (1);
    }
  }

  /////////////////////////////////////////////////////////////////
  // MultiSequence::AddSequence()
  //
  // Add another sequence to an existing sequence list
  /////////////////////////////////////////////////////////////////

  void AddSequence (Sequence *sequence){
    assert (sequence);
    assert (!sequence->Fail());

    // add sequence
    if (!sequences) sequences = new SafeVector<Sequence *>;
    sequences->push_back (sequence);
  }

  /////////////////////////////////////////////////////////////////
  // MultiSequence::RemoveSequence()
  //
  // Remove a sequence from the MultiSequence
  /////////////////////////////////////////////////////////////////

  void RemoveSequence (int index){
    assert (sequences);

    assert (index >= 0 && index < (int) sequences->size());
    delete (*sequences)[index];

    sequences->erase (sequences->begin() + index);
  }

  /////////////////////////////////////////////////////////////////
  // MultiSequence::WriteMFA()
  //
  // Write MFA to the outfile.  Allows the user to specify the
  // number of columns for the output.  Also, useIndices determines
  // whether or not the actual sequence comments will be printed
  // out or whether the artificially assigned sequence labels will
  // be used instead.
  /////////////////////////////////////////////////////////////////

  void WriteMFA (ostream &outfile, string *ssCons = NULL, int numColumns = 60, bool useIndices = false){
    if (!sequences) return;

    // loop through all sequences and write them out
    for (SafeVector<Sequence *>::iterator iter = sequences->begin(); iter != sequences->end(); ++iter){
      (*iter)->WriteMFA (outfile, numColumns, useIndices);
    }

    int count = 0;
    if (ssCons != NULL) {
      outfile << ">#=GC SS_cons" << endl;
      int length = ssCons->length();
      for (int i = 1; i < length; i++ ) {
	outfile << ssCons->at(i);
	++count;
	
	if (numColumns <= count) {
	  outfile << endl;
	  count = 0;
	}
	
      }
    }
    outfile << endl;
  }

  void WriteMFAseq (ostream &outfile, int numColumns = 60, bool useIndices = false){
    if (!sequences) return;

    // loop through all sequences and write them out
    for (SafeVector<Sequence *>::iterator iter = sequences->begin(); iter != sequences->end(); ++iter){
      (*iter)->WriteMFA (outfile, numColumns, useIndices);
    }
  }

  /////////////////////////////////////////////////////////////////
  // MultiSequence::GetAnnotationChar()
  //
  // Return CLUSTALW annotation for column.
  /////////////////////////////////////////////////////////////////

  char GetAnnotationChar (SafeVector<char> &column){
    SafeVector<int> counts (256, 0);
    int allChars = (int) column.size();
    
    for (int i = 0; i < allChars; i++){
      counts[(unsigned char) toupper(column[i])]++;
    }
    
    allChars -= counts[(unsigned char) '-'];
    if (allChars == 1) return ' ';
    
    for (int i = 0; i < 256; i++) if ((char) i != '-' && counts[i] == allChars) return '*';
    
    if (counts[(unsigned char) 'S'] + 
	counts[(unsigned char) 'T'] + 
	counts[(unsigned char) 'A'] == allChars) 
      return ':';
    
    if (counts[(unsigned char) 'N'] + 
	counts[(unsigned char) 'E'] + 
	counts[(unsigned char) 'Q'] +
	counts[(unsigned char) 'K'] == allChars) 
      return ':';

    if (counts[(unsigned char) 'N'] + 
	counts[(unsigned char) 'H'] + 
	counts[(unsigned char) 'Q'] +
	counts[(unsigned char) 'K'] == allChars) 
      return ':';

    if (counts[(unsigned char) 'N'] + 
	counts[(unsigned char) 'D'] + 
	counts[(unsigned char) 'E'] +
	counts[(unsigned char) 'Q'] == allChars) 
      return ':';

    if (counts[(unsigned char) 'Q'] + 
	counts[(unsigned char) 'H'] + 
	counts[(unsigned char) 'R'] +
	counts[(unsigned char) 'K'] == allChars) 
      return ':';

    if (counts[(unsigned char) 'M'] + 
	counts[(unsigned char) 'I'] + 
	counts[(unsigned char) 'L'] +
	counts[(unsigned char) 'V'] == allChars) 
      return ':';

    if (counts[(unsigned char) 'M'] + 
	counts[(unsigned char) 'I'] + 
	counts[(unsigned char) 'L'] +
	counts[(unsigned char) 'F'] == allChars) 
      return ':';

    if (counts[(unsigned char) 'H'] + 
	counts[(unsigned char) 'Y'] == allChars) 
      return ':';

    if (counts[(unsigned char) 'F'] + 
	counts[(unsigned char) 'Y'] + 
	counts[(unsigned char) 'W'] == allChars) 
      return ':';

    if (counts[(unsigned char) 'C'] + 
	counts[(unsigned char) 'S'] + 
	counts[(unsigned char) 'A'] == allChars) 
      return '.';

    if (counts[(unsigned char) 'A'] + 
	counts[(unsigned char) 'T'] + 
	counts[(unsigned char) 'V'] == allChars) 
      return '.';

    if (counts[(unsigned char) 'S'] + 
	counts[(unsigned char) 'A'] + 
	counts[(unsigned char) 'G'] == allChars) 
      return '.';

    if (counts[(unsigned char) 'S'] + 
	counts[(unsigned char) 'T'] + 
	counts[(unsigned char) 'N'] + 
	counts[(unsigned char) 'K'] == allChars) 
      return '.';

    if (counts[(unsigned char) 'S'] + 
	counts[(unsigned char) 'T'] + 
	counts[(unsigned char) 'P'] + 
	counts[(unsigned char) 'A'] == allChars) 
      return '.';

    if (counts[(unsigned char) 'S'] + 
	counts[(unsigned char) 'G'] + 
	counts[(unsigned char) 'N'] + 
	counts[(unsigned char) 'D'] == allChars) 
      return '.';

    if (counts[(unsigned char) 'S'] + 
	counts[(unsigned char) 'N'] + 
	counts[(unsigned char) 'D'] + 
	counts[(unsigned char) 'E'] + 
	counts[(unsigned char) 'Q'] + 
	counts[(unsigned char) 'K'] == allChars) 
      return '.';

    if (counts[(unsigned char) 'N'] + 
	counts[(unsigned char) 'D'] + 
	counts[(unsigned char) 'E'] + 
	counts[(unsigned char) 'Q'] + 
	counts[(unsigned char) 'H'] + 
	counts[(unsigned char) 'K'] == allChars) 
      return '.';

    if (counts[(unsigned char) 'N'] + 
	counts[(unsigned char) 'E'] + 
	counts[(unsigned char) 'H'] + 
	counts[(unsigned char) 'Q'] + 
	counts[(unsigned char) 'R'] + 
	counts[(unsigned char) 'K'] == allChars) 
      return '.';

    if (counts[(unsigned char) 'F'] + 
	counts[(unsigned char) 'V'] + 
	counts[(unsigned char) 'L'] + 
	counts[(unsigned char) 'I'] + 
	counts[(unsigned char) 'M'] == allChars) 
      return '.';

    if (counts[(unsigned char) 'H'] + 
	counts[(unsigned char) 'F'] + 
	counts[(unsigned char) 'Y'] == allChars) 
      return '.';

    return ' ';
  }

  /////////////////////////////////////////////////////////////////
  // MultiSequence::WriteALN()
  //
  // Write ALN to the outfile.  Allows the user to specify the
  // number of columns for the output.  
  /////////////////////////////////////////////////////////////////

  void WriteALN (ostream &outfile, int numColumns = 60){
    if (!sequences) return;

//   outfile << "Multplex SCARNA version " << VERSION << " multiple sequence alignment"  << endl;
//   outfile << "PROBCONS version " << VERSION << " multiple sequence alignment" << endl;
    outfile << "CLUSTAL W(1.83) multiple sequence alignment" << endl;
//    outfile << "//" << endl;

    int longestComment = 0;
    SafeVector<SafeVector<char>::iterator> ptrs (GetNumSequences());
    SafeVector<int> lengths (GetNumSequences());
    for (int i = 0; i < GetNumSequences(); i++){
      ptrs[i] = GetSequence (i)->GetDataPtr();
      lengths[i] = GetSequence (i)->GetLength();
      longestComment = max (longestComment, (int) GetSequence(i)->GetName().length());
    }
    longestComment += 4;

    int writtenChars = 0;    
    bool allDone = false;

    while (!allDone){
      outfile << endl;
      allDone = true;

      // loop through all sequences and write them out
      for (int i = 0; i < GetNumSequences(); i++){

	if (writtenChars < lengths[i]){
	  outfile << GetSequence(i)->GetName();
	  for (int j = 0; j < longestComment - (int) GetSequence(i)->GetName().length(); j++)
	    outfile << ' ';

	  for (int j = 0; j < numColumns; j++){
	    if (writtenChars + j < lengths[i])
	      outfile << ptrs[i][writtenChars + j + 1];
	    else
	      break;
	  }
	  
	  outfile << endl;
	  
	  if (writtenChars + numColumns < lengths[i]) allDone = false;
	}
      }

      // write annotation line
      for (int j = 0; j < longestComment; j++)
	outfile << ' ';

      for (int j = 0; j < numColumns; j++){
	SafeVector<char> column;

	for (int i = 0; i < GetNumSequences(); i++)
	  if (writtenChars + j < lengths[i])
	    column.push_back (ptrs[i][writtenChars + j + 1]);
	
	if (column.size() > 0)
	  outfile << GetAnnotationChar (column);	
      }

      outfile << endl;
      writtenChars += numColumns;
    }
    outfile << endl;
  }

  ////////////////////////////////////////////////////////////////
  // MultiSequence::WriteWEB();
  //
  // Write ALN to the outfile.  Allows the user to specify the
  // number of columns for the output.  
  ///////////////////////////////////////////////////////////////
   void WriteWEB (ostream &outfile, string *ssCons = NULL, int numColumns = 60, bool useIndices = false){
    if (!sequences) return;

    // loop through all sequences and write them out
    for (SafeVector<Sequence *>::iterator iter = sequences->begin(); iter != sequences->end(); ++iter){
      (*iter)->WriteWEB (outfile, numColumns, useIndices);
    }
    
    // write conservation 
    outfile << "<conservation>" << endl;
        int longestComment = 0;
    SafeVector<SafeVector<char>::iterator> ptrs (GetNumSequences());
    SafeVector<int> lengths (GetNumSequences());
    for (int i = 0; i < GetNumSequences(); i++){
      ptrs[i] = GetSequence (i)->GetDataPtr();
      lengths[i] = GetSequence (i)->GetLength();
      longestComment = max (longestComment, (int) GetSequence(i)->GetName().length());
    }
    longestComment += 4;

    int writtenChars = 0;    
    bool allDone = false;

    while (!allDone){
//      outfile << endl;
      allDone = true;

      // loop through all sequences and write them out
      for (int i = 0; i < GetNumSequences(); i++){

	if (writtenChars < lengths[i]){
//	  outfile << GetSequence(i)->GetName();
	  for (int j = 0; j < longestComment - (int) GetSequence(i)->GetName().length(); j++)
//	    outfile << ' ';

	  for (int j = 0; j < numColumns; j++){
	      if (writtenChars + j < lengths[i]);
//	      outfile << ptrs[i][writtenChars + j + 1];
	    else
	      break;
	  }
	  
//	  outfile << endl;
	  
	  if (writtenChars + numColumns < lengths[i]) allDone = false;
	}
      }

      // write annotation line
//      for (int j = 0; j < longestComment; j++)
//	outfile << ' ';

      for (int j = 0; j < numColumns; j++){
	SafeVector<char> column;

	for (int i = 0; i < GetNumSequences(); i++)
	  if (writtenChars + j < lengths[i])
	    column.push_back (ptrs[i][writtenChars + j + 1]);
	
	if (column.size() > 0)
	  outfile << GetAnnotationChar (column);	
      }

//      outfile << endl;
      writtenChars += numColumns;
    }
    outfile << endl;
    outfile << "</conservation>" << endl;

    // write structure information
    if (ssCons != NULL) {
      outfile << "<structure>" << endl;
      int length = ssCons->length();
      for (int i = 1; i < length; i++ ) {
	outfile << ssCons->at(i);
      }
      outfile << endl;
      outfile << "</structure>" << endl;

      // add coordinate information 06/09/14
      outfile << "<coordinate>" << endl;
    
      int segmentPos = 1;
      for (int i = 1; i < length; i++) {
	int count = 0;
	
	if ( ssCons->at(i) == '(' ) {
	  ++count;
	  
	  int j = i;
	  while (count != 0) {
	    char ch = ssCons->at(++j);
	    if      (ch == '(')
	      ++count;
	    else if (ch == ')')
	      --count;
	  }
	    
	  outfile << "<segment position=\"" << segmentPos++ << "\" starts=\"" 
		  << i << "\"" << " ends=\"" << j << "\"/>" << endl;
	    
	}
      }
    }
    outfile << "</coordinate>" << endl;

    outfile << "<mxscarna>" << endl;
    WriteMXSCARNA (outfile, ssCons);
    outfile << "</mxscarna>" << endl;
    
    outfile << "<aln>" << endl;
    WriteALN (outfile);
    outfile << "</aln>" << endl;

    outfile << "<mfa>" << endl;
    WriteMFA (outfile, ssCons);
    outfile << "</mfa>" << endl;

    outfile << "<stockholm>" << endl;
    WriteWebSTOCKHOLM (outfile, ssCons);
    outfile << "</stockholm>" << endl;
  }
  
  ////////////////////////////////////////////////////////////////
  // MultiSequence::WriteSTOCKHOLM();
  //
  // Write STOCKHOLM to the outfile.  Allows the user to specify the
  // number of columns for the output.  
  ///////////////////////////////////////////////////////////////
  void WriteSTOCKHOLM (ostream &outfile, string *ssCons = NULL, int numColumns = 60) {
    if (!sequences) return;
    
        outfile << "# STOCKHOLM 1.0" << endl;

    int longestComment = 0;
    SafeVector<SafeVector<char>::iterator> ptrs (GetNumSequences());
    SafeVector<int> lengths (GetNumSequences());
    for (int i = 0; i < GetNumSequences(); i++){
      ptrs[i] = GetSequence (i)->GetDataPtr();
      lengths[i] = GetSequence (i)->GetLength();
      longestComment = max (longestComment, (int) GetSequence(i)->GetName().length());
    }
    longestComment += 4;

    int writtenChars = 0;    
    bool allDone = false;

    while (!allDone){
      outfile << endl;
      allDone = true;

      // loop through all sequences and write them out
      for (int i = 0; i < GetNumSequences(); i++){

	if (writtenChars < lengths[i]){
	  outfile << GetSequence(i)->GetName();
	  for (int j = 0; j < longestComment - (int) GetSequence(i)->GetName().length(); j++)
	    outfile << ' ';

	  for (int j = 0; j < numColumns; j++){
	    if (writtenChars + j < lengths[i])
		if (ptrs[i][writtenChars + j + 1] != '-')
		  outfile << ptrs[i][writtenChars + j + 1];
	        else 
		  outfile << ".";
	    else
	      break;
	  }
	  
	  outfile << endl;
	  
	  if (writtenChars + numColumns < lengths[i]) allDone = false;
	}
      }

      // write ssCons

      if (ssCons != NULL) {
	  outfile << "#=GC SS_cons";
	  int lengthSScons = 12;
	  for (int j = 0; j < longestComment - lengthSScons; j++)
	      outfile << ' ';

	  for (int j = 0; j < numColumns; j++) {
	      if (ssCons->at(writtenChars + j + 1) == '(')
		outfile << "<";
	      else if (ssCons->at(writtenChars + j + 1) == ')')
		outfile << ">";
	      else 
		outfile << ".";
	      if ((unsigned int)writtenChars + j + 1 >= ssCons->length() - 1) 
		  break;
	  }
	  outfile << endl;
      }

      writtenChars += numColumns;
    }
    outfile << "//";
    outfile << endl;
  }
  
    ////////////////////////////////////////////////////////////////
  // MultiSequence::WriteSTOCKHOLM();
  //
  // Write STOCKHOLM to the outfile.  Allows the user to specify the
  // number of columns for the output.  
  ///////////////////////////////////////////////////////////////
  void WriteWebSTOCKHOLM (ostream &outfile, string *ssCons = NULL, int numColumns = 60) {
    if (!sequences) return;
    
        outfile << "# STOCKHOLM 1.0" << endl;

    int longestComment = 0;
    SafeVector<SafeVector<char>::iterator> ptrs (GetNumSequences());
    SafeVector<int> lengths (GetNumSequences());
    for (int i = 0; i < GetNumSequences(); i++){
      ptrs[i] = GetSequence (i)->GetDataPtr();
      lengths[i] = GetSequence (i)->GetLength();
      longestComment = max (longestComment, (int) GetSequence(i)->GetName().length());
    }
    longestComment += 4;

    int writtenChars = 0;    
    bool allDone = false;

    while (!allDone){
      outfile << endl;
      allDone = true;

      // loop through all sequences and write them out
      for (int i = 0; i < GetNumSequences(); i++){

	if (writtenChars < lengths[i]){
	  outfile << GetSequence(i)->GetName();
	  for (int j = 0; j < longestComment - (int) GetSequence(i)->GetName().length(); j++)
	    outfile << ' ';

	  for (int j = 0; j < numColumns; j++){
	    if (writtenChars + j < lengths[i])
		if (ptrs[i][writtenChars + j + 1] != '-')
		  outfile << ptrs[i][writtenChars + j + 1];
	        else 
		  outfile << ".";
	    else
	      break;
	  }
	  
	  outfile << endl;
	  
	  if (writtenChars + numColumns < lengths[i]) allDone = false;
	}
      }

      // write ssCons

      if (ssCons != NULL) {
	  outfile << "#=GC SS_cons";
	  int lengthSScons = 12;
	  for (int j = 0; j < longestComment - lengthSScons; j++)
	      outfile << ' ';

	  for (int j = 0; j < numColumns; j++) {
	      outfile << ssCons->at(writtenChars + j + 1);

	      if ((unsigned int)writtenChars + j + 1 >= ssCons->length() - 1) 
		  break;
	  }
	  outfile << endl;
      }

      writtenChars += numColumns;
    }
    outfile << "//";
    outfile << endl;
  }

  ////////////////////////////////////////////////////////////////
  // MultiSequence::WriteMXSCARNA();
  //
  // Write MXSCARNA to the outfile.  Allows the user to specify the
  // number of columns for the output.  
  ///////////////////////////////////////////////////////////////
  void WriteMXSCARNA (ostream &outfile, string *ssCons = NULL, int numColumns = 60){
    if (!sequences) return;

   outfile << "Multplex SCARNA version " << VERSION << " multiple sequence alignment"  << endl;

    int longestComment = 0;
    SafeVector<SafeVector<char>::iterator> ptrs (GetNumSequences());
    SafeVector<int> lengths (GetNumSequences());
    for (int i = 0; i < GetNumSequences(); i++){
      ptrs[i] = GetSequence (i)->GetDataPtr();
      lengths[i] = GetSequence (i)->GetLength();
      longestComment = max (longestComment, (int) GetSequence(i)->GetName().length());
    }
    longestComment += 4;

    int writtenChars = 0;    
    bool allDone = false;

    while (!allDone){
      outfile << endl;
      allDone = true;

      // loop through all sequences and write them out
      for (int i = 0; i < GetNumSequences(); i++){

	if (writtenChars < lengths[i]){
	  outfile << GetSequence(i)->GetName();
	  for (int j = 0; j < longestComment - (int) GetSequence(i)->GetName().length(); j++)
	    outfile << ' ';

	  for (int j = 0; j < numColumns; j++){
	    if (writtenChars + j < lengths[i])
	      outfile << ptrs[i][writtenChars + j + 1];
	    else
	      break;
	  }
	  
	  outfile << endl;
	  
	  if (writtenChars + numColumns < lengths[i]) allDone = false;
	}
      }

      // write ssCons
      if (ssCons != NULL) {
	  outfile << "ss_cons";
	  int lengthSScons = 7;
	  for (int j = 0; j < longestComment - lengthSScons; j++)
	      outfile << ' ';

	  for (int j = 0; j < numColumns; j++) {
	      outfile << ssCons->at(writtenChars + j + 1);
	      if ((unsigned int)writtenChars + j + 1 >= ssCons->length() - 1) 
		  break;
	  }
	  outfile << endl;
      }

      // write annotation line
      for (int j = 0; j < longestComment; j++)
	outfile << ' ';

      for (int j = 0; j < numColumns; j++){
	SafeVector<char> column;

	for (int i = 0; i < GetNumSequences(); i++)
	  if (writtenChars + j < lengths[i])
	    column.push_back (ptrs[i][writtenChars + j + 1]);
	
	if (column.size() > 0)
	  outfile << GetAnnotationChar (column);	
      }

      outfile << endl;
      writtenChars += numColumns;
    }
    outfile << endl;
  }

  /////////////////////////////////////////////////////////////////
  // MultiSequence::GetSequence()
  //
  // Retrieve a sequence from the MultiSequence object.
  /////////////////////////////////////////////////////////////////

  Sequence* GetSequence (int i){
    assert (sequences);
    assert (0 <= i && i < (int) sequences->size());

    return (*sequences)[i];
  }

  /////////////////////////////////////////////////////////////////
  // MultiSequence::GetSequence()
  //
  // Retrieve a sequence from the MultiSequence object
  // (const version).
  /////////////////////////////////////////////////////////////////

  const Sequence* GetSequence (int i) const {
    assert (sequences);
    assert (0 <= i && i < (int) sequences->size());

    return (*sequences)[i];
  }

  /////////////////////////////////////////////////////////////////
  // MultiSequence::GetNumSequences()
  //
  // Returns the number of sequences in the MultiSequence.
  /////////////////////////////////////////////////////////////////

  int GetNumSequences () const {
    if (!sequences) return 0;
    return (int) sequences->size();
  }

  /////////////////////////////////////////////////////////////////
  // MultiSequence::SortByHeader()
  //
  // Organizes the sequences according to their sequence headers
  // in ascending order.
  /////////////////////////////////////////////////////////////////

  void SortByHeader () {
    assert (sequences);

    // a quick and easy O(n^2) sort
    for (int i = 0; i < (int) sequences->size()-1; i++){
      for (int j = i+1; j < (int) sequences->size(); j++){
        if ((*sequences)[i]->GetHeader() > (*sequences)[j]->GetHeader())
          swap ((*sequences)[i], (*sequences)[j]);
      }
    }
  }

  /////////////////////////////////////////////////////////////////
  // MultiSequence::SortByLabel()
  //
  // Organizes the sequences according to their sequence labels
  // in ascending order.
  /////////////////////////////////////////////////////////////////

  void SortByLabel () {
    assert (sequences);

    // a quick and easy O(n^2) sort
    for (int i = 0; i < (int) sequences->size()-1; i++){
      for (int j = i+1; j < (int) sequences->size(); j++){
        if ((*sequences)[i]->GetSortLabel() > (*sequences)[j]->GetSortLabel())
          swap ((*sequences)[i], (*sequences)[j]);
      }
    }
  }

  /////////////////////////////////////////////////////////////////
  // MultiSequence::SaveOrdering()
  //
  // Relabels sequences so as to preserve the current ordering.
  /////////////////////////////////////////////////////////////////

  void SaveOrdering () {
    assert (sequences);
    
    for (int i = 0; i < (int) sequences->size(); i++)
      (*sequences)[i]->SetSortLabel (i);
  }

  /////////////////////////////////////////////////////////////////
  // MultiSequence::Project()
  //
  // Given a set of indices, extract all sequences from the current
  // MultiSequence object whose index is included in the set.
  // Then, project the multiple alignments down to the desired
  // subset, and return the projection as a new MultiSequence
  // object.
  /////////////////////////////////////////////////////////////////

  MultiSequence *Project (const set<int> &indices){
    SafeVector<SafeVector<char>::iterator> oldPtrs (indices.size());
    SafeVector<SafeVector<char> *> newPtrs (indices.size());

    assert (indices.size() != 0);

    // grab old data
    int i = 0;
    for (set<int>::const_iterator iter = indices.begin(); iter != indices.end(); ++iter){
      oldPtrs[i++] = GetSequence (*iter)->GetDataPtr();
    }

    // compute new length
    int oldLength = GetSequence (*indices.begin())->GetLength();
    int newLength = 0;
    for (i = 1; i <= oldLength; i++){

      // check to see if there is a gap in every sequence of the set
      bool found = false;
      for (int j = 0; !found && j < (int) indices.size(); j++)
        found = (oldPtrs[j][i] != '-');

      // if not, then this column counts towards the sequence length
      if (found) newLength++;
    }

    // build new alignments
    for (i = 0; i < (int) indices.size(); i++){
      newPtrs[i] = new SafeVector<char>(); assert (newPtrs[i]);
      newPtrs[i]->push_back ('@');
    }

    // add all needed columns
    for (i = 1; i <= oldLength; i++){

      // make sure column is not gapped in all sequences in the set
      bool found = false;
      for (int j = 0; !found && j < (int) indices.size(); j++)
        found = (oldPtrs[j][i] != '-');

      // if not, then add it
      if (found){
        for (int j = 0; j < (int) indices.size(); j++)
          newPtrs[j]->push_back (oldPtrs[j][i]);
      }
    }

    // wrap sequences in MultiSequence object
    MultiSequence *ret = new MultiSequence();
    i = 0;
    for (set<int>::const_iterator iter = indices.begin(); iter != indices.end(); ++iter){
      ret->AddSequence (new Sequence (newPtrs[i++], GetSequence (*iter)->GetHeader(), newLength,
                                      GetSequence (*iter)->GetSortLabel(), GetSequence (*iter)->GetLabel()));
    }

    return ret;
  }
};
}
#endif
