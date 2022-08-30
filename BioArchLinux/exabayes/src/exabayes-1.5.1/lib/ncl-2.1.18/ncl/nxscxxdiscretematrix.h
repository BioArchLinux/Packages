//	Copyright (C) 2008 Mark Holder
//
//	This file is part of NCL (Nexus Class Library) version 2.1
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


#if !defined(NXS_CXX_DISCRETE_MATRIX_H)
#define NXS_CXX_DISCRETE_MATRIX_H

#include <string>
#include <vector>
#include "ncl/nxsdefs.h"
#include "ncl/nxsallocatematrix.h"
#include "ncl/nxscharactersblock.h"
#include "ncl/nxscdiscretematrix.h"

class NxsCharacterPattern;
	/**
	 * A C++ class that wraps a CDiscretMatrix in order to handle the memory
	 management more cleanly. This is intended to be an alternate, low-level way
	 to get character data out of a NxsCharactersBlock
	 */
class NxsCXXDiscreteMatrix
	{
	public:
		NxsCXXDiscreteMatrix()
			{
			Initialize(0L, false);
			}
		NxsCXXDiscreteMatrix(const NxsCDiscreteMatrix & );
		NxsCXXDiscreteMatrix(const NxsCharactersBlock & cb, bool convertGapsToMissing, const NxsUnsignedSet * toInclude = 0L, bool standardizeCoding = true);

		void Initialize(const NxsCharactersBlock * cb, bool convertGapsToMissing, const NxsUnsignedSet * toInclude = 0L, bool standardizeCoding = true);

		const NxsCDiscreteMatrix & getConstNativeC() const
			{
			return nativeCMatrix;
			}

		NxsCDiscreteMatrix & getNativeC()
			{
			return nativeCMatrix;
			}

		unsigned	getNChar() const
			{
			return nativeCMatrix.nChar;
			}

		unsigned	getNTax() const
			{
			return nativeCMatrix.nTax;
			}

		unsigned	getNStates() const
			{
			return nativeCMatrix.nStates;
			}

		const char *	getSymbolsList() const   //POL added 15-Nov-2005
			{
			return nativeCMatrix.symbolsList;
			}

		const std::vector<int8_t> &getStateList() const
			{
			return stateListAlias;
			}

		const std::vector<unsigned> &getStateListPos() const
			{
			return stateListPosAlias;
			}

		const NxsCDiscreteStateSet *getRow(unsigned i) const
			{
			NCL_ASSERT(i < nativeCMatrix.nTax);
			return nativeCMatrix.matrix[i];
			}

		const std::vector<int8_t> getRowAsVector(unsigned i) const
			{
			NCL_ASSERT(i < nativeCMatrix.nTax);
			std::vector<int8_t> v;
			for (unsigned j = 0; j < nativeCMatrix.nChar; j++)
				{
				v.push_back(nativeCMatrix.matrix[i][j]);
				}
			return v;
			}

		const NxsCDiscreteStateSet * const * getMatrix() const
			{
			return nativeCMatrix.matrix;
			}

		const int getDatatype() const
			{
			return (int)nativeCMatrix.datatype;
			}

		bool hasWeights() const
			{
			return hasIntWeights() || hasDblWeights();
			}

		bool hasIntWeights() const
			{
			return !(intWts.empty());
			}

		bool hasDblWeights() const
			{
			return !(dblWts.empty());
			}

		std::vector<int> & getIntWeights()
			{
			return intWts;
			}

		std::vector<double> & getDblWeights()
			{
			return dblWts;
			}

		const std::vector<int> & getIntWeightsConst() const
			{
			return intWts;
			}

		const std::vector<double> & getDblWeightsConst() const
			{
			return dblWts;
			}

		const std::set<unsigned> & getExcludedCharIndices() const
			{
			return activeExSet;
			}

		std::vector<unsigned> getExcludedCharIndicesAsVector() const
			{
			return std::vector<unsigned>(activeExSet.begin(), activeExSet.end());
			}

	private:
		typedef ScopedTwoDMatrix<NxsCDiscreteStateSet> ScopedStateSetTwoDMatrix;

		NxsCDiscreteMatrix			nativeCMatrix; 		/** taxa x characters matrix in a C struct*/
		std::string					symbolsStringAlias;	/** memory management alias to symbols field of nativeCMatrix */
		ScopedStateSetTwoDMatrix	matrixAlias;		/** memory management alias to matrix field of nativeCMatrix */
		std::vector<NxsCDiscreteState_t>	stateListAlias;		/** memory management alias to ambigList field of nativeCMatrix */
		std::vector<unsigned>		stateListPosAlias;		/** memory management alias to symbolsList field of nativeCMatrix */
		std::vector<int>			intWts;
		std::vector<double>			dblWts;
		std::set<unsigned>			activeExSet;
		NxsCXXDiscreteMatrix(const NxsCXXDiscreteMatrix &); /** don't define, not copyable*/
		NxsCXXDiscreteMatrix & operator=(const NxsCXXDiscreteMatrix &); /** don't define, not copyable*/
	};




