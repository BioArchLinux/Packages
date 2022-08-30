//	Copyright (C) 1999-2003 Paul O. Lewis and Mark T. Holder
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


/* The phylip parser is based on code from PHYLIP which is:
 * version 3.6. (c) Copyright 1993-2004 by the University of Washington.
 * Written by Joseph Felsenstein, Akiko Fuseki, Sean Lamont, Andrew Keeffe,
 * Mike Palczewski, Doug Buxton and Dan Fineman. Permission is granted to
 * copy and use this program provided no fee is charged for it and provided
 * that this copyright notice is not removed.
 */


/*
 * This file is a phylip to NEXUS converter that consists of code from PHYLIP
 * 3.6.5 (see copyright above) tweaked by Mark Holder to output NEXUS.
 *
 * This file was created by concatenating the headers, and .c files:
 *	phylip.h,
 *	seq.h,
 *	discrete.h
 *	phylip.c
 *	seq.c,
 *	pars.c, and
 *	discrete.c concatenated
 *	followed by removal of code that is unused in this simple program, and
 * the addition of routines for printing out NEXUS.
 */


#include <cassert>
#include <fstream>
#include <algorithm>
#include "ncl/nxsmultiformat.h"
#include "ncl/nxsstring.h"

const unsigned MAX_BUFFER_SIZE = 0x80000;







const char * gFormatNames[] = {	"nexus",
								"dnafasta",
								"aafasta",
								"rnafasta",
								"dnaphylip",
								"rnaphylip",
								"aaphylip",
								"discretephylip",
								"dnaphylipinterleaved",
								"rnaphylipinterleaved",
								"aaphylipinterleaved",
								"discretephylipinterleaved",
								"dnarelaxedphylip",
								"rnarelaxedphylip",
								"aarelaxedphylip",
								"discreterelaxedphylip",
								"dnarelaxedphylipinterleaved",
								"rnarelaxedphylipinterleaved",
								"aarelaxedphylipinterleaved",
								"discreterelaxedphylipinterleaved",
								"dnaaln",
								"rnaaln",
								"aaaln",
								"phyliptree",
								"relaxedphyliptree",
								"nexml",
								"dnafin",
								"aafin",
								"rnafin"
							};
const unsigned gNumFormats = 29;
const unsigned PHYLIP_NMLNGTH = 10;

std::vector<std::string> MultiFormatReader::getFormatNames()
	{
	std::vector<std::string> v(gNumFormats);
	for (unsigned i = 0; i < gNumFormats; ++i)
		{
		v[i] = std::string(gFormatNames[i]);
		}
	return v;
	}



class FileToCharBuffer
{
		char prevChar;
		std::istream & inf;
		unsigned remaining;
		unsigned pos;
	public:
		unsigned totalSize;
	protected:
		unsigned lineNumber;
		unsigned prevNewlinePos;
	public:
		/* reads at most MAX_BUFFER_SIZE characters from inf into the buffer that is
		returned. The caller must delete the buffer.  On exit `len` will store the
		length of the buffer.
		*/

		FileToCharBuffer(std::istream & instream);

		/* reads at most maxLen characters from `inf` into the `buffer`
		Returns false if no characters are read.
		If true is returned then `maxLen` will indicate the number of characters read.
		*/
		bool refillBuffer(unsigned offset);
		char current() const
			{
			return buffer[pos];
			}
		bool advance()
			{
			if (pos + 1 >= inbuffer)
				{
				if (!refillBuffer(0))
					return false;
				}
			else
				++pos;
			const char c = current();
			if (c == 13)
				{
				++lineNumber;
				prevNewlinePos = position();
				}
			else if (c == 10)
				{
				if (prev() != 13)
					++lineNumber;
				prevNewlinePos = position();
				}
			return true;
			}
		bool advance_then_store(char & c)
			{
			if (!this->advance())
				return false;
			c = this->current();
			return true;
			}
		bool skip_to_beginning_of_line(char & next);
		char prev() const
			{
			if (pos == 0)
				return prevChar;
			return buffer[pos - 1];
			}
		~FileToCharBuffer()
			{
			delete [] buffer;
			}
		unsigned position() const
			{
			return totalSize +  pos - remaining - inbuffer;
			}
		unsigned line() const
			{
			return lineNumber;
			}
		unsigned column() const
			{
			unsigned p = position();
			if (p < prevNewlinePos)
				return 0;
			return p - prevNewlinePos;
			}
		char * buffer;
		unsigned inbuffer;

};


void MultiFormatReader::ReadFilepath(const char * filepath, const char * formatName)
	{
	if (!formatName)
		return;
	DataFormatType f =  formatNameToCode(formatName);
	if (f == UNSUPPORTED_FORMAT)
		{
		NxsString m;
		m << "Unsupported format: " << formatName;
		throw NxsException(m);
		}
	this->ReadFilepath(filepath, f);
	}

void MultiFormatReader::ReadStream(std::istream & inf, const char * formatName)
	{
	if (!formatName)
		return;
	DataFormatType f =  formatNameToCode(formatName);
	if (f == UNSUPPORTED_FORMAT)
		{
		NxsString m;
		m << "Unsupported format: " << formatName;
		throw NxsException(m);
		}
	this->ReadStream(inf, f);
	}

FileToCharBuffer::FileToCharBuffer(std::istream & instream)
	:prevChar('\n'),
	inf(instream),
	pos(0),
	totalSize(0),
	lineNumber(1),
	prevNewlinePos(0),
	buffer(0L)
	{
	std::streampos s = inf.tellg();
	inf.seekg (0, std::ios::end);
	std::streampos e = inf.tellg();
	if (e <= s)
		{
		inbuffer = 0;
		remaining = 0;
		return;
		}
	inf.seekg(s);
	totalSize = static_cast<unsigned>(e - s);
	inbuffer = std::min(MAX_BUFFER_SIZE, totalSize);
	remaining = totalSize - inbuffer;
	buffer = new char [inbuffer];
	inf.read(buffer, inbuffer);
	const char c = current();
	if (c == 13)
		{
		++lineNumber;
		prevNewlinePos = position();
		}
	else if (c == 10)
		{
		if (prev() != 13)
			++lineNumber;
		prevNewlinePos = position();
		}
	}

