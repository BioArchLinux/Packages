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
#include <iterator>
#include "ncl/nxscxxdiscretematrix.h"
#include "ncl/nxsutilcopy.h"
#include <cassert>
using std::string;
using std::vector;
using std::cout;
using std::endl;

/**===========================================================================
| fills compressedTransposedMatrix and empties patternSet
|
| If `originalIndexToCompressed` or `compressedIndexToOriginal` are requested
|   then the `compressedIndexPattern` mapping must be supplied. `compressedIndexPattern`
|   must contain pointers to the keys in `patternSet.` Note that these will 
|   be invalid after the call because patternSet will be emptied).
*/
void NxsConsumePatternSetToPatternVector(
  std::set<NxsCharacterPattern> & patternSet, /* INPUT matrix that will hold the compressed columns */
  std::vector<NxsCharacterPattern> & compressedTransposedMatrix, /* OUTPUT matrix that will hold the compressed columns */
  const std::vector<const NxsCharacterPattern *> * compressedIndexPattern, /** INPUT This mapping must be provided if either  `originalIndexToCompressed` or `compressedIndexToOriginal` is requested */
  std::vector<int> * originalIndexToCompressed, /** OUTPUT if not 0L, this will be filled to provide map an index in `mat` to the corresponding index in `compressedTransposedMatrix` (-1 in the vector indicates that the character was not included) */
  std::vector<std::set<unsigned> > * compressedIndexToOriginal) /** OUTPUT  if not 0L, this will be filled to provide a map from an index in `compressedTransposedMatrix` to the original character count */
{
    const unsigned patternIndexOffset = (unsigned const)compressedTransposedMatrix.size();
    const unsigned numCompressedPatterns = (unsigned const)patternSet.size();
    if (originalIndexToCompressed != 0L || compressedIndexToOriginal != 0L)
        {
        if (compressedIndexPattern == 0L)
            throw NxsException("compressedIndexPattern must be provided in ConsumePatternSetToPatternVector if mappings are requested");
        unsigned patternIndex = 0;
        for (std::set<NxsCharacterPattern>::iterator pIt = patternSet.begin(); pIt != patternSet.end(); ++pIt, ++patternIndex)
            {
            pIt->patternIndex = patternIndex + patternIndexOffset;
            }
        if (originalIndexToCompressed)
            originalIndexToCompressed->resize(compressedIndexPattern->size());
        if (compressedIndexToOriginal)
            {
            compressedIndexToOriginal->clear();
            compressedIndexToOriginal->resize(numCompressedPatterns);
            }
        for (unsigned i = 0; i < compressedIndexPattern->size(); ++ i)
            {
            const NxsCharacterPattern * pat = (*compressedIndexPattern)[i];
            if (pat)
                {
                if (originalIndexToCompressed)
                    (*originalIndexToCompressed)[i] = pat->patternIndex;
                if (compressedIndexToOriginal)
                    {
                    NCL_ASSERT(pat->patternIndex < numCompressedPatterns);
                    compressedIndexToOriginal->at(pat->patternIndex).insert(i);
                    }
                }
            else
                {
                if (originalIndexToCompressed)
                    (*originalIndexToCompressed)[i] = -1;
                }
            }
        }
    compressedTransposedMatrix.reserve(numCompressedPatterns);
    for (std::set<NxsCharacterPattern>::iterator pIt = patternSet.begin(); pIt != patternSet.end();)
        {
        compressedTransposedMatrix.push_back(*pIt);
        std::set<NxsCharacterPattern>::iterator prevIt = pIt++;
        patternSet.erase(prevIt);
        }
    patternSet.clear();
}


