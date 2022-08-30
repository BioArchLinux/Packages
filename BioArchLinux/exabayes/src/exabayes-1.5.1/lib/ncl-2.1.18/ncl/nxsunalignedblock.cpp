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

#include <climits>
#include "ncl/nxsunalignedblock.h"
#include "ncl/nxsreader.h"
using namespace std;

//@POL Note: This file is not yet ready for use (Paul Lewis, 19-May-2007)

/*!
	Initializes `NCL_BLOCKTYPE_ATTR_NAME' to "UNALIGNED", `taxa' to `tb', `assumptionsBlock' to `ab', `ntax' and `ntaxTotal' to 0, `newtaxa'
	and `respectingCase' to false, `labels' to true, `datatype' to `NxsUnalignedBlock::standard', `missing' to '?', and
	`taxonPos' and `activeTaxon' to NULL. The `equates' map and `uMatrix' vector are both cleared. The ResetSymbols
	function is called to reset the `symbols' data member.
*/
NxsUnalignedBlock::NxsUnalignedBlock(
  NxsTaxaBlockAPI * tb)			/* is the taxa block object to consult for taxon labels */
  : NxsBlock(),
  NxsTaxaBlockSurrogate(tb, NULL)
	{
	NCL_BLOCKTYPE_ATTR_NAME = "UNALIGNED";
	Reset();
	}

/*!
	Deletes any memory allocated to the arrays `symbols', `taxonPos' and `activeTaxon'. Flushes the containers
	`equates', and `uMatrix'.
*/
NxsUnalignedBlock::~NxsUnalignedBlock()
	{
	Reset();
	}

/*!
	Returns NxsUnalignedBlock object to the state it was in when first created. See NxsUnalignedBlock constructor for
	details.
*/
void NxsUnalignedBlock::Reset()
	{
	NxsBlock::Reset();
	ResetSurrogate();
	nTaxWithData = 0;
	newtaxa = false;
	respectingCase = false;
	labels = true;
	originalDatatype = datatype = NxsCharactersBlock::standard;
	missing = '?';
	gap = '\0';
	ResetSymbols();	// also resets equates
	nChar = 0;
	uMatrix.clear();
	}

bool NxsUnalignedBlock::TaxonIndHasData(
  unsigned taxInd) const /* the character in question, in the range [0..`nchar') */
	{
	return (taxInd < uMatrix.size() && !uMatrix[taxInd].empty());
	}

std::string NxsUnalignedBlock::GetMatrixRowAsStr(const unsigned rowIndex) const /* output stream on which to print matrix */
	{
	if (!this->TaxonIndHasData(rowIndex))
		return std::string();
	std::ostringstream o;
	WriteStatesForMatrixRow(o, rowIndex);
	return o.str();
	}


void NxsUnalignedBlock::ResetDatatypeMapper()
	{
	mapper = NxsDiscreteDatatypeMapper(datatype, symbols, missing, gap, matchchar, respectingCase, equates);
	datatype = mapper.GetDatatype();
	}
/*!
	Resets standard symbol set after a change in `datatype' is made. Also flushes equates list and installs standard
	equate macros for the current `datatype'.
*/
void NxsUnalignedBlock::ResetSymbols()
	{
	switch(datatype)
		{
		case NxsCharactersBlock::nucleotide:
		case NxsCharactersBlock::dna:
			symbols =  "ACGT";
			break;

		case NxsCharactersBlock::rna:
			symbols = "ACGU";
			break;

		case NxsCharactersBlock::protein:
			symbols =  "ACDEFGHIKLMNPQRSTVWY*";
			break;

		default:
			symbols = "01";
		}

	equates.clear();
	this->equates = NxsCharactersBlock::GetDefaultEquates(datatype);
	ResetDatatypeMapper();
	}
