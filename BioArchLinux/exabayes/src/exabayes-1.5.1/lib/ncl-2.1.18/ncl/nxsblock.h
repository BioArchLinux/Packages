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
#ifndef NCL_NXSBLOCK_H
#define NCL_NXSBLOCK_H

#include <vector>
#include "ncl/nxsdefs.h"
#include "ncl/nxsexception.h"
#include "ncl/nxstoken.h"

class NxsReader;
class NxsBlock;
class NxsTaxaBlockAPI;

typedef std::vector<NxsBlock *> VecBlockPtr;
typedef std::vector<const NxsBlock *> VecConstBlockPtr;
typedef std::pair<const NxsBlock *, std::string> BlockUniqueID;
typedef std::map<BlockUniqueID, NxsBlock *> NxsBlockMapper;



/*! This is the base class for the block interfaces that correspond to blocks
that hold ordered lists (TAXA, CHARACTERS, TREES).

	This interface is used internally during parsing, and is usually NOT
	called directly by client code.
*/
class NxsLabelToIndicesMapper
	{
	public:
		virtual ~NxsLabelToIndicesMapper(){}
		virtual unsigned GetMaxIndex() const = 0;
		virtual unsigned GetNumLabelsCurrentlyStored() const {return GetMaxIndex();}
		/* Adds the 0-based indices corresponding to a label to the set.

		 \returns the number of indices that correspond to the label (and the number
		 of items that would be added to *inds if inds points to an empty set).
		*/
		virtual unsigned GetIndicesForLabel(const std::string &label, /* label, set name or string with the 1-based numeric representation of the object */
											NxsUnsignedSet *inds /* The set of indices to add the taxa indices to (can be 0L). */
											) const = 0;
		/* Confusingly named function.
			This function looks for the index set than is named `label` in the NxsLabelToIndicesMapper
			It adds the indices from this set into `toFill` (if toFill is not NULL).
			\returns the size of the set which was named label (the number of indices that were inserted).
		*/
		virtual unsigned GetIndexSet(const std::string &label, NxsUnsignedSet * toFill) const = 0;
		/* Adds set `inds` to the collection of sets and gives it the name `label`
			\returns true if the set replaced an existing set (in this case a warning will be issued - which can generate an NxsException, if the client code wants warning to generate exceptions).
		*/
		virtual bool AddNewIndexSet(const std::string &label, const NxsUnsignedSet & inds) = 0;
		/* Adds partition `inds` to the collection of partition and gives it the name `label`
			\returns true if the set replaced an existing partition (in this case a warning will be issued - which can generate an NxsException, if the client code wants warning to generate exceptions).
		*/
		virtual bool AddNewPartition(const std::string &label, const NxsPartition & inds) = 0;

		/* Adds a new label to the collection of valid labels
			AppendNewLabel is only overloaded in Taxa and State LabelToIndexMappers, all other
			NxsLabelToIndicesMapper instances \throw NxsUnimplementedException
		*/
		virtual unsigned AppendNewLabel(std::string &/*label*/)
			{
			throw NxsUnimplementedException("AppendNewLabel called on fixed label interface");
			}
		static bool allowNumberAsIndexPlusOne;
	protected:
		static unsigned GetIndicesFromSets(const std::string &label, NxsUnsignedSet *inds, const NxsUnsignedSetMap & itemSets);
		static unsigned GetIndicesFromSetOrAsNumber(const std::string &label, NxsUnsignedSet *inds, const NxsUnsignedSetMap & itemSets, const unsigned maxInd, const char * itemType);
	};

class NxsSetVectorItemValidator;

