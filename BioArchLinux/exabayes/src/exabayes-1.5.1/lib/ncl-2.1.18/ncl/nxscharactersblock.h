//	Copyright (C) 1999-2003 Paul O. Lewis
//
//	This file is part of NCL (Nexus Class Library) version 2.0.
//
//	NCL is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	NCL is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with NCL; if not, write to the Free Software Foundation, Inc.,
//	59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//

#ifndef NCL_NXSCHARACTERSBLOCK_H
#define NCL_NXSCHARACTERSBLOCK_H

#include <sstream>
#include <cfloat>
#include <climits>

#include "ncl/nxsdefs.h"
#include "ncl/nxsdiscretedatum.h"
#include "ncl/nxstaxablock.h"


class NxsTaxaBlockAPI;
class NxsAssumptionsBlockAPI;
class NxsDiscreteDatatypeMapper;

void NxsWriteSetCommand(const char *cmd, const NxsUnsignedSetMap & usetmap, std::ostream &out, const char * nameOfDef = NULL);
void NxsWritePartitionCommand(const char *cmd, const NxsPartitionsByName &partitions, std::ostream & out, const char * nameOfDef = NULL);

/*! Internal representation of a stepmatrix with cells composed of doubles.
  The ordering of columns and rows is identical, and is accessible via NxsRealStepMatrix::GetSymbols
*/
class NxsRealStepMatrix
{
 public:
  typedef std::vector<double> DblVec;
  typedef std::vector<DblVec> DblMatrix;

 NxsRealStepMatrix(const std::vector<std::string> &symbolsOrder, const DblMatrix & mat)
   :symbols(symbolsOrder),
    matrix(mat)
    {
    }

  const std::vector<std::string> & GetSymbols() const
  {
    return symbols;
  }

  const DblMatrix & GetMatrix() const
  {
    return matrix;
  }
 private:
  std::vector<std::string> symbols;
  DblMatrix matrix;
};

/*! Internal representation of a stepmatrix with cells composed of ints.
  The ordering of columns and rows is identical, and is accessible via NxsRealStepMatrix::GetSymbols
*/
class NxsIntStepMatrix
{
 public:
  typedef std::vector<int> IntVec;
  typedef std::vector<IntVec> IntMatrix;

 NxsIntStepMatrix(const std::vector<std::string> &symbolsOrder, const IntMatrix & mat)
   :symbols(symbolsOrder),
    matrix(mat)
    {
    }
  const std::vector<std::string> & GetSymbols() const
  {
    return symbols;
  }
  const IntMatrix & GetMatrix() const
  {
    return matrix;
  }
 private:
  std::vector<std::string> symbols;
  IntMatrix matrix;
};

/* Work in progress...
 */
class NxsGeneticCodesManager
{
 public:
  NxsGeneticCodesManager();
  void Reset() {}
  bool IsEmpty() const
  {
    return true;
  }
  void WriteGeneticCode(std::ostream &	) const
  {}
  bool IsValidCodeName(const std::string &cn) const;
 protected:
  std::set<std::string> standardCodeNames;
  std::set<std::string> userDefinedCodeNames;

};


/*! NEXUS "types" (as in UserType and TypeSet commands) are assumptions about the costs of transformations of state (in
  parsimony.

*/
class NxsTransformationManager
{
 public:
  typedef std::pair<int, std::set<unsigned> > IntWeightToIndexSet;
  typedef std::list<IntWeightToIndexSet> ListOfIntWeights;

  typedef std::pair<double, std::set<unsigned> > DblWeightToIndexSet;
  typedef std::list<DblWeightToIndexSet> ListOfDblWeights;

  typedef std::pair<std::string, std::set<unsigned> > TypeNameToIndexSet;
  typedef std::list<TypeNameToIndexSet> ListOfTypeNamesToSets;


  NxsTransformationManager()
    {
      Reset();
    }

  /*! \return the weight for a character index from a weight set */
  static int GetWeightForIndex(const ListOfIntWeights & wtset, /*!< the weight set */
			       unsigned index); /*!< character index which should be in the range in [0, nchar) */

  /*! \returns the weight for a character index from a weight set */
  static double GetWeightForIndex(const ListOfDblWeights & wtset, /*!< the weight set */
				  unsigned index); /*!< character index which should be in the range in [0, nchar) */


  /*! \returns an integer step matrix for an ordered type with `nStates` states */
  static const NxsIntStepMatrix::IntMatrix GetOrderedType(unsigned nStates);
  /*! \returns an integer step matrix for an unorder type with `nStates` states (a matrix of 1's off the diagonal and 0's on the diagonal.*/
  static const NxsIntStepMatrix::IntMatrix GetUnorderedType(unsigned nStates);

  /*! \returns a set with all of the registered type names (the name will be all caps, not in the same case as the used in the NEXUS file)*/
  const std::set<std::string> & GetTypeNames() const;
  /*! \returns a set with all of the registered type names that were defined with UserType */
  const std::set<std::string> & GetUserTypeNames() const;
  /*! \returns a set with all of the builtin type names*/
  const std::set<std::string> & GetStandardTypeNames() const;
  /*! \returns the name of the current default type (starts as unorderd but can be overridden by a TypeSet command */
  const std::string GetDefaultTypeName() const;
  /*! \returns a set with all of the WtSet names */
  std::set<std::string> GetWeightSetNames() const;
  /*! \returns true if the name corresponds to a WtSet that is has double entries
    If true, access the set via GetDoubleWeights
    If false, retrieve the set by GetIntWeights
  */
  bool IsDoubleWeightSet(const std::string &) const;

  /*! \returns the default ("active") weights as doubles. If the list is empty then the default weights are available from GetDefaultIntWeights or have not been set (implying equal weights) */
  std::vector<double> GetDefaultDoubleWeights() const
    {
      return GetDoubleWeights(def_wtset);
    }

  /*! \returns the default ("active") weights as doubles. If the list is empty then the default weights are available from GetDefaultDoubleWeights or have not been set (implying equal weights) */
  std::vector<int> GetDefaultIntWeights() const
    {
      return GetIntWeights(def_wtset);
    }

  /*! \returns the double weights assocaited with the WtSet with name `wtsetname`.

    If the list is empty then the default weights are available from GetIntWeights or
    wtsetname is not the name of o WtSet
  */
  std::vector<double> GetDoubleWeights(const std::string &wtsetname) const;
  /*! \returns the int weights assocaited with the WtSet with name `wtsetname`.

    If the list is empty then the default weights are available from GetDoubleWeights or
    wtsetname is not the name of o WtSet
  */
  std::vector<int> GetIntWeights(const std::string &) const;

  /*! \returns a set with all of the TypeSet names */
  std::set<std::string> GetTypeSetNames() const;

  /*! \returns the name of the active ("default") WtSet */
  const std::string & GetDefaultWeightSetName() const;
  /*! \returns the name of the active ("default") TypeSet */
  const std::string & GetDefaultTypeSetName() const;

  bool IsEmpty() const;

  bool IsValidTypeName(const std::string & ) const;
  bool IsStandardType(const std::string & ) const;
  bool IsIntType(const std::string & ) const;

  const NxsIntStepMatrix & GetIntType(const std::string & name) const;

  const NxsRealStepMatrix & GetRealType(const std::string & name) const;


  void SetDefaultTypeName(const std::string &);
  bool AddIntType(const std::string &, const NxsIntStepMatrix &);
  bool AddRealType(const std::string &, const NxsRealStepMatrix &);

  bool AddIntWeightSet(const std::string &, const ListOfIntWeights &, bool isDefault);
  bool AddRealWeightSet(const std::string &, const ListOfDblWeights &, bool isDefault);

  bool AddTypeSet(const std::string &, const NxsPartition &, bool isDefault);

  void Reset();

  void WriteUserType(std::ostream &out) const;
  void WriteWtSet(std::ostream &out) const;
  void WriteTypeSet(std::ostream &out) const
  {
    NxsWritePartitionCommand("TypeSet", typeSets, out, def_typeset.c_str());
  }

 private:
  std::set<std::string> standardTypeNames;
  std::set<std::string> userTypeNames;
  std::set<std::string> allTypeNames;
  std::map<std::string, NxsRealStepMatrix> dblUserTypes;
  std::map<std::string, NxsIntStepMatrix> intUserTypes;
  std::set<std::string> allWtSetNames;
  std::map<std::string, ListOfDblWeights> dblWtSets;
  std::map<std::string, ListOfIntWeights> intWtSets;
  NxsPartitionsByName typeSets;
  std::string def_wtset;
  std::string def_typeset;
  std::string def_type;
};

inline const std::string NxsTransformationManager::GetDefaultTypeName() const
{
  return def_type;
}
inline const std::string & NxsTransformationManager::GetDefaultWeightSetName() const
{
  return def_wtset;
}
inline const std::string & NxsTransformationManager::GetDefaultTypeSetName() const
{
  return def_typeset;
}
inline const std::set<std::string> & NxsTransformationManager::GetTypeNames() const
{
  return allTypeNames;
}
inline const std::set<std::string> & NxsTransformationManager::GetUserTypeNames() const
{
  return userTypeNames;
}
inline const std::set<std::string> & NxsTransformationManager::GetStandardTypeNames() const
{
  return standardTypeNames;
}
inline bool NxsTransformationManager::IsDoubleWeightSet(const std::string &s) const
{
  const std::vector<double> d = GetDoubleWeights(s);
  return !(d.empty());
}

/*! Intended to specify the interface of a NxsCharactersBlock, but actually does
  not list all of the relevant functions. See NxsCharactersBlock documentation.
*/
class NxsCharactersBlockAPI
: public NxsBlock, public NxsLabelToIndicesMapper
{
 public:
  virtual unsigned	ApplyExset(NxsUnsignedSet &exset) = 0;
  virtual bool AddNewExSet(const std::string &label, const NxsUnsignedSet & inds) = 0;
  virtual bool IsRespectCase() const = 0;
  virtual unsigned	GetNCharTotal() const = 0;
  virtual NxsTransformationManager & GetNxsTransformationManagerRef() = 0;
  virtual const NxsTransformationManager & GetNxsTransformationManagerRef() const = 0;
  virtual std::vector<const NxsDiscreteDatatypeMapper *> GetAllDatatypeMappers() const = 0;
  virtual bool AddNewCodonPosPartition(const std::string &label, const NxsPartition & inds, bool isDefault) = 0;
  virtual std::string GetDefaultCodonPosPartitionName() const = 0;
  virtual NxsPartition GetCodonPosPartition(const std::string &label) const = 0;
  enum GapModeEnum
  {
    GAP_MODE_MISSING = 0,
    GAP_MODE_NEWSTATE = 1
  };
  virtual GapModeEnum GetGapModeSetting() const = 0;
  virtual void SetGapModeSetting(GapModeEnum m) = 0;

};

