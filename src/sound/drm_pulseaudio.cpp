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

#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include "drm_pulseaudio.h"

#ifndef PA_STREAM_ADJUST_LATENCY
# define PA_STREAM_ADJUST_LATENCY 0
#endif
#ifndef PA_STREAM_DONT_MOVE
# define PA_STREAM_DONT_MOVE 0
#endif

#define RECORD_BUFFER_US    2000000 // us
#define PLAYBACK_BUFFER_US  2000000 // us
#define PLAYBACK_LATENCY_US 1000000 // us (50% of PLAYBACK_BUFFER_US)
#define WAIT_PREBUFFER ((int)(PLAYBACK_LATENCY_US + 399999) / 400000) // 400000us = .4s (frame duration)
#define NUM_CHANNELS 2 // Stereo
#define BYTES_PER_SAMPLE ((int)sizeof(_SAMPLE))
#define PA_RECORD_MAXLENGTH ((int)((double)(NUM_CHANNELS * BYTES_PER_SAMPLE * iSampleRate) * ((double)RECORD_BUFFER_US)    / (double)1000000))
#define PA_PLAYBACK_TLENGTH ((int)((double)(NUM_CHANNELS * BYTES_PER_SAMPLE * iSampleRate) * ((double)PLAYBACK_BUFFER_US)  / (double)1000000))
#define PA_PLAYBACK_PREBUF  ((int)((double)(NUM_CHANNELS * BYTES_PER_SAMPLE * iSampleRate) * ((double)PLAYBACK_LATENCY_US) / (double)1000000))
#define STREAM_FLAGS (PA_STREAM_ADJUST_LATENCY | PA_STREAM_DONT_MOVE)
#define STREAM_NAME(io, blocking) (blocking ? "Signal " io : "Audio " io)
#define APP_NAME(io, blocking) ((!io != !blocking) ? "Dream Transmitter" : "Dream Receiver")

#define DEBUG_MSG(...) fprintf(stderr, __VA_ARGS__)


#ifdef CLOCK_DRIFT_ADJ_ENABLED
/* TODO more optimized algorithm and/or parameters */
# define ALGO_ERROR_MULTIPLIER 256.0
# define ALGO_ERROR_EXPONENT 2.0
# include "LatencyFilter.h"
#endif


/* stdin/stdout ***************************************************************/

#ifdef ENABLE_STDIN_STDOUT
# define STDIN_STDOUT_DEVICE_NAME "-"
static int StdoutWrite(const char *buf, ssize_t count)
{
	ssize_t chunk;
	while (count > 0) {
		chunk = write(STDOUT_FILENO, buf, count);
		if (chunk <= 0)
			return 1;
		buf += chunk;
		count -= chunk;
	};
	return 0;
}
static int StdinRead(char *buf, ssize_t count)
{
	ssize_t chunk;
	while (count > 0) {
		chunk = read(STDIN_FILENO, buf, count);
		if (chunk <= 0) {
			memset(buf, 0, count);
			return 1;
		}
		buf += chunk;
		count -= chunk;
	};
	return 0;
}
#endif


/* PulseAudio low level functions *********************************************/


static pa_object pa_obj =
{
	/*.pa_m =*/			NULL,
	/*.pa_c =*/			NULL,
	/*.ref_count =*/	1
};

static int pa_c_sync(pa_object *pa_obj, int error)
{
	int retval;
	pa_mainloop *pa_m = pa_obj->pa_m;
	pa_context *pa_c = pa_obj->pa_c;
	pa_context_state_t pa_c_s;
	if (error==PA_OK) {
		do {
			pa_mainloop_iterate(pa_m, 1, &retval);
			pa_c_s = pa_context_get_state(pa_c);
		} while (pa_c_s!=PA_CONTEXT_READY && pa_c_s!=PA_CONTEXT_FAILED);
		return pa_c_s==PA_CONTEXT_READY ? PA_OK : PA_ERR_MAX;
	}
	DEBUG_MSG("pa_c_sync failed, error %i\n", error);
	return error;
}
static int pa_o_sync(pa_object *pa_obj, pa_operation *pa_o)
{
	int retval, error;
	pa_mainloop *pa_m = pa_obj->pa_m;
	pa_context *pa_c = pa_obj->pa_c;
	pa_operation_state pa_o_s;
	if (pa_o) {
		do {
			pa_mainloop_iterate(pa_m, 1, &retval);
			pa_o_s = pa_operation_get_state(pa_o);
		} while (pa_o_s==PA_OPERATION_RUNNING);
		pa_operation_unref(pa_o);
		return pa_o_s==PA_OPERATION_DONE ? PA_OK : PA_ERR_MAX;
	}
	error = pa_context_errno(pa_c);
	DEBUG_MSG("pa_o_sync failed, error %i\n", error);
	return error;
}
static int pa_s_sync(pa_object *pa_obj, pa_stream *pa_s, int error)
{
	int retval;
	pa_mainloop *pa_m = pa_obj->pa_m;
	pa_stream_state pa_s_s;
	if (error==PA_OK) {
		do {
			pa_mainloop_iterate(pa_m, 1, &retval);
			pa_s_s = pa_stream_get_state(pa_s);
		} while (pa_s_s==PA_STREAM_CREATING);
		return pa_s_s==PA_STREAM_READY ? PA_OK : PA_ERR_MAX;
	}
	DEBUG_MSG("pa_s_sync failed, error %i\n", error);
	return error;
}

