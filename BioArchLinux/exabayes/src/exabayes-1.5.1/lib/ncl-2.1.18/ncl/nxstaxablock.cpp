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
#include <climits>
#include "ncl/nxstaxablock.h"
#include "ncl/nxsreader.h"

using namespace std;

/*! \return (NTax - 1) or  UINT_MAX if NTax is 0
*/
unsigned NxsTaxaBlock::GetMaxIndex() const
	{
	unsigned nct = dimNTax;
	if (nct == 0)
		return UINT_MAX;
	return nct - 1;
	}
unsigned NxsTaxaBlock::GetNumLabelsCurrentlyStored() const
	{
	return (unsigned)taxLabels.size();
	}


std::vector<std::string> NxsTaxaBlockAPI::GetAllLabels() const
	{
	const unsigned n = GetNTaxTotal();
	std::vector<std::string> v(n);
	for (unsigned i = 0; i < n; ++i)
		{
		NxsString nextLabel = GetTaxonLabel(i);  /*v2.1to2.2 4 */
		v[i] = std::string(nextLabel.c_str());
		}
	return v;
	}

/* \returns a 1-based number of the taxon with label of `label` (not case-sensitive).
	This is a low-level function not intended for widespread use (it is faster way to
	query the label list because it does not throw exceptions or do the numeric interpretation
	of labels).

	\warning{does NOT apply the numeric interpretation of the label.}

	\warning{ 1-based numbering}
*/
unsigned NxsTaxaBlock::TaxLabelToNumber(const std::string &label) const
	{
	std::string r(label.c_str());
	NxsString::to_upper(r);
	return CapitalizedTaxLabelToNumber(r);
	}

/* Used internally in reading of sets

 \returns the number of indices that correspond to the label (and the number
 of items that would be added to *inds if inds points to an empty set).
*/
unsigned NxsTaxaBlock::GetIndicesForLabel(const std::string &label, /*!< label, set name or string with the 1-based numeric representation of the object */
	NxsUnsignedSet *inds) const /* The set of indices to add the taxa indices to (can be 0L). */
	{
	NxsString emsg;
	const unsigned numb = TaxLabelToNumber(label);
	if (numb != 0)
		{
		if (inds)
			inds->insert(numb - 1);
		return 1;
		}
	return GetIndicesFromSetOrAsNumber(label, inds, taxSets, GetMaxIndex(), "taxon");
	}

bool NxsTaxaBlock::AddNewIndexSet(const std::string &label, const NxsUnsignedSet & inds)
	{
	NxsString nlab(label.c_str());
	const bool replaced = taxSets.count(nlab) > 0;
	taxSets[nlab] = inds;
	return replaced;
	}
/* Returns true if this set replaces an older definition.
*/
bool NxsTaxaBlock::AddNewPartition(const std::string &label, const NxsPartition & inds)
	{
	NxsString ls(label.c_str());
	bool replaced = taxPartitions.count(ls) > 0;
	taxPartitions[ls] = inds;
	return replaced;
	}

/* Initializes NCL_BLOCKTYPE_ATTR_NAME to "TAXA" and dimNTax to 0.
*/
NxsTaxaBlock::NxsTaxaBlock()
  	{
	dimNTax	= 0;
	NCL_BLOCKTYPE_ATTR_NAME		= "TAXA";
	}

NxsTaxaBlock::~NxsTaxaBlock()
	{}

/*! Other than the commands handled by NxsBlock::HandleBasicBlockCommands(), this
	function will deal with Dimensions and call NxsTaxaBlock::HandleTaxLabels()
	to parse the TaxLabels commands.

	All other commands will be skipped
*/
void NxsTaxaBlock::Read(
  NxsToken &token)	/* the token used to read from in */
	{
	Reset();
	isEmpty				= false;
	isUserSupplied		= true;

	DemandEndSemicolon(token, "BEGIN TAXA");

	for (;;)
		{
		token.GetNextToken();
		NxsBlock::NxsCommandResult res = HandleBasicBlockCommands(token);
		if (res == NxsBlock::NxsCommandResult(STOP_PARSING_BLOCK))
			return;
		if (res != NxsBlock::NxsCommandResult(HANDLED_COMMAND))
			{
			if (token.Equals("DIMENSIONS"))
				{
				token.GetNextToken();
				if (!token.Equals("NTAX"))
					{
					errormsg = "Expecting NTAX keyword, but found ";
					errormsg += token.GetToken();
					errormsg += " instead";
					throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
					}
				DemandEquals(token, "after NTAX");
				dimNTax = DemandPositiveInt(token, "NTAX");
				taxLabels.reserve(dimNTax);
				DemandEndSemicolon(token, "DIMENSIONS");
				}	// if (token.Equals("DIMENSIONS"))
			else if (token.Equals("TAXLABELS"))
				HandleTaxLabels(token);
			else
				SkipCommand(token);
			}
		}	// GetNextToken loop
	}

