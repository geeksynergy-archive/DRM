/******************************************************************************\
 *
 * Copyright (c) 2012-2013
 *
 * Author(s):
 *  David Flamand
 *
 * Description:
 *  Opus codec class
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

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include "opus_codec.h"
#ifndef USE_OPUS_LIBRARY
# include "../util/LibraryLoader.h"
#endif

#undef EPRINTF
#define EPRINTF(...) do {fprintf(stderr, __VA_ARGS__); fflush(stderr);} while(0)
//#define EPRINTF(...) {}
#undef CRC_BYTES
#define CRC_BYTES 1 // For testing purpose, MUST be set to ONE


#ifndef USE_OPUS_LIBRARY
static void* hOpusLib;
static opus_encode_t *opus_encode;
static opus_encoder_create_t *opus_encoder_create;
static opus_encoder_ctl_t *opus_encoder_ctl;
static opus_encoder_destroy_t *opus_encoder_destroy;
static opus_decode_t *opus_decode;
static opus_decoder_create_t *opus_decoder_create;
static opus_decoder_ctl_t *opus_decoder_ctl;
static opus_decoder_destroy_t *opus_decoder_destroy;
static opus_get_version_string_t *opus_get_version_string;
static opus_strerror_t *opus_strerror;
static const LIBFUNC LibFuncs[] = {
	{ "opus_decode",             (void**)&opus_decode,             (void*)NULL },
	{ "opus_decoder_create",     (void**)&opus_decoder_create,     (void*)NULL },
	{ "opus_decoder_ctl",        (void**)&opus_decoder_ctl,        (void*)NULL },
	{ "opus_decoder_destroy",    (void**)&opus_decoder_destroy,    (void*)NULL },
	{ "opus_encode",             (void**)&opus_encode,             (void*)NULL },
	{ "opus_encoder_create",     (void**)&opus_encoder_create,     (void*)NULL },
	{ "opus_encoder_ctl",        (void**)&opus_encoder_ctl,        (void*)NULL },
	{ "opus_encoder_destroy",    (void**)&opus_encoder_destroy,    (void*)NULL },
	{ "opus_get_version_string", (void**)&opus_get_version_string, (void*)NULL },
	{ "opus_strerror",           (void**)&opus_strerror,           (void*)NULL },
	{ NULL, NULL, NULL }
};
# if defined(_WIN32)
static const char* LibNames[] = { "libopus-0.dll", "libopus.dll", "opus.dll", NULL };
# elif defined(__APPLE__)
static const char* LibNames[] = { "libopus.dylib", NULL };
# else
static const char* LibNames[] = { "libopus.so.0", "libopus.so", NULL };
# endif
#endif


//###################################################################################################
//# Common


const char *opusGetVersion(
	void
	)
{
	return opus_get_version_string();
}

void opusSetupParam(
	CAudioParam &AudioParam,
	int toc
	)
{
	static const CAudioParam::EOPUSBandwidth bandwidth_silk[] =
		{ CAudioParam::OB_NB, CAudioParam::OB_MB, CAudioParam::OB_WB };
	static const CAudioParam::EOPUSBandwidth bandwidth_celt[] =
		{ CAudioParam::OB_NB, CAudioParam::OB_WB, CAudioParam::OB_SWB, CAudioParam::OB_FB };
	int stereo, config, mode;
	stereo = (toc >> 2) & 0x01;
	config = (toc >> 3) & 0x1F;
	mode = (config >> 2) & 0x07;
	if (config < 12)
	{
		AudioParam.eOPUSSubCod = CAudioParam::OS_SILK;
		AudioParam.eOPUSBandwidth = bandwidth_silk[mode];
	}
	else if (config < 16)
	{
		AudioParam.eOPUSSubCod = CAudioParam::OS_HYBRID;
		AudioParam.eOPUSBandwidth = bandwidth_celt[((config >> 1) & 1) + 2];
	}
	else
	{
		AudioParam.eOPUSSubCod = CAudioParam::OS_CELT;
		AudioParam.eOPUSBandwidth = bandwidth_celt[mode-4];
	}
	AudioParam.eOPUSChan = stereo ? CAudioParam::OC_STEREO : CAudioParam::OC_MONO;
}


//###################################################################################################
//# Encoder


opus_encoder *opusEncOpen(
	unsigned long sampleRate,
	unsigned int numChannels,
	unsigned int bytes_per_frame,
	unsigned long *inputSamples,
	unsigned long *maxOutputBytes
	)
{
	opus_encoder *enc;
	OpusEncoder *oe;
	int ret;

	enc = (opus_encoder*)calloc(1, sizeof(opus_encoder));
	if (!enc)
	{
		return NULL;
	}
	enc->CRCObject = new CCRC();
	if (!enc->CRCObject)
	{
		free(enc);
		return NULL;
	}

	enc->samples_per_channel = OPUS_PCM_FRAME_SIZE;
	enc->samplerate = sampleRate;
	enc->channels   = numChannels;
	enc->bytes_per_frame = bytes_per_frame;
	int frequency = (int)((float)enc->samplerate / (float)enc->samples_per_channel);
	int frame_extra_bytes = CRC_BYTES;
	int frames_per_packet = (int)((float)enc->samples_per_channel / (float)enc->samplerate / (1.0f / (float)frequency));
//	int bitrate_by_bytes = enc->samplerate * 8 / enc->samples_per_channel;
//	int bitrate = bitrate_by_bytes * (bytes_per_frame - frame_extra_bytes);
	enc->frames_per_packet = frames_per_packet;

    *inputSamples = enc->samples_per_channel * numChannels;
    *maxOutputBytes = OPUS_MAX_DATA_FRAME;
    *maxOutputBytes += frame_extra_bytes;

//	EPRINTF("opusEncOpen: sampleRate=%i numChannels=%i inputSamples=%i maxOutputBytes=%i bytes_per_frame=%i bitrate=%i\n", (int)sampleRate, (int)numChannels, (int)*inputSamples, (int)*maxOutputBytes, (int)bytes_per_frame, bitrate);

	oe = opus_encoder_create(sampleRate, numChannels, OPUS_APPLICATION_AUDIO, &ret);
	if (!oe)
		EPRINTF("opusEncOpen: opus_encoder_create returned: %s\n", opus_strerror(ret));
/*	ret = opus_encoder_ctl(oe, OPUS_SET_FORCE_CHANNELS(numChannels));
	if (ret != OPUS_OK)
		EPRINTF("opusEncOpen: OPUS_SET_FORCE_CHANNELS returned: %s\n", opus_strerror(ret));
	ret = opus_encoder_ctl(oe, OPUS_SET_BANDWIDTH(bandwidth));
	if (ret != OPUS_OK)
		EPRINTF("opusEncOpen: OPUS_SET_BANDWIDTH returned: %s\n", opus_strerror(ret));
	ret = opus_encoder_ctl(oe, OPUS_SET_MAX_BANDWIDTH(bandwidth));
	if (ret != OPUS_OK)
		EPRINTF("opusEncOpen: OPUS_SET_MAX_BANDWIDTH returned: %s\n", opus_strerror(ret));
	ret = opus_encoder_ctl(oe, OPUS_SET_BITRATE(bitrate));
	if (ret != OPUS_OK)
		EPRINTF("opusEncOpen: OPUS_SET_BITRATE returned: %s\n", opus_strerror(ret));
*/	ret = opus_encoder_ctl(oe, OPUS_SET_VBR(0));
	if (ret != OPUS_OK)
		EPRINTF("opusEncOpen: OPUS_SET_VBR returned: %s\n", opus_strerror(ret));
	ret = opus_encoder_ctl(oe, OPUS_SET_VBR_CONSTRAINT(0));
	if (ret != OPUS_OK)
		EPRINTF("opusEncOpen: OPUS_SET_VBR_CONSTRAINT returned: %s\n", opus_strerror(ret));
	ret = opus_encoder_ctl(oe, OPUS_SET_COMPLEXITY(10));
	if (ret != OPUS_OK)
		EPRINTF("opusEncOpen: OPUS_SET_COMPLEXITY returned: %s\n", opus_strerror(ret));