static int pa_init(pa_object *pa_obj, const char *app_name)
{
	int ret;
	pa_mainloop *pa_m_tmp;
	pa_context *pa_c_tmp;
//	DEBUG_MSG("pa_init ref_count=%i\n", pa_obj->ref_count);
	if (pa_obj->pa_m == NULL) {
		pa_m_tmp = pa_mainloop_new();
		if (!pa_m_tmp) {
			DEBUG_MSG("pa_init pa_mainloop_new failed\n");
			return PA_ERR_MAX;
		}
		pa_c_tmp = pa_context_new(pa_mainloop_get_api(pa_m_tmp), app_name);
		if (!pa_c_tmp) {
			DEBUG_MSG("pa_init pa_context_new failed\n");
			pa_mainloop_free(pa_m_tmp);
			return PA_ERR_MAX;
		}
		pa_object pa_obj_tmp = { /*.pa_m=*/pa_m_tmp, /*.pa_c=*/pa_c_tmp, /*.ref_count=*/1 };
		ret = pa_context_connect(pa_c_tmp, NULL, PA_CONTEXT_NOFLAGS, NULL);
		if (pa_c_sync(&pa_obj_tmp, ret)!=PA_OK) {
			DEBUG_MSG("pa_init pa_context_connect failed\n");
			pa_context_unref(pa_c_tmp);
			pa_mainloop_free(pa_m_tmp);
			return PA_ERR_MAX;
		}
		pa_obj->pa_m = pa_m_tmp;
		pa_obj->pa_c = pa_c_tmp;
	}
	pa_obj->ref_count++;
	return PA_OK;
}

static void pa_free(pa_object *pa_obj)
{
	pa_mainloop **pa_m = &pa_obj->pa_m;
	pa_context **pa_c = &pa_obj->pa_c;
//	DEBUG_MSG("pa_free ref_count=%i\n", pa_obj->ref_count);
	if (pa_obj->ref_count > 0)
	{
		pa_obj->ref_count--;
		if (pa_obj->ref_count == 0)
		{
//			DEBUG_MSG("pa_free freeing\n");
			if (*pa_c) {
				pa_context_disconnect(*pa_c);
				pa_context_unref(*pa_c);
				*pa_c = NULL;
			}
			if (*pa_m) {
				pa_mainloop_free(*pa_m);
				*pa_m = NULL;
			}
		}
	}
}

static void pa_s_free(pa_stream **pa_s)
{
	if (*pa_s) {
		pa_stream_disconnect(*pa_s);
		pa_stream_unref(*pa_s);
		*pa_s = NULL;
	}
}

static void pa_stream_notify_cb(pa_stream *, void *userdata)
{
	pa_stream_notify_cb_userdata_t* ud = (pa_stream_notify_cb_userdata_t*)userdata;
	CSoundOutPulse& SoundOutPulse = *ud->SoundOutPulse;
	if (!SoundOutPulse.bMuteError)
	{
		if (!SoundOutPulse.bPrebuffer)
		{
			DEBUG_MSG("*** playback %sFLOW\n", ud->bOverflow ? "OVER" : "UNDER");
			SoundOutPulse.bBufferingError = TRUE;
		}
	}
	if (ud->bOverflow)
		SoundOutPulse.bSeek = TRUE;
	else
		SoundOutPulse.bPrebuffer = TRUE;
}

static void pa_stream_success_cb(pa_stream *, int /*success*/, void */*userdata*/)
{
//	if (userdata)
//		DEBUG_MSG("pa_stream_success_cb(%s) = %i\n", (char*)userdata, success);
}

