/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Julian Cable
 *
 * Decription:
 * sound interfaces
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

#ifndef _SELECTIONINTERFACE_H
#define _SELECTIONINTERFACE_H

#include "../GlobalDefinitions.h"
#include <vector>
#include <string>

class CSelectionInterface
{
public:
    virtual 			~CSelectionInterface() {}
    virtual void		Enumerate(vector<string>& names, vector<string>& descriptions)=0;
    virtual string		GetDev()=0;
    virtual void		SetDev(string sNewDev)=0;
};

#endif
