/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Implements:
 *	- Hamlib interface
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

#ifndef __HAMLIB_H
#define __HAMLIB_H

#include "../GlobalDefinitions.h"
#include "Settings.h"
#include <hamlib/rig.h>

class CHamlib
{
public:
	enum ESMeterState {SS_VALID, SS_NOTVALID, SS_TIMEOUT};

	CHamlib();
	virtual ~CHamlib();

	struct SDrRigCaps
	{
		SDrRigCaps() : strManufacturer(""), strModelName(""),
			eRigStatus(RIG_STATUS_ALPHA),bIsSpecRig(FALSE) {}
		SDrRigCaps(const string& strNMan, const string& strNModN, rig_status_e eNSt, _BOOLEAN bNsp) :
			strManufacturer(strNMan), strModelName(strNModN), eRigStatus(eNSt),
			bIsSpecRig(bNsp)
			{}
		SDrRigCaps(const SDrRigCaps& nSDRC) :
			strManufacturer(nSDRC.strManufacturer),
			strModelName(nSDRC.strModelName),
			eRigStatus(nSDRC.eRigStatus), bIsSpecRig(nSDRC.bIsSpecRig) {}

		inline SDrRigCaps& operator=(const SDrRigCaps& cNew)
		{
			strManufacturer = cNew.strManufacturer;
			strModelName = cNew.strModelName;
			eRigStatus = cNew.eRigStatus;
			bIsSpecRig = cNew.bIsSpecRig;
			return *this;
		}

		string			strManufacturer;
		string			strModelName;
		rig_status_e	eRigStatus;
		_BOOLEAN		bIsSpecRig;
	};

	_BOOLEAN		SetFrequency(const int iFreqkHz);
	ESMeterState	GetSMeter(_REAL& rCurSigStr);

	/* backend selection */
	void			GetRigList(map<rig_model_t,SDrRigCaps>&);
	void			SetHamlibModelID(const rig_model_t model);
	rig_model_t		GetHamlibModelID() const {return iHamlibModelID;}

	/* com port selection */
	void			GetPortList(map<string,string>&);
	void			SetComPort(const string&);
	string			GetComPort() const;

	void			SetEnableModRigSettings(const _BOOLEAN bNSM);
	_BOOLEAN		GetEnableModRigSettings() const {return bModRigSettings;}
	string			GetInfo() const;

	void			RigSpecialParameters(rig_model_t id, const string& sSet, int iFrOff, const string& sModSet);
	void			ConfigureRig(const string & strSet);
	void			LoadSettings(CSettings& s);
	void			SaveSettings(CSettings& s);

protected:
	class CSpecDRMRig
	{
	public:
		CSpecDRMRig() : strDRMSetMod(""), strDRMSetNoMod(""), iFreqOffs(0) {}
		CSpecDRMRig(const CSpecDRMRig& nSpec) :
			strDRMSetMod(nSpec.strDRMSetMod),
			strDRMSetNoMod(nSpec.strDRMSetNoMod), iFreqOffs(nSpec.iFreqOffs) {}
		CSpecDRMRig(string sSet, int iNFrOff, string sModSet) :
			strDRMSetMod(sModSet), strDRMSetNoMod(sSet), iFreqOffs(iNFrOff) {}

		string		strDRMSetMod; /* Special DRM settings (modified) */
		string		strDRMSetNoMod; /* Special DRM settings (not mod.) */
		int			iFreqOffs; /* Frequency offset */
	};

	map<rig_model_t,CSpecDRMRig>	SpecDRMRigs;
	map<rig_model_t,SDrRigCaps>		CapsHamlibModels;

	void EnableSMeter(const _BOOLEAN bStatus);

	static int			PrintHamlibModelList(const struct rig_caps* caps, void* data);
	void				SetRigModes();
	void				SetRigLevels();
	void				SetRigFuncs();
	void				SetRigParams();
	void				SetRigConfig();

	RIG*				pRig;
	_BOOLEAN			bSMeterIsSupported;
	_BOOLEAN			bModRigSettings;
	rig_model_t			iHamlibModelID;
	string				strHamlibConf;
	string				strSettings;
	int					iFreqOffset;
	map<string,string> modes;
	map<string,string> levels;
	map<string,string> functions;
	map<string,string> parameters;
	map<string,string> config;

};

#endif
