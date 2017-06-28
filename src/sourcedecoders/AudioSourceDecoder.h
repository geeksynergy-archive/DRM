/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 * Volker Fischer, Ollie Haffenden
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

#ifndef _AUIDOSOURCEDECODER_H_INCLUDED_
#define _AUIDOSOURCEDECODER_H_INCLUDED_

#include "../GlobalDefinitions.h"
#include "../Parameter.h"
#include "../util/Modul.h"
#include "../util/CRC.h"
#include "../TextMessage.h"
#include "../resample/Resample.h"
#include "../datadecoding/DataDecoder.h"
#include "../util/Utilities.h"
#include "AudioCodec.h"

/* Definitions ****************************************************************/

// TEST -> for now only use CRC calculation
#define USE_CELP_DECODER

#define USE_HVXC_DECODER


/* Elements of table for UEP parameters for CELP:
 - CELP bit rate index
 - Bit rate (bits / s)
 - Audio frame length (ms)
 - Higher protected part (bits/audio frame)
 - Lower protected part (bits/audio frame)
 - Higher protected part (bytes/audio super frame)
 - Lower protected part (bytes/audio super frame)
 - Audio super frame length (bytes) */
/* 8 kHz sampling rate */
#define LEN_CELP_8KHZ_UEP_PARAMS_TAB 27
const int iTableCELP8kHzUEPParams[LEN_CELP_8KHZ_UEP_PARAMS_TAB][8] = {
    { 3850, 40, 36, 118,  45, 148, 193},
    { 4250, 40, 36, 134,  45, 168, 213},
    { 4650, 40, 36, 150,  45, 188, 233},
    {    0,  0,  0,   0,   0,   0,   0},
    {    0,  0,  0,   0,   0,   0,   0},
    {    0,  0,  0,   0,   0,   0,   0},
    { 5700, 20, 24,  90,  60, 225, 285},
    { 6000, 20, 24,  96,  60, 240, 300},
    { 6300, 20, 24, 102,  60, 255, 315},
    { 6600, 20, 24, 108,  60, 270, 330},
    { 6900, 20, 24, 114,  60, 285, 345},
    { 7100, 20, 24, 118,  60, 295, 355},
    { 7300, 20, 24, 122,  60, 305, 365},
    { 7700, 20, 36, 118,  90, 295, 385},
    { 8300, 20, 36, 130,  90, 325, 415},
    { 8700, 20, 36, 138,  90, 345, 435},
    { 9100, 20, 36, 146,  90, 365, 455},
    { 9500, 20, 36, 154,  90, 385, 475},
    { 9900, 20, 36, 162,  90, 405, 495},
    {10300, 20, 36, 170,  90, 425, 515},
    {10500, 20, 36, 174,  90, 435, 525},
    {10700, 20, 36, 178,  90, 445, 535},
    {11000, 10, 24,  86, 120, 430, 550},
    {11400, 10, 24,  90, 120, 450, 570},
    {11800, 10, 24,  94, 120, 470, 590},
    {12000, 10, 24,  96, 120, 480, 600},
    {12200, 10, 24,  98, 120, 490, 610}
};

/* 16 kHz sampling rate */
#define LEN_CELP_16KHZ_UEP_PARAMS_TAB 32
const int iTableCELP16kHzUEPParams[LEN_CELP_16KHZ_UEP_PARAMS_TAB][8] = {
    {10900, 20, 64, 154, 160, 385, 545},
    {11500, 20, 64, 166, 160, 415, 575},
    {12100, 20, 64, 178, 160, 445, 605},
    {12700, 20, 64, 190, 160, 475, 635},
    {13300, 20, 64, 202, 160, 505, 665},
    {13900, 20, 64, 214, 160, 535, 695},
    {14300, 20, 64, 222, 160, 555, 715},
    {    0,  0,  0,   0,   0,   0,   0},
    {14700, 20, 92, 202, 230, 505, 735},
    {15900, 20, 92, 226, 230, 565, 795},
    {17100, 20, 92, 250, 230, 625, 855},
    {17900, 20, 92, 266, 230, 665, 895},
    {18700, 20, 92, 282, 230, 705, 935},
    {19500, 20, 92, 298, 230, 745, 975},
    {20300, 20, 92, 314, 230, 785, 1015},
    {21100, 20, 92, 330, 230, 825, 1055},
    {13600, 10, 50,  86, 250, 430, 680},
    {14200, 10, 50,  92, 250, 460, 710},
    {14800, 10, 50,  98, 250, 490, 740},
    {15400, 10, 50, 104, 250, 520, 770},
    {16000, 10, 50, 110, 250, 550, 800},
    {16600, 10, 50, 116, 250, 580, 830},
    {17000, 10, 50, 120, 250, 600, 850},
    {    0,  0,  0,   0,   0,   0,   0},
    {17400, 10, 64, 110, 320, 550, 870},
    {18600, 10, 64, 122, 320, 610, 930},
    {19800, 10, 64, 134, 320, 670, 990},
    {20600, 10, 64, 142, 320, 710, 1030},
    {21400, 10, 64, 150, 320, 750, 1070},
    {22200, 10, 64, 158, 320, 790, 1110},
    {23000, 10, 64, 166, 320, 830, 1150},
    {23800, 10, 64, 174, 320, 870, 1190}
};