static int pa_stream_get_latency(pa_object *pa_obj, pa_stream *pa_s, int sample_rate, uint64_t *usec)
{
	if (pa_o_sync(pa_obj, pa_stream_update_timing_info(pa_s, pa_stream_success_cb, NULL)) == PA_OK) {
		const pa_timing_info *ti;
		ti = pa_stream_get_timing_info(pa_s);
		if (ti && !ti->write_index_corrupt && !ti->read_index_corrupt) {
			uint64_t samples = llabs(ti->write_index - ti->read_index) / (NUM_CHANNELS*BYTES_PER_SAMPLE);
			*usec = samples * (uint32_t)1000000 / (uint32_t)sample_rate;
			return 1;
		}
	}
	return 0;
}

#ifdef CLOCK_DRIFT_ADJ_ENABLED
static void pa_set_sample_rate(pa_object *pa_obj, pa_stream *pa_s, int sample_rate)
{
	if (pa_o_sync(pa_obj, pa_stream_update_sample_rate(pa_s, sample_rate, pa_stream_success_cb, NULL)) != PA_OK)
		DEBUG_MSG("pa_set_sample_rate(%i): pa_stream_update_sample_rate failed\n", sample_rate);
}
#endif


/****************/
/* devices list */

typedef struct USERDATA {
	vector<string> *names;
	vector<string> *descriptions;
} USERDATA;

static void pa_source_info_cb(pa_context *, const pa_source_info *i, int eol, void *userdata)
{
	if (!eol)
	{
		((USERDATA*)userdata)->names->push_back(string(i->name));
		((USERDATA*)userdata)->descriptions->push_back(string(i->description));
	}
}
static void pa_sink_info_cb(pa_context *, const pa_sink_info *i, int eol, void *userdata)
{
	if (!eol)
	{
		((USERDATA*)userdata)->names->push_back(string(i->name));
		((USERDATA*)userdata)->descriptions->push_back(string(i->description));
	}
}


/* Classes ********************************************************************/

/* Implementation *************************************************************/


void CSoundInPulse::Init_HW()
{
	int ret;
	pa_sample_spec ss;
	pa_buffer_attr pa_attr;
	const char *recdevice=NULL;

	DEBUG_MSG("CSoundInPulse::Init_HW()\n");

	ss.format = PA_SAMPLE_S16NE;
	ss.channels = NUM_CHANNELS;
	ss.rate = iSampleRate;

	/* record device */
	if (!IsDefaultDevice())
		recdevice = sCurrentDevice.c_str();

	if (pa_init(&pa_obj, APP_NAME(1, bBlockingRec)) != PA_OK)
	{
		DEBUG_MSG("CSoundInPulse::Init_HW pa_init failed\n");
		return;
	}

	pa_s = pa_stream_new(pa_obj.pa_c,		// The context to create this stream in
		STREAM_NAME("input", bBlockingRec),	// A name for this stream
		&ss,								// Our sample format.
		NULL								// Use default channel map
		);
	if (!pa_s)
	{
		DEBUG_MSG("CSoundInPulse::Init_HW pa_stream_new failed\n");
		return;
	}

	pa_attr.maxlength = PA_RECORD_MAXLENGTH;			// Maximum length of the buffer.
	pa_attr.tlength   = -1;								// Playback only: target length of the buffer.
	pa_attr.prebuf    = -1; 							// Playback only: pre-buffering.
	pa_attr.minreq    = -1;								// Playback only: minimum request.
	pa_attr.fragsize  = iBufferSize*BYTES_PER_SAMPLE;	// Recording only: fragment size.

	ret = pa_stream_connect_record(pa_s,	// The stream to connect to a source 
		recdevice,							// Name of the source to connect to, or NULL for default
		&pa_attr,							// Buffer attributes, or NULL for default
#ifdef CLOCK_DRIFT_ADJ_ENABLED
		(pa_stream_flags_t)(STREAM_FLAGS | (!bBlockingRec ? PA_STREAM_VARIABLE_RATE : 0)) // Additional flags, or 0 for default
#else
		(pa_stream_flags_t)STREAM_FLAGS		// Additional flags, or 0 for default
#endif
		);
	if (pa_s_sync(&pa_obj, pa_s, ret) != PA_OK)
		DEBUG_MSG("CSoundInPulse::Init_HW pa_stream_connect_record failed\n");

	remaining_nbytes = 0;
	remaining_data   = NULL;

	DEBUG_MSG("pulseaudio input device '%s', init done\n", sCurrentDevice.c_str());
}

