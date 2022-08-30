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
#include <csignal>
#include <algorithm>
#include <set>
#include <fstream>
#include <climits>
#include <sstream>
#include <iterator>
#include "ncl/nxsreader.h"
#include "ncl/nxsdefs.h"
#include "ncl/nxscharactersblock.h"
#include "ncl/nxstaxablock.h"
#include "ncl/nxstreesblock.h"

using namespace std;

#if defined(NCL_CONST_FUNCS) && NCL_CONST_FUNCS
	int onlyDefinedInCouldBeConst()
	{
		return 1;
	}

#endif


static void NxsHandleSignalCallback(int);

NxsReader::SignalHandlerFuncPtr NxsReader::prevSignalCatcher = 0L;
bool NxsReader::nclCatchesSignals = false;
unsigned NxsReader::numSigIntsCaught = 0;
bool NxsReader::prevSignalStored = true;

unsigned NxsReader::getNumSignalIntsCaught()
	{
	return NxsReader::numSigIntsCaught;
	}

void NxsReader::setNumSignalsIntsCaught(unsigned n)
	{
	NxsReader::numSigIntsCaught = n;
	}

void NxsReader::setNCLCatchesSignals(bool v)
	{
	NxsReader::nclCatchesSignals = v;
	}

bool NxsReader::getNCLCatchesSignals()
	{
	return NxsReader::nclCatchesSignals;
	}


void NxsHandleSignalCallback(int)
	{
	unsigned nc = NxsReader::getNumSignalIntsCaught();
	NxsReader::setNumSignalsIntsCaught(1 + nc);
	}

void NxsReader::installNCLSignalHandler()
	{
	NxsReader::SignalHandlerFuncPtr prev = std::signal(SIGINT, SIG_IGN);
	if (prev != SIG_IGN)
		{
		NxsReader::prevSignalCatcher = prev;
		NxsReader::prevSignalStored = true;
		std::signal(SIGINT, NxsHandleSignalCallback);
		}
	}

void NxsReader::uninstallNCLSignalHandler()
	{
	if (prevSignalStored)
		{
		std::signal(SIGINT, NxsReader::prevSignalCatcher);
		NxsReader::prevSignalCatcher = 0L;
		NxsReader::prevSignalStored = false;
		}
	}



/*! Reads a filename with NxsToken object. Calls NexusError on failures */
void NxsReader::ReadFilepath(const char *filename)
	{
	std::ifstream inf;
	try{
		inf.open(filename, ios::binary);
		if (!inf.good())
			{
			NxsString err;
			err << "Could not open the file \"" << filename <<"\"";
			this->NexusError(err, 0, -1, -1);
			}
		}
	catch (...)
		{
		NxsString err;
		err << '\"' << filename <<"\" does not refer to a valid file." ;
		this->NexusError(err, 0, -1, -1);
		}
	this->ReadFilestream(inf);
	}


/*! Reads the content of string `s` as if it were NEXUS. */
void NxsReader::ReadStringAsNexusContent(const std::string & s)
	{
	std::istringstream inf(s);
	this->ReadFilestream(inf);
	}

/*! Reads the istream `inf` by creating a NxsToken object and then calling NxsReader::Execute() */
void NxsReader::ReadFilestream(std::istream & inf)
	{
	NxsToken token(inf);
	this->Execute(token);
	}

/*! Returns the set of blocks that have been created from factories, and
	removes reference to from the NxsReader's collections.
*/
std::set<NxsBlock *> NxsReader::RemoveBlocksFromFactoriesFromUsedBlockLists()
	{
	std::set<NxsBlock *> todel;
	BlockReaderList saved;
	for (BlockReaderList::iterator bIt = blocksInOrder.begin(); bIt != blocksInOrder.end(); ++bIt)
		{
		NxsBlock * b  = *bIt;
		if (BlockIsASingeltonReader(b))
			saved.push_back(b);
		else
			{
			todel.insert(b);
			}
		}
	for (std::set<NxsBlock *>::iterator d = todel.begin(); d != todel.end(); ++d)
		{
		RemoveBlockFromUsedBlockList(*d);
		}
	return todel;
	}

/*! Deletes the set of blocks that have been created from factories and
	removes reference to from the NxsReader's collections.
*/
void NxsReader::DeleteBlocksFromFactories()
	{
	std::set<NxsBlock *> todel = RemoveBlocksFromFactoriesFromUsedBlockLists();
	for (std::set<NxsBlock *>::iterator d = todel.begin(); d != todel.end(); ++d)
		delete *d;
	}

/*! \returns true if the block `b` is one of the registered block readers (rather
	than a block from a factory).
*/
bool NxsReader::BlockIsASingeltonReader(NxsBlock *b) const
	{
	NxsBlock * sb = blockList;
	while (sb)
		{
		if (b == sb)
			return true;
		sb = sb->next;
		}
	return false;
	}

/*! \returns a NxsBlock from `chosenBlockList` with a Title that matches `title`.
	In the event of ties, the most recently read block is returned.
	If `title` is NULL, then any block is considered a match.
	On output *nMatches will be the number of matches (if `nMatches` is not NULL).
	NULL will be returned if there are no matches.
*/
NxsBlock *NxsReader::FindBlockByTitle(const BlockReaderList & chosenBlockList, const char *title, unsigned *nMatches)
	{
	BlockReaderList  found = FindAllBlocksByTitle(chosenBlockList, title);

	if (found.empty())
		{
		if (nMatches)
			*nMatches = 0;
		return NULL;
		}
	if (nMatches)
		*nMatches = (unsigned)found.size();
	return (NxsBlock *) found.back();
	}



BlockReaderList NxsReader::FindAllBlocksByTitle(const BlockReaderList & chosenBlockList, const char *title)
	{
	BlockReaderList found = FindAllBlocksByTitleNoPrioritization(chosenBlockList, title);
	if (found.empty())
		return found;
	map<int, BlockReaderList> byPriority;
	for (BlockReaderList::const_iterator fIt = found.begin(); fIt != found.end(); ++fIt)
		{
		NxsBlock * b = *fIt;
		int priority = GetBlockPriority(b);
		byPriority[priority].push_back(b);
		}
	NCL_ASSERT(!byPriority.empty());
	return byPriority.rbegin()->second;
	}