/*!
	Provides a dump of the contents of the `uMatrix' variable. Useful for testing whether data is being read as
	expected. If `marginText' is NULL, output is flush left. If each line of output should be prefaced with
	a tab character, specify "\t" for `marginText'.
*/
void NxsUnalignedBlock::DebugShowMatrix(
  std::ostream & out,		/* is the output stream on which to print */
  const char * marginText) NCL_COULD_BE_CONST /* is text printed first on each line */ /*v2.1to2.2 1 */
	{
	if (!taxa)
		return;
	unsigned width = taxa->GetMaxTaxonLabelLength();
	const unsigned ntt = GetNTaxTotal();
	NCL_ASSERT(uMatrix.size() >= ntt);
	for (unsigned i = 0; i < ntt; i++)
		{
		const NxsDiscreteStateRow * row = GetDiscreteMatrixRow(i);
		if (row && !(row->empty()))
			{
			if (marginText != NULL)
				out << marginText;
			const NxsString currTaxonLabel = taxa->GetTaxonLabel(i); /*v2.1to2.2 4 */
			out << currTaxonLabel;
			unsigned currTaxonLabelLen = (unsigned)currTaxonLabel.size();
			unsigned diff = width - currTaxonLabelLen;
			std::string spacer(diff+5, ' ');
			out << spacer;
			mapper.WriteStateCodeRowAsNexus(out, *row);
			}
		}
	}

/*!
	Returns a string containing a formatted representation of the state `x'.
*/
std::string NxsUnalignedBlock::FormatState(
  NxsDiscreteDatum d)		/* is the element of `uMatrix' to format */
  const
	{
	if (d.taxInd >= GetNTaxTotal())
		throw NxsNCLAPIException("Taxon out of range in NxsUnalignedBlock::FormatState");
	const NxsDiscreteStateRow & row = uMatrix[d.taxInd];
	if (d.charInd >= row.size())
		return std::string(1, missing);
	return mapper.StateCodeToNexusString(row[d.charInd]);
	}

/*!
	Returns true if `ch' can be found in the `symbols' array. The value of `respectingCase' is used to determine
	whether or not the search should be case sensitive. Assumes `symbols' is non-NULL.
*/
bool NxsUnalignedBlock::IsInSymbols(
  char ch)	/* the symbol character to search for */
	{
	char char_in_question = (respectingCase ? ch : (char)toupper(ch));
	for (std::string::const_iterator sIt = symbols.begin(); sIt != symbols.end(); ++sIt)
		{
		const char char_in_symbols = (respectingCase ? *sIt : (char)toupper(*sIt));
		if (char_in_symbols == char_in_question)
			return true;
		}
	return false;
	}

/*!
	Called when DIMENSIONS command needs to be parsed from within the UNALIGNED block. Deals with everything after the
	token DIMENSIONS up to and including the semicolon that terminates the DIMENSIONS command.
*/
void NxsUnalignedBlock::HandleDimensions(
  NxsToken & token)			/* the token used to read from `in' */
	{
	unsigned ntaxRead = 0;
	for (;;)
		{
		token.GetNextToken();
		if (token.Equals("NEWTAXA"))
			newtaxa = true;
		else if (token.Equals("NTAX"))
			{
			DemandEquals(token, "after NTAX in DIMENSIONS command");
			ntaxRead = DemandPositiveInt(token, "NTAX");
			}
		else if (token.Equals(";"))
			break;
		}
	if (newtaxa)
		{
		if (ntaxRead == 0)
			{
			errormsg = "DIMENSIONS command must have an NTAX subcommand when the NEWTAXA option is in effect.";
			throw NxsException(errormsg, token);
			}
		AssureTaxaBlock(createImpliedBlock, token, "Dimensions");
		if (!createImpliedBlock)
			{
			taxa->Reset();
			if (nexusReader)
				nexusReader->RemoveBlockFromUsedBlockList(taxa);
			}
		taxa->SetNtax(ntaxRead);
		nTaxWithData = ntaxRead;
		}
	else
		{
		AssureTaxaBlock(false, token, "Dimensions");
		const unsigned ntaxinblock = taxa->GetNTax();
		if (ntaxinblock == 0)
			{
			errormsg = "A TAXA block must be read before character data, or the DIMENSIONS command must use the NEWTAXA.";
			throw NxsException(errormsg, token);
			}
		if (ntaxinblock < ntaxRead)
			{
			errormsg = "NTAX in UNALIGNED block must be less than or equal to NTAX in TAXA block\nNote: one circumstance that can cause this error is \nforgetting to specify NTAX in DIMENSIONS command when \na TAXA block has not been provided";
			throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
			}
		nTaxWithData = (ntaxRead == 0 ? ntaxinblock : ntaxRead);
		}
	}

