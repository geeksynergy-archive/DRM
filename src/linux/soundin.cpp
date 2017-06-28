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

#ifdef WITH_SOUND
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <iostream>
#include <fstream>
#include <sstream>

/*****************************************************************************/

#ifdef USE_OSS

#include <sys/soundcard.h>
#include <errno.h>

CSoundIn::CSoundIn():devices(),dev(),names(),iCurrentDevice(-1)
{
    RecThread.pSoundIn = this;
    getdevices(names, devices, false);
    /* Set flag to open devices */
    bChangDev = TRUE;
}

void CSoundIn::Init_HW()
{
    /* Open sound device (Use O_RDWR only when writing a program which is
       going to both record and play back digital audio) */
    if (devices.size()==0)
        throw CGenErr("no capture devices available");

    /* Default ? */
    if (iCurrentDevice < 0)
        iCurrentDevice = devices.size()-1;

    /* out of range ? (could happen from command line parameter or USB device unplugged */
    if (iCurrentDevice >= int(devices.size()))
        iCurrentDevice = devices.size()-1;

    string devname = devices[iCurrentDevice];
    dev.open(devname, O_RDONLY );
    if (dev.fildes() < 0)
        throw CGenErr("open of "+devname+" failed");

#if 0
    /* Get ready for us.
       ioctl(audio_fd, SNDCTL_DSP_SYNC, 0) can be used when application wants
       to wait until last byte written to the device has been played (it doesn't
       wait in recording mode). After that the call resets (stops) the device
       and returns back to the calling program. Note that this call may take
       several seconds to execute depending on the amount of data in the
       buffers. close() calls SNDCTL_DSP_SYNC automaticly */
    ioctl(dev.fildes(), SNDCTL_DSP_SYNC, 0);

    /* Set sampling parameters always so that number of channels (mono/stereo)
       is set before selecting sampling rate! */
    /* Set number of channels (0=mono, 1=stereo) */
    arg = NUM_IN_CHANNELS - 1;
    status = ioctl(dev.fildes(), SNDCTL_DSP_STEREO, &arg);
    if (status == -1)
        throw CGenErr("SNDCTL_DSP_CHANNELS ioctl failed");

    if (arg != (NUM_IN_CHANNELS - 1))
        throw CGenErr("unable to set number of channels");


    /* Sampling rate */
    arg = Parameters.GetSampleRate();
    status = ioctl(dev.fildes(), SNDCTL_DSP_SPEED, &arg);
    if (status == -1)
        throw CGenErr("SNDCTL_DSP_SPEED ioctl failed");
    if (arg != Parameters.GetSampleRate())
        throw CGenErr("unable to set sample rate");


    /* Sample size */
    arg = (BITS_PER_SAMPLE == 16) ? AFMT_S16_LE : AFMT_U8;
    status = ioctl(dev.fildes(), SNDCTL_DSP_SAMPLESIZE, &arg);
    if (status == -1)
        throw CGenErr("SNDCTL_DSP_SAMPLESIZE ioctl failed");
    if (arg != ((BITS_PER_SAMPLE == 16) ? AFMT_S16_LE : AFMT_U8))
        throw CGenErr("unable to set sample size");
#endif
}


int CSoundIn::read_HW( void * recbuf, int size) {

    int ret = read(dev.fildes(), recbuf, size * NUM_IN_CHANNELS * BYTES_PER_SAMPLE );
    if (ret < 0) {
        switch (errno)
        {
        case 0:
            return 0;
            break;
        case EINTR:
            return 0;
            break;
        case EAGAIN:
            return 0;
            break;
        default:
            qDebug("read error: %s", strerror(errno));
            throw CGenErr("CSound:Read");
        }
    } else
        return ret / (NUM_IN_CHANNELS * BYTES_PER_SAMPLE);
}

void CSoundIn::close_HW( void ) {
    dev.close();
}
#endif

/*****************************************************************************/

#ifdef USE_ALSA

#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API

CSoundIn::CSoundIn(): devices(), handle(NULL), names(),bChangDev(TRUE), iCurrentDevice(-1)
{
    RecThread.pSoundIn = this;
    getdevices(names, devices, false);
}

