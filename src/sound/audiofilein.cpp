/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007, 2012, 2013
 *
 * Author(s):
 *	Julian Cable, David Flamand
 *
 * Decription:
 *  Read a file at the correct rate
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

#include "audiofilein.h"
#ifdef _WIN32
# include <windows.h>
#endif
#include <iostream>
#ifdef HAVE_LIBSNDFILE
# include <sndfile.h>
#endif
#include <cstdlib>
#include <cstring>
#include <string.h>


CAudioFileIn::CAudioFileIn(): CSoundInInterface(), eFmt(fmt_other),
    pFileReceiver(NULL), iSampleRate(0), iRequestedSampleRate(0), iBufferSize(0),
    iFileSampleRate(0), iFileChannels(0), pacer(NULL),
    ResampleObjL(NULL), ResampleObjR(NULL), buffer(NULL)
{
}

CAudioFileIn::~CAudioFileIn()
{
    Close();
}

void
CAudioFileIn::SetFileName(const string& strFileName)
{
    strInFileName = strFileName;
    string ext;
    size_t p = strInFileName.rfind('.');
    if (p != string::npos)
        ext = strInFileName.substr(p+1);
	eFmt = fmt_other;
    if (ext == "txt") eFmt = fmt_txt;
    if (ext == "TXT") eFmt = fmt_txt;
    if (ext.substr(0,2) == "iq") eFmt = fmt_raw_stereo;
    if (ext.substr(0,2) == "IQ") eFmt = fmt_raw_stereo;
    if (ext.substr(0,2) == "if") eFmt = fmt_raw_stereo;
    if (ext.substr(0,2) == "IF") eFmt = fmt_raw_stereo;
    if (ext == "pcm") eFmt = fmt_raw_mono;
    if (ext == "PCM") eFmt = fmt_raw_mono;
    switch (eFmt)
    {
    case fmt_raw_stereo:
        iFileChannels = 2;
        if (ext.length() == 4 || ext.length() == 5) /* e.g.: iq48, IF192 */
            iFileSampleRate = 1000 * atoi(ext.substr(2).c_str());
        else
            iFileSampleRate = DEFAULT_SOUNDCRD_SAMPLE_RATE;
        break;
    default:
        iFileChannels = 1;
        if (ext.length() == 5 || ext.length() == 6) /* e.g.: TXT48, pcm192 */
            iFileSampleRate = 1000 * atoi(ext.substr(3).c_str());
        else
            iFileSampleRate = DEFAULT_SOUNDCRD_SAMPLE_RATE;
        break;
    }

#ifdef HAVE_LIBSNDFILE
    SF_INFO sfinfo;
    memset(&sfinfo, 0, sizeof(SF_INFO));
    switch (eFmt)
    {
    case fmt_txt:
        pFileReceiver = fopen(strInFileName.c_str(), "r");
        break;
    case fmt_raw_mono:
    case fmt_raw_stereo:
        sfinfo.samplerate = iFileSampleRate;
        sfinfo.channels = iFileChannels;
        sfinfo.format = SF_FORMAT_RAW|SF_FORMAT_PCM_16|SF_ENDIAN_LITTLE;
        pFileReceiver = (FILE*)sf_open(strInFileName.c_str(), SFM_READ, &sfinfo);
        if (pFileReceiver == NULL)
            throw CGenErr(string("")+sf_strerror(0)+" raised on "+strInFileName);
        break;
    case fmt_other:
        pFileReceiver = (FILE*)sf_open(strInFileName.c_str(), SFM_READ, &sfinfo);
        if (pFileReceiver != NULL)
        {
            iFileChannels = sfinfo.channels;
            iFileSampleRate = sfinfo.samplerate;
		} else {
            string errs = string("")+sf_strerror(0)+" for "+strInFileName;
            throw CGenErr(errs);
		}
        break;
    default:
        pFileReceiver = NULL;
        break;
    }
#else
    if (eFmt == fmt_txt)
        pFileReceiver = fopen(strInFileName.c_str(), "r");
    else
        pFileReceiver = fopen(strInFileName.c_str(), "rb");
#endif

    /* Check for errors */
#ifdef HAVE_LIBSNDFILE
    if (pFileReceiver != NULL)
    {
        sf_count_t count;
        switch (eFmt)
        {
        case fmt_txt:
            // TODO
            break;
        case fmt_raw_mono:
        case fmt_raw_stereo:
        case fmt_other:
            count = sf_seek((SNDFILE*)pFileReceiver, 1, SEEK_SET);
            /* Must contain at least one sample, mono or stereo */
            if (sf_error((SNDFILE*)pFileReceiver) || count != 1 || iFileChannels < 1 || iFileChannels > 2)
            {
                sf_close((SNDFILE*)pFileReceiver);
                pFileReceiver = NULL;
            }
            else
            {
                /* Reset position */
                sf_seek((SNDFILE*)pFileReceiver, 0, SEEK_SET);
            }
            break;
        }
    }
#else
    // TODO
#endif

// The error is reported when reading (red light in system eval on interface IO led)
//    if (pFileReceiver == NULL)
//        throw CGenErr("The file " + strInFileName + " could not be openned");

    iRequestedSampleRate = iFileSampleRate;
    if      (iRequestedSampleRate <= 24000) iRequestedSampleRate = 24000;
    else if (iRequestedSampleRate <= 48000) iRequestedSampleRate = 48000;
    else if (iRequestedSampleRate <= 96000) iRequestedSampleRate = 96000;
    else                                    iRequestedSampleRate = 192000;
}

