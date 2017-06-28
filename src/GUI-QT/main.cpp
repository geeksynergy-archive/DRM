/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Stephane Fillod
 *
 * Description:
 *
 * 11/10/2004 Stephane Fillod
 *	- QT translation
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

#ifdef _WIN32
# include <windows.h>
#endif
#if defined(__unix__) && !defined(__APPLE__)
# include <csignal>
#endif

#include "../GlobalDefinitions.h"
#include "../DrmReceiver.h"
#include "../DrmTransmitter.h"
#include "../DrmSimulation.h"
#include "../util/Settings.h"
#include <iostream>

#ifdef QT_CORE_LIB
# ifdef QT_GUI_LIB
#  include "fdrmdialog.h"
#  include "TransmDlg.h"
#  include "DialogUtil.h"
#  ifdef HAVE_LIBHAMLIB
#   include "../util-QT/Rig.h"
#  endif
#  include <QApplication>
#  include <QMessageBox>
# endif
# include <QCoreApplication>
# include <QTranslator>
# include <QThread>

class CRx: public QThread
{
public:
	CRx(CDRMReceiver& nRx):rx(nRx)
	{}
	void run();
private:
	CDRMReceiver& rx;
};

void
CRx::run()
{
    qDebug("Working thread started");
    try
    {
        /* Call receiver main routine */
        rx.Start();
    }
    catch (CGenErr GenErr)
    {
        ErrorMessage(GenErr.strError);
    }
    catch (string strError)
    {
        ErrorMessage(strError);
    }
    qDebug("Working thread complete");
}
#endif

#ifdef USE_OPENSL
# include <SLES/OpenSLES.h>
SLObjectItf engineObject = NULL;
#endif

#ifdef QT_GUI_LIB
/******************************************************************************\
* Using GUI with QT                                                            *
\******************************************************************************/
int
main(int argc, char **argv)
{
#ifdef USE_OPENSL
    (void)slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    (void)(*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
#endif
#if defined(__unix__) && !defined(__APPLE__)
	/* Prevent signal interaction with popen */
	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGPIPE);
	sigaddset(&sigset, SIGCHLD);
	pthread_sigmask(SIG_BLOCK, &sigset, NULL);
#endif

	/* create app before running Settings.Load to consume platform/QT parameters */
	QApplication app(argc, argv);

#if defined(__APPLE__)
	/* find plugins on MacOs when deployed in a bundle */
	app.addLibraryPath(app.applicationDirPath()+"../PlugIns");
#endif
#ifdef _WIN32
	WSADATA wsaData;
	(void)WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

	/* Load and install multi-language support (if available) */
	QTranslator translator(0);
	if (translator.load("dreamtr"))
		app.installTranslator(&translator);

	CDRMSimulation DRMSimulation;

	/* Call simulation script. If simulation is activated, application is
	   automatically exit in that routine. If in the script no simulation is
	   activated, this function will immediately return */
	DRMSimulation.SimScript();

	CSettings Settings;
	/* Parse arguments and load settings from init-file */
	Settings.Load(argc, argv);

	try
	{
		string mode = Settings.Get("command", "mode", string());
		if (mode == "receive")
		{
			CDRMReceiver DRMReceiver(&Settings);

			/* First, initialize the working thread. This should be done in an extra
			   routine since we cannot 100% assume that the working thread is
			   ready before the GUI thread */

#ifdef HAVE_LIBHAMLIB
			CRig rig(DRMReceiver.GetParameters());
			rig.LoadSettings(Settings); // must be before DRMReceiver for G313
#endif
			DRMReceiver.LoadSettings();

#ifdef HAVE_LIBHAMLIB
			DRMReceiver.SetRig(&rig);

			if(DRMReceiver.GetDownstreamRSCIOutEnabled())
			{
				rig.subscribe();
			}
			FDRMDialog *pMainDlg = new FDRMDialog(DRMReceiver, Settings, rig);
#else
			FDRMDialog *pMainDlg = new FDRMDialog(DRMReceiver, Settings);
#endif
			(void)pMainDlg;

			/* Start working thread */
			CRx rx(DRMReceiver);
			rx.start();

			/* Set main window */
			app.exec();

#ifdef HAVE_LIBHAMLIB
			if(DRMReceiver.GetDownstreamRSCIOutEnabled())
			{
				rig.unsubscribe();
			}
			rig.SaveSettings(Settings);
#endif
			DRMReceiver.SaveSettings();
		}
		else if(mode == "transmit")
		{
			TransmDialog* pMainDlg = new TransmDialog(Settings);

			/* Show dialog */
			pMainDlg->show();
			app.exec();
		}
		else
		{
			CHelpUsage HelpUsage(Settings.UsageArguments(), argv[0]);
			app.exec();
			exit(0);
		}
	}

	catch(CGenErr GenErr)
	{
		ErrorMessage(GenErr.strError);
	}
	catch(string strError)
	{
		ErrorMessage(strError);
	}
	catch(char *Error)
	{
		ErrorMessage(Error);
	}

	/* Save settings to init-file */
	Settings.Save();

	return 0;
}