unsigned NxsCompressDiscreteMatrix(
  const NxsCXXDiscreteMatrix & mat,			/**< is the data source */
  std::set<NxsCharacterPattern> & patternSet, /* matrix that will hold the compressed columns */
  std::vector<const NxsCharacterPattern *> * compressedIndexPattern, /** if not 0L, this will be filled to provide a map from an index in `compressedTransposedMatrix` to the original character count */
  const NxsUnsignedSet * taxaToInclude,	/**< if not 0L, this should be  the indices of the taxa in `mat` to include (if 0L all characters will be included). Excluding taxa will result in shorter patterns (the skipped taxa will not be filled with empty codes, instead the taxon indexing will be frameshifted -- the client code must keep track of these frameshifts). */
  const NxsUnsignedSet * charactersToInclude)
    {
    const unsigned origNumPatterns = (unsigned) patternSet.size();
	unsigned ntax = mat.getNTax();
	unsigned patternLength = ntax;
	unsigned nchar = mat.getNChar();
	if (compressedIndexPattern)
	    {
	    compressedIndexPattern->resize(nchar);
	    }
	NxsUnsignedSet allTaxaInds;
	if (taxaToInclude)
	    {
	    if (taxaToInclude->empty())
	        return 0; // might want to warn about this!
	    const unsigned lastTaxonIndex = *(taxaToInclude->rbegin());
	    if (lastTaxonIndex >= ntax)
	        throw NxsException("Taxon index in taxaToInclude argument to NxsCompressDiscreteMatrix is out of range");
        patternLength -= taxaToInclude->size();
	    }
    else
        {
        for (unsigned i = 0; i < ntax; ++i)
            allTaxaInds.insert(i);
        taxaToInclude = &allTaxaInds;
        }
	if (charactersToInclude)
	    {
	    if (charactersToInclude->empty())
	        return 0; // might want to warn about this!
	    const unsigned lastColumnIndex = *(charactersToInclude->rbegin());
	    if (lastColumnIndex >= nchar)
	        throw NxsException("Character index in charactersToInclude argument to NxsCompressDiscreteMatrix is out of range");
	    }

    // Create actingWeights vector and copy the integer weights from mat into it
    // If there are no integer weights in mat, copy the floating point weights instead
    // if floating point weights have been defined
	const std::vector<int> & iwts = mat.getIntWeightsConst();
	std::vector<double> actingWeights(nchar, 1.0);
	bool weightsSpecified = false;
	bool weightsAsInts = false;
	if (!iwts.empty())
		{
		NCL_ASSERT(iwts.size() >= nchar);
		weightsSpecified = true;
		weightsAsInts = true;
		for (unsigned j = 0; j < nchar; ++j)
			actingWeights[j] = (double)iwts.at(j);
		}
	else
		{
		const std::vector<double> & dwts = mat.getDblWeightsConst();
		if (!dwts.empty())
			{
    		weightsSpecified = true;
			actingWeights = dwts;
			NCL_ASSERT(actingWeights.size() == nchar);
			}
		}

    // Set corresponding actingWeights elements to zero if any characters have been excluded in mat
	const NxsUnsignedSet & excl = mat.getExcludedCharIndices();
	for (NxsUnsignedSet::const_iterator eIt = excl.begin(); eIt != excl.end(); ++eIt)
		{
		NCL_ASSERT(*eIt < nchar);
		actingWeights[*eIt] = 0.0;
		}
	const double * wts = &(actingWeights[0]);
	
	NxsCharacterPattern patternTemp;
    patternTemp.count = 1;
	for (unsigned j = 0; j < nchar; ++j)
		{
        double patternWeight = wts[j];
        bool shouldInclude = (charactersToInclude == 0L || (charactersToInclude->find(j) != charactersToInclude->end()));
        if (patternWeight > 0.0 &&  shouldInclude)
            {
            // Build up a vector representing the pattern of state codes at this site
            patternTemp.stateCodes.clear();
            patternTemp.stateCodes.reserve(patternLength);
            patternTemp.sumOfPatternWeights = patternWeight;

            unsigned indexInPattern = 0;
            for (NxsUnsignedSet::const_iterator taxIndIt = taxaToInclude->begin(); taxIndIt != taxaToInclude->end(); ++taxIndIt, ++indexInPattern)
                {
                const unsigned taxonIndex = *taxIndIt;
                const NxsCDiscreteStateSet * row	= mat.getRow(taxonIndex);
                const NxsCDiscreteStateSet code = row[j];
                patternTemp.stateCodes.push_back(code);
                }
            NCL_ASSERT(indexInPattern == patternLength);

            std::set<NxsCharacterPattern>::iterator lowBoundLoc = patternSet.lower_bound(patternTemp);
            if ((lowBoundLoc == patternSet.end()) || (patternTemp < *lowBoundLoc))
                {
                std::set<NxsCharacterPattern>::iterator insertedIt = patternSet.insert(lowBoundLoc, patternTemp);
                if (compressedIndexPattern)
                    {
                    const NxsCharacterPattern & patInserted = *insertedIt;
                    (*compressedIndexPattern)[j] = &patInserted;
                    }
                }
            else
                {
                NCL_ASSERT(patternTemp == *lowBoundLoc);
                lowBoundLoc->sumOfPatternWeights += patternWeight;
                lowBoundLoc->count += 1;
                if (compressedIndexPattern)
                    {
                    (*compressedIndexPattern)[j] = &(*lowBoundLoc);
                    }
                }
            }
		}
	return (unsigned)patternSet.size() - origNumPatterns;	
    }

