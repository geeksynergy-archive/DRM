/******************************************************************************\
 *
 * Copyright (c) 2013
 *
 * Author(s):
 *  David Flamand
 *
 * Description:
 *  AAC codec class
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

#include "aac_codec.h"
#if !defined(USE_FAAD2_LIBRARY) || !defined(USE_FAAC_LIBRARY)
# include "../util/LibraryLoader.h"
#endif


#ifndef USE_FAAD2_LIBRARY
static void* hFaadLib;
static NeAACDecOpen_t *NeAACDecOpen;
static NeAACDecInitDRM_t *NeAACDecInitDRM;
static NeAACDecClose_t *NeAACDecClose;
static NeAACDecDecode_t *NeAACDecDecode;
static const LIBFUNC FaadLibFuncs[] = {
	{ "NeAACDecOpen",    (void**)&NeAACDecOpen,    (void*)NULL },
	{ "NeAACDecInitDRM", (void**)&NeAACDecInitDRM, (void*)NULL },
	{ "NeAACDecClose",   (void**)&NeAACDecClose,   (void*)NULL },
	{ "NeAACDecDecode",  (void**)&NeAACDecDecode,  (void*)NULL },
	{ NULL, NULL, NULL }
};
# if defined(_WIN32)
static const char* FaadLibNames[] = { "faad2_drm.dll", "libfaad2_drm.dll", "faad_drm.dll", "libfaad2.dll", NULL };
# elif defined(__APPLE__)
static const char* FaadLibNames[] = { "libfaad2_drm.dylib", NULL };
# else
static const char* FaadLibNames[] = { "libfaad2_drm.so", "libfaad_drm.so", "libfaad.so.2", NULL };
# endif
#endif


#ifndef USE_FAAC_LIBRARY
static void* hFaacLib;
static faacEncGetVersion_t* faacEncGetVersion;
static faacEncGetCurrentConfiguration_t* faacEncGetCurrentConfiguration;
static faacEncSetConfiguration_t* faacEncSetConfiguration;
static faacEncOpen_t* faacEncOpen;
static faacEncEncode_t* faacEncEncode;
static faacEncClose_t* faacEncClose;
static const LIBFUNC FaacLibFuncs[] = {
	{ "faacEncGetVersion",              (void**)&faacEncGetVersion,              (void*)NULL },
	{ "faacEncGetCurrentConfiguration", (void**)&faacEncGetCurrentConfiguration, (void*)NULL },
	{ "faacEncSetConfiguration",        (void**)&faacEncSetConfiguration,        (void*)NULL },
	{ "faacEncOpen",                    (void**)&faacEncOpen,                    (void*)NULL },
	{ "faacEncEncode",                  (void**)&faacEncEncode,                  (void*)NULL },
	{ "faacEncClose",                   (void**)&faacEncClose,                   (void*)NULL },
	{ NULL, NULL, NULL }
};
# if defined(_WIN32)
static const char* FaacLibNames[] = { "faac_drm.dll", "libfaac_drm.dll", "libfaac.dll", "faac.dll", NULL };
# elif defined(__APPLE__)
static const char* FaacLibNames[] = { "libfaac_drm.dylib", NULL };
# else
static const char* FaacLibNames[] = { "libfaac_drm.so", "libfaac.so.0", NULL };
# endif
static bool FaacCheckCallback()
{
    bool bLibOk = false;
    unsigned long lNumSampEncIn = 0;
    unsigned long lMaxBytesEncOut = 0;
    faacEncHandle hEncoder = faacEncOpen(24000, 1, &lNumSampEncIn, &lMaxBytesEncOut);
    if (hEncoder != NULL)
    {
        /* lMaxBytesEncOut is odd when DRM is supported */
        bLibOk = lMaxBytesEncOut & 1;
        faacEncClose(hEncoder);
    }
    return bLibOk;
}
#endif


/******************************************************************************/
/* Implementation *************************************************************/

AacCodec::AacCodec() :
	hFaadDecoder(NULL), hFaacEncoder(NULL)
{
#ifndef USE_FAAD2_LIBRARY
	if (hFaadLib == NULL)
	{
		hFaadLib = CLibraryLoader::Load(FaadLibNames, FaadLibFuncs);
		if (!hFaadLib)
		    cerr << "No usable FAAD2 aac decoder library found" << endl;
		else
		    cerr << "Got FAAD2 library" << endl;
	}
#endif
#ifndef USE_FAAC_LIBRARY
    if (hFaacLib == NULL)
    {
        hFaacLib = CLibraryLoader::Load(FaacLibNames, FaacLibFuncs, FaacCheckCallback);
        if (!hFaacLib)
            cerr << "No usable FAAC aac encoder library found" << endl;
        else
            cerr << "Got FAAC library" << endl;
    }
#endif
}
AacCodec::~AacCodec()
{
	DecClose();
	EncClose();
}

/******************************************************************************/
/* Decoder Implementation *****************************************************/

string
AacCodec::DecGetVersion()
{
	return string("Nero AAC version: ") + FAAD2_VERSION;
}

bool
AacCodec::CanDecode(CAudioParam::EAudCod eAudioCoding)
{
#ifndef USE_FAAD2_LIBRARY
	return hFaadLib && eAudioCoding == CAudioParam::AC_AAC;
#else
	return eAudioCoding == CAudioParam::AC_AAC;
#endif
}