/*!
	Called when the END or ENDBLOCK command needs to be parsed from within the UNALIGNED block. Does two things:
~
	o checks to make sure the next token in the data file is a semicolon
	o eliminates character labels and character state labels for characters that have been eliminated
~
*/
void NxsUnalignedBlock::HandleEndblock(
  NxsToken & token)		/* the token used to read from `in' */
	{
	DemandEndSemicolon(token, "END or ENDBLOCK");
	}

/*!
	Called when FORMAT command needs to be parsed from within the DIMENSIONS block. Deals with everything after the
	token FORMAT up to and including the semicolon that terminates the FORMAT command.
*/
void NxsUnalignedBlock::HandleFormat(
  NxsToken & token)	/* is the token used to read from `in' */
	{
	bool standardDataTypeAssumed = false;
	bool ignoreCaseAssumed = false;

	for (;;)
		{
		token.GetNextToken();

		if (token.Equals("DATATYPE"))
			{
			DemandEquals(token, "after keyword DATATYPE");
			// This should be one of the following: STANDARD, DNA, RNA, NUCLEOTIDE or PROTEIN
			token.GetNextToken();

			if (token.Equals("STANDARD"))
				datatype = NxsCharactersBlock::standard;
			else if (token.Equals("DNA"))
				datatype = NxsCharactersBlock::dna;
			else if (token.Equals("RNA"))
				datatype = NxsCharactersBlock::rna;
			else if (token.Equals("NUCLEOTIDE"))
				datatype = NxsCharactersBlock::nucleotide;
			else if (token.Equals("PROTEIN"))
				datatype = NxsCharactersBlock::protein;
			else
				{
				errormsg = token.GetToken();
				errormsg += " is not a valid DATATYPE within a ";
				errormsg += NCL_BLOCKTYPE_ATTR_NAME;
				errormsg += " block";
				throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
				}
			if (standardDataTypeAssumed && datatype != NxsCharactersBlock::standard)
				{
				errormsg = "DATATYPE must be specified first in FORMAT command";
				throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
				}
			originalDatatype = datatype;
			ResetSymbols();
			}
		else if (token.Equals("RESPECTCASE"))
			{
			if (ignoreCaseAssumed)
				{
				errormsg = "RESPECTCASE must be specified before MISSING and SYMBOLS in FORMAT command";
				throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
				}
			standardDataTypeAssumed = true;
			respectingCase = true;
			}
		else if (token.Equals("MISSING"))
			{
			DemandEquals(token, "after keyword MISSING");
			// This should be the missing data symbol (single character)
			token.GetNextToken();

			if (token.GetTokenLength() != 1)
				{
				errormsg = "MISSING symbol should be a single character, but ";
				errormsg += token.GetToken();
				errormsg += " was specified";
				throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
				}
			else if (token.IsPunctuationToken() && !token.IsPlusMinusToken())
				{
				errormsg = "MISSING symbol specified cannot be a punctuation token (";
				errormsg += token.GetToken();
				errormsg += " was specified)";
				throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
				}
			else if (token.IsWhitespaceToken())
				{
				errormsg = "MISSING symbol specified cannot be a whitespace character (";
				errormsg += token.GetToken();
				errormsg += " was specified)";
				throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
				}

			missing = token.GetToken()[0];

			ignoreCaseAssumed = true;
			standardDataTypeAssumed = true;
			}
		else if (token.Equals("SYMBOLS") || token.Equals("SYMBOL"))
			{
			NxsDiscreteStateCell numDefStates;
			unsigned maxNewStates;
			switch(datatype)
				{
				case NxsCharactersBlock::dna:
				case NxsCharactersBlock::rna:
				case NxsCharactersBlock::nucleotide:
					numDefStates = 4;
					maxNewStates = NCL_MAX_STATES-4;
					break;
				case NxsCharactersBlock::protein:
					numDefStates = 21;
					maxNewStates = NCL_MAX_STATES-21;
					break;
				default:
					numDefStates = 0; // replace symbols list for standard datatype
					symbols[0] = '\0';
					maxNewStates = NCL_MAX_STATES;
				}
			DemandEquals(token, "after keyword SYMBOLS");

			// This should be the symbols list
			token.SetLabileFlagBit(NxsToken::doubleQuotedToken);
			token.GetNextToken();

			token.StripWhitespace();
			unsigned numNewSymbols = token.GetTokenLength();

			if (numNewSymbols > maxNewStates)
				{
				errormsg = "SYMBOLS defines ";
				errormsg += numNewSymbols;
				errormsg += " new states but only ";
				errormsg += maxNewStates;
				errormsg += " new states allowed for this DATATYPE";
				throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
				}

			NxsString to = token.GetToken();
			unsigned tlen = (unsigned)to.size();
			NxsString processedS;
			// Check to make sure user has not used any symbols already in the
			// default symbols list for this data type
			for (unsigned i = 0; i < tlen; i++)
				{
				if (IsInSymbols(to[i]))
					{
					errormsg = "The character ";
					errormsg << to[i] << " defined in SYMBOLS is predefined for this DATATYPE and shoud not occur in a SYMBOLS subcommand of a FORMAT command.";
					if (nexusReader)
						{
						nexusReader->NexusWarnToken(errormsg, NxsReader::SKIPPING_CONTENT_WARNING, token);
						errormsg.clear();
						}
					}
				else
					processedS += to[i];
				}

			// If we've made it this far, go ahead and add the user-defined
			// symbols to the end of the list of predefined symbols
			symbols.append(processedS);

			ignoreCaseAssumed = true;
			standardDataTypeAssumed = true;
			}

		else if (token.Equals("EQUATE"))
			{
			DemandEquals(token, "after keyword EQUATE");

			// This should be a double-quote character
			token.GetNextToken();

			if (!token.Equals("\""))
				{
				errormsg = "Expecting '\"' after keyword EQUATE but found ";
				errormsg += token.GetToken();
				errormsg += " instead";
				throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
				}

			// Loop until second double-quote character is encountered
			for (;;)
				{
				token.GetNextToken();
				if (token.Equals("\""))
					break;

				// If token is not a double-quote character, then it must be the equate symbol (i.e., the
				// character to be replaced in the data matrix)
				if (token.GetTokenLength() != 1)
					{
					errormsg = "Expecting single-character EQUATE symbol but found ";
					errormsg += token.GetToken();
					errormsg += " instead";
					throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
					}

				// Check for bad choice of equate symbol
				NxsString t = token.GetToken();
				const char ch = t[0];
				bool badEquateSymbol = false;

				// The character '^' cannot be an equate symbol
				if (ch == '^')
					badEquateSymbol = true;

				// Equate symbols cannot be punctuation (except for + and -)
				if (token.IsPunctuationToken() && !token.IsPlusMinusToken())
					badEquateSymbol = true;

				// Equate symbols cannot be same as matchchar, missing, or gap
				if (ch == missing || ch == gap)
					badEquateSymbol = true;

				// Equate symbols cannot be one of the state symbols currently defined
				if (IsInSymbols(ch))
					badEquateSymbol = true;

				if (badEquateSymbol)
					{
					errormsg = "EQUATE symbol specified (";
					errormsg += token.GetToken();
					errormsg += ") is not valid; must not be same as missing, \nmatchchar, gap, state symbols, or any of the following: ()[]{}/\\,;:=*'\"`<>^";
					throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
					}

				NxsString k = token.GetToken();

				DemandEquals(token, "in EQUATE definition");

				// This should be the token to be substituted in for the equate symbol
				token.SetLabileFlagBit(NxsToken::parentheticalToken);
				token.SetLabileFlagBit(NxsToken::curlyBracketedToken);
				token.GetNextToken();
				NxsString v = token.GetToken();

				// Add the new equate association to the equates list
				equates[ch] = v;
				}

			standardDataTypeAssumed = true;
			}
		else if (token.Equals("LABELS"))
			{
			labels = true;
			standardDataTypeAssumed = true;
			}
		else if (token.Equals("NOLABELS"))
			{
			labels = false;
			standardDataTypeAssumed = true;
			}
		else if (token.Equals(";"))
			{
			break;
			}
		}
	ResetDatatypeMapper();
	}

