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

#include "ncl/nxsdefs.h"
#include "ncl/nxsblock.h"
#include "ncl/nxsreader.h"
#include <cstdlib>
using namespace std;

bool NxsLabelToIndicesMapper::allowNumberAsIndexPlusOne = true; //@TEMPORARY hack


/* i18 */ /*v2.1to2.2 18 */

#if !defined(IGNORE_NXS_ASSERT) && !defined(NDEBUG)
	void ncl_assertion_failed(char const * expr, char const * function, char const * file, long line)
		{
#		if defined (ASSERTS_TO_EXCEPTIONS)
			NxsString err;
#		else
			std::ostream & err(cerr);
#		endif
			err << "\nNCL assertion failed:";
			err << "\n  expr: " << expr;
			err << "\n  func: " << function;
			err << "\n  file: " << file;
			err << "\n  line: " << line ;
#		if defined (ASSERTS_TO_EXCEPTIONS)
			throw NxsNCLAPIException(err);
#		else
			std::cerr << std::endl;
			std::exit(1);
#		endif
		}
#endif


/// returns the number of indices added (could be zero if the set name is not found.

unsigned NxsLabelToIndicesMapper::GetIndicesFromSets(const std::string &label,
  NxsUnsignedSet *inds,
  const NxsUnsignedSetMap & itemSets)
	{
	const unsigned labelLen = (unsigned)label.length();
	NxsUnsignedSetMap::const_iterator csIt = itemSets.begin();
	for (; csIt != itemSets.end(); ++csIt)
		{
		const NxsString & csLabel = csIt->first;
		if (labelLen == csLabel.length() && NxsString::case_insensitive_equals(label.c_str(), csLabel.c_str()))
			{
			const std::set<unsigned> & csSet = csIt->second;
			if (inds)
				inds->insert(csSet.begin(), csSet.end());
			return (unsigned)csSet.size();
			}
		}
	return 0;
	}


/// returns the number of indices added (will generate an NxsException if the name is neither a set name or a number).
unsigned NxsLabelToIndicesMapper::GetIndicesFromSetOrAsNumber(const std::string &label,
  NxsUnsignedSet *inds,
  const NxsUnsignedSetMap & itemSets,
  const unsigned maxInd,
  const char * itemType) /* "tree", "character"... */
	{
	unsigned n = GetIndicesFromSets(label, inds, itemSets);
	if (n > 0)
		return n;
	long i;
	if (!NxsString::to_long(label.c_str(), &i))
		{
		NxsString emsg;
		emsg << "Expecting a  number or " << itemType << " label, found " <<  label;
		throw NxsException(emsg);
		}
	if (!NxsLabelToIndicesMapper::allowNumberAsIndexPlusOne)
		{
		NxsString emsg;
		emsg << "Numbers are not to be used as labels to indicate " << itemType << " indices, but " << label << " was encountered.";
		throw NxsException(emsg);		
		}
	i--;
	if (i > (long)maxInd  || i < 0)
		{
		NxsString emsg = "Expecting a ";
		emsg << itemType << " name or a number corresponding to a " << itemType << "\'s number (a number from 1 to ";
		emsg << maxInd + 1 << "). Found " << label;
		throw NxsException(emsg);
		}
	unsigned asu = (unsigned) (i);
	if (inds)
		inds->insert(asu);
	return 1;
	}

void NxsBlock::WarnDangerousContent(const std::string &s, const NxsToken &token)
	{
	if (nexusReader)
		nexusReader->NexusWarnToken(s, NxsReader::PROBABLY_INCORRECT_CONTENT_WARNING, token);
	else
		throw NxsException(s, token);
	}

void NxsBlock::WarnDangerousContent(const std::string &s, const ProcessedNxsToken &token)
	{
	if (nexusReader)
		nexusReader->NexusWarnToken(s, NxsReader::PROBABLY_INCORRECT_CONTENT_WARNING, token);
	else
		throw NxsException(s, token);
	}
void NxsBlock::SkipCommand(NxsToken & token)
	{
	if (nexusReader)
		{
		errormsg = "Skipping command: ";
		errormsg << token.GetTokenReference();
		nexusReader->NexusWarnToken(errormsg, NxsReader::SKIPPING_CONTENT_WARNING, token);
		errormsg.clear();
		}
	if (!token.Equals(";"))
		SkippingCommand(token.GetToken());
	if (storeSkippedCommands)
		{
		ProcessedNxsCommand pnc;
		token.ProcessAsCommand(&pnc);
		skippedCommands.push_back(pnc);
		}
	else
		token.ProcessAsCommand(NULL);
	}