/*! Resets the taxLabels. \throws NxsException for illegal or duplicated labels.

	All other commands will be skipped.
*/
void NxsTaxaBlock::HandleTaxLabels(NxsToken &token)
	{
	if (dimNTax == 0)
		{
		errormsg = "NTAX must be specified before TAXLABELS command";
		throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
		}
	taxLabels.clear();
	labelToIndex.clear();
	for (unsigned i = 0; i < dimNTax; i++)
		{
		token.GetNextToken();
		try
			{
			NxsString t = token.GetToken();
			AddTaxonLabel(t);
			}
		catch (const NxsException & x)
			{
			throw NxsException(x.msg, token);
			}
		}
	DemandEndSemicolon(token, "TAXLABELS");
	}

/* This function outputs a brief report of the contents of this taxa block. Overrides the abstract virtual function in
	the base class.
*/
void NxsTaxaBlock::Report(
  std::ostream &out) NCL_COULD_BE_CONST /* the output stream to which to write the report */ /*v2.1to2.2 1 */
	{
	out << endl;
	out << NCL_BLOCKTYPE_ATTR_NAME << " block contains ";
	if (dimNTax == 0)
		{
		out << "no taxa" << endl;
		return;
		}
	if (dimNTax == 1)
		out << "1 taxon" << endl;
	else
		out << dimNTax << " taxa" << endl;
	for (unsigned k = 0; k < dimNTax; k++)
		out << "    " << (k+1) << "    " << GetTaxonLabel(k) << endl;
	}

/* Writes contents of this block in NEXUS format to `out'.
*/
void NxsTaxaBlock::WriteAsNexus(std::ostream &out) const
	{
	out << "BEGIN TAXA;\n";
	WriteBasicBlockCommands(out);
	out << "    DIMENSIONS NTax = " << dimNTax << ";\n";
	this->WriteTaxLabelsCommand(out);
	WriteSkippedCommands(out);
	out << "END;\n";
	}

/* Writes the NEXUS TaxLabels commands
*/
void NxsTaxaBlock::WriteTaxLabelsCommand(std::ostream &out) const
	{
	const unsigned nLabels = this->GetNumTaxonLabels();
	if (nLabels > 0)
		{
		out << "    TAXLABELS";
		for (NxsStringVector::const_iterator kIt = taxLabels.begin(); kIt != taxLabels.end(); ++kIt)
			out << ' ' << NxsString::GetEscaped(*kIt);
		out << ";\n";
		}
	}

/* Flushes taxonLabels and sets dimNTax to 0 in preparation for reading a new TAXA block.
*/
void NxsTaxaBlock::Reset()
	{
	NxsBlock::Reset();
	taxLabels.clear();
	labelToIndex.clear();
	dimNTax = 0;
	inactiveTaxa.clear();
	taxSets.clear();
	taxPartitions.clear();
	}

/* Adds taxon label 's' to end of list of taxon labels and increments dimNTax by 1.

	\returns the (0-based) index of taxon label just added.
*/
unsigned NxsTaxaBlock::AddTaxonLabel(
  const std::string & rs)	/* the taxon label to add */
	{
	unsigned ind = (unsigned)taxLabels.size();
	NxsString s(rs.c_str());
	std::string x(rs.c_str());
	NxsString::to_upper(x);
	CheckCapitalizedTaxonLabel(x);
	taxLabels.push_back(s);
	labelToIndex[x] = ind;
	return ind;
	}

/*  No action if the label `s` could be added to list of taxon labels

	\throws a NxsException if the label `s` is illegal, and
	\throws a DuplicatedLabelNxsException if the label is already in the block
*/
void NxsTaxaBlock::CheckCapitalizedTaxonLabel(
  const std::string &s) const /*!< potential taxon label to check */
  {
	unsigned ind = (unsigned)taxLabels.size();
	if (dimNTax < ind)
		{
		NxsString e;
		e << "Number of stored labels (" << taxLabels.size() << ") exceeds the NTax specified (" << dimNTax<<").";
		throw NxsException(e);
		}
	if (CapitalizedTaxLabelToNumber(s) != 0)
		{
		NxsString e = "TaxLabels cannot be repeated. The label ";
		e << s << " has already been stored.";
		throw DuplicatedLabelNxsException(e);
		}
	if (s.length() == 1 && NxsString::IsNexusPunctuation(s[0]))
		{
		NxsString e = "Illegal TaxLabel found:\n";
		e << s << "\n TaxLabels cannot be punctuation.";
		throw NxsException(e);
		}
	}