std::string GetBlockIDTitleString(NxsBlock &);
/*!
	This is the base class from which all block classes are derived. A NxsBlock-derived class encapsulates a Nexus block
	(e.g. DATA block, TREES block, etc.). The abstract virtual function Read must be overridden for each derived class
	to provide the ability to read everything following the block name (which is read by the NxsReader object) to the
	end or endblock statement. Derived classes must provide their own data storage and access functions. The abstract
	virtual function Report must be overridden to provide some feedback to user on contents of block. The abstract
	virtual function Reset must be overridden to empty the block of all its contents, restoring it to its
	just-constructed state.
*/
class NxsBlock
	{
	friend class NxsReader;
		/* i20 */ /*v2.1to2.2 20 */
	public:
		enum NxsBlockLinkStatus
			{
			BLOCK_LINK_UNINITIALIZED = 	       0x00,
			BLOCK_LINK_UNKNOWN_STATUS =        0x01, /*backwards compatibility, this is the status of old block links*/
			BLOCK_LINK_TO_ONLY_CHOICE =        0x02,
			BLOCK_LINK_TO_MOST_RECENT =        0x04,
			BLOCK_LINK_TO_IMPLIED_BLOCK =      0x08,
			BLOCK_LINK_FROM_LINK_CMD =         0x10,
			BLOCK_LINK_EQUIVALENT_TO_IMPLIED = 0x20,
			BLOCK_LINK_UNUSED_MASK =           0x3F,
			BLOCK_LINK_USED =                  0x40
			};
		enum NxsCommandResult
			{
			STOP_PARSING_BLOCK,
			HANDLED_COMMAND,
			UNKNOWN_COMMAND
			};
							NxsBlock();
		virtual				~NxsBlock();

		virtual void SetNexus(NxsReader *nxsptr);
		NxsReader *GetNexus() const;
		virtual bool CanReadBlockType(const NxsToken & token)
			{
			return token.Equals(NCL_BLOCKTYPE_ATTR_NAME);
			}

		NxsString			GetID() const;
		bool				IsEmpty() NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */

		void				Enable();
		void				Disable();
		bool				IsEnabled() NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
		bool				IsUserSupplied() NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */

		virtual unsigned	CharLabelToNumber(NxsString s) NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
		virtual unsigned	TaxonLabelToNumber(NxsString s) const;

		virtual void		SkippingCommand(NxsString commandName);

		virtual void		HandleBlockIDCommand(NxsToken &token);
		virtual void		HandleEndblock(NxsToken &token);
		virtual void		HandleLinkCommand(NxsToken &token);
		virtual void		HandleTitleCommand(NxsToken &token);

		virtual void		Report(std::ostream &out) NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
		virtual void		Reset();

		mutable NxsString	errormsg;			/* workspace for creating error messages */


		virtual VecBlockPtr GetImpliedBlocks();
		virtual VecConstBlockPtr	GetImpliedBlocksConst() const;
		BlockUniqueID		GetInstanceIdentifier() const
			{
			return BlockUniqueID(this, GetInstanceName());
			}
		/* i21 */ /*v2.1to2.2 21 */
		const std::string  &GetInstanceName() const
			{
			return title;
			}
		virtual NxsBlock			*CloneBlock(NxsBlockMapper &memo) const;
		bool				ImplementsLinkAPI() const;
		void				SetImplementsLinkAPI(bool v);

		virtual void				WriteAsNexus(std::ostream &out) const;
		virtual void 				WriteBlockIDCommand(std::ostream &out) const;
		virtual void 				WriteLinkCommand(std::ostream &out) const;
		virtual void 				WriteTitleCommand(std::ostream &out) const;
		std::string GetTitle() const
			{
			return title;
			}
		void SetTitle(const std::string &t, bool autogeneratedTitle)
			{
			title = t;
			/* i19 */ /*v2.1to2.2 19 */
			autoTitle = autogeneratedTitle;
			}
		bool IsAutoGeneratedTitle() const
			{
			return autoTitle;
			}
		void StoreSkippedCommands(bool v)
			{
			storeSkippedCommands = v;
			}
		void ClearStoredSkippedCommands()
			{
			skippedCommands.clear();
			}

		/*----------------------------------------------------------------------------------------
		| Copies all NxsBlock fields - execept  the `nexusReader` and `next` pointers.
		|	Aliasing of Block pointers results in very dangerous implication of copying for
		|	many subclasses of NxsBlock.
		| Copying of blocks should be restricted to empty blocks without linkages (e.g.
		|	the CloneFactory mechanism requires some form of copy, but should typically be used with
		|	empty blocks.
		*/

		virtual void CopyBaseBlockContents(const NxsBlock &other)
			{
			errormsg = other.errormsg;
			isEmpty = other.isEmpty;
			isEnabled = other.isEnabled;
			isUserSupplied = other.isUserSupplied;
			NCL_BLOCKTYPE_ATTR_NAME = other.NCL_BLOCKTYPE_ATTR_NAME;
			title = other.title;
			/* i19 */ /*v2.1to2.2 19 */
			blockIDString = other.blockIDString;
			linkAPI = other.linkAPI;
			storeSkippedCommands = other.storeSkippedCommands;
			skippedCommands = other.skippedCommands;
			autoTitle = other.autoTitle;
			}

		virtual NxsBlock * Clone() const
			{
			NxsBlock * b = new NxsBlock();
			b->CopyBaseBlockContents(*this);
			b->nexusReader = NULL;
			b->next = NULL;
			return b;
			}

		unsigned			ReadVectorPartitionDef(NxsPartition &np, NxsLabelToIndicesMapper &ltm, const std::string & partName, const char * ptype, const char * cmd, NxsToken & token, bool warnAsterisked, bool demandAllInds, NxsSetVectorItemValidator & v);
		void 				ReadPartitionDef(NxsPartition &np, NxsLabelToIndicesMapper &ltm, const std::string & partName, const char * ptype, const char * cmd, NxsToken & token, bool warnAsterisked, bool demandAllInds, bool storeAsPartition);
		virtual bool		TolerateEOFInBlock() const
			{
			return false;
			}
		void 				WarnDangerousContent(const std::string &s, const NxsToken &t);
		void 				WarnDangerousContent(const std::string &s, const ProcessedNxsToken &t);

		void				WriteBasicBlockCommands(std::ostream & out) const;
		virtual void		WriteSkippedCommands(std::ostream & out) const;
		// used internally to deal with multiple blocks spawning the same TAXA block
		virtual bool 		SwapEquivalentTaxaBlock(NxsTaxaBlockAPI * )
		{
			return false;
		}
		/*! This is the argument from the BLOCKID command.  It should be unique, but
			that is dependent on the file being valid (NCL does not verify uniqueness).

			This is not the ID used to identify block type. \ref BlockTypeIDDiscussion
		*/
		std::string GetBlockIDCommandString() const { return blockIDString; }
	protected:
		void				SkipCommand(NxsToken & token);

		NxsCommandResult	HandleBasicBlockCommands(NxsToken & token);
		void				DemandEndSemicolon(NxsToken &token, const char *contextString) const;
		void				DemandEquals(NxsToken &token, const char *contextString) const;
		void				DemandEquals(ProcessedNxsCommand::const_iterator &tokIt, const ProcessedNxsCommand::const_iterator & endIt, const char *contextString) const ;
		void				DemandIsAtEquals(NxsToken &token, const char *contextString) const;
		unsigned 			DemandPositiveInt(NxsToken &token, const char *contextString) const;
		void				GenerateNxsException(NxsToken &token, const char *message = NULL) const;
		void				GenerateUnexpectedTokenNxsException(NxsToken &token, const char *expected = NULL) const;
		bool				isEmpty;			/* true if this object is currently storing data */
		bool				isEnabled;			/* true if this block is currently ebabled */
		bool				isUserSupplied;		/* true if this object has been read from a file; false otherwise */
		NxsReader			*nexusReader;		/* pointer to the Nexus file reader object */
		NxsBlock			*next;				/* DEPRECATED field pointer to next block in list */
		NxsString			NCL_BLOCKTYPE_ATTR_NAME;					/* holds name of block (e.g., "DATA", "TREES", etc.) \ref BlockTypeIDDiscussion */
		std::string			title;				/* holds the title of the block empty by default */
		std::string 		blockIDString; 		/* Mesquite generates these. I don't know what they are for */
		bool				linkAPI;
		bool				autoTitle;			/* true if the title was generated internally*/
		bool				storeSkippedCommands;
		std::list<ProcessedNxsCommand> skippedCommands; /* commands accumulate by SkipCommand or by some other means */

		virtual void		Read(NxsToken &token);
		/* i22 */ /*v2.1to2.2 22 */
		private:
			NxsBlock &operator=(const NxsBlock &other); /*intentionally not defined because of aliasing issues */

	};

