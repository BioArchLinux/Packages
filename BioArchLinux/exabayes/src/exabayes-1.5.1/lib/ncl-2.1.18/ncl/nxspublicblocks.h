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
#ifndef NCL_NXSPUBLICBLOCKS_H
#define NCL_NXSPUBLICBLOCKS_H

#include <vector>
#include "ncl/nxsdefs.h"
#include "ncl/nxsblock.h"
#include "ncl/nxsassumptionsblock.h"
#include "ncl/nxscharactersblock.h"
#include "ncl/nxsdatablock.h"
#include "ncl/nxsdistancesblock.h"
#include "ncl/nxstaxablock.h"
#include "ncl/nxstaxaassociationblock.h"
#include "ncl/nxstreesblock.h"
#include "ncl/nxsunalignedblock.h"
#include "ncl/nxsreader.h"

class NxsStoreTokensBlockReader
	: public NxsBlock
	{
	public:
		/*---------------------------------------------------------------------------------------
		| If the blockname is empty then, any block will be read by the instance
		*/
		NxsStoreTokensBlockReader(std::string blockName, bool storeTokenInfo)
			:storeAllTokenInfo(storeTokenInfo),
			tolerateEOFInBlock(false)
			{
			NCL_BLOCKTYPE_ATTR_NAME = NxsString(blockName.c_str());
			}
		void Read(NxsToken &token);
		void Reset();
		void Report(std::ostream &out) NCL_COULD_BE_CONST  /*v2.1to2.2 1 */
			{
			ReportConst(out);
			}
		void WriteAsNexus(std::ostream &out) const;
		/*---------------------------------------------------------------------------------------
		| Results in aliasing of the taxa, assumptionsBlock blocks!
		*/
		NxsStoreTokensBlockReader & operator=(const NxsStoreTokensBlockReader &other)
			{
			Reset();
			CopyBaseBlockContents(static_cast<const NxsBlock &>(other));
			commandsRead = other.commandsRead;
			justTokens = other.justTokens;
			storeAllTokenInfo = other.storeAllTokenInfo;
			tolerateEOFInBlock = other.tolerateEOFInBlock;
			return *this;
			}

		NxsStoreTokensBlockReader * Clone() const
			{
			NxsStoreTokensBlockReader * b = new NxsStoreTokensBlockReader(NCL_BLOCKTYPE_ATTR_NAME, storeAllTokenInfo);
			*b = *this;
			return b;
			}
		/*! \ref BlockTypeIDDiscussion */
		virtual bool CanReadBlockType(const NxsToken & token)
			{
			if (NCL_BLOCKTYPE_ATTR_NAME.length() == 0)
				{
				NCL_BLOCKTYPE_ATTR_NAME.assign(token.GetTokenReference().c_str());
				NCL_BLOCKTYPE_ATTR_NAME.ToUpper();
				return true;
				}
			return token.Equals(NCL_BLOCKTYPE_ATTR_NAME);
			}
		virtual bool TolerateEOFInBlock() const
			{
			return tolerateEOFInBlock; /*  */
			}
		void SetTolerateEOFInBlock(bool v)
			{
			tolerateEOFInBlock = v;
			}
		const std::list<ProcessedNxsCommand> & GetCommands() const
			{
			return commandsRead;
			}
	protected:
		void ReadCommand(NxsToken &token);
		void ReportConst(std::ostream &out) const;

		typedef std::vector<std::string> VecString;
		typedef std::list<VecString> ListVecString;


		std::list<ProcessedNxsCommand> commandsRead;
		ListVecString justTokens;
		bool storeAllTokenInfo;
		bool tolerateEOFInBlock;
	};
