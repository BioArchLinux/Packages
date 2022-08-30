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

#ifndef NCL_NXSREADER_H
#define NCL_NXSREADER_H

#include "ncl/nxsdefs.h"
#include "ncl/nxsstring.h"
#include "ncl/nxsexception.h"
#include "ncl/nxstoken.h"

class NxsBlock;
class NxsBlockFactory;
class NxsTaxaBlockFactory;
class NxsAssumptionsBlockAPI;
class NxsCharactersBlockAPI;
class NxsTaxaBlockAPI;
class NxsTreesBlockAPI;

typedef std::list<NxsBlock *> BlockReaderList;
typedef std::map<std::string, BlockReaderList> BlockTypeToBlockList;


/*!
	This is the class that orchestrates the reading of a NEXUS data file, and so is the central class to NCL.

	NxsReader does not call delete on any of the blocks that are added to it via the Add method.


	In the "classic" (v2.0) NCL API:
		-# An NxsReader is created.
		-# pointers to instances of NxsBlocks that are expected to be needed should be added to `blockList' using the
			NxsReader::Add() member function.
		-# NxsReader::Execute() is then called, which reads the data file until encountering a block name, at which
			point the correct block is looked up in `blockList' and that object's NxsBlock::Read() method is called.
		-# NxsReader::PostBlockReadingHook(NxsBlock) is called after a block is successfully read.  This allows one to gather
			the parsed data from the NxsBlock.  If another block of the same type is encountered, then NxsBlock::Reset()
			will be called and the same NxsBlock instance will be used to read the next block.
		.
	Versions of NCL after 2.0.04 also support a "factory API" augments the former behavior:
		-# An NxsReader is created.
		-# In addition to NxsBlocks added using the Add method, you can register instances of a NxsBlockFactory using
			the NxsReader::AddFactory() method.
		-# In the NxsReader::Execute() method, if an appropriate block is not found in the `blockList` then the
			the factories are asked to create a block for the current block name.  The first non-NULL block pointer
			returned is used.
		-#  PostBlockReadingHook is still called, but blocks created by a factory will not be "recycled" later in the
			NxsReader::Execute(), so it is not necessary to pull all of the data out of them.
		-#If a block created by a factory is skipped or has an error, then the factory will be notified using
			NxsBlockFactory::BlockError(NxsBlock *) or NxsBlockFactory::BlockSkipped(NxsBlock *).  In the event of
			skipping or an error, NxsReader will never refer to that instance of the factory-created block again.
			Hence the base class behavior of BlockError() and BlockSkipped() is to delete the instance.
		-#	Every time a NxsBlock successfully reads a NEXUS block, the NxsBlock is added to container of "used" blocks.
			When a block is Reset() it is removed from this container.  NxsReader::GetUsedBlocks() can be called at any
			point to get a copy of this container (which maps a block type name to a list of NxsBlock*). This container
			thus stores the state of the parsed NEXUS file.  If no blocks were recycled (if all of the blocks came from
			factories rather than blocks added using NxsReader::Add() method), then the GetUsedBlocks will contain binary
			representation of every block parsed.
		.

	Important: The use of the factories that are supplied with NCL can trigger casts of pointers. This can be unsafe if
		you create NxsBlocks that do not have the expected inheritance.  For example, if you create a class to
		read Taxa blocks, but do NOT derive this class from NxsTaxaBlockAPI then the casts will be unsafe.  If you
		do this, and you wish to use the factory API then you must write your own factories.

	See NCL_TOP/examples/normalizer examples for an example of the factory API (using the MultiFormatReader).  In those
		examples the PublicNexusReader::PublicNexusReader() constructor is the function that installs the templates for
		a clone factory.

	\section NexusErrors Errors in NEXUS files
	When an illegal construct is found, a NxsException is raised. This exception is caught within NxsReader::Execute
		and NxsReader::NexusError is called. This allows subclasses of NxsReader to handle exceptional circumstances by
		overriding one function. Whenever you are using a NxsReader instance that is NOT a subclass of ExceptionRaisingNxsReader
		this is still the behavior (thus any code that was written to the v2.0 API will still have this behavior).

	ExceptionRaisingNxsReader implements NexusError by raising another NxsException.  This results in the exception
		being propagated to the caller.  The newer NxsReader classes (including PublicNexusReader and MultiFormatReader)
		are derived from ExceptionRaisingNxsReader. So their Execute methods will also raise NxsExceptions. Deriving a subclass
		of these classes and overriding NexusError would prevent this behavior.

	The advantage of ExceptionRaisingNxsReader is that one is no longer required to subclass NxsReader to handle errors.

	\section signalsection Signal Handling in NCL:
		Traditionally, the user of an application can send an SIGINT to cause it to stop. NCL has very limited support
	for handling signals, and this support is turned off by default.

		If you want NCL to raise an NxsSignalCanceledParseException if a signal is encountered during a parse then call:
			NxsReader::setNCLCatchesSignals(true);
	before calling Execute on your NxsReader instance. Note that only the slowly-parsed blocks (TREES and CHARACTERS) and
	the NxsReader currently check to see if a signal has been caught.  So the NxsSignalCanceledParseException will often
	have a generic message indicating that the signal was caught during the parse.

		The NCL signal handler is only installed during NxsReader::Execute calls!

		Note: that if you want your program to exit on SIGINT, you can leave the signal handling turned off. If you do turn
			NCL's signal handling on, then after you do your apps clean up you'll have to exit by something like this:
				signal(SIGINT, SIG_DFL);
				kill(getpid(), SIGINT);
			\see http://www.cons.org/cracauer/sigint.html

*/
class NxsReader
	{
		static void installNCLSignalHandler();
		static void uninstallNCLSignalHandler();
	public:
		/** Enum of arguments for ExceptionRaisingNxsReader ctor */
		enum WarningHandlingMode
			{
			IGNORE_WARNINGS,  /**< warnings that are not error-generating are ignored silently */
			WARNINGS_TO_STDERR, /**< warnings that are not error-generating are written to standard error stream */
			WARNINGS_TO_STDOUT, /**< warnings that are not error-generating are written to standard output stream */
			WARNINGS_ARE_ERRORS /**< warnings that are not error-generating by some other mechanism are still converted to NxsException objects */
			};
		/** Enum different levels of warnings.  See NxsReader::SetWarningOutputLevel*/
		enum NxsWarnLevel
			{ //TODO: we need another warning level for status messages.
			UNCOMMON_SYNTAX_WARNING = 0,  /**< Legal but uncommon syntax that could indicate a typo */
			SKIPPING_CONTENT_WARNING = 1, /**< Content is being skipped by NCL */
			OVERWRITING_CONTENT_WARNING = 2, /**< New content is replacing old information (eg. CharSets with the same name as a previously defined CharSet)*/
			DEPRECATED_WARNING = 3, /**< Use of a deprecated feature */
			AMBIGUOUS_CONTENT_WARNING = 4, /**< commands that could have multiple plausible interpretations */
			ILLEGAL_CONTENT_WARNING = 5, /**< content that violates NEXUS rules, but is still parseable (eg. CharPartitions that only have some of the characters)*/
			PROBABLY_INCORRECT_CONTENT_WARNING = 6, /**< Severe Warning that is generated when the file contains characters that should almost certainly be removed */
			FATAL_WARNING = 7, /**< a higher warning level then any of the warning generated by NCL.  Primarily used in constructs such as (FATAL_WARNING - 1) to mean only the most severe warnings.*/
			SUPPRESS_WARNINGS_LEVEL = 8 /**<  if the NxsReader's warning level is set to this, then warnings will be suppressed */
			};

		/*! If true then NCL will call a handler function if signals are encountered during NxsReader::Execute
			(signal handling is off by default).
			\sa The section on signal handling \ref signalsection
		*/
		static void setNCLCatchesSignals(bool);
		/*! \returns true if NCL will call a handler function if signals are encountered during NxsReader::Execute
					(signal handling is off by default).
			\sa The section on signal handling \ref signalsection
		*/
		static bool getNCLCatchesSignals();
		/*! Usually used internally when signal catching is enabled. If the number of sigints has changed, since the last
			call, then NCL has detected a signal.
			\sa The section on signal handling \ref signalsection
		*/
		static unsigned getNumSignalIntsCaught();
		/*! Used internally.  If NCL is handling SIGINTs then this will be incremented with every SIGINT.
			\sa The section on signal handling \ref signalsection
		*/
		static void setNumSignalsIntsCaught(unsigned);


						NxsReader();
		virtual			~NxsReader();


		// functions to add and remove Block reader objects or factories for block readers.
		virtual void    Add(NxsBlock *newBlock);
		void			Detach(NxsBlock *newBlock);
		virtual void    AddFactory(NxsBlockFactory *);
		void			RemoveFactory(NxsBlockFactory *);

		// trigger for NEXUS parsing.
		virtual void	Execute(NxsToken& token, bool notifyStartStop = true);

		// shortcuts for calling execute...
		void			ReadFilepath(const char *filename);
		void			ReadFilestream(std::istream & inf);
		void			ReadStringAsNexusContent(const std::string & s);

		virtual void	DebugReportBlock(NxsBlock &nexusBlock);

		const char		*NCLNameAndVersion();
		const char		*NCLCopyrightNotice();
		const char		*NCLHomePageURL();

		// hooks for subclasses to specialize the behavior
		virtual void	ExecuteStarting();
		virtual void	ExecuteStopping();
		virtual bool	EnteringBlock(NxsString blockName);
		virtual void	ExitingBlock(NxsString blockName);
		virtual void	OutputComment(const NxsString &comment);
		virtual void	SkippingDisabledBlock(NxsString blockName);
		virtual void	SkippingBlock(NxsString blockName);

		/*! This hook is called by the block readers when they encounter content
			that is interpretable but is questionable (or even illegal according
			to the NEXUS standard).

			The default NexusWarn behavior is to generate a NexusException for any
			warnLevel >= PROBABLY_INCORRECT_CONTENT_WARNING
		 	and to ignore all other warnings.
		*/
		virtual void	NexusWarn(const std::string &s, NxsWarnLevel warnLevel, file_pos pos, long line, long col)
			{
			if (warnLevel >= PROBABLY_INCORRECT_CONTENT_WARNING)
				{
				NxsString e(s.c_str());
				throw NxsException(e, pos, line, col);
				}
			}
		/*! Used internally as a more convenient way of calling NexusWarn */
		void	NexusWarnToken(const std::string &m, NxsWarnLevel warnLevel, const ProcessedNxsToken &token)
			{
			NexusWarn(m , warnLevel, token.GetFilePosition(), token.GetLineNumber(), token.GetColumnNumber());
			}
		/*! Used internally as a more convenient way of calling NexusWarn */
		void	NexusWarnToken(const std::string &m, NxsWarnLevel warnLevel, const NxsToken &token)
			{
			NexusWarn(m , warnLevel, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
			}

		/*! Called when a erron is detected */
		virtual void	NexusError(NxsString msg, file_pos pos, long line, long col);



		virtual void			ClearUsedBlockList();
		NxsBlock 			   *CreateBlockFromFactories(const std::string & currBlockName, NxsToken &token, NxsBlockFactory **sourceOfBlock = NULL);
		BlockReaderList 		GetUsedBlocksInOrder();
		BlockReaderList 		GetBlocksFromLastExecuteInOrder();
		BlockTypeToBlockList 	GetUsedBlocks();
		std::set<NxsBlock*> 	GetSetOfAllUsedBlocks();

		NxsTaxaBlockAPI 		*GetLastStoredTaxaBlock();
		NxsCharactersBlockAPI 	*GetLastStoredCharactersBlock();
		NxsTreesBlockAPI 		*GetLastStoredTreesBlock();


		NxsTaxaBlockAPI 		*GetTaxaBlockByTitle(const char *title, unsigned *nMatches);
		NxsCharactersBlockAPI 	*GetCharBlockByTitle(const char *title, unsigned *nMatches);
		NxsTreesBlockAPI 		*GetTreesBlockByTitle(const char *title, unsigned *nMatches);

		NxsTaxaBlockFactory 	*GetTaxaBlockFactory();
		//NxsCharactersBlockFactory 	*GetCharBlockFactory();
		//NxsTreesBlockFactory 	*GetTreesBlockFactory();

		void			SetTaxaBlockFactory(NxsTaxaBlockFactory *);
		//void			SetCharBlockFactory(NxsCharactersBlockFactory *);
		//void			SetTreesBlockFactory(NxsTreesBlockFactory *);

		virtual void    DeleteBlocksFromFactories();
		unsigned		RemoveBlockFromUsedBlockList(NxsBlock *);

		/*! throws away references to all blocks that that have been read. If the block
			 was registered with the reader, then "Reset" is called on the block.
				If the block came from a factory then the reference to the block is
					removed from the reader (resulting in a memory leak if the client
					code does not delete the block).
			 This can be called if the client would like to store the information
				from the NEXUS file, and get rid of the blocks to save memory (but
				still maintain things like factories that were registered with the
				NxsReader and tweaks to the default settings).
		*/
		virtual void	ClearContent();

		/*!
			 This function is useful after ClearContext() has been called.
			 Instances of blocks that have been read can be registered with reader
				so that future NEXUS files can be parsed with the context of those blocks.
			 For example one might want to reregister a NxsTaxaBlock before reading
				 a trees file, then you could call:
				reader->AddReadBlock("TAXA", taxaB);
			 NOTE: if you are using a PublicNexusReader, or MultiFormatReader
				you should almost certainly call the type-specific forms such as
				reader->AddReadTaxaBlock(taxaB);
		*/
		virtual void AddReadBlock(const NxsString &blockID, NxsBlock *block)
			{
			this->BlockReadHook(blockID, block);
			}

		/*! Call cullIdenticalTaxaBlocks(true) before reading a file if you want
				the reader to discard a TaxaBlock that is identical to a previous
				taxa block.  Use of this assumes that the reader of taxa blocks is
				a NxsTaxaBlockAPI instance.
			\warning: this function should only be called if you have registered
				a NxsTaxaBlockFactory.  The culling of blocks WILL CALL DELETE
				on them.
		*/
		void cullIdenticalTaxaBlocks(bool v=true)
			{
			this->destroyRepeatedTaxaBlocks = v;
			}
		std::vector<std::string> GetAllTitlesForBlock(const NxsBlock *b) const;


		/*! Passing true to this method is a hacky way to enable all status messages while still filtering
			NexusWarn messages.
		*/
		void SetAlwaysReportStatusMessages(bool v) {
			this->alwaysReportStatusMessages = v;
		}
		/*! The reader's currentWarningLevel is set here.
			By default this field is set to UNCOMMON_SYNTAX_WARNING.
			By setting it to any other facet of NxsWarnLevel you can reduce the
				number of warnings sent to std::cerr.
			The warning level is checked in ExceptionRaisingNxsReader::NexusWarn
				and NxsReader::statusMessage.
			If the level of the message is greater or equal to the readers level
				then the message will be sent to std::cerr
			By calling SetWarningOutputLevel(SUPPRESS_WARNINGS_LEVEL) you can
				make the reader ignore warnings.
			\note if alwaysReportStatusMessages then NxsReader::statusMessage
				will report all messages
		*/
		void SetWarningOutputLevel(NxsWarnLevel lev) {
			currentWarningLevel = lev;
		}
		NxsWarnLevel GetWarningOutputLevel() const {
			return currentWarningLevel;
		}

		/*! Messages about the status processing a file (such as "Executing...")
			are sent here. If the reader's currentWarningLevel is set to the lowest
			level (UNCOMMON_SYNTAX_WARNING) then these messages will show up in stderr.
		*/
		virtual void statusMessage(const std::string & m) const;

		/*! \deprecated This function is almost never needed.
			\returns if true no blocks have registered as readers (does not indicate
				whether content has been read, nor does it indicate if any
				factories have been added).
		*/
		bool			BlockListEmpty();
		unsigned		PositionInBlockList(NxsBlock *b);

		void			Reassign(NxsBlock *oldb, NxsBlock *newb);

		/*! The block `b` will be given a priority level of `priorityLevel`

			By default all blocks have priority level of 0. Lowering a blocks
				priority level below 0, means that a Get....ByTitle() function
				will not return the block if it finds a matching block that has
				a higher priority

			If you are re-using blocks (if you are not using the v2.1 API) be
				aware that if a block has a low (negative) priority, but is then
				used to read content then its priority will be bumped back up to 0.

			The block priority affects calls Get...ByTitle() functions and
				Find...ByTitle() functions.

			This function is mainly used to "demote" NxsTaxaBlock instances so that they
				will not clash with a TAXA block found in the file. \ref TaxaBlockClashes
		*/
		void AssignBlockPriority(NxsBlock *b, int priorityLevel);
		/*! Returns the block priority for a block (or 0 if the block instance is unknown) */
		int	GetBlockPriority(NxsBlock *b) const;
		/*! Lowers the priority of all read blocks to `priorityLevel` */
		void DemoteBlocks(int priorityLevel=-1);
	protected:
		/*! A convenience function to allow one to quickly get a list of block reader
			that were generated (or used) in reading a filepath */
		static 			BlockReaderList parseFileWithReader(NxsReader & reader, const char *filepath, bool parsePrivateBlocks=true, bool storeTokenInfo=true);
		static bool nclCatchesSignals; // default False;
		typedef void (*SignalHandlerFuncPtr) (int);
		static SignalHandlerFuncPtr prevSignalCatcher; // the signal handler that was installed before NCL's signal handler
		static unsigned numSigIntsCaught;
		static bool prevSignalStored ;

		void			CoreExecutionTasks(NxsToken& token, bool notifyStartStop = true);


		void			AddBlockToUsedBlockList(const std::string &, NxsBlock *, NxsToken *);
		bool 			BlockIsASingeltonReader(NxsBlock *) const ;
		void 			BlockReadHook(const NxsString &currBlockName, NxsBlock *currBlock, NxsToken *token = NULL );
		bool 			ExecuteBlock(NxsToken &token, const NxsString &currBlockName, NxsBlock *currBlock, NxsBlockFactory * sourceOfBlock);
		NxsBlock	   *FindBlockOfTypeByTitle(const std::string &btype, const char *title, unsigned *nMatches);
		NxsBlock	   *FindBlockByTitle(const BlockReaderList & chosenBlockList, const char *title, unsigned *nMatches);
		BlockReaderList FindAllBlocksByTitle(const BlockReaderList & chosenBlockList, const char *title);
		NxsBlock	   *GetLastStoredBlockByID(const std::string &key);
		NxsTaxaBlockAPI *GetOriginalTaxaBlock(const NxsTaxaBlockAPI *) const;
		bool			IsRepeatedTaxaBlock(const NxsTaxaBlockAPI *) const;
		void 			NewBlockTitleCheckHook(const std::string &blockname, NxsBlock *p, NxsToken *token);
		bool 			ReadUntilEndblock(NxsToken &token, const std::string & currBlockName);
		void			RegisterAltTitle(const NxsBlock * b, std::string t);
		std::set<NxsBlock *> RemoveBlocksFromFactoriesFromUsedBlockLists();
		virtual void 	PostBlockReadingHook(NxsBlock &);


		NxsBlock		*blockList;	/* pointer to first block in list of blocks */
		NxsBlock		*currBlock;	/* pointer to current block in list of blocks */
		typedef std::list<NxsBlockFactory *> BlockFactoryList;
		NxsTaxaBlockFactory *taxaBlockFactory;
		BlockFactoryList factories; /* list of pointers to factories capable of creating NxsBlock objects*/
		bool destroyRepeatedTaxaBlocks;
		NxsWarnLevel currentWarningLevel;
		bool alwaysReportStatusMessages;

	private:

		BlockReaderList FindAllBlocksByTitleNoPrioritization(const BlockReaderList & chosenBlockList, const char *title);

		BlockReaderList blocksInOrder;
		std::map<NxsBlock *, int> blockPriorities;

		BlockReaderList lastExecuteBlocksInOrder;
		BlockTypeToBlockList blockTypeToBlockList;
		typedef std::pair<unsigned, std::list<std::string> > NxsBlockTitleHistory;
		typedef std::map<std::string, NxsBlockTitleHistory > NxsBlockTitleHistoryMap;
		NxsBlockTitleHistoryMap blockTitleHistoryMap;
		std::map<const NxsBlock *, std::list<std::string> > blockTitleAliases; // to deal with culling blocks and then using the titles of culled copies


	};

typedef NxsBlock NexusBlock;
typedef NxsReader Nexus;

/*! A subclass of NxsReader that is used in much of NCL v2.1.

	The NexusError function raises a NxsException so that all errors are treated
		as parse-ending conditions that the caller of Execute must handle.
	See \ref NexusErrors
*/
class ExceptionRaisingNxsReader : public NxsReader
	{
	public:
		/*! The `warnMode` argument should be a facet of NxsReader::WarningHandlingMode
			this arguments determines what happens to warnings which are NOT converted
			to exceptions -- by default only content that is probably incorrect will
			be converted to a NxsException -- see ::SetWarningToErrorThreshold().


		*/
		ExceptionRaisingNxsReader(NxsReader::WarningHandlingMode mode=NxsReader::WARNINGS_TO_STDERR)
			:warnMode(mode),
			warningToErrorThreshold(PROBABLY_INCORRECT_CONTENT_WARNING)
			{}
		/*! Raise a NxsException. */
		void NexusError(NxsString msg, file_pos pos, long line, long col)
			{
			throw NxsException(msg, pos, line, col);
			}
		virtual void NexusWarn(const std::string & msg, NxsWarnLevel level, file_pos pos, long line, long col);

		void SkippingBlock(NxsString blockName);
		void SkippingDisabledBlock(NxsString blockName);
		/*! Sets the threshold for converting a warning into an error.   This
			is useful for making the parser stricter.

			Argument should be a facet of NxsReader::NxsWarnLevel.
			Overrides the default setting of PROBABLY_INCORRECT_CONTENT_WARNING
		*/
		void SetWarningToErrorThreshold(int t)
			{
			warningToErrorThreshold = t;
			}
		virtual void ClearContent()
			{
			NxsReader::ClearContent();
			}
	private:
		NxsReader::WarningHandlingMode warnMode;
		int warningToErrorThreshold;
	};

/*! A subclass of NxsReader that is used preserves the same output style as version 2.0, but
		allows for more flexibility in the streams that are used for output.

	Messages from SkippingBlock, SkippingDisabledBlock, and Warnings are sent to the outstream
	Warnings and Errors are written to the errstream

*/
class DefaultErrorReportNxsReader : public NxsReader
	{
	public:
		static BlockReaderList parseFile(const char *filepath, std::ostream * stdOutstream, std::ostream * errOutstream, bool parsePrivateBlocks=true, bool storeTokenInfo=true);
		/*! creates an instance that will write messages to the specified streams */
		DefaultErrorReportNxsReader(std::ostream * stdOutstream, /*!< outputstream */
									std::ostream * errOutstream) /*!< error stream */
			:NxsReader(),
			stdOut(stdOutstream),
			errOut(errOutstream)
			{
			}

		virtual ~DefaultErrorReportNxsReader() {}

		/*! \returns true. silent */
		virtual bool EnteringBlock(NxsString )
			{
			return true;
			}

		/*! writes a message to output  */
		void SkippingBlock(NxsString blockName)
			{
			if (stdOut != 0L)
				{
				*stdOut << "[!Skipping unknown block (" << blockName << ")...]\n";
				stdOut->flush();
				}
			}

		/*! writes a message to output  */
		void SkippingDisabledBlock(NxsString blockName)
			{
			if (stdOut != 0L)
				{
				*stdOut << "[!Skipping disabled block (" << blockName << ")...]\n";
				stdOut->flush();
				}
			}

		/*! writes a message to output and the error stream if the warnLevel >=  currentWarningLevel
			If the message is = PROBABLY_INCORRECT_CONTENT_WARNING (and the reader is not
			ignoring warnings, a NxsException will be raised.
		*/
		void NexusWarn(const std::string & msg, /*!< description of the warning */
						NxsWarnLevel warnLevel, /*!< severity of the warning*/
						file_pos pos, long line, long col)
			{
			if (warnLevel < currentWarningLevel)
				return;
			if (warnLevel >= PROBABLY_INCORRECT_CONTENT_WARNING)
				{
				NxsString e(msg.c_str());
				throw NxsException(e, pos, line, col);
				}
			if (errOut != 0)
				{
				*errOut << "\nWarning:  ";
				if (line > 0 || pos > 0)
					*errOut << "at line " << line << ", column " << col << " (file position " << pos << "):\n";
				*errOut  << msg << std::endl;
				}
			else if (stdOut != 0L)
				{
				*stdOut << "\nWarning:  ";
				if (line > 0 || pos > 0)
					*stdOut << "at line " << line << ", column " << col << " (file position " << pos << "):\n";
				*stdOut  << msg << std::endl;
				}
			}

		/*! Raises a NxsException.
		*/
		void NexusError(NxsString msg, file_pos pos, long line, long col)
			{
			NexusWarn(msg, NxsReader::FATAL_WARNING, pos, line, col);
			throw NxsException(msg, pos, line, col);
			}

		std::ostream * stdOut;
		std::ostream * errOut;
	};

/*!
	Returns a map from all block ids that have been read to all instances that the NxsReader knows have been read and
		have NOT been cleared.
	NOTE:  If the factory interface to NCL is not being used this may not be a complete list of all of the blocks that
		have been read!!!
*/
inline BlockTypeToBlockList NxsReader::GetUsedBlocks()
	{
	return blockTypeToBlockList;
	}

/*! Convenience function to get the factory for NxsTaxaBlocks */
inline bool NxsReader::IsRepeatedTaxaBlock(const NxsTaxaBlockAPI * testB) const
	{
	return (GetOriginalTaxaBlock(testB) != NULL);
	}


/*! Returns a list of  all blocks that the NxsReader knows have been read and have NOT been cleared.

	If a block has been "implied" by another block then the implied block will appear before the
	explicit block.
*/
inline BlockReaderList NxsReader::GetUsedBlocksInOrder()
	{
	return blocksInOrder;
	}

/*! Similar to NxsReader::GetUsedBlocksInOrder, except this list of blocks is cleared at the beginning
	of each  NxsReader::Execute  or (NxsReader::Read...) call. So the list returned list will
	only reflect blocks from the last execution operation.

	If a block has been "implied" by another block then the implied block will appear before the
	explicit block.
*/
inline BlockReaderList NxsReader::GetBlocksFromLastExecuteInOrder()
	{
	return lastExecuteBlocksInOrder;
	}

/*! Convenience function to get the factory for NxsTaxaBlocks */
inline NxsTaxaBlockFactory * NxsReader::GetTaxaBlockFactory()
	{
	return this->taxaBlockFactory;
	}



#endif

