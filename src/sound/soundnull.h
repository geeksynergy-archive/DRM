/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Julian Cable
 *
 * Description:
 *	dummy sound classes
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

#ifndef _SOUNDNULL_H
#define _SOUNDNULL_H

#include "soundinterface.h"

/* Classes ********************************************************************/
class CSoundInNull : public CSoundInInterface
{
public:
    CSoundInNull() {}
    virtual ~CSoundInNull() {}
    virtual _BOOLEAN	Init(int, int, _BOOLEAN) {
        return TRUE;
    }
    virtual _BOOLEAN	Read(CVector<short>&) {
        return FALSE;
    }
    virtual void		Enumerate(vector<string>&choices, vector<string>&) {
        choices.push_back("(File or Network)");
    }
    virtual string		GetDev() {
        return sDev;
    }
    virtual void		SetDev(string sNewDev) {
        sDev = sNewDev;
    }
    virtual void		Close() {}
private:
    string sDev;
};

class CSoundOutNull : public CSoundOutInterface
{
public:
    CSoundOutNull() {}
    virtual ~CSoundOutNull() {}
    virtual _BOOLEAN	Init(int, int, _BOOLEAN) {
        return TRUE;
    }
    virtual _BOOLEAN	Write(CVector<short>&) {
        return FALSE;
    }
    virtual void		Enumerate(vector<string>& choices, vector<string>&) {
        choices.push_back("(None)");
    }
    virtual string		GetDev() {
        return sDev;
    }
    virtual void		SetDev(string sNewDev) {
        sDev = sNewDev;
    }
    virtual void		Close() {}
private:
    string sDev;
};

#endif