#if defined(NCL_SMALL_STATE_CELL)
typedef signed char NxsDiscreteStateCell;
#else
typedef int NxsDiscreteStateCell;
#endif
typedef std::vector<NxsDiscreteStateCell> NxsDiscreteStateRow;
typedef std::vector<NxsDiscreteStateRow> NxsDiscreteStateMatrix;


/*!
  NXS_INVALID_STATE_CODE is used as a flag for uninitialized or unrecognized values
  NXS_GAP_STATE_CODE may not be found in all datatypes, but is always -2 when
  it present.
  NXS_MISSING_CODE is always -1. It must be distinguished from the ambiguous set of all states because ? does not
  mean that a new state could necessarily be present. This arises is PAUP-style symbols extensions to the
  built-in datatypes. If you say FORMAT DATATYPE=DNA SYMBOLS="01" ; then the valid symbols become "ACGT01"
  See AugmentedSymbolsToMixed.
*/
enum {
  NXS_INVALID_STATE_CODE = -3, /* this must be kept negative */
  NXS_GAP_STATE_CODE = -2, /* this must be kept negative */
  NXS_MISSING_CODE = -1 /* this must be kept negative */
};

class NxsCodonTriplet {
 public:
  unsigned char firstPos;
  unsigned char secondPos;
  unsigned char thirdPos;

  NxsCodonTriplet(const char *triplet);
  ////////////////////////////////////////////////////////////////////////
  // returns for a this => other substitution a (from-base, to-base) pair or
  // (-1,-1) for codons that differ by more than one position.
  // If codons are identical, then (0,0) will be returned.
  //
  typedef std::pair<int, int> MutDescription;
  MutDescription getSingleMut(const NxsCodonTriplet & other) const;

};

enum NxsGeneticCodesEnum {
  NXS_GCODE_NO_CODE = -1,
  NXS_GCODE_STANDARD = 0,
  NXS_GCODE_VERT_MITO = 1,
  NXS_GCODE_YEAST_MITO = 2,
  NXS_GCODE_MOLD_MITO = 3,
  NXS_GCODE_INVERT_MITO = 4,
  NXS_GCODE_CILIATE = 5,
  NXS_GCODE_ECHINO_MITO = 8,
  NXS_GCODE_EUPLOTID = 9,
  NXS_GCODE_PLANT_PLASTID = 10,
  NXS_GCODE_ALT_YEAST = 11,
  NXS_GCODE_ASCIDIAN_MITO = 12,
  NXS_GCODE_ALT_FLATWORM_MITO = 13,
  NXS_GCODE_BLEPHARISMA_MACRO = 14,
  NXS_GCODE_CHLOROPHYCEAN_MITO = 15,
  NXS_GCODE_TREMATODE_MITO = 20,
  NXS_GCODE_SCENEDESMUS_MITO = 21,
  NXS_GCODE_THRAUSTOCHYTRIUM_MITO = 22,
  NXS_GCODE_CODE_ENUM_SIZE = 23
};
NxsGeneticCodesEnum geneticCodeNameToEnum(std::string);
std::string geneticCodeEnumToName(NxsGeneticCodesEnum);
std::string getGeneticCodeAAOrder(NxsGeneticCodesEnum codeIndex);
std::vector<std::string> getGeneticCodeNames();

/* structure used to store information about how the codon indices of a compressed
   (no stop codons permitted) character matrix correspond to the:
   * 64 codons in alphabetical order,
   * the amino acids
   * the codon strings ("AAA", "AAC"...)
   */

class CodonRecodingStruct
{
 public:
  std::vector<int> compressedCodonIndToAllCodonsInd;
  std::vector<int> aaInd; /* The index 0 to 20 of the amino acid for each codon - the order of the aas is "ACDEFGHIKLMNPQRSTVWY*" */
  std::vector<std::string> codonStrings; /* The nucleotide abbreviations for each codon "AAA", "AAC"... */
};