int CSoundInPulse::Read_HW(void *recbuf, int size)
{
	int ret, retval, filled, chunk;
	const void *data;
	size_t nbytes;

	filled = 0;
	size *= BYTES_PER_SAMPLE;

	if (pa_s)
	{
		/* Buffering error when latency >= 75% of RECORD_BUFFER_US */
		_BOOLEAN bError = TRUE;
		uint64_t recording_usec;
		if (pa_stream_get_latency(&pa_obj, pa_s, iSampleRate, &recording_usec))
			bError = recording_usec >= (uint64_t)(RECORD_BUFFER_US * 75 / 100) ? TRUE : FALSE;
		bBufferingError |= bError;

#ifdef CLOCK_DRIFT_ADJ_ENABLED
		if (!bBlockingRec && cp) {
			if (bClockDriftComp != cp->bClockDriftComp) {
				bClockDriftComp = cp->bClockDriftComp;
				DEBUG_MSG("CSoundInPulse::Read_HW(): bClockDriftComp=%i\n", bClockDriftComp);
				if (!bClockDriftComp)
					pa_set_sample_rate(&pa_obj, pa_s, iSampleRate);
			}
			int sample_rate = iSampleRate - cp->sample_rate_offset;
			if (record_sample_rate != sample_rate) {
				record_sample_rate = sample_rate;
				pa_set_sample_rate(&pa_obj, pa_s, sample_rate);
			}
		}
#endif

		while (size) {
			if (!remaining_nbytes) {
				nbytes = 0;
				data   = NULL;
				ret = pa_stream_peek(pa_s, &data, &nbytes);
				if (ret != PA_OK) break;
				if (!data) {
					ret = pa_mainloop_iterate(pa_obj.pa_m, 1, &retval);
					if (ret < 0) break;
				}
			}
			else {
				nbytes = remaining_nbytes;
				data   = remaining_data;
			}
			if (data) {
				if (nbytes > (size_t)size) {
					chunk = size;
					remaining_nbytes = nbytes - chunk;
					remaining_data   = (char*)data   + chunk;
//					DEBUG_MSG("pa_stream_peek frag %6i %6i\n", (int)nbytes, chunk);
					memcpy(recbuf, data, chunk);
				}
				else {
					chunk = (int)nbytes;
					remaining_nbytes = 0;
					remaining_data   = NULL;
//					DEBUG_MSG("pa_stream_peek full %6i %6i\n", (int)nbytes, chunk);
					memcpy(recbuf, data, chunk);
					pa_stream_drop(pa_s); // <- after memcpy
				}
				filled += chunk;
				size -= chunk;
				recbuf = ((char*)recbuf)+chunk;
			}
		}
	}

	if (size > 0) {
		memset(recbuf, 0, size);
		if (bBlockingRec)
			usleep(lTimeToWait);
	}

//	DEBUG_MSG("CSoundInPulse::read_HW filled %6i\n", filled);
	return filled / BYTES_PER_SAMPLE;
}

void CSoundInPulse::Close_HW()
{
	if (pa_s)
	{
		DEBUG_MSG("CSoundInPulse::close_HW()\n");
		remaining_nbytes = 0;
		remaining_data   = NULL;
		pa_s_free(&pa_s);
		pa_free(&pa_obj);
	}
}

void CSoundInPulse::SetBufferSize_HW()
{
#ifdef PA_STREAM_FIX_RATE /* used to check for at least version 0.9.8 */
	pa_buffer_attr pa_attr;
	pa_attr.maxlength = PA_RECORD_MAXLENGTH;			// Maximum length of the buffer.
	pa_attr.tlength   = -1;								// Playback only: target length of the buffer.
	pa_attr.prebuf    = -1; 							// Playback only: pre-buffering.
	pa_attr.minreq    = -1;								// Playback only: minimum request.
	pa_attr.fragsize  = iBufferSize*BYTES_PER_SAMPLE;	// Recording only: fragment size.
	if (pa_o_sync(&pa_obj, pa_stream_set_buffer_attr(pa_s, &pa_attr, pa_stream_success_cb, NULL)) != PA_OK)
		DEBUG_MSG("CSoundInPulse::SetBufferSize_HW() error\n");
#endif
}

