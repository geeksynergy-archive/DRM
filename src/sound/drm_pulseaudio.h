/******************************************************************************\
 *
 * Copyright (c) 2012-2013
 *
 * Author(s):
 *	David Flamand
 *
 * Decription:
 *  PulseAudio sound interface with clock drift adjustment (optional)
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

#ifndef DRM_PULSEAUDIO_H_INCLUDED
#define DRM_PULSEAUDIO_H_INCLUDED


/* Master switch */
//#define ENABLE_CLOCK_DRIFT_ADJ


#include <pulse/pulseaudio.h>
#include "../sound/soundinterface.h"
#if defined(PA_STREAM_VARIABLE_RATE) && defined(ENABLE_CLOCK_DRIFT_ADJ)
# define CLOCK_DRIFT_ADJ_ENABLED
#endif

#ifdef CLOCK_DRIFT_ADJ_ENABLED
# include "../matlib/MatlibSigProToolbox.h"
#endif

#ifndef _WIN32
# define ENABLE_STDIN_STDOUT
#endif


class CSoundOutPulse;
typedef struct pa_stream_notify_cb_userdata_t
{
	CSoundOutPulse*		SoundOutPulse;
	_BOOLEAN			bOverflow;
} pa_stream_notify_cb_userdata_t;

typedef struct pa_common
{
	_BOOLEAN		bClockDriftComp;
	int				sample_rate_offset;
} pa_common;

typedef struct pa_object
{
	pa_mainloop		*pa_m;
	pa_context		*pa_c;
	int				ref_count;
} pa_object;


/* Classes ********************************************************************/

class CSoundPulse
{
public:
	CSoundPulse(_BOOLEAN bPlayback);
	virtual ~CSoundPulse() {}
	void			Enumerate(vector<string>& names, vector<string>& descriptions);
	void			SetDev(string sNewDevice);
	string			GetDev();
protected:
	_BOOLEAN		IsDefaultDevice();
	_BOOLEAN		bPlayback;
	_BOOLEAN		bChangDev;
	string			sCurrentDevice;
#ifdef ENABLE_STDIN_STDOUT
	_BOOLEAN		IsStdinStdout();
	_BOOLEAN		bStdinStdout;
#endif
};

class CSoundInPulse : public CSoundPulse, public CSoundInInterface
{
public:
	CSoundInPulse();
	virtual ~CSoundInPulse() {}
	void			Enumerate(vector<string>& names, vector<string>& descriptions) {CSoundPulse::Enumerate(names, descriptions);};
	string			GetDev() {return CSoundPulse::GetDev();};
	void			SetDev(string sNewDevice) {CSoundPulse::SetDev(sNewDevice);};

	_BOOLEAN		Init(int iNewSampleRate, int iNewBufferSize, _BOOLEAN bNewBlocking);
	_BOOLEAN		Read(CVector<_SAMPLE>& psData);
	void			Close();
#ifdef CLOCK_DRIFT_ADJ_ENABLED
	void			SetCommonParamPtr(pa_common *cp_ptr) { cp = cp_ptr; }
#endif

protected:
	void			Init_HW();
	int				Read_HW(void *recbuf, int size);
	void			Close_HW();
	void			SetBufferSize_HW();

	int				iSampleRate;
	int				iBufferSize;
	long			lTimeToWait;
	_BOOLEAN		bBlockingRec;

	_BOOLEAN		bBufferingError;

	pa_stream		*pa_s;
	size_t			remaining_nbytes;
	const char		*remaining_data;

#ifdef CLOCK_DRIFT_ADJ_ENABLED
	int				record_sample_rate;
	_BOOLEAN		bClockDriftComp;
	pa_common		*cp;
#endif
};

class CSoundOutPulse : public CSoundPulse, public CSoundOutInterface
{
public:
	CSoundOutPulse();
	virtual ~CSoundOutPulse() {}
	void			Enumerate(vector<string>& names, vector<string>& descriptions) {CSoundPulse::Enumerate(names, descriptions);};
	string			GetDev() {return CSoundPulse::GetDev();};
	void			SetDev(string sNewDevice) {CSoundPulse::SetDev(sNewDevice);};

	_BOOLEAN		Init(int iNewSampleRate, int iNewBufferSize, _BOOLEAN bNewBlocking);
	_BOOLEAN		Write(CVector<_SAMPLE>& psData);
	void			Close();
#ifdef CLOCK_DRIFT_ADJ_ENABLED
	pa_common *		GetCommonParamPtr() { return &cp; }
	void			EnableClockDriftAdj(_BOOLEAN bEnable) { bNewClockDriftComp = bEnable; }
	_BOOLEAN		IsClockDriftAdjEnabled() { return bNewClockDriftComp; }
#endif

	_BOOLEAN		bPrebuffer;
	_BOOLEAN		bSeek;
	_BOOLEAN		bBufferingError;
	_BOOLEAN		bMuteError;

protected:
	void			Init_HW();
	int				Write_HW(void *playbuf, int size);
	void			Close_HW();

	int				iSampleRate;
	int				iBufferSize;
	long			lTimeToWait;
	_BOOLEAN		bBlockingPlay;

	pa_stream		*pa_s;
	pa_stream_notify_cb_userdata_t pa_stream_notify_cb_userdata_underflow;
	pa_stream_notify_cb_userdata_t pa_stream_notify_cb_userdata_overflow;

#ifdef CLOCK_DRIFT_ADJ_ENABLED
	int				iMaxSampleRateOffset;
	CReal			playback_usec_smoothed;
	int				target_latency;
	int				filter_stabilized;
	int				wait_prebuffer;
	CMatlibVector<CReal> B, A, X, Z;
	_BOOLEAN		bNewClockDriftComp;
	pa_common		cp;
	int				playback_usec; // DEBUG
	int				clock; // DEBUG
#endif
};

#endif