bool FileToCharBuffer::refillBuffer(unsigned offset)
	{
	if (remaining  == 0)
		return false;
	if (offset == 0)
		prevChar = buffer[inbuffer-1];
	inbuffer = std::min(inbuffer - offset, remaining);
	remaining -= inbuffer;
	inf.read(buffer + offset, inbuffer);
	pos = offset;
	return true;
	}


MultiFormatReader::DataFormatType MultiFormatReader::formatNameToCode(const std::string &s)
	{
	std::string l(s);
	NxsString::to_lower(l);
	int ind = NxsString::index_in_array(l, gFormatNames, gNumFormats);
	if (ind < 0)
		return UNSUPPORTED_FORMAT;
	NCL_ASSERT(ind < UNSUPPORTED_FORMAT);
	return MultiFormatReader::DataFormatType(ind);
	}



/* Assumes that `contents` was returned from readFileToMemory() has been called
	with `inf` and the `len` refers the size of the buffer allocated by
	readFileToMemory
*/
bool  MultiFormatReader::readFastaSequences(
	FileToCharBuffer & ftcb,
	const NxsDiscreteDatatypeMapper &dm,
	std::list<std::string> & taxaNames,
	std::list<NxsDiscreteStateRow> & matList,
	size_t & longest)
	{
	NCL_ASSERT(ftcb.buffer);
	NxsString err;
	for (;;)
		{
		if (ftcb.current() == '>' && ( ftcb.prev() == '\n' ||  ftcb.prev() == '\r'))
			{
			std::string n;
			if (!ftcb.advance())
				break;
			for (;;)
				{
				char c = ftcb.current();
				if (c == '\n' || c == '\r')
					break;
				n.append(1, c);
				if (!ftcb.advance())
					break;
				}
			std::string nameStripped = NxsString::strip_surrounding_whitespace(n);
			if (this->coerceUnderscoresToSpaces)
			    {
			    NxsString x(nameStripped.c_str());
			    x.UnderscoresToBlanks();
			    nameStripped = x;
			    }
			taxaNames.push_back(nameStripped);

			matList.push_back(NxsDiscreteStateRow());
			if (!ftcb.advance())
				break;
			NxsDiscreteStateRow & row = *(matList.rbegin());
			row.reserve(longest);
			for (;;)
				{
				char c = ftcb.current();
				if (c == '>' && (ftcb.prev() == '\n' || ftcb.prev() == '\r'))
					break;
				if (isgraph(c))
					{
					NxsDiscreteStateCell stateCode = dm.GetStateCodeStored(c);
					if (stateCode == NXS_INVALID_STATE_CODE)
						{
						err << "Illegal state code \"" << c << "\" found when reading character " << (unsigned) row.size() << " for taxon " << n;
						throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
						}
					row.push_back(stateCode);
					}
				if (!ftcb.advance())
					break;
				}
			longest = std::max(longest, row.size());
			}
		else 
			{
			if (isgraph(ftcb.current()))
				{
				err << "Illegal non-whitespace occurring outside of a name/sequence pair.  Expecting the first name to startwith > but found \"" << ftcb.current() << "\".";
				throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
				}
			if (!ftcb.advance())
				break;
			}
		}
	// pad with missing data to make even rows
	std::list<NxsDiscreteStateRow>::iterator sIt = matList.begin();
	bool allSameLength = true;
	for (; sIt != matList.end(); ++sIt)
		{
		NxsDiscreteStateRow & row = *sIt;
		if (row.size() < longest)
			{
			allSameLength = false;
			break;
			}
		}
	return allSameLength;
	}

std::string  MultiFormatReader::readPhylipName(FileToCharBuffer & ftcb, unsigned i, bool relaxedNames)
	{
	NxsString err;
	std::string n;
	if (relaxedNames)
		{
		do {
			n.append(1,ftcb.current());
			if (!ftcb.advance())
				{
				err << "End of file found when reading the name of taxon " << i+1 << ", \"" << n << "\"";
				throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
				}
			}
		while (isgraph(ftcb.current()));
		while (!isgraph(ftcb.current()))
			{
			if (!ftcb.advance())
				{
				err << "End of file found when expecting the beginning of the data for taxon " << i+1 << ", \"" << n << "\"";
				throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
				}
			}
		}
	else
		{
		std::string ws;
		for (unsigned letter = 0; letter < PHYLIP_NMLNGTH; ++letter)
			{
			char c = ftcb.current();
			if (isgraph(c))
				{
				n.append(ws);
				n.append(1,c);
				ws.clear();
				}
			else
				ws.append(1, c);
			if (!ftcb.advance())
				{
				err << "End of file found when reading the name for taxon " << i+1 << ", \"" << n << "\"";
				throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
				}
			}
		}
    if (this->coerceUnderscoresToSpaces)
        {
        NxsString x(n.c_str());
        x.UnderscoresToBlanks();
        n = x;
        }

	return n;
	}