class NxsCharacterPattern
    {
    public: 
        
        bool operator < (const NxsCharacterPattern & other) const {
            return this->stateCodes < other.stateCodes;
        }
        bool operator == (const NxsCharacterPattern & other) const {
            return this->stateCodes == other.stateCodes;
        }
        // returns true if none of the state codes are the missing or gap codes (negative values
        //  note this does not test if all of the state codes correspond to completely specified
        //  cells that are only compatible with one state!
        bool StateCodesAreNonNegative() const {
            for (std::vector<NxsCDiscreteState_t>::const_iterator scIt = stateCodes.begin();
                                                                  scIt != stateCodes.end(); 
                                                                  ++scIt)
                {
                if (*scIt < 0)
                    return false;
                }
            return true;
        }
        std::vector<NxsCDiscreteState_t> stateCodes;
        mutable unsigned count;
        mutable unsigned patternIndex; // used as scratchspace not always valid!!!
        mutable double sumOfPatternWeights; // stored as float.  Use NxsCXXDiscreteMatrix::hasIntWeights of the original matrix to see if these weights should be interpretted as ints
    };
    

/*----------------------------------------------------------------------------------------------------------------------
| Fills `compressedTransposedMatrix` with the compressed patterns found in `mat`
|
| Data structure for mapping between indices in these patterns can be obtained by the client providing
|   `compressedIndexPattern` arguments.
|
| Characters or taxa can be omitted by providing `taxaToInclude` or `charactersToInclude` arguments.
|   If these arguments are 0L (or not provided) then all data will be included. Note that skipping taxa
|   will cause the taxon indexing within a pattern to disagree with the overall taxon numbering because there will
|   be "frameshifts" for all of the skipped taxa.  The included taxa will be present in the expected order, but it is 
|   the caller code's responsibility to keep track of which taxa are included in the pattern.
*/
unsigned NxsCompressDiscreteMatrix(
  const NxsCXXDiscreteMatrix & mat,			/**< is the data source */
  std::set<NxsCharacterPattern> & patternSet, /* matrix that will hold the compressed columns */
  std::vector<const NxsCharacterPattern *> * compressedIndexPattern = 0L, /** if not 0L, this will be filled to provide a map from an index in `compressedTransposedMatrix` to the original character count */
  const NxsUnsignedSet * taxaToInclude = 0L,	/**< if not 0L, this should be  the indices of the taxa in `mat` to include (if 0L all characters will be included). Excluding taxa will result in shorter patterns (the skipped taxa will not be filled with empty codes, instead the taxon indexing will be frameshifted -- the client code must keep track of these frameshifts). */
  const NxsUnsignedSet * charactersToInclude = 0L);	/**< if not 0L, this should be  the indices of the characters in `mat` to include (if 0L all characters will be included) */
    
/*----------------------------------------------------------------------------------------------------------------------
| Fills `compressedTransposedMatrix` with the compressed patterns found in `mat`
|
| Data structure for mapping between indices in these representations can be obtained by the client providing
|   `originalIndexToCompressed` and/or compressedIndexToOriginal arguments.
|
| Characters or taxa can be omitted by providing `taxaToInclude` or `charactersToInclude` arguments.
|   If these arguments are 0L (or not provided) then all data will be included. Note that skipping taxa
|   will cause the taxon indexing within a pattern to disagree with the overall taxon numbering because there will
|   be "frameshifts" for all of the skipped taxa.  The included taxa will be present in the expected order, but it is 
|   the caller code's responsibility to keep track of which taxa are included in the pattern.
*/
unsigned NxsCompressDiscreteMatrix(
  const NxsCXXDiscreteMatrix & mat,			/**< is the data source */
  std::vector<NxsCharacterPattern> & compressedTransposedMatrix, /* matrix that will hold the compressed columns */
  std::vector<int> * originalIndexToCompressed, /** if not 0L, this will be filled to provide map an index in `mat` to the corresponding index in `compressedTransposedMatrix` (-1 in the vector indicates that the character was not included) */
  std::vector<std::set<unsigned> > * compressedIndexToOriginal, /** if not 0L, this will be filled to provide a map from an index in `compressedTransposedMatrix` to the original character count */
  const NxsUnsignedSet * taxaToInclude = 0L,	/**< if not 0L, this should be  the indices of the taxa in `mat` to include (if 0L all characters will be included). Excluding taxa will result in shorter patterns (the skipped taxa will not be filled with empty codes, instead the taxon indexing will be frameshifted -- the client code must keep track of these frameshifts). */
  const NxsUnsignedSet * charactersToInclude = 0L);	/**< if not 0L, this should be  the indices of the characters in `mat` to include (if 0L all characters will be included) */
	

void NxsConsumePatternSetToPatternVector(
  std::set<NxsCharacterPattern> & patternSet, /* INPUT matrix that will hold the compressed columns */
  std::vector<NxsCharacterPattern> & compressedTransposedMatrix, /* OUTPUT matrix that will hold the compressed columns */
  const std::vector<const NxsCharacterPattern *> * compressedIndexPattern = 0L, /** INPUT This mapping must be provided if either  `originalIndexToCompressed` or `compressedIndexToOriginal` is requested */
  std::vector<int> * originalIndexToCompressed = 0L, /** OUTPUT if not 0L, this will be filled to provide map an index in `mat` to the corresponding index in `compressedTransposedMatrix` (-1 in the vector indicates that the character was not included) */
  std::vector<std::set<unsigned> > * compressedIndexToOriginal = 0L); /** OUTPUT  if not 0L, this will be filled to provide a map from an index in `compressedTransposedMatrix` to the original character count */

void NxsTransposeCompressedMatrix(
  const std::vector<NxsCharacterPattern> & compressedTransposedMatrix, 
  ScopedTwoDMatrix<NxsCDiscreteStateSet> & destination,
  std::vector<unsigned> * patternCounts = 0L,
  std::vector<double> * patternWeights = 0L);
  
 

#endif  // NXS_CXX_DISCRETE_MATRIX_H