_BOOLEAN
CAudioFileIn::Init(int iNewSampleRate, int iNewBufferSize, _BOOLEAN bNewBlocking)
{
	//qDebug("CAudioFileIn::Init() iNewSampleRate=%i iNewBufferSize=%i bNewBlocking=%i", iNewSampleRate, iNewBufferSize, bNewBlocking);

    if (pacer)
    {
        delete pacer;
        pacer = NULL;
    }
    if (bNewBlocking)
    {
        double interval = double(iNewBufferSize/2) / double(iNewSampleRate);
        pacer = new CPacer(uint64_t(1e9*interval));
    }

    if (pFileReceiver == NULL)
        return TRUE;

    _BOOLEAN bChanged = FALSE;

	if (iSampleRate != iNewSampleRate)
    {
        iSampleRate = iNewSampleRate;
        bChanged = TRUE;
    }

    if (iBufferSize != iNewBufferSize || bChanged)
    {
        iBufferSize = iNewBufferSize;
        if (buffer)
            delete[] buffer;
        /* Create a resampler object if the file's sample rate isn't supported */
        if (iNewSampleRate != iFileSampleRate)
        {
            iOutBlockSize = iNewBufferSize / 2; /* Mono */
            if (ResampleObjL == NULL)
                ResampleObjL = new CAudioResample();
            ResampleObjL->Init(iOutBlockSize, iFileSampleRate, iNewSampleRate);
            if (iFileChannels == 2)
            {
                if (ResampleObjR == NULL)
                    ResampleObjR = new CAudioResample();
                ResampleObjR->Init(iOutBlockSize, iFileSampleRate, iNewSampleRate);
            }
            const int iMaxInputSize = ResampleObjL->GetMaxInputSize();
            vecTempResBufIn.Init(iMaxInputSize, (_REAL) 0.0);
            vecTempResBufOut.Init(iOutBlockSize, (_REAL) 0.0);
            buffer = new short[iMaxInputSize * 2];
            if (bChanged)
            {
                if (ResampleObjL != NULL)
                    ResampleObjL->Reset();
                if (ResampleObjR != NULL)
                    ResampleObjR->Reset();
            }
        }
        else
        {
            buffer = new short[iNewBufferSize * 2];
        }
    }

    return bChanged;
}