void  MultiFormatReader::readPhylipData(
	FileToCharBuffer & ftcb,
	const NxsDiscreteDatatypeMapper &dm,
	std::list<std::string> & taxaNames,
	std::list<NxsDiscreteStateRow> & matList,
	const unsigned n_taxa,
	const unsigned n_char,
	bool relaxedNames)
	{
	NCL_ASSERT(n_taxa > 0 && n_char > 0);
	NxsString err;
	matList.clear();
	matList.assign(n_taxa, NxsDiscreteStateRow(n_char, NXS_INVALID_STATE_CODE));
	std::list<NxsDiscreteStateRow>::iterator mIt = matList.begin();
	while (!isgraph(ftcb.current()))
		{
		if (!ftcb.advance())
			goto funcExit;
		}

	for (unsigned i = 0; i < n_taxa; ++i)
		{
		std::string n = readPhylipName(ftcb, i, relaxedNames);
        taxaNames.push_back(n);
		NCL_ASSERT(mIt != matList.end());
		NxsDiscreteStateRow & row = *mIt++;
		for (unsigned j = 0; j < n_char; ++j)
			{
			bool readChar = false;
			for (;;)
				{
				const char c = ftcb.current();
				if (isgraph(c))
					{
					if (isdigit(c))// I don't know why PHYLIP allows digits in the midst of the sequence, but it seems to.
						{
						err << "Number encountered (and ignored) within sequence for taxon " << n;
						NexusWarn(err, NxsReader::PROBABLY_INCORRECT_CONTENT_WARNING, ftcb.position(), ftcb.line(), ftcb.column());
						err.clear();
						}
					else
						{
						const NxsDiscreteStateCell stateCode = dm.GetStateCodeStored(c);
						if (stateCode == NXS_INVALID_STATE_CODE)
							{
							if (c == '.')
								{
								if (i == 0)
									{
									err << "Illegal match character state code  \".\" found in the first taxon for character " << j + 1 ;
									throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
									}
								NxsDiscreteStateRow & firstRow = *(matList.begin());
								row[j] = firstRow.at(j);
								}
							else
								{
								err << "Illegal state code \"" << c << "\" found when reading site " << j + 1 << " for taxon " << n;
								throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
								}
							}
						else
							row[j] = stateCode;
						readChar = true;
						}
					}
				if (!ftcb.advance())
					goto funcExit;
				if (readChar)
					break;
				}
			}
		char f = ftcb.current();
		while (f != '\r' && f != '\n')
			{
			if (isgraph(f))
				{
				err << "Sequence longer than " << n_char << " found for taxon " << n << ". The character \""<< f << "\" was found, and will be ignored. If the file position of this error corresponds to sequences for the next taxon in the matrix, then that is an indication that the sequences for taxon " << n << " are too short.";
				NexusWarn(err, NxsReader::PROBABLY_INCORRECT_CONTENT_WARNING, ftcb.position(), ftcb.line(), ftcb.column());
				err.clear();
				}
			if (!ftcb.advance())
				goto funcExit;
			f = ftcb.current();
			}
		while (!isgraph(ftcb.current()))
			{
			if (!ftcb.advance())
				goto funcExit;
			}
		}
	funcExit:
		if (matList.size() != n_taxa)
			{
			err << "Unexpected end of file.\nExpecting data for " << n_taxa << " taxa, but only found data for " << (unsigned) matList.size();
			throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
			}
		const NxsDiscreteStateRow & lastRow = *matList.rbegin();
		if (lastRow.size() != n_char)
			{
			err << "Unexpected end of file.\nExpecting " << n_char << " characters for taxon " <<  *(taxaNames.rbegin()) << ", but only found " << (unsigned) lastRow.size() << " characters.";
			throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
			}
	}


void  MultiFormatReader::readInterleavedPhylipData(
	FileToCharBuffer & ftcb,
	const NxsDiscreteDatatypeMapper &dm,
	std::list<std::string> & taxaNames,
	std::list<NxsDiscreteStateRow> & matList,
	const unsigned n_taxa,
	const unsigned n_char,
	bool relaxedNames)
	{
	NCL_ASSERT(n_taxa > 0 && n_char > 0);
	NxsString err;
	matList.clear();
	matList.assign(n_taxa, NxsDiscreteStateRow(n_char, NXS_INVALID_STATE_CODE));
	std::list<NxsDiscreteStateRow>::iterator mIt = matList.begin();
	unsigned startCharIndex = 0;
	unsigned endCharIndex = n_char;
	while (!isgraph(ftcb.current()))
		{
		if (!ftcb.advance())
			goto funcExit;
		}
	while (startCharIndex < n_char)
		{
		for (unsigned i = 0; i < n_taxa; ++i)
			{
			if (startCharIndex == 0)
				{
				std::string n = readPhylipName(ftcb, i, relaxedNames);
				taxaNames.push_back(n);
				}
			if (i == 0)
				mIt = matList.begin();
			NCL_ASSERT(mIt != matList.end());
			NxsDiscreteStateRow & row = *mIt++;
			unsigned j = startCharIndex;
			for (;;)
				{
				const char c = ftcb.current();
				if (isgraph(c))
					{
					if (j >= endCharIndex)
						{
						if (i == 0)
							{
							err << "Too many characters were found for the taxon " << *(taxaNames.begin());
							throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
							}
						else
							{
							std::list<std::string>::const_iterator nIt = taxaNames.begin();
							for (unsigned q = 0; q < i ; ++q)
								++nIt;
							err << "Illegal character \"" << c << "\" found, after all of the data for this interleave page has been read for the taxon " << *nIt;
							throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
							}
						}
					if (isdigit(c))// I don't know why PHYLIP allows digits in the midst of the sequence, but it seems to.
						{
						std::list<std::string>::const_iterator nIt = taxaNames.begin();
						for (unsigned q = 0; q < i ; ++q)
							++nIt;
						err << "Number encountered (and ignored) within sequence for taxon " << *nIt;
						NexusWarn(err, NxsReader::PROBABLY_INCORRECT_CONTENT_WARNING, ftcb.position(), ftcb.line(), ftcb.column());
						err.clear();
						}
					else
						{
						const NxsDiscreteStateCell stateCode = dm.GetStateCodeStored(c);
						if (stateCode == NXS_INVALID_STATE_CODE)
							{
							if (c == '.')
								{
								if (i == 0)
									{
									err << "Illegal match character state code  \".\" found in the first taxon for character " << j + 1 ;
									throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
									}
								NxsDiscreteStateRow & firstRow = *(matList.begin());
								row[j] = firstRow.at(j);
								}
							else
								{
								std::list<std::string>::const_iterator nIt = taxaNames.begin();
								for (unsigned q = 0; q < i ; ++q)
									++nIt;
								err << "Illegal state code \"" << c << "\" found when reading site " << j + 1 << " for taxon " << *nIt;
								throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
								}
							}
						else
							row[j] = stateCode;
						j++;
						}
					}
				else if (c == '\r' || c == '\n')
					{
					if (i == 0)
						endCharIndex = j;
					else if (j != endCharIndex)
						{
						std::list<std::string>::const_iterator nIt = taxaNames.begin();
						for (unsigned q = 0; q < i ; ++q)
							++nIt;
						err << "Expecting " << endCharIndex -  startCharIndex << "characters  in this interleave page (based on the number of characters in the first taxon), but only found " << j - startCharIndex << " for taxon " << *nIt;
						throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
						}
					break;
					}
				if (!ftcb.advance())
					goto funcExit;
				}
			while (!isgraph(ftcb.current()))
				{
				if (!ftcb.advance())
					goto funcExit;
				}
			}
		startCharIndex = endCharIndex;
		endCharIndex = n_char;
		}
	funcExit:
		if (matList.size() != n_taxa)
			{
			err << "Unexpected end of file.\nExpecting data for " << n_taxa << " taxa, but only found data for " << (unsigned) matList.size();
			throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
			}
		const NxsDiscreteStateRow & lastRow = *matList.rbegin();
		if (lastRow.size() != n_char)
			{
			err << "Unexpected end of file.\nExpecting " << n_char << " characters for taxon " <<  *(taxaNames.rbegin()) << ", but only found " << (unsigned) lastRow.size() << " characters.";
			throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
			}
	}