/*!
	Called from HandleMatrix function to read in the next state. Returns true if next token encountered is a comma,
	false otherwise. A comma signals the end of data for the current taxon in an UNALIGNED block.
*/
bool NxsUnalignedBlock::HandleNextState(
  NxsToken & token,			/* is the token used to read from `in' */
  unsigned taxNum,				/* is the row in range [0..ntax) (used for error reporting only) */
  unsigned charNum,				/* is the column (used for error reporting only) */
  NxsDiscreteStateRow & row, const NxsString &nameStr)	/* is the container for storing new state */
	{
	token.SetLabileFlagBit(NxsToken::parentheticalToken);
	token.SetLabileFlagBit(NxsToken::curlyBracketedToken);
	token.SetLabileFlagBit(NxsToken::singleCharacterToken);

	token.GetNextToken();

	if (token.Equals(",") || token.Equals(";"))
		return false;
	const NxsString stateAsNexus = token.GetToken();
	const NxsDiscreteStateCell stateCode = mapper.EncodeNexusStateString(stateAsNexus, token, taxNum, charNum, NULL, nameStr);
	if (charNum < row.size())
		row[charNum] = stateCode;
	else
		{
		while (charNum < row.size())
			row.push_back(NXS_INVALID_STATE_CODE);
		row.push_back(stateCode);
		}
	return true;
	}