void CSoundOutPulse::Init_HW()
{
	int ret;
	pa_sample_spec ss;
	pa_buffer_attr pa_attr;
	const char *playdevice=NULL;

	DEBUG_MSG("CSoundOutPulse::Init_HW()\n");

	ss.format = PA_SAMPLE_S16NE;
	ss.channels = NUM_CHANNELS;
	ss.rate = iSampleRate;

	/* playback device */
	if (!IsDefaultDevice())
		playdevice = sCurrentDevice.c_str();

	if (pa_init(&pa_obj, APP_NAME(0, bBlockingPlay)) != PA_OK)
	{
		DEBUG_MSG("CSoundOutPulse::Init_HW pa_init failed\n");
		return;
	}

	pa_s = pa_stream_new(pa_obj.pa_c,			// The context to create this stream in
		STREAM_NAME("output", bBlockingPlay),	// A name for this stream
		&ss,									// Our sample format.
		NULL									// Use default channel map
		);
	if (!pa_s)
	{
		DEBUG_MSG("CSoundOutPulse::Init_HW pa_stream_new failed\n");
		return;
	}

	pa_attr.maxlength = PA_PLAYBACK_TLENGTH;	// Maximum length of the buffer.
	pa_attr.tlength   = PA_PLAYBACK_TLENGTH;	// Playback only: target length of the buffer.
	pa_attr.prebuf    = PA_PLAYBACK_PREBUF;		// Playback only: pre-buffering.
	pa_attr.minreq    = -1;						// Playback only: minimum request.
	pa_attr.fragsize  = -1;						// Recording only: fragment size.

	ret = pa_stream_connect_playback(pa_s,	// The stream to connect to a sink
		playdevice,							// Name of the source to connect to, or NULL for default
		&pa_attr,							// Buffer attributes, or NULL for default
#ifdef CLOCK_DRIFT_ADJ_ENABLED
		(pa_stream_flags_t)(STREAM_FLAGS | (!bBlockingPlay ? PA_STREAM_VARIABLE_RATE : 0)), // Additional flags, or 0 for default
#else
		(pa_stream_flags_t)STREAM_FLAGS,	// Additional flags, or 0 for default
#endif
		NULL,								// Initial volume, or NULL for default
		NULL								// Synchronize this stream with the specified one, or NULL for a standalone stream
		);
	if (pa_s_sync(&pa_obj, pa_s, ret) != PA_OK)
		DEBUG_MSG("CSoundOutPulse::Init_HW pa_stream_connect_playback failed\n");

	pa_stream_notify_cb_userdata_underflow.SoundOutPulse = this;
	pa_stream_notify_cb_userdata_underflow.bOverflow = FALSE;
	pa_stream_set_underflow_callback(pa_s, &pa_stream_notify_cb, (void*)&pa_stream_notify_cb_userdata_underflow);
	pa_stream_notify_cb_userdata_overflow.SoundOutPulse = this;
	pa_stream_notify_cb_userdata_overflow.bOverflow = TRUE;
	pa_stream_set_overflow_callback(pa_s, &pa_stream_notify_cb, (void*)&pa_stream_notify_cb_userdata_overflow);

#ifdef CLOCK_DRIFT_ADJ_ENABLED
	X.Init(1);
	A.Init(1, 1.0);
	B.Init(NUM_TAPS_LATENCY_FILT);
	for (int i = 0; i < NUM_TAPS_LATENCY_FILT; i++)
		B[i] = dLatFilt[i];
#endif

	/* Clear mute error flag */
	bMuteError = FALSE;

	DEBUG_MSG("pulseaudio output device '%s', init done\n", sCurrentDevice.c_str());
}