bool FileToCharBuffer::skip_to_beginning_of_line(char & next)
	{
	next = this->current();
	for (;;)
		{
		const char c = next;
		if (!this->advance_then_store(next))
			return false;
		if (c == '\n')
			return true;
		if (c == '\r')
			{
			if (next == '\n' && (!this->advance_then_store(next)))
				return false;
			return true;
			}
		}
	}

bool  MultiFormatReader::readAlnData(
	FileToCharBuffer & ftcb,
	const NxsDiscreteDatatypeMapper &dm,
	std::list<std::string> & taxaNames,
	std::list<NxsDiscreteStateRow> & matList)
	{
	taxaNames.clear();
	NCL_ASSERT(ftcb.buffer);
	NxsString err;
	char c;
	if (!ftcb.current())
		throw NxsException("Could not read from file", ftcb.position(), ftcb.line(), ftcb.column());

	c = ftcb.current();
	unsigned index = 0;
	const char * firstWord = "CLUSTAL";
	std::string found;
	const unsigned lenFirstWord = (unsigned const)strlen(firstWord);
	while (index < lenFirstWord)
		{
		found.append(1, c);
		if (toupper(c) != firstWord[index] || !ftcb.advance())
			{
			err << "Expecting file to start \"CLUSTAL\" found \"" << found << "\"";
			throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
			}
		++index;
		c = ftcb.current();
		}
	do {
		if (!ftcb.skip_to_beginning_of_line(c))
			throw NxsException("Expecting multi-line file",ftcb.position(), ftcb.line(), ftcb.column());
	} while (!isgraph(c));
	bool readingFirstBlock = true;
	for (;;)
		{
		// skip lines starting with whitespace
		while (!isgraph(c))
			{
			if (!ftcb.skip_to_beginning_of_line(c))
				{
				if (taxaNames.empty())
					throw NxsException("Sequences after clustal header", ftcb.position(), ftcb.line(), ftcb.column());
				goto funcExit;
				}
			}
		unsigned curr_tax_ind = 0;
		std::list<std::string>::const_iterator taxNameIt;
		std::list<NxsDiscreteStateRow>::iterator matRowIt;
		if (!readingFirstBlock)
			{
			taxNameIt = taxaNames.begin();
			matRowIt = matList.begin();
			}
		NxsDiscreteStateRow * row = NULL;
		// this is the loop over taxa for a "page" of interleave data
		for (;isgraph(c);)
			{
			std::string n;
			for (;;)
				{
				n.append(1, c);
				if (!ftcb.advance())
					break;
				c = ftcb.current();
				if (!isgraph(c))
					break;
				}
			if (readingFirstBlock)
				{
                if (this->coerceUnderscoresToSpaces)
                    {
                    NxsString x(n.c_str());
                    x.UnderscoresToBlanks();
                    n = x;
                    }
				taxaNames.push_back(n);
				matList.push_back(NxsDiscreteStateRow());
				row = &(*(matList.rbegin()));
				}
			else if (curr_tax_ind > taxaNames.size())
				{
				err << "Expecting a line beginning with whitespace (or a blank line), but found \"" << n << "\"";
				throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
				}
			else
				{
				std::string prev_name = *taxNameIt++;
				if (!NxsString::case_insensitive_equals(prev_name.c_str(), n.c_str()))
					{
					err << "Expecting data for taxon # " << (1 + curr_tax_ind) << " \"" << prev_name << "\" but got \"" << n << "\"";
					throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
					}
				row = &(*matRowIt++);
				}


			while (ftcb.advance_then_store(c))
				{
				if (isgraph(c))
					break;
				}
			if  (!isgraph(c))
				{
				err << "Unexpected end-of-file after taxon name \"" << n << "\"";
				throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
				}
			// this is the loop over states for a given taxon
			bool eof = false;
			bool eoseq = false;
			for (;!eoseq;)
				{
				if (isgraph(c))
					{
					if (isdigit(c))
						{
						if (!ftcb.skip_to_beginning_of_line(c))
							{
							if (!readingFirstBlock && (curr_tax_ind + 1) != taxaNames.size())
								{
								err << "Unexpected End of file. Expecting data for " << (unsigned) taxaNames.size() << " sequences";
								throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
								}
							goto funcExit;
							}
						break;
						}
					else
						{
						NxsDiscreteStateCell stateCode = dm.GetStateCodeStored(c);
						if (stateCode == NXS_INVALID_STATE_CODE)
							{
							err << "Illegal state code \"" << c << "\" found when reading character " << (unsigned long) row->size() << " for taxon " << n;
							throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
							}
						row->push_back(stateCode);
						eof = !ftcb.advance_then_store(c);
						}
					}
				if ((!eof) && (!isgraph(c)))
					{
					if (c == '\n')
						{
						eof = !ftcb.advance_then_store(c);
						eoseq = true;
						}
					else if (c == '\r')
						{
						eof = !ftcb.advance_then_store(c);
						if (!eof && c == '\n')
							eof = !ftcb.advance_then_store(c);
						eoseq = true;
						}
					else
						eof = !ftcb.advance_then_store(c);
					}
				if (eof)
					{
					if (!readingFirstBlock && (curr_tax_ind + 1) != taxaNames.size())
						{
						err << "Unexpected End of file. Expecting data for " << (unsigned) taxaNames.size() << " sequences";
						throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
						}
					goto funcExit;
					}
				}
			if (isgraph(c))
				curr_tax_ind++;
			else
				{
				if (!readingFirstBlock && (1 + curr_tax_ind) != taxaNames.size())
					{
					err << "Unexpected line beginning with whitespace. Expecting data for " << (unsigned) taxaNames.size() << " sequences";
					throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
					}
				curr_tax_ind = 0;
				readingFirstBlock = false;
				}
			}
		}

	funcExit:
		// pad with missing data to make even rows
		std::list<NxsDiscreteStateRow>::iterator sIt = matList.begin();
		long longest = -1;
		for (; sIt != matList.end(); ++sIt)
			{
			NxsDiscreteStateRow & row = *sIt;
			if (longest == -1)
				longest = (long) row.size();
			else if (longest != (long) row.size())
				return false;
			}
		return true;
	}