BlockReaderList NxsReader::FindAllBlocksByTitleNoPrioritization(const BlockReaderList & chosenBlockList, const char *title)
	{
	BlockReaderList found;
	if (chosenBlockList.empty() || title == NULL)
		{
		found = chosenBlockList;
		}
	else
		{
		bool emptyTitle = strlen(title) == 0;
		for (BlockReaderList::const_iterator cblIt = chosenBlockList.begin(); cblIt != chosenBlockList.end(); ++cblIt)
			{
			NxsBlock * b = *cblIt;
			std::vector<std::string> v = this->GetAllTitlesForBlock(b);
			for (std::vector<std::string>::const_iterator vIt = v.begin(); vIt != v.end(); ++vIt)
				{
				const std::string & n = *vIt;
				if ((emptyTitle && n.empty()) || (NxsString::case_insensitive_equals(title, n.c_str())))
					{
					found.push_back(b);
					break;
					}
				}
			}
		}
	return found;

	}

/*! 	\returns all of the TITLEs that have been used for the same block.

	Identical blocks with the different titles can be stored once with all of the
titles stored a list of "alias titles"

	This will only happen for TAXA blocks, currently.
*/
std::vector<std::string> NxsReader::GetAllTitlesForBlock(const NxsBlock *b) const
	{
	std::vector<std::string> v;
	v.push_back(b->GetTitle());
	std::map<const NxsBlock *, std::list<std::string> >::const_iterator a = blockTitleAliases.find(b);
	if (a != blockTitleAliases.end())
		std::copy(a->second.begin(), a->second.end(), back_inserter(v));
	return v;
	}

/*! 	used internally to register a new "alias title" for a block */
void NxsReader::RegisterAltTitle(const NxsBlock * b, std::string t)
	{
	std::list<std::string> & v = blockTitleAliases[b];
	v.push_back(t);
	}

/*! \returns the pointer to the block with type ID (TAXA, CHARACTERS, ...) matching `btype`
	 and title matching `title` or 0L if there is no such block.
	 on output `nMatches` (if it is not 0L) will list the number of blocks that match this
	 criteria.
*/
NxsBlock *NxsReader::FindBlockOfTypeByTitle(const std::string &btype, const char *title, unsigned *nMatches)
	{
	BlockTypeToBlockList::const_iterator btblIt = blockTypeToBlockList.find(btype);
	if (btblIt == blockTypeToBlockList.end())
		{
		if (nMatches)
			*nMatches = 0;
		return NULL;
		}
	const BlockReaderList & chosenBlockList = btblIt->second;
	return FindBlockByTitle(chosenBlockList, title, nMatches);
	}

/*!
	NOTE: cast to NxsTaxaBlockAPI *.  This should only called by NCL when factories and the Link API are in effect.
	When using these APIs, block readers that read "TAXA" blocks in a NEXUS file must inherit from
	NxsTaxaBlockAPI, or the behavior will be undefined.
	This requirement also applies to "implied" taxa blocks that are returned from CHARACTERS (or other) blocks.
*/
NxsTaxaBlockAPI *NxsReader::GetTaxaBlockByTitle(const char *title, unsigned *nMatches)
	{
	const std::string btype("TAXA");
	return static_cast<NxsTaxaBlockAPI *>(FindBlockOfTypeByTitle(btype, title, nMatches));
	}

/*!
	NOTE: cast to NxsCharactersBlockAPI *.	This should only called by NCL when factories and the Link API are in effect.
	When using these APIs, block readers that read "CHARACTERS" or "DATA" blocks in a NEXUS file must inherit from
	NxsCharactersBlockAPI, or the behavior will be undefined.
*/
NxsCharactersBlockAPI	*NxsReader::GetCharBlockByTitle(const char *title, unsigned *nMatches)
	{
	const std::string btype("CHARACTERS");
	return static_cast<NxsCharactersBlockAPI *>(FindBlockOfTypeByTitle(btype, title, nMatches));
	}
/*!
	NOTE: cast to NxsTreesBlockAPI *.  This should only called by NCL when factories and the Link API are in effect.  In
	this case block readers that read "TREES" blocks in a NEXUS file must inherit from NxsTaxaBlockAPI, or the
	behavior will be undefined.
*/
NxsTreesBlockAPI *NxsReader::GetTreesBlockByTitle(const char *title, unsigned *nMatches)
	{
	const std::string btype("TREES");
	return static_cast<NxsTreesBlockAPI *>(FindBlockOfTypeByTitle(btype, title, nMatches));
	}

/*! Initializes both `blockList' and `currBlock' to NULL.
*/
NxsReader::NxsReader() : currentWarningLevel(UNCOMMON_SYNTAX_WARNING), alwaysReportStatusMessages(false)
	{
	blockList	= NULL;
	currBlock	= NULL;
	taxaBlockFactory = NULL;
	destroyRepeatedTaxaBlocks = false;
	}

NxsReader::~NxsReader()
	{
	NxsBlock *curr;
	for (curr = blockList; curr;)
		{
		if (curr->GetNexus() == this)
			curr->SetNexus(NULL);
		curr = curr->next;
		}
	for (BlockReaderList::iterator b = blocksInOrder.begin(); b != blocksInOrder.end(); ++b)
		{
		if ((*b)->GetNexus() == this)
			(*b)->SetNexus(NULL);
		}

	}

/*!
	Add a factory for NEXUS block readers to the front of the factories list.
*/
void NxsReader::AddFactory(NxsBlockFactory *f)
	{
	if (f)
		factories.push_front(f);
	}
/*!
	Remove a factory for NEXUS block readers.
*/
void NxsReader::RemoveFactory(NxsBlockFactory *f)
	{
	factories.remove(f);
	}