int CSoundOutPulse::Write_HW(void *playbuf, int size)
{
	int ret;
	int retval;

	if (pa_s)
	{
#ifdef CLOCK_DRIFT_ADJ_ENABLED
		_BOOLEAN bInitClockDriftComp = FALSE;
		if (cp.bClockDriftComp != bNewClockDriftComp) {
			cp.bClockDriftComp = bNewClockDriftComp;
			DEBUG_MSG("CSoundOutPulse::write_HW(): bClockDriftComp=%i\n", cp.bClockDriftComp);
			if (cp.bClockDriftComp)
				bInitClockDriftComp = TRUE;
			else
				if (!bBlockingPlay)
					pa_set_sample_rate(&pa_obj, pa_s, iSampleRate);
		}
#endif

		if (bPrebuffer) {
	//		pa_o_sync(&pa_obj, pa_stream_prebuf(pa_s, pa_stream_success_cb, NULL);
	//		DEBUG_MSG("CSoundOutPulse::write_HW(): prebuffering ...\n");
			if (pa_o_sync(&pa_obj, pa_stream_prebuf(pa_s, pa_stream_success_cb, NULL)) != PA_OK)
				DEBUG_MSG("CSoundOutPulse::write_HW(): prebuffering failed\n");
			bPrebuffer = FALSE;
			bSeek = FALSE;
#ifdef CLOCK_DRIFT_ADJ_ENABLED
			bInitClockDriftComp = TRUE;
#endif
		}

#ifdef CLOCK_DRIFT_ADJ_ENABLED
		if (cp.bClockDriftComp) {
			if (bInitClockDriftComp) {
				iMaxSampleRateOffset = iSampleRate * 2 / 100; /* = 2% */
				playback_usec_smoothed = PLAYBACK_LATENCY_US;
				target_latency = PLAYBACK_LATENCY_US;
				wait_prebuffer = WAIT_PREBUFFER;
				cp.sample_rate_offset = 0;
				filter_stabilized = NUM_TAPS_LATENCY_FILT;
				Z.Init(NUM_TAPS_LATENCY_FILT, CReal(PLAYBACK_LATENCY_US));
				clock = playback_usec = 0; // DEBUG
			}

			if (!bBlockingPlay)
	/*Receiver*/	DEBUG_MSG("playback latency: %07i us, smoothed %07i us, %i, %02i, %02i, %i\n", playback_usec, (int)playback_usec_smoothed, iSampleRate + cp.sample_rate_offset, wait_prebuffer, filter_stabilized, ++clock);
			else
	/*Transmitter*/	DEBUG_MSG("playback latency: %07i us, smoothed %07i us, %i, %02i, %02i, %i\n", playback_usec, (int)playback_usec_smoothed, iSampleRate - cp.sample_rate_offset, wait_prebuffer, filter_stabilized, ++clock);

			if (wait_prebuffer > 0) {
				wait_prebuffer--;
			}
			else {
				if (!filter_stabilized) {
					/****************************************************************************************************************/
					/* The Clock Drift Adjustment Algorithm                                                                         */
					int offset;
					double error = (playback_usec_smoothed - (double)target_latency) / (double)target_latency * ALGO_ERROR_MULTIPLIER;
	//				error = error * error * (error >= 0.0 ? 1.0 : -1.0);
					error = pow(fabs(error), ALGO_ERROR_EXPONENT) * (error >= 0.0 ? 1.0 : -1.0);
					if (error >= 0.0) offset = (int)floor(error + 0.5);
					else              offset = (int) ceil(error - 0.5);
					if      (offset>iMaxSampleRateOffset)  offset=iMaxSampleRateOffset;
					else if (offset<-iMaxSampleRateOffset) offset=-iMaxSampleRateOffset;
					/****************************************************************************************************************/
					if (!bBlockingPlay && cp.sample_rate_offset != offset) {
						pa_set_sample_rate(&pa_obj, pa_s, iSampleRate + offset);
					}
					cp.sample_rate_offset = offset;
				}
			}
		}
#endif

		ret = pa_stream_write(pa_s,		// The stream to use
			playbuf,					// The data to write
			size * BYTES_PER_SAMPLE,	// The length of the data to write in bytes
			NULL,						// A cleanup routine for the data or NULL to request an internal copy
			bSeek ? -(PA_PLAYBACK_PREBUF/2) : 0,	// Offset for seeking, must be 0 for upload streams
			PA_SEEK_RELATIVE			// Seek mode, must be PA_SEEK_RELATIVE for upload streams
			);

#ifdef CLOCK_DRIFT_ADJ_ENABLED
		if (cp.bClockDriftComp) {
			if (!wait_prebuffer) {
				uint64_t playback_usec;
				if (pa_stream_get_latency(&pa_obj, pa_s, iSampleRate, &playback_usec)) {
					this->playback_usec = (int)playback_usec; // DEBUG
					X[0] = CReal(playback_usec);
					playback_usec_smoothed = Filter(B, A, X, Z)[0];
					if (filter_stabilized > 0) {
						filter_stabilized--;
						if(!filter_stabilized) {
							target_latency = playback_usec_smoothed;
						}
					}
				}
			}
		}
#endif

		bSeek = FALSE;

		if (ret == PA_OK) {
			do {
				ret = pa_mainloop_iterate(pa_obj.pa_m, 0, &retval);
			} while (ret>0);
			return size;
		}
	}

	if (size > 0) {
		if (bBlockingPlay)
			usleep(lTimeToWait);
	}

	return -1;
}

void CSoundOutPulse::Close_HW()
{
	if (pa_s)
	{
		DEBUG_MSG("CSoundOutPulse::close_HW()\n");
		bMuteError = TRUE;
		if (bBlockingPlay && pa_obj.pa_m!=NULL && pa_obj.pa_c!=NULL && pa_s!=NULL)
		{
			if (pa_o_sync(&pa_obj, pa_stream_drain(pa_s, pa_stream_success_cb, NULL)) != PA_OK)
			{
				DEBUG_MSG("CSoundOutPulse::close_HW() pa_stream_drain failed\n");
			}		
		}
		pa_s_free(&pa_s);
		pa_free(&pa_obj);
	}
}


