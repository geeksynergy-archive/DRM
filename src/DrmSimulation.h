/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See DrmSimulation.cpp
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

#if !defined(DRMSIMULATION_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define DRMSIMULATION_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include <iostream>
#include "GlobalDefinitions.h"
#include "Parameter.h"
#include "util/Buffer.h"
#include "DataIO.h"
#include "OFDM.h"
#include "DRMSignalIO.h"
#include "MSCMultiplexer.h"
#include "InputResample.h"
#include "mlc/MLC.h"
#include "interleaver/SymbolInterleaver.h"
#include "ofdmcellmapping/OFDMCellMapping.h"
#include "chanest/ChannelEstimation.h"
#include "chanest/IdealChannelEstimation.h"
#include "sync/FreqSyncAcq.h"
#include "sync/TimeSync.h"
#include "sync/SyncUsingPil.h"
#include "drmchannel/ChannelSimulation.h"



/* Classes ********************************************************************/
class CDRMSimulation
{
public:
    CDRMSimulation();
    virtual ~CDRMSimulation() {}

    void SimScript();

    CParameter* GetParameters() {
        return &Parameters;
    }

protected:
    void Run();
    void Init();
    string SimFileName(CParameter& Param, string strAddInf, _BOOLEAN bWithSNR);

    int iSimTime;
    int iSimNumErrors;
    _REAL rStartSNR, rEndSNR, rStepSNR;

    /* Parameters */
    CParameter				Parameters;


    /* Buffers -------------------------------------------------------------- */
    /* If you want to add a new buffer, make sure that it is cleared in the
       "Init()" routine! */
    /* Transmitter buffers */
    CSingleBuffer<_BINARY>				DataBuf;
    CSingleBuffer<_COMPLEX>				MLCEncBuf;
    CCyclicBuffer<_COMPLEX>				IntlBuf;
    CSingleBuffer<_BINARY>				GenFACDataBuf;
    CCyclicBuffer<_COMPLEX>				FACMapBuf;
    CSingleBuffer<_BINARY>				GenSDCDataBuf;
    CCyclicBuffer<_COMPLEX>				SDCMapBuf;
    CSingleBuffer<_COMPLEX>				CarMapBuf;
    CSingleBuffer<_COMPLEX>				OFDMModBuf;

    /* Simulation */
    CCyclicBuffer<CChanSimDataDemod>	OFDMDemodBufChan2;
    CSingleBuffer<_COMPLEX>				ChanEstInBufSim;
    CSingleBuffer<CChanSimDataDemod>	ChanEstOutBufChan;
    CSingleBuffer<CChanSimDataMod>		RecDataBuf;
    CSingleBuffer<_REAL>				ChanResInBuf;

    /* Receiver buffers */
    CCyclicBuffer<_REAL>				InpResBuf;
    CSingleBuffer<_COMPLEX>				FreqSyncAcqBuf;
    CSingleBuffer<_COMPLEX>				TimeSyncBuf;
    CSingleBuffer<_COMPLEX>				OFDMDemodBuf;
    CSingleBuffer<_COMPLEX>				SyncUsingPilBuf;
    CSingleBuffer<CEquSig>				ChanEstBuf;
    CCyclicBuffer<CEquSig>				MSCCarDemapBuf;
    CCyclicBuffer<CEquSig>				FACCarDemapBuf;
    CCyclicBuffer<CEquSig>				SDCCarDemapBuf;
    CSingleBuffer<CEquSig>				DeintlBuf;
    CSingleBuffer<_BINARY>				FACDecBuf;
    CSingleBuffer<_BINARY>				SDCDecBuf;
    CSingleBuffer<_BINARY>				MSCMLCDecBuf;


    /* Modules -------------------------------------------------------------- */
    /* Transmitter modules */
    CGenSimData				GenSimData;

    CMSCMLCEncoder			MSCMLCEncoder;
    CSymbInterleaver		SymbInterleaver;
    CGenerateFACData		GenerateFACData;
    CFACMLCEncoder			FACMLCEncoder;
    CGenerateSDCData		GenerateSDCData;
    CSDCMLCEncoder			SDCMLCEncoder;
    COFDMCellMapping		OFDMCellMapping;
    COFDMModulation			OFDMModulation;

    /* DRM channel */
    CDRMChannel				DRMChannel;

    /* Receiver modules */
    CInputResample			InputResample;
    CFreqSyncAcq			FreqSyncAcq;
    CTimeSync				TimeSync;
    COFDMDemodulation		OFDMDemodulation;
    CSyncUsingPil			SyncUsingPil;
    CChannelEstimation		ChannelEstimation;
    COFDMCellDemapping		OFDMCellDemapping;
    CFACMLCDecoder			FACMLCDecoder;
    CUtilizeFACData			UtilizeFACData;
    CSDCMLCDecoder			SDCMLCDecoder;
    CUtilizeSDCData			UtilizeSDCData;
    CSymbDeinterleaver		SymbDeinterleaver;
    CMSCMLCDecoder			MSCMLCDecoder;

    /* Simulation modules */
    CEvaSimData				EvaSimData;
    COFDMDemodSimulation	OFDMDemodSimulation;
    CIdealChanEst			IdealChanEst;

    CDataConvChanResam		DataConvChanResam;

};


#endif // !defined(DRMSIMULATION_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
