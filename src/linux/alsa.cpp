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
#include <map>
#include <algorithm>

#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API

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

void
getdevices(vector < string > &names, vector < string > &devices,
           bool playback)
{
    vector < string > tmp;
    names.clear();
    devices.clear();
    ifstream sndstat("/proc/asound/pcm");
    if (sndstat.is_open())
    {
        while (!(sndstat.eof() || sndstat.fail()))
        {
            char s[200];
            sndstat.getline(s, sizeof(s));
            if (strlen(s) == 0)
                break;
            if (strstr(s, playback ? "playback" : "capture") != NULL)
                tmp.push_back(s);
        }
        sndstat.close();
    }
    if (tmp.size() > 0)
    {
        sort(tmp.begin(), tmp.end());
        for (size_t i = 0; i < tmp.size(); i++)
        {
            stringstream o(tmp[i]);
            char p, n[200], d[200], cap[80];
            int maj, min;
            o >> maj >> p >> min;
            o >> p;
            o.getline(n, sizeof(n), ':');
            o.getline(d, sizeof(d), ':');
            o.getline(cap, sizeof(cap));
            stringstream dev;
            dev << "plughw:" << maj << "," << min;
            devices.push_back(dev.str());
            names.push_back(n);
        }
    }
    if (playback)
    {
        names.push_back("Default Playback Device");
        devices.push_back("dmix");
    }
    else
    {
        names.push_back("Default Capture Device");
        devices.push_back("dsnoop");
    }
}
