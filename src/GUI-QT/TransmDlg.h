/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
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

#ifndef _TRANSMDLG_H_
#define _TRANSMDLG_H_

#include "ui_TransmDlgbase.h"
#include "CodecParams.h"
#include "DialogUtil.h"
#include "CWindow.h"
#include "../DrmTransmitter.h"
#include "../Parameter.h"
#include <QPushButton>
#include <QString>
#include <QLabel>
#include <QRadioButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QTabWidget>
#include <QComboBox>
#include <QFileInfo>
#include <QStringList>
#include <QMenuBar>
#include <QLayout>
#include <QThread>
#include <QTimer>
#include <QMainWindow>
#include <QMenu>
#include <qwt_thermo.h>


/* Classes ********************************************************************/
/* Thread class for the transmitter */
class CTransmitterThread : public QThread 
{
public:
	CTransmitterThread(CSettings& Settings) : DRMTransmitter(&Settings) {}

	void Stop()
	{
		/* Stop working thread */
		DRMTransmitter.Stop();
	}

	virtual void run()
	{
		try
		{
			/* Call receiver main routine */
			DRMTransmitter.Start();
		}

		catch (CGenErr GenErr)
		{
			ErrorMessage(GenErr.strError);
		}
	}

	CDRMTransmitter	DRMTransmitter;
};


class TransmDialog : public CWindow, public Ui_TransmDlgBase
{
	Q_OBJECT

public:
	TransmDialog(CSettings&, QWidget* parent=0);
	virtual ~TransmDialog();

protected:
	virtual void eventClose(QCloseEvent*);
	void DisableAllControlsForSet();
	void EnableAllControlsForSet();
	void TabWidgetEnableTabs(QTabWidget* tabWidget, bool enable);

	CTransmitterThread	TransThread; /* Working thread object */
	CDRMTransmitter&	DRMTransmitter;
	CAboutDlg			AboutDlg;
	QTimer				Timer;
	QTimer				TimerStop;
	CVector<string>		vecstrTextMessage;
	QMenu*				pSettingsMenu;
	CodecParams*		pCodecDlg;
    CSysTray*           pSysTray;
	QAction*			pActionStartStop;

	_BOOLEAN			bIsStarted;
	int					iIDCurrentText;
	int					iServiceDescr;
	_BOOLEAN			bCloseRequested;
	int					iButtonCodecState;

	void				ShowButtonCodec(_BOOLEAN bShow, int iKey);
	_BOOLEAN			GetMessageText(const int iID);
	void				UpdateMSCProtLevCombo();
	void				EnableTextMessage(const _BOOLEAN bFlag);
	void				EnableAudio(const _BOOLEAN bFlag);
	void				EnableData(const _BOOLEAN bFlag);
	void				AddWhatsThisHelp();

public slots:
	void OnButtonStartStop();
	void OnPushButtonAddText();
	void OnButtonClearAllText();
	void OnPushButtonAddFileName();
	void OnButtonClearAllFileNames();
	void OnButtonCodec();
	void OnToggleCheckBoxHighQualityIQ(bool bState);
	void OnToggleCheckBoxAmplifiedOutput(bool bState);
	void OnToggleCheckBoxEnableData(bool bState);
	void OnToggleCheckBoxEnableAudio(bool bState);
	void OnToggleCheckBoxEnableTextMessage(bool bState);
	void OnToggleCheckBoxRemovePath(bool bState);
	void OnComboBoxMSCInterleaverActivated(int iID);
	void OnComboBoxMSCConstellationActivated(int iID);
	void OnComboBoxSDCConstellationActivated(int iID);
	void OnComboBoxLanguageActivated(int iID);
	void OnComboBoxProgramTypeActivated(int iID);
	void OnComboBoxTextMessageActivated(int iID);
	void OnComboBoxMSCProtLevActivated(int iID);
	void OnRadioRobustnessMode(int iID);
	void OnRadioBandwidth(int iID);
	void OnRadioOutput(int iID);
	void OnRadioCodec(int iID);
	void OnRadioCurrentTime(int iID);
	void OnTextChangedServiceLabel(const QString& strLabel);
	void OnTextChangedServiceID(const QString& strID);
	void OnTextChangedSndCrdIF(const QString& strIF);
	void OnTimer();
	void OnTimerStop();
	void OnSysTrayActivated(QSystemTrayIcon::ActivationReason);
	void OnWhatsThis();
};

#endif // _TRANSMDLG_H_
