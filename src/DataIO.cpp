/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2006
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
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

#include "DataIO.h"
#include <iomanip>
#include <time.h>

/* Implementation *************************************************************/
/******************************************************************************\
* MSC data																	   *
\******************************************************************************/
/* Transmitter -------------------------------------------------------------- */
void CReadData::ProcessDataInternal(CParameter&)
{
    /* Get data from sound interface */
    pSound->Read(vecsSoundBuffer);

    /* Write data to output buffer */
    for (int i = 0; i < iOutputBlockSize; i++)
        (*pvecOutputData)[i] = vecsSoundBuffer[i];

    /* Update level meter */
    SignalLevelMeter.Update((*pvecOutputData));
}

void CReadData::InitInternal(CParameter& Parameters)
{
    /* Get audio sample rate */
    const int iSampleRate = Parameters.GetAudSampleRate();

    /* Define block-size for output, an audio frame always corresponds
       to 400 ms. We use always stereo blocks */
    iOutputBlockSize = (int) ((_REAL) iSampleRate *
                              (_REAL) 0.4 /* 400 ms */ * 2 /* stereo */);

    /* Init sound interface and intermediate buffer */
    pSound->Init(iSampleRate, iOutputBlockSize, FALSE);
    vecsSoundBuffer.Init(iOutputBlockSize);

    /* Init level meter */
    SignalLevelMeter.Init(0);
}

/* Receiver ----------------------------------------------------------------- */
#ifdef QT_MULTIMEDIA_LIB
void
CWriteData::SetSoundInterface(QIODevice* p)
{
    pIODevice = p;
}
#endif


void CWriteData::ProcessDataInternal(CParameter& Parameters)
{
    int i;

    /* Calculate size of each individual audio channel */
    const int iHalfBlSi = iInputBlockSize / 2;

    switch (eOutChanSel)
    {
    case CS_BOTH_BOTH:
        /* left -> left, right -> right (vector sizes might not be the
           same -> use for-loop for copying) */
        for (i = 0; i < iInputBlockSize; i++)
            vecsTmpAudData[i] = (*pvecInputData)[i]; /* Just copy data */
        break;

    case CS_LEFT_LEFT:
        /* left -> left, right muted */
        for (i = 0; i < iHalfBlSi; i++)
        {
            vecsTmpAudData[2 * i] = (*pvecInputData)[2 * i];
            vecsTmpAudData[2 * i + 1] = 0; /* mute */
        }
        break;

    case CS_RIGHT_RIGHT:
        /* left muted, right -> right */
        for (i = 0; i < iHalfBlSi; i++)
        {
            vecsTmpAudData[2 * i] = 0; /* mute */
            vecsTmpAudData[2 * i + 1] = (*pvecInputData)[2 * i + 1];
        }
        break;

    case CS_LEFT_MIX:
        /* left -> mix, right muted */
        for (i = 0; i < iHalfBlSi; i++)
        {
            /* Mix left and right channel together. Prevent overflow! First,
               copy recorded data from "short" in "_REAL" type variables */
            const _REAL rLeftChan = (*pvecInputData)[2 * i];
            const _REAL rRightChan = (*pvecInputData)[2 * i + 1];

            vecsTmpAudData[2 * i] =
                Real2Sample((rLeftChan + rRightChan) * rMixNormConst);

            vecsTmpAudData[2 * i + 1] = 0; /* mute */
        }
        break;

    case CS_RIGHT_MIX:
        /* left muted, right -> mix */
        for (i = 0; i < iHalfBlSi; i++)
        {
            /* Mix left and right channel together. Prevent overflow! First,
               copy recorded data from "short" in "_REAL" type variables */
            const _REAL rLeftChan = (*pvecInputData)[2 * i];
            const _REAL rRightChan = (*pvecInputData)[2 * i + 1];

            vecsTmpAudData[2 * i] = 0; /* mute */
            vecsTmpAudData[2 * i + 1] =
                Real2Sample((rLeftChan + rRightChan) * rMixNormConst);
        }
        break;
    }

    if (bMuteAudio == TRUE)
    {
        /* Clear both channels if muted */
        for (i = 0; i < iInputBlockSize; i++)
            vecsTmpAudData[i] = 0;
    }

    /* Put data to sound card interface. Show sound card state on GUI */

#ifdef QT_MULTIMEDIA_LIB
    bool bBad = true;
    if(pIODevice)
    {
        qint64 n = 2*vecsTmpAudData.Size();
        int m = pIODevice->write((char*)&vecsTmpAudData[0], n);
        if(m==n)
            bBad = false;
    }
#else
    const _BOOLEAN bBad = pSound->Write(vecsTmpAudData);
#endif
    Parameters.Lock();
    Parameters.ReceiveStatus.InterfaceO.SetStatus(bBad ? DATA_ERROR : RX_OK); /* Yellow light */
    Parameters.Unlock();

    /* Write data as wave in file */
    if (bDoWriteWaveFile == TRUE)
    {
        /* Write audio data to file only if it is not zero */
        _BOOLEAN bDoNotWrite = TRUE;
        for (i = 0; i < iInputBlockSize; i++)
        {
            if ((*pvecInputData)[i] != 0)
                bDoNotWrite = FALSE;
        }

        if (bDoNotWrite == FALSE)
        {
            for (i = 0; i < iInputBlockSize; i += 2)
            {
                WaveFileAudio.AddStereoSample((*pvecInputData)[i] /* left */,
                                              (*pvecInputData)[i + 1] /* right */);
            }
        }
    }

    /* Store data in buffer for spectrum calculation */
    vecsOutputData.AddEnd((*pvecInputData), iInputBlockSize);
}

