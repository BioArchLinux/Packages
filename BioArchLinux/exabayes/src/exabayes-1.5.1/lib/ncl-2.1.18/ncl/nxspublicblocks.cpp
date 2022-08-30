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



#include <istream>
#include <fstream>
#include "ncl/nxspublicblocks.h"
#include "ncl/nxsreader.h"
using namespace std;


/*! Registers the strings is the vector as taxon labels.

	This is is a convenience function that creates a NxsTaxaBlock, fills it, and
	then calls AddReadTaxaBlock.

	\returns the block created.
*/
NxsTaxaBlock * PublicNexusReader::RegisterTaxa(const std::vector<std::string> & tl) {
	if (tl.empty()) {
		return 0L;
	}
	NxsTaxaBlock *tb = new NxsTaxaBlock();
	tb->SetNtax( (unsigned)tl.size() );
	for (std::vector<std::string>::const_iterator labelIt = tl.begin(); labelIt != tl.end(); ++labelIt)
		tb->AddTaxonLabel(*labelIt);
	AddReadTaxaBlock(tb);
	return tb;
}

/*! A convenience function to get a list of NxsBlocks from a file path without having to create and dispose of a  PublicNexusReader object*/
BlockReaderList PublicNexusReader::parseFileOrThrow(
    const char *filepath, /* path of file to parse */
    NxsReader::WarningHandlingMode mode,
    bool parsePrivateBlocks, /* true to store the commands found in  private blocks */
    bool storeTokenInfo)
    {
    PublicNexusReader nexusReader(mode);
    return NxsReader::parseFileWithReader(nexusReader, filepath, parsePrivateBlocks, storeTokenInfo);
    }

BlockReaderList DefaultErrorReportNxsReader::parseFile(
    const char *filepath, /* path of file to parse */
    std::ostream * stdOutstream,
    std::ostream * errOutstream,
    bool parsePrivateBlocks, /* true to store the commands found in  private blocks */
    bool storeTokenInfo)
    {
    DefaultErrorReportNxsReader nexusReader(stdOutstream, errOutstream);
    return NxsReader::parseFileWithReader(nexusReader, filepath, parsePrivateBlocks, storeTokenInfo);
    }

/*! Convenience function for reading a filepath.
   Returns a list of NxsBlock pointers (which the caller must delete)
   corresponding to the NxsBlocks found in the file.
   Raises NxsExceptions on errors.
*/
BlockReaderList NxsReader::parseFileWithReader(
    NxsReader & nexusReader,
    const char *filepath, /*!< path of file to parse */
    bool parsePrivateBlocks, /*!< true to store the commands found in  private blocks */
    bool storeTokenInfo) /*!< true for storage of full token info (such as file position) for private blocks */
    {
    if (!filepath)
        nexusReader.NexusError("Invalid (NULL) file specified to be parsed", 0, -1, -1);
    ifstream inf(filepath, ios::binary);
    if (!inf.good())
        {
        NxsString err;
        err << "Could not parse the file \"" << filepath <<"\"";
        nexusReader.NexusError(err, 0, -1, -1);
        }
    nexusReader.statusMessage("Creating token");
	NxsToken token(inf);
	NxsDefaultPublicBlockFactory factory(parsePrivateBlocks, storeTokenInfo);
	nexusReader.AddFactory(&factory);
	try {
        nexusReader.statusMessage("Executing");
	    nexusReader.Execute(token);
	    }
	catch(...)
	    {
        nexusReader.RemoveFactory(&factory);
        throw;
	    }
	nexusReader.RemoveFactory(&factory);
	BlockReaderList brl = nexusReader.GetBlocksFromLastExecuteInOrder();
	return brl;
    }


void NxsStoreTokensBlockReader::Reset()
	{
	NxsBlock::Reset();
	commandsRead.clear();
	}

