//	Copyright (C) 2007 Paul O. Lewis
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

#ifndef NCL_NXSUNALIGNEDBLOCK_H
#define NCL_NXSUNALIGNEDBLOCK_H

#include "ncl/nxsdefs.h"
#include "ncl/nxstaxablock.h"
#include "ncl/nxscharactersblock.h"
//@POL Note: This file is not yet ready for use (Paul Lewis, 19-May-2007)

class NxsTaxaBlockAPI;

/*!
	This class handles reading and storage for the NEXUS block UNALIGNED. It overrides the member functions Read and
	Reset, which are abstract virtual functions in the base class NxsBlock.
>
	Below is a table showing the correspondence between the elements of an UNALIGNED block in a NEXUS file and the
	variables and member functions of the NxsUnalignedBlock class that can be used to access each piece of information
	stored. Items in parenthesis should be viewed as "see also" items.
>
	NEXUS		  Command		 Data			Member
	Command		  Atribute		 Member			Functions
	---------------------------------------------------------------------
	DIMENSIONS	  NEWTAXA		 newtaxa

				  NTAX			 ntax			GetNTax

	FORMAT		  DATATYPE		 datatype		GetDataType

				  RESPECTCASE	 respectingCase IsRespectCase

				  MISSING		 missing		GetMissingSymbol

				  SYMBOLS		 symbols		GetSymbols

				  EQUATE		 equates		GetEquateKey
												GetEquateValue
												GetNumEquates

				  (NO)LABELS	 labels			IsLabels

	TAXLABELS					 taxonLabels	GetTaxonLabels

	MATRIX						 matrix			GetState
												GetInternalRepresentation
												GetNumStates
												GetNumMatrixRows
												IsPolymorphic
>
*/
class NxsUnalignedBlock
  : public NxsBlock, public NxsTaxaBlockSurrogate
	{
	public:

		class NxsX_NoDataForTaxon
			{
			public:
				NxsX_NoDataForTaxon(unsigned i) : taxon_index(i) {}
				unsigned taxon_index;
			};	/* thrown if a function is called with an index to a taxon for which no data is stored */

								NxsUnalignedBlock(NxsTaxaBlockAPI * tb);
		virtual					~NxsUnalignedBlock();

		void					ShowStateLabels(std::ostream & out, NxsDiscreteDatum s);
		NxsCharactersBlock::DataTypesEnum	GetDataType() const ;
		NxsCharactersBlock::DataTypesEnum	GetOriginalDataType() const ;
		const NxsDiscreteStateRow * GetDiscreteMatrixRow(unsigned taxInd) const
			{
			if (taxInd >= uMatrix.size())
				return NULL;
			return &uMatrix[taxInd];
			}
		NxsDiscreteStateRow		GetInternalRepresentation(unsigned i, unsigned j);
		unsigned				GetNTaxWithData();
		unsigned				GetNTaxTotal();
		unsigned				GetNTaxTotal() const;
		unsigned				GetNumEquates();
		unsigned				GetNumMatrixRows();
		unsigned				GetNumStates(unsigned i, unsigned j);
		unsigned				NumCharsForTaxon(unsigned i);
		char					GetMissingSymbol();
		bool					IsLabels();
		bool					IsMissingState(unsigned i, unsigned j);
		bool					IsPolymorphic(unsigned i, unsigned j);
		bool					IsRespectCase();
		unsigned				GetStateSymbolIndex(unsigned i, unsigned j, unsigned k = 0);	// added by mth for standard data types
		const char *			GetSymbols();
		virtual void			DebugShowMatrix(std::ostream & out, const char * marginText = NULL) NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
		virtual void			Report(std::ostream & out) NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
		virtual void			Reset();
		void					SetNexus(NxsReader *nxsptr)
			{
			NxsBlock::SetNexus(nxsptr);
			NxsTaxaBlockSurrogate::SetNexusReader(nxsptr);
			}
		virtual const std::string & GetBlockName() const
			{
			return NCL_BLOCKTYPE_ATTR_NAME;
			}

		void					WriteAsNexus(std::ostream & out) const;
		void					WriteFormatCommand(std::ostream & out) const;
		void					WriteMatrixCommand(std::ostream & out) const;
		const char *			GetDatatypeName() const
			{
			return NxsCharactersBlock::GetNameOfDatatype(datatype);
			}

		virtual VecBlockPtr		GetImpliedBlocks()
			{
			return GetCreatedTaxaBlocks();
			}

		/*only used it the linkAPI is enabled*/
		virtual void		HandleLinkCommand(NxsToken & token)
			{
			HandleLinkTaxaCommand(token);
			}
		virtual void		WriteLinkCommand(std::ostream &out) const
			{
			WriteLinkTaxaCommand(out);
			}

		/*---------------------------------------------------------------------------------------
		| Results in aliasing of the taxa, assumptionsBlock blocks!
		*/
		NxsUnalignedBlock & operator=(const NxsUnalignedBlock &other)
			{
			Reset();
			CopyBaseBlockContents(static_cast<const NxsBlock &>(other));
			CopyTaxaBlockSurrogateContents(other);
			CopyUnalignedBlockContents(other);
			return *this;
			}

		/*---------------------------------------------------------------------------------------
		| Results in aliasing of the taxa, assumptionsBlock blocks!
		*/
		virtual void CopyUnalignedBlockContents(const NxsUnalignedBlock &other)
			{
			nChar = other.nChar;
			nTaxWithData = other.nTaxWithData;
			matchchar = other.matchchar;
			respectingCase = other.respectingCase;
			transposing = other.transposing;
			labels = other.labels;
			missing = other.missing;
			gap = other.gap;
			symbols = other.symbols;
			equates = other.equates;
			mapper = other.mapper;
			uMatrix = other.uMatrix;
			datatype = other.datatype;
			statesFormat = other.statesFormat;
			}

		virtual NxsUnalignedBlock * Clone() const
			{
			NxsUnalignedBlock * a = new NxsUnalignedBlock(taxa);
			*a = *this;
			return a;
			}
		bool 		SwapEquivalentTaxaBlock(NxsTaxaBlockAPI * tb)
		{
			return SurrogateSwapEquivalentTaxaBlock(tb);
		}
        std::string GetMatrixRowAsStr(const unsigned rowIndex) const;
	protected:
		bool					IsInSymbols(char ch);
		void					HandleDimensions(NxsToken & token);
		void					HandleEndblock(NxsToken & token);
		virtual void			HandleFormat(NxsToken & token);
		virtual void			HandleMatrix(NxsToken & token);
		virtual bool			HandleNextState(NxsToken & token, unsigned taxInd, unsigned charInd, NxsDiscreteStateRow & new_row, const NxsString &);
		virtual void			Read(NxsToken & token);
		void					ResetSymbols();
		std::string				FormatState(NxsDiscreteDatum x) const;

		void					WriteStatesForMatrixRow(std::ostream &out, unsigned currTaxonIndex) const;

		unsigned				nChar;				/* number of columns in matrix	*/
		unsigned				nTaxWithData;		/* number of non empty rows in the matrix*/

		char					matchchar;			/* match symbol to use in matrix */
		bool					respectingCase;		/* if true, RESPECTCASE keyword specified in FORMAT command */
		bool					transposing;		/* indicates matrix will be in transposed format */
		bool					labels;				/* indicates whether or not labels will appear on left side of matrix */

		char					missing;			/* missing data symbol */
		char                    gap; /* gap symbol, will often be \0, but can be - */

		std::string				symbols;			/* list of valid character state symbols */
		std::map<char, NxsString> equates;			/* list of associations defined by EQUATE attribute of FORMAT command */

		NxsDiscreteDatatypeMapper mapper;
		NxsDiscreteStateMatrix	uMatrix;		/* storage for unaligned data */

	private:
		NxsCharactersBlock::DataTypesEnum			datatype;			/* flag variable (see datatypes enum) */
		NxsCharactersBlock::DataTypesEnum			originalDatatype;			/* flag variable (see datatypes enum) */
		NxsCharactersBlock::StatesFormatEnum		statesFormat;

		NxsDiscreteStateCell						GetStateIndex(unsigned i, unsigned j, unsigned k);
		void					ResetDatatypeMapper();
		bool					TaxonIndHasData(const unsigned ind) const;
		friend class PublicNexusReader;
		friend class MultiFormatReader;
	};

