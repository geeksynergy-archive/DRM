#ifndef SOUNDIN_H
#define SOUNDIN_H

#include "../sound/soundinterface.h"

/* Classes ********************************************************************/
class COpenSLESIn : public CSoundInInterface
{
public:
    COpenSLESIn();
    virtual ~COpenSLESIn();

    virtual void		Enumerate(vector<string>&, vector<string>&);
    virtual void		SetDev(string sNewDevice);
    virtual string		GetDev();
    virtual int			GetSampleRate();

    virtual _BOOLEAN	Init(int iNewSampleRate, int iNewBufferSize, _BOOLEAN bNewBlocking);
    virtual _BOOLEAN 	Read(CVector<short>& psData);
    virtual void 		Close();

protected:
    string currentDevice;
};

#endif // SOUNDIN_H
