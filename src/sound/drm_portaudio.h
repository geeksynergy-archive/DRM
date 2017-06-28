/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Julian Cable
 *
 * Decription:
 * PortAudio sound interface
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

#ifndef _DRM_PORTAUDIO_H
#define _DRM_PORTAUDIO_H

#include "../sound/soundinterface.h"
#include <portaudio.h>
#include "pa_ringbuffer.h"

class CPaCommon: public CSelectionInterface
{
public:
    CPaCommon(bool);
    virtual 		~CPaCommon();

    virtual void	Enumerate(vector<string>& choices, vector<string>& descriptions);
    virtual void	SetDev(string sNewDevice);
    virtual string	GetDev();

    _BOOLEAN		Init(int iSampleRate, int iNewBufferSize, _BOOLEAN bNewBlocking);
    void			ReInit();
    _BOOLEAN		Read(CVector<short>& psData);
    _BOOLEAN		Write(CVector<short>& psData);
    void			Close();

    PaUtilRingBuffer ringBuffer;
    int xruns;

protected:

    PaStream *stream;
    vector<string> names;
    vector<PaDeviceIndex> devices;
    string dev;
    bool is_capture,blocking,device_changed,xrun;
    int framesPerBuffer;
    int iBufferSize;
    char *ringBufferData;
    double samplerate;

    static int pa_count;
};

class CPaIn: public CSoundInInterface
{
public:
    CPaIn();
    virtual 			~CPaIn();
    virtual void		Enumerate(vector<string>& choices, vector<string>& descriptions) {
        hw.Enumerate(choices, descriptions);
    }
    virtual void		SetDev(string sNewDevice) {
        hw.SetDev(sNewDevice);
    }
    virtual string		GetDev() {
        return hw.GetDev();
    }

    virtual _BOOLEAN	Init(int iSampleRate, int iNewBufferSize, _BOOLEAN bNewBlocking);
    virtual void		Close();
    virtual _BOOLEAN	Read(CVector<short>& psData);

protected:

    CPaCommon hw;
};

class CPaOut: public CSoundOutInterface
{
public:
    CPaOut();
    virtual 			~CPaOut();
    virtual void		Enumerate(vector<string>& choices, vector<string>& descriptions) {
        hw.Enumerate(choices, descriptions);
    }
    virtual void		SetDev(string sNewDevice) {
        hw.SetDev(sNewDevice);
    }
    virtual string		GetDev() {
        return hw.GetDev();
    }

    virtual _BOOLEAN	Init(int iSampleRate, int iNewBufferSize, _BOOLEAN bNewBlocking);
    virtual void		Close();
    virtual _BOOLEAN	Write(CVector<short>& psData);

protected:

    CPaCommon hw;
};

#endif