class NxsDiscreteDatatypeMapper;
/*! This class handles reading and storage for the NEXUS block CHARACTERS. It overrides the member functions Read and
  Reset, which are abstract virtual functions in the base class NxsBlock.



  \note{"ActiveChar" is equivalent to "IncludedChar". }


  Important change in starting in version 2.1:
  The ELIMINATE command is now dealt with as if it were an automatic exclude statment.
  Previous versions of NCL were more in keeping with the NEXUS specification, in that NCL did not store
  eliminate characters. This resulted
  in a confusing situation in which the Characters block maintained an original index for a character and a
  current index.	Some public functions of NxsCharactersBlock took arguments that needed the original character
  index, while the vast majority of methods interpretted a character index as the current index of a character.
  ELIMINATE commands are *very* rare in modern NEXUS files (Mesquite does not even recognize the command), thus
  the increased complexity of the API that was caused by not storing ELIMINATED character was deemed a
  counterproductive.
  In NCL 2.1 (and later), the characters block stores every character, and the user of NCL can query the
  NxsCharactersBlock about whether a character has been excluded or not (you can also ask for the set of
  eliminated chararcters). Optimizations for avoiding excluded characters are no longer the responsibility
  of NxsCharactersBlock.

  Thus, a normal loop through all characters in the data matrix should look something
  like this:
  >
  for(unsigned j = 0; j < nchar; j++)
  {
  if (IsExcluded(j))
  continue;
  .
  .
  .
  }

  Below is a table showing the correspondence between the elements of a CHARACTERS block in a NEXUS file and the
  variables and member functions of the NxsCharactersBlock class that can be used to access each piece of information
  stored. Items in parenthesis should be viewed as "see also" items.
  >
  NEXUS		  Command		 Data			Member
  Command		  Atribute		 Member			Functions
  ---------------------------------------------------------------------
  DIMENSIONS	  NEWTAXA		 newtaxa


  NCHAR			 nChar			GetNChar

  FORMAT		  DATATYPE		 datatype		GetDataType

  RESPECTCASE	 respectingCase IsRespectCase

  MISSING		 missing		GetMissingSymbol

  GAP			 gap			GetGapSymbol

  SYMBOLS		 symbols		GetSymbols

  EQUATE		 userEquates	GetEquateKey
  GetEquateValue
  GetNumEquates

  MATCHCHAR		 matchchar		GetMatchcharSymbol

  (NO)LABELS	 labels			IsLabels

  TRANSPOSE		 transposing	IsTranspose

  INTERLEAVE	 interleaving	IsInterleave

  ITEMS							GetItems

  STATESFORMAT					GetStatesPresent

  (NO)TOKENS	 tokens			IsTokens

  ELIMINATE					 eliminated		GetNumEliminated
  IsExcluded
  MATRIX						 matrix			GetState
  GetInternalRepresentation
  GetNumStates
  GetNumMatrixRows
  GetNumMatrixCols
  IsPolymorphic
  >
*/
class NxsCharactersBlock
: public NxsCharactersBlockAPI, public NxsTaxaBlockSurrogate
{
  friend class NxsAssumptionsBlock;


 public:
  typedef std::map<std::string, std::vector<double> > ContinuousCharCell;
  typedef std::vector<ContinuousCharCell> ContinuousCharRow;
  typedef std::vector<ContinuousCharRow> ContinuousCharMatrix;
  typedef std::vector<std::string> VecString;
  typedef std::map<unsigned, std::string> IndexToLabelMap;
  typedef std::map<std::string, unsigned> LabelToIndexMap;
  typedef std::pair<NxsDiscreteDatatypeMapper, NxsUnsignedSet> DatatypeMapperAndIndexSet;
  typedef std::vector<DatatypeMapperAndIndexSet> VecDatatypeMapperAndIndexSet;


  enum DataTypesEnum /*! values used to represent different basic types of data stored in a CHARACTERS block, and used with the data member `datatype' */
  {
    standard = 1, /*! indicates `matrix' holds characters with arbitrarily-assigned, discrete states, such as discrete morphological data */
    dna = 2, /*! indicates `matrix' holds DNA sequences (states A, C, G, T) */
    rna = 3, /*! indicates `matrix' holds RNA sequences (states A, C, G, U) */
    nucleotide = 4, /*! indicates `matrix' holds nucleotide sequences */
    protein = 5, /*! indicates `matrix' holds amino acid sequences */
    continuous = 6, /*! indicates `matrix' holds continuous data */
    codon = 7, /*! AAA=>0, AAC=1, AAAG=>2, AAU=>3, ACA=>4... UUU=>63 */
    mixed = 8 /*! indicates that there are multiple datatype mappers that must be used to decode the columns of the matrix (one mapper per column, but not one mapper per matrix). A MrBayes NEXUS feature*/
  };
  enum StatesFormatEnum
  {
    STATES_PRESENT = 1,
    STATE_COUNT,
    STATE_FREQUENCY,
    INDIVIDUALS
  };

  /*! In v2.1 of the API, the NxsTaxaBlockAPI and NxsAssumptionsBlockAPI pointers
    are usually NULL. These block assignments are made during the parse. */
  NxsCharactersBlock(NxsTaxaBlockAPI *tb, /*!< the taxa block object to consult for taxon labels (can be 0L)*/
		     NxsAssumptionsBlockAPI *ab);	/*!< the assumptions block object to consult for exclusion sets (can be 0L) */
  virtual ~NxsCharactersBlock() {}

  // Commonly used functions
  //Configuration
  /*! Controls whether or not a characters block reader will support MrBayes' datatype=MIXED extension to NEXUS
    The default is false.
    \ref mixedDatatypes
  */
  void SetSupportMixedDatatype(bool v)
  {
    supportMixedDatatype = v;
  }
  /*! \returns true if a sequence type will be converted to standard (default block would return false).
    the "setter" function is NxsCharactersBlock::SetConvertAugmentedToMixed()

    \ref mixedDatatypes
  */
  bool AugmentedSymbolsToMixed();
  /*! Instructs the NxsCharactersBlock to convert sequence data character blocks that have
    "augmented" symbols lists into a mixture datatype.

    By default or after SetConvertAugmentedToMixed(false), the NxsCharacterBlock will change
    the datatype to standard (to indicate that the datatype is no longer simply a sequence type).

    Note that GetOriginalDataType() will still store the name of the type that occurred in the file.

    This is only applicable if SetAllowAugmentingOfSequenceSymbols(true) has been called.
    \ref mixedDatatypes
  */
  void SetConvertAugmentedToMixed(bool v)
  {
    convertAugmentedToMixed = v;
  }
  /*! Instructs the NxsCharactersBlock to accept extra symbols even if the datatype is
    declared to be sequence data character blocks that have
    "augmented" symbols lists into the standard datatype.
  */
  void SetAllowAugmentingOfSequenceSymbols(bool v)
  {
    allowAugmentingOfSequenceSymbols = v;
  }

  /*! \retutrns the setting of allowAugmentingOfSequenceSymbols.
    The default is false.
    \ref mixedDatatypes
  */
  bool GetAllowAugmentingOfSequenceSymbols() const
  {
    return allowAugmentingOfSequenceSymbols;
  }
  /*! \returns a data structure that allows you to identify the set of character
    indices (each element in [0, nchar) range). If the type is not mixed, then
    the map may be empty.
  */
  std::map<DataTypesEnum, NxsUnsignedSet> GetDatatypeMapForMixedType() const
    {
      return mixedTypeMapping;
    }


  /*! \returns a facet of the DataTypesEnum that indicates the general type
    of data. Because symbols can be augmented non-default polymorphism codes
    can be introduced, this is not a complete description of the datatype
    encoding.

    If you want to test if the internal representation datatype of a NxsCharactersBlock
    has been modified use some form of this idiom:

    mapper = charBlock.GetDatatypeMapperForChar(0);
    const bool hasGaps = charBlock.GetGapSymbol() != '\0';
    NxsDiscreteDatatypeMapper defaultMapper(NxsCharactersBlock::dna, hasGaps);
    if (mapper->IsSemanticallyEquivalent(&defaultMapper))
    {
    // the datatype has not been changed in a substantive way.
    }
    else
    {
    // new state codes have been introduced, so routines that do not
    //	interrogate the mapper to check the mapper about the status of all of the
    //	states may fail.
    }



  */
  DataTypesEnum GetDataType() const;
  /*! Returns true If character `taxInd' has some stored character state. Assumes `taxInd' is in the range [0..`nchar').
   */
  bool TaxonIndHasData(const unsigned ind) const;
  /*! \returns the number of characters stored in the block.*/
  unsigned GetNCharTotal() const ;
  /*! Returns datatype listed in the CHARACTERS block.
    The original datatype can differ from the current datatype if the datatype
    was listed as a sequence type, but the symbols list of a built in type was augmented
    (thus converting it to standard).
    This will only happen if SetAllowAugmentingOfSequenceSymbols(true) has been called on the block.
    see \ref mixedDatatypes
  */
  DataTypesEnum GetOriginalDataType() const;
  /*! Returns the number of characters that have not been exclude (via exset or eliminate command, for
    example).
    Synonymous with GetNumIncludedChars and (GetNChar - GetNumEliminated)
  */
  unsigned GetNumActiveChar() NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
  /*! \returns a reference to the set of indices that are currently excluded. */
  const NxsUnsignedSet & GetExcludedIndexSet() const;
  /*! \returns true if character `j' is active. If character `j' has been excluded, returns false.
    Assumes `j' is in the range [0..`nchar')
  */
  bool IsActiveChar(unsigned j) const;
  /*!  excludes all of the  indices in exset.

    indices should be in the range [0, nchar)
  */
  unsigned ApplyExset(NxsUnsignedSet &exset);
  /*!  includes all of the  indices in exset.

    indices should be in the range [0, nchar)
  */
  unsigned ApplyIncludeset(NxsUnsignedSet &inset);
  /*!  excludes character `i`

    i should be in the range [0, nchar)
  */
  void ExcludeCharacter(unsigned i);
  /*!  includes character `i`

    i should be in the range [0, nchar)
  */
  void IncludeCharacter(unsigned i);

  /*! \returns the label for character `i` has a label or a string with a single space if the is no label*/
  NxsString GetCharLabel(unsigned i) const; /*v2.1to2.2 4 */
  /*! \returns true if charlabels were stored for the matrix*/
  bool HasCharLabels() const;
  /*! \returns the current gapMode setting
    During a parse this is controlled by the OPTIONS command in the ASSUMPTIONS block).
    \note{The gapmode setting basically just holds the value for the client code's convenience.
    It only affects post-read operations, such as NxsCharactersBlock::GetObsStates.
    It does NOT change the internal encoding of the data (just triggers some filtering) of the data}
  */
  GapModeEnum GetGapModeSetting() const
  {
    return this->gapMode;
  }

  //v2.0 API that queries based on symbols see \ref NxsCharacterBlockQueries
  const char *GetSymbols() NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
  unsigned GetNumStates(unsigned i, unsigned j) NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
  char GetState(unsigned i, unsigned j, unsigned k = 0) const;
  /*! \returns a reference to the Transformation Manager that stores information such as character transformation type (ORDERED, UNORDERED...)*/
  const NxsTransformationManager & GetNxsTransformationManagerRef() const
  {
    return transfMgr;
  }
  /*! Returns true iff taxon `taxInd` has a gap for character `charInd` (both indices 0-based)*/
  bool IsGapState(unsigned taxInd, unsigned charInd) NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
  /*! Returns true iff taxon `taxInd` has is missing for character `charInd` (both indices 0-based) */
  bool IsMissingState(unsigned i, unsigned j) NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
  /*! Returns true iff taxon `taxInd` has is missing for character `charInd` (both indices 0-based) */
  bool IsPolymorphic(unsigned i, unsigned j) NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */

  //v2.1 API for extracting character info
  // for discrete characters
  /*! \returns a const reference to the row of state codes for the taxon.

    This enables faster access to the data. See \ref newerCharQueries

    The row should not be modified by the caller.

    taxonIndex should be in the range [0, ntax)
  */
  const NxsDiscreteStateRow & GetDiscreteMatrixRow(unsigned taxonIndex) const;
  /* \returns a pointer to the the NxsDiscreteDatatypeMapper that "knows" how the
     internal state code labellings corrspond to symbols.

     If the datatype is not mixed, then the same instance will apply regardless of
     character index.

     charIndex should be in the range [0, nchar)
  */
  const NxsDiscreteDatatypeMapper * GetDatatypeMapperForChar(unsigned charIndex) const;
  /*! \returns a vector of all of the NxsDiscreteDatatypeMapper used by any char in the matrix.

    If the datatype is not mixed, then the vector should have length 1.

    See GetDatatypeMapForMixedType().
  */
  std::vector<const NxsDiscreteDatatypeMapper *> GetAllDatatypeMappers() const;

  // For continuous characters
  /*! \returns the vector of values for the appropriate item `key` in the indicated cell of the matrix.

    For continuous data matrices, each cell of the matrix can store multiple "items"
    such as the average, range, max...
    This function retrieves the collection of data for the taxon `taxIndex`
    and the character `taxIndex` and then returns the values for the item designated by `key`

    GetItems() returns the list of all possible items
  */
  std::vector<double> GetContinuousValues(unsigned taxIndex, unsigned charIndex, const std::string key) NCL_COULD_BE_CONST; /*v2.1to2.2 1 */
  /*! \returns vector of items stored for each cell. (this is mainly relevant for continouos data).
    For discrete data, only "STATES" is supported.
  */
  std::vector<std::string> GetItems() const;
  /*! \returns a facet of the StatesFormatEnum to indicate what the states mean for a cell.

    For discrete data, only STATES_PRESENT is accepted by NCL.
    For continuous matrices either STATES_PRESENT or INDIVIDUALS will be accepted.
  */
  StatesFormatEnum GetStatesFormat() const;



  // Functions that are used by many NCL-clients, but are often not needed
  void FindConstantCharacters(NxsUnsignedSet &c) const;
  void FindGappedCharacters(NxsUnsignedSet &c) const;
  virtual const std::string & GetBlockName() const;
  /*! sets the current gapMode setting.
    During a parse this is controlled by the OPTIONS command in the ASSUMPTIONS block).
    \note{The gapmode setting basically just holds the value for the client code's convenience.
    It only affects post-read operations, such as NxsCharactersBlock::GetObsStates.
    It does NOT change the internal encoding of the data (just triggers some filtering) of the data}
  */
  void SetGapModeSetting(GapModeEnum m)
  {
    this->gapMode = m;
  }
  static const char * GetNameOfDatatype(DataTypesEnum);
  NxsDiscreteStateCell GetInternalRepresentation(unsigned i, unsigned j, unsigned k = 0) NCL_COULD_BE_CONST; /*v2.1to2.2 1 */
  /*! \returns the maximum observed number of states for any character.
    \note{this function is slow}

    If `onlyActiveChars` is true then calculation will skip characters that have been excluded (eg. by an exset).

  */
  virtual unsigned GetMaxObsNumStates(bool countMissingStates=true, bool onlyActiveChars=false) NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
  /*! \returns the number of states observed in a column
    If `countMissingStates` is true then missing data is treated as an observation of each state. If
    it is false then missing codes do not contribute to the count.
    Partially ambiguous states result in all states in the cell being counted as observed.

    columnIndex should be in [0, nchar)
    \warning{If countMissingStates is true then it a gap as a state, if countMissingStates is false
    the the gap will count as a state if the gapmode is GAP_MODE_NEWSTATE}
  */
  virtual unsigned GetNumObsStates(unsigned columnIndex, bool countMissingStates=true) NCL_COULD_BE_CONST { /*v2.1to2.2 1 */
    return (unsigned)GetObsStates(columnIndex, countMissingStates).size();
  }
  /*! Returns the set of "fundamental" states seen in a column (possibly including the gap "state").

    If `countMissingStates` is true then missing data is treated as an observation of each state. If
    it is false then missing codes do not contribute to the count.
    Partially ambiguous states result in all states in the cell being counted as observed.

    columnIndex should be in [0, nchar)
    \warning{If countMissingStates is true then it a gap as a state, if countMissingStates is false
    a gap will count as a state if the gapmode is GAP_MODE_NEWSTATE}
  */
  std::set<NxsDiscreteStateCell> GetObsStates(unsigned columnIndex, bool countMissingStates=true) const {
    if (countMissingStates)
      return GetMaximalStateSetOfColumn(columnIndex);
    return GetNamedStateSetOfColumn(columnIndex);
  }

  double GetSimpleContinuousValue(unsigned i, unsigned j) NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */

  /*! \returns label for character state `charStateIndex' at character `charIndex', if a label has been specified. If no label was specified,
    returns string containing a single blank (i.e., " ").

    Both charIndex and charStateIndex should be 0-based
  */
  NxsString GetStateLabel(unsigned charIndex, unsigned charStateIndex) const /*v2.1to2.2 4 */
  {
    return GetStateLabelImpl(charIndex, charStateIndex);
  }
  /*! If a datatype is standard, then it may have been originally specified as "restriction".

    The restriction site datatype was added by MrBayes, It is supported by NCL, but you have
    to call this function to see if the matrix was specified as restriction site data (we did not
    add a facet to the DataTypesEnum to avoid the need to extend other code).
  */
  bool WasRestrictionDataype() const;



  // Rarely-needed functions (usually used mainly internally by NCL)
  static std::map<char, NxsString> GetDefaultEquates(DataTypesEnum);
  static std::string GetDefaultSymbolsForType(DataTypesEnum dt);
  const NxsDiscreteDatatypeMapper & GetDatatypeMapperForCharRef(unsigned charIndex) const;
  /*! \return the datatype as a human-readable string (uses NxsCharactersBlock::GetNameOfDatatype)*/
  const char * GetDatatypeName() const
  {
    return NxsCharactersBlock::GetNameOfDatatype(datatype);
  }
  char GetGapSymbol() const;
  void SetGapSymbol(char);
  char GetMatchcharSymbol() NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
  char GetMissingSymbol() const;
  unsigned GetMaxIndex() const;
  const NxsDiscreteStateMatrix & GetRawDiscreteMatrixRef() const
  {
    return discreteMatrix;
  }


  // Low-level functions (difficult to use, or with potentially suprising behavior). These are used internally by NCL
  virtual unsigned CharLabelToNumber(NxsString s) NCL_COULD_BE_CONST ; /*v2.1to2.2 d */
  virtual unsigned CharLabelToNumber(const std::string & s) const;
  unsigned GetIndexSet(const std::string &label, NxsUnsignedSet * toFill) const
  {
    return NxsLabelToIndicesMapper::GetIndicesFromSets(label, toFill, charSets);
  }
  unsigned GetIndicesForLabel(const std::string &label, NxsUnsignedSet *inds) const;
  unsigned GetNCharTotal() ; // non const version for backwark compat.
  unsigned GetNumIncludedChars() const ; // synonymous with GetNumActiveChar
  unsigned GetNumEliminated() NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */ //backward-compat.
  unsigned GetNChar() const;
  unsigned GetNumChar() const;
  // poor function name -- same as GetNumObsStates. Backward compatibility \deprecated
  virtual unsigned GetObsNumStates(unsigned columnIndex, bool countMissingStates=true) NCL_COULD_BE_CONST { /*v2.1to2.2 1 */
    return (unsigned) GetObsStates(columnIndex, countMissingStates).size();
  }
  /*! Returns label for character state `charStateIndex' at character `charIndex', if a label has been specified. If no label was specified,
    returns string containing a single blank (i.e., " ").
  */
  NxsString GetStateLabel(unsigned charIndex, unsigned charStateIndex) /*v2.1to2.2 4 */ //			non-const version for backward compat

  {
    return GetStateLabelImpl(charIndex, charStateIndex);
  }
  /* It is probably better to ask for the taxa block (via GetTaxaBlockPtr() method) and then have
     the full TaxaBlockAPI to query from rather than ask about the taxa via the characters block.*/
  NxsString GetTaxonLabel(unsigned i) const; /*v2.1to2.2 4 */

  // non-const version
  NxsTransformationManager & GetNxsTransformationManagerRef()
    {
      return transfMgr;
    }
  bool IsActiveChar(unsigned j) ; // non-const version for backward
  bool IsEliminated(unsigned charIndex) NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */ //same as (!IsActive(charIndex))
  bool IsExcluded(unsigned j) const; //same as IsEliminated(j)
  bool IsExcluded(unsigned j) ;  //same as IsEliminated(j) non-const for backward compat.
  bool IsMixedType() const;

  virtual unsigned TaxonLabelToNumber(NxsString s) const; /*v2.1to2.2 4 */
  virtual bool AddNewCodonPosPartition(const std::string &label, const NxsPartition & inds, bool isDefault);
  bool AddNewIndexSet(const std::string &label, const NxsUnsignedSet & inds);
  bool AddNewExSet(const std::string &label, const NxsUnsignedSet & inds);
  bool AddNewPartition(const std::string &label, const NxsPartition & inds);
  void Consume(NxsCharactersBlock &other);
  /*! Behaves like GetMaximalStateSetOfColumn except that missing data columns do not increase
    size of the returned state set.
    This function is sensitive to the gapmode setting. A gap will count as a state if the gapmode is
    GAP_MODE_NEWSTATE.

    columnIndex should be in [0, nchar)
  */
  std::set<NxsDiscreteStateCell> GetNamedStateSetOfColumn(const unsigned colIndex) const;
  /*! Returns the set of "fundamental" states seen in a column (possibly including the gap "state").

    Missing data, gaps, and partially ambiguous cells result in all states in the cell being counted as observed.

    columnIndex should be in [0, nchar)

  */
  std::set<NxsDiscreteStateCell> GetMaximalStateSetOfColumn(const unsigned colIndex) const;




  // Output/writing functions
  virtual void Report(std::ostream &out) NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
  void ShowStateLabels(std::ostream &out, unsigned i, unsigned c, unsigned first_taxon) const;
  void WriteAsNexus(std::ostream &out) const;
  virtual void DebugShowMatrix(std::ostream &out, bool use_matchchar, const char *marginText = NULL) const;
  virtual void WriteLinkCommand(std::ostream &out) const;
  void WriteStatesForTaxonAsNexus(std::ostream &out, unsigned taxNum, unsigned begChar, unsigned endChar) const;
  void WriteCharLabelsCommand(std::ostream &out) const;
  void WriteCharStateLabelsCommand(std::ostream &out) const;
  void WriteEliminateCommand(std::ostream &out) const;
  void WriteFormatCommand(std::ostream &out) const;
  void WriteMatrixCommand(std::ostream &out) const;



  // parsing related functions (used internally by NCL, rarely needed by client code)
  bool IsInterleave() NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
  bool IsLabels() NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
  bool IsRespectCase() const;
  bool IsTokens() NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
  bool IsTranspose() NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
  /* Returns the number of taxa that should be stored. Used during the parse.
     \warning{After parsing, use TaxonIndHasData() for each taxon rather than this function.}
     \todo{After the parse this, GetNTaxWithData() *should* agree with GetNTaxTotal(). We need a
     test for this needed property of }
  */
  unsigned GetNTaxWithData() const ; //
  virtual VecBlockPtr GetImpliedBlocks();
  unsigned GetNumEquates() NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
  unsigned GetNumUserEquates() const;
  unsigned GetNumMatrixCols() NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
  /* Returns the number of taxa (should agree with GetNTaxTotal()) */
  unsigned GetNumMatrixRows() NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
  virtual void Reset();
  void SetNexus(NxsReader *nxsptr);
  const ContinuousCharRow & GetContinuousMatrixRow(unsigned taxNum) const;


  /*only used it the linkAPI is enabled*/
  virtual void HandleLinkCommand(NxsToken & token);



  /*---------------------------------------------------------------------------------------
    | Results in aliasing of the taxa, assumptionsBlock blocks!
  */
  NxsCharactersBlock & operator=(const NxsCharactersBlock &other)
    {
      Reset();
      CopyBaseBlockContents(static_cast<const NxsBlock &>(other));
      CopyTaxaBlockSurrogateContents(other);
      CopyCharactersContents(other);
      return *this;
    }

  virtual void CopyCharactersContents(const NxsCharactersBlock &other);
  virtual NxsCharactersBlock * Clone() const
  {
    NxsCharactersBlock * a = new NxsCharactersBlock(taxa, assumptionsBlock);
    *a = *this;
    return a;
  }


  void SetWriteInterleaveLen(int interleaveLen)
  {
    writeInterleaveLen = interleaveLen;
  }

  std::string GetMatrixRowAsStr(const unsigned rowIndex) const;
  NxsDiscreteStateCell	GetStateIndex(unsigned i, unsigned j, unsigned k) const;


  /** converts a CodonPosPartition into the list<int> argument needed for the NewCodonsCharactersBlock call */
  static void CodonPosPartitionToPosList(const NxsPartition &codonPos, std::list<int> * charIndices);

  /* allocates a new charaters block with all of the active characters in `charBlock`
     but with a 64-state codon datatype. The order of codons is:
     0   1   2   3   4   5  ... 63
     AAA AAC AAG AAT ACA ACC ... TTT
     The caller is responsible for deleting the new NxsCharactersBlock object
     If charIndices is provided, it lists the bases in the RF by position the int can be < 0 to indicate that that position was not sampled
  */
  static NxsCharactersBlock * NewCodonsCharactersBlock(
						       const NxsCharactersBlock * charBlock,
						       bool mapPartialAmbigToUnknown,
						       bool gapsToUnknown,
						       bool honorCharActive, /* if true then inactive characters are treated as missing */
						       const std::list<int> * charIndices = NULL, /* specifies the indices of the positions in the gene. -1 can be used to indicate tha codon position was not included in the original matrix */
						       NxsCharactersBlock ** spareNucs = NULL /* If non-null, then, on exit the NxsCharactersBlock * pointer will refer to a new character block with all of the positions that were not translated (all of the non-coding nucleotide positions) */
						       );
  static NxsCharactersBlock * NewProteinCharactersBlock(
							const NxsCharactersBlock * codonBlock,
							bool mapPartialAmbigToUnknown,
							bool gapToUnknown,
							NxsGeneticCodesEnum codeIndex);
  static NxsCharactersBlock * NewProteinCharactersBlock(
							const NxsCharactersBlock * codonBlock,
							bool mapPartialAmbigToUnknown,
							bool gapToUnknown,
							const std::vector<NxsDiscreteStateCell> & aaIndices); /** the index of the amino acid symbols for the codon (where the order of codons is alphabetical: AAA, AAC, AAG, AAT, ACA, ...TTT **/

  virtual std::string GetDefaultCodonPosPartitionName() const {
    return defCodonPosPartitionName;
  }
  virtual NxsPartition GetCodonPosPartition(const std::string &label) const {
    NxsPartitionsByName::const_iterator pIt = codonPosPartitions.find(label);
    if (pIt == codonPosPartitions.end())
      return NxsPartition();
    return pIt->second;
  }

  unsigned NumAmbigInTaxon(const unsigned taxInd, const NxsUnsignedSet * charIndices, const bool countOnlyCompletelyMissing, const bool treatGapsAsMissing) const;
  bool FirstTaxonStatesAreSubsetOfSecond(const unsigned firstTaxonInd, const unsigned secondTaxonInd, const NxsUnsignedSet * charIndices, const bool treatAmbigAsMissing, const bool treatGapAsMissing) const;
  //Returns the number of characters that differ, and the number of positions for which both taxa were non-missing

  std::pair<unsigned, unsigned> GetPairwiseDist(const unsigned firstTaxonInd, const unsigned secondTaxonInd, const NxsUnsignedSet * charIndices, const bool treatAmbigAsMissing, const bool treatGapAsMissing) const;
  CodonRecodingStruct RemoveStopCodons(NxsGeneticCodesEnum);
  bool SwapEquivalentTaxaBlock(NxsTaxaBlockAPI * tb)
  {
    return SurrogateSwapEquivalentTaxaBlock(tb);
  }

  /*! Writes a range of characater states as NEXUS to out.

   */
  void WriteStatesForMatrixRow(std::ostream &out, /*!< ostream that will be written to.*/
			       unsigned taxon, /*!< index of the row (taxon) to be written.  Should be in [0,ntax). */
			       unsigned first_taxon, /*!< UINT_MAX to avoid using the matchchar in output. Otherwise the [0,ntax) index of the taxon that is printed first. */
			       unsigned begChar, /*!< first character index to write. Should be in [0, nchar). */
			       unsigned endChar) const; /*!< end of character range. This index is one greater than the last index to be printed. Should be in the range (begChar, nchar] */


 protected:
  // This function should not be called to remove characters, it is only used in the creation of new char blocks from existing blocks
  void SetNChar(unsigned nc)
  {
    this->nChar = nc;
  }
  // This function should not be called to remove characters, it is only used in the creation of new char blocks from existing blocks
  void SetNTax(unsigned nt)
  {
    this->nTaxWithData = nt;
  }

  NxsString GetStateLabelImpl(unsigned i, unsigned j) const; /*v2.1to2.2 4 */

  NxsDiscreteDatatypeMapper * GetMutableDatatypeMapperForChar(unsigned charIndex);
  bool IsInSymbols(char ch) NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
  void ShowStates(std::ostream &out, unsigned i, unsigned j) NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */

  void HandleCharlabels(NxsToken &token);
  void HandleCharstatelabels(NxsToken &token);
  void HandleDimensions(NxsToken &token, NxsString newtaxaLabel, NxsString ntaxLabel, NxsString ncharLabel);
  void HandleEliminate(NxsToken &token);
  virtual void HandleFormat(NxsToken &token);
  virtual void HandleMatrix(NxsToken &token);
  bool HandleNextContinuousState(NxsToken &token, unsigned taxNum, unsigned charNum, ContinuousCharRow & row, const NxsString & nameStr);
  bool HandleNextDiscreteState(NxsToken &token, unsigned taxNum, unsigned charNum, NxsDiscreteStateRow & row, NxsDiscreteDatatypeMapper &, const NxsDiscreteStateRow * firstTaxonRow, const NxsString & nameStr);
  bool HandleNextTokenState(NxsToken &token, unsigned taxNum, unsigned charNum, NxsDiscreteStateRow & row, NxsDiscreteDatatypeMapper &, const NxsDiscreteStateRow * firstTaxonRow, const NxsString & nameStr);
  void HandleStatelabels(NxsToken &token);
  virtual void HandleStdMatrix(NxsToken &token);
  virtual NxsDiscreteStateCell HandleTokenState(NxsToken &token, unsigned taxNum, unsigned charNum, NxsDiscreteDatatypeMapper &mapper, const NxsDiscreteStateRow * firstTaxonRow, const NxsString & nameStr);
  virtual void HandleTransposedMatrix(NxsToken &token);
  virtual void Read(NxsToken &token);
  void ResetSymbols();

  void WriteStates(NxsDiscreteDatum &d, char *s, unsigned slen) NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */


  NxsAssumptionsBlockAPI	*assumptionsBlock;	/* pointer to the ASSUMPTIONS block in which exsets, taxsets and charsets are stored */

  unsigned nChar; /* number of columns in matrix	*/
  unsigned nTaxWithData; /* number of non empty rows in the matrix*/

  char matchchar; /* match symbol to use in matrix */
  bool respectingCase; /* if true, RESPECTCASE keyword specified in FORMAT command */
  bool transposing; /* indicates matrix will be in transposed format */
  bool interleaving; /* indicates matrix will be in interleaved format */
  mutable bool tokens; /* if false, data matrix entries must be single symbols; if true, multicharacter entries are allows */
  bool labels; /* indicates whether or not labels will appear on left side of matrix */

  char missing; /* missing data symbol */
  char gap; /* gap symbol for use with molecular data */
  GapModeEnum gapMode; /* manipulated by the assumptions block. This setting basically just holds the value. It only affects post-read operations, such as GetNamedStateSetOfColumn and it does NOT change the internal encoding of the data (just triggers some filtering) */
  std::string symbols; /* list of valid character state symbols */
  std::map<char, NxsString> userEquates; /* list of associations defined by EQUATE attribute of FORMAT command */
  std::map<char, NxsString> defaultEquates;
  VecDatatypeMapperAndIndexSet datatypeMapperVec;
  NxsDiscreteStateMatrix	discreteMatrix; /* storage for discrete data */
  ContinuousCharMatrix	continuousMatrix;	/* */

  NxsUnsignedSet eliminated; /* array of (0-offset) character numbers that have been eliminated (will remain empty if no ELIMINATE command encountered) */
  NxsUnsignedSet excluded; /* set of (0-offset) indices of characters that have been excluded.*/

  LabelToIndexMap ucCharLabelToIndex;
  IndexToLabelMap indToCharLabel;
  NxsStringVectorMap charStates; /* storage for character state labels (if provided) */
  NxsStringVector globalStateLabels; /* state labels that apply to all characters (if not pre-empted by thy charStates field) */
  VecString items;

  NxsUnsignedSetMap charSets;
  NxsUnsignedSetMap exSets;
  NxsPartitionsByName charPartitions;
  NxsTransformationManager transfMgr;
  bool datatypeReadFromFormat;
  NxsPartitionsByName codonPosPartitions;
  std::string defCodonPosPartitionName;
  std::map<DataTypesEnum, NxsUnsignedSet> mixedTypeMapping;
 private:
  DataTypesEnum datatype; /* flag variable (see datatypes enum) */
  DataTypesEnum originalDatatype; /* flag variable (see datatypes enum) */
  StatesFormatEnum statesFormat;
  bool restrictionDataype;
  bool supportMixedDatatype;	/* (false by default) flag for whether or not MrBayes-style Mixed blocks should be supported */
  bool convertAugmentedToMixed; /* false by default (see AugmentedSymbolsToMixed) */
  bool allowAugmentingOfSequenceSymbols;
  int writeInterleaveLen;

  void CreateDatatypeMapperObjects(const NxsPartition & , const std::vector<DataTypesEnum> &);
  friend class PublicNexusReader;
  friend class MultiFormatReader;
};

