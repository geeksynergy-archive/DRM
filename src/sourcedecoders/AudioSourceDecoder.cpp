/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Audio source encoder/decoder
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

#include "AudioSourceDecoder.h"
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#ifdef _WIN32
# include <direct.h>
#else
# include <sys/stat.h>
# include <sys/types.h>
#endif


/* Implementation *************************************************************/

CAudioSourceDecoder::CAudioSourceDecoder()
    :	bWriteToFile(FALSE), TextMessage(FALSE),
        bUseReverbEffect(TRUE), codec(NULL), pFile(NULL)
{
    /* Initialize Audio Codec List */
    CAudioCodec::InitCodecList();

    /* Needed by fdrmdialog.cpp to report missing codec */
    bCanDecodeAAC  = CAudioCodec::GetDecoder(CAudioParam::AC_AAC,  true) != NULL;
    bCanDecodeCELP = CAudioCodec::GetDecoder(CAudioParam::AC_CELP, true) != NULL;
    bCanDecodeHVXC = CAudioCodec::GetDecoder(CAudioParam::AC_HVXC, true) != NULL;
    bCanDecodeOPUS = CAudioCodec::GetDecoder(CAudioParam::AC_OPUS, true) != NULL;
}

CAudioSourceDecoder::~CAudioSourceDecoder()
{
    /* Unreference Audio Codec List */
    CAudioCodec::UnrefCodecList();
}

string
CAudioSourceDecoder::AACFileName(CParameter & Parameters)
{
    // Store AAC-data in file
    stringstream ss;
    ss << "test/aac_";

//    Parameters.Lock(); // TODO CAudioSourceDecoder::InitInternal() already have the lock
    if (Parameters.
            Service[Parameters.GetCurSelAudioService()].AudioParam.
            eAudioSamplRate == CAudioParam::AS_12KHZ)
    {
        ss << "12kHz_";
    }
    else
        ss << "24kHz_";

    switch (Parameters.
            Service[Parameters.GetCurSelAudioService()].
            AudioParam.eAudioMode)
    {
    case CAudioParam::AM_MONO:
        ss << "mono";
        break;

    case CAudioParam::AM_P_STEREO:
        ss << "pstereo";
        break;

    case CAudioParam::AM_STEREO:
        ss << "stereo";
        break;
    }

    if (Parameters.
            Service[Parameters.GetCurSelAudioService()].AudioParam.
            eSBRFlag == CAudioParam::SB_USED)
    {
        ss << "_sbr";
    }
//    Parameters.Unlock(); // TODO CAudioSourceDecoder::InitInternal() already have the lock
    ss << ".dat";

    return ss.str();
}

string
CAudioSourceDecoder::CELPFileName(CParameter & Parameters)
{
    stringstream ss;
    ss << "test/celp_";
//    Parameters.Lock(); // TODO CAudioSourceDecoder::InitInternal() already have the lock
    if (Parameters.Service[Parameters.GetCurSelAudioService()].
            AudioParam.eAudioSamplRate == CAudioParam::AS_8_KHZ)
    {
        ss << "8kHz_" << 
            iTableCELP8kHzUEPParams
                 [Parameters.
                  Service[Parameters.GetCurSelAudioService()].
                  AudioParam.iCELPIndex][0];
    }
    else
    {
        ss << "16kHz_" <<
            iTableCELP16kHzUEPParams
                 [Parameters.
                  Service[Parameters.GetCurSelAudioService()].
                  AudioParam.iCELPIndex][0];
    }
    ss << "bps";

    if (Parameters.Service[Parameters.GetCurSelAudioService()].
            AudioParam.eSBRFlag == CAudioParam::SB_USED)
    {
        ss << "_sbr";
    }
//    Parameters.Unlock(); // TODO CAudioSourceDecoder::InitInternal() already have the lock
    ss << ".dat";

    return ss.str();
}

string
CAudioSourceDecoder::HVXCFileName(CParameter & Parameters)
{
    stringstream ss;
    ss << "test/hvxc_";
//    Parameters.Lock(); // TODO CAudioSourceDecoder::InitInternal() already have the lock
    if (Parameters.Service[Parameters.GetCurSelAudioService()].
            AudioParam.eAudioSamplRate == CAudioParam::AS_8_KHZ)
    {
        ss << "8kHz";
    }
    else
    {
        ss << "unknown";
    }

    if (Parameters.Service[Parameters.GetCurSelAudioService()].
            AudioParam.eHVXCRate == CAudioParam::HR_2_KBIT)
    {
        ss << "_2kbps";
    }
    else if (Parameters.Service[Parameters.GetCurSelAudioService()].
             AudioParam.eHVXCRate == CAudioParam::HR_4_KBIT)
    {
        ss << "_4kbps";
    }
    else
    {
        ss << "_unknown";
    }

    if (Parameters.Service[Parameters.GetCurSelAudioService()].
            AudioParam.bHVXCCRC)
    {
        ss << "_crc";
    }

    if (Parameters.Service[Parameters.GetCurSelAudioService()].
            AudioParam.eSBRFlag == CAudioParam::SB_USED)
    {
        ss << "_sbr";
    }
//    Parameters.Unlock(); // TODO CAudioSourceDecoder::InitInternal() already have the lock
    ss << ".dat";

    return ss.str();
}

