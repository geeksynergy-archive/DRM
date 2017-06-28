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

#include "soundin.h"
#include "soundout.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <iostream>
#include <fstream>
#include <sstream>

/* ************************************************************************* */

CSoundIn::CSoundIn():devices(),names(),iCurrentDevice(-1)
#ifdef USE_OSS
        ,dev()
#endif
#ifdef USE_ALSA
        ,handle(NULL)
#endif
{
    RecThread.pSoundIn = this;
    getdevices(names, devices, false);
    /* Set flag to open devices */
    bChangDev = TRUE;
}

void
CSoundIn::CRecThread::run()
{
    while (SoundBuf.keep_running) {

        int fill;

        SoundBuf.lock();
        fill = SoundBuf.GetFillLevel();
        SoundBuf.unlock();

        if (  (SOUNDBUFLEN - fill) > (FRAGSIZE * NUM_IN_CHANNELS) ) {
            // enough space in the buffer

            int size = pSoundIn->read_HW( tmprecbuf, FRAGSIZE);

            // common code
            if (size > 0) {
                CVectorEx<_SAMPLE>*	ptarget;

                /* Copy data from temporary buffer in output buffer */
                SoundBuf.lock();

                ptarget = SoundBuf.QueryWriteBuffer();

                for (int i = 0; i < size * NUM_IN_CHANNELS; i++)
                    (*ptarget)[i] = tmprecbuf[i];

                SoundBuf.Put( size * NUM_IN_CHANNELS );
                SoundBuf.unlock();
            }
        } else {
            msleep( 1 );
        }
    }
    qDebug("Rec Thread stopped");
}


/* Wave in ********************************************************************/

void CSoundIn::Init(int iNewBufferSize, _BOOLEAN bNewBlocking)
{
    qDebug("initrec %d", iNewBufferSize);

    /* Save < */
    RecThread.SoundBuf.lock();
    iInBufferSize = iNewBufferSize;
    bBlockingRec = bNewBlocking;
    RecThread.SoundBuf.unlock();

    /* Check if device must be opened or reinitialized */
    if (bChangDev == TRUE)
    {

        Init_HW( );

        /* Reset flag */
        bChangDev = FALSE;
    }

    if ( RecThread.running() == FALSE ) {
        RecThread.SoundBuf.lock();
        RecThread.SoundBuf.Init( SOUNDBUFLEN );
        RecThread.SoundBuf.unlock();
        RecThread.start();
    }

}


_BOOLEAN CSoundIn::Read(CVector< _SAMPLE >& psData)
{
    CVectorEx<_SAMPLE>*	p;

    /* Check if device must be opened or reinitialized */
    if (bChangDev == TRUE)
    {
        /* Reinit sound interface */
        Init(iBufferSize, bBlockingRec);

        /* Reset flag */
        bChangDev = FALSE;
    }

    RecThread.SoundBuf.lock();	// we need exclusive access

    if (iCurrentDevice == -1)
        iCurrentDevice = names.size()-1;

    while ( RecThread.SoundBuf.GetFillLevel() < iInBufferSize ) {


        // not enough data, sleep a little
        RecThread.SoundBuf.unlock();
        usleep(1000); //1ms
        RecThread.SoundBuf.lock();
    }

    // copy data

    p = RecThread.SoundBuf.Get( iInBufferSize );
    for (int i=0; i<iInBufferSize; i++)
        psData[i] = (*p)[i];

    RecThread.SoundBuf.unlock();

    return FALSE;
}

void CSoundIn::Close()
{
    qDebug("stoprec");

    // stop the recording threads

    if (RecThread.running() ) {
        RecThread.SoundBuf.keep_running = FALSE;
        // wait 1sec max. for the threads to terminate
        RecThread.wait(1000);
    }

    close_HW();

    /* Set flag to open devices the next time it is initialized */
    bChangDev = TRUE;
}

void CSoundIn::SetDev(int iNewDevice)
{
    /* Change only in case new device id is not already active */
    if (iNewDevice != iCurrentDevice)
    {
        iCurrentDevice = iNewDevice;
        bChangDev = TRUE;
    }
}

int CSoundIn::GetDev()
{
    return iCurrentDevice;
}