typedef NxsCharactersBlock CharactersBlock;


class NxsCharactersBlockFactory
:public NxsBlockFactory
{
 public:
  virtual NxsCharactersBlock	*	GetBlockReaderForID(const std::string & NCL_BLOCKTYPE_ATTR_NAME, NxsReader *reader, NxsToken *token);
};

class NxsDiscreteStateSetInfo
{
 public:
 NxsDiscreteStateSetInfo(const std::set<NxsDiscreteStateCell> & stateSet, bool polymorphic=false, char symbol='\0')
   :states(stateSet),
    nexusSymbol(symbol),
    isPolymorphic(polymorphic)
    {}


  std::set<NxsDiscreteStateCell> states;
  char nexusSymbol;
  bool isPolymorphic;
};

/*! This class stores the information needed to map the internal storage for a cell of a matrix (a "state code") to
  the set of states that it corresponds to.
*/
class NxsDiscreteDatatypeMapper
{
 public:




  static void GenerateNxsExceptionMatrixReading(const char *, unsigned taxInd, unsigned charInd, NxsToken *, const NxsString &nameStr);
  static void GenerateNxsExceptionMatrixReading(const std::string &s, unsigned taxInd, unsigned charInd, NxsToken * token, const NxsString &nameStr)
  {
    GenerateNxsExceptionMatrixReading(s.c_str(), taxInd, charInd, token, nameStr);
  }


