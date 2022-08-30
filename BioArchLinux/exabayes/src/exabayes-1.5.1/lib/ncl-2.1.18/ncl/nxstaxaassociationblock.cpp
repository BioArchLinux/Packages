 
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
#include <iomanip>
#include <cassert>
#include <climits>
#include <cstdlib>

#include "ncl/nxstaxaassociationblock.h"
#include "ncl/nxsreader.h"

using namespace std;


NxsTaxaAssociationBlock::NxsTaxaAssociationBlock()
    :firstTaxaBlock(0L),
    secondTaxaBlock(0L)
    {
    NCL_BLOCKTYPE_ATTR_NAME = "TAXAASSOCIATION";
	Reset();
	}

NxsTaxaBlockAPI * NxsTaxaAssociationBlock::ProcessTaxaBlockName(const NxsString & value,  NxsToken &token) const {
    assert(this->nexusReader);
    NxsTaxaBlockAPI * cb = this->nexusReader->GetTaxaBlockByTitle(value.c_str(), NULL);
    if (cb == NULL)
        {
        errormsg = "Unknown TAXA block (";
        errormsg += value;
        errormsg +=") referred to in the TAXA command";
        throw NxsException(errormsg, token);
        }
    return cb;
}

void NxsTaxaAssociationBlock::HandleTaxaCommand(
  NxsToken &token)
{
	if (!this->nexusReader)
		NxsNCLAPIException("No NxsReader when reading TaxaAssociation block.");

	token.GetNextToken();
	this->firstTaxaBlock = this->ProcessTaxaBlockName(token.GetTokenReference(), token);
	token.GetNextToken();
	if (!token.Equals(","))
	    {
	    errormsg << "Expecting comma in the TAXA command, found \"" << token.GetTokenReference() << "\".";
	    throw NxsException(errormsg, token);
	    }
	token.GetNextToken();
	this->secondTaxaBlock = this->ProcessTaxaBlockName(token.GetTokenReference(), token);
    NxsToken::DemandEndSemicolon(token, this->errormsg, "TAXA");
}
void NxsTaxaAssociationBlock::HandleAssociatesCommand(
  NxsToken &token)
{
    if (this->firstTaxaBlock == 0L || this->secondTaxaBlock == 0L)
        {
        errormsg << "Expecting TAXA command to precede an ASSOCIATES command.";
	    throw NxsException(errormsg, token);
        }
    token.GetNextToken();
    for (;;)
        {
		std::set<unsigned> fSet;
		while (!token.IsPunctuationToken() || !(token.Equals(";") || token.Equals(",") || token.Equals("/")))
		    {
		    try {
		        this->firstTaxaBlock->GetIndicesForLabel(token.GetTokenReference(), &fSet);
                }
            catch(...)
                {
                errormsg << "Unrecognized taxon \"" << token.GetTokenReference() << "\" in ASSOCIATES command";
                throw NxsException(errormsg, token);
                }
            token.GetNextToken();
		    }
		if (!token.Equals("/"))
		    {
            errormsg << "Expecting / in ASSOCIATES command, found \"" << token.GetTokenReference() << "\"";
            throw NxsException(errormsg, token);
		    }

		if (fSet.empty())
		    {
            errormsg << "Expecting taxon labels from the first TAXA block before the / in ASSOCIATES command.";
            throw NxsException(errormsg, token);
		    }
        token.GetNextToken();

		std::set<unsigned> sSet;
		
		while (!token.IsPunctuationToken() || !(token.Equals(";") || token.Equals(",") || token.Equals("/")))
		    {
		    try {
		        this->secondTaxaBlock->GetIndicesForLabel(token.GetTokenReference(), &sSet);
                }
            catch(...)
                {
                errormsg << "Unrecognized taxon \"" << token.GetTokenReference() << "\" in ASSOCIATES command";
                throw NxsException(errormsg, token);
                }
            token.GetNextToken();
		    }

		if (!(token.Equals(";") || token.Equals(",")))
		    {
            errormsg << "Expecting , or ; in ASSOCIATES command, found \"" << token.GetTokenReference() << "\"";
            throw NxsException(errormsg, token);
		    }

		if (sSet.empty())
		    {
            errormsg << "Expecting taxon labels from the second TAXA block after the / in ASSOCIATES command.";
            throw NxsException(errormsg, token);
		    }
		
		for (std::set<unsigned>::const_iterator fIt = fSet.begin(); fIt != fSet.end(); ++fIt)
		    {
		    this->AddAssociation(*fIt, sSet);
		    }
		if (token.Equals(";"))
		    break;
		token.GetNextToken();
	    }
}

