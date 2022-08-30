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
#ifndef NCL_NXSTAXAASSOCIATIONBLOCK_H
#define NCL_NXSTAXAASSOCIATIONBLOCK_H

#include "ncl/nxsdefs.h"
#include "ncl/nxstaxablock.h"

/*! 
*/
class NxsTaxaAssociationBlockAPI
  : public NxsBlock
  {
  public:
		virtual NxsTaxaBlockAPI * GetFirstTaxaBlock() const = 0;
		virtual NxsTaxaBlockAPI * GetSecondTaxaBlock() const = 0;
		virtual std::set<unsigned> GetAssociatesForTaxonInFirstBlock(unsigned) const = 0;
		virtual std::set<unsigned> GetAssociatesForTaxonInSecondBlock(unsigned) const = 0;
  };

/*! The default implementation of the NxsTaxaBlockAPI that is used to parse TAXA blocks into a list of
	unique (case-insensitive) labels.

*/
class NxsTaxaAssociationBlock
  : public NxsTaxaAssociationBlockAPI
	{
        typedef std::map<unsigned, std::set<unsigned> > AssociationMap;
	public:
							NxsTaxaAssociationBlock();
		virtual				~NxsTaxaAssociationBlock() {}

		virtual void AddAssociation(unsigned firstIndex, const std::set<unsigned> & secIndices) {
		    std::set<unsigned> & former = this->firstToSecond[firstIndex];
		    for (std::set<unsigned>::const_iterator sIt = secIndices.begin(); sIt != secIndices.end(); ++sIt) {
		        const unsigned & secIndex = *sIt;
		        former.insert(secIndex);
		        this->secondToFirst[secIndex].insert(firstIndex);
		    }
		}
		
		
		virtual void SetFirstTaxaBlock(NxsTaxaBlockAPI *f) {
		    NxsTaxaBlockAPI * s = this->secondTaxaBlock;
		    this->Reset();
		    this->secondTaxaBlock = s;
		    this->firstTaxaBlock = f;
		}
		virtual NxsTaxaBlockAPI * GetFirstTaxaBlock() const {
		    return this->firstTaxaBlock;
		}
		virtual void SetSecondTaxaBlock(NxsTaxaBlockAPI *s) {
		    NxsTaxaBlockAPI * f = this->firstTaxaBlock;
		    this->Reset();
		    this->secondTaxaBlock = s;
		    this->firstTaxaBlock = f;
		}
		virtual NxsTaxaBlockAPI * GetSecondTaxaBlock() const {
		    return this->secondTaxaBlock;
		}
		virtual std::set<unsigned> GetAssociatesForTaxonInFirstBlock(unsigned i) const {
		    AssociationMap::const_iterator m = this->firstToSecond.find(i);
		    if (m == this->firstToSecond.end()) {
		        return std::set<unsigned>();
		    }
		    return m->second;
		}
		virtual std::set<unsigned> GetAssociatesForTaxonInSecondBlock(unsigned i) const  {
		    AssociationMap::const_iterator m = this->secondToFirst.find(i);
		    if (m == this->secondToFirst.end()) {
		        return std::set<unsigned>();
		    }
		    return m->second;
		}

		virtual void		Report(std::ostream &out) NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
		virtual void 		Reset();
		void				WriteAsNexus(std::ostream &out) const;


		NxsTaxaAssociationBlock &operator=(const NxsTaxaAssociationBlock &other)
			{
			Reset();
			CopyBaseBlockContents(static_cast<const NxsBlock &>(other));
			CopyTaxaAssociationContents(other);
			return *this;
			}

		void CopyTaxaAssociationContents(const NxsTaxaAssociationBlock &other)
			{
			firstToSecond = other.firstToSecond;
			secondToFirst = other.secondToFirst;
			firstTaxaBlock = other.firstTaxaBlock;
			secondTaxaBlock = other.secondTaxaBlock;
			}
		NxsTaxaAssociationBlock * Clone() const
			{
			NxsTaxaAssociationBlock *taxa = new NxsTaxaAssociationBlock();
			*taxa = *this;
			return taxa;
			}
	protected:
        AssociationMap firstToSecond;
        AssociationMap secondToFirst;
        NxsTaxaBlockAPI * firstTaxaBlock;
        NxsTaxaBlockAPI * secondTaxaBlock;

		virtual void 	Read(NxsToken &token);
        void HandleTaxaCommand(NxsToken &token);
        void HandleAssociatesCommand(NxsToken &token);
        NxsTaxaBlockAPI * ProcessTaxaBlockName(const NxsString & value,  NxsToken &token) const;

};

class NxsTaxaAssociationBlockFactory
	:public NxsBlockFactory
	{
	public:
		virtual NxsTaxaAssociationBlock  *	GetBlockReaderForID(const std::string & NCL_BLOCKTYPE_ATTR_NAME, NxsReader *reader, NxsToken *token);
	};

#endif