void CWriteData::InitInternal(CParameter& Parameters)
{
    /* Get audio sample rate */
    iAudSampleRate = Parameters.GetAudSampleRate();

    /* Set maximum audio frequency */
	iMaxAudioFrequency = MAX_SPEC_AUDIO_FREQUENCY;
	if (iMaxAudioFrequency > iAudSampleRate/2)
		iMaxAudioFrequency = iAudSampleRate/2;

    /* Length of vector for audio spectrum. We use a power-of-two length to
       make the FFT work more efficient, need to be scaled from sample rate to
       keep the same frequency resolution */
    iNumSmpls4AudioSprectrum = ADJ_FOR_SRATE(1024, iAudSampleRate);

    /* Number of blocks for averaging the audio spectrum */
    iNumBlocksAvAudioSpec = Ceil(((_REAL) iAudSampleRate *
        TIME_AV_AUDIO_SPECT_MS / 1000 / iNumSmpls4AudioSprectrum));

    /* Inits for audio spectrum plotting */
    vecsOutputData.Init((int) iNumBlocksAvAudioSpec * iNumSmpls4AudioSprectrum *
                   2 /* stereo */, 0); /* Init with zeros */
    FftPlan.Init(iNumSmpls4AudioSprectrum);
    veccFFTInput.Init(iNumSmpls4AudioSprectrum);
    veccFFTOutput.Init(iNumSmpls4AudioSprectrum);
    vecrAudioWindowFunction.Init(iNumSmpls4AudioSprectrum);

    /* An audio frame always corresponds to 400 ms.
       We use always stereo blocks */
    const int iAudFrameSize = (int) ((_REAL) iAudSampleRate *
                                     (_REAL) 0.4 /* 400 ms */);

    /* Check if blocking behaviour of sound interface shall be changed */
    if (bNewSoundBlocking != bSoundBlocking)
        bSoundBlocking = bNewSoundBlocking;

    /* Init sound interface with blocking or non-blocking behaviour */
    pSound->Init(iAudSampleRate, iAudFrameSize * 2 /* stereo */, bSoundBlocking);

    /* Init intermediate buffer needed for different channel selections */
    vecsTmpAudData.Init(iAudFrameSize * 2 /* stereo */);

    /* Inits for audio spectrum plot */
    vecrAudioWindowFunction = Hann(iNumSmpls4AudioSprectrum);
    vecsOutputData.Reset(0); /* Reset audio data storage vector */

    /* Define block-size for input (stereo input) */
    iInputBlockSize = iAudFrameSize * 2 /* stereo */;
}

CWriteData::CWriteData(CSoundOutInterface* pNS) :
#ifdef QT_MULTIMEDIA_LIB
        pIODevice(NULL),