void NxsStoreTokensBlockReader::ReportConst(std::ostream &out) const
	{
	out << NCL_BLOCKTYPE_ATTR_NAME << " block contains ";
	if (storeAllTokenInfo)
		{
		out << (unsigned)commandsRead.size() << " commands:\n";
		for (std::list<ProcessedNxsCommand>::const_iterator cIt = commandsRead.begin(); cIt != commandsRead.end(); ++cIt)
			{
			const ProcessedNxsToken & t = (*cIt)[0];
			out << "    " << t.GetToken() << "\n";
			}
		}
	else
		{
		out << (unsigned)justTokens.size() << " commands:\n";
		for (ListVecString::const_iterator cIt = justTokens.begin(); cIt != justTokens.end(); ++cIt)
			out << "    " << cIt->at(0) << "\n";
		}
	}

void NxsStoreTokensBlockReader::ReadCommand(
  NxsToken &token)	/* the token used to read from in */
	{
	if (storeAllTokenInfo)
		{
		ProcessedNxsCommand fullTokens;
		token.ProcessAsCommand(&fullTokens);
		if (!fullTokens.empty())
			commandsRead.push_back(fullTokens);
		}
	else
		{
		VecString justString;
		while (!token.Equals(";"))
			{
			justString.push_back(token.GetToken());
			token.GetNextToken();
			}
		if (!justString.empty())
			justTokens.push_back(justString);
		}
	}

void NxsStoreTokensBlockReader::Read(
  NxsToken &token)	/* the token used to read from in */
	{
	isEmpty = false;
	isUserSupplied = true;
	NxsString begcmd("BEGIN ");
	begcmd += this->NCL_BLOCKTYPE_ATTR_NAME;
	DemandEndSemicolon(token, begcmd.c_str());

	for(;;)
		{
		token.GetNextToken();
        if (token.Equals("END") || token.Equals("ENDBLOCK"))
            {
            HandleEndblock(token);
            return ;
            }
		this->ReadCommand(token);
		}
	}

void NxsStoreTokensBlockReader::WriteAsNexus(std::ostream &out) const
	{
	out << "BEGIN " << NxsString::GetEscaped(this->NCL_BLOCKTYPE_ATTR_NAME) << ";\n";
	if (storeAllTokenInfo)
		{
		for (std::list<ProcessedNxsCommand>::const_iterator cIt = commandsRead.begin(); cIt != commandsRead.end(); ++cIt)
			{
			const ProcessedNxsCommand & t = *cIt;
			if (WriteCommandAsNexus(out, t))
    			out << '\n';
			}
		}
	else
		{
		for (ListVecString::const_iterator cIt = justTokens.begin(); cIt != justTokens.end(); ++cIt)
			{
			const VecString & t = *cIt;
			out << "   ";
			for (VecString::const_iterator wIt = t.begin(); wIt != t.end(); ++wIt)
				out << ' ' << NxsString::GetEscaped(*wIt);
			out << ";\n";
			}
		}
	WriteSkippedCommands(out);
	out << "END;\n";
	}