/*!
	Called when MATRIX command needs to be parsed from within the UNALIGNED block. Deals with everything after the
	token MATRIX up to and including the semicolon that terminates the MATRIX command.
*/
void NxsUnalignedBlock::HandleMatrix(
  NxsToken & token)	/* is the token used to read from `in' */
	{
	if (taxa == NULL)
		{
		AssureTaxaBlock(false, token, "Matrix");
		unsigned ntax = taxa->GetNTax();
		if (ntax == 0)
			{
			errormsg = "Must precede ";
			errormsg += NCL_BLOCKTYPE_ATTR_NAME;
			errormsg += " block with a TAXA block or specify NEWTAXA and NTAX in the DIMENSIONS command";
			throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
			}
		}
	const unsigned ntax = taxa->GetNTax();
	uMatrix.clear();
	uMatrix.resize(ntax);
	unsigned indOfTaxInMemory = 0;
	std::vector<unsigned> toInMem(nTaxWithData, UINT_MAX);
	const unsigned ntlabels = taxa->GetNumTaxonLabels();
	errormsg.clear();
	bool taxaBlockNeedsLabels = (ntlabels == 0);
	if (!taxaBlockNeedsLabels && ntlabels < nTaxWithData)
		{
		errormsg << "Not enough taxlabels are known to read characters for " << nTaxWithData << " taxa in the Matrix command.";
		throw NxsException(errormsg, token);
		}
	for (unsigned indOfTaxInCommand = 0; indOfTaxInCommand < nTaxWithData; indOfTaxInCommand++)
		{
		NxsString nameStr;
		if (labels)
			{
			token.GetNextToken();
			nameStr = token.GetToken();
			if (taxaBlockNeedsLabels)
				{
				if (taxa->IsAlreadyDefined(nameStr))
					{
					errormsg << "Data for this taxon (" << nameStr << ") has already been saved";
					throw NxsException(errormsg, token);
					}
				try {
					indOfTaxInMemory = taxa->AddTaxonLabel(nameStr);
					}
				catch (NxsException &x)
					{
					if (nameStr == ";")
						{
						errormsg << "Unexpected ; after only " << indOfTaxInCommand << " taxa were read (expecting characters for " << nTaxWithData << " taxa).";
						throw NxsException(errormsg, token);
						}
					x.addPositionInfo(token);
					throw x;
					}
				}
			else
				{
				unsigned numOfTaxInMemory = taxa->TaxLabelToNumber(nameStr);
				if (numOfTaxInMemory == 0)
					{
					if (token.Equals(";"))
						errormsg << "Unexpected ;";
					else
						errormsg << "Could not find taxon named " << nameStr << " among stored taxon labels";
					throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
					}
				indOfTaxInMemory = numOfTaxInMemory - 1;
				}
			}
		else
			{
			indOfTaxInMemory = indOfTaxInCommand;
			nameStr << 1+indOfTaxInMemory;
			}
		if (toInMem[indOfTaxInCommand] != UINT_MAX)
			{
			errormsg << "Characters for taxon " << indOfTaxInCommand << " (" << taxa->GetTaxonLabel(indOfTaxInMemory) << ") have already been stored";
			throw NxsException(errormsg, token);
			}
		toInMem[indOfTaxInCommand] = indOfTaxInMemory;
		NxsDiscreteStateRow * new_row = &uMatrix[indOfTaxInMemory];
		unsigned charInd = 0;
		while (HandleNextState(token, indOfTaxInMemory, charInd, *new_row, nameStr))
			charInd++;
		}
	}