#endif
        pSound(pNS), /* Sound interface */
        bMuteAudio(FALSE), bDoWriteWaveFile(FALSE),
        bSoundBlocking(FALSE), bNewSoundBlocking(FALSE),
        eOutChanSel(CS_BOTH_BOTH), rMixNormConst(MIX_OUT_CHAN_NORM_CONST),
        iAudSampleRate(0), iNumSmpls4AudioSprectrum(0), iNumBlocksAvAudioSpec(0),
        iMaxAudioFrequency(MAX_SPEC_AUDIO_FREQUENCY)
{
    /* Constructor */
}

void CWriteData::StartWriteWaveFile(const string& strFileName)
{
    /* No Lock(), Unlock() needed here */
    if (bDoWriteWaveFile == FALSE)
    {
        WaveFileAudio.Open(strFileName, iAudSampleRate);
        bDoWriteWaveFile = TRUE;
    }
}

void CWriteData::StopWriteWaveFile()
{
    Lock();

    WaveFileAudio.Close();
    bDoWriteWaveFile = FALSE;

    Unlock();
}

void CWriteData::GetAudioSpec(CVector<_REAL>& vecrData,
                              CVector<_REAL>& vecrScale)
{
    if (iAudSampleRate == 0)
    {
        /* Init output vectors to zero data */
        vecrData.Init(0, (_REAL) 0.0);
        vecrScale.Init(0, (_REAL) 0.0);
        return;
    }

    /* Real input signal -> symmetrical spectrum -> use only half of spectrum */
    const _REAL rLenPowSpec = _REAL(iNumSmpls4AudioSprectrum) * iMaxAudioFrequency / iAudSampleRate;
    const int iLenPowSpec = int(rLenPowSpec);

    /* Init output vectors */
    vecrData.Init(iLenPowSpec, (_REAL) 0.0);
    vecrScale.Init(iLenPowSpec, (_REAL) 0.0);

    int i;

    /* Lock resources */
    Lock();

    /* Init vector storing the average spectrum with zeros */
    CVector<_REAL> veccAvSpectrum(iLenPowSpec, (_REAL) 0.0);

    int iCurPosInStream = 0;
    for (i = 0; i < iNumBlocksAvAudioSpec; i++)
    {
        int j;

        /* Mix both channels */
        for (j = 0; j < iNumSmpls4AudioSprectrum; j++)
        {
            int jj =  2*(iCurPosInStream + j);
            veccFFTInput[j] = _REAL(vecsOutputData[jj] + vecsOutputData[jj + 1]) / 2;
        }

        /* Apply window function */
        veccFFTInput *= vecrAudioWindowFunction;

        /* Calculate Fourier transformation to get the spectrum */
        veccFFTOutput = Fft(veccFFTInput, FftPlan);

        /* Average power (using power of this tap) */
        for (j = 0; j < iLenPowSpec; j++)
            veccAvSpectrum[j] += SqMag(veccFFTOutput[j]);

        iCurPosInStream += iNumSmpls4AudioSprectrum;
    }

    /* Calculate norm constant and scale factor */
    const _REAL rNormData = (_REAL) iNumSmpls4AudioSprectrum *
                            iNumSmpls4AudioSprectrum * _MAXSHORT * _MAXSHORT *
                            iNumBlocksAvAudioSpec;

    /* Define scale factor for audio data */
    const _REAL rFactorScale = _REAL(iMaxAudioFrequency) / iLenPowSpec / 1000;

    /* Apply the normalization (due to the FFT) */
    for (i = 0; i < iLenPowSpec; i++)
    {
        const _REAL rNormPowSpec = veccAvSpectrum[i] / rNormData;

        if (rNormPowSpec > 0)
            vecrData[i] = (_REAL) 10.0 * log10(rNormPowSpec);
        else
            vecrData[i] = RET_VAL_LOG_0;

        vecrScale[i] = (_REAL) i * rFactorScale;
    }

    /* Release resources */
    Unlock();
}