  NxsDiscreteDatatypeMapper();
  NxsDiscreteDatatypeMapper(NxsCharactersBlock::DataTypesEnum datatypeE, bool hasGaps);
  NxsDiscreteDatatypeMapper(NxsCharactersBlock::DataTypesEnum datatype, const std::string & symbols,
			    char missingChar, char gapChar, char matchChar,
			    bool respectCase, const std::map<char, NxsString> & extraEquates);

  /*! \returns the number of state codes (including partially ambiguous) */
  unsigned GetNumStateCodes() const
  {
    return (unsigned)stateSetsVec.size();
  }

  NxsCharactersBlock::DataTypesEnum GetDatatype() const
    {
      return datatype;
    }
  unsigned GetNumStates() const;
  unsigned GetNumStatesIncludingGap() const;
  std::string GetSymbols() const
    {
      return symbols;
    }
  std::string GetSymbolsWithGapChar() const
    {
      if (gapChar == '\0')
	return GetSymbols();
      std::string s;
      s = symbols;
      s.append(1, gapChar);
      return s;
    }

  const std::set<NxsDiscreteStateCell> & GetStateSetForCode(NxsDiscreteStateCell stateCode) const;
  bool IsSemanticallyEquivalent(const NxsDiscreteDatatypeMapper &other) const;
  bool IsPolymorphic(NxsDiscreteStateCell stateCode) const;
  NxsDiscreteStateCell PositionInSymbols(const char currChar) const;
  /*! Returns a state code for a NEXUS symbol.

    will return NXS_INVALID_STATE_CODE if the char is unknown
  */
  NxsDiscreteStateCell GetStateCodeStored(char currChar) const
  {
    return cLookup[static_cast<int>(currChar)];
  }
  /*! \returns the highest legal internal state code (this is note the number of
    state codes because the missing data and gap codes are negative).

    Note that this is the highest state code that the mapper understands, it does
    not imply that the NxsCharacterBlock that "owns" the mapper has a matrix
    with a state code that is as large as the number returned.

  */
  NxsDiscreteStateCell GetHighestStateCode() const
  {
    return ((NxsDiscreteStateCell) stateSetsVec.size()) + sclOffset - 1;
  }





 NxsDiscreteDatatypeMapper(const NxsDiscreteDatatypeMapper& other)
   :datatype(other.datatype)
    {
      *this = other;
    }
  NxsDiscreteDatatypeMapper & operator=(const NxsDiscreteDatatypeMapper&);

  char GetGapSymbol() const
  {
    return gapChar;
  }
  // warning: unsafe to call after reading -- does not recode data!
  void SetGapSymbol(char c)
  {
    gapChar = c;
  }
  char GetMissingSymbol() const
  {
    return missing;
  }
  std::map<char, NxsString> GetExtraEquates() const
    {
      return extraEquates;
    }
  unsigned GetNumStatesInStateCode(NxsDiscreteStateCell stateCode) const;
  NxsDiscreteStateCell GetOneStateForCode(NxsDiscreteStateCell stateCode, unsigned stateIndex) const;
  NxsDiscreteStateRow GetStateVectorForCode(NxsDiscreteStateCell stateCode) const;
  std::vector<std::vector<int> > GetPythonicStateVectors() const;
  NxsDiscreteStateCell PositionInSymbolsOrGaps(const char currChar) const
  {
    if (currChar == gapChar)
      return NXS_GAP_STATE_CODE;
    return PositionInSymbols(currChar);
  }
  std::string StateCodeToNexusString(NxsDiscreteStateCell, bool demandSymbols = true) const;
  NxsDiscreteStateCell StateCodeForNexusChar(const char currChar, NxsToken * token,
					     unsigned taxInd, unsigned charInd,
					     const NxsDiscreteStateRow * firstTaxonRow, const NxsString &nameStr) const;
  void WriteStartOfFormatCommand(std::ostream & out) const;
  void WriteStateCodeRowAsNexus(std::ostream & out, const std::vector<NxsDiscreteStateCell> &row) const;
  void WriteStateCodeRowAsNexus(std::ostream & out, std::vector<NxsDiscreteStateCell>::const_iterator & begIt, const std::vector<NxsDiscreteStateCell>::const_iterator & endIt) const;
  void WriteStateCodeAsNexusString(std::ostream & out, NxsDiscreteStateCell scode, bool demandSymbols = true) const;
  bool WasRestrictionDataype() const;
  void SetWasRestrictionDataype(bool v) {restrictionDataype = v;}
  NxsDiscreteStateCell EncodeNexusStateString(const std::string &stateAsNexus, NxsToken & token,
					      const unsigned taxInd, const unsigned charInd,
					      const NxsDiscreteStateRow * firstTaxonRow, const NxsString &nameStr);
  NxsDiscreteStateCell StateCodeForStateSet(const std::set<NxsDiscreteStateCell> &, const bool isPolymorphic,
					    const bool addToLookup, const char symbol);

