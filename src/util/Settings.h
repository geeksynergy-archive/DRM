/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2007
 *
 * Author(s):
 *	Volker Fischer, Robert Kesterson, Andrew Murphy
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

#if !defined(SETTINGS_H__3B0BA660_DGEG56GE64B2B_23DSG9876D31912__INCLUDED_)
#define SETTINGS_H__3B0BA660_DGEG56GE64B2B_23DSG9876D31912__INCLUDED_

#include "../GlobalDefinitions.h"
#include <map>
#include <string>


/* Definitions ****************************************************************/
#define DREAM_INIT_FILE_NAME		"Dream.ini"

/* Maximum number of sound devices */
#define MAX_NUM_SND_DEV				10

/* Maximum number of iterations in MLC decoder */
#define MAX_NUM_MLC_IT				4

/* Maximum value of input/output channel selection */
#define MAX_VAL_IN_CHAN_SEL			9
#define MAX_VAL_OUT_CHAN_SEL		4

/* Minimum and maximum of initial sample rate offset parameter */
#define MIN_SAM_OFFS_INI			-200
#define MAX_SAM_OFFS_INI			200

/* Maximum for frequency acqisition search window size and center frequency */
#define MAX_FREQ_AQC_SE_WIN_SZ		(+1e6)
#define MAX_FREQ_AQC_SE_WIN_CT		(+1e6)

/* Maximum carrier frequency  */
# define MAX_RF_FREQ				30000 /* kHz */

#ifdef QT_CORE_LIB
/* Maximum minutes for delayed log file start */
# define MAX_SEC_LOG_FI_START		3600 /* seconds */

/* Maximum value for window position and size */
# define MAX_WIN_GEOM_VAL			10000 /* Pixel */

/* Maximum value for color schemes for main plot */
# define MAX_COLOR_SCHEMES_VAL		(NUM_AVL_COLOR_SCHEMES_PLOT - 1)

# define MAX_MDI_PORT_IN_NUM		65535
#endif

#ifdef HAVE_LIBHAMLIB
/* Maximum ID number for hamlib library */
# define MAX_ID_HAMLIB				32768
#endif

/* max magnitude of front-end cal factor */
#define MAX_CAL_FACTOR				200

/* change this if you expect to have huge lines in your INI files. Note that
   this is the max size of a single line, NOT the max number of lines */
#define MAX_INI_LINE				500

/* Maximum number of chart windows and plot types */
#define MAX_NUM_CHART_WIN_EV_DLG	50
#define MAX_IND_CHART_TYPES			1000

/* Maximum for preview */
#define MAX_NUM_SEC_PREVIEW			3600

/* Maximum value for rgb-colors encoded as integers */
#define MAX_NUM_COL_MAIN_DISP		16777215

/* Maximum for font weight (99 is written into the Qt reference manual) */
#define MAX_FONT_WEIGHT				99

/* Maximum for font point size (256 it is maybe not true but we assume that
   this is a good value */
#define MAX_FONT_POINT_SIZE			256

/* Minimum number of seconds for MOT BWS refresh */
#define MIN_MOT_BWS_REFRESH_TIME	5

/* Miximum number of seconds for MOT BWS refresh */
#define MAX_MOT_BWS_REFRESH_TIME	1800

/* number of available color schemes for the plot */
#define NUM_AVL_COLOR_SCHEMES_PLOT				3

/* Classes ********************************************************************/
	/* Function declarations for stlini code written by Robert Kesterson */
	struct StlIniCompareStringNoCase
	{
		bool operator()(const std::string& x, const std::string& y) const;
	};
	/* These typedefs just make the code a bit more readable */
	typedef std::map<string, string, StlIniCompareStringNoCase > INISection;
	typedef std::map<string, INISection , StlIniCompareStringNoCase > INIFile;

	class CWinGeom
	{
	public:
		CWinGeom() : iXPos(0), iYPos(0), iHSize(0), iWSize(0) {}

		int iXPos, iYPos;
		int iHSize, iWSize;
	};

class CIniFile
{
public:
	CIniFile() {}
	virtual ~CIniFile() {}
	void SaveIni(ostream&) const;
	void SaveIni(const char*) const;
	bool LoadIni(const char*);

	string GetIniSetting(const string& strSection, const string& strKey,
				const string& strDefaultVal = "") const;
	void PutIniSetting(const string& strSection, const string& strKey="",
				const string& strVal = "");
protected:
	INIFile ini;
	CMutex Mutex;
};

class CSettings: public CIniFile
{
public:
	CSettings() {}
	void Load(int argc, char** argv);
	void Save();
	void Clear();
	string Get(const string& section, const string& key, const string& def="") const;
	void Put(const string& section, const string& key, const string& value);
	bool Get(const string& section, const string& key, const bool def) const;
	void Put(const string& section, const string& key, const bool value);
	int Get(const string& section, const string& key, const int def) const;
	void Put(const string& section, const string& key, const int value);
	_REAL Get(const string& section, const string& key, const _REAL def) const;
	void Put(const string& section, const string& key, const _REAL value);
	void Get(const string& section, CWinGeom&) const;
	void Put(const string& section, const CWinGeom&);
	const char* UsageArguments();

protected:
	int IsReceiver(const char *argv0);
	void ParseArguments(int argc, char** argv);
	void FileArg(const string&);
	_BOOLEAN GetFlagArgument(int argc, char** argv, int& i, string strShortOpt,
							 string strLongOpt);
	_BOOLEAN GetNumericArgument(int argc, char** argv, int& i,
								string strShortOpt, string strLongOpt,
								_REAL rRangeStart, _REAL rRangeStop,
								_REAL& rValue);
	_BOOLEAN GetStringArgument(int argc, char** argv, int& i,
							   string strShortOpt, string strLongOpt,
							   string& strArg);
};

#endif // !defined(SETTINGS_H__3B0BA660_DGEG56GE64B2B_23DSG9876D31912__INCLUDED_)