/*----------------------------------------------------------------------------------------------------------------------
|	Copies data from `mat' to `pattern_vect' and `pattern_counts'. The `pattern_vect' vector holds the patterns while
|	`pattern_counts' holds the count of the number of sites having each pattern. Additionally, the vectors 
|	`pattern_to_sites' and `charIndexToPatternIndex' are built: `pattern_to_sites' allows you to get a list of sites
|	given a specific pattern, and `charIndexToPatternIndex' lets you find the index of a pattern in `pattern_vect' and
|	`pattern_counts' given an original site index.
*/
unsigned NxsCompressDiscreteMatrix(
  const NxsCXXDiscreteMatrix & mat,
  std::vector<NxsCharacterPattern> & compressedTransposedMatrix,
  std::vector<int> * originalIndexToCompressed,
  std::vector<std::set<unsigned> > * compressedIndexToOriginal,
  const NxsUnsignedSet * taxaToInclude,
  const NxsUnsignedSet * charactersToInclude)
	{
	std::set<NxsCharacterPattern> patternSet;
	std::vector<const NxsCharacterPattern *> toPatternMap;
	std::vector<const NxsCharacterPattern *> *toPatternMapPtr = 0L;
	if (originalIndexToCompressed != 0L || compressedIndexToOriginal != 0L)
	    toPatternMapPtr = &toPatternMap;

	NxsCompressDiscreteMatrix(mat, patternSet, toPatternMapPtr, taxaToInclude, charactersToInclude);
    const unsigned numPatternsAdded = (unsigned const)patternSet.size();
	
	NxsConsumePatternSetToPatternVector(patternSet, compressedTransposedMatrix, toPatternMapPtr, originalIndexToCompressed, compressedIndexToOriginal);
	return numPatternsAdded;
	}

void NxsTransposeCompressedMatrix(
  const std::vector<NxsCharacterPattern> & compressedTransposedMatrix, 
  ScopedTwoDMatrix<NxsCDiscreteStateSet> & destination,
  std::vector<unsigned> * patternCounts,
  std::vector<double> * patternWeights)
{
	const unsigned npatterns = (unsigned const)compressedTransposedMatrix.size();
	if (npatterns == 0)
	    {
	    destination.Initialize(0, 0);
	    return;
	    }
	const unsigned ntaxa = (unsigned const)compressedTransposedMatrix[0].stateCodes.size();
	destination.Initialize(ntaxa, npatterns);
    NxsCDiscreteStateSet ** matrix = destination.GetAlias();			/** taxa x characters matrix of indices of state sets */
    if (patternCounts)
        patternCounts->resize(npatterns);
    if (patternWeights)
        patternWeights->resize(npatterns);
	for (unsigned p = 0; p < npatterns; ++p)
		{
		const NxsCharacterPattern & pattern = compressedTransposedMatrix[p];
		const std::vector<NxsCDiscreteState_t> & states = pattern.stateCodes;
		for (unsigned t = 0; t < ntaxa; ++t)
		    matrix[t][p] = states[t];
        if (patternCounts)
            (*patternCounts)[p] = pattern.count;
        if (patternWeights)
            (*patternWeights)[p] = pattern.sumOfPatternWeights;
		}
}

NxsCXXDiscreteMatrix::NxsCXXDiscreteMatrix(const NxsCharactersBlock & cb, bool gapsToMissing, const NxsUnsignedSet * toInclude, bool standardizeCoding)
	{
	Initialize(&cb, gapsToMissing, toInclude, standardizeCoding);
	}