/*	ret = opus_encoder_ctl(oe, OPUS_SET_PACKET_LOSS_PERC(0));
	if (ret != OPUS_OK)
		EPRINTF("opusEncOpen: OPUS_SET_PACKET_LOSS_PERC returned: %s\n", opus_strerror(ret));
*/	ret = opus_encoder_ctl(oe, OPUS_SET_LSB_DEPTH(16));
	if (ret != OPUS_OK)
		EPRINTF("opusEncOpen: OPUS_SET_LSB_DEPTH returned: %s\n", opus_strerror(ret));
/*	ret = opus_encoder_ctl(oe, OPUS_SET_INBAND_FEC(0));
	if (ret != OPUS_OK)
		EPRINTF("opusEncOpen: OPUS_SET_INBAND_FEC returned: %s\n", opus_strerror(ret));
	ret = opus_encoder_ctl(oe, OPUS_SET_SIGNAL(OPUS_SIGNAL_MUSIC));
	if (ret != OPUS_OK)
		EPRINTF("opusEncOpen: OPUS_SET_SIGNAL returned: %s\n", opus_strerror(ret));
*/	ret = opus_encoder_ctl(oe, OPUS_SET_DTX(0));
	if (ret != OPUS_OK)
		EPRINTF("opusEncOpen: OPUS_SET_DTX_REQUEST returned: %s\n", opus_strerror(ret));

	enc->oe = oe;
	return enc;
}


