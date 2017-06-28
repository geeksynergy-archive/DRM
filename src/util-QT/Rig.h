/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2006
 *
 * Author(s):
 *	Volker Fischer, Andrea Russo
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

#ifndef __RIG_H
#define __RIG_H

#include "../Parameter.h"
#include "../util/Hamlib.h"
#include <QObject>
#include <QTimer>

class CRig :
	public QObject
{
	Q_OBJECT
public:
	CRig(CParameter* np);
	void LoadSettings(CSettings&);
	void SaveSettings(CSettings&);
	void SetFrequency(int);
#ifdef HAVE_LIBHAMLIB
	void GetRigList(map<rig_model_t,CHamlib::SDrRigCaps>& r) { Hamlib.GetRigList(r); }
	rig_model_t GetHamlibModelID() { return Hamlib.GetHamlibModelID(); }
	void SetHamlibModelID(rig_model_t r) { Hamlib.SetHamlibModelID(r); }
	void SetEnableModRigSettings(_BOOLEAN b) { Hamlib.SetEnableModRigSettings(b); }
	void GetPortList(map<string,string>& ports) { Hamlib.GetPortList(ports); }
	string GetComPort() { return Hamlib.GetComPort(); }
	void SetComPort(const string& s) { Hamlib.SetComPort(s); }
	_BOOLEAN GetEnableModRigSettings() { return Hamlib.GetEnableModRigSettings(); }
	CHamlib::ESMeterState GetSMeter(_REAL& r) { return Hamlib.GetSMeter(r); }
	CHamlib* GetRig() { return &Hamlib; }

protected:
	CHamlib Hamlib;
	QTimer* timer;
#endif
protected:
	int subscribers;
	CParameter* pParameters;

signals:
    void sigstr(double);
public slots:
	void subscribe();
	void unsubscribe();
	void onTimer();
};

#endif