void NxsBlock::WriteSkippedCommands(std::ostream & out) const
	{
	for (list<ProcessedNxsCommand>::const_iterator cIt = skippedCommands.begin(); cIt != skippedCommands.end(); ++cIt)
		{
		if (WriteCommandAsNexus(out, *cIt))
			out << '\n';
		}

	}



/*!
	Hook to consolidate the handling of COMMANDS that are common to all blocks (TITLE, BLOCKID, END, ENDBLOCK -- and,
		evenually, LINK).
	HandleXYZ() where XYZ is the command name is then called.
	Returns NxsCommandResult(HANDLED_COMMAND), NxsCommandResult(HANDLED_COMMAND), or NxsCommandResult(UNKNOWN_COMMAND)
		to tell the caller whether the command was recognized.
*/
NxsBlock::NxsCommandResult NxsBlock::HandleBasicBlockCommands(NxsToken & token)
	{
	if (token.Equals("TITLE"))
		{
		HandleTitleCommand(token);
		return NxsBlock::NxsCommandResult(HANDLED_COMMAND);
		}
	if (false && token.Equals("BLOCKID")) /*now we are skipping this to put it at the end of blocks*/
		{
		HandleBlockIDCommand(token);
		return NxsBlock::NxsCommandResult(HANDLED_COMMAND);
		}
	if (token.Equals("LINK") && this->ImplementsLinkAPI())
		{
		HandleLinkCommand(token);
		return NxsBlock::NxsCommandResult(HANDLED_COMMAND);
		}
	if (token.Equals("END") || token.Equals("ENDBLOCK"))
		{
		HandleEndblock(token);
		return NxsBlock::NxsCommandResult(STOP_PARSING_BLOCK);
		}
	return NxsBlock::NxsCommandResult(UNKNOWN_COMMAND);
	}

/*!
	Stores the next token as the this->title field.
*/
void NxsBlock::HandleTitleCommand(NxsToken & token)
	{
	token.GetNextToken();
	if (token.Equals(";"))
		GenerateUnexpectedTokenNxsException(token, "a title for the block");
	if (!title.empty() && nexusReader)
		{
		errormsg = "Multiple TITLE commands were encountered the title \"";
		errormsg += title;
		errormsg += "\" will be replaced by \"";
		errormsg += token.GetToken() ;
		errormsg += '\"';
		nexusReader->NexusWarnToken(errormsg, NxsReader::OVERWRITING_CONTENT_WARNING, token);
		errormsg.clear();
		}
	title = token.GetToken();
	/* i19 */ /*v2.1to2.2 19 */
	autoTitle = false;
	DemandEndSemicolon(token, "TITLE");
	}

/*!
	Stores the next token as the this->blockid field.
*/
void NxsBlock::HandleBlockIDCommand(NxsToken & token)
	{
	token.GetNextToken();
	if (token.Equals(";"))
		GenerateUnexpectedTokenNxsException(token, "an id for the block");
	blockIDString = token.GetToken();
	DemandEndSemicolon(token, "BLOCKID");
	}

/*!
	The LINK command was invented for use in Mesquite where multiple instances of a particular type of data-containing
	block are allowed. The NEXUS specification does not discuss the correct behavior in such cases.

	Some of the problems caused by failing to specify how multiple data-containing block should be handled can
	be avoided by explicitly linking blocks. For instance a CHARACTERS block may have a "LINK taxa=TaxaBlockTitle;"
	to indicate which block of taxa it uses.

	The NxsBlock version merely raises a NxsUnimplementedException.

	Before version 2.1 Links between blocks were "off" by default (see below)
	In version 2.1, the block scoping was made more robust, so the Link API was
	enabled for all factory-created blocks in the commonly-used PublicNexusReader.
	In 2.1 and greater it is safe to call SetImplementsLinkAPI(true) on any block (as far as we know).

LINK API in NCL version > 2.0.04 and < 2.1
	NCL versions after 2.0.04 will support for LINK for the public blocks, but will have the functionality
		turned off by default (for backwards-compatibility).  When turned-off, LINK commands will be skipped.
		Calling SetImplementsLinkAPI(true) on an instance will enable the use of the HandleLinkCommand() and
		WriteLinkCommand()

	HandleLinkCommand should be a pure virtual function, but implementing it that way would break old code that uses
		NCL.  Instead the ImplementsLinkAPI/SetImplementsLinkAPI mechanism was invented.
		NCL components will only call HandleLinkCommand() or WriteLinkCommand() if ImplementsLinkAPI() returns true.
		For backward compatibility default all blocks have linkAPI=false.

	Client code should always call ImplementsLinkAPI() to check whether HandleLinkCommand() or WriteLinkCommand()
		are available.  Failure to do this may result in NxsUnimplementedException() being called.
*/
void NxsBlock::HandleLinkCommand(NxsToken & )
	{
	throw NxsUnimplementedException("NxsBlock::HandleLinkCommand");
	}