/*!
 	A factory class that delegates calls to the other "default" public block parsers that NCL provides.

	Provided as a convenience class to make it possible to read all supported blocks with the addition of one factory
		to the NxsReader.


*/
class NxsDefaultPublicBlockFactory
	: public NxsBlockFactory
	{
	public:
		/**----------------------------------------------------------------------------------------------------------------------
		|	Constructor takes two booleans.
		|	If readUnknownBlocks is "true" then a NxsStoreTokensBlockReader will be spawned for every unknown block.
		|	storeTokenInfo is passed to the NxsStoreTokensBlockReader constructor (true for storage of full token info - such as
		|		file position.
		*/
		NxsDefaultPublicBlockFactory(bool readUnknownBlocks, bool storeTokenInfo)
			:tokenizeUnknownBlocks(readUnknownBlocks),
			storeTokenInfoArg(storeTokenInfo)
			{}
		virtual NxsBlock  *	GetBlockReaderForID(const std::string & NCL_BLOCKTYPE_ATTR_NAME, NxsReader *reader, NxsToken *token);

	protected:
		NxsAssumptionsBlockFactory assumpBlockFact;
		NxsCharactersBlockFactory charBlockFact;
		NxsDataBlockFactory dataBlockFact;
		NxsDistancesBlockFactory distancesBlockFact;
		NxsTaxaBlockFactory taxaBlockFact;
		NxsTaxaAssociationBlockFactory taxaAssociationBlockFact;
		NxsTreesBlockFactory treesBlockFact;
		NxsUnalignedBlockFactory unalignedBlockFact;

		bool tokenizeUnknownBlocks;
		bool storeTokenInfoArg;
	};


/*!
 	A factory class that takes examplar that will be cloned to read each block.

	To use this factory you MUST overload NxsBlock::Clone() for class that you would like to use to parse blocks
*/
class NxsCloneBlockFactory
	: public NxsBlockFactory
	{
	public:
		NxsCloneBlockFactory()
			:defPrototype(NULL)
			{}
		/*! \returns a new NxsBlock instance (or NULL) to read the NEXUS content
		in a block of name `NCL_BLOCKTYPE_ATTR_NAME`.

			This function is called by the NxsReader during the parse if no
			NxsBlock instances for this block ID type were added to the reader.
		*/
		virtual NxsBlock  *	GetBlockReaderForID(const std::string & NCL_BLOCKTYPE_ATTR_NAME, /*!< The block ID \ref BlockTypeIDDiscussion */
								NxsReader *, /*!< pointer to the NxsReader that is conducting the parse */
								NxsToken *) /*!< pointer to the current NxsToken object that wraps the istream (this function should not advance the token) */
			{
			std::string b(NCL_BLOCKTYPE_ATTR_NAME.c_str());
			NxsString::to_upper(b);
			std::map<std::string , const NxsBlock *>::const_iterator pIt = prototypes.find(b);
			if (pIt == prototypes.end())
				return (defPrototype ? defPrototype->Clone() : NULL);
			return pIt->second->Clone();
			}

		/*! Registers a block instance to be used whenever an unknown block (any
		block with an ID that does not correspond with any of the registered blocks)
		is encountered in a file.
		*/
		bool AddDefaultPrototype(const NxsBlock * exemplar)
			{
			bool replaced = defPrototype != NULL;
			defPrototype = exemplar;
			return replaced;
			}
		/*! Registers the block instance passed in as a template to clone a block reader
			whenever a block with the name `blockName` is encountered.
		*/
		bool AddPrototype(const NxsBlock * exemplar, /*!< The block to be cloned */
						  const char * blockName = NULL) /*!< The block ID \ref BlockTypeIDDiscussion */
			{
			std::string b;
			if (blockName)
				b.assign(blockName);
			else
				{
				if (exemplar == NULL)
					return false;
				NxsString bId  = exemplar->GetID();
				b.assign(bId.c_str());
				}
			NxsString::to_upper(b);
			bool replaced = prototypes.find(b) != prototypes.end();
			prototypes[b] = exemplar;
			return replaced;
			}

	protected:
		std::map<std::string , const NxsBlock *> prototypes;
		const NxsBlock * defPrototype;
	};



typedef std::pair<std::string, std::string> NxsNameToNameTrans;

/*! hacky (home-spun)  writing of XML attributes */
void writeAttributeValue(std::ostream & out, const std::string & v);

