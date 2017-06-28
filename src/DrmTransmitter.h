/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See DrmTransmitter.cpp
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

#if !defined(DRMTRANSM_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define DRMTRANSM_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include <iostream>
#include "util/Buffer.h"
#include "Parameter.h"
#include "DataIO.h"
#include "mlc/MLC.h"
#include "interleaver/SymbolInterleaver.h"
#include "ofdmcellmapping/OFDMCellMapping.h"
#include "OFDM.h"
#include "DRMSignalIO.h"
#include "sourcedecoders/AudioSourceEncoder.h"
#include "sound/soundinterface.h"
#include "DrmTransceiver.h"

/* Classes ********************************************************************/
class CDRMTransmitter : public CDRMTransceiver
{
public:
    CDRMTransmitter(CSettings* pSettings=NULL);
    virtual ~CDRMTransmitter();

    void LoadSettings();
    void SaveSettings();
    void Init();
    void Start();

    CAudioSourceEncoder*	GetAudSrcEnc() {
        return &AudioSourceEncoder;
    }
    CTransmitData*			GetTransData() {
        return &TransmitData;
    }
    CReadData*				GetReadData() {
        return &ReadData;
    }

    void SetCarOffset(const _REAL rNewCarOffset)
    {
        /* Has to be set in OFDM modulation and transmitter filter module */
        OFDMModulation.SetCarOffset(rNewCarOffset);
        TransmitData.SetCarOffset(rNewCarOffset);
        rDefCarOffset = rNewCarOffset;
    }
    _REAL GetCarOffset() {
        return rDefCarOffset;
    }
#ifdef QT_MULTIMEDIA_LIB
    void SetInputDevice(const QAudioDeviceInfo&) {}
    void SetOutputDevice(const QAudioDeviceInfo&) {}
#endif

protected:
    void Run();
    void InitSoftStop() { iSoftStopSymbolCount=0; };
    _BOOLEAN CanSoftStopExit();

    /* Buffers */
    CSingleBuffer<_SAMPLE>	DataBuf;
    CSingleBuffer<_BINARY>	AudSrcBuf;

    CSingleBuffer<_COMPLEX>	MLCEncBuf;
    CCyclicBuffer<_COMPLEX>	IntlBuf;

    CSingleBuffer<_BINARY>	GenFACDataBuf;
    CCyclicBuffer<_COMPLEX>	FACMapBuf;

    CSingleBuffer<_BINARY>	GenSDCDataBuf;
    CCyclicBuffer<_COMPLEX>	SDCMapBuf;

    CSingleBuffer<_COMPLEX>	CarMapBuf;
    CSingleBuffer<_COMPLEX>	OFDMModBuf;

    /* Modules */
    CReadData				ReadData;
    CAudioSourceEncoder		AudioSourceEncoder;
    CMSCMLCEncoder			MSCMLCEncoder;
    CSymbInterleaver		SymbInterleaver;
    CGenerateFACData		GenerateFACData;
    CFACMLCEncoder			FACMLCEncoder;
    CGenerateSDCData		GenerateSDCData;
    CSDCMLCEncoder			SDCMLCEncoder;
    COFDMCellMapping		OFDMCellMapping;
    COFDMModulation			OFDMModulation;
    CTransmitData			TransmitData;

    _REAL					rDefCarOffset;
    _BOOLEAN				bUseUEP;
    int						iSoftStopSymbolCount;
};


#endif // !defined(DRMTRANSM_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