/*!
	Adds `newBlock' to the end of the list of NxsBlock objects growing from `blockList'. If `blockList' points to NULL,
	this function sets `blockList' to point to `newBlock'. Calls SetNexus method of `newBlock' to inform `newBlock' of
	the NxsReader object that now owns it. This is useful when the `newBlock' object needs to communicate with the
	outside world through the NxsReader object, such as when it issues progress reports as it is reading the contents
	of its block.
*/
void NxsReader::Add(
  NxsBlock *newBlock)	/* a pointer to an existing block object */
	{
	NCL_ASSERT(newBlock != NULL);

	newBlock->SetNexus(this);

	if (!blockList)
		blockList = newBlock;
	else
		{
		// Add new block to end of list
		//
		NxsBlock *curr;
		for (curr = blockList; curr && curr->next;)
			curr = curr->next;
		NCL_ASSERT(curr && !curr->next);
		curr->next = newBlock;
		}
	}

/*!
	\deprecated
	Returns position (first block has position 0) of block `b' in `blockList'. Returns UINT_MAX if `b' cannot be found
	in `blockList'.
*/
unsigned NxsReader::PositionInBlockList(
  NxsBlock *b)	/* a pointer to an existing block object */
	{
	unsigned pos = 0;
	NxsBlock *curr = blockList;

	for (;;)
		{
		if (curr == NULL || curr == b)
			break;
		pos++;
		curr = curr->next;
		}

	if (curr == NULL)
		pos = UINT_MAX;

	return pos;
	}

/*!
	Reassign should be called if a block (`oldb') is about to be deleted (perhaps to make way for new data). Create
	the new block (`newb') before deleting `oldb', then call Reassign to replace `oldb' in `blockList' with `newb'.
	Assumes `oldb' exists and is in `blockList'.

	This function is necessary in v2.0, but replacement of blocks is more easily done
	with block factories in NCL v2.1 and higher.
*/
void NxsReader::Reassign(
  NxsBlock *oldb,	/* a pointer to the block object soon to be deleted */
  NxsBlock *newb)	/* a pointer to oldb's replacement */
	{
	NxsBlock *prev = NULL;
	NxsBlock *curr = blockList;
	newb->SetNexus(this);

	for (;;)
		{
		if (curr == NULL || curr == oldb)
			break;
		prev = curr;
		curr = curr->next;
		}

	NCL_ASSERT(curr != NULL);

	newb->next = curr->next;
	if (prev == NULL)
		blockList = newb;
	else
		prev->next = newb;
	curr->next = NULL;
	curr->SetNexus(NULL);
	}

bool NxsReader::BlockListEmpty()
	{
	return (blockList == NULL ? true : false);
	}

/*!
	This function was created for purposes of debugging a new NxsBlock. This version does nothing; to create an active
	DebugReportBlock function, override this version in the derived class and call the Report function of `nexusBlock'.
	This function is called whenever the main NxsReader Execute function encounters the [&spillall] command comment
	between blocks in the data file. The Execute function goes through all blocks and passes them, in turn, to this
	DebugReportBlock function so that their contents are displayed. Placing the [&spillall] command comment between
	different versions of a block allows multiple blocks of the same type to be tested using one long data file. Say
	you are interested in testing whether the normal, transpose, and interleave format of a matrix can all be read
	correctly. If you put three versions of the block in the data file one after the other, the second one will wipe out
	the first, and the third one will wipe out the second, unless you have a way to report on each one before the next
	one is read. This function provides that ability.
*/
void NxsReader::DebugReportBlock(
  NxsBlock &)	/* the block that should be reported */
	{
	}

/*!
	Detaches `oldBlock' from the list of NxsBlock objects growing from `blockList'. If `blockList' itself points to
	`oldBlock', this function sets `blockList' to point to `oldBlock->next'. Note: the object pointed to by `oldBlock'
	is not deleted, it is simply detached from the linked list. No harm is done in Detaching a block pointer that has
	already been detached previously; if `oldBlock' is not found in the block list, Detach simply returns quietly. If
	`oldBlock' is found, its SetNexus object is called to set the NxsReader pointer to NULL, indicating that it is no
	longer owned by (i.e., attached to) a NxsReader object.
*/
void NxsReader::Detach(
  NxsBlock *oldBlock)	/* a pointer to an existing block object */
	{
	NCL_ASSERT(oldBlock != NULL);
	RemoveBlockFromUsedBlockList(oldBlock);
	// Return quietly if there are not blocks attached
	//
	if (blockList == NULL)
		return;

	if (blockList == oldBlock)
		{
		blockList = oldBlock->next;
		oldBlock->SetNexus(NULL);
		}
	else
		{
		// Bug fix MTH 6/17/2002: old version detached intervening blocks as well
		//
		NxsBlock *curr = blockList;
		for (; curr->next != NULL && curr->next != oldBlock;)
			curr = curr->next;

		// Line below can be uncommented to find cases where Detach function is
		// called for pointers that are not in the linked list. If line below is
		// uncommented, the part of the descriptive comment that precedes this
		// function about "...simply returns quietly" will be incorrect (at least
		// in the Debugging version of the program where asserts are active).
		//
		//NCL_ASSERT(curr->next == oldBlock);

		if (curr->next == oldBlock)
			{
			curr->next = oldBlock->next;
			oldBlock->SetNexus(NULL);
			}
		}
	}

/*!
	Called by the NxsReader object when a block named `blockName' is entered. Allows derived class overriding this
	function to notify user of progress in parsing the NEXUS file. Also gives program the opportunity to ask user if it
	is ok to purge data currently contained in this block. If user is asked whether existing data should be deleted, and
	the answer comes back no, then then the overrided function should return false, otherwise it should return true.
	This (base class) version always returns true.
*/
bool NxsReader::EnteringBlock(
  NxsString )	/* the name of the block just entered */
	{
	return true;
	}

/*!
	Called by the NxsReader object when a block named `blockName' is being exited. Allows derived class overriding this
	function to notify user of progress in parsing the NEXUS file.
*/
void NxsReader::ExitingBlock(
  NxsString )	/* the name of the block being exited */
	{
	}

/*!
	Called after `block' has returned from NxsBlock::Read()
*/
void NxsReader::PostBlockReadingHook(
  NxsBlock & /*block*/) /// the block that was just read
	{
	}