void NxsTaxaBlock::ChangeTaxonLabel(
  unsigned i,	/* the taxon label number to change */
  NxsString s)	/* the string used to replace label i */ /*v2.1to2.2 4 */
	{
	if (i >= (unsigned)taxLabels.size())
		{
		NxsString e = "The label for taxon ";
		e << (i+1) << " cannot be changed, because the only " << (unsigned)taxLabels.size() << " taxlabel(s) have been assigned.";
		throw NxsNCLAPIException(e);
		}
	RemoveTaxonLabel(i);
	std::string x(s.c_str());
	NxsString::to_upper(x);
	CheckCapitalizedTaxonLabel(x);
	taxLabels[i] = NxsString(s.c_str()); /* odd construct for v2.1->v2.2 translation */
	labelToIndex[x] = i;
	}

void NxsTaxaBlock::RemoveTaxonLabel(
  unsigned i)	/* the taxon label number to remove */
	{
	std::string oldLabel(taxLabels[i].c_str());
	NxsString::to_upper(oldLabel);
	labelToIndex.erase(oldLabel);
	taxLabels[i] = NxsString();
	}

/* Returns the length of the longest taxon label stored. Useful for formatting purposes in outputting the data matrix
	(i.e., you want the left edge of the matrix to line up).
*/
unsigned NxsTaxaBlock::GetMaxTaxonLabelLength()
	{
	NxsStringVector::const_iterator tlIt = taxLabels.begin();
	unsigned maxlen = 0;
	for (; tlIt < taxLabels.end(); ++tlIt)
		{
		const unsigned thislen = (unsigned)tlIt->size();
		if (thislen > maxlen)
			maxlen = thislen;
		}
	return maxlen;
	}

/* Returns the label for taxon 'i'. where i is in the range [0-dimNTax)
*/
NxsString NxsTaxaBlock::GetTaxonLabel(unsigned i) const  /*v2.1to2.2 4 */
	{
	if (i >= dimNTax)
		{
		NxsString e = "The  taxon index ";
		e << i  << " is out of range.  Only " << dimNTax << " taxa in block.";
		throw NxsNCLAPIException(e);
		}
	if (i < (unsigned)taxLabels.size())
		return taxLabels[i];
	NxsString s;
	s += (i + 1);
	return s; /*the number is a default label*/
	}

/*!
	Returns true if taxonLabels[i] contains embedded spaces and thus should be surrounded by single quotes if output is
	NEXUS format.
*/
bool NxsTaxaBlock::NeedsQuotes(
  unsigned i)	/* the taxon label number in question */
	{
	const NxsString x(GetTaxonLabel(i).c_str());
	return x.QuotesNeeded();
	}

/*! Returns true if taxon label equal to 's' can be found in the taxonLabels list, and returns false otherwise.
*/
bool NxsTaxaBlock::IsAlreadyDefined(
  const std::string & s)	/* the s to attempt to find in the taxonLabels list */
	{
	return (TaxLabelToNumber(s) != 0);
	}

/*!
	Returns a (0-based) index of taxon named 's' in taxonLabels list. If taxon named 's' cannot be found, or if there are no
	labels currently stored in the taxonLabels list, throws NxsX_NoSuchTaxon exception.
*/
unsigned NxsTaxaBlock::FindTaxon(
  const NxsString &s) const /* the string to attempt to find in the taxonLabels list */  /*v2.1to2.2 4 */
	{
	unsigned k = TaxLabelToNumber(s);
	if (k == 0)
		throw NxsTaxaBlock::NxsX_NoSuchTaxon();
	return (k - 1);
	}

/*! Returns number of taxon labels currently stored.
*/
unsigned NxsTaxaBlock::GetNumTaxonLabels() const
	{
	return (unsigned)taxLabels.size();
	}

/*! Sets dimNTax to n.
*/
void NxsTaxaBlock::SetNtax(
  unsigned n)	/* the number of taxa */
	{
	dimNTax = n;
	if (taxLabels.size() > dimNTax)
		{
		for (unsigned i = dimNTax; i < taxLabels.size(); i++)
			RemoveTaxonLabel(i);
		taxLabels.resize(dimNTax);
		}
	else
		taxLabels.reserve(dimNTax);
	}