/*! This class is used internally to keep track of operations that may be needed
to make taxon labels from different sources avoid clashing with each other.
*/
class NxsConversionOutputRecord
	{
	public:

		NxsConversionOutputRecord()
			:addNumbersToDisambiguateNames(false),
			writeNameTranslationFile(true),
			translationFilename("NameTranslationFile"),
			numberTranslationFiles(true),
			verboseWritingOfNameTranslationFile(true)
			{}

		void writeNameTranslation(std::vector<NxsNameToNameTrans>, const NxsTaxaBlockAPI * );

		static std::string getUniqueFilenameWithLowestIndex(const char * prefix);
		static void writeTaxonNameTranslationFilepath(const char * fn, const std::vector<NxsNameToNameTrans> & nameTrans, const NxsTaxaBlockAPI *, bool verbose=false);
		static void writeTaxonNameTranslationStream(std::ostream & fn, const std::vector<NxsNameToNameTrans> & nameTrans, const NxsTaxaBlockAPI *);

		//The following set of members were added to deal with name clashes that
		//	are legal (but a very bad idea) in phylip.  If names are auto-translated
		//	to unique names (by the addition of numbers, then it is very useful to print out a file
		//	listing the translations.
		bool addNumbersToDisambiguateNames; // if true, then taxon names may be altered on reading to  make them unique
		bool writeNameTranslationFile; // if true, and taxon names are modified, then a translation file will be written
		std::string translationFilename; // if writeNameTranslationFile is used, then this will be the file name or prefix
		bool numberTranslationFiles; // if true, then translationFilename will serve as a prefix and the real filename may contain a number to make it unique.
		bool verboseWritingOfNameTranslationFile; // if true, then writing a translationFilename will trigger a message to std::cerr
		std::map<const NxsTaxaBlockAPI *, std::string> taxaBlocksToConversionFiles;
	};


