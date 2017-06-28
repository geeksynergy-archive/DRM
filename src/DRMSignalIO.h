/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See DRMSignalIO.cpp
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

#if !defined(DRMSIGNALIO_H__3B0BA660_CA63_4344_B_23E7A0D31912__INCLUDED_)
#define DRMSIGNALIO_H__3B0BA660_CA63_4344_B_23E7A0D31912__INCLUDED_

#include "sound/soundinterface.h"
#ifdef QT_MULTIMEDIA_LIB
#include <QAudioInput>
#include <QIODevice>
#endif
#include "Parameter.h"
#include "matlib/Matlib.h"
#include "IQInputFilter.h"
#include "util/Modul.h"
#include "util/Utilities.h"
#include <math.h>
#include <vector>

/* Definitions ****************************************************************/
/* Number of FFT blocks used for averaging. See next definition
   ("NUM_SMPLS_4_INPUT_SPECTRUM") for how to set the parameters */
#define NUM_AV_BLOCKS_PSD			16
#define LEN_PSD_AV_EACH_BLOCK		512

/* same but for the rpsd tag */
#define NUM_AV_BLOCKS_PSD_RSI	150
#define LEN_PSD_AV_EACH_BLOCK_RSI		256
#define PSD_OVERLAP_RSI	128

/* power gain of the Hamming window */
#define PSD_WINDOW_GAIN 0.39638

/* Length of vector for input spectrum. We use approx. 0.2 sec
   of sampled data for spectrum calculation, this is 2^13 = 8192 to
   make the FFT work more efficient. Make sure that this number is not smaller
   than the symbol lenght in 48 khz domain of longest mode (which is mode A/B:
   1280) */
#define NUM_SMPLS_4_INPUT_SPECTRUM (NUM_AV_BLOCKS_PSD * LEN_PSD_AV_EACH_BLOCK)

/* The RSI output needs 400ms with a 50% overlap, so this needs more space
   I think the RSCI spec is slightly wrong - using 150 windows consumes just over 400ms, 149 would be exact */
#define INPUT_DATA_VECTOR_SIZE (NUM_AV_BLOCKS_PSD_RSI * (LEN_PSD_AV_EACH_BLOCK_RSI-PSD_OVERLAP_RSI)+PSD_OVERLAP_RSI)

#define RNIP_SEARCH_RANGE_NARROW 5100.0
#define RNIP_SEARCH_RANGE_WIDE 15100.0
#define RNIP_EXCLUDE_BINS 2 // either side of the peak

/* Use raw 16 bit data or in text form for file format for DRM data. Defining
   the following macro will enable the raw data option */
#define FILE_DRM_USING_RAW_DATA


/* Classes ********************************************************************/
class CTransmitData : public CTransmitterModul<_COMPLEX, _COMPLEX>
{
public:
    enum EOutFormat {OF_REAL_VAL /* real valued */, OF_IQ_POS,
                     OF_IQ_NEG /* I / Q */, OF_EP /* envelope / phase */
                    };

    CTransmitData(CSoundOutInterface* pNS) : pFileTransmitter(NULL), pSound(pNS),
            eOutputFormat(OF_REAL_VAL), rDefCarOffset((_REAL) VIRTUAL_INTERMED_FREQ),
            strOutFileName("test/TransmittedData.txt"), bUseSoundcard(TRUE),
            bAmplified(FALSE), bHighQualityIQ(FALSE) {}
    virtual ~CTransmitData();

    void SetIQOutput(const EOutFormat eFormat) {
        eOutputFormat = eFormat;
    }
    EOutFormat GetIQOutput() {
        return eOutputFormat;
    }

    void SetAmplifiedOutput(_BOOLEAN bEnable) {
        bAmplified = bEnable;
    }
    _BOOLEAN GetAmplifiedOutput() {
        return bAmplified;
    }

    void SetHighQualityIQ(_BOOLEAN bEnable) {
        bHighQualityIQ = bEnable;
    }
    _BOOLEAN GetHighQualityIQ() {
        return bHighQualityIQ;
    }

    void SetCarOffset(const CReal rNewCarOffset)
    {
        rDefCarOffset = rNewCarOffset;
    }

    void SetWriteToFile(const string strNFN)
    {
        strOutFileName = strNFN;
        bUseSoundcard = FALSE;
    }

    void FlushData();

protected:
    FILE*				pFileTransmitter;
    CSoundOutInterface*	pSound;
    CVector<short>		vecsDataOut;
    int					iBlockCnt;
    int					iNumBlocks;
    EOutFormat			eOutputFormat;

    CDRMBandpassFilt	BPFilter;
    CReal				rDefCarOffset;

    CReal				rNormFactor;

    int					iBigBlockSize;