/* Simulation --------------------------------------------------------------- */
void CGenSimData::ProcessDataInternal(CParameter& TransmParam)
{
    int			i;
    uint32_t	iTempShiftRegister1;
    _BINARY		biPRBSbit;
    uint32_t	iShiftRegister;
    FILE*		pFileCurPos;
    time_t		tiElTi;
    long int	lReTi;
    _REAL		rReDays;

    /* Get elapsed time since this run was started (seconds) */
    tiElTi = time(NULL) - tiStartTime;

    /* Stop simulation if stop condition is true */
    iCounter++;
    switch (eCntType)
    {
    case CT_TIME:
        try
        {
            /* Estimate remaining time */
            lReTi = (long int) (((_REAL) iNumSimBlocks - iCounter) /
                                iCounter * tiElTi);

            /* Store current counter position in file */
            pFileCurPos = fopen(strFileName.c_str(), "w");
            if (pFileCurPos != NULL)
            {
                fprintf(pFileCurPos,
                        "%d / %d (%ld min elapsed, estimated time remaining: %ld min)",
                        iCounter, iNumSimBlocks,
                        (long int)tiElTi / 60, lReTi / 60);

                /* Write current paramter value */
                _REAL rCurParamVal;
                switch (TransmParam.eSimType)
                {
                case CParameter::ST_SYNC_PARAM:
                    rCurParamVal = TransmParam.rSyncTestParam;
                    break;

                case CParameter::ST_SINR:
                    rCurParamVal = TransmParam.rSINR;
                    break;

                default:
                    rCurParamVal = TransmParam.rBitErrRate;
                    break;
                }
                fprintf(pFileCurPos, "\n%e %e",
                        TransmParam.GetNominalSNRdB(), rCurParamVal);

                fclose(pFileCurPos);
            }
        }

        catch (...)
        {
            /* Catch all file errors to avoid stopping the simulation */
        }

        if (iCounter == iNumSimBlocks)
        {
		TransmParam.eRunState = CParameter::STOP_REQUESTED;
		iCounter = 0;
        }
        break;

    case CT_ERRORS:
        try
        {
            if (iCounter >= iMinNumBlocks)
            {
                /* Estimate remaining time */
                lReTi = (long int)
                        (((_REAL) iNumErrors - TransmParam.iNumBitErrors) /
                         TransmParam.iNumBitErrors * tiElTi);

                /* Estimated remaining days ( x / (60 * 60 * 24) ) */
                rReDays = (_REAL) lReTi / 86400;

                /* Store current counter position in file */
                pFileCurPos = fopen(strFileName.c_str(), "w");
                if (pFileCurPos != NULL)
                {
                    fprintf(pFileCurPos,
                            "%d / %d (%ld min elapsed, estimated time remaining: %ld min [%.1f days])",
                            TransmParam.iNumBitErrors, iNumErrors, (long int)tiElTi / 60,
                            lReTi / 60, rReDays);

                    /* Add current value of BER */
                    fprintf(pFileCurPos, "\n%e %e", TransmParam.
                            GetNominalSNRdB(), TransmParam.rBitErrRate);
                    fclose(pFileCurPos);
                }
            }
            else
            {
                /* Estimate remaining time */
                lReTi = (long int)
                        (((_REAL) iMinNumBlocks - iCounter) / iCounter * tiElTi);

                /* Store current counter position in file */
                pFileCurPos = fopen(strFileName.c_str(), "w");
                if (pFileCurPos != NULL)
                {
                    fprintf(pFileCurPos,
                            "%d / %d (%ld min elapsed, estimated minimum"
                            " time remaining: %ld min)\n",
                            iCounter, iMinNumBlocks, (long int)tiElTi / 60, lReTi / 60);

                    lReTi = (long int)
                            (((_REAL) iNumErrors - TransmParam.iNumBitErrors) /
                             TransmParam.iNumBitErrors * tiElTi);
                    fprintf(pFileCurPos,
                            "%d / %d (%ld min elapsed, estimated time remaining: %ld min)",
                            TransmParam.iNumBitErrors, iNumErrors, (long int)tiElTi / 60,
                            lReTi / 60);

                    /* Add current value of BER */
                    fprintf(pFileCurPos, "\n%e %e", TransmParam.
                            GetNominalSNRdB(), TransmParam.rBitErrRate);
                    fclose(pFileCurPos);
                }
            }
        }

        catch (...)
        {
            /* Catch all file errors to avoid stopping the simulation */
        }

        if (TransmParam.iNumBitErrors >= iNumErrors)
        {
            /* A minimum simulation time must be elapsed */
            if (iCounter >= iMinNumBlocks)
            {
		TransmParam.eRunState = CParameter::STOP_REQUESTED;
                iCounter = 0;
            }
        }
        break;
    }

    /* Generate a pseudo-noise test-signal (PRBS) */
    /* Init shift register with an arbitrary number (Must be known at the
       receiver AND transmitter!) */
    iShiftRegister = (uint32_t) (time(NULL) + rand());
    TransmParam.RawSimDa.Add(iShiftRegister);

    for (i = 0; i < iOutputBlockSize; i++)
    {
        /* Calculate new PRBS bit */
        iTempShiftRegister1 = iShiftRegister;

        /* P(X) = X^9 + X^5 + 1,
           in this implementation we have to shift n-1! */
        biPRBSbit = _BINARY(((iTempShiftRegister1 >> 4) & 1) ^
                            ((iTempShiftRegister1 >> 8) & 1));

        /* Shift bits in shift register and add new bit */
        iShiftRegister <<= 1;
        iShiftRegister |= (biPRBSbit & 1);

        /* Use PRBS output */
        (*pvecOutputData)[i] = biPRBSbit;
    }
}