void MultiFormatReader::addTaxaNames(const std::list<std::string> & taxaNames, NxsTaxaBlockAPI * taxa)
	{
	NCL_ASSERT(taxa);
	std::list<std::string>::const_iterator nIt = taxaNames.begin();

	std::vector<NxsNameToNameTrans> nameTrans;
	bool nameTransNeeded = false;
	NxsString t;

	for (; nIt != taxaNames.end(); ++nIt)
		{
		std::string name = *nIt;
		NxsNameToNameTrans trans(name, name);
		for (unsigned i = 1; ; ++i)
			{
			try {
				taxa->AddTaxonLabel(name);
				break;
				}
			catch (DuplicatedLabelNxsException & x)
				{
				if (!this->conversionOutputRecord.addNumbersToDisambiguateNames)
					throw;
				nameTransNeeded = true;
				t.assign(*nIt);
				t << i;
				trans.second = t;
				name = t;
				}
			}
		if (this->conversionOutputRecord.addNumbersToDisambiguateNames)
			nameTrans.push_back(trans);
		}


	// write out a name translation file if we need to
	if (nameTransNeeded && this->conversionOutputRecord.writeNameTranslationFile)
		this->conversionOutputRecord.writeNameTranslation(nameTrans, taxa);
	}

void MultiFormatReader::moveDataToMatrix(std::list<NxsDiscreteStateRow> & matList,  NxsDiscreteStateMatrix &mat)
	{
	mat.clear();
	mat.resize(matList.size());
	NxsDiscreteStateMatrix::iterator dIt = mat.begin();
	std::list<NxsDiscreteStateRow>::iterator sIt = matList.begin();
	for (; sIt != matList.end(); ++sIt, ++dIt)
		{
		NxsDiscreteStateRow & source = *sIt;
		NxsDiscreteStateRow & dest = *dIt;
		dest.swap(source);
		}
	}

void  MultiFormatReader::moveDataToDataBlock(const std::list<std::string> & taxaNames, std::list<NxsDiscreteStateRow> & matList, const unsigned nchar, NxsDataBlock * dataB)
	{
	NCL_ASSERT(dataB);
	NxsString d;
	d << "Dimensions ntax = " << (unsigned) matList.size() << " nchar = " << nchar << " ; ";
	std::istringstream fakeDimStream(d);
	NxsToken fakeDimToken(fakeDimStream);
	NxsString newTaxLabel("NewTaxa");
	NxsString ntaxLabel("NTax");
	NxsString ncharLabel("NChar");
	dataB->HandleDimensions(fakeDimToken, newTaxLabel, ntaxLabel, ncharLabel);

	NCL_ASSERT(dataB->taxa);
	addTaxaNames(taxaNames, dataB->taxa);

	moveDataToMatrix(matList, dataB->discreteMatrix);
	}

void  MultiFormatReader::moveDataToUnalignedBlock(const std::list<std::string> & taxaNames, std::list<NxsDiscreteStateRow> & matList, NxsUnalignedBlock * uB)
	{
	NCL_ASSERT(uB);
	NxsString d;
	d << "Dimensions NewTaxa ntax = " << (unsigned) matList.size() << " ; ";
	std::istringstream fakeDimStream(d);
	NxsToken fakeDimToken(fakeDimStream);
	uB->HandleDimensions(fakeDimToken);

	NCL_ASSERT(uB->taxa);
	addTaxaNames(taxaNames, uB->taxa);

	moveDataToMatrix(matList, uB->uMatrix);
	}

void  MultiFormatReader::readFastaFile(std::istream & inf, NxsCharactersBlock::DataTypesEnum dt)
	{
	NxsString blockID("DATA");
	NxsBlock *nb = cloneFactory.GetBlockReaderForID(blockID, this, NULL);
	NCL_ASSERT(nb);
	if (!nb)
		return;
	nb->SetNexus(this);

	NxsDataBlock * dataB = static_cast<NxsDataBlock *>(nb); // this should be safe because we know that the PublicNexusReader has a DataBlock assigned to "DATA" -- unless the caller has replaced that clone template (gulp)
	FileToCharBuffer ftcb(inf);
	if (ftcb.buffer)
		{
		dataB->Reset();
		dataB->datatype = dt;
		dataB->ResetSymbols();
		dataB->gap = '-';
		NxsPartition dtParts;
		std::vector<NxsCharactersBlock::DataTypesEnum> dtv;
		dataB->CreateDatatypeMapperObjects(dtParts, dtv);

		const NxsDiscreteDatatypeMapper * dm = dataB->GetDatatypeMapperForChar(0);

		std::list<std::string> taxaNames;
		std::list<NxsDiscreteStateRow> matList;
		size_t longest = 0;
		bool aligned = true;
		try {
			aligned = readFastaSequences(ftcb, *dm, taxaNames, matList, longest);
			}
		catch (...)
			{
			cloneFactory.BlockError(dataB);
			throw;
			}

		if (aligned)
			{
			moveDataToDataBlock(taxaNames, matList, (unsigned int)longest, dataB);
			BlockReadHook(blockID, dataB);
			}
		else
			{
			cloneFactory.BlockError(dataB);
			blockID.assign("UNALIGNED");
			NxsBlock * nub = cloneFactory.GetBlockReaderForID(blockID, this, NULL);
			if (!nub)
				{
				NCL_ASSERT(nub);
				return;
				}
			nub->SetNexus(this);

			NxsUnalignedBlock * unalignedB = static_cast<NxsUnalignedBlock *>(nub); // this should be safe because we know that the PublicNexusReader has a DataBlock assigned to "DATA" -- unless the caller has replaced that clone template (gulp)
			unalignedB->Reset();
			unalignedB->datatype = dt;
			unalignedB->ResetSymbols();
			unalignedB->gap = '-';
			unalignedB->ResetDatatypeMapper();
			moveDataToUnalignedBlock(taxaNames, matList, unalignedB);
			BlockReadHook(blockID, unalignedB);
			}
		}
	else
		{
		cloneFactory.BlockError(dataB);
		NxsString err;
		err << "No Data read -- file appears to be empty";
		this->NexusError(err, 0, -1, -1);
		}
	}