/*!
	This function provides the ability to read everything following the block name (which is read by the NxsReader
	object) to the END or ENDBLOCK statement. Characters are read from the input stream `in'. Overrides the abstract
	virtual function in the base class.
*/
void NxsUnalignedBlock::Read(
  NxsToken & token)	/* is the token used to read from `in' */
	{
	isEmpty = false;
	isUserSupplied = true;

	// This should be the semicolon after the block name
	token.GetNextToken();
	if (!token.Equals(";"))
		{
		errormsg = "Expecting ';' after ";
		errormsg += NCL_BLOCKTYPE_ATTR_NAME;
		errormsg += " block name, but found ";
		errormsg += token.GetToken();
		errormsg += " instead";
		throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
		}
	nTaxWithData = 0;

	for (;;)
		{
		token.GetNextToken();
		NxsBlock::NxsCommandResult res = HandleBasicBlockCommands(token);
		if (res == NxsBlock::NxsCommandResult(STOP_PARSING_BLOCK))
			return;
		if (res != NxsBlock::NxsCommandResult(HANDLED_COMMAND))
			{
			if (token.Equals("DIMENSIONS"))
				HandleDimensions(token);
			else if (token.Equals("FORMAT"))
				HandleFormat(token);
			else if (token.Equals("TAXLABELS"))
				HandleTaxLabels(token);
			else if (token.Equals("MATRIX"))
				HandleMatrix(token);
			else
				SkipCommand(token);
			}
		}	// for (;;)
	}

/*!
	This function outputs a brief report of the contents of this UNALIGNED block. Overrides the abstract virtual
	function in the base class.
*/
void NxsUnalignedBlock::Report(
  std::ostream & out) NCL_COULD_BE_CONST /* is the output stream to which to write the report */ /*v2.1to2.2 1 */
	{
	out << '\n' << NCL_BLOCKTYPE_ATTR_NAME << " block contains ";
	if (nTaxWithData == 0)
		out << "no taxa";
	else if (nTaxWithData == 1)
		out << "one taxon";
	else
		out << nTaxWithData << " taxa";

	out << "\n  Data type is \"" << this->GetDatatypeName() << "\"" << endl;

	if (respectingCase)
		out << "  Respecting case" << endl;
	else
		out << "  Ignoring case" << endl;

	if (labels)
		out << "  Taxon labels were provided on left side of matrix" << endl;
	else
		out << "  No taxon labels were provided on left side of matrix" << endl;

	out << "  Missing data symbol is '" << missing << '\'' << endl;
	out << "  Valid symbols are: " << symbols << endl;

	int numEquateMacros = (int)equates.size();
	if (numEquateMacros > 0)
		{
		out << "  Equate macros in effect:" << endl;
		std::map<char, NxsString>::const_iterator i = equates.begin();
		for (; i != equates.end(); ++i)
			{
			out << "    " << (*i).first << " = " << (*i).second << endl;
			}
		}
	else
		out << "  No equate macros have been defined" << endl;

	out << "  Data matrix:" << endl;
	DebugShowMatrix(out, "    ");
	}

/*!
	Writes out the information in this block in NEXUS format to the specified std::ostream.
*/
void NxsUnalignedBlock::WriteAsNexus(
  std::ostream & out)	/* is the output stream on which to write */
  const
	{
	out << "BEGIN UNALIGNED;\n";
	WriteBasicBlockCommands(out);
	if (this->taxa && taxa->GetNumTaxonLabels() > this->nTaxWithData)
		out << "    DIMENSIONS NTax=" << this->nTaxWithData << ";\n";

	this->WriteFormatCommand(out);
	this->WriteMatrixCommand(out);
	WriteSkippedCommands(out);
	out << "END;\n";
	}