/*! Returns a new instance of a block  for the appropriate block type NCL_BLOCKTYPE_ATTR_NAME. \ref BlockTypeIDDiscussion
*/
NxsBlock  *NxsDefaultPublicBlockFactory::GetBlockReaderForID(
  const std::string & NCL_BLOCKTYPE_ATTR_NAME, /*! \ref BlockTypeIDDiscussion */
  NxsReader *reader,
  NxsToken *token)
	{
	if (NCL_BLOCKTYPE_ATTR_NAME == "ASSUMPTIONS" || NCL_BLOCKTYPE_ATTR_NAME == "SETS")
		return assumpBlockFact.GetBlockReaderForID(NCL_BLOCKTYPE_ATTR_NAME, reader, token);
	if (NCL_BLOCKTYPE_ATTR_NAME == "CHARACTERS")
		return charBlockFact.GetBlockReaderForID(NCL_BLOCKTYPE_ATTR_NAME, reader, token);
	if (NCL_BLOCKTYPE_ATTR_NAME == "DATA")
		return dataBlockFact.GetBlockReaderForID(NCL_BLOCKTYPE_ATTR_NAME, reader, token);
	if (NCL_BLOCKTYPE_ATTR_NAME == "DISTANCES")
		return distancesBlockFact.GetBlockReaderForID(NCL_BLOCKTYPE_ATTR_NAME, reader, token);
	if (NCL_BLOCKTYPE_ATTR_NAME == "TAXA")
		return taxaBlockFact.GetBlockReaderForID(NCL_BLOCKTYPE_ATTR_NAME, reader, token);
	if (NCL_BLOCKTYPE_ATTR_NAME == "TREES")
		return treesBlockFact.GetBlockReaderForID(NCL_BLOCKTYPE_ATTR_NAME, reader, token);
	if (NCL_BLOCKTYPE_ATTR_NAME == "TAXAASSOCIATION")
		return taxaAssociationBlockFact.GetBlockReaderForID(NCL_BLOCKTYPE_ATTR_NAME, reader, token);
	if (NCL_BLOCKTYPE_ATTR_NAME == "UNALIGNED")
		return unalignedBlockFact.GetBlockReaderForID(NCL_BLOCKTYPE_ATTR_NAME, reader, token);
	if (tokenizeUnknownBlocks)
		{
		NxsStoreTokensBlockReader * nb = new NxsStoreTokensBlockReader(NCL_BLOCKTYPE_ATTR_NAME, storeTokenInfoArg);
		nb->SetImplementsLinkAPI(false);
    	return nb;
        }
	return NULL;
	}