/* Implementation of global functions *****************************************/

void
ErrorMessage(string strErrorString)
{
	/* Workaround for the QT problem */
	string strError = "The following error occured:\n";
	strError += strErrorString.c_str();
	strError += "\n\nThe application will exit now.";

#ifdef _WIN32
	MessageBoxA(NULL, strError.c_str(), "Dream",
			   MB_SYSTEMMODAL | MB_OK | MB_ICONEXCLAMATION);
#else
	perror(strError.c_str());
#endif

/*
// Does not work correctly. If it is called by a different thread, the
// application hangs! FIXME
	QMessageBox::critical(0, "Dream",
		QString("The following error occured:<br><b>") +
		QString(strErrorString.c_str()) +
		"</b><br><br>The application will exit now.");
*/
	exit(1);
}
#else /* QT_GUI_LIB */
/******************************************************************************\
* No GUI                                                                       *
\******************************************************************************/

int
main(int argc, char **argv)
{
#ifdef USE_OPENSL
    (void)slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    (void)(*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
#endif
    try
	{
		CSettings Settings;
		Settings.Load(argc, argv);

		string mode = Settings.Get("command", "mode", string());
		if (mode == "receive")
		{
			CDRMSimulation DRMSimulation;
			CDRMReceiver DRMReceiver(&Settings);

			DRMSimulation.SimScript();
			DRMReceiver.LoadSettings();

#ifdef _WIN32
	WSADATA wsaData;
	(void)WSAStartup(MAKEWORD(2,2), &wsaData);
#endif
#ifdef QT_CORE_LIB
			QCoreApplication app(argc, argv);
			/* Start working thread */
			CRx rx(DRMReceiver);
			rx.start();
			return app.exec();
#else
			DRMReceiver.Start();
#endif
		}
		else if (mode == "transmit")
		{
			CDRMTransmitter DRMTransmitter(&Settings);
			DRMTransmitter.LoadSettings();
			DRMTransmitter.Start();
		}
		else
		{
			string usage(Settings.UsageArguments());
			for (;;)
			{
				size_t pos = usage.find("$EXECNAME");
				if (pos == string::npos) break;
				usage.replace(pos, sizeof("$EXECNAME")-1, argv[0]);
			}
			cerr << usage << endl << endl;
			exit(0);
		}
	}
	catch(CGenErr GenErr)
	{
		ErrorMessage(GenErr.strError);
	}

	return 0;
}

void
ErrorMessage(string strErrorString)
{
	perror(strErrorString.c_str());
}
#endif /* QT_GUI_LIB */

void
DebugError(const char *pchErDescr, const char *pchPar1Descr,
		   const double dPar1, const char *pchPar2Descr, const double dPar2)
{
	FILE *pFile = fopen("test/DebugError.dat", "a");
	fprintf(pFile, "%s", pchErDescr);
	fprintf(pFile, " ### ");
	fprintf(pFile, "%s", pchPar1Descr);
	fprintf(pFile, ": ");
	fprintf(pFile, "%e ### ", dPar1);
	fprintf(pFile, "%s", pchPar2Descr);
	fprintf(pFile, ": ");
	fprintf(pFile, "%e\n", dPar2);
	fclose(pFile);
	fprintf(stderr, "\nDebug error! For more information see test/DebugError.dat\n");
	exit(1);
}
