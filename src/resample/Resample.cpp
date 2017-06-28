/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2002, 2012, 2013
 *
 * Author(s):
 *	Volker Fischer, David Flamand (added CSpectrumResample, speex resampler)
 *
 * Description:
 * Resample routine for arbitrary sample-rate conversions in a low range (for
 * frequency offset correction).
 * The algorithm is based on a polyphase structure. We upsample the input
 * signal with a factor INTERP_DECIM_I_D and calculate two successive samples
 * whereby we perform a linear interpolation between these two samples to get
 * an arbitraty sample grid.
 * The polyphase filter is calculated with Matlab(TM), the associated file
 * is ResampleFilter.m.
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

#include "Resample.h"
#include "ResampleFilter.h"
#ifdef HAVE_SPEEX
# include <string.h>
# define RESAMPLING_QUALITY 6 /* 0-10 : 0=fast/bad 10=slow/good */
#endif


/* Implementation *************************************************************/
int CResample::Resample(CVector<_REAL>* prInput, CVector<_REAL>* prOutput,
						_REAL rRation)
{
	/* Move old data from the end to the history part of the buffer and
	   add new data (shift register) */
	vecrIntBuff.AddEnd((*prInput), iInputBlockSize);

	/* Sample-interval of new sample frequency in relation to interpolated
	   sample-interval */
	rTStep = (_REAL) INTERP_DECIM_I_D / rRation;

	/* Init output counter */
	int im = 0;

	/* Main loop */
	do
	{
		/* Quantize output-time to interpolated time-index */
		const int ik = (int) rtOut;


		/* Calculate convolutions for the two interpolation-taps ------------ */
		/* Phase for the linear interpolation-taps */
		const int ip1 = ik % INTERP_DECIM_I_D;
		const int ip2 = (ik + 1) % INTERP_DECIM_I_D;

		/* Sample positions in input vector */
		const int in1 = (int) (ik / INTERP_DECIM_I_D);
		const int in2 = (int) ((ik + 1) / INTERP_DECIM_I_D);

		/* Convolution */
		_REAL ry1 = (_REAL) 0.0;
		_REAL ry2 = (_REAL) 0.0;
		for (int i = 0; i < RES_FILT_NUM_TAPS_PER_PHASE; i++)
		{
			ry1 += fResTaps1To1[ip1][i] * vecrIntBuff[in1 - i];
			ry2 += fResTaps1To1[ip2][i] * vecrIntBuff[in2 - i];
		}


		/* Linear interpolation --------------------------------------------- */
		/* Get numbers after the comma */
		const _REAL rxInt = rtOut - (int) rtOut;
		(*prOutput)[im] = (ry2 - ry1) * rxInt + ry1;


		/* Increase output counter */
		im++;

		/* Increase output-time and index one step */
		rtOut = rtOut + rTStep;
	} 
	while (rtOut < rBlockDuration);

	/* Set rtOut back */
	rtOut -= iInputBlockSize * INTERP_DECIM_I_D;

	return im;
}

void CResample::Init(const int iNewInputBlockSize)
{
	iInputBlockSize = iNewInputBlockSize;

	/* History size must be one sample larger, because we use always TWO
	   convolutions */
	iHistorySize = RES_FILT_NUM_TAPS_PER_PHASE + 1;

	/* Calculate block duration */
	rBlockDuration =
		(iInputBlockSize + RES_FILT_NUM_TAPS_PER_PHASE) * INTERP_DECIM_I_D;

	/* Allocate memory for internal buffer, clear sample history */
	vecrIntBuff.Init(iInputBlockSize + iHistorySize, (_REAL) 0.0);

	/* Init absolute time for output stream (at the end of the history part) */
	rtOut = (_REAL) RES_FILT_NUM_TAPS_PER_PHASE * INTERP_DECIM_I_D;
}