void NxsCXXDiscreteMatrix::Initialize(const NxsCharactersBlock * cb, bool gapsToMissing, const NxsUnsignedSet * toInclude, bool standardizeCoding)
{
	this->nativeCMatrix.stateList = 0L;
	this->nativeCMatrix.stateListPos = 0L;
	this->nativeCMatrix.matrix = 0L;
	this->nativeCMatrix.symbolsList = 0L;
	this->nativeCMatrix.nStates = 0;
	this->nativeCMatrix.nChar = 0;
	this->nativeCMatrix.nTax = 0L;
	this->nativeCMatrix.nObservedStateSets = 0;
	this->nativeCMatrix.datatype = NxsAltGeneric_Datatype;
	this->symbolsStringAlias.clear();
	this->matrixAlias.Initialize(0, 0);
	this->stateListAlias.clear();
	this->stateListPosAlias.clear();
	this->intWts.clear();
	this->dblWts.clear();
	this->activeExSet.clear();
	if (cb == NULL)
		return;
	std::vector<const NxsDiscreteDatatypeMapper *> mappers = cb->GetAllDatatypeMappers();
	if (mappers.empty() || mappers[0] == NULL)
		throw NxsException("no mappers");

	std::set <const NxsDiscreteDatatypeMapper * > usedMappers;
	NxsUnsignedSet scratchSet;
	if (toInclude == 0L)
		{
		for (unsigned i = 0; i < cb->GetNChar(); ++i)
			scratchSet.insert(i);
		toInclude = & scratchSet;
	 	}
	for (NxsUnsignedSet::const_iterator indIt = toInclude->begin(); indIt != toInclude->end(); ++indIt)
		{
		unsigned charIndex = *indIt;
		usedMappers.insert(cb->GetDatatypeMapperForChar(charIndex));
		}
	
	
	if (usedMappers.size() > 1)
		throw NxsException("too many mappers");
	if (usedMappers.empty())
		throw NxsException("no mappers - or empty charset");
	

	const NxsDiscreteDatatypeMapper & mapper = **usedMappers.begin();
	const NxsDiscreteStateMatrix & rawMatrix = cb->GetRawDiscreteMatrixRef();

	NxsCharactersBlock::DataTypesEnum inDatatype = mapper.GetDatatype();
	if (inDatatype < LowestNxsCDatatype || inDatatype > HighestNxsCDatatype)
		throw NxsException("Datatype cannot be converted to NxsCDiscreteMatrix");
	this->nativeCMatrix.datatype = NxsAltDatatypes(inDatatype);
	this->nativeCMatrix.nStates = mapper.GetNumStates();
	const std::string fundamentalSymbols = mapper.GetSymbols();
	const std::string fundamentalSymbolsPlusGaps = mapper.GetSymbolsWithGapChar();
	const bool hadGaps = !(fundamentalSymbols == fundamentalSymbolsPlusGaps);

	this->symbolsStringAlias = fundamentalSymbols;
	char missingSym = cb->GetMissingSymbol();
	const NxsCDiscreteState_t newMissingStateCode = (standardizeCoding ? (NxsCDiscreteState_t) this->nativeCMatrix.nStates : (NxsCDiscreteState_t) NXS_MISSING_CODE);
	NCL_ASSERT((int)NXS_MISSING_CODE < 0);
	NCL_ASSERT((int)NXS_GAP_STATE_CODE < 0);
	NxsDiscreteStateCell sclOffsetV;
	if (hadGaps)
		sclOffsetV = std::min((NxsDiscreteStateCell)NXS_GAP_STATE_CODE, (NxsDiscreteStateCell)NXS_MISSING_CODE);
	else
		sclOffsetV = NXS_MISSING_CODE;
	const NxsDiscreteStateCell sclOffset(sclOffsetV);

	const NxsDiscreteStateCell negSCLOffset = -sclOffset;
	const unsigned nMapperStateCodes = mapper.GetNumStateCodes();
	const unsigned recodeVecLen = nMapperStateCodes;
	const unsigned nMapperPosStateCodes = nMapperStateCodes + sclOffset;
	std::vector<NxsCDiscreteState_t> recodeVec(recodeVecLen + negSCLOffset, -2);
	NxsCDiscreteState_t * recodeArr = &recodeVec[negSCLOffset];

	if (fundamentalSymbols.length() < this->nativeCMatrix.nStates)
		throw NxsException("Fundamental states missing from the symbols string");
	const unsigned nfun_sym = (const unsigned)fundamentalSymbols.length();
	for (NxsCDiscreteState_t i = 0; i < (NxsCDiscreteState_t)this->nativeCMatrix.nStates; ++i)
		{
		if (i < (NxsCDiscreteState_t)nfun_sym && (NxsCDiscreteState_t)fundamentalSymbols[i] == '\0' && mapper.PositionInSymbols(fundamentalSymbols[i]) != (NxsDiscreteStateCell) i)
			{
			NCL_ASSERT(i >= (NxsCDiscreteState_t)nfun_sym || fundamentalSymbols[i] == '\0' || mapper.PositionInSymbols(fundamentalSymbols[i]) == (NxsDiscreteStateCell) i);
			}
#		if !defined (NDEBUG)
			const std::set<NxsDiscreteStateCell>	 & ss =  mapper.GetStateSetForCode(i);
			NCL_ASSERT(ss.size() == 1);
			NCL_ASSERT(*ss.begin() == i);
#		endif
		stateListAlias.push_back(1);
		stateListAlias.push_back(i);
		stateListPosAlias.push_back((unsigned) 2*i);
		recodeArr[i] = i;
		}

	//NXS_INVALID_STATE_CODE

	if (hadGaps)
		{
		if (standardizeCoding)
		    recodeArr[NXS_GAP_STATE_CODE] = ((hadGaps && gapsToMissing)? newMissingStateCode : -1);
        else
		    recodeArr[NXS_GAP_STATE_CODE] = NXS_GAP_STATE_CODE;
        }

	if (missingSym == '\0')
		missingSym = (hadGaps ? mapper.GetGapSymbol() : '?');
	else
		{
		NCL_ASSERT(NXS_MISSING_CODE == mapper.GetStateCodeStored(missingSym));
		}
	recodeArr[NXS_MISSING_CODE] = newMissingStateCode;
	const unsigned nCodesInMissing  = this->nativeCMatrix.nStates + (gapsToMissing ?  0 : 1);
	if (standardizeCoding)
	    {
	    this->symbolsStringAlias.append(1, missingSym);
        stateListPosAlias.push_back(2*this->nativeCMatrix.nStates);
        stateListAlias.push_back(nCodesInMissing);
        if (!gapsToMissing)
            stateListAlias.push_back(-1);
        for (NxsCDiscreteState_t i = 0; i < (NxsCDiscreteState_t)this->nativeCMatrix.nStates; ++i)
            stateListAlias.push_back(i);
        }

	NxsCDiscreteState_t nextStateCode = (standardizeCoding ? (newMissingStateCode + 1) : this->nativeCMatrix.nStates);
	for (NxsDiscreteStateCell i = (NxsDiscreteStateCell)this->nativeCMatrix.nStates; i < (NxsDiscreteStateCell) nMapperPosStateCodes; ++i)
		{
		const std::set<NxsDiscreteStateCell>	 &ss = mapper.GetStateSetForCode( i);
		const unsigned ns = (const unsigned)ss.size();
		const bool mapToMissing  = (!mapper.IsPolymorphic(i) && (nCodesInMissing + 1 == ns || nCodesInMissing == ns));
		if (mapToMissing)
			recodeArr[i] = newMissingStateCode;
		else
			{
			recodeArr[i] = nextStateCode++;
			stateListPosAlias.push_back((unsigned)stateListAlias.size());
			stateListAlias.push_back(ns);
			for (std::set<NxsDiscreteStateCell>::const_iterator sIt = ss.begin(); sIt != ss.end(); ++sIt)
				stateListAlias.push_back((NxsCDiscreteState_t) *sIt);
			std::string stateName = mapper.StateCodeToNexusString(i);
			if (stateName.length() != 1)
				this->symbolsStringAlias.append(1, ' ');
			else
				this->symbolsStringAlias.append(1, stateName[0]);
			}
		}
	NCL_ASSERT(stateListPosAlias.size() == (unsigned)nextStateCode);
	NCL_ASSERT(symbolsStringAlias.size() == (unsigned)nextStateCode);
	this->nativeCMatrix.nObservedStateSets = nextStateCode;

	this->nativeCMatrix.nTax = (unsigned)rawMatrix.size();
	this->nativeCMatrix.nChar = (this->nativeCMatrix.nTax == 0 ? 0 : toInclude->size());
	this->matrixAlias.Initialize(this->nativeCMatrix.nTax, this->nativeCMatrix.nChar);
	nativeCMatrix.matrix = matrixAlias.GetAlias();
	const unsigned nt = this->nativeCMatrix.nTax;
	const unsigned nc = this->nativeCMatrix.nChar;
	for (unsigned r = 0; r < nt; ++r)
		{
		NxsCDiscreteStateSet	 * recodedRow = nativeCMatrix.matrix[r];
		const std::vector<NxsDiscreteStateCell> & rawRowVec = rawMatrix[r];
		if (rawRowVec.empty())
			{
			NxsCDiscreteState_t recodedMissing = recodeArr[NXS_MISSING_CODE];
			for (unsigned c = 0; c < nc; ++c)
				*recodedRow++ = recodedMissing;
			}
		else
			{
			NCL_ASSERT(rawRowVec.size() >= nc);
			const NxsDiscreteStateCell * rawRow = &rawRowVec[0];
		    NxsUnsignedSet::const_iterator includedIt = toInclude->begin();
			for (unsigned c = 0; c < nc; ++c)
				{
				unsigned charIndex = *includedIt++;
				const NxsDiscreteStateCell rawC = rawRow[charIndex];
				if ((unsigned)(rawC +  negSCLOffset) >= recodeVecLen)
					{
					NCL_ASSERT((unsigned)(rawC +  negSCLOffset) < recodeVecLen);
					}
				NCL_ASSERT(rawC >= sclOffset);
				const NxsCDiscreteState_t recodedC = recodeArr[rawC];
				NCL_ASSERT(recodedC > -2 || !standardizeCoding);
				NCL_ASSERT(recodedC < nextStateCode);
				*recodedRow++ = recodedC;
				}
			}
		}
	nativeCMatrix.symbolsList = symbolsStringAlias.c_str();
	nativeCMatrix.stateListPos = &stateListPosAlias[0];
	nativeCMatrix.stateList = &stateListAlias[0];

	intWts.clear();
	dblWts.clear();
	const NxsTransformationManager &tm = cb->GetNxsTransformationManagerRef();
	intWts = tm.GetDefaultIntWeights();
	if (intWts.empty())
		dblWts = tm.GetDefaultDoubleWeights();
	activeExSet = cb->GetExcludedIndexSet();
}