void CGenSimData::InitInternal(CParameter& TransmParam)
{
    /* Define output block size */
    iOutputBlockSize = TransmParam.iNumDecodedBitsMSC;

    /* Minimum simulation time depends on the selected channel */
    switch (TransmParam.iDRMChannelNum)
    {
    case 1:
        /* AWGN: No fading */
        iMinNumBlocks = (int) ((_REAL) 2000.0 / (_REAL) 0.4);
        break;

    case 2:
        /* Rice with delay: 0.1 Hz */
        iMinNumBlocks = (int) ((_REAL) 5000.0 / (_REAL) 0.4);
        break;

    case 3:
        /* US Consortium: slowest 0.1 Hz */
        iMinNumBlocks = (int) ((_REAL) 15000.0 / (_REAL) 0.4);
        break;

    case 4:
        /* CCIR Poor: 1 Hz */
        iMinNumBlocks = (int) ((_REAL) 4000.0 / (_REAL) 0.4);
        break;

    case 5:
        /* Channel no 5: 2 Hz -> 30 sec */
        iMinNumBlocks = (int) ((_REAL) 3000.0 / (_REAL) 0.4);
        break;

    case 6:
        /* Channel no 6: same as case "2" */
        iMinNumBlocks = (int) ((_REAL) 2000.0 / (_REAL) 0.4);
        break;

    default:
        /* My own channels */
        iMinNumBlocks = (int) ((_REAL) 2000.0 / (_REAL) 0.4);
        break;
    }

    /* Prepare shift register used for storing the start values of the PRBS
       shift register */
    TransmParam.RawSimDa.Reset();

    /* Init start time */
    tiStartTime = time(NULL);
}

void CGenSimData::SetSimTime(int iNewTi, string strNewFileName)
{
    /* One MSC frame is 400 ms long */
    iNumSimBlocks = (int) ((_REAL) iNewTi /* sec */ / (_REAL) 0.4);

    /* Set simulation count type */
    eCntType = CT_TIME;

    /* Reset counter */
    iCounter = 0;

    /* Set file name */
    strFileName = string(SIM_OUT_FILES_PATH) +
                  strNewFileName + "__SIMTIME" + string(".dat");
}

void CGenSimData::SetNumErrors(int iNewNE, string strNewFileName)
{
    iNumErrors = iNewNE;

    /* Set simulation count type */
    eCntType = CT_ERRORS;

    /* Reset counter, because we also use it at the beginning of a run */
    iCounter = 0;

    /* Set file name */
    strFileName = string(SIM_OUT_FILES_PATH) +
                  strNewFileName + "__SIMTIME" + string(".dat");
}

