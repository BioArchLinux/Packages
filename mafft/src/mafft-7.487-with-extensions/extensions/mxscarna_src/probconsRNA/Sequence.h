/////////////////////////////////////////////////////////////////
// Sequence.h
//
// Class for reading/manipulating single sequence character data.
/////////////////////////////////////////////////////////////////

#ifndef __SEQUENCE_H__
#define __SEQUENCE_H__

#include <string>
#include <fstream>
#include <iostream>
#include <cctype>
#include <cstdlib>
#include "SafeVector.h"
#include "FileBuffer.h"

/////////////////////////////////////////////////////////////////
// Sequence
//
// Class for storing sequence information.
/////////////////////////////////////////////////////////////////
namespace MXSCARNA {
class Sequence {

  bool isValid;                // a boolean indicating whether the sequence data is valid or not
  string header;               // string containing the comment line of the FASTA file
  SafeVector<char> *data;      // pointer to character data
  int length;                  // length of the sequence
  int sequenceLabel;           // integer sequence label, typically to indicate the ordering of sequences
                               //   in a Multi-FASTA file
  int inputLabel;              // position of sequence in original input
  float weight;

  /////////////////////////////////////////////////////////////////
  // Sequence::Sequence()
  //
  // Default constructor.  Does nothing.
  /////////////////////////////////////////////////////////////////

  Sequence () : isValid (false), header (""), data (NULL), length (0), sequenceLabel (0), inputLabel (0) {}

 public:

  /////////////////////////////////////////////////////////////////
  // Sequence::Sequence()
  //
  // Constructor.  Reads the sequence from a FileBuffer.
  /////////////////////////////////////////////////////////////////

  Sequence (FileBuffer &infile, bool stripGaps = false) : isValid (false), header ("~"), data (NULL), length(0), sequenceLabel (0), inputLabel (0) {

    // read until the first non-blank line
    while (!infile.eof()){
      infile.GetLine (header);
      if (header.length() != 0) break;
    }

    // check to make sure that it is a correct header line
    if (header[0] == '>'){

      // if so, remove the leading ">"
      header = header.substr (1);

      // remove any leading or trailing white space in the header comment
      while (header.length() > 0 && isspace (header[0])) header = header.substr (1);
      while (header.length() > 0 && isspace (header[header.length() - 1])) header = header.substr(0, header.length() - 1);

      // get ready to read the data[] array; note that data[0] is always '@'
      char ch;
      data = new SafeVector<char>; assert (data);
      data->push_back ('@');

      // get a character from the file
      while (infile.Get(ch)){

        // if we've reached a new comment line, put the character back and stop
        if (ch == '>'){ infile.UnGet(); break; }

        // skip whitespace
        if (isspace (ch)) continue;

        // substitute gap character
        if (ch == '.') ch = '-';
	if (stripGaps && ch == '-') continue;

        // check for known characters
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '*' || ch == '-')){
          cerr << "ERROR: Unknown character encountered: " << ch << endl;
          exit (1);
        }

        // everything's ok so far, so just store this character.
	data->push_back(ch);
	++length;
      }

