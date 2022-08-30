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
#include "ncl/nxssetreader.h"
#include "ncl/nxstoken.h"
#include <algorithm>
#include <iterator>
using namespace std;

void NxsSetReader::AddRangeToSet(unsigned first, unsigned last, unsigned stride, NxsUnsignedSet * destination, const NxsUnsignedSet * taboo, NxsToken &token)
	{
	NCL_ASSERT (last >= first);
	NCL_ASSERT (last !=UINT_MAX);
	NCL_ASSERT (stride !=UINT_MAX);
	if (destination == NULL)
		return;
	NxsUnsignedSet::iterator dIt = destination->insert(first).first;
	for (unsigned curr = first + stride; curr <= last; curr += stride)
		{
		if (taboo != NULL && taboo->count(curr) > 0)
			{
			NxsString errormsg;
			errormsg << "Illegal repitition of an index (" << curr + 1 << ") in multiple subsets.";
			throw NxsException(errormsg, token);
			}
		dIt = destination->insert(dIt, curr);
		}
	}

/**
	returns the number of indices added.
*/
unsigned NxsSetReader::InterpretTokenAsIndices(NxsToken &token,
  const NxsLabelToIndicesMapper & mapper,
  const char * setType,
  const char * cmdName,
  NxsUnsignedSet * destination)
	{
	try {
		const std::string t = token.GetToken();
		if (NxsString::case_insensitive_equals(t.c_str(), "ALL"))
			{
			unsigned m = mapper.GetMaxIndex();
			NxsUnsignedSet s;
			for (unsigned i = 0; i <= m; ++i)
				s.insert(i);
			destination->insert(s.begin(), s.end());
			return (unsigned)s.size();
			}
		return mapper.GetIndicesForLabel(t, destination);
		}
	catch (const NxsException & x)
		{
		NxsString errormsg = "Error in the ";
		errormsg << setType << " descriptor of a " << cmdName << " command.\n";
		errormsg += x.msg;
		throw NxsException(errormsg, token);
		}
	catch (...)
		{
		NxsString errormsg = "Expecting a ";
		errormsg << setType << " descriptor (number or label) in the " << cmdName << ".  Encountered ";
		errormsg <<  token.GetToken();
		throw NxsException(errormsg, token);
		}
	}