int opusEncEncode(opus_encoder *enc,
	opus_int16 *inputBuffer,
	unsigned int samplesInput,
	unsigned char *outputBuffer,
	unsigned int bufferSize
	)
{
	if (bufferSize < (OPUS_MAX_DATA_FRAME + CRC_BYTES))
	{
		EPRINTF("opusEncEncode: bufferSize %i < %i\n", bufferSize, OPUS_MAX_DATA_FRAME + CRC_BYTES);
		return 0;
	}
	OpusEncoder *oe = enc->oe;

	int frame_extra_bytes = CRC_BYTES;
	int frame_bytes, i, pcm_pos;
	int channels = enc->channels;
	int frames_per_packet = 1;//enc->frames_per_packet;
	int samples_per_channel = samplesInput / channels / frames_per_packet;
//	int max_data = (bufferSize-frame_extra_bytes) / frames_per_packet;
	int max_data = enc->bytes_per_frame - frame_extra_bytes;
//	EPRINTF("opusEncEncode: samplesInput=%i samples_per_channel=%i\n", (int)samplesInput, samples_per_channel);
	for (i=0, frame_bytes=0, pcm_pos=0; i<frames_per_packet; i++, pcm_pos+=samples_per_channel*channels)
	{
		frame_bytes += opus_encode(oe, ((opus_int16*)inputBuffer)+pcm_pos, samples_per_channel, &outputBuffer[frame_extra_bytes+frame_bytes], max_data);
		if (frame_bytes < 0)
			EPRINTF("opusEncEncode: opus_encode returned: %s\n", opus_strerror(frame_bytes));
	}
#if CRC_BYTES != 0
	enc->CRCObject->Reset(8*CRC_BYTES);
	for (i=CRC_BYTES; i<frame_bytes; i++)
		enc->CRCObject->AddByte(outputBuffer[i]);
	int crc = enc->CRCObject->GetCRC();
# if CRC_BYTES == 1
	outputBuffer[0] = crc;
# elif CRC_BYTES == 2
	outputBuffer[0] = (crc >> 8) & 0xFF;
	outputBuffer[1] = (crc >> 0) & 0xFF;
# else
#  error wrong CRC_BYTES
# endif
#endif
	return frame_bytes + frame_extra_bytes;
}


int opusEncClose(
	opus_encoder *enc
	)
{
//	EPRINTF("opusEncClose\n");
	if (enc)
	{
		if (enc->CRCObject)
			delete enc->CRCObject;
		if (enc->oe)
			opus_encoder_destroy(enc->oe);
		free(enc);
	}
	return 0;
}