/* Assumes that `contents` was returned from readFileToMemory() has been called
	with `inf` and the `len` refers the size of the buffer allocated by
	readFileToMemory
*/
bool  MultiFormatReader::readFinSequences(
	FileToCharBuffer & ftcb,
	NxsDiscreteDatatypeMapper &dm,
	std::list<std::string> & taxaNames,
	std::list<NxsDiscreteStateRow> & matList,
	size_t & longest)
	{
	NCL_ASSERT(ftcb.buffer);
	NxsString err;

	std::string firstLine;
	for (;;)
		{
		char c = ftcb.current();
		if (c == '\n' || c == '\r')
			break;
		firstLine.append(1, c);
		if (!ftcb.advance())
			break;
		}
	std::string sfl = NxsString::strip_surrounding_whitespace(firstLine);
	if (!NxsString::case_insensitive_equals(sfl.c_str(), "label data"))
		{
		err << "Expecting the first line of the file to contain just the words \"label data\", but found \"" << sfl << '\"';
		throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
		}

	for (;;)
		{
		const char cc = ftcb.current();
		if (!isgraph(cc))
			{
			if (!ftcb.advance())
				break;
			}
		else if (cc == '@')
			break;
		else 
			{
			std::string name;
			bool commentLine= false;
			if (ftcb.current() == '/')
				{
				if (!ftcb.advance())
					{
					err << "Unexpected end of file after / character";
					throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
					}
				if (ftcb.current() == '*')
					{
					commentLine = true;
					bool prevStar = false;
					for (;;)
						{
						if (!ftcb.advance())
							{
							err << "Unexpected end of file in comment";
							throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
							}
						char cmtc = ftcb.current();
						if (prevStar && cmtc == '/')
							break;
						prevStar = (cmtc == '*');
						}
					if (!ftcb.advance())
						break;
					}
				else
					name.append(1, '/');
				}
			if (commentLine)
				continue;
			// read taxon name -- no escaping of characters will be done
			for (;;)
				{
				char c = ftcb.current();
				if (!isgraph(c))
					break;
				name.append(1, c);
				if (!ftcb.advance())
					break;
				}
			// skip ws
			for (;;)
				{
				char sc = ftcb.current();
				if (isgraph(sc))
					break;
				if (sc == '\n' || sc == '\r' || !ftcb.advance())
					{
					err << "Unexpected end of line (or end of file).  Expecting sequence for " << name;
					throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
					}
				}
			if (this->coerceUnderscoresToSpaces)
			    {
			    NxsString x(name.c_str());
			    x.UnderscoresToBlanks();
			    name = x;
			    }
			taxaNames.push_back(name);
			matList.push_back(NxsDiscreteStateRow());
			NxsDiscreteStateRow & row = *(matList.rbegin());
			row.reserve(longest);
			// read sequence
			for (;;)
				{
				char seqc = ftcb.current();
				if (isgraph(seqc))
					{
					NxsDiscreteStateCell stateCode;
					if (seqc == '[')
						{
						std::string recoded;
						recoded.append(1, '{');
						if (!ftcb.advance())
							{
							err << "Unexpected end of file is [ group!";
							throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
							}
						while (ftcb.current() != ']')
							{
							recoded.append(1, ftcb.current());
							if (!ftcb.advance())
								{
								err << "Unexpected end of file is [ group!";
								throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
								}
							}
						recoded.append(1, '}');
						try{
							NxsString nn;
							nn << name;
							stateCode = dm.StateCodeForNexusMultiStateSet('\0',
  																	  recoded,
  																	  0L,
  																	  (unsigned int)taxaNames.size(),
  																	  (unsigned int)row.size(),
  																	  0L,
  																	  nn);
							}
						catch (NxsException & x)
							{
							x.addPositionInfo(ftcb.position(), ftcb.line(), ftcb.column());
							throw x;
							}
						}
					else
						{
						stateCode = dm.GetStateCodeStored(seqc);
						if (stateCode == NXS_INVALID_STATE_CODE)
							{
							err << "Illegal state code \"" << seqc << "\" found when reading character " << (unsigned) row.size() << " for taxon \"" << name << "\".";
							throw NxsException(err, ftcb.position(), ftcb.line(), ftcb.column());
							}
						}
					row.push_back(stateCode);
					}
				else if (seqc == '\n' || seqc == '\r')
					break;
				if (!ftcb.advance())
					break;
				}
			longest = std::max(longest, row.size());
			}
		}
	// pad with missing data to make even rows
	std::list<NxsDiscreteStateRow>::iterator sIt = matList.begin();
	bool allSameLength = true;
	for (; sIt != matList.end(); ++sIt)
		{
		NxsDiscreteStateRow & row = *sIt;
		if (row.size() < longest)
			{
			allSameLength = false;
			break;
			}
		}
	return allSameLength;
	}


void  MultiFormatReader::readFinFile(std::istream & inf, NxsCharactersBlock::DataTypesEnum dt)
	{
	NxsString blockID("DATA");
	NxsBlock *nb = cloneFactory.GetBlockReaderForID(blockID, this, NULL);
	NCL_ASSERT(nb);
	if (!nb)
		return;
	nb->SetNexus(this);

	NxsDataBlock * dataB = static_cast<NxsDataBlock *>(nb); // this should be safe because we know that the PublicNexusReader has a DataBlock assigned to "DATA" -- unless the caller has replaced that clone template (gulp)
	FileToCharBuffer ftcb(inf);
	if (ftcb.buffer)
		{
		dataB->Reset();
		dataB->datatype = dt;
		dataB->ResetSymbols();
		dataB->gap = '-';
		NxsPartition dtParts;
		std::vector<NxsCharactersBlock::DataTypesEnum> dtv;
		dataB->CreateDatatypeMapperObjects(dtParts, dtv);

		NxsDiscreteDatatypeMapper * dm = dataB->GetMutableDatatypeMapperForChar(0);

		std::list<std::string> taxaNames;
		std::list<NxsDiscreteStateRow> matList;
		size_t longest = 0;
		bool aligned = true;
		try {
			aligned = readFinSequences(ftcb, *dm, taxaNames, matList, longest);
			}
		catch (...)
			{
			cloneFactory.BlockError(dataB);
			throw;
			}

		if (aligned)
			{
			moveDataToDataBlock(taxaNames, matList, (unsigned int)longest, dataB);
			BlockReadHook(blockID, dataB);
			}
		else
			{
			cloneFactory.BlockError(dataB);
			blockID.assign("UNALIGNED");
			NxsBlock * nub = cloneFactory.GetBlockReaderForID(blockID, this, NULL);
			if (!nub)
				{
				NCL_ASSERT(nub);
				return;
				}
			nub->SetNexus(this);

			NxsUnalignedBlock * unalignedB = static_cast<NxsUnalignedBlock *>(nub); // this should be safe because we know that the PublicNexusReader has a DataBlock assigned to "DATA" -- unless the caller has replaced that clone template (gulp)
			unalignedB->Reset();
			unalignedB->datatype = dt;
			unalignedB->ResetSymbols();
			unalignedB->ResetDatatypeMapper();
			moveDataToUnalignedBlock(taxaNames, matList, unalignedB);
			BlockReadHook(blockID, unalignedB);
			}
		}
	else
		{
		cloneFactory.BlockError(dataB);
		NxsString err;
		err << "No Data read -- file appears to be empty";
		this->NexusError(err, 0, -1, -1);
		}
	}

