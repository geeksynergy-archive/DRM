/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Alexander Kurpiers
 *
 * Decription:
 * Linux sound interface
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

#ifndef _SOUNDIN_H
#define _SOUNDIN_H

#include "../soundinterface.h"
#include "../util/Buffer.h"
#include "soundcommon.h"

/* Definitions ****************************************************************/
#define RECORDING_CHANNEL		0		/* 0: Left, 1: Right */

#define SOUNDBUFLEN 102400

#define FRAGSIZE 8192
//#define FRAGSIZE 1024

/* Classes ********************************************************************/
class CSoundIn : public CSoundInInterface
{
public:
    CSoundIn();
    virtual ~CSoundIn() {}

    virtual void				Enumerate(vector<string>& choices) {
        choices = names;
    }
    virtual void				SetDev(int iNewDevice);
    virtual int					GetDev();

    void Init(int iNewBufferSize, _BOOLEAN bNewBlocking = TRUE);
    _BOOLEAN Read(CVector<short>& psData);
    void Close();

protected:
    void Init_HW();
    int read_HW( void * recbuf, int size);
    void close_HW( void );

    int 	iBufferSize, iInBufferSize;
    short int *tmprecbuf;
    _BOOLEAN	bBlockingRec;
    vector<string> devices;

    class CRecThread : public CThread
    {
    public:
        virtual ~CRecThread() {}
        virtual void run();
        CSoundBuf SoundBuf;
        CSoundIn*	pSoundIn;
    protected:
        _SAMPLE	tmprecbuf[NUM_IN_CHANNELS * FRAGSIZE];
    } RecThread;

protected:
    vector<string> names;
    _BOOLEAN bChangDev;
    int	iCurrentDevice;
#ifdef USE_ALSA
    snd_pcm_t *handle;
#endif
#ifdef USE_OSS
    COSSDev dev;
#endif
};

#endif