NxsTaxaBlock *NxsTaxaBlockFactory::GetBlockReaderForID(const std::string & idneeded, NxsReader *reader, NxsToken *)
	{
	if (reader == NULL || idneeded != "TAXA")
		return NULL;
	NxsTaxaBlock * nb = new NxsTaxaBlock();
	nb->SetImplementsLinkAPI(false);
	return nb;
	}

NxsTaxaBlockAPI * NxsTaxaBlockSurrogate::GetTaxaBlockPtr(int *status) const
	{
	if (status)
		*status = GetTaxaLinkStatus();
	return taxa;
	}


void NxsTaxaBlockSurrogate::SetTaxaLinkStatus(NxsBlock::NxsBlockLinkStatus s)
	{
	if (taxaLinkStatus & NxsBlock::BLOCK_LINK_USED)
		{
		throw NxsNCLAPIException("Resetting a used taxaLinkStatus");
		}
	taxaLinkStatus = s;
	}


void NxsTaxaBlockSurrogate::SetTaxaBlockPtr(NxsTaxaBlockAPI *c, NxsBlock::NxsBlockLinkStatus s)
	{
	SetTaxaLinkStatus(s);
	taxa = c;
	}

/*only used it the linkAPI is enabled*/
void NxsTaxaBlockSurrogate::HandleLinkTaxaCommand(NxsToken & token)
	{
	token.GetNextToken();
	const std::map<std::string, std::string> kv = token.ProcessAsSimpleKeyValuePairs("LINK");
	std::map<std::string, std::string>::const_iterator pairIt = kv.begin();
	for (;pairIt != kv.end(); ++pairIt)
		{
		NxsTaxaBlockAPI *entryTaxa = taxa;
		int entryTaxaLinkStatus = taxaLinkStatus;
		NxsString key(pairIt->first.c_str());
		key.ToUpper();
		NxsString value(pairIt->second.c_str());
		if (key == "TAXA")
			{
			if (taxa && !taxa->GetID().EqualsCaseInsensitive(value))
				{
				if (GetTaxaLinkStatus() & NxsBlock::BLOCK_LINK_USED)
					{
					NxsString errormsg = "LINK to a Taxa block must occur before commands that use a taxa block";
					throw NxsException(errormsg, token);
					}
				SetTaxaBlockPtr(NULL, NxsBlock::BLOCK_LINK_UNINITIALIZED);
				}
			if (!taxa)
				{
				if (!nxsReader)
					{
					NxsString errormsg =  "API Error: No nxsReader during parse in NxsTaxaBlockSurrogate::HandleLinkTaxaCommand";
					throw NxsNCLAPIException(errormsg, token);
					}
				NxsTaxaBlockAPI * cb = nxsReader->GetTaxaBlockByTitle(value.c_str(), NULL);
				if (cb == NULL)
					{
					NxsString errormsg = "Unknown TAXA block (";
					errormsg += value;
					errormsg +=") referred to in the LINK command";
					taxa = entryTaxa;
					taxaLinkStatus = entryTaxaLinkStatus;
					throw NxsException(errormsg, token);
					}
				SetTaxaBlockPtr(cb, NxsBlock::BLOCK_LINK_FROM_LINK_CMD);
				}
			}
		else
			{
			NxsString errormsg = "Skipping unknown LINK subcommand: ";
			errormsg += pairIt->first.c_str();
			nxsReader->NexusWarnToken(errormsg, NxsReader::SKIPPING_CONTENT_WARNING, token);
			errormsg.clear(); //this token pos will be off a bit.
			}
		}
	}
void NxsTaxaBlockSurrogate::WriteLinkTaxaCommand(std::ostream &out) const
	{
	if (taxa && !(taxa->GetTitle().empty()))
		out << "    LINK TAXA = " << NxsString::GetEscaped(taxa->GetTitle()) << ";\n";
	}