void  MultiFormatReader::ReadFilepath(const char * filepath, DataFormatType format)
	{
	if (format == NEXUS_FORMAT)
		{
		NxsReader::ReadFilepath(filepath);
		}
	else
		{
		std::ifstream inf;
		try{
			inf.open(filepath, std::ios::binary);
			if (!inf.good())
				{
				NxsString err;
				err << "Could not open the file \"" << filepath <<"\"";
				this->NexusError(err, 0, -1, -1);
				}
			else
				this->ReadStream(inf, format, filepath);
			}
		catch (NxsException & x)
			{
			this->NexusError(x.msg, x.pos, x.line, x.col);
			}
		catch (...)
			{
			NxsString err;
			err << "Unknown error occurred while reading \"" << filepath <<"\"." ;
			this->NexusError(err, 0, -1, -1);
			}
		
		}
	}

void  MultiFormatReader::ReadStream(std::istream & inf, DataFormatType format, const char * filepath)
	{
	if (format == NEXUS_FORMAT)
		{
		NxsReader::ReadFilestream(inf);
		}
	else
		{
		if (format == FASTA_DNA_FORMAT)
			readFastaFile(inf, NxsCharactersBlock::dna);
		else if (format == FASTA_RNA_FORMAT)
			readFastaFile(inf, NxsCharactersBlock::rna);
		else if (format == FASTA_AA_FORMAT)
			readFastaFile(inf, NxsCharactersBlock::protein);
		else if (format == PHYLIP_DNA_FORMAT)
			readPhylipFile(inf, NxsCharactersBlock::dna, false, false);
		else if (format == PHYLIP_RNA_FORMAT)
			readPhylipFile(inf, NxsCharactersBlock::rna, false, false);
		else if (format == PHYLIP_AA_FORMAT)
			readPhylipFile(inf, NxsCharactersBlock::protein, false, false);
		else if (format == PHYLIP_DISC_FORMAT)
			readPhylipFile(inf, NxsCharactersBlock::standard, false, false);
		else if (format == INTERLEAVED_PHYLIP_DNA_FORMAT)
			readPhylipFile(inf, NxsCharactersBlock::dna, false, true);
		else if (format == INTERLEAVED_PHYLIP_RNA_FORMAT)
			readPhylipFile(inf, NxsCharactersBlock::rna, false, true);
		else if (format == INTERLEAVED_PHYLIP_AA_FORMAT)
			readPhylipFile(inf, NxsCharactersBlock::protein, false, true);
		else if (format == INTERLEAVED_PHYLIP_DISC_FORMAT)
			readPhylipFile(inf, NxsCharactersBlock::standard, false, true);
		else if (format == RELAXED_PHYLIP_DNA_FORMAT)
			readPhylipFile(inf, NxsCharactersBlock::dna, true, false);
		else if (format == RELAXED_PHYLIP_RNA_FORMAT)
			readPhylipFile(inf, NxsCharactersBlock::rna, true, false);
		else if (format == RELAXED_PHYLIP_AA_FORMAT)
			readPhylipFile(inf, NxsCharactersBlock::protein, true, false);
		else if (format == RELAXED_PHYLIP_DISC_FORMAT)
			readPhylipFile(inf, NxsCharactersBlock::standard, true, false);
		else if (format == INTERLEAVED_RELAXED_PHYLIP_DNA_FORMAT)
			readPhylipFile(inf, NxsCharactersBlock::dna, true, true);
		else if (format == INTERLEAVED_RELAXED_PHYLIP_RNA_FORMAT)
			readPhylipFile(inf, NxsCharactersBlock::rna, true, true);
		else if (format == INTERLEAVED_RELAXED_PHYLIP_AA_FORMAT)
			readPhylipFile(inf, NxsCharactersBlock::protein, true, true);
		else if (format == INTERLEAVED_RELAXED_PHYLIP_DISC_FORMAT)
			readPhylipFile(inf, NxsCharactersBlock::standard, true, true);
		else if (format == ALN_DNA_FORMAT)
			readAlnFile(inf, NxsCharactersBlock::dna);
		else if (format == ALN_RNA_FORMAT)
			readAlnFile(inf, NxsCharactersBlock::rna);
		else if (format == ALN_AA_FORMAT)
			readAlnFile(inf, NxsCharactersBlock::protein);
		else if (format == RELAXED_PHYLIP_TREE_FORMAT)
			readPhylipTreeFile(inf, true);
		else if (format == PHYLIP_TREE_FORMAT)
			readPhylipTreeFile(inf, false);
		else if (format == FIN_DNA_FORMAT)
			readFinFile(inf, NxsCharactersBlock::dna);
		else if (format == FIN_RNA_FORMAT)
			readFinFile(inf, NxsCharactersBlock::rna);
		else if (format == FIN_AA_FORMAT)
			readFinFile(inf, NxsCharactersBlock::protein);
		else
			{
			NxsString m;
			if (filepath)
				m << "The file " << filepath << " is not in a supported format.";
			else
				m << "Unsupported format.";
			NexusError(m, 0, -1, -1);
			return;
			}
		PostExecuteHook();
		}
	}
