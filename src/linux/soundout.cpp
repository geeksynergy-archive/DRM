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

#include "soundout.h"

#ifdef WITH_SOUND
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

/*****************************************************************************/

#ifdef USE_OSS

#include <sys/soundcard.h>
#include <errno.h>

CSoundOut::CSoundOut():devices(),dev(),names(),iCurrentDevice(-1)
{
    PlayThread.pSoundOut = this;
    getdevices(names, devices, true);
    /* Set flag to open devices */
    bChangDev = TRUE;
}

void CSoundOut::Init_HW()
{
    /* Open sound device (Use O_RDWR only when writing a program which is
       going to both record and play back digital audio) */
    if (devices.size()==0)
        throw CGenErr("no playback devices available");

    /* Default ? */
    if (iCurrentDevice < 0)
        iCurrentDevice = devices.size()-1;

    /* out of range ? (could happen from command line parameter or USB device unplugged */
    if (iCurrentDevice >= int(devices.size()))
        iCurrentDevice = devices.size()-1;

    string devname = devices[iCurrentDevice];
    dev.open(devname, O_WRONLY );
#if 0
    if (dev.fildes() < 0)
        throw CGenErr("open of "+devname+" failed");
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
    arg = NUM_OUT_CHANNELS - 1;
    status = ioctl(dev.fildes(), SNDCTL_DSP_STEREO, &arg);
    if (status == -1)
        throw CGenErr(string("SNDCTL_DSP_CHANNELS ioctl failed: ")+strerror(errno));

    if (arg != (NUM_OUT_CHANNELS - 1))
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
#if 0
    /* Check capabilities of the sound card */
    status = ioctl(dev.fildes(), SNDCTL_DSP_GETCAPS, &arg);
    if (status ==  -1)
        throw CGenErr("SNDCTL_DSP_GETCAPS ioctl failed");
    if ((arg & DSP_CAP_DUPLEX) == 0)
        throw CGenErr("Soundcard not full duplex capable!");
#endif
#endif
}

int CSoundOut::write_HW( _SAMPLE *playbuf, int size )
{

    int start = 0;
    int ret;

    size *= BYTES_PER_SAMPLE * NUM_OUT_CHANNELS;

    while (size)
    {
        ret = write(dev.fildes(), &playbuf[start], size);
        if (ret < 0) {
            if (errno == EINTR || errno == EAGAIN)
            {
                continue;
            }
            throw CGenErr("CSound:Write");
        }
        size -= ret;
        start += ret / BYTES_PER_SAMPLE;
    }
    return 0;
}

void CSoundOut::close_HW( void )
{
    dev.close();
}
#endif

/*****************************************************************************/

#ifdef USE_ALSA

#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API

#include <alsa/asoundlib.h>

CSoundOut::CSoundOut() : devices(), handle(NULL),names(),bChangDev(TRUE), iCurrentDevice(-1)
{
    PlayThread.pSoundOut = this;
    getdevices(names, devices, true);
}