/*! Creates a reader for the specified blocks.
	The first argument is integer with bits that is composed of bits from PublicNexusReader::NexusBlocksToRead,
		indicating which of the public blocks should be read. Either compose the
		argument by ORing together bits (such as NEXUS_TREES_BLOCK_BIT|NEXUS_TAXA_BLOCK_BIT)
		or simply pass in -1 to read all public blocks.
	The "standard" NCL NxsBlock (NxsCharactersBlock, NxsTaxaBlock...) instances will be
		created as initial clone block templates in the contained NxsCloneBlockFactory.
		These instances can be altered by using getting pointers to them using the
		GetAssumptionsBlockTemplate(), GetTaxaBlockTemplate()... methods.
*/
PublicNexusReader::PublicNexusReader(
  const int blocksToRead, /*!< integer with bits that is composed of bits from PublicNexusReader::NexusBlocksToRead, indicating which of the public blocks should be read*/
  NxsReader::WarningHandlingMode warnModeArg) /*!< warning mode (passed to ExceptionRaisingNxsReader::ExceptionRaisingNxsReader() */
	:ExceptionRaisingNxsReader(warnModeArg),
	bitsForBlocksToRead(blocksToRead),
	assumptionsBlockTemplate(0L),
	charactersBlockTemplate(0L),
	dataBlockTemplate(0L),
	distancesBlockTemplate(0L),
	storerBlockTemplate(0L),
	taxaBlockTemplate(0L),
	taxaAssociationBlockTemplate(0L),
	treesBlockTemplate(0L),
	unalignedBlockTemplate(0L)
{
	this->AddFactory(&cloneFactory);

	taxaBlockTemplate = new NxsTaxaBlock();
	taxaBlockTemplate->SetImplementsLinkAPI(false);
	cloneFactory.AddPrototype(taxaBlockTemplate);

	if (blocksToRead & NEXUS_ASSUMPTIONS_BLOCK_BIT)
		{
		assumptionsBlockTemplate = new NxsAssumptionsBlock(0L);
		assumptionsBlockTemplate->SetImplementsLinkAPI(true);
		cloneFactory.AddPrototype(assumptionsBlockTemplate, "ASSUMPTIONS");
		cloneFactory.AddPrototype(assumptionsBlockTemplate, "SETS");
		cloneFactory.AddPrototype(assumptionsBlockTemplate, "CODONS");
		}

	if (blocksToRead & NEXUS_TREES_BLOCK_BIT)
		{
		treesBlockTemplate = new NxsTreesBlock(NULL);
		treesBlockTemplate->SetCreateImpliedBlock(true);
		treesBlockTemplate->SetImplementsLinkAPI(true);
		treesBlockTemplate->SetProcessAllTreesDuringParse(true);
		treesBlockTemplate->SetAllowImplicitNames(true);
		treesBlockTemplate->SetWriteFromNodeEdgeDataStructure(true);
		cloneFactory.AddPrototype(treesBlockTemplate);
		}
	if (blocksToRead & NEXUS_CHARACTERS_BLOCK_BIT)
		{
		charactersBlockTemplate = new NxsCharactersBlock(NULL, NULL);
		charactersBlockTemplate->SetCreateImpliedBlock(true);
		charactersBlockTemplate->SetImplementsLinkAPI(true);
		charactersBlockTemplate->SetSupportMixedDatatype(true);
		charactersBlockTemplate->SetConvertAugmentedToMixed(true);

		dataBlockTemplate = new NxsDataBlock(NULL, NULL);
		dataBlockTemplate->SetCreateImpliedBlock(true);
		dataBlockTemplate->SetImplementsLinkAPI(true);
		dataBlockTemplate->SetSupportMixedDatatype(true);
		dataBlockTemplate->SetConvertAugmentedToMixed(true);
		cloneFactory.AddPrototype(charactersBlockTemplate, "CHARACTERS");
		cloneFactory.AddPrototype(dataBlockTemplate, "DATA");
		}
	if (blocksToRead & NEXUS_UNALIGNED_BLOCK_BIT)
		{
		unalignedBlockTemplate = new NxsUnalignedBlock(NULL);
		unalignedBlockTemplate->SetCreateImpliedBlock(true);
		unalignedBlockTemplate->SetImplementsLinkAPI(true);
		cloneFactory.AddPrototype(unalignedBlockTemplate);
		}
	if (blocksToRead & NEXUS_DISTANCES_BLOCK_BIT)
		{
		distancesBlockTemplate = new NxsDistancesBlock(NULL);
		distancesBlockTemplate->SetCreateImpliedBlock(true);
		distancesBlockTemplate->SetImplementsLinkAPI(true);
		cloneFactory.AddPrototype(distancesBlockTemplate);
		}
	if (blocksToRead & NEXUS_TAXAASSOCIATION_BLOCK_BIT)
		{
		taxaAssociationBlockTemplate = new NxsTaxaAssociationBlock();
		cloneFactory.AddPrototype(taxaAssociationBlockTemplate);
		}
	if (blocksToRead & NEXUS_UNKNOWN_BLOCK_BIT)
		{
		std::string emptyString;
		storerBlockTemplate = new NxsStoreTokensBlockReader(emptyString, true);
		storerBlockTemplate->SetImplementsLinkAPI(false);
		cloneFactory.AddDefaultPrototype(storerBlockTemplate);
		}
}

/*! \ref NxsReader::Execute().  This method calls PostExecuteHook() after NxsReader::Execute
		is completed.
*/
void PublicNexusReader::Execute(NxsToken& token, bool notifyStartStop)
{
	NxsReader::Execute(token, notifyStartStop);
	PostExecuteHook();
}