/**
 *	Constructs  from the native C struct NxsCDiscreteMatrix
 *		by deep copy.
 */
NxsCXXDiscreteMatrix::NxsCXXDiscreteMatrix(const NxsCDiscreteMatrix & mat)
	:nativeCMatrix(mat),//aliases pointers, but we'll fix this below
	symbolsStringAlias(mat.symbolsList),
	matrixAlias(mat.nTax, mat.nChar),
	stateListPosAlias(mat.stateListPos, (mat.stateListPos + mat.nObservedStateSets))
	{
	nativeCMatrix.symbolsList = symbolsStringAlias.c_str();
	nativeCMatrix.stateListPos = &stateListPosAlias[0];
	if (mat.nObservedStateSets > 0)
		{
		const unsigned lastStateIndex = nativeCMatrix.stateListPos[nativeCMatrix.nObservedStateSets - 1];
		const unsigned lenAmbigList = lastStateIndex + mat.stateList[lastStateIndex] + 1;
		//	cout << "lenAmbigList = "<< lenAmbigList <<endl;
		stateListAlias.reserve(lenAmbigList);
		ncl_copy(mat.stateList, (mat.stateList + lenAmbigList), std::back_inserter(stateListAlias));
		}
	nativeCMatrix.stateList = &stateListAlias[0];
	nativeCMatrix.matrix = matrixAlias.GetAlias();

	// cout << "Matrix in NxsCXXDiscreteMatrix ctor:" << mat.nTax << ' '<< mat.nChar<< endl;
	for (unsigned i = 0; i < mat.nTax; ++i)
		{
		if (mat.nChar > 0)
			ncl_copy(mat.matrix[i], mat.matrix[i] + mat.nChar, nativeCMatrix.matrix[i]);
		}

	}