/*! Uses the registered Factories to spawn a reader for blocks with the name "currBlockName."
	if sourceOfBlock is not NULL, then *sourceOfBlock will alias to the NxsBlockFactory used.
	Returns NULL (and does not modify *sourceOfBlock), if no Factory is found that returns a block
*/
NxsBlock *NxsReader::CreateBlockFromFactories(const std::string & currBlockName, NxsToken &token, NxsBlockFactory **sourceOfBlock)
	{
	for (BlockFactoryList::iterator fIt = factories.begin(); currBlock == NULL && fIt != factories.end(); ++fIt)
		{
		NxsBlock *b = (*fIt)->GetBlockReaderForID(currBlockName, this, &token);
		if (b)
			{
			if (b->CanReadBlockType(token))
				{
				if (sourceOfBlock)
					*sourceOfBlock = *fIt;
				b->SetNexus(this);
				return b;
				}
			(*fIt)->BlockError(b);
			}
		}
	return NULL;
	}

/*!
	Reads the NxsReader data file from the input stream provided by `token'. This function is responsible for reading
	through the name of a each block. Once it has read a block name, it searches for a block reader to
	handle reading the remainder of the block's contents.

	The block object's Read() method is responsible for reading the END or ENDBLOCK command as well as the trailing semicolon.

	Execute() handles reading comments that are outside of blocks, as well as the initial "#NEXUS" keyword.

	As discussed in \ref NexusErrors exceptions raised during parsing result in calls to ::NexusError()

	If `notifyStartStop' is false then ExecuteStart and ExecuteStop functions will not to be called.

	The order of operations is:
		-# Read until next Begin command.
		-# Search through the registered block instances to find one that returns
			true from NxsBlock::CanReadBlockType()
		-# If none is found then CreateBlockFromFactories is called.
		-# If no appropriate block reader has been created, then SkippingBlock hook will
			be called (and the NxsReader will call ReadUntilEndblock to read until
			the END of the block before returning to step 1.
		-# If an appropriate block reader was found in steps 2 or 3 then ExecuteBlock() will be called.

*/
void NxsReader::Execute(
  NxsToken	&token,				/*!< the token object used to grab NxsReader tokens */
  bool		notifyStartStop)	/*!< if true, ExecuteStarting and ExecuteStopping will be called */
	{
	bool signalHandlerInstalled = false;
	unsigned numSigInts = 0;
	if (NxsReader::nclCatchesSignals)
		{
		numSigInts = getNumSignalIntsCaught();
		installNCLSignalHandler();
		signalHandlerInstalled = true;
		}
	try {
		CoreExecutionTasks(token, notifyStartStop);
		}
	catch (...)
		{
		if (signalHandlerInstalled)
			uninstallNCLSignalHandler();
		throw;
		}
	if (signalHandlerInstalled)
		{
		uninstallNCLSignalHandler();
		if (numSigInts != getNumSignalIntsCaught())
			throw NxsSignalCanceledParseException("Reading NEXUS content");
		}
	}