    string				strOutFileName;
    _BOOLEAN			bUseSoundcard;

    _BOOLEAN			bAmplified;
    _BOOLEAN			bHighQualityIQ;
    CVector<_REAL>		vecrReHist;

    void HilbertFilt(_COMPLEX& vecData);

    virtual void InitInternal(CParameter& TransmParam);
    virtual void ProcessDataInternal(CParameter& Parameter);
};

class CReceiveData : public CReceiverModul<_REAL, _REAL>
{
public:
    enum EInChanSel {CS_LEFT_CHAN, CS_RIGHT_CHAN, CS_MIX_CHAN, CS_SUB_CHAN, CS_IQ_POS,
                     CS_IQ_NEG, CS_IQ_POS_ZERO, CS_IQ_NEG_ZERO, CS_IQ_POS_SPLIT, CS_IQ_NEG_SPLIT
                    };

    CReceiveData() :
#ifdef QT_MULTIMEDIA_LIB
        pIODevice(NULL),
#endif
        pSound(NULL),
        vecrInpData(INPUT_DATA_VECTOR_SIZE, (_REAL) 0.0),
            bFippedSpectrum(FALSE), eInChanSelection(CS_MIX_CHAN), iPhase(0)
    {}
    virtual ~CReceiveData();

    _REAL ConvertFrequency(_REAL rFrequency, _BOOLEAN bInvert=FALSE) const;

    void GetInputSpec(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
    void GetInputPSD(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale,
                     const int iLenPSDAvEachBlock = LEN_PSD_AV_EACH_BLOCK,
                     const int iNumAvBlocksPSD = NUM_AV_BLOCKS_PSD,
                     const int iPSDOverlap = 0);

    void SetFlippedSpectrum(const _BOOLEAN bNewF) {
        bFippedSpectrum = bNewF;
    }
    _BOOLEAN GetFlippedSpectrum() {
        return bFippedSpectrum;
    }

    void ClearInputData() {
        mutexInpData.Lock();
        vecrInpData.Init(INPUT_DATA_VECTOR_SIZE, (_REAL) 0.0);
        mutexInpData.Unlock();
    }

    void SetSoundInterface(CSoundInInterface* pS) {
        pSound = pS;
    }
#ifdef QT_MULTIMEDIA_LIB
    void SetSoundInterface(QAudioInput *);
#endif
    void SetInChanSel(const EInChanSel eNS) {
        eInChanSelection = eNS;
    }
    EInChanSel GetInChanSel() {
        return eInChanSelection;
    }

protected:
    CSignalLevelMeter		SignalLevelMeter;

#ifdef QT_MULTIMEDIA_LIB
    QAudioInput*            pAudioInput;
    QIODevice*              pIODevice;
#endif
    CSoundInInterface*		pSound;
    CVector<_SAMPLE>		vecsSoundBuffer;

    /* Access to vecrInpData buffer must be done 
       inside mutexInpData mutex */
    CShiftRegister<_REAL>	vecrInpData;
    CMutex                  mutexInpData;

    int					iSampleRate;
    _BOOLEAN			bFippedSpectrum;

    int					iUpscaleRatio;
    vector<float>		vecf_B, vecf_YL, vecf_YR, vecf_ZL, vecf_ZR;

    EInChanSel			eInChanSelection;

    CVector<_REAL>		vecrReHist;
    CVector<_REAL>		vecrImHist;
    _COMPLEX			cCurExp;
    _COMPLEX			cExpStep;
    int					iPhase;

    _REAL HilbertFilt(const _REAL rRe, const _REAL rIm);

    /* OPH: counter to count symbols within a frame in order to generate */
    /* RSCI output */
    int							iFreeSymbolCounter;

    virtual void InitInternal(CParameter& Parameters);
    virtual void ProcessDataInternal(CParameter& Parameters);

    void PutPSD(CParameter& Parameters);
    void CalculatePSD(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale,
                      const int iLenPSDAvEachBlock = LEN_PSD_AV_EACH_BLOCK,
                      const int iNumAvBlocksPSD = NUM_AV_BLOCKS_PSD,
                      const int iPSDOverlap = 0);

    void CalculateSigStrengthCorrection(CParameter &Parameters, CVector<_REAL> &vecrPSD);
    void CalculatePSDInterferenceTag(CParameter &Parameters, CVector<_REAL> &vecrPSD);

    int FreqToBin(_REAL rFreq);
    _REAL CalcTotalPower(CVector<_REAL> &vecrData, int iStartBin, int iEndBin);

    void InterpFIR_2X(const int channels, _SAMPLE* X, vector<float>& Z, vector<float>& Y, vector<float>& B);
};


#endif // !defined(DRMSIGNALIO_H__3B0BA660_CA63_4344_B_23E7A0D31912__INCLUDED_)