void NxsSetReader::ReadSetDefinition(
  NxsToken &token,
  const NxsLabelToIndicesMapper & mapper,
  const char * setType, /* "TAXON" or "CHARACTER" -- for error messages only */
  const char * cmdName, /* command name -- "TAXSET" or "EXSET"-- for error messages only */
  NxsUnsignedSet * destination, /** to be filled */
  const NxsUnsignedSet * taboo)
	{
	NxsString errormsg;
	NxsUnsignedSet tmpset;
	NxsUnsignedSet dummy;
	if (destination == NULL)
		destination = & dummy;
	unsigned previousInd = UINT_MAX;
	std::vector<unsigned> intersectVec;
	while (!token.Equals(";"))
		{
		if (taboo && token.Equals(","))
			return;
		if (token.Equals("-"))
			{
			if (previousInd == UINT_MAX)
				{
				errormsg = "The '-' must be preceded by number or a ";
				errormsg << setType << " label in the " << cmdName << " command.";
				throw NxsException(errormsg, token);
				}
			token.GetNextToken();
			if (token.Equals(";") || token.Equals("\\"))
				{
				errormsg = "Range in the ";
				errormsg << setType << " set definition in the " << cmdName << " command must be closed with a number or label.";
				throw NxsException(errormsg, token);
				}
			unsigned endpoint;
			if (token.Equals("."))
				endpoint = mapper.GetMaxIndex();
			else
				{
				tmpset.clear();
				unsigned nAdded = NxsSetReader::InterpretTokenAsIndices(token, mapper, setType, cmdName, &tmpset);
				if (nAdded != 1)
					{
					errormsg = "End of a range in a ";
					errormsg << setType << " set definition in the " << cmdName << " command must be closed with a single number or label (not a set).";
					throw NxsException(errormsg, token);
					}
				endpoint = *(tmpset.begin());
				if (endpoint < previousInd)
					{
					errormsg = "End of a range in a ";
					errormsg << setType << " set definition in the " << cmdName << " command must be a larger index than the start of the range (found ";
					errormsg << previousInd + 1 << " - " << token.GetToken();
					throw NxsException(errormsg, token);
					}
				}
			token.GetNextToken();
			if (token.Equals("\\"))
				{
				token.GetNextToken();
				NxsString t = token.GetToken();
				unsigned stride = 0;
				try
					{
					stride = t.ConvertToUnsigned();
					}
				catch (const NxsString::NxsX_NotANumber &)
					{}
				if (stride == 0)
					{
					errormsg = "Expecting a positive number indicating the 'stride' after the \\ in the ";
					errormsg << setType << " set definition in the " << cmdName << " command. Encountered ";
					errormsg << t;
					throw NxsException(errormsg, token);
					}
				AddRangeToSet(previousInd, endpoint, stride, destination, taboo, token);
				token.GetNextToken();
				}
			else
				AddRangeToSet(previousInd, endpoint, 1, destination, taboo, token);
			previousInd = UINT_MAX;
			}
		else
			{
			tmpset.clear();
			const unsigned nAdded = NxsSetReader::InterpretTokenAsIndices(token, mapper, setType, cmdName, &tmpset);
			if (taboo != NULL)
				{
				set_intersection(taboo->begin(), taboo->end(), tmpset.begin(), tmpset.end(), back_inserter(intersectVec));
				if (!intersectVec.empty())
					{
					errormsg << "Illegal repitition of an index (" << 1 + *(intersectVec.begin()) << ") in multiple subsets.";
					throw NxsException(errormsg, token);
					}
				}
			if (nAdded == 1 )
				{
				previousInd = *(tmpset.begin());
				destination->insert(previousInd);
				}
			else
				{
				previousInd = UINT_MAX;
				destination->insert(tmpset.begin(), tmpset.end());
				}
			token.GetNextToken();
			}
		}
	}

/*!
	Initializes `max' to maxValue, `settype' to `type', `token' to `t', `block' to `nxsblk' and `nxsset' to `iset',
	then clears `nxsset'.
*/
NxsSetReader::NxsSetReader(
  NxsToken			&t,			/* reference to the NxsToken being used to read in the NEXUS data file */
  unsigned			maxValue,	/* maximum possible value allowed in this set (e.g. nchar or ntax) */
  NxsUnsignedSet	&iset,		/* reference to the set object to store the set defined in the NEXUS data file */
  NxsBlock			&nxsblk,	/* reference to the NxsBlock object (used for looking up taxon or character labels when encountered in the set definition) */
  unsigned			type)		/* one of the elements in the NxsSetReaderEnum enumeration */
  : block(nxsblk), token(t), nxsset(iset)
	{
	max		= maxValue;
	settype	= type;
	nxsset.clear();
	}

/*!
	Adds the range specified by `first', `last', and `modulus' to the set. If `modulus' is zero it is ignored. The
	parameters `first' and `last' refer to numbers found in the data file itself, and thus have range [1..`max']. They
	are stored in `nxsset', however, with offset 0. For example, if the data file says "4-10\2" this function would be
	called with `first' = 4, `last' = 10 and `modulus' = 2, and the values stored in `nxsset' would be 3, 5, 7, 9. The
	return value is true unless `last' is greater than `max', `first' is less than 1, or `first' is greater than `last':
	in any of these cases, the return value is false to indicate failure to store this range.
*/
bool NxsSetReader::AddRange(
  unsigned first,		/* the first member of the range (inclusive, offset 1) */
  unsigned last,		/* the last member of the range (inclusive, offset 1) */
  unsigned modulus)		/* the modulus to use (if non-zero) */
	{
	if (last > max || first < 1 || first > last)
		return false;

	for (unsigned i = first - 1; i < last; i++)
		{
		unsigned diff = i - first + 1;
		if (modulus > 0 && diff % modulus != 0)
			continue;
		nxsset.insert(i);
		}

	return true;
	}

