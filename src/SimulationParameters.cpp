/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	DRM simulation script
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

#include "DrmSimulation.h"

#ifdef _WIN32
#include <windows.h>
#else
# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# endif
#endif

/* Implementation *************************************************************/
void CDRMSimulation::SimScript()
{
    int									i;
    int									iNumItMLC;
    _REAL								rSNRCnt;
    FILE*								pFileSimRes;
    CVector<_REAL>						vecrMSE;
    string								strSimFile;
    string								strSpecialRemark;
    CChannelEstimation::ETypeIntFreq	eChanEstFreqIntType;
    CChannelEstimation::ETypeIntTime	eChanEstTimIntType;


    /**************************************************************************\
    * Simulation settings vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*
    \**************************************************************************/
    /* Choose which type of simulation, if you choose "ST_NONE", the regular
       application will be started
       For ease of use, all possible types are plotted below. To enable a
       certain type, just cut and past it at the end of the list
       There are  different types of simulations available:
       ST_BITERROR: BER simulation using actual channel estimation algorithms
    		(either Wiener, linear or DFT method, can be set by
    		eChanEstFreqIntType and eChanEstTimIntType)
       ST_SYNC_PARAM: testing synchronization units (requires modifications in
    		the actual synchronization code to make it work)
       ST_MSECHANEST: MSE versus carrier for channel estimation algorithms
       ST_BER_IDEALCHAN: BER assuming ideal channel estimation

       Make sure the /drm/[linux, windows]/test directory exists since all the
       simulation results are stored there. Together with the actual simulation
       results, a file called x__SIMTIME.dat is also created and updated very
       frequently, showing the remaining time.
       By using the skript "plotsimres.m" in Matlab, a nice plot of the results
       is generated automatically (this script reads all available simulation
       results) */
    Parameters.eSimType = CParameter::ST_BITERROR;
    Parameters.eSimType = CParameter::ST_SYNC_PARAM; /* Check "SetSyncInput()"s! */
    Parameters.eSimType = CParameter::ST_BER_IDEALCHAN;
    Parameters.eSimType = CParameter::ST_MSECHANEST;
    Parameters.eSimType = CParameter::ST_NONE; /* No simulation, regular GUI */


    if (Parameters.eSimType != CParameter::ST_NONE)
    {
        /* Choose channel model. The channel numbers are according to the
           numbers in the DRM-standard (e.g., channel number 1 is AWGN) plus
           some additional channel profiles defined by the author */
        Parameters.iDRMChannelNum = 1;

        /* This parameter is only needed for the non-standard special channels 8, 10, 11, 12 */
        Parameters.iSpecChDoppler = 2; /* Hz (integer value!) */

        /* Defines the SNR range for the simulation. A start, stop and step
           value can be set here */
        rStartSNR = (_REAL) 14;
        rEndSNR = (_REAL) 15.2;
        rStepSNR = (_REAL) 0.3;

        /* A string which is appended to the result files */
        strSpecialRemark = "AWGNtest";

        /* Define length of simulation. Only one of the following methods can
           be used: time or number of bit errors:
           iSimTime: simulation time in seconds of the virtual DRM stream (which
           is not the simulation time)
           iSimNumErrors: number of bit errors */
        iSimTime = 100;
//		iSimNumErrors = 100000;


        /* Number of iterations for MLC */
        iNumItMLC = 1;

        /* The associated code rate is R = 0,6 and the modulation is 64-QAM */
        Parameters.MSCPrLe.iPartB = 1;
        Parameters.eMSCCodingScheme = CS_3_SM;


        /* Choose the type of channel estimation algorithms used for simlation */
        eChanEstFreqIntType = CChannelEstimation::FWIENER;//FDFTFILTER;//FLINEAR;
        eChanEstTimIntType = CChannelEstimation::TWIENER;//TLINEAR;


        /* Define which synchronization algorithms we want to use */
        /* In case of bit error simulations, a synchronized DRM data stream is
           used. Set corresponding modules to synchronized mode */
        InputResample.SetSyncInput(TRUE);
        FreqSyncAcq.SetSyncInput(TRUE);
        SyncUsingPil.SetSyncInput(TRUE);
        TimeSync.SetSyncInput(TRUE);


        if (Parameters.iDRMChannelNum < 3)
        {
            Parameters.InitCellMapTable(RM_ROBUSTNESS_MODE_A, SO_2);
            Parameters.eSymbolInterlMode = CParameter::SI_SHORT;
        }
        else if ((Parameters.iDRMChannelNum == 8) || (Parameters.iDRMChannelNum == 10))
        {
            /* Special setting for channel 8 */
            Parameters.InitCellMapTable(RM_ROBUSTNESS_MODE_B, SO_0);
            Parameters.eSymbolInterlMode = CParameter::SI_LONG;
        }
        else
        {
            Parameters.InitCellMapTable(RM_ROBUSTNESS_MODE_B, SO_3);
            Parameters.eSymbolInterlMode = CParameter::SI_LONG;
        }


        /**************************************************************************\
        * Simulation settings ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*
        \**************************************************************************/


        /* Apply settings --------------------------------------------------- */
        /* Set channel estimation interpolation type */
        ChannelEstimation.SetFreqInt(eChanEstFreqIntType);
        ChannelEstimation.SetTimeInt(eChanEstTimIntType);

        /* Number of iterations for MLC */
        MSCMLCDecoder.SetNumIterations(iNumItMLC);


        /* Set the simulation priority to lowest possible value */
#ifdef _WIN32
        SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
#else
# ifdef HAVE_UNISTD_H
        /* re-nice the process to highest possible value -> 19 */
        int n = nice(19); (void)n;

        /* Try to get hostname */
        char chHostName[255];
        if (gethostname(chHostName, size_t(255)) == 0)
        {
            /* Append it to the file name of simulation output */
            strSpecialRemark += "_";
            strSpecialRemark += chHostName;
        }
# endif
#endif

        /* Set simulation time or number of errors. Slighty different convention
           for __SIMTIME file name */
        if (iSimTime != 0)
        {
            GenSimData.SetSimTime(iSimTime,
                                  SimFileName(Parameters, strSpecialRemark, TRUE));
        }
        else
        {
            GenSimData.SetNumErrors(iSimNumErrors,
                                    SimFileName(Parameters, strSpecialRemark, TRUE));
        }

        /* Set file name for simulation results output (in case of MSE, plot
           SNR range, too) */
        strSimFile = string(SIM_OUT_FILES_PATH) +
                     SimFileName(Parameters, strSpecialRemark,
                                 Parameters.eSimType == CParameter::ST_MSECHANEST) + string(".dat");

        /* Open file for storing simulation results */
        pFileSimRes = fopen(strSimFile.c_str(), "w");

        /* If opening of file was not successful, exit simulation */
        if (pFileSimRes == NULL)
            exit(1);

        /* Show name as console output */
        fprintf(stderr, "%s\n", strSimFile.c_str());

        /* Main SNR loop */
        for (rSNRCnt = rStartSNR; rSNRCnt <= rEndSNR; rSNRCnt += rStepSNR)
        {
            /* Set SNR in global struct and run simulation */
            Parameters.SetNominalSNRdB(rSNRCnt);
            Run();

            /* Store results in file */
            switch (Parameters.eSimType)
            {
            case CParameter::ST_MSECHANEST:
                /* After the simulation get results */
                IdealChanEst.GetResults(vecrMSE);

                /* Store results in a file */
                for (i = 0; i < vecrMSE.Size(); i++)
                    fprintf(pFileSimRes, "%e ", vecrMSE[i]);
                fprintf(pFileSimRes, "\n"); /* New line */
                break;

            case CParameter::ST_SYNC_PARAM:
                /* Save results in the file */
                fprintf(pFileSimRes, "%e %e\n", rSNRCnt, Parameters.rSyncTestParam);
                break;

            default:
                /* Save results in the file */
                fprintf(pFileSimRes, "%e %e\n", rSNRCnt, Parameters.rBitErrRate);
                break;
            }

            /* Make sure results are actually written in the file. In case the
               simulation machine crashes, at least the last results are
               preserved */
            fflush(pFileSimRes);
        }

        /* Close simulation results file afterwards */
        fclose(pFileSimRes);

        /* At the end of the simulation, exit the application */
        exit(1);
    }
}