void CEvaSimData::ProcessDataInternal(CParameter& Parameters)
{
    uint32_t	iTempShiftRegister1;
    _BINARY		biPRBSbit;
    uint32_t	iShiftRegister;
    int			iNumBitErrors;
    int			i;

    /* -------------------------------------------------------------------------
       Generate a pseudo-noise test-signal (PRBS) for comparison with
       received signal */
    /* Init shift register with an arbitrary number (Must be known at the
       receiver AND transmitter!) */
    iShiftRegister = Parameters.RawSimDa.Get();

    iNumBitErrors = 0;

    for (i = 0; i < iInputBlockSize; i++)
    {
        /* Calculate new PRBS bit */
        iTempShiftRegister1 = iShiftRegister;

        /* P(X) = X^9 + X^5 + 1,
           in this implementation we have to shift n-1! */
        biPRBSbit = _BINARY(((iTempShiftRegister1 >> 4) & 1) ^
                            ((iTempShiftRegister1 >> 8) & 1));

        /* Shift bits in shift register and add new bit */
        iShiftRegister <<= 1;
        iShiftRegister |= (biPRBSbit & 1);

        /* Count bit errors */
        if (biPRBSbit != (*pvecInputData)[i])
            iNumBitErrors++;
    }

    /* Save bit error rate, debar initialization blocks */
    if (iIniCnt > 0)
        iIniCnt--;
    else
    {
        rAccBitErrRate += (_REAL) iNumBitErrors / iInputBlockSize;
        iNumAccBitErrRate++;

        Parameters.rBitErrRate = rAccBitErrRate / iNumAccBitErrRate;
        Parameters.iNumBitErrors += iNumBitErrors;
    }
}

void CEvaSimData::InitInternal(CParameter& Parameters)
{
    /* Reset bit error rate parameters */
    rAccBitErrRate = (_REAL) 0.0;
    iNumAccBitErrRate = 0;

    /* Number of blocks at the beginning we do not want to use */
    iIniCnt = 10;

    /* Init global parameters */
    Parameters.rBitErrRate = (_REAL) 0.0;
    Parameters.iNumBitErrors = 0;

    /* Define block-size for input */
    iInputBlockSize = Parameters.iNumDecodedBitsMSC;
}


/******************************************************************************\
* FAC data																	   *
\******************************************************************************/
/* Transmitter */
void CGenerateFACData::ProcessDataInternal(CParameter& TransmParam)
{
    FACTransmit.FACParam(pvecOutputData, TransmParam);
}

void CGenerateFACData::InitInternal(CParameter& TransmParam)
{
    FACTransmit.Init(TransmParam);

    /* Define block-size for output */
    iOutputBlockSize = NUM_FAC_BITS_PER_BLOCK;
}

/* Receiver */
void CUtilizeFACData::ProcessDataInternal(CParameter& Parameters)
{
    /* Do not use received FAC data in case of simulation */
    if (bSyncInput == FALSE)
    {
        bCRCOk = FACReceive.FACParam(pvecInputData, Parameters);
        /* Set FAC status for RSCI, log file & GUI */
        if (bCRCOk)
            Parameters.ReceiveStatus.FAC.SetStatus(RX_OK);
        else
            Parameters.ReceiveStatus.FAC.SetStatus(CRC_ERROR);
    }

    if ((bSyncInput == TRUE) || (bCRCOk == FALSE))
    {
        /* If FAC CRC check failed we should increase the frame-counter
           manually. If only FAC data was corrupted, the others can still
           decode if they have the right frame number. In case of simulation
           no FAC data is used, we have to increase the counter here */
        Parameters.iFrameIDReceiv++;

        if (Parameters.iFrameIDReceiv == NUM_FRAMES_IN_SUPERFRAME)
            Parameters.iFrameIDReceiv = 0;
    }
}

void CUtilizeFACData::InitInternal(CParameter& Parameters)
{

// This should be in FAC class in an Init() routine which has to be defined, this
// would be cleaner code! TODO
    /* Init frame ID so that a "0" comes after increasing the init value once */
    Parameters.iFrameIDReceiv = NUM_FRAMES_IN_SUPERFRAME - 1;

    /* Reset flag */
    bCRCOk = FALSE;

    /* Define block-size for input */
    iInputBlockSize = NUM_FAC_BITS_PER_BLOCK;
}


/******************************************************************************\
* SDC data																	   *
\******************************************************************************/
/* Transmitter */
void CGenerateSDCData::ProcessDataInternal(CParameter& TransmParam)
{
    SDCTransmit.SDCParam(pvecOutputData, TransmParam);
}

void CGenerateSDCData::InitInternal(CParameter& TransmParam)
{
    /* Define block-size for output */
    iOutputBlockSize = TransmParam.iNumSDCBitsPerSFrame;
}