/*! \ref Called after successful execute.
	in the PublicNexusReader, this function up-casts blocks to the type that
	they should be.

	\warn if you derive from PublicNexusReader and change the type of the clone templates, then
	you must override this function so that the casts in this function will be safe.
*/
void PublicNexusReader::PostExecuteHook()
{
	BlockReaderList blocks = GetBlocksFromLastExecuteInOrder();
	for (BlockReaderList::const_iterator bIt = blocks.begin(); bIt != blocks.end(); ++bIt)
		{
		NxsBlock * b = *bIt;
		const std::string NCL_BLOCKTYPE_ATTR_NAME = b->GetID();
		const std::string capId = NxsString::get_upper(NCL_BLOCKTYPE_ATTR_NAME);
		const char * capIdP = capId.c_str();
		if (strcmp(capIdP, "TAXA") == 0)
			taxaBlockVec.push_back(static_cast<NxsTaxaBlock *>(b));
		else if (strcmp(capIdP, "TREES") == 0)
			treesBlockVec.push_back(static_cast<NxsTreesBlock *>(b));
		else if ((strcmp(capIdP, "CHARACTERS") == 0) || (strcmp(capIdP, "DATA") == 0))
			charactersBlockVec.push_back(static_cast<NxsCharactersBlock *>(b));
		else if ((strcmp(capIdP, "ASSUMPTIONS") == 0) || (strcmp(capIdP, "SETS") == 0) || (strcmp(capIdP, "CODONS") == 0))
			assumptionsBlockVec.push_back(static_cast<NxsAssumptionsBlock *>(b));
		else if (strcmp(capIdP, "DISTANCES") == 0)
			distancesBlockVec.push_back(static_cast<NxsDistancesBlock *>(b));
		else if (strcmp(capIdP, "TAXAASSOCIATION") == 0)
			taxaAssociationBlockVec.push_back(static_cast<NxsTaxaAssociationBlock *>(b));
		else if (strcmp(capIdP, "UNALIGNED") == 0)
			unalignedBlockVec.push_back(static_cast<NxsUnalignedBlock *>(b));
		else
			{
			storerBlockVec.push_back(static_cast<NxsStoreTokensBlockReader *>(b));
			}
		}
}

void PublicNexusReader::AddFactory(NxsBlockFactory *f)
{
	if (f == &cloneFactory)
		NxsReader::AddFactory(f);
	else
		{
		NCL_ASSERT(false);
		}
}

PublicNexusReader::~PublicNexusReader()
{
	delete assumptionsBlockTemplate;
	delete charactersBlockTemplate;
	delete dataBlockTemplate;
	delete distancesBlockTemplate;
	delete storerBlockTemplate;
	delete taxaBlockTemplate;
	delete taxaAssociationBlockTemplate;
	delete treesBlockTemplate;
	delete unalignedBlockTemplate;
}

unsigned PublicNexusReader::GetNumAssumptionsBlocks(const NxsTaxaBlock *taxa) const
	{
	unsigned n = 0;
	std::vector<NxsAssumptionsBlock *>::const_iterator bIt = assumptionsBlockVec.begin();
	for (; bIt != assumptionsBlockVec.end(); ++bIt)
		{
		NxsAssumptionsBlock * b = *bIt;
		if (taxa && taxa != b->taxa)
			continue;
		n++;
		}
	return n;
	}

NxsAssumptionsBlock * PublicNexusReader::GetAssumptionsBlock(const NxsTaxaBlock *taxa, unsigned index) const
	{
	unsigned n = 0;
	std::vector<NxsAssumptionsBlock *>::const_iterator bIt = assumptionsBlockVec.begin();
	for (; bIt != assumptionsBlockVec.end(); ++bIt)
		{
		NxsAssumptionsBlock * b = *bIt;
		if (taxa && taxa != b->taxa)
			continue;
		if (index == n)
			return b;
		n++;
		}
	return 0L;
	}

unsigned PublicNexusReader::GetNumAssumptionsBlocks( const NxsCharactersBlock * chars) const
	{
	unsigned n = 0;
	std::vector<NxsAssumptionsBlock *>::const_iterator bIt = assumptionsBlockVec.begin();
	for (; bIt != assumptionsBlockVec.end(); ++bIt)
		{
		NxsAssumptionsBlock * b = *bIt;
		if (chars && chars != b->charBlockPtr)
			continue;
		n++;
		}
	return n;
	}