/*!

*/
void NxsBlock::WriteBasicBlockCommands(std::ostream &out) const
	{
	WriteTitleCommand(out);
	WriteBlockIDCommand(out);
	if (this->ImplementsLinkAPI())
		WriteLinkCommand(out);
	}

/*!
	Initializes all pointer data members to NULL, and all bool data members to true except isUserSupplied, which is
	initialized to false.
*/
NxsBlock::NxsBlock()
	:isEmpty(true),
	isEnabled(true),
	isUserSupplied(false),
	nexusReader(NULL),
	next(NULL),
	linkAPI(false),
	storeSkippedCommands(true)
	{
	}


/*!
	Raises an exception if it is not an equals sign.

 	Sets errormsg and raises a NxsException on failure.
	`contextString` is used in error messages:
		"Expecting '=' ${contextString} but found..."
*/void NxsBlock::DemandIsAtEquals(NxsToken &token, const char *contextString) const
	{
	if (!token.Equals("="))
		{
		errormsg = "Expecting '=' ";
		if (contextString)
			errormsg.append(contextString);
		errormsg << " but found " << token.GetToken() << " instead";
		throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
		}
	}

void NxsBlock::DemandEquals(ProcessedNxsCommand::const_iterator & tokIt, const ProcessedNxsCommand::const_iterator & endIt, const char *contextString) const
	{
	++tokIt;
	if (tokIt == endIt)
		{
		errormsg = "Expecting '=' ";
		if (contextString)
			errormsg.append(contextString);
		errormsg << " but found ; instead";
		--tokIt;
		throw NxsException(errormsg, *tokIt);
		}
	if (!tokIt->Equals("="))
		{
		errormsg = "Expecting '=' ";
		if (contextString)
			errormsg.append(contextString);
		errormsg << " but found " << tokIt->GetToken() << " instead";
		throw NxsException(errormsg, *tokIt);
		}
	}


/*!
 throws a NxsException with the token info for `token`
 `expected` should fill in the phrase "Expecting ${expected}, but found..."
 expected can be NULL.

 Sets this->errormsg
*/
void NxsBlock::GenerateNxsException(NxsToken &token, const char *message) const
	{
	if (message)
		errormsg = message;
	throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
	}

/*!
 throws a NxsException with the token info for `token`
 `expected` should fill in the phrase "Expecting ${expected}, but found..."
 expected can be NULL.

 Sets this->errormsg
*/
void NxsBlock::GenerateUnexpectedTokenNxsException(NxsToken &token, const char *expected) const
	{
	errormsg = "Unexpected token";
	if (expected)
		{
		errormsg += ". Expecting ";
		errormsg += expected;
		errormsg += ", but found: ";
		}
	else
		{
		errormsg += ": ";
		}
	errormsg += token.GetToken();
	throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
	}



/*!
	Called when the END or ENDBLOCK command needs to be parsed from within a block.
 	Basically just checks to make sure the next token in the data file is a semicolon.
*/
void NxsBlock::HandleEndblock(
  NxsToken &token)	/* the token used to read from in */
	{
	DemandEndSemicolon(token, "END or ENDBLOCK");
	}

/*!
	Nothing to be done.
*/
NxsBlock::~NxsBlock()
	{
	if (nexusReader)
		nexusReader->Detach(this);
	}