/*********************************************************************************************************************/


/* Common ********************************************************************/

CSoundPulse::CSoundPulse(_BOOLEAN bPlayback)
	: bPlayback(bPlayback), bChangDev(TRUE)
#ifdef ENABLE_STDIN_STDOUT
	, bStdinStdout(FALSE)
#endif
{
}

void CSoundPulse::Enumerate(vector<string>& names, vector<string>& descriptions)
{
	pa_object pa_obj = { /*.pa_m=*/NULL, /*.pa_c=*/NULL, /*.ref_count=*/0 };
	pa_operation *pa_o;
	USERDATA userdata;

	names.clear();
	descriptions.clear();
	names.push_back(""); /* default device */
	descriptions.push_back("");

	if (pa_init(&pa_obj, "") != PA_OK)
	{
		DEBUG_MSG("CSoundPulse::Enumerate(): pa_init failed\n");
		return;
	}

	userdata.names = &names;
	userdata.descriptions = &descriptions;

	if (bPlayback)
		pa_o = pa_context_get_sink_info_list(pa_obj.pa_c, pa_sink_info_cb, &userdata);
	else
		pa_o = pa_context_get_source_info_list(pa_obj.pa_c, pa_source_info_cb, &userdata);

	if (pa_o_sync(&pa_obj, pa_o) != PA_OK)
		DEBUG_MSG("CSoundPulse::Enumerate(): pa_context_get_(sink/source)_info_list failed\n");

	pa_free(&pa_obj);
}

void CSoundPulse::SetDev(string sNewDevice)
{
	if (sNewDevice != sCurrentDevice)
	{
		sCurrentDevice = sNewDevice;
		bChangDev = TRUE;
	}
}

string CSoundPulse::GetDev()
{
	return sCurrentDevice;
}

_BOOLEAN CSoundPulse::IsDefaultDevice()
{
	const char *str = sCurrentDevice.c_str();
	return *str == 0;
}

#ifdef ENABLE_STDIN_STDOUT
_BOOLEAN CSoundPulse::IsStdinStdout()
{
	bStdinStdout = sCurrentDevice == STDIN_STDOUT_DEVICE_NAME;
	return bStdinStdout;
}
#endif


/* Wave in ********************************************************************/

CSoundInPulse::CSoundInPulse(): CSoundPulse(FALSE),
	iSampleRate(0), iBufferSize(0), lTimeToWait(0),
	bBlockingRec(FALSE), bBufferingError(FALSE), pa_s(NULL),
	remaining_nbytes(0), remaining_data(NULL)
#ifdef CLOCK_DRIFT_ADJ_ENABLED
	, record_sample_rate(0), bClockDriftComp(FALSE), cp(NULL)
#endif
{
}

_BOOLEAN CSoundInPulse::Init(int iNewSampleRate, int iNewBufferSize, _BOOLEAN bNewBlocking)
{
	DEBUG_MSG("initrec %i %i %i\n", iNewSampleRate, iNewBufferSize, bNewBlocking);
	_BOOLEAN bChanged = FALSE;

#ifdef ENABLE_STDIN_STDOUT
	/* Check if it's stdin */
	if (IsStdinStdout())
	{
		iBufferSize = iNewBufferSize * BYTES_PER_SAMPLE;
		return FALSE;
	}
#endif

	/* Save blocking mode */
	bBlockingRec = bNewBlocking;

	/* Check if device must be opened or reinitialized */
	if (bChangDev == TRUE || iSampleRate != iNewSampleRate)
	{
		/* Save samplerate buffer size */
		iSampleRate = iNewSampleRate;
		iBufferSize = iNewBufferSize;

		/* Time to wait in case of read error (ns) */
		lTimeToWait = (iNewBufferSize / NUM_CHANNELS) * 1000l / (iNewSampleRate / 1000l) + 1000;

		/* Close the previous input */
		Close_HW();

		/* Open the new input */
		Init_HW();

		/* Reset flag */
		bChangDev = FALSE;

		/* Set changed flag */
		bChanged = TRUE;
	}
	else {
		if (iBufferSize != iNewBufferSize)
		{
			/* Save buffer size */
			iBufferSize = iNewBufferSize;

			/* Set buffer size */
			SetBufferSize_HW();
		}
	}

	/* Clear buffering error flag */
	bBufferingError = FALSE;

	return bChanged;
}