/* Receiver */
void CUtilizeSDCData::ProcessDataInternal(CParameter& Parameters)
{
//    _BOOLEAN bSDCOK = FALSE;

    /* Decode SDC block and return CRC status */
    CSDCReceive::ERetStatus eStatus = SDCReceive.SDCParam(pvecInputData, Parameters);

    Parameters.Lock();
    switch (eStatus)
    {
    case CSDCReceive::SR_OK:
        Parameters.ReceiveStatus.SDC.SetStatus(RX_OK);
//        bSDCOK = TRUE;
        break;

    case CSDCReceive::SR_BAD_CRC:
        /* SDC block depends on only a few parameters: robustness mode,
           DRM bandwidth and coding scheme (can be 4 or 16 QAM). If we
           initialize these parameters with resonable parameters it might
           be possible that these are the correct parameters. Therefore
           try to decode SDC even in case FAC wasn't decoded. That might
           speed up the DRM signal acqisition. But quite often it is the
           case that the parameters are not correct. In this case do not
           show a red light if SDC CRC was not ok */
        if (bFirstBlock == FALSE)
            Parameters.ReceiveStatus.SDC.SetStatus(CRC_ERROR);
        break;

    case CSDCReceive::SR_BAD_DATA:
        /* CRC was ok but data seems to be incorrect */
        Parameters.ReceiveStatus.SDC.SetStatus(DATA_ERROR);
        break;
    }
    Parameters.Unlock();

    /* Reset "first block" flag */
    bFirstBlock = FALSE;
}

void CUtilizeSDCData::InitInternal(CParameter& Parameters)
{
    /* Init "first block" flag */
    bFirstBlock = TRUE;

    /* Define block-size for input */
    iInputBlockSize = Parameters.iNumSDCBitsPerSFrame;
}


/* CWriteIQFile : module for writing an IQ or IF file */

CWriteIQFile::CWriteIQFile() : pFile(0), iFrequency(0), bIsRecording(FALSE), bChangeReceived(FALSE)
{
}

CWriteIQFile::~CWriteIQFile()
{
    if (pFile != 0)
        fclose(pFile);
}

void CWriteIQFile::StartRecording(CParameter&)
{
    bIsRecording = TRUE;
    bChangeReceived = TRUE;
}

void CWriteIQFile::OpenFile(CParameter& Parameters)
{
    iFrequency = Parameters.GetFrequency();

    /* Get current UTC time */
    time_t ltime;
    time(&ltime);
    struct tm* gmtCur = gmtime(&ltime);

    stringstream filename;
    filename << Parameters.GetDataDirectory();
    filename << Parameters.sReceiverID << "_";
    filename << setw(4) << setfill('0') << gmtCur->tm_year + 1900 << "-" << setw(2) << setfill('0')<< gmtCur->tm_mon + 1;
    filename << "-" << setw(2) << setfill('0')<< gmtCur->tm_mday << "_";
    filename << setw(2) << setfill('0') << gmtCur->tm_hour << "-" << setw(2) << setfill('0')<< gmtCur->tm_min;
    filename << "-" << setw(2) << setfill('0')<< gmtCur->tm_sec << "_";
    filename << setw(8) << setfill('0') << (iFrequency*1000) << ".iq" << (Parameters.GetSigSampleRate()/1000);

    pFile = fopen(filename.str().c_str(), "wb");

}

void CWriteIQFile::StopRecording()
{
    bIsRecording = FALSE;
    bChangeReceived = TRUE;
}

void CWriteIQFile::NewFrequency(CParameter &)
{
}