NxsAssumptionsBlock * PublicNexusReader::GetAssumptionsBlock(const NxsCharactersBlock * chars, unsigned index) const
	{
	unsigned n = 0;
	std::vector<NxsAssumptionsBlock *>::const_iterator bIt = assumptionsBlockVec.begin();
	for (; bIt != assumptionsBlockVec.end(); ++bIt)
		{
		NxsAssumptionsBlock * b = *bIt;
		if (chars && chars != b->charBlockPtr)
			continue;
		if (index == n)
			return b;
		n++;
		}
	return 0L;
	}

unsigned PublicNexusReader::GetNumAssumptionsBlocks(const NxsTreesBlock *tree) const
	{
	unsigned n = 0;
	std::vector<NxsAssumptionsBlock *>::const_iterator bIt = assumptionsBlockVec.begin();
	for (; bIt != assumptionsBlockVec.end(); ++bIt)
		{
		NxsAssumptionsBlock * b = *bIt;
		if (tree && tree != b->treesBlockPtr)
			continue;
		n++;
		}
	return n;
	}

NxsAssumptionsBlock * PublicNexusReader::GetAssumptionsBlock(const NxsTreesBlock *tree, unsigned index) const
	{
	unsigned n = 0;
	std::vector<NxsAssumptionsBlock *>::const_iterator bIt = assumptionsBlockVec.begin();
	for (; bIt != assumptionsBlockVec.end(); ++bIt)
		{
		NxsAssumptionsBlock * b = *bIt;
		if (tree && tree != b->treesBlockPtr)
			continue;
		if (index == n)
			return b;
		n++;
		}
	return 0L;
	}

unsigned PublicNexusReader::GetNumCharactersBlocks(const NxsTaxaBlock *taxa) const
	{
	unsigned n = 0;
	std::vector<NxsCharactersBlock *>::const_iterator bIt = charactersBlockVec.begin();
	for (; bIt != charactersBlockVec.end(); ++bIt)
		{
		NxsCharactersBlock * b = *bIt;
		if (!taxa || taxa == b->taxa)
			n++;
		}
	return n;
	}

NxsCharactersBlock * PublicNexusReader::GetCharactersBlock(const NxsTaxaBlock *taxa, unsigned index) const
	{
	unsigned n = 0;
	std::vector<NxsCharactersBlock *>::const_iterator bIt = charactersBlockVec.begin();
	for (; bIt != charactersBlockVec.end(); ++bIt)
		{
		NxsCharactersBlock * b = *bIt;
		if (!taxa || taxa == b->taxa)
			{
			if (index == n)
				return b;
			n++;
			}
		}
	return 0L;
	}

unsigned PublicNexusReader::GetNumDistancesBlocks(const NxsTaxaBlock *taxa) const
	{
	unsigned n = 0;
	std::vector<NxsDistancesBlock *>::const_iterator bIt = distancesBlockVec.begin();
	for (; bIt != distancesBlockVec.end(); ++bIt)
		{
		NxsDistancesBlock * b = *bIt;
		if (!taxa || taxa == b->taxa)
			n++;
		}
	return n;
	}

NxsDistancesBlock * PublicNexusReader::GetDistancesBlock(const NxsTaxaBlock *taxa, unsigned index) const
	{
	unsigned n = 0;
	std::vector<NxsDistancesBlock *>::const_iterator bIt = distancesBlockVec.begin();
	for (; bIt != distancesBlockVec.end(); ++bIt)
		{
		NxsDistancesBlock * b = *bIt;
		if (!taxa || taxa == b->taxa)
			{
			if (index == n)
				return b;
			n++;
			}
		}
	return 0L;
	}

unsigned PublicNexusReader::GetNumUnalignedBlocks(const NxsTaxaBlock *taxa) const
	{
	unsigned n = 0;
	std::vector<NxsUnalignedBlock *>::const_iterator bIt = unalignedBlockVec.begin();
	for (; bIt != unalignedBlockVec.end(); ++bIt)
		{
		NxsUnalignedBlock * b = *bIt;
		if (!taxa || taxa == b->taxa)
			n++;
		}
	return n;
	}