  void DebugPrint(std::ostream &) const;

  bool GetUserDefinedEquatesBeforeConversion() const
  {
    return userDefinedEquatesBeforeConversion;
  }

  bool IsRespectCase() const
  {
    return respectCase;
  }

  const std::set<NxsDiscreteStateCell> & GetStateIntersection(NxsDiscreteStateCell stateCode, NxsDiscreteStateCell otherStateCode) const
  {
    if (stateIntersectionMatrix.empty())
      BuildStateIntersectionMatrix();
    const NxsDiscreteStateCell sc = stateCode - NXS_GAP_STATE_CODE;
    const NxsDiscreteStateCell osc = otherStateCode - NXS_GAP_STATE_CODE;
    return stateIntersectionMatrix.at(sc).at(osc);
  }

  bool FirstIsSubset(NxsDiscreteStateCell stateCode, NxsDiscreteStateCell otherStateCode, bool treatGapAsMissing) const
  {
    if (isStateSubsetMatrix.empty())
      BuildStateSubsetMatrix();
    const NxsDiscreteStateCell sc = stateCode - NXS_GAP_STATE_CODE;
    const NxsDiscreteStateCell osc = otherStateCode - NXS_GAP_STATE_CODE;
    if (treatGapAsMissing)
      return isStateSubsetMatrixGapsMissing.at(sc).at(osc);
    return isStateSubsetMatrix.at(sc).at(osc);
  }

  NxsGeneticCodesEnum geneticCode; /* only used for compressed codon codings */

  /*! can be used to "See" the mapping while debugging */
  void DebugWriteMapperFields(std::ostream & out) const;
 private:
  NxsDiscreteStateCell AddStateSet(const std::set<NxsDiscreteStateCell> & states, char nexusSymbol, bool symRespectCase, bool isPolymorphic);
  NxsDiscreteStateCell StateCodeForNexusMultiStateSet(const char nexusSymbol, const std::string & stateAsNexus, NxsToken * token,
						      unsigned taxInd, unsigned charInd,
						      const NxsDiscreteStateRow * firstTaxonRow, const NxsString &nameStr);
  NxsDiscreteStateCell StateCodeForNexusPossibleMultiStateSet(const char nexusSymbol, const std::string & stateAsNexus, NxsToken & token,
							      unsigned taxInd, unsigned charInd,
							      const NxsDiscreteStateRow * firstTaxonRow, const NxsString &nameStr);

  void RefreshMappings(NxsToken *token);
  void ValidateStateIndex(NxsDiscreteStateCell state) const;
  void ValidateStateCode(NxsDiscreteStateCell state) const;
  void BuildStateSubsetMatrix() const;
  void BuildStateIntersectionMatrix() const;
  void DeleteStateIndices(const std::set<NxsDiscreteStateCell> & deletedInds);

  NxsDiscreteStateCell * cLookup; /* Nexus char to state code lookup -- alias to member of charToStateCodeLookup*/
  NxsDiscreteStateSetInfo * stateCodeLookupPtr; /* state code to NxsDiscreteStateSetInfo object table -- alias to stateSets */
  std::string symbols;
  std::string lcsymbols; /* lowercase symbols (in the same order as symbols) */
  unsigned nStates;
  char matchChar;
  char gapChar;
  char missing;
  bool respectCase;
  std::map<char, NxsString> extraEquates;
  NxsCharactersBlock::DataTypesEnum datatype; /* flag variable (see datatypes enum) */
  std::vector<NxsDiscreteStateSetInfo> stateSetsVec; /* memory management for cLookup*/
  std::vector<NxsDiscreteStateCell> charToStateCodeLookup; /* stateCodeLookup */
  int sclOffset; /* offset of stateCodeLookup in stateSets */
  bool restrictionDataype;
  bool userDefinedEquatesBeforeConversion;

  typedef std::vector< std::set<NxsDiscreteStateCell> > StateIntersectionRow;
  typedef std::vector< StateIntersectionRow > StateIntersectionMatrix;
  typedef std::vector< bool > IsStateSubsetRow;
  typedef std::vector< IsStateSubsetRow > IsStateSubsetMatrix;
  mutable StateIntersectionMatrix stateIntersectionMatrix;
  mutable IsStateSubsetMatrix isStateSubsetMatrix;
  mutable IsStateSubsetMatrix isStateSubsetMatrixGapsMissing;

  friend class NxsCharactersBlock;
  friend class MultiFormatReader;
};

inline unsigned NxsDiscreteDatatypeMapper::GetNumStatesIncludingGap() const
{
  return nStates + (gapChar == '\0' ? 0 : 1);
}

inline unsigned NxsDiscreteDatatypeMapper::GetNumStates() const
{
  return nStates;
}

/*!
  Returns the set of state indices that correspond to the states of state code `c`
  Generates a NxsNCLAPIException if `c` is not a valid state code.
  Not as efficient as GetStateSetForCode
*/
inline std::vector<NxsDiscreteStateCell> NxsDiscreteDatatypeMapper::GetStateVectorForCode(NxsDiscreteStateCell c) const
{
  const std::set<NxsDiscreteStateCell> & ss = GetStateSetForCode(c);
  return std::vector<NxsDiscreteStateCell>(ss.begin(), ss.end());
}

/*!
  Returns the set of state indices that correspond to the states of state code `c`
  Generates a NxsNCLAPIException if `c` is not a valid state code.
*/
inline const std::set<NxsDiscreteStateCell>	& NxsDiscreteDatatypeMapper::GetStateSetForCode(NxsDiscreteStateCell c) const
{
  NCL_ASSERT(stateCodeLookupPtr);
  ValidateStateCode(c);
  return stateCodeLookupPtr[c].states;
}

/*!
  Returns the `stateIndex`-th state for `stateCode` Thus if stateCode = 6 and this corresponds to {AG}
  then:
  GetOneStateForCode(6, 0) would return 0 (assuming that A is state 0), and
  GetOneStateForCode(6, 1) would return 2 (assuming that G is state 2 in the symbols list)
*/
inline NxsDiscreteStateCell NxsDiscreteDatatypeMapper::GetOneStateForCode(NxsDiscreteStateCell stateCode, unsigned stateIndex) const
{
  const std::set<NxsDiscreteStateCell> & s = GetStateSetForCode(stateCode);
  unsigned i = 0;
  for (std::set<NxsDiscreteStateCell>::const_iterator sIt = s.begin(); sIt != s.end(); ++sIt, ++i)
    {
      if (i == stateIndex)
	return *sIt;
    }
  NCL_ASSERT(false);
  throw NxsException("State index out of range in NxsDiscreteDatatypeMapper::GetOneStateForCode");
}

/*!
  Returns the NEXUS reperesenation of the state code `scode` which may be a multiple character string such as {DNY}
  Generates a NxsNCLAPIException if `c` is not a valid state code.
  If insufficient symbols exist, then `demandSymbols` controls the behavior (if true then an NxsNCLAPIException
  is raised, otherwise an empty string is returned.

  Note that
  WriteStateCodeAsNexusString(out, c);
  Is more efficient than
  out << StateCodeToNexusString(c);
*/
inline std::string NxsDiscreteDatatypeMapper::StateCodeToNexusString(NxsDiscreteStateCell scode, bool demandSymbols) const
{
  std::ostringstream o;
  WriteStateCodeAsNexusString(o, scode, demandSymbols);
  return o.str();
}


/*!
  Called from HandleStdMatrix or HandleTransposedMatrix function to read in the next state. Always returns true
  except in the special case of an interleaved matrix, in which case it returns false if a newline character is
  encountered before the next token.
*/
inline NxsDiscreteStateCell NxsDiscreteDatatypeMapper::EncodeNexusStateString(
									      const std::string &stateAsNexus,
									      NxsToken & token, /* the token used to read from `in' */
									      const unsigned taxInd, /* the taxon index, in range [0..`ntax') */
									      const unsigned charInd, /* the character index, in range [0..`nChar') */
									      const NxsDiscreteStateRow * firstTaxonRow, const NxsString &nameStr)
{
  const unsigned tlen = (unsigned) stateAsNexus.length();
  if (tlen == 0)
    GenerateNxsExceptionMatrixReading("Unexpected empty token encountered", taxInd, charInd, &token, nameStr);
  if (tlen == 1)
    return StateCodeForNexusChar(stateAsNexus[0], &token, taxInd, charInd, firstTaxonRow, nameStr);
  return StateCodeForNexusMultiStateSet('\0', stateAsNexus, &token, taxInd, charInd, firstTaxonRow, nameStr);
}

/*! MrBayes introduced the datatype=restriction syntax for 01 symbols.
  NCL reads this type as standard, but sets a flag. If the datatype is reported as Standard, then you can call
  WasRestrictionDataype to see if the datatype was declared "RESTRICTION"
*/
inline bool NxsDiscreteDatatypeMapper::WasRestrictionDataype() const
{
  return restrictionDataype;
}
/*! MrBayes introduced the datatype=restriction syntax for 01 symbols.
  NCL reads this type as standard, but sets a flag. If the datatype is reported as Standard, then you can call
  WasRestrictionDataype to see if the datatype was declared "RESTRICTION"
*/
inline bool NxsCharactersBlock::WasRestrictionDataype() const
{
  return restrictionDataype;
}

inline void NxsCharactersBlock::SetNexus(NxsReader *nxsptr)
{
  NxsBlock::SetNexus(nxsptr);
  NxsTaxaBlockSurrogate::SetNexusReader(nxsptr);
}

inline bool NxsCharactersBlock::IsMixedType() const
{
  return (datatypeMapperVec.size() > 1);
}

/*! Returns the list of items that will be in each cell. This is always "STATES" for discrete datatypes, but can be
  a vector of any string for continuous types
*/
inline std::vector<std::string> NxsCharactersBlock::GetItems() const
{
  return items;
}