bool
AacCodec::DecOpen(CAudioParam& AudioParam, int *iAudioSampleRate, int *iLenDecOutPerChan)
{
	int iAACSampleRate = 12000;
	if (hFaadDecoder == NULL)
		hFaadDecoder = NeAACDecOpen();
	if (hFaadDecoder != NULL)
	{
		int iDRMchanMode = DRMCH_MONO;
		/* Only 12 kHz and 24 kHz is allowed */
		switch (AudioParam.eAudioSamplRate)
		{
		case CAudioParam::AS_12KHZ:
			iAACSampleRate = 12000;
			break;

		case CAudioParam::AS_24KHZ:
			iAACSampleRate = 24000;
			break;

		default:
			break;
		}
		/* Number of channels for AAC: Mono, PStereo, Stereo */
		switch (AudioParam.eAudioMode)
		{
		case CAudioParam::AM_MONO:
			if (AudioParam.eSBRFlag == CAudioParam::SB_USED)
				iDRMchanMode = DRMCH_SBR_MONO;
			else
				iDRMchanMode = DRMCH_MONO;
			break;

		case CAudioParam::AM_P_STEREO:
			/* Low-complexity only defined in SBR mode */
			iDRMchanMode = DRMCH_SBR_PS_STEREO;
			break;

		case CAudioParam::AM_STEREO:
			if (AudioParam.eSBRFlag == CAudioParam::SB_USED)
				iDRMchanMode = DRMCH_SBR_STEREO;
			else
				iDRMchanMode = DRMCH_STEREO;
			break;
		}
		NeAACDecInitDRM(&hFaadDecoder, iAACSampleRate,
			(unsigned char)iDRMchanMode);
	}
    /* In case of SBR, AAC sample rate is half the total sample rate.
       Length of output is doubled if SBR is used */
    if (AudioParam.eSBRFlag == CAudioParam::SB_USED)
    {
        *iAudioSampleRate = iAACSampleRate * 2;
        *iLenDecOutPerChan = AUD_DEC_TRANSFROM_LENGTH * 2;
    }
    else
    {
        *iAudioSampleRate = iAACSampleRate;
        *iLenDecOutPerChan = AUD_DEC_TRANSFROM_LENGTH;
    }
	return hFaadDecoder != NULL;
}

_SAMPLE*
AacCodec::Decode(CVector<uint8_t>& vecbyPrepAudioFrame, int *iChannels, CAudioCodec::EDecError *eDecError)
{
	_SAMPLE* psDecOutSampleBuf = NULL;
	NeAACDecFrameInfo DecFrameInfo;
	DecFrameInfo.channels = 1;
	DecFrameInfo.error = 1;
	if (hFaadDecoder != NULL)
	{
		psDecOutSampleBuf = (_SAMPLE*) NeAACDecDecode(hFaadDecoder,
			&DecFrameInfo, &vecbyPrepAudioFrame[0], vecbyPrepAudioFrame.size());
	}
	*iChannels = DecFrameInfo.channels;
	*eDecError = DecFrameInfo.error ? CAudioCodec::DECODER_ERROR_UNKNOWN : CAudioCodec::DECODER_ERROR_OK;
	return psDecOutSampleBuf;
}

void
AacCodec::DecClose()
{
	if (hFaadDecoder != NULL)
	{
		NeAACDecClose(hFaadDecoder);
		hFaadDecoder = NULL;
	}
}

void
AacCodec::DecUpdate(CAudioParam&)
{
}

/******************************************************************************/
/* Encoder Implementation *****************************************************/

string
AacCodec::EncGetVersion()
{
	char nul = 0;
	char *faac_id_string = &nul;
	char *faac_copyright_string = &nul;
	faacEncGetVersion(&faac_id_string, &faac_copyright_string);
	return string("FAAC version: ") + faac_id_string;
}

bool
AacCodec::CanEncode(CAudioParam::EAudCod eAudioCoding)
{
#ifndef USE_FAAC_LIBRARY
	return hFaacLib && eAudioCoding == CAudioParam::AC_AAC;
#else
	return eAudioCoding == CAudioParam::AC_AAC;
#endif
}

bool
AacCodec::EncOpen(int iSampleRate, int iChannels, unsigned long *lNumSampEncIn, unsigned long *lMaxBytesEncOut)
{
	hFaacEncoder = faacEncOpen(iSampleRate, iChannels,
		lNumSampEncIn, lMaxBytesEncOut);
	return hFaacEncoder != NULL;
}

int
AacCodec::Encode(CVector<_SAMPLE>& vecsEncInData, unsigned long lNumSampEncIn, CVector<uint8_t>& vecsEncOutData, unsigned long lMaxBytesEncOut)
{
	int bytesEncoded = 0;
	if (hFaacEncoder != NULL)
	{
		bytesEncoded = faacEncEncode(hFaacEncoder,
			(int32_t *) &vecsEncInData[0],
			lNumSampEncIn, &vecsEncOutData[0],
			lMaxBytesEncOut);
	}
	return bytesEncoded;
}

void
AacCodec::EncClose()
{
	if (hFaacEncoder != NULL)
	{
		faacEncClose(hFaacEncoder);
		hFaacEncoder = NULL;
	}
}

void
AacCodec::EncSetBitrate(int iBitRate)
{
	if (hFaacEncoder != NULL)
	{
		/* Set encoder configuration */
		faacEncConfigurationPtr CurEncFormat;
		CurEncFormat = faacEncGetCurrentConfiguration(hFaacEncoder);
		CurEncFormat->inputFormat = FAAC_INPUT_16BIT;
		CurEncFormat->useTns = 1;
		CurEncFormat->aacObjectType = LOW;
		CurEncFormat->mpegVersion = MPEG4;
		CurEncFormat->outputFormat = 0;	/* (0 = Raw; 1 = ADTS -> Raw) */
		CurEncFormat->bitRate = iBitRate;
		CurEncFormat->bandWidth = 0;	/* Let the encoder choose the bandwidth */
		faacEncSetConfiguration(hFaacEncoder, CurEncFormat);
	}
}

void
AacCodec::EncUpdate(CAudioParam&)
{
}