NxsUnalignedBlock * PublicNexusReader::GetUnalignedBlock(const NxsTaxaBlock *taxa, unsigned index) const
	{
	unsigned n = 0;
	std::vector<NxsUnalignedBlock *>::const_iterator bIt = unalignedBlockVec.begin();
	for (; bIt != unalignedBlockVec.end(); ++bIt)
		{
		NxsUnalignedBlock * b = *bIt;
		if (!taxa || taxa == b->taxa)
			{
			if (index == n)
				return b;
			n++;
			}
		}
	return 0L;
	}


unsigned PublicNexusReader::GetNumTaxaAssociationBlocks(const NxsTaxaBlock *taxa) const
	{
	unsigned n = 0;
	std::vector<NxsTaxaAssociationBlock *>::const_iterator bIt = taxaAssociationBlockVec.begin();
	for (; bIt != taxaAssociationBlockVec.end(); ++bIt)
		{
		NxsTaxaAssociationBlock * b = *bIt;
		if (!taxa || taxa == b->GetFirstTaxaBlock() || taxa == b->GetSecondTaxaBlock())
			n++;
		}
	return n;
	}

NxsTaxaAssociationBlock * PublicNexusReader::GetTaxaAssociationBlock(const NxsTaxaBlock *taxa, unsigned index) const
	{
	unsigned n = 0;
	std::vector<NxsTaxaAssociationBlock *>::const_iterator bIt = taxaAssociationBlockVec.begin();
	for (; bIt != taxaAssociationBlockVec.end(); ++bIt)
		{
		NxsTaxaAssociationBlock * b = *bIt;
		if (!taxa || taxa == b->GetFirstTaxaBlock() || taxa == b->GetSecondTaxaBlock())
			{
			if (index == n)
				return b;
			n++;
			}
		}
	return 0L;
	}

unsigned PublicNexusReader::GetNumTreesBlocks(const NxsTaxaBlock *taxa) const
	{
	unsigned n = 0;
	std::vector<NxsTreesBlock *>::const_iterator bIt = treesBlockVec.begin();
	for (; bIt != treesBlockVec.end(); ++bIt)
		{
		NxsTreesBlock * b = *bIt;
		if (!taxa || taxa == b->taxa)
			n++;
		}
	return n;
	}

NxsTreesBlock * PublicNexusReader::GetTreesBlock(const NxsTaxaBlock *taxa, unsigned index) const
	{
	unsigned n = 0;
	std::vector<NxsTreesBlock *>::const_iterator bIt = treesBlockVec.begin();
	for (; bIt != treesBlockVec.end(); ++bIt)
		{
		NxsTreesBlock * b = *bIt;
		if (!taxa || taxa == b->taxa)
			{
			if (index == n)
				return b;
			n++;
			}
		}
	return 0L;
	}

unsigned PublicNexusReader::GetNumUnknownBlocks() const
	{
	return (unsigned)storerBlockVec.size();
	}

NxsStoreTokensBlockReader * PublicNexusReader::GetUnknownBlock(unsigned index) const
	{
	if (index < storerBlockVec.size())
		return storerBlockVec[index];
	return 0L;
	}

unsigned PublicNexusReader::GetNumTaxaBlocks() const
	{
	return (unsigned)taxaBlockVec.size();
	}

NxsTaxaBlock * PublicNexusReader::GetTaxaBlock(unsigned index) const
	{
	if (index < taxaBlockVec.size())
		return taxaBlockVec[index];
	return 0L;
	}

void PublicNexusReader::ClearUsedBlockList()
	{
	NxsReader::ClearUsedBlockList();
	assumptionsBlockVec.clear();
	charactersBlockVec.clear();
	dataBlockVec.clear();
	distancesBlockVec.clear();
	storerBlockVec.clear();
	taxaBlockVec.clear();
	taxaAssociationBlockVec.clear();
	treesBlockVec.clear();
	unalignedBlockVec.clear();
	}




bool fileExists(const std::string &fn);

// this is not a great way to check for existence - we may lack read permissions.
bool fileExists(const std::string &fn)
{
	std::ifstream inf;
	inf.open(fn.c_str());
	const bool b = inf.good();
	inf.close();
	return b;
}