/*! This abstract class defines the interface for a factory that can generate NxsBlocks.

	When the NxsReader::Execute() method encounters a block that it needs to handle, it will first check the registered "singelton"
		blocks (a block that has been added to it using NxsReader::Add(), those NxsBlock instances are recycled).
		If no singleton block says that it can read that block of NEXUS (see NxsBlock::CanReadBlockType()), then the NxsReader
		will walk through its list of factories calling NxsBlockFactory::GetBlockReaderForID() for each until it gets a
		non-NULL pointer.

	If there is an exception during the parsing of that block BlockError will be called for the factory instance that generated the block

	If the block returns "false" from NxsBlock::IsEnabled() method, then BlockSkipped will be called by the NxsReader using
		 the factory instance that generated the block

	Blocks generated by factories but used successfully in a parse have to be deleted by the client code (See \ref memoryManagement discussion).

*/
class NxsBlockFactory
	{
	public:
		virtual ~NxsBlockFactory()
			{
			}
		/*! \returns a NxsBlock instance with NxsBlock::Read method that is capable of parsing a NEXUS block of type `NCL_BLOCKTYPE_ATTR_NAME`
		*/
		virtual NxsBlock  *	GetBlockReaderForID(const std::string & NCL_BLOCKTYPE_ATTR_NAME, /*!< The type of block that needs to be read see \ref BlockTypeIDDiscussion */
												NxsReader *reader,  /*!< a pointer to the NxsReader controlling the parse. Can be NULL. Usually not needed for an implementation of this method */
												NxsToken *token  /*!< a pointer to the NxsToken that is generating token. Can be NULL. Usually not needed for an implementation of this method */
												) = 0;

		/*! base-class implementation deletes the block (the NxsReader will not retain a reference to the block, so failing to delete can
			lead to memory leaks if you do not have some fancy memory management scheme).

			If there is an exception during the parsing of that block BlockError will be called for the factory instance that generated the block
		*/
		virtual void		BlockError(NxsBlock *b)
			{
			delete b;
			}
		/*! base-class implementation deletes the block (the NxsReader will not retain a reference to the block, so failing to delete can
			lead to memory leaks if you do not have some fancy memory management scheme).

		If the block returns "false" from NxsBlock::IsEnabled() method, then BlockSkipped will be called by the NxsReader using
			 the factory instance that generated the block
		*/
		virtual void BlockSkipped(NxsBlock *b)
			{
			delete b;
			}
	};

/*!
	Sets the nexusReader data member of the NxsBlock object to 'nxsptr'.
*/
inline void NxsBlock::SetNexus(
  NxsReader *nxsptr)	/* pointer to a NxsReader object */
	{
	nexusReader = nxsptr;
	}
/*!
	Gets the nexusReader data member of the NxsBlock object to 'nxsptr'.
*/
inline NxsReader * NxsBlock::GetNexus() const
	{
	return nexusReader;
	}


/*!
	Advances the token, and raise an exception if it is not an equals sign.

 	Sets errormsg and raises a NxsException on failure.
	`contextString` is used in error messages:
		"Expecting '=' ${contextString} but found..."
*/
inline void NxsBlock::DemandEquals(NxsToken &token, const char *contextString) const
	{
	token.GetNextToken();
	DemandIsAtEquals(token, contextString);
	}
#endif