void CSoundIn::Init_HW() {

    int err, dir=0;
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_sw_params_t *swparams;
    snd_pcm_uframes_t period_size = FRAGSIZE * NUM_IN_CHANNELS/2;
    snd_pcm_uframes_t buffer_size;

    /* Default ? */
    if (iCurrentDevice < 0)
        iCurrentDevice = int(devices.size())-1;

    /* out of range ? (could happen from command line parameter or USB device unplugged */
    if (iCurrentDevice >= int(devices.size()))
        iCurrentDevice = int(devices.size())-1;

    /* record device */
    string recdevice = devices[iCurrentDevice];

    if (handle != NULL)
        return;

    err = snd_pcm_open( &handle, recdevice.c_str(), SND_PCM_STREAM_CAPTURE, 0 );
    if ( err != 0)
    {
        qDebug("open error: %s", snd_strerror(err));
        throw CGenErr("alsa CSoundIn::Init_HW record, can't open "+recdevice+" ("+names[iCurrentDevice]+")");
    }

    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_sw_params_alloca(&swparams);

    /* Choose all parameters */
    err = snd_pcm_hw_params_any(handle, hwparams);
    if (err < 0) {
        qDebug("Broken configuration : no configurations available: %s", snd_strerror(err));
        throw CGenErr("alsa CSoundIn::Init_HW ");
    }
    /* Set the interleaved read/write format */
    err = snd_pcm_hw_params_set_access(handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);

    if (err < 0) {
        qDebug("Access type not available : %s", snd_strerror(err));
        throw CGenErr("alsa CSoundIn::Init_HW ");

    }
    /* Set the sample format */
    err = snd_pcm_hw_params_set_format(handle, hwparams, SND_PCM_FORMAT_S16);
    if (err < 0) {
        qDebug("Sample format not available : %s", snd_strerror(err));
        throw CGenErr("alsa CSoundIn::Init_HW ");
    }
    /* Set the count of channels */
    err = snd_pcm_hw_params_set_channels(handle, hwparams, NUM_IN_CHANNELS);
    if (err < 0) {
        qDebug("Channels count (%i) not available s: %s", NUM_IN_CHANNELS, snd_strerror(err));
        throw CGenErr("alsa CSoundIn::Init_HW ");
    }
    /* Set the stream rate */
    dir=0;
    err = snd_pcm_hw_params_set_rate(handle, hwparams, Parameters.GetSampleRate(), dir);
    if (err < 0) {
        qDebug("Rate %iHz not available : %s", Parameters.GetSampleRate(), snd_strerror(err));
        throw CGenErr("alsa CSoundIn::Init_HW ");

    }
    dir=0;
    unsigned int buffer_time = 500000;              /* ring buffer length in us */
    /* set the buffer time */
    err = snd_pcm_hw_params_set_buffer_time_near(handle, hwparams, &buffer_time, &dir);
    if (err < 0) {
        qDebug("Unable to set buffer time %i for playback: %s\n", buffer_time, snd_strerror(err));
        throw CGenErr("alsa CSoundIn::Init_HW ");
    }
    err = snd_pcm_hw_params_get_buffer_size(hwparams, &buffer_size);
    if (err < 0) {
        qDebug("Unable to get buffer size for playback: %s\n", snd_strerror(err));
        throw CGenErr("alsa CSoundIn::Init_HW ");
    }
    // qDebug("buffer size %d", buffer_size);
    /* set the period time */
    unsigned int period_time = 100000;              /* period time in us */
    err = snd_pcm_hw_params_set_period_time_near(handle, hwparams, &period_time, &dir);
    if (err < 0) {
        qDebug("Unable to set period time %i for playback: %s\n", period_time, snd_strerror(err));
        throw CGenErr("alsa CSoundIn::Init_HW ");
    }
    err = snd_pcm_hw_params_get_period_size_min(hwparams, &period_size, &dir);
    if (err < 0) {
        qDebug("Unable to get period size for playback: %s\n", snd_strerror(err));
        throw CGenErr("alsa CSoundIn::Init_HW ");
    }
    // qDebug("period size %d", period_size);

    /* Write the parameters to device */
    err = snd_pcm_hw_params(handle, hwparams);
    if (err < 0) {
        qDebug("Unable to set hw params : %s", snd_strerror(err));
        throw CGenErr("alsa CSoundIn::Init_HW ");
    }
    /* Get the current swparams */
    err = snd_pcm_sw_params_current(handle, swparams);
    if (err < 0) {
        qDebug("Unable to determine current swparams : %s", snd_strerror(err));
        throw CGenErr("alsa CSoundIn::Init_HW ");
    }
    /* Start the transfer when the buffer immediately */
    err = snd_pcm_sw_params_set_start_threshold(handle, swparams, 0);
    if (err < 0) {
        qDebug("Unable to set start threshold mode : %s", snd_strerror(err));
        throw CGenErr("alsa CSoundIn::Init_HW ");
    }
    /* Allow the transfer when at least period_size samples can be processed */
    err = snd_pcm_sw_params_set_avail_min(handle, swparams, period_size);
    if (err < 0) {
        qDebug("Unable to set avail min : %s", snd_strerror(err));
        throw CGenErr("alsa CSoundIn::Init_HW ");
    }
    /* Align all transfers to 1 sample */
    err = snd_pcm_sw_params_set_xfer_align(handle, swparams, 1);
    if (err < 0) {
        qDebug("Unable to set transfer align : %s", snd_strerror(err));
        throw CGenErr("alsa CSoundIn::Init_HW ");
    }
    /* Write the parameters to the record/playback device */
    err = snd_pcm_sw_params(handle, swparams);
    if (err < 0) {
        qDebug("Unable to set sw params : %s", snd_strerror(err));
        throw CGenErr("alsa CSoundIn::Init_HW ");
    }
    snd_pcm_start(handle);
    qDebug("alsa init done");

}

int CSoundIn::read_HW( void * recbuf, int size) {

    int ret = snd_pcm_readi(handle, recbuf, size);


    if (ret < 0)
    {
        if (ret == -EPIPE)
        {
            qDebug("rpipe");
            /* Under-run */
            qDebug("rprepare");
            ret = snd_pcm_prepare(handle);

            if (ret < 0)
                qDebug("Can't recover from underrun, prepare failed: %s", snd_strerror(ret));

            ret = snd_pcm_start(handle);

            if (ret < 0)
                qDebug("Can't recover from underrun, start failed: %s", snd_strerror(ret));
            return 0;

        }
        else if (ret == -ESTRPIPE)
        {
            qDebug("strpipe");

            /* Wait until the suspend flag is released */
            while ((ret = snd_pcm_resume(handle)) == -EAGAIN)
                sleep(1);

            if (ret < 0)
            {
                ret = snd_pcm_prepare(handle);

                if (ret < 0)
                    qDebug("Can't recover from suspend, prepare failed: %s", snd_strerror(ret));
                throw CGenErr("CSound:Read");
            }
            return 0;
        }
        else
        {
            qDebug("CSoundIn::Read: %s", snd_strerror(ret));
            throw CGenErr("CSound:Read");
        }
    } else
        return ret;

}

void CSoundIn::close_HW( void ) {

    if (handle != NULL)
        snd_pcm_close( handle );

    handle = NULL;
}

#endif


/* ************************************************************************* */


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

#endif

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

