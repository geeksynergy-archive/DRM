/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2002, 2012, 2013
 *
 * Author(s):
 *	Volker Fischer, David Flamand (added CSpectrumResample, speex resampler)
 *
 * Description:
 *	See Resampling.cpp
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

#if !defined(RESAMPLE_H__3B0FEUFE7876F_FE8FE_CA63_4344_1912__INCLUDED_)
#define RESAMPLE_H__3B0FEUFE7876F_FE8FE_CA63_4344_1912__INCLUDED_

#include "../GlobalDefinitions.h"
#include "../util/Vector.h"
#ifdef HAVE_SPEEX
# include <speex/speex_resampler.h>
#endif


/* Classes ********************************************************************/
class CResample
{
public:
	CResample() {}
	virtual ~CResample() {}

	void Init(const int iNewInputBlockSize);
	int Resample(CVector<_REAL>* prInput, CVector<_REAL>* prOutput,
				 _REAL rRation);

protected:
	_REAL					rTStep;
	_REAL					rtOut;
	_REAL					rBlockDuration;

	CShiftRegister<_REAL>	vecrIntBuff;
	int						iHistorySize;

	int						iInputBlockSize;
};

class CAudioResample
{
public:
	CAudioResample();
	virtual ~CAudioResample();

	void Init(int iNewInputBlockSize, _REAL rNewRation);
	void Init(int iNewOutputBlockSize, int iInputSamplerate, int iOutputSamplerate);
	void Resample(CVector<_REAL>& rInput, CVector<_REAL>& rOutput);
	int GetFreeInputSize() const;
	int GetMaxInputSize() const;
	void Reset();

protected:
	_REAL					rRation;
	int						iInputBlockSize;
	int						iOutputBlockSize;
#ifdef HAVE_SPEEX
	SpeexResamplerState*	resampler; 
	CVector<float>			vecfInput;
	CVector<float>			vecfOutput;
	int						iInputBuffered;
	int						iMaxInputSize;
	void Free();
#else
	CShiftRegister<_REAL>	vecrIntBuff;
	int						iHistorySize;
#endif
};

class CSpectrumResample
{
public:
	CSpectrumResample() : iOutputBlockSize(0) {}
	virtual ~CSpectrumResample() {}

	void Resample(CVector<_REAL>* prInput, CVector<_REAL>** pprOutput, int iNewOutputBlockSize, _BOOLEAN bResample);

protected:
	CVector<_REAL>			vecrIntBuff;
	int						iOutputBlockSize;
};

#endif // !defined(RESAMPLE_H__3B0FEUFE7876F_FE8FE_CA63_4344_1912__INCLUDED_)
