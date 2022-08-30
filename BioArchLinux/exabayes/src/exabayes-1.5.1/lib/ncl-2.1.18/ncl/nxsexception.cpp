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

#include "ncl/nxsexception.h"
#include "ncl/nxstoken.h"

using namespace std;
/*!
	Copies 's' to msg and sets line, col and pos to the current line, column and position in the file where parsing
	stopped.
*/
NxsException::NxsException(
  const std::string & s,	/* the message for the user */
  file_pos fp,	/* the current file position */
  long fl,		/* the current file line */
  long fc)		/* the current file column */
	{
	msg.assign(s);
	addPositionInfo(fp, fl, fc);
	}

/*!
	Creates a NxsException object with the specified message, getting file position information from the NxsToken.
*/
NxsException::NxsException(
  const std::string &s,		/* message that describes the error */
  const NxsToken &t)		/* NxsToken that was supplied the last token (the token that caused the error) */
	{
	msg		= NxsString(s.c_str());
	this->addPositionInfo(t);
  	}

NxsException::NxsException(const std::string &s, const ProcessedNxsToken &t)
	{
	msg		= NxsString(s.c_str());
	this->addPositionInfo(t);
	}

NxsException::NxsException(const std::string &s, const NxsTokenPosInfo &t)
	{
	msg		= NxsString(s.c_str());
	this->addPositionInfo(t);
	}

const char * NxsException::nxs_what () const
	{
	std::string m = "Nexus Parsing error: ";
	m.append(msg);
	msg.assign(m);
	if (line >= 0)
		msg << " at line " << line;
	if (col >= 0)
		msg << " column " << col;
	return msg.c_str();
	}

NxsSignalCanceledParseException::NxsSignalCanceledParseException(const std::string & s)
	:NxsException(s)
	{
	msg = "Signal detected during NEXUS class library";
	if (!s.empty())
		msg << " in the processing step: " << s;
	msg << '.';
	}


void NxsException::addPositionInfo(const NxsToken & t) 
	{
	pos = t.GetFilePosition();
	line = t.GetFileLine();
	col = t.GetFileColumn();
	}
void NxsException::addPositionInfo(const ProcessedNxsToken & t)
	{
	pos		= t.GetFilePosition();
	line	= t.GetLineNumber();
	col		= t.GetColumnNumber();
	}
void NxsException::addPositionInfo(const NxsTokenPosInfo & t)
	{
	pos		= t.GetFilePosition();
	line	= t.GetLineNumber();
	col		= t.GetColumnNumber();
	}
void NxsException::addPositionInfo(file_pos fp, long fl, long fc)
	{
	pos		= fp;
	line	= fl;
	col		= fc;
	}
