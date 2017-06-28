/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Julian Cable
 *
 * Decription:
 * ALSA sound interface
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

#ifndef _ALSA_H
#define _ALSA_H

#include "../soundinterface.h"
#include <alsa/asoundlib.h>

class CAlsaSoundIn: public CSoundInInterface
{
public:
    CAlsaSoundIn();
    virtual 			~CAlsaSoundIn();

    virtual void		Enumerate(vector<string>& choices);
    virtual void		SetDev(int iNewDevice);
    virtual int			GetDev();
    virtual void		Init(int iNewBufferSize, _BOOLEAN bNewBlocking = TRUE);
    virtual _BOOLEAN	Read(CVector<short>& psData);
    virtual void		Close();
protected:
    snd_pcm_t *handle;
    vector<string> names;
    vector<string> devices;
    int dev;
};

class CAlsaSoundOut: public CSoundOutInterface
{
public:
    CAlsaSoundOut();
    virtual 			~CAlsaSoundOut();

    virtual void		Enumerate(vector<string>& choices);
    virtual void		SetDev(int iNewDevice);
    virtual int			GetDev();
    virtual void		Init(int iNewBufferSize, _BOOLEAN bNewBlocking = TRUE);
    virtual _BOOLEAN	Write(CVector<short>& psData);
    virtual void		Close();
protected:
    snd_pcm_t *handle;
    vector<string> names;
    vector<string> devices;
    int dev;
};

#endif
