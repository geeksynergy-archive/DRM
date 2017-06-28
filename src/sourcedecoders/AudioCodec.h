/******************************************************************************\
 *
 * Copyright (c) 2013
 *
 * Author(s):
 *  David Flamand
 *
 * Description:
 *  See AudioCodec.cpp
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

#ifndef _AUDIOCODEC_H_
#define _AUDIOCODEC_H_

#include "../GlobalDefinitions.h"
#include "../Parameter.h"
#include <string>
#include <vector>

#define AC_NULL ((CAudioParam::EAudCod)-1)

class CAudioCodec
{
public:
	virtual ~CAudioCodec() {}
	/* Decoder */
	enum EDecError { DECODER_ERROR_OK, DECODER_ERROR_CRC, DECODER_ERROR_CORRUPTED, DECODER_ERROR_UNKNOWN };
	virtual string DecGetVersion() = 0;
	virtual bool CanDecode(CAudioParam::EAudCod eAudioCoding) = 0;
	virtual bool DecOpen(CAudioParam& AudioParam, int *iAudioSampleRate, int *iLenDecOutPerChan) = 0;
	virtual _SAMPLE* Decode(CVector<uint8_t>& vecbyPrepAudioFrame, int *iChannels, EDecError *eDecError) = 0;
	virtual void DecClose() = 0;
	virtual void DecUpdate(CAudioParam& AudioParam) = 0;
	/* Encoder */
	virtual string EncGetVersion() = 0;
	virtual bool CanEncode(CAudioParam::EAudCod eAudioCoding) = 0;
	virtual bool EncOpen(int iSampleRate, int iChannels, unsigned long *lNumSampEncIn, unsigned long *lMaxBytesEncOut) = 0;
	virtual int Encode(CVector<_SAMPLE>& vecsEncInData, unsigned long lNumSampEncIn, CVector<uint8_t>& vecsEncOutData, unsigned long lMaxBytesEncOut) = 0;
	virtual void EncClose() = 0;
	virtual void EncSetBitrate(int iBitRate) = 0;
	virtual void EncUpdate(CAudioParam& AudioParam) = 0;
	/* Common */
	static void InitCodecList();
	static void UnrefCodecList();
	static CAudioCodec* GetDecoder(CAudioParam::EAudCod eAudioCoding, bool bCanReturnNullPtr=false);
	static CAudioCodec* GetEncoder(CAudioParam::EAudCod eAudioCoding, bool bCanReturnNullPtr=false);
private:
	static vector<CAudioCodec*> CodecList;
	static int RefCount;
};

#endif // _AUDIOCODEC_H_