/* Classes ********************************************************************/

class CAudioSourceDecoder : public CReceiverModul<_BINARY, _SAMPLE>
{
public:
    CAudioSourceDecoder();

    virtual ~CAudioSourceDecoder();

    bool CanDecode(CAudioParam::EAudCod eAudCod) {
        switch (eAudCod)
        {
        case CAudioParam::AC_NONE: return true;
        case CAudioParam::AC_AAC:  return bCanDecodeAAC;
        case CAudioParam::AC_CELP: return bCanDecodeCELP;
        case CAudioParam::AC_HVXC: return bCanDecodeHVXC;
        case CAudioParam::AC_OPUS: return bCanDecodeOPUS;
        }
        return false;
    }
    int GetNumCorDecAudio();
    void SetReverbEffect(const _BOOLEAN bNER) {
        bUseReverbEffect = bNER;
    }
    _BOOLEAN GetReverbEffect() {
        return bUseReverbEffect;
    }

    _BOOLEAN bWriteToFile;

protected:
    enum EInitErr {ET_ALL, ET_AUDDECODER}; /* ET: Error type */
    class CInitErr
    {
    public:
        CInitErr(EInitErr eNewErrType) : eErrType(eNewErrType) {}
        EInitErr eErrType;
    };

    /* General */
    _BOOLEAN DoNotProcessData;
    _BOOLEAN DoNotProcessAudDecoder;
    int iTotalFrameSize;
    int iNumCorDecAudio;

    /* Text message */
    _BOOLEAN bTextMessageUsed;
    CTextMessageDecoder TextMessage;
    CVector<_BINARY> vecbiTextMessBuf;

    /* Resampling */
    int iResOutBlockSize;

    CAudioResample ResampleObjL;
    CAudioResample ResampleObjR;

    CVector<_REAL> vecTempResBufInLeft;
    CVector<_REAL> vecTempResBufInRight;
    CVector<_REAL> vecTempResBufOutCurLeft;
    CVector<_REAL> vecTempResBufOutCurRight;
    CVector<_REAL> vecTempResBufOutOldLeft;
    CVector<_REAL> vecTempResBufOutOldRight;

    /* Drop-out masking (reverberation) */
    _BOOLEAN bAudioWasOK;
    _BOOLEAN bUseReverbEffect;
    CAudioReverb AudioRev;

    int iLenDecOutPerChan;
    int iNumAudioFrames;

    CAudioParam::EAudCod eAudioCoding;
	CAudioCodec* codec;

    int iNumBorders;
    int iNumHigherProtectedBytes;
    int iMaxLenOneAudFrame;

    int iBadBlockCount;
    int iAudioPayloadLen;

    /* HVXC decoding */
    CMatrix<_BINARY> hvxc_frame;
    int iNumHvxcBits;

    /* CELP decoding */
    CMatrix<_BINARY> celp_frame;
    CVector<_BYTE> celp_crc_bits;
    int iNumHigherProtectedBits;
    int iNumLowerProtectedBits;
    _BOOLEAN bCELPCRC;
    CCRC CELPCRCObject;

    string audiodecoder;
    bool bCanDecodeAAC;
    bool bCanDecodeCELP;
    bool bCanDecodeHVXC;
    bool bCanDecodeOPUS;

    FILE *pFile;

#ifdef USE_CELP_DECODER
    /* TODO put here decoder specific things */
#endif
    virtual void InitInternal(CParameter& Parameters);
    virtual void ProcessDataInternal(CParameter& Parameters);
    string AACFileName(CParameter&);
    string CELPFileName(CParameter&);
    string HVXCFileName(CParameter&);
	void CloseDecoder();
};

#endif // _AUIDOSOURCEDECODER_H_INCLUDED_