void NxsTaxaAssociationBlock::Read(
  NxsToken &token)	/* the token used to read from `in' */
	{
	isEmpty = false;

	DemandEndSemicolon(token, "BEGIN TAXAASSOCIATION");

	for (;;)
		{
		token.GetNextToken();
		NxsBlock::NxsCommandResult res = HandleBasicBlockCommands(token);
		if (res == NxsBlock::NxsCommandResult(STOP_PARSING_BLOCK))
			return;
		if (res != NxsBlock::NxsCommandResult(HANDLED_COMMAND))
			{
			if (token.Equals("TAXA"))
				HandleTaxaCommand(token);
			else if (token.Equals("ASSOCIATES"))
				HandleAssociatesCommand(token);
			else
				SkipCommand(token);
			}
		}
	}

void NxsTaxaAssociationBlock::Report(std::ostream &out) NCL_COULD_BE_CONST  /*v2.1to2.2 1 */
{
	out << '\n';
	if (this->firstTaxaBlock && this->secondTaxaBlock)
	    {
        out << NCL_BLOCKTYPE_ATTR_NAME << " block contains the following:\n";
        out << firstToSecond.size() << " associations between taxa in " << this->firstTaxaBlock->GetTitle() << " and " << this->secondTaxaBlock->GetTitle() << '\n';
        out << secondToFirst.size() << " associations between taxa in " << this->secondTaxaBlock->GetTitle() << " and " << this->firstTaxaBlock->GetTitle() << '\n';
	    }
}

void NxsTaxaAssociationBlock::Reset() 
{
	NxsBlock::Reset();
    this->firstToSecond.clear();
    this->secondToFirst.clear();
    this->firstTaxaBlock = 0L;
    this->secondTaxaBlock = 0L;
}

void NxsTaxaAssociationBlock::WriteAsNexus(std::ostream &out) const
	{
	if (this->firstTaxaBlock && this->secondTaxaBlock)
	    {
	    out << "BEGIN TAXAASSOCIATION;\n";
        WriteBasicBlockCommands(out);

        out << "    TAXA ";
        std::string taxaBlockName = this->firstTaxaBlock->GetTitle();
        out << NxsString::GetEscaped(taxaBlockName);
        out << " , ";
        taxaBlockName = this->secondTaxaBlock->GetTitle();
        out << NxsString::GetEscaped(taxaBlockName);
        out << ";\n";
        
        
        out << "    ASSOCIATES\n        ";
        bool firstAssoc = true;
        for (AssociationMap::const_iterator ftsIt = this->firstToSecond.begin(); ftsIt != this->firstToSecond.end(); ++ftsIt)
            {
            if (!firstAssoc)
                out << ",\n        ";
            unsigned fTaxonInd = ftsIt->first;
            std::string f = this->firstTaxaBlock->GetTaxonLabel(fTaxonInd);
            out << NxsString::GetEscaped(f);
            out << " / ";
            const std::set<unsigned> & secSet = ftsIt->second;
            for (std::set<unsigned>::const_iterator sIt = secSet.begin(); sIt != secSet.end(); ++sIt)
                {
                unsigned sTaxonInd = *sIt;
                std::string s = this->secondTaxaBlock->GetTaxonLabel(sTaxonInd);
                out << NxsString::GetEscaped(s) << ' ';
                }
            firstAssoc = false;
            }
        out << ";\n";
        
        out << "END;\n";
        }
	}

NxsTaxaAssociationBlock *NxsTaxaAssociationBlockFactory::GetBlockReaderForID(const std::string & idneeded, NxsReader *reader, NxsToken *)
	{
	if (reader == NULL || idneeded != "TAXAASSOCIATION")
		return NULL;
	NxsTaxaAssociationBlock * nb  = new NxsTaxaAssociationBlock();
	nb->SetImplementsLinkAPI(false);
	return nb;
	}
