/******************************************************************************\
 * BBC World Service
 * Copyright (c) 2006
 *
 * Author(s):
 *	Julian Cable
 *
 * Description:
 *	See Reassemble.cpp
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#ifndef REASSEMBLE_H_INCLUDED
#define REASSEMBLE_H_INCLUDED

#include "Vector.h"

class CSegmentTrackerN
{
public:

	CSegmentTrackerN():vecbHaveSegment() { }

	void Reset ()
	{
		vecbHaveSegment.clear ();
	}

	size_t size ()
	{
		return vecbHaveSegment.size ();
	}

	_BOOLEAN Ready ()
	{
		if (vecbHaveSegment.size () == 0)
			return FALSE;
		for (size_t i = 0; i < vecbHaveSegment.size (); i++)
		{
			if (vecbHaveSegment[i] == FALSE)
			{
				return FALSE;
			}
		}
		return TRUE;
	}

	void AddSegment (int iSegNum)
	{
		if ((iSegNum + 1) > int (vecbHaveSegment.size ()))
			vecbHaveSegment.resize (iSegNum + 1, FALSE);
		vecbHaveSegment[iSegNum] = TRUE;
	}

	_BOOLEAN HaveSegment (int iSegNum)
	{
		if (iSegNum < int (vecbHaveSegment.size ()))
			return vecbHaveSegment[iSegNum];
		return FALSE;
	}

protected:
	vector < _BOOLEAN > vecbHaveSegment;
};

/* The base class reassembles chunks of byte vectors into one big vector.
 * It assumes that all chunks except the last chunk are the same size.
 * Usage:
 *
 * CReassemblerN o;
 * o.AddSegment (veco, iSegSize, 1);
 * o.AddSegment (veco, iSegSize, 3);
 * o.AddSegment (veco, iSegSize, 7, TRUE); // last segment, ie there are 8 segments, 0..7
 * o.AddSegment (veco, iSegSize, 2);
 * o.AddSegment (veco, iSegSize, 4);
 * o.AddSegment (veco, iSegSize, 6);
 * o.AddSegment (veco, iSegSize, 5);
 * o.AddSegment (veco, iSegSize, 0);
 * if(o.Ready())
 *   vecoComplete = o.vecData;
 *
 */

class CReassemblerN
{
public:

	CReassemblerN(): vecData(), vecLastSegment(),
		iLastSegmentNum(-1), iLastSegmentSize(-1), iSegmentSize(0),
		Tracker(), bReady(false)
	{
	}

	CReassemblerN (const CReassemblerN & r);

	virtual ~CReassemblerN ()
	{
	}

	CReassemblerN & operator= (const CReassemblerN & r);

	void Reset ()
	{
		vecData.resize (0);
		vecLastSegment.resize (0);
		iLastSegmentNum = -1;
		iLastSegmentSize = -1;
		iSegmentSize = 0;
		Tracker.Reset ();
		bReady = false;
	}

	_BOOLEAN Ready ()
	{
		return bReady;
	}

	void AddSegment (vector<_BYTE> &vecDataIn, int iSegNum, _BOOLEAN bLast);

	vector<_BYTE> vecData;

protected:

	virtual void copyin (vector<_BYTE> &vecDataIn, size_t iSegNum);
	virtual void cachelast (vector<_BYTE> &vecDataIn, size_t iSegSize);
	virtual void copylast ();

	vector<_BYTE> vecLastSegment;
	int iLastSegmentNum;
	int iLastSegmentSize;
	size_t iSegmentSize;
	CSegmentTrackerN Tracker;
	bool bReady;
};

/* CBitReassemblerN uses the Dream CVector class to take a vector of bytes, each holding one bit.
 * It reassembles the segments into another vector, either of bits or of bytes by packing.
 * Packing reduces the amount of storage needed and prepares the output vector to be used in
 * applications expecting vectors of bytes, such as zlib.
 *
 * The major difference to the base class is that the input vector may have a header in front
 * of the data. This will be ignored as long as the bitaccess pointer in the CVector is correctly
 * positioned.
 */

class CBitReassemblerN:public CReassemblerN
{
public:

	CBitReassemblerN():CReassemblerN(),bPack(false)
	{
	}

	CBitReassemblerN(const CBitReassemblerN& r):CReassemblerN(r),bPack(r.bPack)
	{
	}

	inline CBitReassemblerN & operator= (const CBitReassemblerN & r)
	{
		CReassemblerN(*this) = r;
		bPack = r.bPack;
		return *this;
	}

protected:

	virtual void copyin (CVector < _BYTE > &vecDataIn, size_t iSegNum);
	virtual void cachelast (CVector < _BYTE > &vecDataIn, size_t iSegSize);
	virtual void copylast ();

	bool bPack;
};

class CByteReassemblerN:public CBitReassemblerN
{
public:

	CByteReassemblerN():CBitReassemblerN()
	{
		bPack = true;
	}

};

#endif
