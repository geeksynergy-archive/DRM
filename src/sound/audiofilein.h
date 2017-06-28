/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007, 2012, 2013
 *
 * Author(s):
 *	Julian Cable, David Flamand
 *
 * Decription:
 *  Read a file at the correct rate
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

#ifndef _AUDIOFILEIN
#define _AUDIOFILEIN

#include "soundinterface.h"
#include "../util/Pacer.h"
#include "../resample/Resample.h"

/* Classes ********************************************************************/
class CAudioFileIn : public CSoundInInterface
{
public:
    CAudioFileIn();
    virtual ~CAudioFileIn();

    virtual void		Enumerate(vector<string>&, vector<string>&) {}
    virtual void		SetDev(string sNewDevice) {sCurrentDevice = sNewDevice;}
    virtual string		GetDev() {return sCurrentDevice;}
    virtual void		SetFileName(const string& strFileName);
    virtual int			GetSampleRate() {return iRequestedSampleRate;};

    virtual _BOOLEAN	Init(int iNewSampleRate, int iNewBufferSize, _BOOLEAN bNewBlocking);
    virtual _BOOLEAN 	Read(CVector<short>& psData);
    virtual void 		Close();

protected:
    string				strInFileName;
    CVector<_REAL>		vecTempResBufIn;
    CVector<_REAL>		vecTempResBufOut;
    enum { fmt_txt, fmt_raw_mono, fmt_raw_stereo, fmt_other } eFmt;
    FILE*				pFileReceiver;
    int					iSampleRate;
    int					iRequestedSampleRate;
    int					iBufferSize;
    int					iFileSampleRate;
    int					iFileChannels;
    CPacer*				pacer;
    CAudioResample*		ResampleObjL;
    CAudioResample*		ResampleObjR;
    short*				buffer;
    int					iOutBlockSize;
    string				sCurrentDevice;
};

#endif
