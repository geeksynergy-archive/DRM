/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See Sound.cpp
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

#if !defined(AFX_SOUNDIN_H__9518A621_7F78_11D3_8C0D_EEBF182CF549__INCLUDED_)
#define AFX_SOUNDIN_H__9518A621_7F78_11D3_8C0D_EEBF182CF549__INCLUDED_

#include "../sound/soundinterface.h"
#include <windows.h>
#include <mmsystem.h>

/* Set this number as high as we have to prebuffer symbols for one MSC block.
   In case of robustness mode D we have 24 symbols */
#define NUM_SOUND_BUFFERS_IN	24		/* Number of sound card buffers */

#ifdef QT_GUI_LIB
# define NUM_SOUND_BUFFERS_OUT	6		/* Number of sound card buffers */
#else
# define NUM_SOUND_BUFFERS_OUT	3		/* Number of sound card buffers */
#endif
#define	NUM_IN_OUT_CHANNELS		2		/* Stereo */
#define	BITS_PER_SAMPLE			16		/* Use all bits of the D/A-converter */
#define BYTES_PER_SAMPLE		2		/* Number of bytes per sample */

/* Classes ********************************************************************/
class CSoundIn : public CSoundInInterface
{
    public:
    CSoundIn();
virtual ~CSoundIn();

virtual _BOOLEAN	Init(int iSampleRate, int iNewBufferSize, _BOOLEAN bNewBlocking);
virtual _BOOLEAN	Read(CVector<short>& psData);
virtual void		Enumerate(vector<string>& names, vector<string>& descriptions);
virtual string		GetDev();
virtual void		SetDev(string sNewDev);
virtual void		Close();

protected:
void		OpenDevice();
void		PrepareBuffer(int iBufNum);
void		AddBuffer();

vector<string>	vecstrDevices;
string			sCurDev;
WAVEFORMATEX	sWaveFormatEx;
_BOOLEAN		bChangDev;
HANDLE			m_WaveEvent;
int			iSampleRate;
int			iBufferSize;
int			iWhichBuffer;
_BOOLEAN		bBlocking;

/* Wave in */
WAVEINCAPS		m_WaveInDevCaps;
HWAVEIN			m_WaveIn;
WAVEHDR			m_WaveInHeader[NUM_SOUND_BUFFERS_IN];
short*			psSoundcardBuffer[NUM_SOUND_BUFFERS_IN];

};

class CSoundOut : public CSoundOutInterface
{
    public:
    CSoundOut();
virtual ~CSoundOut();

virtual _BOOLEAN	Init(int iSampleRate, int iNewBufferSize, _BOOLEAN bNewBlocking);
virtual _BOOLEAN	Write(CVector<short>& psData);
virtual void		Enumerate(vector<string>& names, vector<string>& descriptions);
virtual string		GetDev();
virtual void		SetDev(string sNewDev);
virtual void		Close();

protected:
void		OpenDevice();
void		PrepareBuffer(int iBufNum);
void		AddBuffer(int iBufNum);
void		GetDoneBuffer(int& iCntPrepBuf, int& iIndexDoneBuf);

vector<string>	vecstrDevices;
string			sCurDev;
WAVEFORMATEX	sWaveFormatEx;
_BOOLEAN		bChangDev;
HANDLE			m_WaveEvent;
int			iSampleRate;
int			iBufferSize;
int			iWhichBuffer;
_BOOLEAN		bBlocking;

/* Wave out */
WAVEOUTCAPS		m_WaveOutDevCaps;
HWAVEOUT		m_WaveOut;
short*			psPlaybackBuffer[NUM_SOUND_BUFFERS_OUT];
WAVEHDR			m_WaveOutHeader[NUM_SOUND_BUFFERS_OUT];
};

#endif // !defined(AFX_SOUNDIN_H__9518A621_7F78_11D3_8C0D_EEBF182CF549__INCLUDED_)