void opusEncSetParam(opus_encoder *enc,
		CAudioParam& AudioParam
	)
{
	int ret, value;

	/* Reset, must be the first */
	if (AudioParam.bOPUSRequestReset)
	{
		ret = opus_encoder_ctl(enc->oe, OPUS_RESET_STATE);
		if (ret != OPUS_OK)
			EPRINTF("opusEncSetParam: OPUS_RESET_STATE returned: %s\n", opus_strerror(ret));
	}

	/* Channels */
	switch (AudioParam.eOPUSChan)
	{
	case CAudioParam::OC_MONO:
		value = 1;
		break;
	default:
	case CAudioParam::OC_STEREO:
		value = 2;
		break;
	}
	ret = opus_encoder_ctl(enc->oe, OPUS_SET_FORCE_CHANNELS(value));
	if (ret != OPUS_OK)
		EPRINTF("opusEncSetParam: OPUS_SET_FORCE_CHANNELS returned: %s\n", opus_strerror(ret));

	/* Bandwidth */
	switch (AudioParam.eOPUSBandwidth)
	{
	case CAudioParam::OB_NB:
		value = OPUS_BANDWIDTH_NARROWBAND;
		break;
	case CAudioParam::OB_MB:
		value = OPUS_BANDWIDTH_MEDIUMBAND;
		break;
	case CAudioParam::OB_WB:
		value = OPUS_BANDWIDTH_WIDEBAND;
		break;
	case CAudioParam::OB_SWB:
		value = OPUS_BANDWIDTH_SUPERWIDEBAND;
		break;
	default:
	case CAudioParam::OB_FB:
		value = OPUS_BANDWIDTH_FULLBAND;
		break;
	}
	ret = opus_encoder_ctl(enc->oe, OPUS_SET_BANDWIDTH(value));
	if (ret != OPUS_OK)
		EPRINTF("opusEncSetParam: OPUS_SET_BANDWIDTH returned: %s\n", opus_strerror(ret));
	ret = opus_encoder_ctl(enc->oe, OPUS_SET_MAX_BANDWIDTH(value));
	if (ret != OPUS_OK)
		EPRINTF("opusEncSetParam: OPUS_SET_MAX_BANDWIDTH returned: %s\n", opus_strerror(ret));

	/* FEC */
    value = AudioParam.bOPUSForwardErrorCorrection;
	ret = opus_encoder_ctl(enc->oe, OPUS_SET_INBAND_FEC(value));
	if (ret != OPUS_OK)
		EPRINTF("opusEncSetParam: OPUS_SET_INBAND_FEC returned: %s\n", opus_strerror(ret));
	ret = opus_encoder_ctl(enc->oe, OPUS_SET_PACKET_LOSS_PERC(value ? 100 : 0));
	if (ret != OPUS_OK)
		EPRINTF("opusEncSetParam: OPUS_SET_PACKET_LOSS_PERC returned: %s\n", opus_strerror(ret));

	if (AudioParam.bOPUSRequestReset)
	{
		AudioParam.bOPUSRequestReset = FALSE;

		/* Signal */
		switch (AudioParam.eOPUSSignal)
		{
		case CAudioParam::OG_VOICE:
			value = OPUS_SIGNAL_VOICE;
			break;
		default:
		case CAudioParam::OG_MUSIC:
			value = OPUS_SIGNAL_MUSIC;
			break;
		}
		ret = opus_encoder_ctl(enc->oe, OPUS_SET_SIGNAL(value));
		if (ret != OPUS_OK)
			EPRINTF("opusEncSetParam: OPUS_SET_SIGNAL returned: %s\n", opus_strerror(ret));

		/* Application */
		switch (AudioParam.eOPUSApplication)
		{
		case CAudioParam::OA_VOIP:
			value = OPUS_APPLICATION_VOIP;
			break;
		default:
		case CAudioParam::OA_AUDIO:
			value = OPUS_APPLICATION_AUDIO;
			break;
		}
		ret = opus_encoder_ctl(enc->oe, OPUS_SET_APPLICATION(value));
		if (ret != OPUS_OK)
			EPRINTF("opusEncSetParam: OPUS_SET_APPLICATION returned: %s\n", opus_strerror(ret));
	}
}


//###################################################################################################
//# Decoder


opus_decoder *opusDecOpen(
	void
	)
{
//	EPRINTF("opusDecOpen\n");
	opus_decoder *dec = (opus_decoder*)calloc(1, sizeof(opus_decoder));
	if (dec)
	{
		dec->CRCObject = new CCRC();
		if (!dec->CRCObject)
		{
			free(dec);
			dec = NULL;
		}
	}
	return dec;
}

void opusDecClose(
	opus_decoder *dec
	)
{
//	EPRINTF("opusDecClose\n");
	if (dec)
	{
		if (dec->CRCObject)
			delete dec->CRCObject;
		if (dec->od)
			opus_decoder_destroy(dec->od);
		free(dec);
	}
}