void
CAudioSourceDecoder::ProcessDataInternal(CParameter & Parameters)
{
    int i, j;
    _BOOLEAN bCurBlockOK;
    _BOOLEAN bCurBlockFaulty;
    _BOOLEAN bGoodValues;

    int iDecChannels;
    CAudioCodec::EDecError eDecError;
    short *psDecOutSampleBuf;

    bGoodValues = FALSE;

    Parameters.Lock();
    Parameters.vecbiAudioFrameStatus.Init(0);
    Parameters.vecbiAudioFrameStatus.ResetBitAccess();
    Parameters.Unlock();

    /* Check if something went wrong in the initialization routine */
    if (DoNotProcessData == TRUE)
    {
        return;
    }

    /* Text Message ********************************************************** */
    /* Total frame size depends on whether text message is used or not */
    if (bTextMessageUsed == TRUE)
    {
        /* Decode last for bytes of input block for text message */
        for (i = 0; i < SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR; i++)
            vecbiTextMessBuf[i] = (*pvecInputData)[iTotalFrameSize + i];

        TextMessage.Decode(vecbiTextMessBuf);
    }

    /* Audio data header parsing ********************************************* */
    /* Check if audio shall not be decoded */
    if (DoNotProcessAudDecoder == TRUE)
    {
        return;
    }

    /* Reset bit extraction access */
    (*pvecInputData).ResetBitAccess();

    vector< vector<uint8_t> > audio_frame(iNumAudioFrames);
    vector<uint8_t> aac_crc_bits(iNumAudioFrames);

    /* Check which audio coding type is used */
    if (eAudioCoding == CAudioParam::AC_AAC || eAudioCoding == CAudioParam::AC_OPUS)
    {
        /* AAC super-frame-header ------------------------------------------- */
        bGoodValues = TRUE;
        size_t iPrevBorder = 0;

        for (i = 0; i < iNumBorders; i++)
        {
            /* Frame border in bytes (12 bits) */
            size_t iFrameBorder = (*pvecInputData).Separate(12);

            /* The length is difference between borders */
            if(iFrameBorder>=iPrevBorder)
            {
                int size = iFrameBorder - iPrevBorder;
                if (size < iNumHigherProtectedBytes)
                    size = iNumHigherProtectedBytes;
                else if (size > iMaxLenOneAudFrame)
                    size = iMaxLenOneAudFrame;
                audio_frame[i].resize(size);
            }
            else
                bGoodValues = FALSE;
            iPrevBorder = iFrameBorder;
        }

        /* Byte-alignment (4 bits) in case of odd number of borders */
        if (iNumBorders & 1)
            (*pvecInputData).Separate(4);

        /* Frame length of last frame */
        if (iNumBorders != iNumAudioFrames)
        {
            if(iAudioPayloadLen>=int(iPrevBorder))
            {
                int size = iAudioPayloadLen - iPrevBorder;
                if (size < iNumHigherProtectedBytes)
                    size = iNumHigherProtectedBytes;
                else if (size > iMaxLenOneAudFrame)
                    size = iMaxLenOneAudFrame;
                audio_frame[iNumBorders].resize(size);
            }
            else
                bGoodValues = FALSE;
        }

        /* Check if frame length entries represent possible values */
/*      // check not needed anymore, the size is checked above     //
        for (i = 0; i < iNumAudioFrames; i++)
        {
            if(int(audio_frame[i].size()) > iMaxLenOneAudFrame)
            {
                bGoodValues = FALSE;
            }
        }
*/
        if (bGoodValues == TRUE)
        {
            /* Higher-protected part */
            for (i = 0; i < iNumAudioFrames; i++)
            {
                /* Extract higher protected part bytes (8 bits per byte) */
                for (j = 0; j < iNumHigherProtectedBytes; j++)
                    audio_frame[i][j] = _BINARY((*pvecInputData).Separate(8));

                /* Extract CRC bits (8 bits) */
                aac_crc_bits[i] = _BINARY((*pvecInputData).Separate(8));
            }

            /* Lower-protected part */
            for (i = 0; i < iNumAudioFrames; i++)
            {
                /* First calculate frame length, derived from higher protected
                   part frame length and total size */
                const int iNumLowerProtectedBytes =
                    audio_frame[i].size() - iNumHigherProtectedBytes;

                /* Extract lower protected part bytes (8 bits per byte) */
                for (j = 0; j < iNumLowerProtectedBytes; j++)
                {
                    audio_frame[i][iNumHigherProtectedBytes + j] =
                        _BINARY((*pvecInputData).Separate(8));
                }
            }
        }
    }
    else if (eAudioCoding == CAudioParam::AC_CELP)
    {
        /* celp_super_frame(celp_table_ind) --------------------------------- */
        /* Higher-protected part */
        for (i = 0; i < iNumAudioFrames; i++)
        {
            celp_frame[i].ResetBitAccess();

            /* Extract higher protected part bits */
            for (j = 0; j < iNumHigherProtectedBits; j++)
                celp_frame[i].Enqueue((*pvecInputData).Separate(1), 1);

            /* Extract CRC bits (8 bits) if used */
            if (bCELPCRC == TRUE)
                celp_crc_bits[i] = _BINARY((*pvecInputData).Separate(8));
        }

        /* Lower-protected part */
        for (i = 0; i < iNumAudioFrames; i++)
        {
            for (j = 0; j < iNumLowerProtectedBits; j++)
                celp_frame[i].Enqueue((*pvecInputData).Separate(1), 1);
        }
    }
    else if (eAudioCoding == CAudioParam::AC_HVXC)
    {
        for (i = 0; i < iNumAudioFrames; i++)
        {
            hvxc_frame[i].ResetBitAccess();

            for (j = 0; j < iNumHvxcBits; j++)
                hvxc_frame[i].Enqueue((*pvecInputData).Separate(1), 1);
        }
    }


    /* Audio decoding ******************************************************** */
    /* Init output block size to zero, this variable is also used for
       determining the position for writing the output vector */
    iOutputBlockSize = 0;
    _BOOLEAN bCodecUpdated = FALSE;

    for (j = 0; j < iNumAudioFrames; j++)
    {
        bCurBlockFaulty = FALSE;
        if (eAudioCoding == CAudioParam::AC_AAC || eAudioCoding == CAudioParam::AC_OPUS)
        {
            if (bGoodValues == TRUE)
            {
                /* Prepare data vector with CRC at the beginning (the definition
                   with faad2 DRM interface) */
                CVector<uint8_t> vecbyPrepAudioFrame(audio_frame[j].size()+1);
                vecbyPrepAudioFrame[0] = aac_crc_bits[j];

                for (i = 0; i < int(audio_frame[j].size()); i++)
                    vecbyPrepAudioFrame[i + 1] = audio_frame[j][i];

                if (bWriteToFile && pFile!=NULL)
                {
                    int iNewFrL = audio_frame[j].size() + 1;
                    fwrite((void *) &iNewFrL, size_t(4), size_t(1), pFile);	// frame length
                    fwrite((void *) &vecbyPrepAudioFrame[0], size_t(1), size_t(iNewFrL), pFile);	// data
                    fflush(pFile);
                }

                /* The actual decoding */
                psDecOutSampleBuf = codec->Decode(vecbyPrepAudioFrame, &iDecChannels, &eDecError);

                /* Call decoder update */
                if (!bCodecUpdated)
                {
                    bCodecUpdated = TRUE;
                    Parameters.Lock();
                    int iCurSelAudioServ = Parameters.GetCurSelAudioService();
                    codec->DecUpdate(Parameters.Service[iCurSelAudioServ].AudioParam);
                    Parameters.Unlock();
                }

                /* OPH: add frame status to vector for RSCI */
                Parameters.Lock();
                Parameters.vecbiAudioFrameStatus.Add(eDecError == CAudioCodec::DECODER_ERROR_OK ? 0 : 1);
                Parameters.Unlock();
                if (!(eAudioCoding == CAudioParam::AC_OPUS && eDecError == CAudioCodec::DECODER_ERROR_CRC && bUseReverbEffect == FALSE) && eDecError != CAudioCodec::DECODER_ERROR_OK)
                {
                    //cerr << "AAC decode error" << endl;
                    bCurBlockOK = FALSE;	/* Set error flag */
                }
                else
                {
                    bCurBlockOK = TRUE;
                    /* Opus can have FEC embeded, thus the audio frame is always OK */
                    if (eAudioCoding == CAudioParam::AC_OPUS && eDecError != CAudioCodec::DECODER_ERROR_OK)
                        bCurBlockFaulty = TRUE;

                    if(psDecOutSampleBuf) // might be dummy decoder
                    {
                        /* Conversion from _SAMPLE vector to _REAL vector for
                           resampling. ATTENTION: We use a vector which was
                           allocated inside the decoder! */
                        if (iDecChannels == 1)
                        {
                            /* Change type of data (short -> real) */
                            for (i = 0; i < iLenDecOutPerChan; i++)
                                vecTempResBufInLeft[i] = psDecOutSampleBuf[i];

                            /* Resample data */
                            ResampleObjL.Resample(vecTempResBufInLeft,
                                                  vecTempResBufOutCurLeft);

                            /* Mono (write the same audio material in both
                               channels) */
                            for (i = 0; i < iResOutBlockSize; i++)
                            {
                                vecTempResBufOutCurRight[i] =
                                    vecTempResBufOutCurLeft[i];
                            }
                        }
                        else
                        {
                            /* Stereo */
                            for (i = 0; i < iLenDecOutPerChan; i++)
                            {
                                vecTempResBufInLeft[i] = psDecOutSampleBuf[i * 2];
                                vecTempResBufInRight[i] =
                                    psDecOutSampleBuf[i * 2 + 1];
                            }

                            /* Resample data */
                            ResampleObjL.Resample(vecTempResBufInLeft,
                                                  vecTempResBufOutCurLeft);
                            ResampleObjR.Resample(vecTempResBufInRight,
                                                  vecTempResBufOutCurRight);
                        }
                    }
                }
            }
            else
            {
                /* DRM AAC header was wrong, set flag to "bad block" */
                bCurBlockOK = FALSE;
                /* OPH: update audio status vector for RSCI */
                Parameters.Lock();
                Parameters.vecbiAudioFrameStatus.Add(1);
                Parameters.Unlock();
            }
        }
        else if (eAudioCoding == CAudioParam::AC_CELP)
        {
            if (bCELPCRC == TRUE)
            {
                /* Prepare CRC object and data stream */
                CELPCRCObject.Reset(8);
                celp_frame[j].ResetBitAccess();

                for (i = 0; i < iNumHigherProtectedBits; i++)
                    CELPCRCObject.AddBit((_BINARY) celp_frame[j].Separate(1));

                bCurBlockOK = CELPCRCObject.CheckCRC(celp_crc_bits[j]);
            }
            else
                bCurBlockOK = TRUE;

            /* OPH: update audio status vector for RSCI */
            Parameters.Lock();
            Parameters.vecbiAudioFrameStatus.Add(bCurBlockOK == TRUE ? 0 : 1);
            Parameters.Unlock();

            int iTotNumBits =
                iNumHigherProtectedBits + iNumLowerProtectedBits;
            if (bWriteToFile && pFile!=NULL)
            {
                int iNewFrL = (iTotNumBits + 7) / 8; //(int) Ceil((CReal) iTotNumBits / 8);
                fwrite((void *) &iNewFrL, size_t(4), size_t(1), pFile);	// frame length
                celp_frame[j].ResetBitAccess();
                for (i = 0; i < iNewFrL; i++)
                {
                    int iNumBits = Min(iTotNumBits - i * 8, 8);
                    _BYTE bCurVal = (_BYTE) celp_frame[j].Separate(iNumBits);
                    fwrite((void *) &bCurVal, size_t(1), size_t(1), pFile);	// data
                }
                fflush(pFile);
            }

#ifdef USE_CELP_DECODER

            /* Write zeros in current output buffer since we do not have a decoder */
            for (i = 0; i < iResOutBlockSize; i++)
            {
                vecTempResBufOutCurLeft[i] = (_REAL) 0.0;
                vecTempResBufOutCurRight[i] = (_REAL) 0.0;
            }

#endif

        }
        else if (eAudioCoding == CAudioParam::AC_HVXC)
        {

            bCurBlockOK = TRUE; /* CRC always ignored */

            if (bWriteToFile && pFile!=NULL)
            {
                int iNewFrL = (iNumHvxcBits + 7) / 8; //(int) Ceil((CReal) iNumHvxcBits / 8);
                fwrite((void *) &iNewFrL, size_t(4), size_t(1), pFile);	// frame length
                hvxc_frame[j].ResetBitAccess();
                for (i = 0; i < iNewFrL; i++)
                {
                    int iNumBits = Min(iNumHvxcBits - i * 8, 8);
                    _BYTE bCurVal = (_BYTE) hvxc_frame[j].Separate(iNumBits);
                    fwrite((void *) &bCurVal, size_t(1), size_t(1), pFile);	// data
                }
                fflush(pFile);
            }

#ifdef USE_HVXC_DECODER

            /* Write zeros in current output buffer since we do not have a decoder */
            for (i = 0; i < iResOutBlockSize; i++)
            {
                vecTempResBufOutCurLeft[i] = (_REAL) 0.0;
                vecTempResBufOutCurRight[i] = (_REAL) 0.0;
            }

#endif

        }
        else
            bCurBlockOK = FALSE;

// This code is independent of particular audio source type and should work
// fine with CELP and HVXC

        /* Postprocessing of audio blocks, status informations -------------- */
		ETypeRxStatus status = DATA_ERROR;
        if (bCurBlockOK == FALSE)
        {
            if (bAudioWasOK == TRUE)
            {
                /* Post message to show that CRC was wrong (yellow light) */
				status = DATA_ERROR;

                /* Fade-out old block to avoid "clicks" in audio. We use linear
                   fading which gives a log-fading impression */
                for (i = 0; i < iResOutBlockSize; i++)
                {
                    /* Linear attenuation with time of OLD buffer */
                    const _REAL rAtt =
                        (_REAL) 1.0 - (_REAL) i / iResOutBlockSize;

                    vecTempResBufOutOldLeft[i] *= rAtt;
                    vecTempResBufOutOldRight[i] *= rAtt;

                    if (bUseReverbEffect == TRUE)
                    {
                        /* Fade in input signal for reverberation to avoid
                           clicks */
                        const _REAL rAttRev = (_REAL) i / iResOutBlockSize;

                        /* Cross-fade reverberation effect */
                        const _REAL rRevSam = (1.0 - rAtt) * AudioRev.
                                              ProcessSample(vecTempResBufOutOldLeft[i] *
                                                            rAttRev,
                                                            vecTempResBufOutOldRight[i] *
                                                            rAttRev);

                        /* Mono reverbration signal */
                        vecTempResBufOutOldLeft[i] += rRevSam;
                        vecTempResBufOutOldRight[i] += rRevSam;
                    }
                }

                /* Set flag to show that audio block was bad */
                bAudioWasOK = FALSE;
            }
            else
            {
				status = CRC_ERROR;

                if (bUseReverbEffect == TRUE)
                {
                    /* Add Reverberation effect */
                    for (i = 0; i < iResOutBlockSize; i++)
                    {
                        /* Mono reverberation signal */
                        vecTempResBufOutOldLeft[i] =
                            vecTempResBufOutOldRight[i] = AudioRev.
                                                          ProcessSample(0, 0);
                    }
                }
            }

            /* Write zeros in current output buffer */
            for (i = 0; i < iResOutBlockSize; i++)
            {
                vecTempResBufOutCurLeft[i] = (_REAL) 0.0;
                vecTempResBufOutCurRight[i] = (_REAL) 0.0;
            }
        }
        else
        {
            /* Increment correctly decoded audio blocks counter */
			if (bCurBlockFaulty) {
				status = DATA_ERROR;
			}
			else {
                iNumCorDecAudio++;
				status = RX_OK;
			}


            if (bAudioWasOK == FALSE)
            {
                if (bUseReverbEffect == TRUE)
                {
                    /* Add "last" reverbration only to old block */
                    for (i = 0; i < iResOutBlockSize; i++)
                    {
                        /* Mono reverberation signal */
                        vecTempResBufOutOldLeft[i] =
                            vecTempResBufOutOldRight[i] = AudioRev.
                                                          ProcessSample(vecTempResBufOutOldLeft[i],
                                                                  vecTempResBufOutOldRight[i]);
                    }
                }

                /* Fade-in new block to avoid "clicks" in audio. We use linear
                   fading which gives a log-fading impression */
                for (i = 0; i < iResOutBlockSize; i++)
                {
                    /* Linear attenuation with time */
                    const _REAL rAtt = (_REAL) i / iResOutBlockSize;

                    vecTempResBufOutCurLeft[i] *= rAtt;
                    vecTempResBufOutCurRight[i] *= rAtt;

                    if (bUseReverbEffect == TRUE)
                    {
                        /* Cross-fade reverberation effect */
                        const _REAL rRevSam = (1.0 - rAtt) * AudioRev.
                                              ProcessSample(0, 0);

                        /* Mono reverberation signal */
                        vecTempResBufOutCurLeft[i] += rRevSam;
                        vecTempResBufOutCurRight[i] += rRevSam;
                    }
                }

                /* Reset flag */
                bAudioWasOK = TRUE;
            }
        }
        Parameters.Lock();
        Parameters.ReceiveStatus.SLAudio.SetStatus(status);
        Parameters.ReceiveStatus.LLAudio.SetStatus(status);
		Parameters.AudioComponentStatus[Parameters.GetCurSelAudioService()].SetStatus(status);
        Parameters.Unlock();

        /* Conversion from _REAL to _SAMPLE with special function */
        for (i = 0; i < iResOutBlockSize; i++)
        {
            (*pvecOutputData)[iOutputBlockSize + i * 2] = Real2Sample(vecTempResBufOutOldLeft[i]);	/* Left channel */
            (*pvecOutputData)[iOutputBlockSize + i * 2 + 1] = Real2Sample(vecTempResBufOutOldRight[i]);	/* Right channel */
        }

        /* Add new block to output block size ("* 2" for stereo output block) */
        iOutputBlockSize += iResOutBlockSize * 2;

        /* Store current audio block */
        for (i = 0; i < iResOutBlockSize; i++)
        {
            vecTempResBufOutOldLeft[i] = vecTempResBufOutCurLeft[i];
            vecTempResBufOutOldRight[i] = vecTempResBufOutCurRight[i];
        }
    }
}