class NxsUnalignedBlockFactory
	:public NxsBlockFactory
	{
	public:
		virtual NxsUnalignedBlock  *GetBlockReaderForID(const std::string & NCL_BLOCKTYPE_ATTR_NAME, NxsReader *reader, NxsToken *token);
	};

/*!
	Returns datatype listed in the CHARACTERS block.
	The original datatype can differ from the current datatype if the symbols list of a built in type was augmented
	(thus converting it to standard).
*/
inline NxsCharactersBlock::DataTypesEnum NxsUnalignedBlock::GetOriginalDataType() const
	{
	return originalDatatype;
	}


/*!
	Returns value of `datatype' as an unsigned integer. If you want the name of the datatype, you should call
	NxsUnalignedBlock::GetDatatypeName instead.
*/
inline NxsCharactersBlock::DataTypesEnum NxsUnalignedBlock::GetDataType() const
	{
	return datatype;
	}


/*!
	Returns the missing data symbol currently in effect. If no missing data symbol specified, returns '\0'.
*/
inline char NxsUnalignedBlock::GetMissingSymbol()
	{
	return missing;
	}

/*!
	Returns the number of taxa that have data (or will have data according to the Dimensions command, if the matrix
		has not been read.
*/
inline unsigned NxsUnalignedBlock::GetNTaxWithData()
	{
	return nTaxWithData;
	}

/*!
	Returns the number of taxa in the taxa block associated with the unaligned block.
*/
inline unsigned NxsUnalignedBlock::GetNTaxTotal()
	{
	return (unsigned)uMatrix.size();
	}

/*!
	Returns the number of taxa in the taxa block associated with the unaligned block.
*/
inline unsigned NxsUnalignedBlock::GetNTaxTotal() const
	{
	return (unsigned)uMatrix.size();
	}

/*!
	Returns the number of stored equate associations.
*/
inline unsigned NxsUnalignedBlock::GetNumEquates()
	{
	return (unsigned)equates.size();
	}

/*!
	Returns the number of actual rows in `matrix'. This number is equal to `ntax', and hence this function is identical
	to GetNTax. Note that `ntax' can be smaller than `ntaxTotal' since the user did not have to provide data for all
	taxa specified in the TAXA block.
*/
inline unsigned NxsUnalignedBlock::GetNumMatrixRows()
	{
	return (unsigned)uMatrix.size();
	}

/*!
	Returns data member `symbols'. Warning: returned value may be NULL.
*/
inline const char * NxsUnalignedBlock::GetSymbols()
	{
	return symbols.c_str();
	}

/*!
	Returns true if LABELS was specified in the FORMAT command, false otherwise.
*/
inline bool NxsUnalignedBlock::IsLabels()
	{
	return labels;
	}

/*!
	Returns true if RESPECTCASE was specified in the FORMAT command, false otherwise.
*/
inline bool NxsUnalignedBlock::IsRespectCase()
	{
	return respectingCase;
	}

#endif