#ifdef HAVE_SPEEX
CAudioResample::CAudioResample() :
	rRation(1.0), iInputBlockSize(0), iOutputBlockSize(0),
	resampler(NULL), iInputBuffered(0), iMaxInputSize(0)
{
}
CAudioResample::~CAudioResample()
{
	Free();
}
void CAudioResample::Free()
{
	if (resampler != NULL)
	{
		speex_resampler_destroy(resampler);
		resampler = NULL;
	}
	vecfInput.Init(0);
	vecfOutput.Init(0);
	rRation = 1.0;
}
void CAudioResample::Resample(CVector<_REAL>& rInput, CVector<_REAL>& rOutput)
{
	if (rRation == 1.0)
	{
		memcpy(&rOutput[0], &rInput[0], sizeof(_REAL) * iOutputBlockSize);
	}
	else
	{
		int i;
		if (rOutput.Size() != iOutputBlockSize)
			qDebug("CAudioResample::Resample(): rOutput.Size(%i) != iOutputBlockSize(%i)", (int)rOutput.Size(), iOutputBlockSize);

		int iInputSize = GetFreeInputSize();
		for (i = 0; i < iInputSize; i++)
			vecfInput[i+iInputBuffered] = rInput[i];

		int input_frames_used = 0;
		int output_frames_gen = 0;
		int input_frames = iInputBuffered + iInputSize;

		if (resampler != NULL)
		{
			spx_uint32_t in_len = input_frames;
			spx_uint32_t out_len = iOutputBlockSize;
			int err = speex_resampler_process_float(
				resampler,
				0,
				&vecfInput[0],
				&in_len,
				&vecfOutput[0],
				&out_len);
			if (err != RESAMPLER_ERR_SUCCESS)
				qDebug("CAudioResample::Init(): libspeexdsp error: %s", speex_resampler_strerror(err));
			input_frames_used = err != RESAMPLER_ERR_SUCCESS ? 0 : in_len;
			output_frames_gen = err != RESAMPLER_ERR_SUCCESS ? 0 : out_len;
		}

		if (output_frames_gen != iOutputBlockSize)
			qDebug("CAudioResample::Resample(): output_frames_gen(%i) != iOutputBlockSize(%i)", output_frames_gen, iOutputBlockSize);

		for (i = 0; i < iOutputBlockSize; i++)
			rOutput[i] = vecfOutput[i];
		
		iInputBuffered = input_frames - input_frames_used;
		for (i = 0; i < iInputBuffered; i++)
			vecfInput[i] = vecfInput[i+input_frames_used];
	}
}
int CAudioResample::GetMaxInputSize() const
{
	return iMaxInputSize != 0 ? iMaxInputSize : iInputBlockSize;
}
int CAudioResample::GetFreeInputSize() const
{
	return GetMaxInputSize() - iInputBuffered;
}
void CAudioResample::Reset()
{
	iInputBuffered = 0;
	if (resampler != NULL)
	{
		int err = speex_resampler_reset_mem(resampler);
		if (err != RESAMPLER_ERR_SUCCESS)
			qDebug("CAudioResample::Init(): libspeexdsp error: %s", speex_resampler_strerror(err));
	}
}
void CAudioResample::Init(const int iNewInputBlockSize, const _REAL rNewRation)
{
	Free();
	if (!iNewInputBlockSize)
		return;
	iInputBlockSize = iNewInputBlockSize;
	iOutputBlockSize = int(iNewInputBlockSize * rNewRation);
	rRation = _REAL(iOutputBlockSize) / iInputBlockSize;
	iInputBuffered = 0;
	iMaxInputSize = 0;
	if (rRation != 1.0)
	{
		int err;
		resampler = speex_resampler_init(1, spx_uint32_t(iInputBlockSize), spx_uint32_t(iOutputBlockSize), RESAMPLING_QUALITY, &err);
		if (!resampler)
			qDebug("CAudioResample::Init(): libspeexdsp error: %s", speex_resampler_strerror(err));
		vecfInput.Init(iInputBlockSize);
		vecfOutput.Init(iOutputBlockSize);
	}
}
void CAudioResample::Init(const int iNewOutputBlockSize, const int iInputSamplerate, const int iOutputSamplerate)
{
	rRation = _REAL(iOutputSamplerate) / iInputSamplerate;
	iInputBlockSize = int(iNewOutputBlockSize / rRation);
	iOutputBlockSize = iNewOutputBlockSize;
	if (rRation != 1.0)
	{
		const int iNewMaxInputSize = iInputBlockSize * 2;
		const int iInputSize = vecfInput.Size();
		if (iInputSize < iNewMaxInputSize)
		{
			vecfInput.Enlarge(iNewMaxInputSize - iInputSize);
			iMaxInputSize = iNewMaxInputSize;
		}
		vecfOutput.Init(iOutputBlockSize);
		int err = RESAMPLER_ERR_SUCCESS;
		if (resampler == NULL)
		{
			resampler = speex_resampler_init(1, spx_uint32_t(iInputSamplerate), spx_uint32_t(iOutputSamplerate), RESAMPLING_QUALITY, &err);
			iInputBuffered = 0;
		}
		else
		{
			err = speex_resampler_set_rate(resampler, spx_uint32_t(iInputSamplerate), spx_uint32_t(iOutputSamplerate));
		}
		if (err != RESAMPLER_ERR_SUCCESS)
			qDebug("CAudioResample::Init(): libspeexdsp error: %s", speex_resampler_strerror(err));
	}
	else
	{
		Free();
	}
}