int opusDecInit(
	opus_decoder *dec,
	unsigned long samplerate,
	unsigned char channels
	)
{
	/* Check for invalid param */
	if (!dec || !samplerate || channels<1 || channels>2)
	{
		EPRINTF("opusDecInit: invalid param: dec=%p samplerate=%lu channels=%i\n", dec, samplerate, (int)channels);
		return 1; /* error */
	}
	/* Check if something have changed */
	if (!dec->changed && dec->samplerate==(int)samplerate && dec->channels==(int)channels)
		return 0;
	if (dec->od)
	{
		int ret = opus_decoder_ctl(dec->od, OPUS_RESET_STATE);
		if (ret != OPUS_OK)
			EPRINTF("opusDecInit: OPUS_RESET_STATE returned: %s\n", opus_strerror(ret));
	}
	else
	{
		int error;
		OpusDecoder *od = opus_decoder_create(samplerate, channels, &error);
		if (!od)
			EPRINTF("opusDecInit: opus_decoder_create returned: %s\n", opus_strerror(error));
		dec->od = od;
	}
	dec->samplerate = samplerate;
	dec->channels = channels;
	dec->changed = 0;
	return 0;
}

void *opusDecDecode(
	opus_decoder *dec,
	CAudioCodec::EDecError *eDecError,
	int *iChannels,
	unsigned char *buffer,
	unsigned long buffer_size
	)
{
	int frames_per_packet, pcm_len, frame_bytes, sub_frame_bytes, i, pos, pcm_pos;
	int crc_ok=0, corrupted=0;
	int frame_extra_bytes = CRC_BYTES;
	frames_per_packet = 1;
	frame_bytes = buffer_size - frame_extra_bytes;
	if (frame_bytes>=1 && frame_bytes<=OPUS_MAX_DATA_FRAME)
	{
#if CRC_BYTES != 0
		dec->CRCObject->Reset(8*CRC_BYTES);
		for (i=CRC_BYTES; i<frame_bytes; i++)
			dec->CRCObject->AddByte(buffer[i]);
		int crc = dec->CRCObject->GetCRC();
# if CRC_BYTES == 1
		crc_ok = crc == buffer[0];
# elif CRC_BYTES == 2
		crc_ok = crc == ((buffer[0]<<8)|buffer[1]);
# else
#  error wrong CRC_BYTES
# endif
#endif
		sub_frame_bytes = frame_bytes / frames_per_packet;
		for (i=0,pos=0,pcm_pos=0; i<frames_per_packet; i++, pos+=sub_frame_bytes, pcm_pos+=pcm_len*dec->channels)
		{
			pcm_len = opus_decode(dec->od, &buffer[frame_extra_bytes+pos], sub_frame_bytes, dec->out_pcm+pcm_pos, OPUS_MAX_PCM_FRAME, !crc_ok);
			if (pcm_len < 0)
			{
//				EPRINTF("opusDecDecode: opus_decode returned: %s\n", opus_strerror(pcm_len));
				pcm_len = 0;
				corrupted = 1;
			}
		}
	}
	else
	{
		corrupted = 1;
	}

	if (corrupted)
	{
		memset(dec->out_pcm, 0, OPUS_PCM_FRAME_SIZE * sizeof(opus_int16) * dec->channels);
		EPRINTF("opusDecDecode: DECODER_ERROR_CORRUPTED\n");
		*eDecError =  CAudioCodec::DECODER_ERROR_CORRUPTED;
	}
	else if (!crc_ok)
	{
		EPRINTF("opusDecDecode: DECODER_ERROR_CRC\n");
		*eDecError =  CAudioCodec::DECODER_ERROR_CRC;
	}
	else
	{
		*eDecError = CAudioCodec::DECODER_ERROR_OK;
		if (frame_bytes >= 1)
			dec->last_good_toc = buffer[frame_extra_bytes];
	}

	*iChannels = dec->channels;
	dec->changed = 1;
	return dec->out_pcm;
}


/******************************************************************************/
/* Implementation *************************************************************/