void CSoundOut::Init_HW()
{

    int err, dir;
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_sw_params_t *swparams;
    snd_pcm_uframes_t period_size = FRAGSIZE * NUM_OUT_CHANNELS/2;
    snd_pcm_uframes_t buffer_size;

    /* playback device */
    if (devices.size()==0)
        throw CGenErr("alsa CSoundOut::Init_HW no playback devices available!");

    /* Default ? */
    if (iCurrentDevice < 0)
        iCurrentDevice = int(devices.size())-1;

    /* out of range ? (could happen from command line parameter or USB device unplugged */
    if (iCurrentDevice >= int(devices.size()))
        iCurrentDevice = int(devices.size())-1;

    string playdevice = devices[iCurrentDevice];

    if (handle != NULL)
        return;

    err = snd_pcm_open( &handle, playdevice.c_str(), SND_PCM_STREAM_PLAYBACK, 0 );
    if ( err != 0)
    {
        qDebug("open error: %s", snd_strerror(err));
        throw CGenErr("alsa CSoundOut::Init_HW playback, can't open "+playdevice+" ("+names[iCurrentDevice]+")");
    }

    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_sw_params_alloca(&swparams);

    /* Choose all parameters */
    err = snd_pcm_hw_params_any(handle, hwparams);
    if (err < 0) {
        qDebug("Broken configuration : no configurations available: %s", snd_strerror(err));
        throw CGenErr("alsa CSoundOut::Init_HW ");
    }
    /* Set the interleaved read/write format */
    err = snd_pcm_hw_params_set_access(handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);

    if (err < 0) {
        qDebug("Access type not available : %s", snd_strerror(err));
        throw CGenErr("alsa CSoundOut::Init_HW ");

    }
    /* Set the sample format */
    err = snd_pcm_hw_params_set_format(handle, hwparams, SND_PCM_FORMAT_S16);
    if (err < 0) {
        qDebug("Sample format not available : %s", snd_strerror(err));
        throw CGenErr("alsa CSoundOut::Init_HW ");
    }
    /* Set the count of channels */
    err = snd_pcm_hw_params_set_channels(handle, hwparams, NUM_OUT_CHANNELS);
    if (err < 0) {
        qDebug("Channels count (%i) not available s: %s", NUM_OUT_CHANNELS, snd_strerror(err));
        throw CGenErr("alsa CSoundOut::Init_HW ");
    }
    /* Set the stream rate */
    dir=0;
    err = snd_pcm_hw_params_set_rate(handle, hwparams, Parameters.GetSampleRate(), dir);
    if (err < 0) {
        qDebug("Rate %iHz not available : %s", Parameters.GetSampleRate(), snd_strerror(err));
        throw CGenErr("alsa CSoundOut::Init_HW ");
    }
    dir=0;
    unsigned int buffer_time = 500000;              /* ring buffer length in us */
    /* set the buffer time */
    err = snd_pcm_hw_params_set_buffer_time_near(handle, hwparams, &buffer_time, &dir);
    if (err < 0) {
        qDebug("Unable to set buffer time %i for playback: %s\n", buffer_time, snd_strerror(err));
        throw CGenErr("alsa CSoundOut::Init_HW ");
    }
    err = snd_pcm_hw_params_get_buffer_size(hwparams, &buffer_size);
    if (err < 0) {
        qDebug("Unable to get buffer size for playback: %s\n", snd_strerror(err));
        throw CGenErr("alsa CSoundOut::Init_HW ");
    }
    // qDebug("buffer size %d", buffer_size);
    /* set the period time */
    unsigned int period_time = 100000;              /* period time in us */
    err = snd_pcm_hw_params_set_period_time_near(handle, hwparams, &period_time, &dir);
    if (err < 0) {
        qDebug("Unable to set period time %i for playback: %s\n", period_time, snd_strerror(err));
        throw CGenErr("alsa CSoundOut::Init_HW ");
    }
    err = snd_pcm_hw_params_get_period_size_min(hwparams, &period_size, &dir);
    if (err < 0) {
        qDebug("Unable to get period size for playback: %s\n", snd_strerror(err));
        throw CGenErr("alsa CSoundOut::Init_HW ");
    }
    // qDebug("period size %d", period_size);

    /* Write the parameters to device */
    err = snd_pcm_hw_params(handle, hwparams);
    if (err < 0) {
        qDebug("Unable to set hw params : %s", snd_strerror(err));
        throw CGenErr("alsa CSoundOut::Init_HW ");
    }
    /* Get the current swparams */
    err = snd_pcm_sw_params_current(handle, swparams);
    if (err < 0) {
        qDebug("Unable to determine current swparams : %s", snd_strerror(err));
        throw CGenErr("alsa CSoundOut::Init_HW ");
    }
    /* Write the parameters to the playback device */
    err = snd_pcm_sw_params(handle, swparams);
    if (err < 0) {
        qDebug("Unable to set sw params : %s", snd_strerror(err));
        throw CGenErr("alsa CSoundOut::Init_HW ");
    }
    snd_pcm_start(handle);
    qDebug("alsa init done");

}

int CSoundOut::write_HW( _SAMPLE *playbuf, int size )
{

    int start = 0;
    int ret;

    while (size) {

        ret = snd_pcm_writei(handle, &playbuf[start], size );
        if (ret < 0) {
            if (ret ==  -EAGAIN) {
                if ((ret = snd_pcm_wait (handle, 100)) < 0) {
                    qDebug ("poll failed (%s)", snd_strerror (ret));
                    break;
                }
                continue;
            } else
                if (ret == -EPIPE) {    /* under-run */
                    qDebug("underrun");
                    ret = snd_pcm_prepare(handle);
                    if (ret < 0)
                        qDebug("Can't recover from underrun, prepare failed: %s", snd_strerror(ret));
                    continue;
                } else if (ret == -ESTRPIPE) {
                    qDebug("strpipe");
                    while ((ret = snd_pcm_resume(handle)) == -EAGAIN)
                        sleep(1);       /* wait until the suspend flag is released */
                    if (ret < 0) {
                        ret = snd_pcm_prepare(handle);
                        if (ret < 0)
                            qDebug("Can't recover from suspend, prepare failed: %s", snd_strerror(ret));
                    }
                    continue;
                } else {
                    qDebug("Write error: %s", snd_strerror(ret));
                    throw CGenErr("Write error");
                }
            break;  /* skip one period */
        }
        size -= ret;
        start += ret;
    }
    return 0;
}

void CSoundOut::close_HW( void )
{

    if (handle != NULL)
        snd_pcm_close( handle );

    handle = NULL;
}

#endif

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

#else
CSoundOut::CSoundOut():names(),iCurrentDevice(-1) {}
#endif

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