#else // HAVE_SPEEX
CAudioResample::CAudioResample() {}
CAudioResample::~CAudioResample() {}
void CAudioResample::Resample(CVector<_REAL>& rInput, CVector<_REAL>& rOutput)
{
	int j;

	if (rRation == (_REAL) 1.0)
	{
		/* If ratio is 1, no resampling is needed, just copy vector */
		for (j = 0; j < iOutputBlockSize; j++)
			rOutput[j] = rInput[j];
	}
	else
	{
		/* Move old data from the end to the history part of the buffer and
		   add new data (shift register) */
		vecrIntBuff.AddEnd(rInput, iInputBlockSize);

		/* Main loop */
		for (j = 0; j < iOutputBlockSize; j++)
		{
			/* Phase for the linear interpolation-taps */
			const int ip =
				(int) (j * INTERP_DECIM_I_D / rRation) % INTERP_DECIM_I_D;

			/* Sample position in input vector */
			const int in = (int) (j / rRation) + RES_FILT_NUM_TAPS_PER_PHASE;

			/* Convolution */
			_REAL ry = (_REAL) 0.0;
			for (int i = 0; i < RES_FILT_NUM_TAPS_PER_PHASE; i++)
				ry += fResTaps1To1[ip][i] * vecrIntBuff[in - i];

			rOutput[j] = ry;
		}
	}
}
void CAudioResample::Init(int iNewInputBlockSize, _REAL rNewRation)
{
	rRation = rNewRation;
	iInputBlockSize = iNewInputBlockSize;
	iOutputBlockSize = (int) (iInputBlockSize * rRation);
	Reset();
}
void CAudioResample::Init(const int iNewOutputBlockSize, const int iInputSamplerate, const int iOutputSamplerate)
{
	rRation = _REAL(iOutputSamplerate) / iInputSamplerate;
	iInputBlockSize = (int) (iNewOutputBlockSize / rRation);
	iOutputBlockSize = iNewOutputBlockSize;
	Reset();
}
int CAudioResample::GetMaxInputSize() const
{
	return iInputBlockSize;
}
int CAudioResample::GetFreeInputSize() const
{
	return iInputBlockSize;
}
void CAudioResample::Reset()
{
	/* Allocate memory for internal buffer, clear sample history */
	vecrIntBuff.Init(iInputBlockSize + RES_FILT_NUM_TAPS_PER_PHASE,
		(_REAL) 0.0);
}
#endif // HAVE_SPEEX

void CSpectrumResample::Resample(CVector<_REAL>* prInput, CVector<_REAL>** pprOutput,
	int iNewOutputBlockSize, _BOOLEAN bResample)
{
	if (!bResample)
		iNewOutputBlockSize = 0;

	if (iNewOutputBlockSize != iOutputBlockSize)
	{
		iOutputBlockSize = iNewOutputBlockSize;

		/* Allocate memory for internal buffer */
		vecrIntBuff.Init(iNewOutputBlockSize);
	}

	int iInputBlockSize = prInput->Size();
	_REAL rRation = _REAL(iInputBlockSize) / _REAL(iOutputBlockSize);

	if (!bResample || rRation <= (_REAL) 1.0)
	{
		/* If ratio is 1 or less, no resampling is needed */
		*pprOutput = prInput;
	}
	else
	{
		int j, i;
		CVector<_REAL>* prOutput;
		prOutput = &vecrIntBuff;
		_REAL rBorder = rRation;
		_REAL rMax = -1.0e10;
		_REAL rValue;

		/* Main loop */
		for (j = 0, i = 0; j < iInputBlockSize && i < iOutputBlockSize; j++)
		{
			rValue = (*prInput)[j];
			/* We only take the maximum value within the interval,
			   because what is important it's the signal
			   and not the lack of signal */
			if (rValue > rMax) rMax = rValue;
			if (j > (int)floor(rBorder))
			{
				(*prOutput)[i++] = rMax - 6.0;
				rMax = -1.0e10;
				rBorder = rRation * i;
			}
		}
		if (i < iOutputBlockSize)
			(*prOutput)[i] = rMax - 6.0;
		*pprOutput = prOutput;
	}
}