/*!
A NxsReader that uses clone factories to read public blocks.

The blocks created by reading a file MUST BE DELETED by the caller (either by a
	call to DeleteBlocksFromFactories() or by requesting each pointer to a block
	and then deleting the blocks).

Blocks are created by cloning a template block. If you would like to alter
	the default behavior of a block, you can request a reference to the
	"template" NxsBlock of the appropriate type, modify it, and then parse the file.

You may give the reader "context" programatically by adding "Read" blocks (which
	will mimic the behavior of those blocks having appeared in the file itself.

Commands in Non-public blocks are dealt with by creating a NxsStoreTokensBlockReader
	to store the commands.

After parsing, the client can request the number of TAXA blocks read, and the
	number of CHARACTERS, TREES, ... blocks that refer to a particular taxa
	block.



NOT COPYABLE
*/
class PublicNexusReader: public ExceptionRaisingNxsReader
	{
	public:

		static BlockReaderList parseFileOrThrow(const char *filepath,
												NxsReader::WarningHandlingMode mode = NxsReader::WARNINGS_TO_STDERR,
												bool parsePrivateBlocks=true,
												bool storeTokenInfo=true);

		/*! Enumeration of bits used that can be "ORed" together to create an argument for
			PublicNexusReader instance that will only read certain NEXUS blocks
		*/
		enum NexusBlocksToRead
		{
			NEXUS_TAXA_BLOCK_BIT = 0x01, /// Flags TAXA blocks as a type to be read
			NEXUS_TREES_BLOCK_BIT = 0x02, /// Flags TREES blocks as a type to be read
			NEXUS_CHARACTERS_BLOCK_BIT = 0x04, /// Flags CHARACTERS and DATA blocks as types to be read
			NEXUS_ASSUMPTIONS_BLOCK_BIT = 0x08, /// Flags ASSUMPTIONS blocks as a type to be read
			NEXUS_SETS_BLOCK_BIT = 0x10, /// Flags SETS blocks as a type to be read
			NEXUS_UNALIGNED_BLOCK_BIT = 0x20, /// Flags UNALIGNED blocks as a type to be read
			NEXUS_DISTANCES_BLOCK_BIT = 0x40, /// Flags DISTANCES blocks as a type to be read
			NEXUS_TAXAASSOCIATION_BLOCK_BIT = 0x80, /// Flags TAXAASSOCIATION blocks to be read
			NEXUS_UNKNOWN_BLOCK_BIT = 0x100 /// to be used internally
		};

		/*!	Creates a new PublicNexusReader
			\arg blocksToRead -1 indicates that every block type should be read.
				alternatively, the caller can OR-together bits of the NexusBlocksToRead enum
				to indicate which blocks should be processed.
			\arg mode should be a facet of the NxsReader::WarningHandlingMode enum
				that indicates where warning messages should be directed.
		*/
		PublicNexusReader(const int blocksToRead = -1, NxsReader::WarningHandlingMode mode=NxsReader::WARNINGS_TO_STDERR);
		virtual ~PublicNexusReader();

		virtual void	Execute(NxsToken& token, bool notifyStartStop = true);
		std::string GetErrorMessage()
			{
			return errorMsg;
			}

		/*! \arg a vector of taxon names.
		 	\returns a new taxa block for the these taxa. This taxa block will also
		 	be stored in the reader so that future files can refer to these taxa.

		 	This function is useful if you want to programmatically create
		 	a NEXUS TAXA block and have the reader treat it in the same way as
		 	if the NxsTaxaBlock were read from a file
		*/
		NxsTaxaBlock * RegisterTaxa(const std::vector<std::string> & tl);

		/*!	\returns a pointer to the template for the NxsAssumptionsBlock.
			This object will be cloned whenever an ASSUMPTIONS or SETS block is encountered,
			so modifying the default behavior of this instance will change the behavior
			of every NxsAssumptionsBlock created by the reader.

			Do NOT DELETE the template! (the client has to delete all spawned blocks, but
			the PublicNexusReader deletes its own templates)
		*/
		NxsAssumptionsBlock * GetAssumptionsBlockTemplate() {return assumptionsBlockTemplate;}
		/*!	\returns a pointer to the template for the NxsCharactersBlock.
			This object will be cloned whenever a CHARACTERS block is encountered,
			so modifying the default behavior of this instance will change the behavior
			of every NxsCharactersBlock created by the reader.

			Do NOT DELETE the template! (the client has to delete all spawned blocks, but
			the PublicNexusReader deletes its own templates)
		*/
		NxsCharactersBlock * GetCharactersBlockTemplate() {return charactersBlockTemplate;}
		/*!	\returns a pointer to the template for the NxsDataBlock.
			This object will be cloned whenever a DATA block is encountered,
			so modifying the default behavior of this instance will change the behavior
			of every NxsDataBlock created by the reader.

			Do NOT DELETE the template! (the client has to delete all spawned blocks, but
			the PublicNexusReader deletes its own templates)
		*/
		NxsDataBlock * GetDataBlockTemplate() {return dataBlockTemplate;}
		/*!	\returns a pointer to the template for the NxsDistancesBlock.
			This object will be cloned whenever a DISTANCES block is encountered,
			so modifying the default behavior of this instance will change the behavior
			of every NxsDistancesBlock created by the reader.

			Do NOT DELETE the template! (the client has to delete all spawned blocks, but
			the PublicNexusReader deletes its own templates)
		*/
		NxsDistancesBlock * GetDistancesBlockTemplate() {return distancesBlockTemplate;}
		/*!	\returns a pointer to the template for the NxsTaxaBlock.
			This object will be cloned whenever a TAXA block is encountered,
			so modifying the default behavior of this instance will change the behavior
			of every NxsTaxaBlock created by the reader.

			Do NOT DELETE the template! (the client has to delete all spawned blocks, but
			the PublicNexusReader deletes its own templates)
		*/
		NxsTaxaBlock * GetTaxaBlockTemplate() {return taxaBlockTemplate;}
		/*!	\returns a pointer to the template for the NxsTaxaAssociationBlock.
			This object will be cloned whenever a TAXAASSOCIATION block is encountered,
			so modifying the default behavior of this instance will change the behavior
			of every NxsTaxaBlock created by the reader.

			Do NOT DELETE the template! (the client has to delete all spawned blocks, but
			the PublicNexusReader deletes its own templates)
		*/
		NxsTaxaAssociationBlock * GetTaxaAssociationBlockTemplate() {return taxaAssociationBlockTemplate;}
		/*!	\returns a pointer to the template for the NxsTreesBlock.
			This object will be cloned whenever a TREES block is encountered,
			so modifying the default behavior of this instance will change the behavior
			of every NxsTreesBlock created by the reader.

			Do NOT DELETE the template! (the client has to delete all spawned blocks, but
			the PublicNexusReader deletes its own templates)
		*/
		NxsTreesBlock * GetTreesBlockTemplate() {return treesBlockTemplate;}
		/*!	\returns a pointer to the template for the GetUnalignedBlockTemplate.
			This object will be cloned whenever an UNALIGNED block is encountered,
			so modifying the default behavior of this instance will change the behavior
			of every GetUnalignedBlockTemplate created by the reader.

			Do NOT DELETE the template! (the client has to delete all spawned blocks, but
			the PublicNexusReader deletes its own templates)
		*/
		NxsUnalignedBlock * GetUnalignedBlockTemplate() {return unalignedBlockTemplate;}
		/*!	\returns a pointer to the template for the NxsStoreTokensBlockReader.
			This object will be cloned whenever an unknown block is encountered,
			so modifying the default behavior of this instance will change the behavior
			of every NxsStoreTokensBlockReader created by the reader.

			Do NOT DELETE the template! (the client has to delete all spawned blocks, but
			the PublicNexusReader deletes its own templates)
		*/
		NxsStoreTokensBlockReader * GetUnknownBlockTemplate() const {return storerBlockTemplate;}

		/*! \returns the number of NxsAssumptionsBlock objects created during the parse which
				refer to the taxa block `taxa`
			If `taxa` is 0L, then the total number of Assumptions blocks will be returned
		*/
		unsigned GetNumAssumptionsBlocks(const NxsTaxaBlock *taxa) const;
		/*! \returns the number of NxsAssumptionsBlock objects created during the parse which
				refer to the characters block `chars`
			If `chars` is 0L, then the total number of Assumptions blocks will be returned
		*/
		unsigned GetNumAssumptionsBlocks(const NxsCharactersBlock *chars) const;
		/*! \returns the number of NxsAssumptionsBlock objects created during the parse which
				refer to the trees block `trees`
			If `trees` is 0L, then the total number of Assumptions blocks will be returned
		*/
		unsigned GetNumAssumptionsBlocks(const NxsTreesBlock *trees) const;
		/*! \returns a pointer to the NxsCharactersBlock with index
			Indexing starts at 0 and refers to the index in a list of NxsCharactersBlock
			objects that refer to the NxsTaxaBlock `taxa`.  Thus, the index does not
			necessarily represent the position among ALL of the NxsDistancesBlock objects

			0L will be returned if the index is out of range. Indices should be <
				the number returned by GetNumCharactersBlocks(taxa).
			If `taxa` is 0L, then the total block indexing scheme will refer to the
				total number of Assumptions blocks read.
		*/
		NxsAssumptionsBlock * GetAssumptionsBlock(const NxsTaxaBlock *taxa, unsigned index) const;
		NxsAssumptionsBlock * GetAssumptionsBlock(const NxsCharactersBlock *taxa, unsigned index) const;
		NxsAssumptionsBlock * GetAssumptionsBlock(const NxsTreesBlock *taxa, unsigned index) const;

		/*! \returns the number of NxsStoreTokensBlockReader objects created during the parse.
		*/
		unsigned GetNumUnknownBlocks() const;
		/*! \returns a pointer to the NxsTaxaBlock with index (indexing starts at 0).
				0L will be returned if the index is out of range. index should be < the
				number returned by GetNumUnknownBlocks();
		*/
		NxsStoreTokensBlockReader * GetUnknownBlock(unsigned index) const;

		/*! \returns the number of NxsCharactersBlock objects created during the parse which
				refer to the taxa in `taxa`
			If `taxa` is 0L, then the total number of Characters blocks will be returned
		*/
		unsigned GetNumCharactersBlocks(const NxsTaxaBlock *taxa) const;
		/*! \returns a pointer to the NxsCharactersBlock with index
			Indexing starts at 0 and refers to the index in a list of NxsCharactersBlock
			objects that refer to the NxsTaxaBlock `taxa`.  Thus, the index does not
			necessarily represent the position among ALL of the NxsDistancesBlock objects

			0L will be returned if the index is out of range. Indices should be <
				the number returned by GetNumCharactersBlocks(taxa).
			If `taxa` is 0L, then the total block indexing scheme will refer to the
				total number of Characters blocks read.
		*/
		NxsCharactersBlock * GetCharactersBlock(const NxsTaxaBlock *taxa, unsigned index) const;

		/*! \returns the number of NxsDistancesBlock objects created during the parse which
				refer to the taxa in `taxa`
			If `taxa` is 0L, then the total number of Distances blocks will be returned
		*/
		unsigned GetNumDistancesBlocks(const NxsTaxaBlock *taxa) const;
		/*! \returns a pointer to the NxsDistancesBlock with index
			Indexing starts at 0 and refers to the index in a list of NxsDistancesBlock
			objects that refer to the NxsTaxaBlock `taxa`.  Thus, the index does not
			necessarily represent the position among ALL of the NxsDistancesBlock objects

			0L will be returned if the index is out of range. Indices should be <
				the number returned by GetNumDistancesBlocks(taxa).
			If `taxa` is 0L, then the total block indexing scheme will refer to the
				total number of Distances blocks read.
		*/
		NxsDistancesBlock * GetDistancesBlock(const NxsTaxaBlock *taxa, unsigned index) const;

		/*! \returns the number of NxsTaxaBlock objects created during the parse (some
		 	of these objects may be "implied" by another block, but the client
		 	code can treat them as if they occurred in the file explicitly).
		*/
		unsigned GetNumTaxaBlocks() const;
		/*! \returns a pointer to the NxsTaxaBlock with index (indexing starts at 0).
				0L will be returned if the index is out of range.
		*/
		NxsTaxaBlock * GetTaxaBlock(unsigned index) const;

		/*! \returns the number of NxsTaxaAssociationBlock objects created during the parse which
				refer to the taxa as their first of second taxa block
			If `taxa` is 0L, then the total number of Trees blocks will be returned
		*/
		unsigned GetNumTaxaAssociationBlocks(const NxsTaxaBlock *taxa) const;
		/*! \returns a pointer to the NxsTaxaAssociationBlock with index
			Indexing starts at 0 and refers to the index in a list of NxsTaxaAssociationBlock
			objects that refer to the NxsTaxaBlock `taxa`.  Thus, the index does not
			necessarily represent the position among ALL of the NxsTaxaAssociationBlock objects

			0L will be returned if the index is out of range. Indices should be <
				the number returned by GetNumTreesBlocks(taxa).
			If `taxa` is 0L, then the total block indexing scheme will refer to the
				total number of Trees blocks read.
		*/
		NxsTaxaAssociationBlock * GetTaxaAssociationBlock(const NxsTaxaBlock *taxa, unsigned index) const;

		/*! \returns the number of NxsTreesBlock objects created during the parse which
				refer to the taxa in `taxa`
			If `taxa` is 0L, then the total number of Trees blocks will be returned
		*/
		unsigned GetNumTreesBlocks(const NxsTaxaBlock *taxa) const;
		/*! \returns a pointer to the NxsTreesBlock with index
			Indexing starts at 0 and refers to the index in a list of NxsTreesBlock
			objects that refer to the NxsTaxaBlock `taxa`.  Thus, the index does not
			necessarily represent the position among ALL of the NxsTreesBlock objects

			0L will be returned if the index is out of range. Indices should be <
				the number returned by GetNumTreesBlocks(taxa).
			If `taxa` is 0L, then the total block indexing scheme will refer to the
				total number of Trees blocks read.
		*/
		NxsTreesBlock * GetTreesBlock(const NxsTaxaBlock *taxa, unsigned index) const;

		/*! \returns the number of NxsUnalignedBlock objects created during the parse which
				refer to the taxa in `taxa`
			If `taxa` is 0L, then the total number of Unaligned blocks will be returned
		*/
		unsigned GetNumUnalignedBlocks(const NxsTaxaBlock *taxa) const;
		/*! \returns a pointer to the NxsUnalignedBlock with index
			Indexing starts at 0 and refers to the index in a list of NxsUnalignedBlock
			objects that refer to the NxsTaxaBlock `taxa`.  Thus, the index does not
			necessarily represent the position among ALL of the NxsUnalignedBlock objects

			0L will be returned if the index is out of range. Indices should be <
				the number returned by GetNumUnalignedBlocks(taxa).
			If `taxa` is 0L, then the total block indexing scheme will refer to the
				total number of Unaligned blocks read.
		*/
		NxsUnalignedBlock * GetUnalignedBlock(const NxsTaxaBlock *taxa, unsigned index) const;


		/*! Deletes all of the blocks that were spawned during the parse.
			\warning Do not call this function if you still retain references
			  to the spawned objects.
		*/
		virtual void DeleteBlocksFromFactories()
			{
			NxsReader::DeleteBlocksFromFactories();
			ClearUsedBlockList();
			}
		/*! Mainly used internally.
			\ref ClearContent() which is probably the function that you want
			Removes the record of the order of blocks encountered during the
				last parse.

		*/
		virtual void ClearUsedBlockList();
		/*! Removes all references to blocks spawned during the parse, but does
			NOT delete them.
			Call this function if you want to reuse the parser and you want to

			delete all of the spawned blocks yourself (after calling
			ClearUsedBlockList() the DeleteBlocksFromFactories() function
			will not delete blocks)
		*/
		virtual void ClearContent()
			{
			assumptionsBlockVec.clear();
			charactersBlockVec.clear();
			dataBlockVec.clear();
			distancesBlockVec.clear();
			storerBlockVec.clear();
			taxaBlockVec.clear();
			taxaAssociationBlockVec.clear();
			treesBlockVec.clear();
			unalignedBlockVec.clear();
			ExceptionRaisingNxsReader::ClearContent();
			}

		/*! Adds (or "registers") a NxsAssumptionsBlock with the reader. Can be
			useful if you:
				-# obtain references to blocks you want to keep,
				-# call ClearContent()
				-# add back the block instances that provide necessary context for additional parses.
		*/
		void AddReadAssumptionsBlock(NxsAssumptionsBlock * block)
			{
			assumptionsBlockVec.push_back(block);
			AddReadBlock("ASSUMPTIONS", block);
			}
		/*! Adds (or "registers") a NxsCharactersBlock with the reader. Can be
			useful if you:
				-# obtain references to blocks you want to keep,
				-# call ClearContent()
				-# add back the block instances that provide necessary context for additional parses.
		*/
		void AddReadCharactersBlock(NxsCharactersBlock * block)
			{
			charactersBlockVec.push_back(block);
			AddReadBlock("CHARACTERS", block);
			}
		/*! Adds (or "registers") a NxsDataBlock with the reader. Can be
			useful if you:
				-# obtain references to blocks you want to keep,
				-# call ClearContent()
				-# add back the block instances that provide necessary context for additional parses.
		*/
		void AddReadDataBlock(NxsDataBlock * block)
			{
			dataBlockVec.push_back(block);
			AddReadBlock("CHARACTERS", block);
			}
		/*! Adds (or "registers") a NxsDistancesBlock with the reader. Can be
			useful if you:
				-# obtain references to blocks you want to keep,
				-# call ClearContent()
				-# add back the block instances that provide necessary context for additional parses.
		*/
		void AddReadDistancesBlock(NxsDistancesBlock * block)
			{
			distancesBlockVec.push_back(block);
			AddReadBlock("DISTANCES", block);
			}
		/*! Adds (or "registers") a NxsTaxaBlock with the reader. Can be
			useful if you:
				-# obtain references to blocks you want to keep,
				-# call ClearContent()
				-# add back the block instances that provide necessary context for additional parses.
		*/
		void AddReadTaxaBlock(NxsTaxaBlock * block)
			{
			taxaBlockVec.push_back(block);
			AddReadBlock("TAXA", block);
			}
		/*! Adds (or "registers") a NxsTaxaAssociationBlock with the reader. Can be
			useful if you:
				-# obtain references to blocks you want to keep,
				-# call ClearContent()
				-# add back the block instances that provide necessary context for additional parses.
		*/
		void AddReadTaxaAssociationBlock(NxsTaxaAssociationBlock * block)
			{
			taxaAssociationBlockVec.push_back(block);
			AddReadBlock("TAXAASSOCIATION", block);
			}
		/*! Adds (or "registers") a NxsTreesBlock with the reader. Can be
			useful if you:
				-# obtain references to blocks you want to keep,
				-# call ClearContent()
				-# add back the block instances that provide necessary context for additional parses.
		*/
		void AddReadTreesBlock(NxsTreesBlock * block)
			{
			treesBlockVec.push_back(block);
			AddReadBlock("TREES", block);
			}
		/*! Adds (or "registers") a NxsUnalignedBlock with the reader. Can be
			useful if you:
				-# obtain references to blocks you want to keep,
				-# call ClearContent()
				-# add back the block instances that provide necessary context for additional parses.
		*/
		void AddReadUnalignedBlock(NxsUnalignedBlock * block)
			{
			unalignedBlockVec.push_back(block);
			AddReadBlock("UNKNOWN", block);
			}
		/*! Adds (or "registers") a NxsStoreTokensBlockReader with the reader. Can be
			useful if you:
				-# obtain references to blocks you want to keep,
				-# call ClearContent()
				-# add back the block instances that provide necessary context for additional parses.
		*/
		void AddReadUnknownBlock(NxsStoreTokensBlockReader * block)
			{
			storerBlockVec.push_back(block);
			AddReadBlock(block->GetID(), block);
			}

		/*! this public field is used in some hacks that relate to printing out
			translation records during parsing (when names have to change in order
			for the input file to be valid NEXUS, but we want the parser to be
			loose but to log its changes).
		*/
		NxsConversionOutputRecord conversionOutputRecord;

	protected:
		void PostExecuteHook();
		virtual void    AddFactory(NxsBlockFactory *);
		int bitsForBlocksToRead;
		NxsCloneBlockFactory cloneFactory;

		NxsAssumptionsBlock * assumptionsBlockTemplate;
		NxsCharactersBlock * charactersBlockTemplate;
		NxsDataBlock * dataBlockTemplate;
		NxsDistancesBlock * distancesBlockTemplate;
		NxsStoreTokensBlockReader * storerBlockTemplate;
		NxsTaxaBlock * taxaBlockTemplate;
		NxsTaxaAssociationBlock * taxaAssociationBlockTemplate;
		NxsTreesBlock * treesBlockTemplate;
		NxsUnalignedBlock * unalignedBlockTemplate;

		std::vector<NxsAssumptionsBlock *> assumptionsBlockVec;
		std::vector<NxsCharactersBlock *> charactersBlockVec;
		std::vector<NxsDataBlock *> dataBlockVec;
		std::vector<NxsDistancesBlock *> distancesBlockVec;
		std::vector<NxsStoreTokensBlockReader *> storerBlockVec;
		std::vector<NxsTaxaBlock *> taxaBlockVec;
		std::vector<NxsTaxaAssociationBlock *> taxaAssociationBlockVec;
		std::vector<NxsTreesBlock *> treesBlockVec;
		std::vector<NxsUnalignedBlock *> unalignedBlockVec;

		std::string errorMsg;
	private:
		PublicNexusReader(const PublicNexusReader &); // do not define. Not copyable
		PublicNexusReader & operator=(const PublicNexusReader &); // do not define. Not copyable

	};


#endif