string CDRMSimulation::SimFileName(CParameter& SaveParam, string strAddInf,
                                   _BOOLEAN bWithSNR)
{
    /*
    	File naming convention:
    	BER: Bit error rate simulation
    	MSE: MSE for channel estimation
    	BERIDEAL: Bit error rate simulation with ideal channel estimation
    	SYNC: Simulation for a synchronization paramter (usually variance)

    	B3: Robustness mode and spectrum occupancy
    	CH5: Channel number
    	It: Number of iterations in MLC decoder
    	PL: Protection level of channel code
    	"64SM": Modulation
    	"T50": Length of simulation

    	example: BERIDEAL_CH8_B0_It1_PL1_64SM_T50_MyRemark
    */
    char chNumTmpLong[256];
    string strFileName = "";

    /* What type of simulation ---------------------------------------------- */
    switch (SaveParam.eSimType)
    {
    case CParameter::ST_BITERROR:
        strFileName += "BER_";
        break;
    case CParameter::ST_MSECHANEST:
        strFileName += "MSE_";
        break;
    case CParameter::ST_BER_IDEALCHAN:
        strFileName += "BERIDEAL_";
        break;
    case CParameter::ST_SYNC_PARAM:
        strFileName += "SYNC_";
        break;
    case CParameter::ST_NONE:
    case CParameter::ST_SINR:
        break;
    }


    /* Channel -------------------------------------------------------------- */
    /* In case of channels 8 / 10 also write Doppler frequency in file name */
    if ((SaveParam.iDRMChannelNum == 8) || (SaveParam.iDRMChannelNum == 10))
    {
        sprintf(chNumTmpLong, "CH%d_%dHz_",
                SaveParam.iDRMChannelNum, SaveParam.iSpecChDoppler);
    }
    else
        sprintf(chNumTmpLong, "CH%d_", SaveParam.iDRMChannelNum);

    strFileName += chNumTmpLong;


    /* Robustness mode and spectrum occupancy ------------------------------- */
    switch (SaveParam.GetWaveMode())
    {
    case RM_ROBUSTNESS_MODE_A:
        strFileName += "A";
        break;
    case RM_ROBUSTNESS_MODE_B:
        strFileName += "B";
        break;
    case RM_ROBUSTNESS_MODE_C:
        strFileName += "C";
        break;
    case RM_ROBUSTNESS_MODE_D:
        strFileName += "D";
        break;
    case RM_NO_MODE_DETECTED:
        break;
    }
    switch (SaveParam.GetSpectrumOccup())
    {
    case SO_0:
        strFileName += "0_";
        break;
    case SO_1:
        strFileName += "1_";
        break;
    case SO_2:
        strFileName += "2_";
        break;
    case SO_3:
        strFileName += "3_";
        break;
    case SO_4:
        strFileName += "4_";
        break;
    case SO_5:
        strFileName += "5_";
        break;
    }


    /* Channel estimation method -------------------------------------------- */
    /* In case of BER simulation, print out channel estimation methods used */
    if (SaveParam.eSimType == CParameter::ST_BITERROR)
    {
        /* Time direction */
        switch (ChannelEstimation.GetTimeInt())
        {
        case CChannelEstimation::TLINEAR:
            strFileName += "Tl";
            break;

        case CChannelEstimation::TWIENER:
            strFileName += "Tw";
            break;
        }

        /* Frequency direction */
        switch (ChannelEstimation.GetFreqInt())
        {
        case CChannelEstimation::FLINEAR:
            strFileName += "Fl_";
            break;

        case CChannelEstimation::FDFTFILTER:
            strFileName += "Fd_";
            break;

        case CChannelEstimation::FWIENER:
            strFileName += "Fw_";
            break;
        }
    }


    /* Number of iterations in MLC decoder ---------------------------------- */
    sprintf(chNumTmpLong, "It%d_", MSCMLCDecoder.GetInitNumIterations());
    strFileName += chNumTmpLong;


    /* Protection level part B ---------------------------------------------- */
    sprintf(chNumTmpLong, "PL%d_", SaveParam.MSCPrLe.iPartB);
    strFileName += chNumTmpLong;


    /* MSC coding scheme ---------------------------------------------------- */
    switch (SaveParam.eMSCCodingScheme)
    {
    case CS_1_SM:
        break;

    case CS_2_SM:
        strFileName += "16SM_";
        break;

    case CS_3_SM:
        strFileName += "64SM_";
        break;

    case CS_3_HMMIX:
        strFileName += "64MIX_";
        break;

    case CS_3_HMSYM:
        strFileName += "64SYM_";
        break;
    }


    /* Number of error events or simulation time ---------------------------- */
    int iCurNum;
    string strMultPl = "_";
    if (iSimTime != 0)
    {
        strFileName += "T"; /* T -> time */
        iCurNum = iSimTime;
    }
    else
    {
        strFileName += "E"; /* E -> errors */
        iCurNum = iSimNumErrors;
    }

    if (iCurNum / 1000 > 0)
    {
        strMultPl = "K_";
        iCurNum /= 1000;

        if (iCurNum / 1000 > 0)
        {
            strMultPl = "M_";
            iCurNum /= 1000;
        }
    }

    sprintf(chNumTmpLong, "%d", iCurNum);
    strFileName += chNumTmpLong;
    strFileName += strMultPl;


    /* SNR range (optional) ------------------------------------------------- */
    if (bWithSNR == TRUE)
    {
        strFileName += "SNR";
        sprintf(chNumTmpLong, "%.1f-", rStartSNR);
        strFileName += chNumTmpLong;
        sprintf(chNumTmpLong, "%.1f-", rStepSNR);
        strFileName += chNumTmpLong;
        sprintf(chNumTmpLong, "%.1f_", rEndSNR);
        strFileName += chNumTmpLong;
    }

    /* Special remark */
    if (!strAddInf.empty())
        strFileName += strAddInf;

    return strFileName;
}