/*! used internally to  do most of the work of Execute() */
void NxsReader::CoreExecutionTasks(
  NxsToken	&token,				/* the token object used to grab NxsReader tokens */
  bool		notifyStartStop)	/* if true, ExecuteStarting and ExecuteStopping will be called */
	{
	unsigned numSigInts = NxsReader::getNumSignalIntsCaught();
	const bool checkingSignals = NxsReader::getNCLCatchesSignals();

	lastExecuteBlocksInOrder.clear();
	currBlock = NULL;

	NxsString errormsg;
	token.SetEOFAllowed(true);

	try
		{
		token.SetLabileFlagBit(NxsToken::saveCommandComments);
		token.GetNextToken();
		}
	catch (NxsException x)
		{
		NexusError(token.errormsg, 0, 0, 0);
		return;
		}

	if (token.Equals("#NEXUS"))
		{
		token.SetLabileFlagBit(NxsToken::saveCommandComments);
		token.GetNextToken();
		}
	else
		{
		errormsg = "Expecting #NEXUS to be the first token in the file, but found ";
		errormsg += token.GetToken();
		errormsg += " instead";
		/*mth changed this to a warning instead of an error	 because of the large number
			of files that violate this requirement.
		*/
		NexusWarn(errormsg,  NxsReader::AMBIGUOUS_CONTENT_WARNING, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
		}

	if (notifyStartStop)
		ExecuteStarting();
	bool keepReading = true;
	for (;keepReading && !token.AtEOF();)
		{
		if (checkingSignals && NxsReader::getNumSignalIntsCaught() != numSigInts)
			{
			throw NxsSignalCanceledParseException("Reading NEXUS content");
			}
		if (token.Equals("BEGIN"))
			{
			token.SetEOFAllowed(false); /*must exit the block before and EOF*/
			token.GetNextToken();
			token.SetBlockName(token.GetTokenReference().c_str());
			for (currBlock = blockList; currBlock != NULL; currBlock = currBlock->next)
				{
				if (currBlock->CanReadBlockType(token))
					break;
				}
			NxsString currBlockName = token.GetToken();
			currBlockName.ToUpper();
			NxsBlockFactory * sourceOfBlock = NULL;
			if (currBlock == NULL)
				{
				try
					{
					currBlock = CreateBlockFromFactories(currBlockName, token, &sourceOfBlock);
					}
				catch (NxsException x)
					{
					NexusError(x.msg, x.pos, x.line, x.col);
					token.SetBlockName(0L);
					token.SetEOFAllowed(true);
					return;
					}
			    }
			if (currBlock == NULL)
				{
				SkippingBlock(currBlockName);
				if (!ReadUntilEndblock(token, currBlockName))
					{
					token.SetBlockName(0L);
					token.SetEOFAllowed(true);
					return;
					}
				}
			else if (currBlock->IsEnabled())
				keepReading = ExecuteBlock(token, currBlockName, currBlock, sourceOfBlock);
			else
				{
				SkippingDisabledBlock(token.GetToken());
				if (sourceOfBlock)
					sourceOfBlock->BlockSkipped(currBlock);
				if (!ReadUntilEndblock(token, currBlockName))
					{
					token.SetBlockName(0L);
					token.SetEOFAllowed(true);
					return;
					}
				}
			currBlock = NULL;
			token.SetEOFAllowed(true);
			token.SetBlockName(0L);
			}	// if (token.Equals("BEGIN"))
		else if (token.Equals("&SHOWALL"))
			{
			for (NxsBlock*	showBlock = blockList; showBlock != NULL; showBlock = showBlock->next)
				DebugReportBlock(*showBlock);
			}
		else if (token.Equals("&LEAVE"))
			break;
		if (keepReading)
			{
			token.SetLabileFlagBit(NxsToken::saveCommandComments);
			token.GetNextToken();
			}
		}
	if (notifyStartStop)
		ExecuteStopping();

	currBlock = NULL;
	}

void NxsReader::ClearContent()
	{
	for (currBlock = blockList; currBlock;)
		{
		currBlock->Reset();
		currBlock = currBlock->next;
		}
	currBlock = blockList;
	blocksInOrder.clear();
	blockPriorities.clear();
	lastExecuteBlocksInOrder.clear();
	blockTypeToBlockList.clear();
	blockTitleHistoryMap.clear();
	blockTitleAliases.clear();
	}


/*! \returns a pointer to a previously process  NxsTaxaBlock with the same taxon
	labels. The comparison of labels is case-insensitive and not affected by the
	ordering of taxa within the block.

	TAXA blocks are often repeated in sets of NEXUS files (because a bare TREES block
	constitutes an illegal NEXUS file, and because NCL spawns implied Taxa blocks
	if it reads just a Trees block).

	If NxsReader::cullIdenticalTaxaBlocks(true) has been called then NxsReader::GetOriginalTaxaBlock
	will be called as part of determining whether or not a taxa block should be deleted.

	\warning: this is a hole in the const-correctness because the caller could (but shouldn't
		modify the Taxa block).
*/
NxsTaxaBlockAPI * NxsReader::GetOriginalTaxaBlock(const NxsTaxaBlockAPI * testB) const
	{
	if (!testB)
		return 0L;
	const std::string idstring("TAXA");
	BlockTypeToBlockList::const_iterator bttblIt = blockTypeToBlockList.find(idstring);
	if (bttblIt == blockTypeToBlockList.end())
		return 0L;
	const BlockReaderList & brl = bttblIt->second;
	const unsigned ntt = testB->GetNumTaxonLabels();
	const std::vector<std::string> testL = testB->GetAllLabels();
	for (BlockReaderList::const_iterator bIt = brl.begin(); bIt != brl.end(); ++bIt)
		{
		const NxsBlock * nb = *bIt;
		const NxsTaxaBlockAPI * prev = (const NxsTaxaBlockAPI *) nb;
		if (prev->GetNumTaxonLabels() == ntt)
			{
			const std::vector<std::string> prevL = prev->GetAllLabels();
			std::vector<std::string>::const_iterator pIt = prevL.begin();
			std::vector<std::string>::const_iterator testIt = testL.begin();

			for (; (testIt != testL.end()) && (pIt != prevL.end()) ; ++testIt, ++pIt)
				{
				if (!NxsString::case_insensitive_equals(testIt->c_str(), pIt->c_str()))
					break;
				}
			if (testIt == testL.end())
				return const_cast<NxsTaxaBlockAPI *>(prev);
			}
		}

	return 0L;
	}


/*! Called internally when the NxsReader has found the correct NxsBlock to read
	a block in a file.

	`token` will be at the block ID.
	`currBlockName` will be the block ID as a string.
	`currentBlock` will be the block reader to be used
	`sourceOfBlock` is the factory  that created the block (or 0L). If sourceOfBlock
		is not NULL then it will be alerted if the block is skipped (BlockSkipped() method)
		or there was an error in the read (BlockError() method). The factory is expected
		to delete the block instances in these cases (NxsReader will not refer to those
		instances again).



	The following steps occur:
		- the EnteringBlock hook is called (if it returns false, the block will be skipped by calling
			NxsReader::SkippingBlock
		- NxsBlock::Reset() is called on the reader block
		- NxsBlock::Read() method of the reader block is called
		- If an exception is generated, the NexusError is called.
		- If no exception is generated by Read then the block is processed:
			- if NxsReader::cullIdenticalTaxaBlocks(true) has been called before Execute and this
				is a repeated TAXA block, the block will be deleted.
			- the BlockReadHook() will store all of the implied blocks
				(by calling NxsBlock::GetImpliedBlocks()) and the block itself.
			- if one of the implied blocks is a repeated TAXA block and
				NxsReader::cullIdenticalTaxaBlocks(true) has been called, then
				the blocks NxsBlock::SwapEquivalentTaxaBlock() method will determine
				whether or not the duplicate taxa block can be deleted.
			- each stored block will generate a call to NxsReader::AddBlockToUsedBlockList()
		- ExitingBlock() is called
		- PostBlockReadingHook() is called
*/
bool NxsReader::ExecuteBlock(NxsToken &token, const NxsString &currBlockName, NxsBlock *currentBlock, NxsBlockFactory * sourceOfBlock)
	{
	if (!EnteringBlock(currBlockName))
		{
		SkippingBlock(currBlockName);
		if (sourceOfBlock)
			sourceOfBlock->BlockSkipped(currentBlock);
		if (!ReadUntilEndblock(token, currBlockName))
			{
			token.SetBlockName(0L);
			token.SetEOFAllowed(true);
			return false;
			}
		return true;
		}
	this->RemoveBlockFromUsedBlockList(currentBlock);
	currentBlock->Reset();
	// We need to back up currentBlock, because the Read statement might trigger
	// a recursive call to Execute (if the block contains instructions to execute
	// another file, then the same NxsReader object may be used and any member fields (e.g. currentBlock)
	//	could be trashed.
	//
	bool eofFound = false;
	try
		{
		try
			{
			currentBlock->Read(token);
			}
		catch (NxsX_UnexpectedEOF &eofx)
			{
			if (!currentBlock->TolerateEOFInBlock())
				throw eofx;
			NxsString m;
			m << "Unexpected End of file in " << currBlockName << "block";
			currentBlock->WarnDangerousContent(m, token);
			eofFound = true;
			}
		if (destroyRepeatedTaxaBlocks && currBlockName.EqualsCaseInsensitive("TAXA"))
			{
			NxsTaxaBlockAPI * oldTB = this->GetOriginalTaxaBlock((NxsTaxaBlockAPI *) currentBlock);
			if (oldTB)
				{
				const std::string altTitle = currentBlock->GetTitle();
				this->RegisterAltTitle(oldTB, altTitle);
				if (sourceOfBlock)
					sourceOfBlock->BlockError(currentBlock);
				return true;
				}
			}
		BlockReadHook(currBlockName, currentBlock, &token);
		}
	catch (NxsException &x)
		{
		NxsString m;
		if (currentBlock->errormsg.length() > 0)
			m = currentBlock->errormsg;
		else
			m = x.msg;
		currentBlock->Reset();
		if (sourceOfBlock != 0)
			{

			sourceOfBlock->BlockError(currentBlock);
			}
		else

		token.SetBlockName(0L);
		token.SetEOFAllowed(true);
		currentBlock = NULL;
		NexusError(m, x.pos, x.line, x.col);
		return false;
		}	// catch (NxsException x)
	ExitingBlock(currBlockName);
	PostBlockReadingHook(*currentBlock);
	return !eofFound;
	}

/*! Called by NxsReader::ExecuteBlock() to store the block and its implied blocks \ref NxsReader::ExecuteBlock()*/
void NxsReader::BlockReadHook(const NxsString &currBlockName, NxsBlock *currentBlock, NxsToken * token)
	{
	VecBlockPtr implied = currentBlock->GetImpliedBlocks();
	for (VecBlockPtr::iterator impIt = implied.begin(); impIt != implied.end(); ++impIt)
		{
		NxsBlock * nb = *impIt;
		NCL_ASSERT(nb);
		NxsString impID = nb->GetID();
		bool storeBlock = true;
		if (destroyRepeatedTaxaBlocks && impID.EqualsCaseInsensitive("TAXA"))
			{
			NxsTaxaBlockAPI * oldTB = this->GetOriginalTaxaBlock((NxsTaxaBlockAPI *) nb);
			if (oldTB)
				{
				storeBlock = ! currentBlock->SwapEquivalentTaxaBlock(oldTB);
				const std::string altTitle = nb->GetTitle();
				this->RegisterAltTitle(oldTB, altTitle);
				if (!storeBlock)
					{
					delete nb;
					}

				}
			}
		if (storeBlock)
			{
			NxsString m;
			m << "storing implied block: " << impID;
			this->statusMessage(m);
			this->AddBlockToUsedBlockList(impID, nb, token);
			}
		}
	NxsString s;
	s << "storing read block: " << currentBlock->GetID();
	this->statusMessage(s);
	this->AddBlockToUsedBlockList(currBlockName, currentBlock, token);
	}

/*!
	Returns a string containing the copyright notice for the NxsReader Class Library, useful for reporting the use of
	this library by programs that interact with the user.
*/
const char * NxsReader::NCLCopyrightNotice()
	{
	return NCL_COPYRIGHT;
	}

/*!
	Returns a string containing the URL for the NxsReader Class Library internet home page.
*/
const char * NxsReader::NCLHomePageURL()
	{
	return NCL_HOMEPAGEURL;
	}

/*!
	Returns a string containing the name and current version of the NxsReader Class Library, useful for reporting the
	use of this library by programs that interact with the user.
*/
const char * NxsReader::NCLNameAndVersion()
	{
	return NCL_NAME_AND_VERSION;
	}

/*!
	Called just after Execute member function reads the opening "#NEXUS" token in a NEXUS data file. Override this
	virtual base class function if your application needs to do anything at this point in the execution of a NEXUS data
	file (e.g. good opportunity to pop up a dialog box showing progress). Be sure to call the Execute function with the
	`notifyStartStop' argument set to true, otherwise ExecuteStarting will not be called.

*/
void NxsReader::ExecuteStarting()
	{
	}

/*!
	Called when Execute member function encounters the end of the NEXUS data file, or the special comment [&LEAVE] is
	found between NEXUS blocks. Override this virtual base class function if your application needs to do anything at
	this point in the execution of a NEXUS data file (e.g. good opportunity to hide or destroy a dialog box showing
	progress). Be sure to call the Execute function with the `notifyStartStop' argument set to true, otherwise
	ExecuteStopping will not be called.
*/
void NxsReader::ExecuteStopping()
	{
	}

/*!
	Called when an error is encountered in a NEXUS file. Allows program to give user details of the error as well as
	the precise location of the error.
*/
void NxsReader::NexusError(
  NxsString ,	/* the error message to be displayed */
  file_pos	,	/* the current file position */
  long	,	/* the current file line */
  long	)	/* the current column within the current file line */
	{
	}

/*!
	This function may be used to report progess while reading through a file. For example, the NxsAllelesBlock class
	uses this function to report the name of the population it is currently reading so the user doesn't think the
	program has hung on large data sets.
*/
void NxsReader::OutputComment(
  const NxsString &)	/* a comment to be shown on the output */
	{
	}

/*!
	This function is called when an unknown block named `blockName' is about to be skipped. Override this pure virtual
	function to provide an indication of progress as the NEXUS file is being read.
*/
void NxsReader::SkippingBlock(
  NxsString )	/* the name of the block being skipped */
	{
	}

/*!
	This function is called when a disabled block named `blockName' is encountered in a NEXUS data file being executed.
	Override this pure virtual function to handle this event in an appropriate manner. For example, the program may
	wish to inform the user that a data block was encountered in what is supposed to be a tree file.
*/
void NxsReader::SkippingDisabledBlock(
  NxsString )	/* the name of the disabled block being skipped */
	{
	}


/*! Used internally to skip until teh END; or ENDBLOCK; command. */
bool NxsReader::ReadUntilEndblock(NxsToken &token, const std::string & )
	{
	for (;;)
		{
		token.GetNextToken();
		if (token.Equals("END") || token.Equals("ENDBLOCK"))
			{
			token.GetNextToken();
			if (!token.Equals(";"))
				{
				std::string errormsg = "Expecting ';' after END or ENDBLOCK command, but found ";
				errormsg += token.GetToken();
				errormsg += " instead";
				NexusError(NxsString(errormsg.c_str()), token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
				return false;
				}
			return true;
			}
		else
			token.ProcessAsCommand(NULL);
		}
	}

/*! Convenience function for setting the NxsTaxaBlockFactory */
void NxsReader::SetTaxaBlockFactory(NxsTaxaBlockFactory *f)
	{
	if (this->taxaBlockFactory)
		this->RemoveFactory(this->taxaBlockFactory);
	this->taxaBlockFactory = f;
	if (taxaBlockFactory)
		this->AddFactory(this->taxaBlockFactory);
	}
/*! \returns the last TAXA block.
	\warning: This should only called when the client knows that the TAXA block
	inherits from NxsTaxaBlockAPI (static_cast is used). This will be true if the
	client code has not derived its own NxsBlock for reading TAXA blocks
*/
NxsTaxaBlockAPI *NxsReader::GetLastStoredTaxaBlock()
	{
	const std::string idstring("TAXA");
	NxsBlock * nb = GetLastStoredBlockByID(idstring);
	return static_cast<NxsTaxaBlockAPI *>(nb); //dynamic_cast<NxsTaxaBlockAPI *>(nb);
	}

/*! \returns the last CHARACTERS/DATA block.
	\warning: This should only called when the client knows that the TAXA block
	inherits from NxsTaxaBlockAPI (static_cast is used). This will be true if the
	client code has not derived its own NxsBlock for reading TAXA blocks
*/
NxsCharactersBlockAPI *NxsReader::GetLastStoredCharactersBlock()
	{
	const std::string idstring("CHARACTERS");
	NxsBlock * nb = GetLastStoredBlockByID(idstring);
	return static_cast<NxsCharactersBlockAPI *>(nb); //dynamic_cast<NxsCharactersBlockAPI *>(nb);
	}

/*! \returns the last TREES block.
	\warning: This should only called when the client knows that the TAXA block
	inherits from NxsTaxaBlockAPI (static_cast is used). This will be true if the
	client code has not derived its own NxsBlock for reading TAXA blocks
*/
NxsTreesBlockAPI *NxsReader::GetLastStoredTreesBlock()
	{
	const std::string idstring("TREES");
	NxsBlock * nb = GetLastStoredBlockByID(idstring);
	return static_cast<NxsTreesBlockAPI *>(nb); //dynamic_cast<NxsTreesBlockAPI *>(nb);
	}

/*! \returns the last block with block ID ("TAXA", "DATA"...) indicated by key
*/

NxsBlock *NxsReader::GetLastStoredBlockByID(const std::string &key)
	{
	BlockTypeToBlockList::iterator bttblIt = blockTypeToBlockList.find(key);
	if (bttblIt == blockTypeToBlockList.end())
		return NULL;
	return bttblIt->second.back();
	}

/*! Used internally. Called by AddBlockToUsedBlockList() this function will generate
	a NxsException if the block's title is found in another block of the same block ID type (TAXA, CHARACTERS, ...)

	If the block has no title, an automatically generated title will be supplied with the form
		Untitled <block type ID> Block #
*/
void NxsReader::NewBlockTitleCheckHook(const std::string &blockname, NxsBlock *p, NxsToken *token)
	{
	NxsBlockTitleHistoryMap::iterator mIt = blockTitleHistoryMap.find(blockname);
	if (mIt == blockTitleHistoryMap.end())
		{
		std::list<std::string> mt;
		blockTitleHistoryMap[blockname] = NxsBlockTitleHistory(1, mt);
		mIt = blockTitleHistoryMap.find(blockname);
		NCL_ASSERT(mIt != blockTitleHistoryMap.end());
		}
	NxsBlockTitleHistory & titleHist = mIt->second;
	unsigned n = titleHist.first;
	std::list<std::string> & previousTitles = titleHist.second;
	std::list<std::string>::iterator lsIt;
	std::string pTitle = p->GetTitle();
	std::string origTitle = pTitle;
	NxsString::to_upper(pTitle);
	if (pTitle.empty())
		{
		while (pTitle.empty())
			{
			NxsString autoName = "Untitled ";
			autoName += p->GetID().c_str();
			autoName += " Block ";
			autoName += n++;
			pTitle.assign(autoName.c_str());
			NxsString::to_upper(pTitle);
            lsIt = find(previousTitles.begin(), previousTitles.end(), pTitle);
			if (lsIt == previousTitles.end())
				{
				p->SetTitle(autoName, true);
				titleHist.first = n;
				}
			else
				pTitle.clear();
			}
		}
	else
		{
        lsIt = find(previousTitles.begin(), previousTitles.end(), pTitle);
		if (lsIt != previousTitles.end())
			{
			NxsString msg = "Block titles cannot be repeated. The TITLE ";
			msg += origTitle;
			msg += " has already been used for a ";
			msg += blockname;
			msg += " block.";
			if (token)
				throw NxsException(msg, *token);
			else
				throw NxsException(msg, 0, -1, -1);
			}
		}
	previousTitles.push_back(pTitle);
	}

/*! Used internally to store the correctly read block `p`
	`token` is the token that is being parsed (or 0L).

	\warning This can generate NxsExceptions if there are clashes in the block title
*/
void NxsReader::AddBlockToUsedBlockList(const std::string &blockTypeID, NxsBlock *p, NxsToken *token)
	{
	NCL_ASSERT(p);
	std::string n;
	if (blockTypeID == "DATA")
		n = std::string("CHARACTERS");
	else
		n = blockTypeID;
	NewBlockTitleCheckHook(n, p, token);
	BlockTypeToBlockList::iterator bttblIt = blockTypeToBlockList.find(n);
	if (bttblIt == blockTypeToBlockList.end())
		blockTypeToBlockList[n] = BlockReaderList(1, p);
	else
		bttblIt->second.push_back(p);
	blocksInOrder.remove(p);
	blocksInOrder.push_back(p);
	if (this->GetBlockPriority(p) < 0)
		AssignBlockPriority(p, 0);

	lastExecuteBlocksInOrder.remove(p);
	lastExecuteBlocksInOrder.push_back(p);
	}

/*! Removes a block from the NxsReader's records. Does NOT delete the block!
	\returns the number of times the block was in the reader's block lists (usually
		either 0 or 1).
*/
unsigned NxsReader::RemoveBlockFromUsedBlockList(NxsBlock *p)
	{
	unsigned totalDel = 0;
	unsigned before, after;
	std::vector<std::string> keysToDel;
	for (BlockTypeToBlockList::iterator bttblIt = blockTypeToBlockList.begin(); bttblIt != blockTypeToBlockList.end(); ++bttblIt)
		{
		BlockReaderList & brl = bttblIt->second;
		before = (unsigned)brl.size();
		brl.remove(p);
		after = (unsigned)brl.size();
		if (after == 0)
			keysToDel.push_back(bttblIt->first);
		totalDel += before - after;
		}
	for (std::vector<std::string>::const_iterator keyIt = keysToDel.begin(); keyIt != keysToDel.end(); ++keyIt)
		blockTypeToBlockList.erase(*keyIt);
	blocksInOrder.remove(p);
	blockPriorities.erase(p);
	lastExecuteBlocksInOrder.remove(p);
	std::string blockID =  p->GetID();
	NxsBlockTitleHistoryMap::iterator mIt = blockTitleHistoryMap.find(blockID);
	if (mIt != blockTitleHistoryMap.end())
		{
		std::string blockName = p->GetTitle();
		NxsBlockTitleHistory & titleHist = mIt->second;
		std::list<std::string> & previousTitles = titleHist.second;
		std::list<std::string>::iterator ptIt = previousTitles.begin();
		while (ptIt != previousTitles.end())
			{
			if (NxsString::case_insensitive_equals(ptIt->c_str(), blockName.c_str()))
				ptIt = previousTitles.erase(ptIt);
			else
				 ++ptIt;
			}
		}
	return totalDel;
	}

/*! Returns a set of all of the blocks that have been successfully read.
*/
std::set<NxsBlock*> NxsReader::GetSetOfAllUsedBlocks()
	{
	std::set<NxsBlock*> s;
	for (BlockTypeToBlockList::iterator bttblIt = blockTypeToBlockList.begin(); bttblIt != blockTypeToBlockList.end(); ++bttblIt)
		{
		BlockReaderList & brl = bttblIt->second;
		s.insert(brl.begin(), brl.end());
		}
	return s;
	}

void ExceptionRaisingNxsReader::NexusWarn(const std::string &msg, NxsWarnLevel warnLevel, file_pos pos, long line, long col)
	{
	if (warnLevel < currentWarningLevel)
		return;
	if (warnLevel >= this->warningToErrorThreshold)
		{
		NxsString e(msg.c_str());
		throw NxsException(e, pos, line, col);
		}

	if (warnMode == NxsReader::IGNORE_WARNINGS)
		return;
	if (warnMode == NxsReader::WARNINGS_TO_STDERR)
		{
		std::cerr << "\nWarning:  ";
		std::cerr << "\n " << msg << std::endl;
		if (line > 0 || pos > 0)
			std::cerr << "at line " << line << ", column (approximately) " << col << " (file position " << pos << ")\n";
		}
	else if (warnMode != NxsReader::WARNINGS_TO_STDOUT)
		{
		std::cout << "\nWarning:  ";
		if (line > 0 || pos > 0)
			std::cout << "at line " << line << ", column " << col << " (file position " << pos << "):\n";
		std::cout << "\n " << msg << '\n';
		if (line > 0 || pos > 0)
			std::cout << "at line " << line << ", column (approximately) " << col << " (file position " << pos << ')' << std::endl;
		}
	else
		{
		NxsString m("WARNING:\n ");
		m += msg.c_str();
		NexusError(m, pos, line, col);
		}
	}

void ExceptionRaisingNxsReader::SkippingBlock(NxsString blockName)
	{
	if (warnMode == NxsReader::IGNORE_WARNINGS)
		return;
	if (warnMode == NxsReader::WARNINGS_TO_STDERR)
		std::cerr << "[!Skipping unknown block (" << blockName << ")...]" << std::endl;
	else if (warnMode != NxsReader::WARNINGS_TO_STDOUT)
		std::cout << "[!Skipping unknown block (" << blockName << ")...]" << std::endl;
	}

void ExceptionRaisingNxsReader::SkippingDisabledBlock(NxsString blockName)
	{
	if (warnMode == NxsReader::IGNORE_WARNINGS)
		return;
	if (warnMode == NxsReader::WARNINGS_TO_STDERR)
		std::cerr << "[!Skipping disabled block (" << blockName << ")...]" << std::endl;
	else if (warnMode != NxsReader::WARNINGS_TO_STDOUT)
		std::cout << "[!Skipping disabled block (" << blockName << ")...]" << std::endl;
	}

void NxsReader::statusMessage(const std::string & m) const
{
	if (alwaysReportStatusMessages || currentWarningLevel == UNCOMMON_SYNTAX_WARNING) {
		std::cerr << m << std::endl;
	}
}

/*! Clears the lists of all of the blocks that have been read.
	NOTE: does NOT free any memory or call Reset() on any blocks"!!

	This call can be used to "tell" a reader instance that you have taken
	over the memory management for all of the blocks that it has read (or created).
*/
void NxsReader::ClearUsedBlockList()
	{
	RemoveBlocksFromFactoriesFromUsedBlockLists();
	blocksInOrder.clear();
	blockPriorities.clear();
	lastExecuteBlocksInOrder.clear();
	blockTypeToBlockList.clear();
	}

void NxsReader::AssignBlockPriority(NxsBlock *b, int priorityLevel)
	{
	blockPriorities[b] = priorityLevel;
	}

int	NxsReader::GetBlockPriority(NxsBlock *b) const
	{
	std::map<NxsBlock *, int>::const_iterator bIt = blockPriorities.find(b);
	if (bIt == blockPriorities.end())
		return 0;
	return bIt->second;
	}

void NxsReader::DemoteBlocks(int priorityLevel)
	{
	BlockReaderList brl = GetUsedBlocksInOrder();
	BlockReaderList::iterator brlIt = brl.begin();
	for (; brlIt != brl.end(); ++brlIt)
		{
		NxsBlock * b = *brlIt;
		AssignBlockPriority(b, priorityLevel);
		}
	}