/*! This base class version simply returns 0 but a derived class should override this function if it needs to construct
	and run a NxsSetReader object to read a set involving characters. The NxsSetReader object may need to use this
	function to look up a character label encountered in the set. A class that overrides this method should return the
	character index in the range [1..nchar].
*/
unsigned NxsBlock::CharLabelToNumber(
  NxsString) NCL_COULD_BE_CONST /* the character label to be translated to the character's number */ /*v2.1to2.2 1 */
	{
	return 0;
	}

/*!
	Sets the value of isEnabled to false. A NxsBlock can be disabled (by calling this method) if blocks of that type
	are to be skipped during execution of the NEXUS file. If a disabled block is encountered, the virtual
	NxsReader::SkippingDisabledBlock function is called, giving your application the opportunity to inform the user
	that a block was skipped.
*/
void NxsBlock::Disable()
	{
	isEnabled = false;
	}

/*!
	Sets the value of isEnabled to true. A NxsBlock can be disabled (by calling Disable) if blocks of that type are to
	be skipped during execution of the NEXUS file. If a disabled block is encountered, the virtual
	NxsReader::SkippingDisabledBlock function is called, giving your application the opportunity to inform the user
	that a block was skipped.
*/
void NxsBlock::Enable()
	{
	isEnabled = true;
	}

/*!
	Returns value of isEnabled, which can be controlled through use of the Enable and Disable member functions. A
	NxsBlock should be disabled if blocks of that type are to be skipped during execution of the NEXUS file. If a
	disabled block is encountered, the virtual NxsReader::SkippingDisabledBlock function is called, giving your
	application the opportunity to inform the user that a block was skipped.
*/
bool NxsBlock::IsEnabled() NCL_COULD_BE_CONST /*v2.1to2.2 1 */
	{
	return isEnabled;
	}

/*!
	Returns value of isUserSupplied, which is true if and only if this block's Read function is called to process a
	block of this type appearing in a data file. This is useful because in some cases, a block object may be created
	internally (e.g. a NxsTaxaBlockAPI may be populated using taxon names provided in a DATA block), and such blocks do
	not require permission from the user to delete data stored therein.
*/
bool NxsBlock::IsUserSupplied() NCL_COULD_BE_CONST /*v2.1to2.2 1 */
	{
	return isUserSupplied;
	}

/*!
	Returns true if Read function has not been called since the last Reset. This base class version simply returns the
	value of the data member isEmpty. If you derive a new block class from NxsBlock, be sure to set isEmpty to true in
	your Reset function and isEmpty to false in your Read function.
*/
bool NxsBlock::IsEmpty() NCL_COULD_BE_CONST /*v2.1to2.2 1 */
	{
	return isEmpty;
	}

/*!
	Returns the `NCL_BLOCKTYPE_ATTR_NAME` NxsString.  \ref BlockTypeIDDiscussion
*/
NxsString NxsBlock::GetID() const
	{
	return NCL_BLOCKTYPE_ATTR_NAME;
	}

/*!
	This virtual function must be overridden for each derived class to provide the ability to read everything following
	the block name (which is read by the NxsReader object) to the end or endblock statement. Characters are read from
	the input stream 'in'. Note that to get output comments displayed, you must derive a class from NxsToken, override
	the member function OutputComment to display a supplied comment, and then pass a reference to an object of the
	derived class to this function.
*/
void NxsBlock::Read(
  NxsToken &)	/* the NxsToken to use for reading block */
	{
	}

/*!
	This virtual function should be overridden for each derived class to completely reset the block object in
	preparation for reading in another block of this type. This function is called by the NxsReader object just prior to
	calling the block object's Read function.
*/
void NxsBlock::Reset()
	{
	title = std::string();
	/* i19 */ /*v2.1to2.2 19 */
	autoTitle = false;
	// Reset base class data members that could have changed
	//
	errormsg.clear();
	isEnabled      = true;
	isEmpty        = true;
	isUserSupplied = false;
	skippedCommands.clear();

	}

/*!
	This virtual function provides a brief report of the contents of the block.
*/
void NxsBlock::Report(
  std::ostream &) NCL_COULD_BE_CONST /* the output stream to which the report is sent */ /*v2.1to2.2 1 */
	{
	}

/*!
	This function is called when an unknown command named commandName is about to be skipped. This version of the
	function does nothing (i.e., no warning is issued that a command was unrecognized). Override this virtual function
	in a derived class to provide such warnings to the user.
*/
void NxsBlock::SkippingCommand(
  NxsString )	/* the name of the command being skipped */
	{
	}