void CWriteIQFile::InitInternal(CParameter& Parameters)
{
    /* Get parameters from info class */
    const int iSymbolBlockSize = Parameters.CellMappingTable.iSymbolBlockSize;

    iInputBlockSize = iSymbolBlockSize;

    /* Init temporary vector for filter input and output */
    rvecInpTmp.Init(iSymbolBlockSize);
    cvecHilbert.Init(iSymbolBlockSize);

    /* Init mixer */
    Mixer.Init(iSymbolBlockSize);

    /* Inits for Hilbert and DC filter -------------------------------------- */
    /* Hilbert filter block length is the same as input block length */
    iHilFiltBlLen = iSymbolBlockSize;

    /* Init state vector for filtering with zeros */
    rvecZReal.Init(iHilFiltBlLen, (CReal) 0.0);
    rvecZImag.Init(iHilFiltBlLen, (CReal) 0.0);

    /* "+ 1" because of the Nyquist frequency (filter in frequency domain) */
    cvecBReal.Init(iHilFiltBlLen + 1);
    cvecBImag.Init(iHilFiltBlLen + 1);

    /* FFT plans are initialized with the long length */
    FftPlansHilFilt.Init(iHilFiltBlLen * 2);

    /* Set up the band-pass filter */

    /* Set internal parameter */
    const CReal rBPNormBW = CReal(0.4);

    /* Actual prototype filter design */
    CRealVector vecrFilter(iHilFiltBlLen);
    vecrFilter = FirLP(rBPNormBW, Nuttallwin(iHilFiltBlLen));

    /* Assume the IQ will be centred on a quarter of the soundcard sampling rate (e.g. 12kHz @ 48kHz) */
    const CReal rBPNormCentOffset = CReal(0.25);

    /* Set filter coefficients ---------------------------------------------- */
    /* Not really necessary since bandwidth will never be changed */
    const CReal rStartPhase = (CReal) iHilFiltBlLen * crPi * rBPNormCentOffset;

    /* Copy actual filter coefficients. It is important to initialize the
       vectors with zeros because we also do a zero-padding */
    CRealVector rvecBReal(2 * iHilFiltBlLen, (CReal) 0.0);
    CRealVector rvecBImag(2 * iHilFiltBlLen, (CReal) 0.0);
    for (int i = 0; i < iHilFiltBlLen; i++)
    {
        rvecBReal[i] = vecrFilter[i] *
                       Cos((CReal) 2.0 * crPi * rBPNormCentOffset * i - rStartPhase);

        rvecBImag[i] = vecrFilter[i] *
                       Sin((CReal) 2.0 * crPi * rBPNormCentOffset * i - rStartPhase);
    }

    /* Transformation in frequency domain for fft filter */
    cvecBReal = rfft(rvecBReal, FftPlansHilFilt);
    cvecBImag = rfft(rvecBImag, FftPlansHilFilt);

}

void CWriteIQFile::ProcessDataInternal(CParameter& Parameters)
{
    int i;

    if (bChangeReceived) // file is open but we want to start a new one
    {
        bChangeReceived = FALSE;
        if (pFile != NULL)
        {
            fclose(pFile);
        }
        pFile = 0;
    }

    // is recording switched on?
    if (!bIsRecording)
    {
        if (pFile != NULL)
        {
            fclose(pFile); // close file if currently open
            pFile = NULL;
        }
        return;
    }


    // Has the frequency changed? If so, close any open file (a new one will be opened)
    int iNewFrequency = Parameters.GetFrequency();

    if (iNewFrequency != iFrequency)
    {
        iFrequency = iNewFrequency;
        // If file is currently open, close it
        if (pFile != NULL)
        {
            fclose(pFile);
            pFile = NULL;
        }
    }
    // Now open the file with correct name if it isn't currently open
    if (!pFile)
    {
        OpenFile(Parameters);
    }

    /* Band-pass filter and mixer ------------------------------------------- */
    /* Copy CVector data in CMatlibVector */
    for (i = 0; i < iInputBlockSize; i++)
        rvecInpTmp[i] = (*pvecInputData)[i];

    /* Cut out a spectrum part of desired bandwidth */
    cvecHilbert = CComplexVector(
                      FftFilt(cvecBReal, rvecInpTmp, rvecZReal, FftPlansHilFilt),
                      FftFilt(cvecBImag, rvecInpTmp, rvecZImag, FftPlansHilFilt));

    /* Mix it down to zero frequency */
    Mixer.SetMixFreq(CReal(0.25));
    Mixer.Process(cvecHilbert);

    /* Write to the file */

    _SAMPLE re, im;
    _BYTE bytes[4];

    CReal rScale = CReal(1.0);
    for (i=0; i<iInputBlockSize; i++)
    {
        re = _SAMPLE(cvecHilbert[i].real() * rScale);
        im = _SAMPLE(cvecHilbert[i].imag() * rScale);
        bytes[0] = _BYTE(re & 0xFF);
        bytes[1] = _BYTE((re>>8) & 0xFF);
        bytes[2] = _BYTE(im & 0xFF);
        bytes[3] = _BYTE((im>>8) & 0xFF);

        fwrite(bytes, 4, sizeof(_BYTE), pFile);
    }
}