std::string NxsConversionOutputRecord::getUniqueFilenameWithLowestIndex(const char * prefix)
{
	NxsString fn;
	fn.assign(prefix);
	const unsigned MAX_SUFFIX =  10000;
	for (unsigned i = 1; i <= MAX_SUFFIX ; ++i)
		{
		if (!fileExists(fn))
			return fn;
		fn.assign(prefix);
		fn << i;
		}
	fn.clear();
	fn << "Files \"" << prefix << "\" through \"" << prefix << MAX_SUFFIX << "\" exist, and I am afraid to write any more files to that directory. I quit.";
	throw NxsException(fn);
}

// writes the name pairs separated by newlines to a file whose filepath is specified
//	by fn.
void NxsConversionOutputRecord::writeTaxonNameTranslationFilepath(const char * fn, const std::vector<NxsNameToNameTrans> & nameTrans, const NxsTaxaBlockAPI *tb, bool verbose)
{
	std::ofstream tnf;
	tnf.open(fn);
	if (!tnf.good())
		{
		NxsString msg;
		msg << "Could not open the file " << fn << " for writing translation of names";
		throw NxsException(msg);
		}
	if (verbose)
		std::cerr << "Writing \"" << fn << "\" to store the translation of names\n";
	writeTaxonNameTranslationStream(tnf, nameTrans, tb);
	tnf.close();
}


// writes the name pairs separated by newlines to the ostream tnf
void NxsConversionOutputRecord::writeTaxonNameTranslationStream(std::ostream & tnf, const std::vector<NxsNameToNameTrans> & nameTrans, const NxsTaxaBlockAPI *tb)
{
	std::string blockLabel = tb->GetTitle();
	tnf << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	tnf << "<taxa label=";
	writeAttributeValue(tnf, blockLabel);
	tnf << " >\n";
	for (std::vector<NxsNameToNameTrans>::const_iterator nIt = nameTrans.begin(); nIt != nameTrans.end(); ++nIt)
		{
		tnf << " <taxon src=";
		writeAttributeValue(tnf, nIt->first);
		tnf << " dest=";
		writeAttributeValue(tnf, nIt->second);
		tnf << " />\n";
		}
	tnf << "</taxa>\n";
}

void NxsConversionOutputRecord::writeNameTranslation(std::vector<NxsNameToNameTrans> nameTrans, const NxsTaxaBlockAPI * taxa)
	{
	if (taxaBlocksToConversionFiles.find(taxa) != taxaBlocksToConversionFiles.end())
		return;
	std::string fn;
	if (this->numberTranslationFiles)
		fn = getUniqueFilenameWithLowestIndex(this->translationFilename.c_str());
	else
		fn = this->translationFilename;
	writeTaxonNameTranslationFilepath(fn.c_str(), nameTrans, taxa, this->verboseWritingOfNameTranslationFile);
	taxaBlocksToConversionFiles[taxa] = fn;
	}

void writeAttributeValue(ostream & out, const std::string & v)
	{
	if (v.c_str() == NULL)
		out << "\'\'";
	else
		{

		if (v.find_first_of("\'\"&") != string::npos)
			{
			if (strchr(v.c_str(), '\'') != NULL)
				{
				out << '\"';
				for (std::string::const_iterator cIt = v.begin(); cIt != v.end(); ++cIt)
					{
					const char & c = *cIt;
					if (c == '\"')
						out << "&quot;";
					else if (c == '&')
						out << "&amp;";
					else
						out << c;
					}
				out << '\"';

				}
			else
				{
				out << '\'';
				for (std::string::const_iterator cIt = v.begin(); cIt != v.end(); ++cIt)
					{
					const char & c = *cIt;
					if (c == '&')
						out << "&amp;";
					else
						out << c;
					}
				out << '\'';
				}
			}
		else
			out << '\'' << v << '\'';
		}
	}