// More tolerant than strict PHYLIP (tolerates any amount of whitespace before or
// between ntax and nchar.
// throws a NxsException if the header cannot be read.
// returns the file position.
unsigned MultiFormatReader::readPhylipHeader(std::istream & inf, unsigned & ntax, unsigned & nchar)
	{
	int ntaxi = 0;
	int nchari = 0;
	if (inf.good())
		{
		inf >> ntaxi;
		}
	else
		{
		NxsString err("Invalid file stream (this probably indicates an error occurred while opening the file).");
		throw NxsException(err, 0, -1, -1);
		}
	
	if (inf.good())
		inf >> nchari;
	else
		{
		NxsString err("A file error occurred while reading ntax.");
		throw NxsException(err, 0, -1, -1);
		}
	if (!inf.good())
		{
		NxsString err("A file error occurred while reading ntax.");
		throw NxsException(err, 0, -1, -1);
		}
	if (ntaxi < 1 || nchari < 1)
		{
		NxsString err("Expecting the file to start with positive number of taxa then the number of characters.");
		throw NxsException(err, 0, -1, -1);
		}
	ntax = (unsigned) ntaxi;
	nchar = (unsigned) nchari;
	return (unsigned) inf.tellg();
	}

void MultiFormatReader::readPhylipTreeFile(std::istream & inf, bool relaxedNames)
	{
	NxsString blockID("TREES");
	NxsBlock *nb = cloneFactory.GetBlockReaderForID(blockID, this, NULL);
	NCL_ASSERT(nb);
	if (!nb)
		return;
	nb->SetNexus(this);

	/* this should be safe because we know that the PublicNexusReader has a
		NxsTreesBlock assigned to "TREES" -- unless the caller has replaced that
		clone template (gulp)
	*/
	NxsTreesBlock * treesB = static_cast<NxsTreesBlock *>(nb);
	NxsString err;
	try {
		treesB->Reset();
		NxsToken inTokens(inf);
		treesB->ReadPhylipTreeFile(inTokens);
		if (!relaxedNames)
			{
			const NxsTaxaBlockAPI * taxa = treesB->GetTaxaBlockPtr(0L);
			if (!taxa)
				{
				err << "No taxa found in tree description (which probably means that no tree was found).";
				throw NxsException(err, inTokens);
				}
			const std::vector<std::string> l = taxa->GetAllLabels();
			for (std::vector<std::string>::const_iterator lIt = l.begin(); lIt != l.end(); ++lIt)
				{
				if (lIt->length() > PHYLIP_NMLNGTH)
					{
					err << "The taxon label " << *lIt << " has more than the allowed number of charcters (" << PHYLIP_NMLNGTH << ')';
					throw NxsException(err);
					}
				}
			}
		BlockReadHook(blockID, treesB);
		}
	catch (...)
		{
		cloneFactory.BlockError(nb);
		throw;
		}
	}

/* if this returns NULL, then the read failed and gLogMessage will contain
	and error message.
*/
void MultiFormatReader::readAlnFile(std::istream & inf, NxsCharactersBlock::DataTypesEnum dt)
	{
	NxsString blockID("DATA");
	NxsBlock *nb = cloneFactory.GetBlockReaderForID(blockID, this, NULL);
	NCL_ASSERT(nb);
	if (!nb)
		return;
	nb->SetNexus(this);
	/* this should be safe because we know that the PublicNexusReader has a
		DataBlock assigned to "DATA" -- unless the caller has replaced that
		clone template (gulp)
	*/
	NxsDataBlock * dataB = static_cast<NxsDataBlock *>(nb);

	try {
		dataB->Reset();
		dataB->datatype = dt;
		dataB->ResetSymbols();
		dataB->gap = '-';
		NxsPartition dtParts;
		std::vector<NxsCharactersBlock::DataTypesEnum> dtv;
		dataB->CreateDatatypeMapperObjects(dtParts, dtv);

		const NxsDiscreteDatatypeMapper * dm = dataB->GetDatatypeMapperForChar(0);
		NCL_ASSERT(dm);
		FileToCharBuffer ftcb(inf);
		if (ftcb.buffer)
			{
			std::list<std::string> taxaNames;
			std::list<NxsDiscreteStateRow> matList;
			if (!readAlnData(ftcb, *dm, taxaNames, matList))
				throw NxsException("Expecting the same number of characters for all sequences in the ALN file");
			const unsigned nchar = (unsigned const)matList.begin()->size();
			moveDataToDataBlock(taxaNames, matList, nchar, dataB);
			BlockReadHook(blockID, dataB);
			}
		}
	catch (...)
		{
		cloneFactory.BlockError(nb);
		throw;
		}
}

/* if this returns NULL, then the read failed and gLogMessage will contain
	and error message.
*/
void MultiFormatReader::readPhylipFile(std::istream & inf, NxsCharactersBlock::DataTypesEnum dt, bool relaxedNames, bool interleaved)
	{
	NxsString blockID("DATA");
	NxsBlock *nb = cloneFactory.GetBlockReaderForID(blockID, this, NULL);
	NCL_ASSERT(nb);
	if (!nb)
		return;
	nb->SetNexus(this);
	/* this should be safe because we know that the PublicNexusReader has a
		DataBlock assigned to "DATA" -- unless the caller has replaced that
		clone template (gulp)
	*/
	NxsDataBlock * dataB = static_cast<NxsDataBlock *>(nb);

	try {
		dataB->Reset();
		dataB->datatype = dt;
		dataB->ResetSymbols();
		dataB->gap = '-';
		NxsPartition dtParts;
		std::vector<NxsCharactersBlock::DataTypesEnum> dtv;
		dataB->CreateDatatypeMapperObjects(dtParts, dtv);

		const NxsDiscreteDatatypeMapper * dm = dataB->GetDatatypeMapperForChar(0);
		NCL_ASSERT(dm);
		unsigned ntax = 0;
		unsigned nchar = 0;
		unsigned headerLen = readPhylipHeader(inf, ntax, nchar);
		FileToCharBuffer ftcb(inf);
		ftcb.totalSize += headerLen;
		if (ftcb.buffer)
			{
			std::list<std::string> taxaNames;
			std::list<NxsDiscreteStateRow> matList;
			if (interleaved)
				readInterleavedPhylipData(ftcb, *dm, taxaNames, matList, ntax, nchar, relaxedNames);
			else
				readPhylipData(ftcb, *dm, taxaNames, matList, ntax, nchar, relaxedNames);
			moveDataToDataBlock(taxaNames, matList, nchar, dataB);
			BlockReadHook(blockID, dataB);
			}
		}
	catch (...)
		{
		cloneFactory.BlockError(nb);
		throw;
		}
}