_BOOLEAN
CAudioFileIn::Read(CVector<short>& psData)
{
    if (pacer)
        pacer->wait();

    if (pFileReceiver == NULL || psData.Size() < iBufferSize)
        return TRUE;

    const int iFrames = ResampleObjL ? ResampleObjL->GetFreeInputSize() : iBufferSize/2;
    int i;

    if (eFmt == fmt_txt)
    {
        for (i = 0; i < iFrames; i++)
        {
            float tIn;
            if (fscanf(pFileReceiver, "%e\n", &tIn) == EOF)
            {
                /* If end-of-file is reached, stop simulation */
                return FALSE;
            }
            psData[2*i] = (short)tIn;
            psData[2*i+1] = (short)tIn;
        }
        return FALSE;
    }

    _BOOLEAN bError = FALSE;
    int iRemainingFrame = iFrames;
    int iReadFrame = 0;
#ifdef HAVE_LIBSNDFILE
    while (iRemainingFrame > 0)
    {
	    sf_count_t c = sf_readf_short((SNDFILE*)pFileReceiver, &buffer[iReadFrame * iFileChannels], iRemainingFrame);
	    if (c != sf_count_t(iRemainingFrame))
	    {
            /* rewind */
            if (sf_error((SNDFILE*)pFileReceiver) || sf_seek((SNDFILE*)pFileReceiver, 0, SEEK_SET) == -1)
            {
                memset(&buffer[iReadFrame * iFileChannels], 0, iRemainingFrame * iFileChannels);
                bError = TRUE;
                break;
            }
	    }
        iRemainingFrame -= c;
        iReadFrame += c;
    }
#else
    while (iRemainingFrame > 0)
    {
        size_t c = fread(&buffer[iReadFrame * iFileChannels], sizeof(short), size_t(iRemainingFrame), pFileReceiver);
        if (c != size_t(iRemainingFrame))
        {
            /* rewind */
            if (ferror(pFileReceiver) || fseek(pFileReceiver, 0, SEEK_SET) == -1)
            {
                memset(&buffer[iReadFrame * iFileChannels], 0, iRemainingFrame * iFileChannels);
                bError = TRUE;
                break;
            }
        }
        iRemainingFrame -= c;
        iReadFrame += c;
    }
#endif

    if (ResampleObjL)
    {   /* Resampling is needed */
        if (iFileChannels == 2)
        {   /* Stereo */
            /* Left channel*/
            for (i = 0; i < iFrames; i++)
                vecTempResBufIn[i] = buffer[2*i];
            ResampleObjL->Resample(vecTempResBufIn, vecTempResBufOut);
            for (i = 0; i < iOutBlockSize; i++)
                psData[i*2] = Real2Sample(vecTempResBufOut[i]);
            /* Right channel*/
            for (i = 0; i < iFrames; i++)
                vecTempResBufIn[i] = buffer[2*i+1];
            ResampleObjR->Resample(vecTempResBufIn, vecTempResBufOut);
            for (i = 0; i < iOutBlockSize; i++)
                psData[i*2+1] = Real2Sample(vecTempResBufOut[i]);
        }
        else
        {   /* Mono */
            for (i = 0; i < iFrames; i++)
                vecTempResBufIn[i] = buffer[i];
            ResampleObjL->Resample(vecTempResBufIn, vecTempResBufOut);
            for (i = 0; i < iOutBlockSize; i++)
                psData[i*2] = psData[i*2+1] = Real2Sample(vecTempResBufOut[i]);
        }
    }
    else
    {   /* Resampling is not needed, only copy data */
        if (iFileChannels == 2)
        {   /* Stereo */
            for (i = 0; i < iFrames; i++)
            {
                psData[2*i] = buffer[2*i];
                psData[2*i+1] = buffer[2*i+1];
            }
        }
        else
        {   /* Mono */
            for (i = 0; i < iFrames; i++)
                psData[2*i] = psData[2*i+1] = buffer[i];
        }
    }

    return bError;
}

void
CAudioFileIn::Close()
{
    /* Close file (if opened) */
    if (pFileReceiver != NULL)
    {
#ifdef HAVE_LIBSNDFILE
        if (eFmt == fmt_txt)
            fclose(pFileReceiver);
        else
            sf_close((SNDFILE*)pFileReceiver);
#else
        fclose(pFileReceiver);
#endif
        pFileReceiver = NULL;
    }

	if (buffer != NULL)
		delete[] buffer;
    buffer = NULL;

    if (pacer != NULL)
        delete pacer;
    pacer = NULL;

    if (ResampleObjL != NULL)
        delete ResampleObjL;
    ResampleObjL = NULL;

    if (ResampleObjR != NULL)
        delete ResampleObjR;
    ResampleObjR = NULL;

    vecTempResBufIn.Init(0, (_REAL) 0.0);
    vecTempResBufOut.Init(0, (_REAL) 0.0);

    iSampleRate = 0;
    iBufferSize = 0;
}