/*!
  Accessor for getting the list of continuous values associated with an "ITEM." Usually, these vectors will have
  length of 1, but the "STATES" item may have a list of all observed values.

  Values of DBL_MAX indicate missing data.
  An empty vector indicates that the key was not used in this cell.
*/
inline std::vector<double> NxsCharactersBlock::GetContinuousValues(
								   unsigned i, /* the taxon in range [0..`ntax') */
								   unsigned j, /* the character in range [0..`nChar') */
								   const std::string key) NCL_COULD_BE_CONST /* The name of the ITEM in the FORMAT command. Must be ALL CAPS.*/ /*v2.1to2.2 1 */
{
  const ContinuousCharCell & cell = continuousMatrix.at(i).at(j);
  ContinuousCharCell::const_iterator cIt = cell.find(key);
  if (cIt == cell.end())
    return std::vector<double>();
  return cIt->second;
}

/*! Short cut for returning the AVERAGE item, which is the default of a continuous cell type. Note this does not
  compute the average, this is just a shortcut for dealing with simple continuous matrices that do not
  use the ITEMS subcommand of FORMAT.

  Values of DBL_MAX indicate missing data.
*/
inline double NxsCharactersBlock::GetSimpleContinuousValue(
							   unsigned i, /* the taxon in range [0..`ntax') */
							   unsigned j) NCL_COULD_BE_CONST /* the character in range [0..`nChar') */ /*v2.1to2.2 1 */
{
  const std::vector<double> av = GetContinuousValues(i, j, std::string("AVERAGE"));
  if (av.empty())
    return DBL_MAX;
  return av.at(0);
}


/*!
  Returns label for character `i' (starting at zero), if a label has been specified. If no label was specified, returns string
  containing a single blank (i.e., " ").
*/
inline NxsString NxsCharactersBlock::GetCharLabel( /*v2.1to2.2 4 */
						  unsigned i) const	/* the character in range [0..`nChar') */
{
  std::map<unsigned, std::string>::const_iterator tlIt = indToCharLabel.find(i);
  if (tlIt == indToCharLabel.end())
    return NxsString(" "); /*v2.1to2.2 4 */
  return NxsString(tlIt->second.c_str()); /*v2.1to2.2 4 */
}

/*!
  Returns true if at least one character has charlabels
*/
inline bool NxsCharactersBlock::HasCharLabels() const
{
  return !indToCharLabel.empty();
}
/*!
  Returns the gap symbol currently in effect. If no gap symbol specified, returns '\0'.
*/
inline char NxsCharactersBlock::GetGapSymbol() const
{
  return gap;
}

/*!
//Warning: this function is unsafe -- it only effects the writing of the matrix as NEXUS and it does not correctly
recode the matrix.
*/
inline void NxsCharactersBlock::SetGapSymbol(char g)
{
  gap = g;
  if (datatypeMapperVec.size() == 1)
    datatypeMapperVec[0].first.SetGapSymbol(g);
}


/*! Returns value of `datatype' from the datatype mapper.
  This if you have told NCL to read augmented symbols list (SetAllowAugmentingOfSequenceSymbols)
  then it is possible that the datatype returned will be standard even if the GetOriginalDataType()
  returns a molecular sequence datatype. This means that the symbols list was augmented.

*/
inline NxsCharactersBlock::DataTypesEnum NxsCharactersBlock::GetDataType() const
{
  if (datatypeMapperVec.empty())
    return datatype;
  if (datatypeMapperVec.size() > 1)
    return mixed;
  return datatypeMapperVec[0].first.GetDatatype();
}

inline NxsCharactersBlock::DataTypesEnum NxsCharactersBlock::GetOriginalDataType() const
{
  return originalDatatype;
}

/*!
  Returns the `matchchar' symbol currently in effect. If no `matchchar' symbol specified, returns '\0'.
*/
inline char NxsCharactersBlock::GetMatchcharSymbol() NCL_COULD_BE_CONST /*v2.1to2.2 1 */
{
  return matchchar;
}

/*!
  This function is no longer the most efficient way to access parsed data (see notes on NxsCharacterBlock and
  GetMatrix() and GetMatrixDecoder() methods.

  Returns internal representation of the state for taxon `i', character `j'. In the normal situation, `k' is 0 meaning
  there is only one state with no uncertainty or polymorphism. If there are multiple states, specify a number in the
  range [0..n) where n is the number of states returned by the GetNumStates function. Use the IsPolymorphic
  function to determine whether the multiple states correspond to uncertainty in state assignment or polymorphism in
  the taxon. The value returned from this function is one of the following:
  ~
  o -3 means gap state (see note below)
  o -2 means missing state (see note below)
  o an integer 0 or greater is internal representation of a state
  ~
  Note: gap and missing states are actually represented internally in a different way; for a description of the actual
  internal representation of states, see the documentation for NxsDiscreteDatum.

*/
inline NxsDiscreteStateCell NxsCharactersBlock::GetInternalRepresentation(
									  unsigned i,	/* the taxon in range [0..`ntax') */
									  unsigned j,	/* the character in range [0..`nchar') */
									  unsigned k) NCL_COULD_BE_CONST /* the 0-offset index of state to return */ /*v2.1to2.2 1 */
{
  if (IsGapState(i, j))
    return -3;
  else if (IsMissingState(i, j))
    return -2;
  else
    return GetStateIndex(i, j, k);
}

/*!
  Returns the missing data symbol currently in effect. If no missing data symbol specified, returns '\0'.
*/
inline char NxsCharactersBlock::GetMissingSymbol() const
{
  return missing;
}

/*!
  Name change to reinforce the change in meaning -- in NCL after 2.1 this will behaves just like the
  GetNCharTotal(). It returns the number of characters in the matrix (regardless of whether they have been excluded).
  The old GetNChar() function is now called GetNumIncludedChars();
*/
inline unsigned NxsCharactersBlock::GetNumChar() const
{
  return nChar;
}

/*!
  Note the change in meaning -- in NCL after 2.1 this will behaves just like the
  GetNCharTotal(). It returns the number of characters in the matrix (regardless of whether they have been excluded).
  The old GetNChar() function is now called GetNumIncludedChars();
*/
inline unsigned NxsCharactersBlock::GetNChar() const
{
  return nChar;
}

/*!
  Returns the number of characters which are not excluded (or eliminated) this number is <= GetNumChar()
*/
inline unsigned NxsCharactersBlock::GetNumIncludedChars() const
{
  return (unsigned)nChar - (unsigned)excluded.size();
}


inline unsigned NxsCharactersBlock::GetNCharTotal()
{
  return nChar;
}

inline unsigned NxsCharactersBlock::GetNCharTotal() const
{
  return nChar;
}

inline unsigned NxsCharactersBlock::GetNTaxWithData() const
{
  return nTaxWithData;
}

/*!
  Returns the number of characters eliminated with the ELIMINATE command.
*/
inline unsigned NxsCharactersBlock::GetNumEliminated() NCL_COULD_BE_CONST /*v2.1to2.2 1 */
{
  return (unsigned)eliminated.size();
}

/*!
  Returns the number of stored equates associations.
*/
inline unsigned NxsCharactersBlock::GetNumUserEquates() const
{
  return (unsigned)(userEquates.size());
}

/*!
  Returns the number of stored equates associations.
*/
inline unsigned NxsCharactersBlock::GetNumEquates() NCL_COULD_BE_CONST /*v2.1to2.2 1 */
{
  return (unsigned)(userEquates.size() + defaultEquates.size());
}

/*!
  Returns the number of actual columns in `matrix'. This number is equal to `nchar', but can be smaller than
  `ncharTotal' since the user could have eliminated some of the characters.
*/
inline unsigned NxsCharactersBlock::GetNumMatrixCols() NCL_COULD_BE_CONST /*v2.1to2.2 1 */
{
  return nChar;
}

/*!
  Returns the number of actual rows in `matrix'. This number is equal to `ntax', but can be smaller than `ntaxTotal'
  since the user did not have to provide data for all taxa specified in the TAXA block.
*/
inline unsigned NxsCharactersBlock::GetNumMatrixRows() NCL_COULD_BE_CONST /*v2.1to2.2 1 */
{
  return GetNTaxTotal();
}

inline NxsDiscreteStateCell NxsCharactersBlock::GetStateIndex(
							      unsigned taxInd,	/* the taxon in range [0..`ntax') */
							      unsigned charInd, /* the character in range [0..`nchar') */
							      unsigned k) const
{
  const NxsDiscreteDatatypeMapper * currMapper =	GetDatatypeMapperForChar(charInd);
  NCL_ASSERT(currMapper);
  const NxsDiscreteStateRow & row = GetDiscreteMatrixRow(taxInd);
  NCL_ASSERT(row.size() > charInd);
  return currMapper->GetOneStateForCode(row[charInd], k);
}
/*! Returns symbol from symbols list representing the state for taxon `i' and character `j'.

  The normal situation in which there is only one state with no uncertainty or polymorphism is
  represented by `k' = 0.
  If there are multiple states, specify a number in the range [0..n) where n is the number of states
  returned by the GetNumStates function.
  Use the IsPolymorphic function to determine whether the multiple states correspond to uncertainty in state
  assignment or polymorphism in the taxon. Assumes `symbols' is non-NULL.

  \warning{In NEXUS it is possible (via the TOKENS mode) to introduce a dataype that does not have unique single
  char symbols for each state. This function is not guaranteed to succeed in such cases.
  The NxsDiscreteDatatypeMapper method of accessing characters is more robust (and faster).
  see \ref NxsCharacterBlockQueries
  }
*/
inline char NxsCharactersBlock::GetState(
					 unsigned i,	/* the taxon in range [0..`ntax') */
					 unsigned j,	/* the character in range [0..`nchar') */
					 unsigned k) const	/* the 0-offset index of the state to return */
{
  NCL_ASSERT(!symbols.empty());
  const NxsDiscreteStateCell p = GetStateIndex(i, j, k);
  if (p < 0)
    {
      NCL_ASSERT(p == NXS_GAP_STATE_CODE);
      return gap;
    }
  NCL_ASSERT(p < (int)symbols.length());
  return symbols[(int)p];
}

/*! \returns The symbols string	Warning: returned value may be NULL.
 */
inline const char *NxsCharactersBlock::GetSymbols() NCL_COULD_BE_CONST /*v2.1to2.2 1 */
{
  return symbols.c_str();
}

/*!
  Returns label for taxon number `i' (`i' ranges from 0 to `ntax' - 1).
*/
inline NxsString NxsCharactersBlock::GetTaxonLabel( /*v2.1to2.2 4 */
						   unsigned i) const	/* the taxon's position in the taxa block */
{
  NxsString s = taxa->GetTaxonLabel(i); /*v2.1to2.2 4 */
  return s;
}

inline bool NxsCharactersBlock::TaxonIndHasData(
						unsigned taxInd) const /* the character in question, in the range [0..`nchar') */
{
  if (datatype == continuous)
    return (taxInd < continuousMatrix.size() && !continuousMatrix[taxInd].empty());
  return (taxInd < discreteMatrix.size() && !discreteMatrix[taxInd].empty());
}