/*!
	Tries to interpret `token' as a number. Failing that, tries to interpret `token' as a character or taxon label,
	which it then converts to a number. Failing that, it throws a NxsException exception.
*/
unsigned NxsSetReader::GetTokenValue()
	{
	int i = -1;
	try {
	    i = token.GetToken().ConvertToInt();
	    }
	catch (NxsString::NxsX_NotANumber &x)
	    {
	    }

	unsigned v = 0;
	if (i > 0)
		v = (unsigned) i;

	if (v == 0 && settype != NxsSetReader::generic)
		{
		if (settype == NxsSetReader::charset)
			v = block.CharLabelToNumber(token.GetToken());
		else if (settype == NxsSetReader::taxset)
			v = block.TaxonLabelToNumber(token.GetToken());
		}

	if (v == 0)
		{
		block.errormsg = "Set element (";
		block.errormsg += token.GetToken();
		block.errormsg += ") not a number ";
		if (settype == NxsSetReader::charset)
			block.errormsg += "and not a valid character label";
		else if (settype == NxsSetReader::taxset)
			block.errormsg += "and not a valid taxon label";

		throw NxsException(block.errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
		}

	return v;
	}

std::vector<unsigned> NxsSetReader::GetSetAsVector(const NxsUnsignedSet &s)
	{
	std::vector<unsigned> u;
	u.reserve(s.size());
	for (NxsUnsignedSet::const_iterator sIt = s.begin(); sIt != s.end(); ++sIt)
		u.push_back(*sIt);
	return u;
	}

void NxsSetReader::WriteSetAsNexusValue(const NxsUnsignedSet & nxsset, std::ostream & out)
	{
	NxsUnsignedSet::const_iterator currIt = nxsset.begin();
	const NxsUnsignedSet::const_iterator endIt = nxsset.end();
	if (currIt == endIt)
		return;
	unsigned rangeBegin = 1 + *currIt++;
	if (currIt == endIt)
		{
		out << ' ' << rangeBegin;
		return;
		}
	unsigned prev = 1 + *currIt++;
	if (currIt == endIt)
		{
		out << ' ' << rangeBegin << ' ' << prev;
		return;
		}
	unsigned stride = prev - rangeBegin;
	unsigned curr = 1 + *currIt++;
	bool inRange = true;
	while (currIt != endIt)
		{
		if (inRange)
			{
			if (curr - prev != stride)
				{
				if (prev - rangeBegin == stride)
					{
					out << ' ' << rangeBegin;
					rangeBegin = prev;
					stride = curr - prev;
					}
				else
					{
					if (stride > 1)
						out << ' ' << rangeBegin << '-' << prev << " \\ " << stride;
					else
						out << ' ' << rangeBegin << '-' << prev ;
					inRange = false;
					}
				}
			}
		else
			{
			inRange = true;
			rangeBegin = prev;
			stride = curr - prev;
			}
		prev = curr;
		curr = 1 + *currIt;
		currIt++;
		}
	if (inRange)
		{
		if (curr - prev != stride)
			{
			if (prev - rangeBegin == stride)
				out << ' ' << rangeBegin << ' ' << prev;
			else
				{
				if (stride > 1)
					out << ' ' << rangeBegin << '-' << prev << " \\ " << stride;
				else
					out << ' ' << rangeBegin << '-' << prev ;
				}
			out << ' ' << curr;
			}
		else
			{
			if (stride > 1)
				out << ' ' << rangeBegin << '-' << curr << " \\ " << stride;
			else
				out << ' ' << rangeBegin << '-' << curr ;
			}
		}
	else
		out << ' ' << prev << ' ' << curr;
	}
/*!
	Reads in a set from a NEXUS data file. Returns true if the set was terminated by a semicolon, false otherwise.
*/
bool NxsSetReader::Run()
	{
	bool ok;
	bool retval = false;

	unsigned rangeBegin = UINT_MAX;
	unsigned rangeEnd = rangeBegin;
	bool insideRange = false;
	unsigned modValue = 1;

	for (;;)
		{
		// Next token should be one of the following:
		//   ';'        --> set definition finished
		//   '-'        --> range being defined
		//   <integer>  --> member of set (or beginning or end of a range)
		//   '.'        --> signifies the number max
		//   '\'        --> signifies modulus value coming next
		//
		token.GetNextToken();

		if (token.Equals("-"))
			{
			// We should not be inside a range when we encounter a hyphenation symbol.
			// The hyphen is what _puts_ us inside a range!
			//
			if (insideRange)
				{
				block.errormsg = "The symbol '-' is out of place here";
				throw NxsException(block.errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
				}
			insideRange = true;
			}

		else if (token.Equals("."))
			{
			// We _should_ be inside a range if we encounter a period, as this
			// is a range termination character
			//
			if (!insideRange)
				{
				block.errormsg = "The symbol '.' can only be used to specify the end of a range";
				throw NxsException(block.errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
				}
			rangeEnd = max;
			}

		else if (token.Equals("\\"))
			{
			// The backslash character is used to specify a modulus to a range, and
			// thus should only be encountered if currently inside a range
			//
			if (!insideRange)
				{
				block.errormsg = "The symbol '\\' can only be used after the end of a range has been specified";
				throw NxsException(block.errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
				}

			// This should be the modulus value
			//
			modValue = NxsToken::DemandPositiveInt(token, block.errormsg, "The modulus value");
			}

		else if (insideRange && rangeEnd == UINT_MAX)
			{
			// The beginning of the range and the hyphen symbol have been read
			// already, just need to store the end of the range at this point
			//
			rangeEnd = GetTokenValue();
			}

		else if (insideRange)
			{
			// If insideRange is true, we must have already stored the beginning
			// of the range and read in the hyphen character. We would not have
			// made it this far if we had also not already stored the range end.
			// Thus, we can go ahead and add the range.
			//
			ok = AddRange(rangeBegin, rangeEnd, modValue);
			modValue = 1;

			if (!ok)
				{
				block.errormsg = "Character number out of range (or range incorrectly specified) in set specification";
				throw NxsException(block.errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
				}

			// We have actually already read in the next token, so deal with it
			// now so that we don't end up skipping a token
			//
			if (token.Equals(";"))
				{
				retval = true;
				break;
				}
			else if (token.Equals(","))
				{
				break;
				}

			rangeBegin = GetTokenValue();
			rangeEnd = UINT_MAX;
			insideRange = false;
			}

		else if (rangeBegin != UINT_MAX)
			{
			// If we were inside a range, we would have not gotten this far.
			// If not in a range, we are either getting ready to begin a new
			// range or have previously read in a single value. Handle the
			// latter possibility here.
			//
			ok = AddRange(rangeBegin, rangeBegin, modValue);
			modValue = 1;

			if (!ok)
				{
				block.errormsg = "Number out of range (or range incorrectly specified) in set specification";
				throw NxsException(block.errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
				}

			if (token.Equals(";"))
				{
				retval = true;
				break;
				}
			else if (token.Equals(","))
				{
				break;
				}

			rangeBegin = GetTokenValue();
			rangeEnd = UINT_MAX;
			}

		else if (token.Equals(";"))
			{
			retval = true;
			break;
			}

		else if (token.Equals(","))
			{
			break;
			}

		else if (token.Equals("ALL"))
			{
			rangeBegin = 1;
			rangeEnd = max;
			ok = AddRange(rangeBegin, rangeEnd);
			}

		else
			{
			// Can only get here if rangeBegin still equals UINT_MAX and thus we
			// are reading in the very first token and that token is neither
			// the word "all" nor is it a semicolon
			//
			rangeBegin = GetTokenValue();
			rangeEnd = UINT_MAX;
			}
		}

	return retval;
	}
