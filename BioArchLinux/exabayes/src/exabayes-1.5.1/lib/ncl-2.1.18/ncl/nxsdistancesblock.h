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

#ifndef NCL_NXSDISTANCESBLOCK_H
#define NCL_NXSDISTANCESBLOCK_H

#include "ncl/nxsdefs.h"
#include "ncl/nxstaxablock.h"
#include "ncl/nxsdistancedatum.h"

/*!
	This class handles reading and storage for the NEXUS block DISTANCES. It overrides the member functions Read and
	Reset, which are abstract virtual functions in the base class NxsBlock. Below is a table showing the correspondence
	between the elements of a DISTANCES block and the variables and member functions that can be used to access each
	piece of information stored.
>
	NEXUS command   Command attribute  Data Members        Member Functions
	------------------------------------------------------------------------
	DIMENSIONS      NEWTAXA            newtaxa

	                NTAX               ntax                GetNtax

	                NCHAR              nchar               GetNchar

	FORMAT          TRIANGLE           triangle            GetTriangle
	                                                       IsUpperTriangular
	                                                       IsLowerTriangular
	                                                       IsRectangular

	                [NO]DIAGONAL       diagonal            IsDiagonal

	                [NO]LABELS         labels              IsLabels

	                MISSING            missing             GetMissingSymbol

	                INTERLEAVE         interleave          IsInterleave

	                TAXLABELS          (stored in the      (access through
					                   NxsTaxaBlockAPI        data member taxa)
									   object)

	MATRIX                             matrix              GetDistance
	                                                       IsMissing
	                                                       SetMissing
	                                                       SetDistance
	------------------------------------------------------------------------
>
*/
class NxsDistancesBlock
  : public NxsBlock, public NxsTaxaBlockSurrogate
	{
	public:
							NxsDistancesBlock(NxsTaxaBlockAPI *t);
		virtual				~NxsDistancesBlock();

		double				GetDistance(unsigned i, unsigned j) const;
		char				GetMissingSymbol() NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
		unsigned			GetNchar() NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
		unsigned			GetTriangle() NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
		bool				IsRectangular() NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
		bool				IsBoth() NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
		bool				IsDiagonal() NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
		bool				IsInterleave() NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
		bool				IsLabels() NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
		bool				IsLowerTriangular() NCL_COULD_BE_CONST ;  /*v2.1to2.2 1 */
		bool				IsMissing(unsigned i, unsigned j) const;
		bool				IsUpperTriangular() NCL_COULD_BE_CONST ;  /*v2.1to2.2 1 */
		virtual void		Report(std::ostream &out) NCL_COULD_BE_CONST ;  /*v2.1to2.2 1 */
		virtual void		Reset();
		void				SetDistance(unsigned i, unsigned j, double d);
		void				SetMissing(unsigned i, unsigned j);
		void				SetNchar(unsigned i);
		void				SetNexus(NxsReader *nxsptr)
			{
			NxsBlock::SetNexus(nxsptr);
			NxsTaxaBlockSurrogate::SetNexusReader(nxsptr);
			}
			/*! \ref BlockTypeIDDiscussion */
        virtual const std::string & GetBlockName() const
            {
            return NCL_BLOCKTYPE_ATTR_NAME;
            }

		enum NxsDistancesBlockEnum		/* used by data member triangle to determine which triangle(s) of the distance matrix is/are occupied */
			{
			upper			= 1,		/* matrix is upper-triangular */
			lower			= 2,		/* matrix is lower-triangular */
			both			= 3			/* matrix is rectangular */
			};

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
		void				WriteAsNexus(std::ostream &out) const;


		NxsDistancesBlock &operator=(const NxsDistancesBlock &other)
			{
			Reset();
			CopyBaseBlockContents(static_cast<const NxsBlock &>(other));
			CopyTaxaBlockSurrogateContents(other);
			CopyDistancesContents(other);
			return *this;
			}

		void CopyDistancesContents(const NxsDistancesBlock &other);
		NxsDistancesBlock * Clone() const
			{
			NxsDistancesBlock *d = new NxsDistancesBlock(taxa);
			*d = *this;
			return d;
			}
		bool 		SwapEquivalentTaxaBlock(NxsTaxaBlockAPI * tb)
		{
			return SurrogateSwapEquivalentTaxaBlock(tb);
		}

	protected:
		void				WriteFormatCommand(std::ostream &out) const;
		void				WriteMatrixCommand(std::ostream &out) const;

		void				HandleDimensionsCommand(NxsToken &token);
		void				HandleFormatCommand(NxsToken &token);
		void				HandleMatrixCommand(NxsToken &token);
		bool				HandleNextPass(NxsToken &token, unsigned &offset, std::vector<unsigned> & fileMatrixCmdOrderToTaxInd, std::set<unsigned> & taxIndsRead);
		virtual void		Read(NxsToken &token);

	private:
		NxsDistanceDatum & GetCell(unsigned i, unsigned j)
			{
			return matrix.at(i).at(j);
			}
		const NxsDistanceDatum & GetCell(unsigned i, unsigned j) const
			{
			return matrix.at(i).at(j);
			}
		typedef std::vector<NxsDistanceDatum> NxsDistanceDatumRow;
		typedef std::vector<NxsDistanceDatumRow> NxsDistanceDatumMatrix;

		unsigned			expectedNtax;		/* number of taxa (determines dimensions of the matrix) */
		unsigned			nchar;		/* the number of characters used in generating the pairwise distances */

		bool				diagonal;	/* true if diagonal elements provided when reading in DISTANCES block */
		bool				interleave;	/* true if interleave format used when reading in DISTANCES block */
		bool				labels;		/* true if taxon labels were provided when reading in DISTANCES block */

		int					triangle;	/* indicates whether matrix is upper triangular, lower triangular, or rectangular, taking on one of the elements of the NxsDistancesBlockEnum enumeration */

		char				missing;	/* the symbol used to represent missing data (e.g. '?') */

		NxsDistanceDatumMatrix	matrix;	/* the structure used for storing the pairwise distance matrix */
		friend class PublicNexusReader;
	};

typedef NxsDistancesBlock	DistancesBlock;

class NxsDistancesBlockFactory
	:public NxsBlockFactory
	{
	public:
		virtual NxsDistancesBlock  *	GetBlockReaderForID(const std::string & NCL_BLOCKTYPE_ATTR_NAME, NxsReader *reader, NxsToken *token);
	};

inline bool NxsDistancesBlock::IsBoth() NCL_COULD_BE_CONST /*v2.1to2.2 1 */
	{
	return this->IsRectangular();
	}

#endif