/*!
	This base class version simply returns 0, but a derived class should override this function if it needs to construct
	and run a NxsSetReader object to read a set involving taxa. The NxsSetReader object may need to use this function to
	look up a taxon label encountered in the set. A class that overrides this method should return the taxon index in
	the range [1..ntax].
*/
unsigned NxsBlock::TaxonLabelToNumber(
  NxsString ) const /* the taxon label to be translated to a taxon number */
	{
	return 0;
	}

/*!
	Returns a vector of Blocks that were created by the reading in of this block (the prototypical case is the taxa block
	that is implied by a data block).
*/
VecBlockPtr	NxsBlock::GetImpliedBlocks()
	{
	return VecBlockPtr();
	}

/*!
	Returns a vector of Blocks that were created by the reading in of this block (the prototypical case is the taxa block
	that is implied by a data block).
*/
VecConstBlockPtr NxsBlock::GetImpliedBlocksConst() const
	{
	return VecConstBlockPtr();
	}

/*!
	Should returns a  new instance (deep copy) of the same type of block with the same state.
	Note the NxsReader field should not be cloned (it should be aliased).

	NxsBlock version throws NxsUnimplementedException (in future versions of NCL this will be a pure virtual.

	NxsBlocks are expected to clone their linked blocks, but memo is passed in to avoid double cloning of shared references.
	memo is an mapper of an old block to a new instance (used when groups of blocks are being cloned).
*/
NxsBlock * NxsBlock::CloneBlock(
  NxsBlockMapper & /// memo is an mapper of an old block to a new instance (used when groups of blocks are being cloned)
  ) const
	{
	throw NxsUnimplementedException(NxsString("CloneBlock"));
	}

/*!
	Should serialize the content of the block as NEXUS.
	NxsBlock version throws NxsUnimplementedException (in future versions of NCL this will be a pure virtual.
*/
void NxsBlock::WriteAsNexus(std::ostream &) const
	{
	throw NxsUnimplementedException(NxsString("NxsBlock::WriteAsNexus"));
	}

void NxsBlock::WriteTitleCommand(std::ostream &out) const
	{
	const std::string &t = this->GetInstanceName();
	if (t.length() > 0)
		out << "    TITLE " << NxsString::GetEscaped(t) << ";\n";
	}

void NxsBlock::WriteBlockIDCommand(std::ostream &out) const
	{
	const std::string & t = this->blockIDString;
	if (t.length() > 0)
		out << "    BLOCKID " << NxsString::GetEscaped(t) << ";\n";
	}

/*!
	Should write the LINK command to out.
	NxsBlock versio merely raises NxsUnimplementedException.
	See notes on HandleLinkCommand.
*/
void NxsBlock::WriteLinkCommand(std::ostream &) const
	{
	throw NxsUnimplementedException("NxsBlock::WriteLinkCommand");
	}

std::string GetBlockIDTitleString(NxsBlock &b)
	{
	const std::string &t = b.GetInstanceName();
	std::string r = b.GetID();
	r.append(" block");
	if (t.length() > 0)
		{
		r.append(" (");
		r.append(t);
		r.append(")");
		}
	return r;
	}

/*!
	Accessor to "linkAPI" field. See notes on HandleLinkCommand.
*/
bool NxsBlock::ImplementsLinkAPI() const
	{
	return this->linkAPI;
	}

/*!
	Should be called with SetImplementsLinkAPI(true) by all blocks that overload both:
		HandleLinkCommand() and
		WriteLinkCommand()
	and that want LINK to be dealt with as a Basic block command.
	See notes on HandleLinkCommand.
*/
void NxsBlock::SetImplementsLinkAPI(bool v)
	{
	this->linkAPI = v;
	}
/*!
	Advances the token, and returns the unsigned int that the token represents

 	Sets errormsg and raises a NxsException on failure.
	`contextString` is used in error messages:
		"${contextString} must be a number greater than 0"
*/
unsigned NxsBlock::DemandPositiveInt(NxsToken &token, const char *contextString) const
	{
	return NxsToken::DemandPositiveInt(token, this->errormsg, contextString);
	}

void NxsBlock::DemandEndSemicolon(NxsToken &token, const char *contextString) const
	{
	NxsToken::DemandEndSemicolon(token, this->errormsg, contextString);
	}