/*!
	Writes out the information in the MATRIX command in NEXUS format to the specified std::ostream.
*/
void NxsUnalignedBlock::WriteMatrixCommand(
  std::ostream & out)	/* is the output stream on which to print the matrix */
  const
	{
	NCL_ASSERT(taxa);
	const unsigned ntax = taxa->GetNTax();
	unsigned width = taxa->GetMaxTaxonLabelLength();
	out << "Matrix";

	//std::vector<unsigned> origIndexVec = this->GetOrigMatrixIndicesToWrite();
	bool first = true;
	for (unsigned i = 0; i < ntax; ++i)
		{
		if (!uMatrix[i].empty())
			{
			if (first)
				out << "\n";
			else
				out << ",\n";
			first = false;
			NxsString nm = taxa->GetTaxonLabel(i); /*v2.1to2.2 4 */
			std::string s = nm.c_str();
			const std::string currTaxonLabel = NxsString::GetEscaped(taxa->GetTaxonLabel(i));
			out << currTaxonLabel;

			// Print out enough spaces to even up the left edge of the matrix output
			unsigned currTaxonLabelLen = (unsigned)currTaxonLabel.size();
			unsigned diff = width - currTaxonLabelLen;
			for (unsigned k = 0; k < diff + 5; k++)
				out << ' ';

			WriteStatesForMatrixRow(out, i);
			}
		}
	out << "\n;\n";
	}

void NxsUnalignedBlock::WriteStatesForMatrixRow(
  std::ostream &out,				/* the output stream on which to write */
  unsigned currTaxonIndex) const	/* the taxon, in range [0..`ntax') */
	{
	const NxsDiscreteStateRow & row = uMatrix[currTaxonIndex];
	for (NxsDiscreteStateRow::const_iterator rIt = row.begin(); rIt != row.end(); ++rIt)
		mapper.WriteStateCodeAsNexusString(out, *rIt);
	}


/*!
	Writes out the information in the FORMAT command in NEXUS format to the specified std::ostream.
*/
void NxsUnalignedBlock::WriteFormatCommand(std::ostream &out) const
	{
	mapper.WriteStartOfFormatCommand(out);
	if (this->respectingCase)
		out << " RespectCase";
	// Output terminating semicolon
	out << ";\n";
	}

NxsUnalignedBlock *NxsUnalignedBlockFactory::GetBlockReaderForID(const std::string & idneeded, NxsReader *reader, NxsToken *)
	{
	if (reader == NULL || idneeded != "UNALIGNED")
		return NULL;
	NxsUnalignedBlock * nb  = new NxsUnalignedBlock(NULL);
	nb->SetCreateImpliedBlock(true);
	nb->SetImplementsLinkAPI(true);
	return nb;
	}

/*!
	Returns internal representation of the state for taxon `i', character `j', as a vector of integer values. In the
	normal situation, there is only one state with no uncertainty or polymorphism and the vector returned will contain
	only a single, positive integer value. If there are multiple states, the vector will contain k values, where k
	equals the number of states plus 1. The first value in the vector will be either 0 (indicating ambiguity) or 1
	(meaning polymorphism), and the remaining values will be positive integers each of which is an index into the
	symbols array. In the case of missing data, an empty vector will be returned. In an UNALIGNED block, there are a
	different number of characters for each taxon. Use NumCharsForTaxon before calling this function to make sure you
	do not ask for a character beyond the end. If that happens, a NxsUnalignedBlock::NxsX_NoSuchCharacter exception
	will be thrown. If no data is stored for taxon `i' in this UNALIGNED block, a NxsUnalignedBlock::NxsX_NoDataForTaxon
	exception will be thrown, with the exception object storing the offending taxon index in its public data member
	`taxon_index'.
*/
NxsDiscreteStateRow NxsUnalignedBlock::GetInternalRepresentation(
  unsigned taxInd,	/* is the index of the taxon in the TAXA block in range [0..`ntaxTotal') */
  unsigned charInd)	/* is the character index (greater than or equal to 0) */
	{
	if (taxInd >= uMatrix.size())
		throw NxsUnalignedBlock::NxsX_NoDataForTaxon(taxInd);
	NxsDiscreteStateRow & row = uMatrix[taxInd];
	if (charInd >= (unsigned)row.size())
		return NxsDiscreteStateRow();
	return mapper.GetStateVectorForCode(row[charInd]);
	}