CSoundOut::CSoundOut():devices(),names(),iCurrentDevice(-1)
#ifdef USE_OSS
        ,dev()
#endif
#ifdef USE_ALSA
        ,handle(NULL)
#endif
{
    PlayThread.pSoundOut = this;
    getdevices(names, devices, true);
    /* Set flag to open devices */
    bChangDev = TRUE;
}

void CSoundOut::CPlayThread::run()
{
    while ( SoundBuf.keep_running ) {
        int fill;

        SoundBuf.lock();
        fill = SoundBuf.GetFillLevel();
        SoundBuf.unlock();

        if ( fill > (FRAGSIZE * NUM_OUT_CHANNELS) ) {

            // enough data in the buffer

            CVectorEx<_SAMPLE>*	p;

            SoundBuf.lock();
            p = SoundBuf.Get( FRAGSIZE * NUM_OUT_CHANNELS );

            for (int i=0; i < FRAGSIZE * NUM_OUT_CHANNELS; i++)
                tmpplaybuf[i] = (*p)[i];

            SoundBuf.unlock();

            pSoundOut->write_HW( tmpplaybuf, FRAGSIZE );

        } else {

            do {
                msleep( 1 );

                SoundBuf.lock();
                fill = SoundBuf.GetFillLevel();
                SoundBuf.unlock();

            } while ((SoundBuf.keep_running) && ( fill < SOUNDBUFLEN/2 ));	// wait until buffer is at least half full
        }
    }
    qDebug("Play Thread stopped");
}

void CSoundOut::Init(int iNewBufferSize, _BOOLEAN bNewBlocking)
{
    qDebug("initplay %d", iNewBufferSize);

    /* Save buffer size */
    PlayThread.SoundBuf.lock();
    iBufferSize = iNewBufferSize;
    bBlockingPlay = bNewBlocking;
    PlayThread.SoundBuf.unlock();

    /* Check if device must be opened or reinitialized */
    if (bChangDev == TRUE)
    {

        Init_HW( );

        /* Reset flag */
        bChangDev = FALSE;
    }

    if ( PlayThread.running() == FALSE ) {
        PlayThread.SoundBuf.lock();
        PlayThread.SoundBuf.Init( SOUNDBUFLEN );
        PlayThread.SoundBuf.unlock();
        PlayThread.start();
    }
}

_BOOLEAN CSoundOut::Write(CVector< _SAMPLE >& psData)
{
    /* Check if device must be opened or reinitialized */
    if (bChangDev == TRUE)
    {
        /* Reinit sound interface */
        Init(iBufferSize, bBlockingPlay);

        /* Reset flag */
        bChangDev = FALSE;
    }

    if ( bBlockingPlay ) {
        // blocking write
        while ( PlayThread.SoundBuf.keep_running ) {
            PlayThread.SoundBuf.lock();
            int fill = SOUNDBUFLEN - PlayThread.SoundBuf.GetFillLevel();
            PlayThread.SoundBuf.unlock();
            if ( fill > iBufferSize) break;
        }
    }

    PlayThread.SoundBuf.lock();	// we need exclusive access

    if ( ( SOUNDBUFLEN - PlayThread.SoundBuf.GetFillLevel() ) > iBufferSize) {

        CVectorEx<_SAMPLE>*	ptarget;

        // data fits, so copy
        ptarget = PlayThread.SoundBuf.QueryWriteBuffer();
        for (int i=0; i < iBufferSize; i++)
        {
            (*ptarget)[i] = psData[i];
        }

        PlayThread.SoundBuf.Put( iBufferSize );
    }

    PlayThread.SoundBuf.unlock();

    return FALSE;
}

void CSoundOut::Close()
{
    qDebug("stopplay");

    // stop the playback thread
    if (PlayThread.running() ) {
        PlayThread.SoundBuf.keep_running = FALSE;
        PlayThread.wait(1000);
    }

    close_HW();

    /* Set flag to open devices the next time it is initialized */
    bChangDev = TRUE;
}

void CSoundOut::SetDev(int iNewDevice)
{
    /* Change only in case new device id is not already active */
    if (iNewDevice != iCurrentDevice)
    {
        iCurrentDevice = iNewDevice;
        bChangDev = TRUE;
    }
}

int CSoundOut::GetDev()
{
    return iCurrentDevice;
}

