/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2010
 *
 * Author(s):
 *	Julian Cable
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

#ifndef __FMDIALOG_H
#define __FMDIALOG_H

#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QString>
#include <QMenuBar>
#include <QEvent>
#include <QLayout>
#include <QPalette>
#include <QMenu>
#include <QDialog>
#include <QColorDialog>
#include "ui_FMMainWindow.h"
#include "SoundCardSelMenu.h"
#include "DialogUtil.h"
#include "CWindow.h"
#include "MultColorLED.h"
#include "../DrmReceiver.h"
#include "../util/Vector.h"

/* Classes ********************************************************************/

class FMDialog : public CWindow, public Ui_FMMainWindow
{
	Q_OBJECT

public:
	FMDialog(CDRMReceiver&, CSettings&, CFileMenu*, CSoundCardSelMenu*,
	QWidget* parent = 0);

protected:
	CDRMReceiver&		DRMReceiver;

	QMenuBar*			pMenu;
	QMenu*				pReceiverModeMenu;
	QMenu*				pSettingsMenu;
	QMenu*				pPlotStyleMenu;
	CFileMenu*			pFileMenu;
	CSoundCardSelMenu*	pSoundCardMenu;
	QTimer				Timer;
	QTimer				TimerClose;

	_BOOLEAN		bSysEvalDlgWasVis;
	_BOOLEAN		bMultMedDlgWasVis;
	_BOOLEAN		bLiveSchedDlgWasVis;
	_BOOLEAN		bStationsDlgWasVis;
	_BOOLEAN		bEPGDlgWasVis;
	ERecMode		eReceiverMode;

	void SetStatus(CMultColorLED* LED, ETypeRxStatus state);
	virtual void	eventClose(QCloseEvent* ce);
	virtual void	eventShow(QShowEvent* pEvent);
	virtual void	eventHide(QHideEvent* pEvent);
	virtual void	eventUpdate();
	void			SetService(int iNewServiceID);
	void			AddWhatsThisHelp();
	void			UpdateDisplay();
	void			ClearDisplay();

	QString			GetCodecString(const int iServiceID);
	QString			GetTypeString(const int iServiceID);

	void			SetDisplayColor(const QColor newColor);

public slots:
	void OnTune();
	void OnTimer();
	void OnTimerClose();
	void OnMenuSetDisplayColor();
	void OnSwitchToDRM();
	void OnSwitchToAM();
	void OnHelpAbout() {emit About();}
	void OnWhatsThis();

signals:
	void SwitchMode(int);
	void ViewStationsDlg();
	void ViewLiveScheduleDlg();
	void Closed();
	void About();
};

#endif