/*!
	Returns number of characters stored for taxon whose index in the TAXA block is `i'. In an UNALIGNED block,
	each taxon can have a different number of characters, and this function can be used to find out how many characters
	are stored for any particular taxon. Note that `i' should be the index of the taxon of interest as it appears in
	the TAXA block. Because there may be fewer taxa in this UNALIGNED block (`ntax') than there are in the TAXA block
	(`ntaxTotal'), it is possible that no data were stored for the taxon having index `i', in which case a
	NxsUnalignedBlock::NxsX_NoDataForTaxon exception is thrown.
*/
unsigned NxsUnalignedBlock::NumCharsForTaxon(
  unsigned taxInd)	/* is the index of the taxon in range [0..`ntaxTotal') */
	{
	if (taxInd >= uMatrix.size())
		throw NxsUnalignedBlock::NxsX_NoDataForTaxon(taxInd);
	return (unsigned)uMatrix[taxInd].size();
	}


/*!
	Returns the number of states for taxon `i', character `j'. If `j' is equal to or greater than the number of
	characters for taxon `i', returns UINT_MAX. If there is missing data, the return value is 0, otherwise a positive
	integer will be returned. An alternative is to use the function GetInternalRepresentation to obtain a vector of all
	states, and the size of that vector could be used to determine both the number and the identity of the states. If
	no data was stored for the taxon having index i in the UNALIGNED block, a NxsUnalignedBlock::NxsX_NoDataForTaxon
	exception is thrown.
*/
unsigned NxsUnalignedBlock::GetNumStates(
  unsigned taxInd,	/* the taxon in range [0..`ntaxTotal') */
  unsigned charInd)	/* the character in range [0..`nchar') */
	{
	if (taxInd >= uMatrix.size())
		throw NxsUnalignedBlock::NxsX_NoDataForTaxon(taxInd);
	NxsDiscreteStateRow & row = uMatrix[taxInd];
	if (charInd >= (unsigned)row.size())
		return UINT_MAX;
	return mapper.GetNumStatesInStateCode(row[charInd]);
	}

/*!
	Returns true if the state at taxon `taxInd', character `j' is the missing state, false otherwise. Throws NxsException if
	`j' is too large (i.e. specifies a character beyond the last character for `uMatrix' row `taxInd'). Calls
	NxsUnalignedBlock::GetInternalRepresentation, so unless all you need is information about missing data, it is more
	efficient to simply call GetInternalRepresentation and see if the returned vector is empty. Note that `taxInd' should be
	the index of the taxon in the TAXA block. If data for that taxon has not been stored in this UNALIGNED block, then
	a NxsUnalignedBlock::NxsX_NoDataForTaxon exception will be thrown by GetInternalRepresentation.
*/
bool NxsUnalignedBlock::IsMissingState(
  unsigned taxInd,	/* the taxon, in range [0..`ntaxTotal') */
  unsigned charInd)	/* the character, in range [0..infinity) */
	{
	if (taxInd >= uMatrix.size())
		throw NxsNCLAPIException("Taxon index out of range of NxsUnalignedBlock::IsMissingState");
	NxsDiscreteStateRow & row = uMatrix[taxInd];
	if (charInd >= (unsigned)row.size())
		throw NxsNCLAPIException("Character index out of range of NxsUnalignedBlock::IsMissingState");
	return mapper.GetNumStates()  == (unsigned) row[charInd];
	}

/*!
	Returns true if taxon `taxInd' is polymorphic for character `j', false otherwise. Throws NxsException if `j' is too large
	(i.e. specifies a character beyond the last character for `uMatrix' row `taxInd'). Calls
	NxsUnalignedBlock::GetInternalRepresentation, so unless all you need is information about polymorphism, it is more
	efficient to simply call GetInternalRepresentation and extract the information you need from the returned vector.
	Note that `taxInd' should be the index of the taxon in the TAXA block. If data for that taxon has not been stored in this
	UNALIGNED block, then a NxsUnalignedBlock::NxsX_NoDataForTaxon exception will be thrown by
	GetInternalRepresentation.
*/
bool NxsUnalignedBlock::IsPolymorphic(
  unsigned taxInd,	/* the taxon in range [0..`ntaxTotal') */
  unsigned charInd)	/* the character in range [0..infinity) */
	{
	if (taxInd >= uMatrix.size())
		throw NxsNCLAPIException("Taxon index out of range of NxsUnalignedBlock::IsMissingState");
	NxsDiscreteStateRow & row = uMatrix[taxInd];
	if (charInd >= (unsigned)row.size())
		throw NxsNCLAPIException("Character index out of range of NxsUnalignedBlock::IsMissingState");
	return mapper.IsPolymorphic(row[charInd]);
	}


