/******************************************************************************\
* Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
* Copyright (c) 2001
*
* Author(s):
*	Alexander Kurpiers
*
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


#ifndef _SOUND_COMMON_H
#define _SOUND_COMMON_H

#ifdef QT_CORE_LIB
# include <qmutex.h>
# include <qthread.h>
#endif
#include "../util/Buffer.h"
#ifdef USE_ALSA
#include <alsa/asoundlib.h>
#endif

/* Definitions ****************************************************************/
#define	NUM_IN_CHANNELS			2		/* Stereo recording (but we only
use one channel for recording) */
#define	NUM_OUT_CHANNELS		2		/* Stereo Playback */
#define	BITS_PER_SAMPLE			16		/* Use all bits of the D/A-converter */
#define BYTES_PER_SAMPLE		2		/* Number of bytes per sample */

#ifdef USE_OSS
#include <map>

class COSSDev
{
    public:
COSSDev():name() {}
void open(const string& devname, int mode);
int fildes() {
    return dev[name].fildes();
}
void close();
protected:
class devdata
{
public:
    devdata():count(0),fd(0) {}
    void open(const string&, int);
    void close();
    int fildes();
protected:
    int count;
    int fd;
};
static map<string,devdata> dev;
string name;
};
#endif

class CSoundBuf : public CCyclicBuffer<_SAMPLE> {

    public:
    CSoundBuf() : keep_running(TRUE)
#ifdef QT_CORE_LIB
            , data_accessed()
#endif
{}
bool keep_running;
#ifdef QT_CORE_LIB
void lock () {
    data_accessed.lock();
}
void unlock () {
    data_accessed.unlock();
}

protected:
QMutex	data_accessed;
#else
void lock () { }
void unlock () { }
#endif
};

#ifdef QT_CORE_LIB
typedef QThread CThread;
#else
class CThread {
    public:
void run() {}
void start() {}
void wait(int) {}
void msleep(int) {}
bool running() {
    return true;
}
};
#endif

void getdevices(vector<string>& names, vector<string>& devices, bool playback);

#endif