      // sequence must contain data in order to be valid
      isValid = length > 0;
      if (!isValid){
        delete data;
        data = NULL;
      }
    }
  }

  
  /////////////////////////////////////////////////////////////////
  // Sequence::Sequence()
  //
  // Constructor.  Builds a sequence from existing data.  Note
  // that the data must use one-based indexing where data[0] should
  // be set to '@'.
  /////////////////////////////////////////////////////////////////

  Sequence (SafeVector<char> *data, string header, int length, int sequenceLabel, int inputLabel) :
    isValid (data != NULL), header(header), data(data), length (length), sequenceLabel (sequenceLabel), inputLabel (inputLabel) {
      assert (data);
      assert ((*data)[0] == '@');
  }
 
  /////////////////////////////////////////////////////////////////
  // Sequence::Sequence()
  //
  // Destructor.  Release allocated memory.
  /////////////////////////////////////////////////////////////////

  ~Sequence (){
    if (data){
      assert (isValid);
      delete data;
      data = NULL;
      isValid = false;
    }
  }

    void SetWeight(float myWeight) {
      weight = myWeight;
  }
  float GetWeight() const {
      return weight;
  }

  /////////////////////////////////////////////////////////////////
  // Sequence::GetHeader()
  //
  // Return the string comment associated with this sequence.
  /////////////////////////////////////////////////////////////////

  string GetHeader () const {
    return header;
  }

  /////////////////////////////////////////////////////////////////
  // Sequence::GetName()
  //
  // Return the first word of the string comment associated with this sequence.
  /////////////////////////////////////////////////////////////////

  string GetName () const {
    char name[1024];
    sscanf (header.c_str(), "%s", name);
    return string(name);
  }

  /////////////////////////////////////////////////////////////////
  // Sequence::GetDataPtr()
  //
  // Return the iterator to data associated with this sequence.
  /////////////////////////////////////////////////////////////////

  SafeVector<char>::iterator GetDataPtr(){
    assert (isValid);
    assert (data);
    return data->begin();
  }

  /////////////////////////////////////////////////////////////////
  // Sequence::GetPosition()
  //
  // Return the character at position i.  Recall that the character
  // data is stored with one-based indexing.
  /////////////////////////////////////////////////////////////////

  char GetPosition (int i) const {
    assert (isValid);
    assert (data);
    assert (i >= 0 && i <= length);
    return (*data)[i];
  }

  /////////////////////////////////////////////////////////////////
  // Sequence::SetLabel()
  //
  // Sets the sequence label to i.
  /////////////////////////////////////////////////////////////////

  void SetLabel (int i){
    assert (isValid);
    sequenceLabel = i;
    inputLabel = i;
  }

  /////////////////////////////////////////////////////////////////
  // Sequence::SetSortLabel()
  //
  // Sets the sequence sorting label to i.
  /////////////////////////////////////////////////////////////////

  void SetSortLabel (int i){
    assert (isValid);
    sequenceLabel = i;
  }

  /////////////////////////////////////////////////////////////////
  // Sequence::GetLabel()
  //
  // Retrieves the input label.
  /////////////////////////////////////////////////////////////////

  int GetLabel () const {
    assert (isValid);
    return inputLabel;
  }

  /////////////////////////////////////////////////////////////////
  // Sequence::GetSortLabel()
  //
  // Retrieves the sorting label.
  /////////////////////////////////////////////////////////////////

  int GetSortLabel () const {
    assert (isValid);
    return sequenceLabel;
  }

  /////////////////////////////////////////////////////////////////
  // Sequence::Fail()
  //
  // Checks to see if the sequence successfully loaded.
  /////////////////////////////////////////////////////////////////

  bool Fail () const {
    return !isValid;
  }

  /////////////////////////////////////////////////////////////////
  // Sequence::Length()
  //
  // Returns the length of the sequence.
  /////////////////////////////////////////////////////////////////

  int GetLength () const {
    assert (isValid);
    assert (data);
    return length;
  }

  /////////////////////////////////////////////////////////////////
  // Sequence::WriteMFA()
  //
  // Writes the sequence to outfile in MFA format.  Uses numColumns
  // columns per line.  If useIndex is set to false, then the
  // header is printed as normal, but if useIndex is true, then
  // ">S###" is printed where ### represents the sequence label.
  /////////////////////////////////////////////////////////////////

  void WriteMFA (ostream &outfile, int numColumns, bool useIndex = false) const {
    assert (isValid);
    assert (data);
    assert (!outfile.fail());

    // print out heading
    if (useIndex)
      outfile << ">S" << GetLabel() << endl;
    else
      outfile << ">" << header << endl;

    // print out character data
    int ct = 1;
    for (; ct <= length; ct++){
      outfile << (*data)[ct];
      if (ct % numColumns == 0) outfile << endl;
    }
    if ((ct-1) % numColumns != 0) outfile << endl;
  }

  /////////////////////////////////////////////////////////////////
  // Sequence::WriteWEB()
  //
  // output for web interfase based on Sequence::WriteMFA()
  /////////////////////////////////////////////////////////////////

  void WriteWEB (ostream &outfile, int numColumns, bool useIndex = false) const {
    assert (isValid);
    assert (data);
    assert (!outfile.fail());

    outfile << "<php ref=\"" << GetLabel() << "\">" << endl;
    outfile << "<name>" << endl;
    // print out heading
    if (useIndex)
      outfile << "S" << GetLabel() << endl;
    else
      outfile << "" << header << endl;

    outfile << "</name>" << endl;

    // print out character data
    outfile << "<sequence>" << endl;
    int ct = 1;
    for (; ct <= length; ct++){
      outfile << (*data)[ct];
      if (ct % numColumns == 0) outfile << endl;
    }
    if ((ct-1) % numColumns != 0) outfile << endl;

    outfile << "</sequence>" << endl;
    outfile << "</php>" << endl;
  }

  /////////////////////////////////////////////////////////////////
  // Sequence::Clone()
  //
  // Returns a new deep copy of the seqeuence.
  /////////////////////////////////////////////////////////////////

  Sequence *Clone () const {
    Sequence *ret = new Sequence();
    assert (ret);

    ret->isValid = isValid;
    ret->header = header;
    ret->data = new SafeVector<char>; assert (ret->data);
    *(ret->data) = *data;
    ret->length = length;
    ret->sequenceLabel = sequenceLabel;
    ret->inputLabel = inputLabel;
    ret->weight = weight;

    return ret;
  }

  /////////////////////////////////////////////////////////////////
  // Sequence::GetRange()
  //
  // Returns a new sequence object consisting of a range of
  // characters from the current seuquence.
  /////////////////////////////////////////////////////////////////

  Sequence *GetRange (int start, int end) const {
    Sequence *ret = new Sequence();
    assert (ret);

    assert (start >= 1 && start <= length);
    assert (end >= 1 && end <= length);
    assert (start <= end);

    ret->isValid = isValid;
    ret->header = header;
    ret->data = new SafeVector<char>; assert (ret->data);
    ret->data->push_back ('@');
    for (int i = start; i <= end; i++)
      ret->data->push_back ((*data)[i]);
    ret->length = end - start + 1;
    ret->sequenceLabel = sequenceLabel;
    ret->inputLabel = inputLabel;

    return ret;
  }

  /////////////////////////////////////////////////////////////////
  // Sequence::AddGaps()
  //
  // Given an SafeVector<char> containing the skeleton for an
  // alignment and the identity of the current character, this
  // routine will create a new sequence with all necesssary gaps added.
  // For instance,
  //    alignment = "XXXBBYYYBBYYXX"
  //    id = 'X'
  // will perform the transformation
  //    "ATGCAGTCA" --> "ATGCC---GT--CA"
  //                    (XXXBBYYYBBYYXX)
  /////////////////////////////////////////////////////////////////

  Sequence *AddGaps (SafeVector<char> *alignment, char id){
    Sequence *ret = new Sequence();
    assert (ret);

    ret->isValid = isValid;
    ret->header = header;
    ret->data = new SafeVector<char>; assert (ret->data);
    ret->length = (int) alignment->size();
    ret->sequenceLabel = sequenceLabel;
    ret->inputLabel = inputLabel;
    ret->data->push_back ('@');

    SafeVector<char>::iterator dataIter = data->begin() + 1;
    for (SafeVector<char>::iterator iter = alignment->begin(); iter != alignment->end(); ++iter){
      if (*iter == 'B' || *iter == id){
        ret->data->push_back (*dataIter);
        ++dataIter;
      }
      else
        ret->data->push_back ('-');
    }

    return ret;
  }

  /////////////////////////////////////////////////////////////////
  // Sequence::AddGaps()
  //
  // Given an SafeVector<char> containing the skeleton for an
  // alignment and the identity of the current character, this
  // routine will create a new sequence with all necesssary gaps added.
  // For instance,
  //    alignment = "XXXBBYYYBBYYXX"
  //    id = 'X'
  // will perform the transformation
  //    "ATGCAGTCA" --> "ATGCC---GT--CA"
  //                    (XXXBBYYYBBYYXX)
  /////////////////////////////////////////////////////////////////
  Sequence *AddGapsReverse (SafeVector<char> *alignment, char id){
    Sequence *ret = new Sequence();
    assert (ret);

    ret->isValid = isValid;
    ret->header = header;
    ret->data = new SafeVector<char>; assert (ret->data);
    ret->length = (int) alignment->size();
    ret->sequenceLabel = sequenceLabel;
    ret->inputLabel = inputLabel;
    ret->data->push_back ('@');

    SafeVector<char>::iterator dataIter = data->begin() + 1;
    for (SafeVector<char>::reverse_iterator iter = alignment->rbegin(); iter != alignment->rend(); ++iter){
      if (*iter == 'B' || *iter == id){
        ret->data->push_back (*dataIter);
        ++dataIter;
      }
      else
        ret->data->push_back ('-');
    }

    return ret;
  }


  /////////////////////////////////////////////////////////////////
  // Sequence::GetString()
  //
  // Returns the sequence as a string with gaps removed.
  /////////////////////////////////////////////////////////////////

  string GetString (){
    string s = " ";
    for (int i = 1; i <= length; i++){
      if ((*data)[i] != '-') s += (*data)[i];
    }
    return s;
  }


  /////////////////////////////////////////////////////////////////
  // Sequence::GetMapping()
  //
  // Returns a SafeVector<int> containing the indices of every
  // character in the sequence.  For instance, if the data is
  // "ATGCC---GT--CA", the method returns {1,2,3,4,5,9,10,13,14}.
  /////////////////////////////////////////////////////////////////

  SafeVector<int> *GetMapping () const {
    SafeVector<int> *ret = new SafeVector<int>(1, 0);
    for (int i = 1; i <= length; i++){
      if ((*data)[i] != '-') ret->push_back (i);
    }
    return ret;
  }

  /////////////////////////////////////////////////////////////////
  // Sequence::GetMappingNumber()
  //
  // Returns a SafeVector<int> containing the indices of every
  // character in the sequence.  For instance, if the data is
  // "ATGCC---GT--CA", the method returns {1,2,3,4,5,0,0,0,6,7,0,0,8,9}.
  /////////////////////////////////////////////////////////////////
  SafeVector<int> *GetMappingNumber () const {
      SafeVector<int> *ret = new SafeVector<int>(1, 0);
      int count = 0;
      for(int i = 1; i <= length; i++) {
	  if((*data)[i] != '-') ret->push_back(++count);
	  else                  ret->push_back(0);
      }
      return ret;
  }

  /////////////////////////////////////////////////////////////////
  // Sequence::Highlight()
  //
  // Changes all positions with score >= cutoff to upper case and
  // all positions with score < cutoff to lower case.
  /////////////////////////////////////////////////////////////////

  void Highlight (const SafeVector<float> &scores, const float cutoff){
    for (int i = 1; i <= length; i++){
      if (scores[i-1] >= cutoff)
        (*data)[i] = toupper ((*data)[i]);
      else
        (*data)[i] = tolower ((*data)[i]);
    }
  }
};
}
#endif // __SQUENCE_HPP__
