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

#ifndef NCL_NXSDISCRETEDATUM_H
#define NCL_NXSDISCRETEDATUM_H

/*!
	Reference to a cell in a DiscreteMatrix.  This class has been deprecated and is retained in NCL >= 2.1 for backward
	compatibility only.
	It no longer stores the data for a cell, but can refer to the cell in a matrix in the context in which the
	matrix is at hand. The only time that NxsDiscreteDatum appears in the public NCL interface is as an argument to
	NxsCharactersBlock::WriteStates().  The new implementation of NxsDiscreteDatum should continue to work in this
	context because the NxsCharactersBlock holds the matrix
*/
class NxsDiscreteDatum
	{
	friend class NxsDiscreteMatrix;
	friend class NxsUnalignedBlock;

	public:
		NxsDiscreteDatum(): taxInd(0), charInd(0){}
		NxsDiscreteDatum(unsigned row, unsigned col): taxInd(row), charInd(col){}
		void				CopyFrom(const NxsDiscreteDatum & other);

		unsigned taxInd; /*row of the matrix */
		unsigned charInd; /*col of the matrix */

	};

typedef NxsDiscreteDatum DiscreteDatum;

/*!
	This assignment operator calls the CopyFrom member function to make a copy of the NxsDiscreteDatum object `other'.
*/
inline void NxsDiscreteDatum::CopyFrom(
  const NxsDiscreteDatum & other)	/* is the object to be copied */
	{
	taxInd = other.taxInd;
	charInd = other.charInd;
	}

#endif