void
CAudioSourceDecoder::InitInternal(CParameter & Parameters)
{
    /* Close previous decoder instance if any */
    CloseDecoder();

    /*
    	Since we use the exception mechanism in this init routine, the sequence of
    	the individual initializations is very important!
    	Requirement for text message is "stream is used" and "audio service".
    	Requirement for AAC decoding are the requirements above plus "audio coding
    	is AAC"
    */
    int iCurAudioStreamID;
    int iMaxLenResamplerOutput;
    int iCurSelServ;
    int iAudioSampleRate;

    /* Init error flags and output block size parameter. The output block
       size is set in the processing routine. We must set it here in case
       of an error in the initialization, this part in the processing
       routine is not being called */
    DoNotProcessAudDecoder = FALSE;
    DoNotProcessData = FALSE;
    iOutputBlockSize = 0;

    /* Set audiodecoder to empty string - means "unknown" and "can't decode" to GUI */
    audiodecoder = "";

    try
    {
        Parameters.Lock();

        /* Init counter for correctly decoded audio blocks */
        iNumCorDecAudio = 0;

        /* Init "audio was ok" flag */
        bAudioWasOK = TRUE;

        /* Get number of total input bits for this module */
        iInputBlockSize = Parameters.iNumAudioDecoderBits;

        /* Get current selected audio service */
        iCurSelServ = Parameters.GetCurSelAudioService();

        /* Get current selected audio param */
        CAudioParam& AudioParam(Parameters.Service[iCurSelServ].AudioParam);

        /* Get current audio coding */
        eAudioCoding = AudioParam.eAudioCoding;

        /* Current audio stream ID */
        iCurAudioStreamID = AudioParam.iStreamID;

        /* The requirement for this module is that the stream is used and the
           service is an audio service. Check it here */
        if ((Parameters.Service[iCurSelServ].eAudDataFlag != CService::SF_AUDIO) ||
                (iCurAudioStreamID == STREAM_ID_NOT_USED))
        {
            throw CInitErr(ET_ALL);
        }

        /* Init text message application ------------------------------------ */
        switch (AudioParam.bTextflag)
        {
        case TRUE:
            bTextMessageUsed = TRUE;

            /* Get a pointer to the string */
            TextMessage.Init(&AudioParam.strTextMessage);

            /* Total frame size is input block size minus the bytes for the text
               message */
            iTotalFrameSize = iInputBlockSize -
                              SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR;

            /* Init vector for text message bytes */
            vecbiTextMessBuf.Init(SIZEOF__BYTE *
                                  NUM_BYTES_TEXT_MESS_IN_AUD_STR);
            break;

        case FALSE:
            bTextMessageUsed = FALSE;

            /* All bytes are used for AAC data, no text message present */
            iTotalFrameSize = iInputBlockSize;
            break;
        }

        /* Get decoder instance */
        codec = CAudioCodec::GetDecoder(eAudioCoding);

        if (codec->CanDecode(eAudioCoding))
            audiodecoder = codec->DecGetVersion();

        if (eAudioCoding == CAudioParam::AC_AAC)
        {
            /* Init for AAC decoding ---------------------------------------- */
            int iNumHeaderBytes;

            /* Length of higher protected part of audio stream */
            const int iLenAudHigh =
                Parameters.Stream[iCurAudioStreamID].iLenPartA;

            /* Set number of AAC frames in a AAC super-frame */
            switch (AudioParam.eAudioSamplRate)	/* Only 12 kHz and 24 kHz is allowed */
            {
            case CAudioParam::AS_12KHZ:
                iNumAudioFrames = 5;
                iNumHeaderBytes = 6;
                break;

            case CAudioParam::AS_24KHZ:
                iNumAudioFrames = 10;
                iNumHeaderBytes = 14;
                break;

            default:
                /* Some error occurred, throw error */
                throw CInitErr(ET_AUDDECODER);
                break;
            }

            /* Number of borders */
            iNumBorders = iNumAudioFrames - 1;

            /* The audio_payload_length is derived from the length of the audio
               super frame (data_length_of_part_A + data_length_of_part_B)
               subtracting the audio super frame overhead (bytes used for the
               audio super frame header() and for the aac_crc_bits)
               (5.3.1.1, Table 5) */
            iAudioPayloadLen = iTotalFrameSize / SIZEOF__BYTE -
                               iNumHeaderBytes - iNumAudioFrames;

            /* Check iAudioPayloadLen value, only positive values make sense */
            if (iAudioPayloadLen < 0)
                throw CInitErr(ET_AUDDECODER);

            /* Calculate number of bytes for higher protected blocks */
            iNumHigherProtectedBytes = (iLenAudHigh - iNumHeaderBytes -
                                        iNumAudioFrames /* CRC bytes */ ) /
                                        iNumAudioFrames;

            if (iNumHigherProtectedBytes < 0)
                iNumHigherProtectedBytes = 0;

            /* The maximum length for one audio frame is "iAudioPayloadLen". The
               regular size will be much shorter since all audio frames share
               the total size, but we do not know at this time how the data is
               split in the transmitter source coder */
            iMaxLenOneAudFrame = iAudioPayloadLen;

            /* Init AAC decoder */
			codec->DecOpen(AudioParam, &iAudioSampleRate, &iLenDecOutPerChan);

            if(bWriteToFile)
            {
                string fn = AACFileName(Parameters);
                if(pFile)
                    fclose(pFile);
                pFile = fopen(fn.c_str(), "wb");
            }
        }
        else if (eAudioCoding == CAudioParam::AC_CELP)
        {
            /* Init for CELP decoding --------------------------------------- */
            int iCurCelpIdx, iCelpFrameLength;

            /* Set number of frames in a super-frame */
            switch (AudioParam.eAudioSamplRate)	/* Only 8000 and 16000 is allowed */
            {
            case CAudioParam::AS_8_KHZ:
                /* Check range */
                iCurCelpIdx = AudioParam.iCELPIndex;

                if ((iCurCelpIdx > 0) &&
                        (iCurCelpIdx < LEN_CELP_8KHZ_UEP_PARAMS_TAB))
                {
                    /* CELP frame length */
                    iCelpFrameLength =
                        iTableCELP8kHzUEPParams[iCurCelpIdx][1];

                    /* Number of bits for lower and higher protected parts */
                    iNumHigherProtectedBits =
                        iTableCELP8kHzUEPParams[iCurCelpIdx][2];
                    iNumLowerProtectedBits =
                        iTableCELP8kHzUEPParams[iCurCelpIdx][3];
                }
                else
                    throw CInitErr(ET_AUDDECODER);

                /* Set audio sample rate */
                iAudioSampleRate = 8000;
                break;

            case CAudioParam::AS_16KHZ:
                /* Check range */
                iCurCelpIdx = AudioParam.iCELPIndex;

                if ((iCurCelpIdx > 0) &&
                        (iCurCelpIdx < LEN_CELP_16KHZ_UEP_PARAMS_TAB))
                {
                    /* CELP frame length */
                    iCelpFrameLength =
                        iTableCELP16kHzUEPParams[iCurCelpIdx][1];

                    /* Number of bits for lower and higher protected parts */
                    iNumHigherProtectedBits =
                        iTableCELP16kHzUEPParams[iCurCelpIdx][2];
                    iNumLowerProtectedBits =
                        iTableCELP16kHzUEPParams[iCurCelpIdx][3];
                }
                else
                    throw CInitErr(ET_AUDDECODER);

                /* Set audio sample rate */
                iAudioSampleRate = 16000;
                break;

            default:
                /* Some error occurred, throw error */
                throw CInitErr(ET_AUDDECODER);
                break;
            }

            /* Check lengths of iNumHigherProtectedBits and
               iNumLowerProtectedBits for overrun */
            const int iTotalNumCELPBits =
                iNumHigherProtectedBits + iNumLowerProtectedBits;

            if (iTotalNumCELPBits * SIZEOF__BYTE > iTotalFrameSize)
                throw CInitErr(ET_AUDDECODER);

            /* Calculate number of audio frames (one audio super frame is
               always 400 ms long) */
            iNumAudioFrames = 400 /* ms */  / iCelpFrameLength /* ms */ ;

            /* Set CELP CRC flag */
            bCELPCRC = AudioParam.bCELPCRC;

            /* Init vectors storing the CELP raw data and CRCs */
            celp_frame.Init(iNumAudioFrames, iTotalNumCELPBits);
            celp_crc_bits.Init(iNumAudioFrames);

// TEST
            iLenDecOutPerChan = 0;


#ifdef USE_CELP_DECODER

// TODO put decoder initialization here
            bWriteToFile = TRUE;
            if(bWriteToFile)
            {
                string fn = CELPFileName(Parameters);
                if(pFile)
                    fclose(pFile);
                pFile = fopen(fn.c_str(), "wb");
            }

#else
            /* No CELP decoder available */
            throw CInitErr(ET_AUDDECODER);
#endif
        }
        else if (eAudioCoding == CAudioParam::AC_HVXC)
        {
            /* Init for HVXC decoding --------------------------------------- */

			iAudioSampleRate = 8000;
            iNumAudioFrames = 400 / 20;

            iLenDecOutPerChan = 0;

            iNumHvxcBits = 0;
            if (AudioParam.eHVXCRate == CAudioParam::HR_2_KBIT)
            {
                iNumHvxcBits = 40;
                if (AudioParam.bHVXCCRC)
                {
                    iNumHvxcBits += 8;
                }
            }
            else if (AudioParam.eHVXCRate == CAudioParam::HR_4_KBIT)
            {
                iNumHvxcBits = 80;
                if (AudioParam.bHVXCCRC)
                {
                    iNumHvxcBits += 13;
                }
            }

            if ( ! iNumHvxcBits ) {
                throw CInitErr(ET_AUDDECODER);
            }

            hvxc_frame.Init(iNumAudioFrames, iNumHvxcBits);

#ifdef USE_HVXC_DECODER

// TODO put decoder initialization here
            bWriteToFile = TRUE;
            if(bWriteToFile)
            {
                string fn = HVXCFileName(Parameters);
                if(pFile)
                    fclose(pFile);
#ifdef _WIN32
                _mkdir("test");
#else
                mkdir("test", 0777);
#endif
                pFile = fopen(fn.c_str(), "wb");
            }

#else
            /* No HVXC decoder available */
            throw CInitErr(ET_AUDDECODER);
#endif
        }
        else if (eAudioCoding == CAudioParam::AC_OPUS)
        {
            /* Init for OPUS decoding ---------------------------------------- */

            /* Length of higher protected part of audio stream */
            const int iLenAudHigh =
                Parameters.Stream[iCurAudioStreamID].iLenPartA;

            /* Number of audio frame */
            iNumAudioFrames = 20;

            /* Number of borders */
            iNumBorders = iNumAudioFrames;

            /* The audio_payload_length is derived from the length of the audio
               super frame (data_length_of_part_A + data_length_of_part_B)
               subtracting the audio super frame overhead (bytes used for the
               audio super frame header() and for the aac_crc_bits)
               (5.3.1.1, Table 5) */
            iAudioPayloadLen = iTotalFrameSize / SIZEOF__BYTE - iNumAudioFrames;

            /* Check iAudioPayloadLen value, only positive values make sense */
            if (iAudioPayloadLen < 0)
                throw CInitErr(ET_AUDDECODER);

            /* Calculate number of bytes for higher protected blocks */
            iNumHigherProtectedBytes = (iLenAudHigh -
                                        iNumAudioFrames /* CRC bytes */ ) /
                                        iNumAudioFrames;

            if (iNumHigherProtectedBytes < 0)
                iNumHigherProtectedBytes = 0;

            /* The maximum length for one audio frame is "iAudioPayloadLen". The
               regular size will be much shorter since all audio frames share
               the total size, but we do not know at this time how the data is
               split in the transmitter source coder */
            iMaxLenOneAudFrame = iAudioPayloadLen;

            /* Init Opus decoder */
			codec->DecOpen(AudioParam, &iAudioSampleRate, &iLenDecOutPerChan);
        }
        else
        {
            /* Audio codec not supported */
            throw CInitErr(ET_AUDDECODER);
        }

        /* set string for GUI */
        Parameters.audiodecoder = audiodecoder;

        /* Set number of Audio frames for log file */
        Parameters.iNumAudioFrames = iNumAudioFrames;

        /* Since we do not correct for sample rate offsets here (yet), we do not
           have to consider larger buffers. An audio frame always corresponds
           to 400 ms */
        iMaxLenResamplerOutput = (int) ((_REAL) Parameters.GetAudSampleRate() *
                                        (_REAL) 0.4 /* 400ms */  *
                                        2 /* for stereo */ );

        iResOutBlockSize = (int) ((_REAL) iLenDecOutPerChan *
                                  Parameters.GetAudSampleRate() / iAudioSampleRate);

        /* Additional buffers needed for resampling since we need conversation
           between _REAL and _SAMPLE. We have to init the buffers with
           zeros since it can happen, that we have bad CRC right at the
           start of audio blocks */
        vecTempResBufInLeft.Init(iLenDecOutPerChan, (_REAL) 0.0);
        vecTempResBufInRight.Init(iLenDecOutPerChan, (_REAL) 0.0);
        vecTempResBufOutCurLeft.Init(iResOutBlockSize, (_REAL) 0.0);
        vecTempResBufOutCurRight.Init(iResOutBlockSize, (_REAL) 0.0);
        vecTempResBufOutOldLeft.Init(iResOutBlockSize, (_REAL) 0.0);
        vecTempResBufOutOldRight.Init(iResOutBlockSize, (_REAL) 0.0);

        /* Init resample objects */
        ResampleObjL.Init(iLenDecOutPerChan,
                          (_REAL) Parameters.GetAudSampleRate() / iAudioSampleRate);
        ResampleObjR.Init(iLenDecOutPerChan,
                          (_REAL) Parameters.GetAudSampleRate() / iAudioSampleRate);

        /* Clear reverberation object */
        AudioRev.Init(1.0 /* seconds delay */, Parameters.GetAudSampleRate());
        AudioRev.Clear();

        /* With this parameter we define the maximum lenght of the output
           buffer. The cyclic buffer is only needed if we do a sample rate
           correction due to a difference compared to the transmitter. But for
           now we do not correct and we could stay with a single buffer
           Maybe TODO: sample rate correction to avoid audio dropouts */
        iMaxOutputBlockSize = iMaxLenResamplerOutput;

        Parameters.Unlock();
    }

    catch(CInitErr CurErr)
    {
        Parameters.Unlock();

        switch (CurErr.eErrType)
        {
        case ET_ALL:
            /* An init error occurred, do not process data in this module */
            DoNotProcessData = TRUE;
            break;

        case ET_AUDDECODER:
            /* Audio part should not be decdoded, set flag */
            DoNotProcessAudDecoder = TRUE;
            break;

        default:
            DoNotProcessData = TRUE;
        }

        /* In all cases set output size to zero */
        iOutputBlockSize = 0;
    }
}

int
CAudioSourceDecoder::GetNumCorDecAudio()
{
    /* Return number of correctly decoded audio blocks. Reset counter
       afterwards */
    const int iRet = iNumCorDecAudio;

    iNumCorDecAudio = 0;

    return iRet;
}

void
CAudioSourceDecoder::CloseDecoder()
{
    if (codec != NULL)
        codec->DecClose();
}

