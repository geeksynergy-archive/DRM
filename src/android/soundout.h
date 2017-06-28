#ifndef SOUNDOUT_H
#define SOUNDOUT_H

#include "../sound/soundinterface.h"

/* Classes ********************************************************************/
class COpenSLESOut : public CSoundOutInterface
{
public:
    COpenSLESOut();
    virtual ~COpenSLESOut();

    virtual void		Enumerate(vector<string>&, vector<string>&);
    virtual void		SetDev(string sNewDevice);
    virtual string		GetDev();
    virtual int			GetSampleRate();

    virtual _BOOLEAN	Init(int iNewSampleRate, int iNewBufferSize, _BOOLEAN bNewBlocking);
    virtual _BOOLEAN 	Write(CVector<short>& psData);
    virtual void 		Close();

protected:
    string currentDevice;
};

#endif // SOUNDOUT_H