inline const NxsUnsignedSet & NxsCharactersBlock::GetExcludedIndexSet() const
{
  return excluded;
}

inline bool NxsCharactersBlock::IsActiveChar(
					     unsigned j) const	/* the character in question, in the range [0..`nchar') */
{
  return (j < nChar && excluded.count(j) == 0);
}


/*!
  Returns true if character `j' has been excluded. If character `j' is active, returns false. Assumes `j' is in the
  range [0..`nchar').
*/
inline bool NxsCharactersBlock::IsExcluded(
					   unsigned j) const	/* the character in question, in the range [0..`nchar') */
{
  return !IsActiveChar(j);
}

inline bool NxsCharactersBlock::IsActiveChar(
					     unsigned j) /* the character in question, in the range [0..`nchar') */
{
  return (j < nChar && excluded.count(j) == 0);
}


/*!
  Returns true if character `j' has been excluded. If character `j' is active, returns false. Assumes `j' is in the
  range [0..`nchar').
*/
inline bool NxsCharactersBlock::IsExcluded(
					   unsigned j) /* the character in question, in the range [0..`nchar') */
{
  return !IsActiveChar(j);
}


/*!
  Returns true if INTERLEAVE was specified in the FORMAT command, false otherwise.
*/
inline bool NxsCharactersBlock::IsInterleave() NCL_COULD_BE_CONST /*v2.1to2.2 1 */
{
  return interleaving;
}

/*!
  Returns true if LABELS was specified in the FORMAT command, false otherwise.
*/
inline bool NxsCharactersBlock::IsLabels() NCL_COULD_BE_CONST /*v2.1to2.2 1 */
{
  return labels;
}

/*!
  Returns true if RESPECTCASE was specified in the FORMAT command, false otherwise.
*/
inline bool NxsCharactersBlock::IsRespectCase() const
{
  return respectingCase;
}

/*!
  Returns true if TOKENS was specified in the FORMAT command, false otherwise.
*/
inline bool NxsCharactersBlock::IsTokens() NCL_COULD_BE_CONST /*v2.1to2.2 1 */
{
  return tokens;
}

/*!
  Returns true if TRANSPOSE was specified in the FORMAT command, false otherwise.
*/
inline bool NxsCharactersBlock::IsTranspose() NCL_COULD_BE_CONST /*v2.1to2.2 1 */
{
  return transposing;
}

/*! Converts a taxon label to a number corresponding to the taxon's position within the list maintained by the
  NxsTaxaBlockAPI object. This method overrides the virtual function of the same name in the NxsBlock base class. If
  `s' is not a valid taxon label, returns the value 0.
*/
inline unsigned NxsCharactersBlock::TaxonLabelToNumber(
						       NxsString s) const	/* the taxon label to convert */ /*v2.1to2.2 4 */
{
  try
    {
      return 1 + taxa->FindTaxon(s);
    }
  catch(NxsTaxaBlock::NxsX_NoSuchTaxon)
    {
    }

  return 0;
}


inline VecBlockPtr NxsCharactersBlock::GetImpliedBlocks()
{
  return GetCreatedTaxaBlocks();
}

inline const std::string & NxsCharactersBlock::GetBlockName() const
{
  return NCL_BLOCKTYPE_ATTR_NAME;
}
inline void NxsCharactersBlock::HandleLinkCommand(NxsToken & token)
{
  HandleLinkTaxaCommand(token);
}
inline void NxsCharactersBlock::WriteLinkCommand(std::ostream &out) const
{
  WriteLinkTaxaCommand(out);
}


inline NxsCharactersBlock::StatesFormatEnum NxsCharactersBlock::GetStatesFormat() const
{
  return statesFormat;
}

inline	unsigned NxsCharactersBlock::CharLabelToNumber(NxsString s) NCL_COULD_BE_CONST /*v2.1to2.2 d */
{ /*v2.1to2.2 d */
  const NxsCharactersBlock *b = (const NxsCharactersBlock *)(this); /*v2.1to2.2 d */
  return b->CharLabelToNumber(s); /*v2.1to2.2 d */
} /*v2.1to2.2 d */

/*!
  Returns true if character with `charIndex' (0-based index) was eliminated, false otherwise.
*/
inline bool NxsCharactersBlock::IsEliminated(
					     unsigned charIndex) NCL_COULD_BE_CONST /* the character in question */ /*v2.1to2.2 1 */
{
  return eliminated.count(charIndex) > 0 ;
}

/*
  Returns an alias to the NxsDiscreteDatatypeMapper for character index.
  NULL will be returned if the NxsCharactersBlock is not fully initialized or
  if the block stores continuous characters.
  The pointer is only guaranteed to be valid until the NxsCharactersBlock is modified.
  (so do not store for long term usage).
*/
inline const NxsDiscreteDatatypeMapper * NxsCharactersBlock::GetDatatypeMapperForChar(unsigned charIndex) const
{
  NxsCharactersBlock *mt = const_cast<NxsCharactersBlock *>(this);
  return mt->GetMutableDatatypeMapperForChar(charIndex);
}

inline const NxsDiscreteDatatypeMapper & NxsCharactersBlock::GetDatatypeMapperForCharRef(unsigned charIndex) const
{
  const NxsDiscreteDatatypeMapper * dm = this->GetDatatypeMapperForChar(charIndex);
  NCL_ASSERT(dm);
  return *dm;
}

inline const NxsDiscreteStateRow & NxsCharactersBlock::GetDiscreteMatrixRow(unsigned int taxIndex) const
{
  return discreteMatrix.at(taxIndex);
}

inline const NxsCharactersBlock::ContinuousCharRow & NxsCharactersBlock::GetContinuousMatrixRow(unsigned taxIndex) const
{
  return continuousMatrix.at(taxIndex);
}

/*!
  Returns an alias to the NxsDiscreteDatatypeMapper for character index.
  NULL will be returned if the NxsCharactersBlock is not fully initialized or
  if the block stores continuous characters.
  The pointer is only guaranteed to be valid until the NxsCharactersBlock is modified.
  (so do not store for long term usage).
*/
inline NxsDiscreteDatatypeMapper * NxsCharactersBlock::GetMutableDatatypeMapperForChar(unsigned int charIndex)
{
  if (datatypeMapperVec.size() == 1)
    return &(datatypeMapperVec[0].first);
  for (VecDatatypeMapperAndIndexSet::iterator dmvIt = datatypeMapperVec.begin(); dmvIt != datatypeMapperVec.end(); ++dmvIt)
    {
      const NxsUnsignedSet & currCS = dmvIt->second;
      if (currCS.count(charIndex) > 0)
	return &(dmvIt->first);
    }
  return NULL;
}

inline std::vector<const NxsDiscreteDatatypeMapper *> NxsCharactersBlock::GetAllDatatypeMappers() const
{
  std::vector<const NxsDiscreteDatatypeMapper *> v;
  for (VecDatatypeMapperAndIndexSet::const_iterator dmvIt = datatypeMapperVec.begin(); dmvIt != datatypeMapperVec.end(); ++dmvIt)
    v.push_back(&(dmvIt->first));
  return v;
}

inline void NxsDiscreteDatatypeMapper::WriteStateCodeRowAsNexus(std::ostream & out, const NxsDiscreteStateRow &row) const
{//@mth optimize
  std::vector<NxsDiscreteStateCell>::const_iterator b = row.begin();
  const std::vector<NxsDiscreteStateCell>::const_iterator e = row.end();
  WriteStateCodeRowAsNexus(out, b, e);
}

inline void NxsDiscreteDatatypeMapper::WriteStateCodeRowAsNexus(std::ostream & out, NxsDiscreteStateRow::const_iterator & begIt, const NxsDiscreteStateRow::const_iterator & endIt) const
{//@mth optimize
  for (; begIt != endIt; ++begIt)
    WriteStateCodeAsNexusString(out, *begIt, true);
}


/*!
  Returns true if this set replaces an older definition.
*/
inline bool NxsCharactersBlock::AddNewIndexSet(const std::string &label, const NxsUnsignedSet & inds)
{
  NxsString ls(label.c_str());
  bool replaced = charSets.count(ls) > 0;
  charSets[ls] = inds;
  return replaced;
}



/*!
  Returns true if this set replaces an older definition.
*/
inline bool NxsCharactersBlock::AddNewPartition(const std::string &label, const NxsPartition & inds)
{
  NxsString ls(label.c_str());
  ls.ToUpper();
  bool replaced = charPartitions.count(ls) > 0;
  charPartitions[ls] = inds;
  return replaced;
}



/*!
  Returns the number of indices that correspond to the label (and the number
  of items that would be added to *inds if inds points to an empty set).
*/
inline unsigned NxsCharactersBlock::GetIndicesForLabel(const std::string &label, NxsUnsignedSet *inds) const
{
  NxsString emsg;
  const unsigned numb = CharLabelToNumber(label);
  if (numb != 0)
    {
      if (inds)
	inds->insert(numb - 1);
      return 1;
    }
  if (!defCodonPosPartitionName.empty())
    {
      std::string t(label);
      NxsString::to_upper(t);
      std::string n;
      if (t == "POS1")
	n.assign("1");
      else if (t == "POS2")
	n.assign("2");
      else if (t == "POS3")
	n.assign("3");
      else if (t == "NONCODING")
	n.assign("N");
      if (!n.empty())
	{
	  NxsPartitionsByName::const_iterator pit = codonPosPartitions.find(defCodonPosPartitionName);
	  if (pit != codonPosPartitions.end())
	    {
	      const NxsPartition & p = pit->second;
	      for (NxsPartition::const_iterator s = p.begin(); s != p.end(); ++s)
		{
		  if (NxsString::case_insensitive_equals(n.c_str(), s->first.c_str()))
		    {
		      unsigned nel = (unsigned)s->second.size();
		      if (inds)
			inds->insert(s->second.begin(), s->second.end());
		      return nel;
		    }
		}
	    }
	}
    }
  if (NxsString::case_insensitive_equals(label.c_str(), "CONSTANT"))
    {
      NxsUnsignedSet c;
      FindConstantCharacters(c);
      if (inds)
	inds->insert(c.begin(), c.end());
      return (unsigned)c.size();
    }
  if (NxsString::case_insensitive_equals(label.c_str(), "GAPPED"))
    {
      NxsUnsignedSet c;
      FindGappedCharacters(c);
      if (inds)
	inds->insert(c.begin(), c.end());
      return (unsigned)c.size();
    }
  return GetIndicesFromSetOrAsNumber(label, inds, charSets, GetMaxIndex(), "character");
}

inline unsigned NxsCharactersBlock::GetMaxIndex() const
{
  unsigned nct = GetNCharTotal();
  if (nct == 0)
    return UINT_MAX;
  return nct - 1;
}

#endif