_BOOLEAN CSoundInPulse::Read(CVector<_SAMPLE>& psData)
{
#ifdef ENABLE_STDIN_STDOUT
	/* Stdin support */
	if (bStdinStdout)
		return StdinRead((char*)&psData[0], iBufferSize);
#endif

	/* Check if device must be opened or reinitialized */
	if (bChangDev == TRUE)
	{
		/* Reinit sound interface */
		Init(iSampleRate, iBufferSize, bBlockingRec);

		/* Reset flag */
		bChangDev = FALSE;
	}

	/* Read from 'hardware' */
	_BOOLEAN bError = Read_HW(&psData[0], iBufferSize) != iBufferSize;
	if (bError)
	    DEBUG_MSG("CSoundInPulse::Read(): read_HW error\n");

	bError |= bBufferingError;
	bBufferingError = FALSE;

	return bError;
}

void CSoundInPulse::Close()
{
	DEBUG_MSG("stoprec\n");

#ifdef ENABLE_STDIN_STDOUT
	/* Stdout support */
	if (bStdinStdout)
		return;
#endif

	/* Close the input */
	Close_HW();

	/* Set flag to open devices the next time it is initialized */
	bChangDev = TRUE;
}


/* Wave out *******************************************************************/

CSoundOutPulse::CSoundOutPulse(): CSoundPulse(TRUE),
	bPrebuffer(FALSE), bSeek(FALSE),
	bBufferingError(FALSE), bMuteError(FALSE),
	iSampleRate(0), iBufferSize(0), lTimeToWait(0),
	bBlockingPlay(FALSE), pa_s(NULL)
#ifdef CLOCK_DRIFT_ADJ_ENABLED
	, iMaxSampleRateOffset(0)
//	, bNewClockDriftComp(TRUE), cp()
	, bNewClockDriftComp(FALSE), cp()
#endif
{
}

_BOOLEAN CSoundOutPulse::Init(int iNewSampleRate, int iNewBufferSize, _BOOLEAN bNewBlocking)
{
	DEBUG_MSG("initplay %i %i %i\n", iNewSampleRate, iNewBufferSize, bNewBlocking);
	_BOOLEAN bChanged = FALSE;

#ifdef ENABLE_STDIN_STDOUT
	/* Check if it's stdin */
	if (IsStdinStdout())
	{
		iBufferSize = iNewBufferSize * BYTES_PER_SAMPLE;
		return FALSE;
	}
#endif

	/* Save blocking mode and buffer size */
	bBlockingPlay = bNewBlocking;
	iBufferSize = iNewBufferSize;

	/* Check if device must be opened or reinitialized */
	if (bChangDev == TRUE || iSampleRate != iNewSampleRate)
	{
		/* Save samplerate */
		iSampleRate = iNewSampleRate;

		/* Time to wait in case of write error (ns) */
		lTimeToWait = (iNewBufferSize / NUM_CHANNELS) * 1000l / (iNewSampleRate / 1000l) + 1000;

		/* Close the previous input */
		Close_HW();

		/* Open the new input */
		Init_HW();

		/* Reset flag */
		bChangDev = FALSE;

		/* Set changed flag */
		bChanged = TRUE;
	}

	/* Set prebuffer flag */
	bPrebuffer = TRUE;
	/* Clear seek flag */
	bSeek = FALSE;
	/* Clear buffering error flag */
	bBufferingError = FALSE;

	return bChanged;
}

_BOOLEAN CSoundOutPulse::Write(CVector<_SAMPLE>& psData)
{
#ifdef ENABLE_STDIN_STDOUT
	/* Stdout support */
	if (bStdinStdout)
		return StdoutWrite((char*)&psData[0], iBufferSize);
#endif

	/* Check if device must be opened or reinitialized */
	if (bChangDev == TRUE)
	{
		/* Reinit sound interface */
		Init(iSampleRate, iBufferSize, bBlockingPlay);

		/* Reset flag */
		bChangDev = FALSE;
	}

	/* Write to 'hardware' */
	_BOOLEAN bError = Write_HW(&psData[0], iBufferSize) != iBufferSize;
	if (bError)
		DEBUG_MSG("CSoundOutPulse::Write(): write_HW error\n");

	bError |= bBufferingError;
	bBufferingError = FALSE;

	return bError;
}

void CSoundOutPulse::Close()
{
	DEBUG_MSG("stopplay\n");

#ifdef ENABLE_STDIN_STDOUT
	/* Stdout support */
	if (bStdinStdout)
		return;
#endif

	/* Close the output */
	Close_HW();

	/* Set flag to open devices the next time it is initialized */
	bChangDev = TRUE;
}