/* This function is called by derived classes right before they start to parse a command
	that requires a Taxa block.
	If a taxa block has not been set at this point, and one cannot be created then
	a NxsException will be generated.

	This enables lazy initialization of the taxa field.
*/
void NxsTaxaBlockSurrogate::AssureTaxaBlock(bool allocBlock, NxsToken &token, const char *cmd)
	{
	if (!allocBlock)
		{
		if (taxa != NULL)
			return;
		if (!nxsReader)
			{
			NxsString  errormsg =  "API Error: No nxsReader during parse in NxsTaxaBlockSurrogate::AssureTaxaBlock";
			throw NxsNCLAPIException(errormsg, token);
			}
		unsigned nTb;
		NxsTaxaBlockAPI * cb = nxsReader->GetTaxaBlockByTitle(NULL, &nTb);
		if (cb == NULL)
			{
			NxsString errormsg =  "TAXA Block has been not been read, but a ";
			if (cmd)
				errormsg += cmd;
			errormsg += " command (which requires a TAXA block) has been encountered. Either add a TAXA block or (for blocks other than the TREES block) you may use a \"DIMENSIONS NEWTAXA NTAX= ...\" command to introduces taxa.";
			throw NxsException(errormsg, token);
			}
		if (nTb > 1)
			{
			NxsString errormsg =  "Multiple TAXA Blocks have been read (or implied using NEWTAXA in other blocks) and a ";
			if (cmd)
				errormsg += cmd;
			errormsg += " command (which requires a TAXA block) has been encountered";
			std::string bn = token.GetBlockName();
			if (!bn.empty())
				{
				errormsg += " in a ";
				errormsg += bn;
				errormsg += " block.";
				}
			errormsg += ".\nThis can be caused by reading multiple files. It is possible that\neach file is readable separately, but cannot be read unambiguously when read in sequence.\n";
			errormsg += "One way to correct this is to use the\n    TITLE some-unique-name-here ;\ncommand in the TAXA block and an accompanying\n    LINK TAXA=the-unique-title-goes here;\n";
			errormsg += "command to specify which TAXA block is needed.";
			cb->WarnDangerousContent(errormsg, token);
			}
		taxa = cb;
		return;
		}
	if (nxsReader != NULL)
		{
		NxsTaxaBlockFactory * tbf = nxsReader->GetTaxaBlockFactory();
		if (tbf)
			{
			std::string s("TAXA");
			taxa = tbf->GetBlockReaderForID(s, nxsReader, &token);
			ownsTaxaBlock = true;
			passedRefOfOwnedBlock = false;
			taxaLinkStatus = NxsBlock::BLOCK_LINK_TO_IMPLIED_BLOCK;
			}
		}
	if (taxa == NULL)
		{
		taxa = new NxsTaxaBlock();
		ownsTaxaBlock = true;
		passedRefOfOwnedBlock = false;
		taxaLinkStatus = NxsBlock::BLOCK_LINK_TO_IMPLIED_BLOCK;
		}
	}

bool NxsTaxaBlockSurrogate::SurrogateSwapEquivalentTaxaBlock(NxsTaxaBlockAPI * tb)
	{
		NxsTaxaBlockFactory * tbf = nxsReader->GetTaxaBlockFactory();
		if (this->taxa && this->ownsTaxaBlock && tbf)
			tbf->BlockError(taxa);
		this->SetTaxaBlockPtr(tb, NxsBlock::BLOCK_LINK_EQUIVALENT_TO_IMPLIED);
		return true;
	}

void NxsTaxaBlockSurrogate::ResetSurrogate()
	{
	if (ownsTaxaBlock)
		{
		if (!passedRefOfOwnedBlock)
			{
			if (taxa != NULL && nxsReader != NULL)
				{
				NxsTaxaBlockFactory * factory = nxsReader->GetTaxaBlockFactory();
				if (factory)
					{
					factory->BlockError(taxa);
					taxa = NULL;
					}
				}
			if (taxa)
				delete taxa;
			}
		taxa = NULL; /*potential memory leak, but a reference should be held, by the nexus reader.*/
		ownsTaxaBlock = false;
		taxaLinkStatus = NxsBlock::BLOCK_LINK_UNINITIALIZED;
		}
	newtaxa = false;
	passedRefOfOwnedBlock = false;
	}

VecBlockPtr NxsTaxaBlockSurrogate::GetCreatedTaxaBlocks()
	{
	VecBlockPtr vbp;
	if (newtaxa && taxa)
		{
		vbp.push_back(taxa);
		passedRefOfOwnedBlock = true;
		}
	return vbp;
	}
/*!
	Called when TAXLABELS command needs to be parsed from within the UNALIGNED block. Deals with everything after the
	token TAXLABELS up to and including the semicolon that terminates the TAXLABELS command.
*/
void NxsTaxaBlockSurrogate::HandleTaxLabels(
  NxsToken & token)	/* the token used to read from `in' */
	{
	if (!newtaxa || taxa == NULL)
		{
		NxsString errormsg = "NEWTAXA must have been specified in DIMENSIONS command to use the TAXLABELS command in a ";
		errormsg << GetBlockName() << " block";
		throw NxsException(errormsg, token);
		}
	taxa->HandleTaxLabels(token);
	}