OpusCodec::OpusCodec() :
	hOpusDecoder(NULL), hOpusEncoder(NULL)
{
#ifndef USE_OPUS_LIBRARY
	if (hOpusLib == NULL)
	{
		hOpusLib = CLibraryLoader::Load(LibNames, LibFuncs);
		if (!hOpusLib)
			cerr << "No usable Opus library found" << endl;
		else
			cerr << "Got Opus library" << endl;
	}
#endif
}
OpusCodec::~OpusCodec()
{
	DecClose();
	EncClose();
}

/******************************************************************************/
/* Decoder Implementation *****************************************************/

string
OpusCodec::DecGetVersion()
{
	return string("Opus version: ") + opusGetVersion();
}

bool
OpusCodec::CanDecode(CAudioParam::EAudCod eAudioCoding)
{
#ifndef USE_OPUS_LIBRARY
	return hOpusLib && eAudioCoding == CAudioParam::AC_OPUS;
#else
	return eAudioCoding == CAudioParam::AC_OPUS;
#endif
}

bool
OpusCodec::DecOpen(CAudioParam& AudioParam, int *iAudioSampleRate, int *iLenDecOutPerChan)
{
	(void)AudioParam;
	const int iSampleRate = 48000;
	if (hOpusDecoder == NULL)
		hOpusDecoder = opusDecOpen();
	if (hOpusDecoder != NULL)
		opusDecInit(hOpusDecoder, iSampleRate, 2);
	*iAudioSampleRate = iSampleRate;
	*iLenDecOutPerChan = AUD_DEC_TRANSFROM_LENGTH;
	return hOpusDecoder != NULL;
}

_SAMPLE*
OpusCodec::Decode(CVector<uint8_t>& vecbyPrepAudioFrame, int *iChannels, CAudioCodec::EDecError *eDecError)
{
	_SAMPLE *sample = NULL;
	if (hOpusDecoder != NULL)
	{
		sample = (_SAMPLE *)opusDecDecode(hOpusDecoder,
			eDecError,
			iChannels,
			&vecbyPrepAudioFrame[0],
			vecbyPrepAudioFrame.size());
	}
	return sample;
}

void
OpusCodec::DecClose()
{
	if (hOpusDecoder != NULL)
	{
		opusDecClose(hOpusDecoder);
		hOpusDecoder = NULL;
	}
}

void
OpusCodec::DecUpdate(CAudioParam& AudioParam)
{
	if (hOpusDecoder != NULL)
	{
        opusSetupParam(AudioParam, hOpusDecoder->last_good_toc);
	}
}

/******************************************************************************/
/* Encoder Implementation *****************************************************/

string
OpusCodec::EncGetVersion()
{
	return DecGetVersion();
}

bool
OpusCodec::CanEncode(CAudioParam::EAudCod eAudioCoding)
{
	return CanDecode(eAudioCoding);
}

bool
OpusCodec::EncOpen(int iSampleRate, int iChannels, unsigned long *lNumSampEncIn, unsigned long *lMaxBytesEncOut)
{
	hOpusEncoder = opusEncOpen(iSampleRate, iChannels,
		0, lNumSampEncIn, lMaxBytesEncOut);
	return hOpusEncoder != NULL;
}

int
OpusCodec::Encode(CVector<_SAMPLE>& vecsEncInData, unsigned long lNumSampEncIn, CVector<uint8_t>& vecsEncOutData, unsigned long lMaxBytesEncOut)
{
	int bytesEncoded = 0;
	if (hOpusEncoder != NULL)
	{
		bytesEncoded = opusEncEncode(hOpusEncoder,
			(opus_int16 *) &vecsEncInData[0],
			lNumSampEncIn, &vecsEncOutData[0],
			lMaxBytesEncOut);
	}
	return bytesEncoded;
}

void
OpusCodec::EncClose()
{
	if (hOpusEncoder != NULL)
	{
		opusEncClose(hOpusEncoder);
		hOpusEncoder = NULL;
	}
}

void
OpusCodec::EncSetBitrate(int iBitRate)
{
	if (hOpusEncoder != NULL)
	{
		hOpusEncoder->bytes_per_frame = iBitRate / SIZEOF__BYTE;
	}
}

void
OpusCodec::EncUpdate(CAudioParam& AudioParam)
{
	if (hOpusEncoder != NULL)
	{
		opusEncSetParam(hOpusEncoder, AudioParam);
	}
}

