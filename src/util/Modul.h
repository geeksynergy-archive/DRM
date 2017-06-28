/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	High level class for all modules. The common functionality for reading
 *	and writing the transfer-buffers are implemented here.
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

#if !defined(AFX_MODUL_H__41E39CD3_2AEC_400E_907B_148C0EC17A43__INCLUDED_)
#define AFX_MODUL_H__41E39CD3_2AEC_400E_907B_148C0EC17A43__INCLUDED_

#include "Buffer.h"
#include "Vector.h"
#include "../Parameter.h"
#include <iostream>


/* Classes ********************************************************************/
/* CModul ------------------------------------------------------------------- */
template<class TInput, class TOutput>
class CModul
{
public:
	CModul();
	virtual ~CModul() {}

	virtual void Init(CParameter& Parameter);
	virtual void Init(CParameter& Parameter, CBuffer<TOutput>& OutputBuffer);

protected:
	CVectorEx<TInput>*	pvecInputData;
	CVectorEx<TOutput>*	pvecOutputData;

	/* Max block-size are used to determine the size of the required buffer */
	int					iMaxOutputBlockSize;
	/* Actual read (or written) size of the data */
	int					iInputBlockSize;
	int					iOutputBlockSize;

	void				Lock() {Mutex.Lock();}
	void				Unlock() {Mutex.Unlock();}

	void				InitThreadSave(CParameter& Parameter);
	virtual void		InitInternal(CParameter& Parameter) = 0;
	void				ProcessDataThreadSave(CParameter& Parameter);
	virtual void		ProcessDataInternal(CParameter& Parameter) = 0;

private:
	CMutex				Mutex;
};


/* CTransmitterModul -------------------------------------------------------- */
template<class TInput, class TOutput>
class CTransmitterModul : public CModul<TInput, TOutput>
{
public:
	CTransmitterModul();
	virtual ~CTransmitterModul() {}

	virtual void		Init(CParameter& Parameter);
	virtual void		Init(CParameter& Parameter,
							 CBuffer<TOutput>& OutputBuffer);
	virtual void		ReadData(CParameter& Parameter,
								 CBuffer<TOutput>& OutputBuffer);
	virtual _BOOLEAN	ProcessData(CParameter& Parameter,
									CBuffer<TInput>& InputBuffer,
									CBuffer<TOutput>& OutputBuffer);
	virtual void		ProcessData(CParameter& Parameter,
									CBuffer<TInput>& InputBuffer,
									CBuffer<TInput>& InputBuffer2,
									CBuffer<TInput>& InputBuffer3,
									CBuffer<TOutput>& OutputBuffer);
	virtual _BOOLEAN	WriteData(CParameter& Parameter,
								  CBuffer<TInput>& InputBuffer);

protected:
	/* Additional buffers if the derived class has multiple input streams */
	CVectorEx<TInput>*	pvecInputData2;
	CVectorEx<TInput>*	pvecInputData3;

	/* Actual read (or written) size of the data */
	int					iInputBlockSize2;
	int					iInputBlockSize3;
};


/* CReceiverModul ----------------------------------------------------------- */
template<class TInput, class TOutput>
class CReceiverModul : public CModul<TInput, TOutput>
{
public:
	CReceiverModul();
	virtual ~CReceiverModul() {}

	void				SetInitFlag() {bDoInit = TRUE;}
	virtual void		Init(CParameter& Parameter);
	virtual void		Init(CParameter& Parameter,
							 CBuffer<TOutput>& OutputBuffer);
	virtual void		Init(CParameter& Parameter,
							 CBuffer<TOutput>& OutputBuffer,
							 CBuffer<TOutput>& OutputBuffer2);
	virtual void		Init(CParameter& Parameter,
							 CBuffer<TOutput>& OutputBuffer,
							 CBuffer<TOutput>& OutputBuffer2,
							 CBuffer<TOutput>& OutputBuffer3);
	virtual void		Init(CParameter& Parameter,
							 CBuffer<TOutput>& OutputBuffer,
							 CBuffer<TOutput>& OutputBuffer2,
							 vector< CSingleBuffer<TOutput> >& vecOutputBuffer);
	virtual void		Init(CParameter& Parameter,
							 vector< CSingleBuffer<TOutput> >& vecOutputBuffer);
	virtual void		ReadData(CParameter& Parameter,
								 CBuffer<TOutput>& OutputBuffer);
	virtual _BOOLEAN	ProcessData(CParameter& Parameter,
									CBuffer<TInput>& InputBuffer,
									CBuffer<TOutput>& OutputBuffer);
	virtual _BOOLEAN	ProcessData(CParameter& Parameter,
									CBuffer<TInput>& InputBuffer,
									CBuffer<TOutput>& OutputBuffer,
									CBuffer<TOutput>& OutputBuffer2);
	virtual _BOOLEAN	ProcessData(CParameter& Parameter,
									CBuffer<TInput>& InputBuffer,
									CBuffer<TOutput>& OutputBuffer,
									CBuffer<TOutput>& OutputBuffer2,
									CBuffer<TOutput>& OutputBuffer3);
	virtual _BOOLEAN	ProcessData(CParameter& Parameter,
									CBuffer<TInput>& InputBuffer,
									CBuffer<TOutput>& OutputBuffer,
									CBuffer<TOutput>& OutputBuffer2,
									vector< CSingleBuffer<TOutput> >& vecOutputBuffer);
	virtual _BOOLEAN	ProcessData(CParameter& Parameter,
									CBuffer<TInput>& InputBuffer,
									vector< CSingleBuffer<TOutput> >& vecOutputBuffer);
	virtual _BOOLEAN	WriteData(CParameter& Parameter,
								  CBuffer<TInput>& InputBuffer);

protected:
	void SetBufReset1() {bResetBuf = TRUE;}
	void SetBufReset2() {bResetBuf2 = TRUE;}
	void SetBufReset3() {bResetBuf3 = TRUE;}
	void SetBufResetN() {for(size_t i=0; i<vecbResetBuf.size(); i++)
     vecbResetBuf[i] = TRUE;}

	/* Additional buffers if the derived class has multiple output streams */
	CVectorEx<TOutput>*	pvecOutputData2;
	CVectorEx<TOutput>*	pvecOutputData3;
	vector<CVectorEx<TOutput>*>	vecpvecOutputData;

	/* Max block-size are used to determine the size of the required buffer */
	int					iMaxOutputBlockSize2;
	int					iMaxOutputBlockSize3;
	vector<int>			veciMaxOutputBlockSize;
	/* Actual read (or written) size of the data */
	int					iOutputBlockSize2;
	int					iOutputBlockSize3;
	vector<int>			veciOutputBlockSize;

private:
	/* Init flag */
	_BOOLEAN			bDoInit;

	/* Reset flags for output cyclic-buffers */
	_BOOLEAN			bResetBuf;
	_BOOLEAN			bResetBuf2;
	_BOOLEAN			bResetBuf3;
	vector<_BOOLEAN>	vecbResetBuf;
};


/* CSimulationModul --------------------------------------------------------- */
template<class TInput, class TOutput, class TInOut2>
class CSimulationModul : public CModul<TInput, TOutput>
{
public:
	CSimulationModul();
	virtual ~CSimulationModul() {}

	virtual void		Init(CParameter& Parameter);
	virtual void		Init(CParameter& Parameter,
							 CBuffer<TOutput>& OutputBuffer);
	virtual void		Init(CParameter& Parameter,
							 CBuffer<TOutput>& OutputBuffer,
							 CBuffer<TInOut2>& OutputBuffer2);
	virtual void		TransferData(CParameter& Parameter,
									 CBuffer<TInput>& InputBuffer,
									 CBuffer<TOutput>& OutputBuffer);


// TEST "ProcessDataIn" "ProcessDataOut"
	virtual _BOOLEAN	ProcessDataIn(CParameter& Parameter,
									  CBuffer<TInput>& InputBuffer,
									  CBuffer<TInOut2>& InputBuffer2,
									  CBuffer<TOutput>& OutputBuffer);
	virtual _BOOLEAN	ProcessDataOut(CParameter& Parameter,
									   CBuffer<TInput>& InputBuffer,
									   CBuffer<TOutput>& OutputBuffer,
									   CBuffer<TInOut2>& OutputBuffer2);


protected:
	/* Additional buffers if the derived class has multiple output streams */
	CVectorEx<TInOut2>*	pvecOutputData2;

	/* Max block-size are used to determine the size of the requiered buffer */
	int					iMaxOutputBlockSize2;
	/* Actual read (or written) size of the data */
	int					iOutputBlockSize2;

	/* Additional buffers if the derived class has multiple input streams */
	CVectorEx<TInOut2>*	pvecInputData2;

	/* Actual read (or written) size of the data */
	int					iInputBlockSize2;
};


/* Implementation *************************************************************/
/******************************************************************************\
* CModul                                                                       *
\******************************************************************************/
template<class TInput, class TOutput>
CModul<TInput, TOutput>::CModul()
{
	/* Initialize everything with zeros */
	iMaxOutputBlockSize = 0;
	iInputBlockSize = 0;
	iOutputBlockSize = 0;
	pvecInputData = NULL;
	pvecOutputData = NULL;
}

template<class TInput, class TOutput>
void CModul<TInput, TOutput>::ProcessDataThreadSave(CParameter& Parameter)
{
	/* Get a lock for the resources */
	Lock();

	/* Call processing routine of derived modul */
	ProcessDataInternal(Parameter);

	/* Unlock resources */
	Unlock();
}

template<class TInput, class TOutput>
void CModul<TInput, TOutput>::InitThreadSave(CParameter& Parameter)
{
	/* Get a lock for the resources */
	Lock();

	try
	{
		/* Call init of derived modul */
		InitInternal(Parameter);

		/* Unlock resources */
		Unlock();
	}

	catch (CGenErr)
	{
		/* Unlock resources */
		Unlock();

		/* Throws the same error again which was send by the function */
		throw;
	}
}

template<class TInput, class TOutput>
void CModul<TInput, TOutput>::Init(CParameter& Parameter)
{
	/* Init some internal variables */
	iInputBlockSize = 0;

	/* Call init of derived modul */
	InitThreadSave(Parameter);
}

template<class TInput, class TOutput>
void CModul<TInput, TOutput>::Init(CParameter& Parameter,
								   CBuffer<TOutput>& OutputBuffer)
{
	/* Init some internal variables */
	iMaxOutputBlockSize = 0;
	iInputBlockSize = 0;
	iOutputBlockSize = 0;

	/* Call init of derived modul */
	InitThreadSave(Parameter);

	/* Init output transfer buffer */
	if (iMaxOutputBlockSize != 0)
		OutputBuffer.Init(iMaxOutputBlockSize);
	else
	{
		if (iOutputBlockSize != 0)
			OutputBuffer.Init(iOutputBlockSize);
	}
}


/******************************************************************************\
* Transmitter modul (CTransmitterModul)                                        *
\******************************************************************************/
template<class TInput, class TOutput>
CTransmitterModul<TInput, TOutput>::CTransmitterModul()
{
	/* Initialize all member variables with zeros */
	iInputBlockSize2 = 0;
	iInputBlockSize3 = 0;
	pvecInputData2 = NULL;
	pvecInputData3 = NULL;
}

template<class TInput, class TOutput>
void CTransmitterModul<TInput, TOutput>::Init(CParameter& Parameter)
{
	/* Init some internal variables */
	iInputBlockSize2 = 0;
	iInputBlockSize3 = 0;

	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter);
}

template<class TInput, class TOutput>
void CTransmitterModul<TInput, TOutput>::Init(CParameter& Parameter,
											  CBuffer<TOutput>& OutputBuffer)
{
	/* Init some internal variables */
	iInputBlockSize2 = 0;
	iInputBlockSize3 = 0;

	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter, OutputBuffer);
}

template<class TInput, class TOutput>
_BOOLEAN CTransmitterModul<TInput, TOutput>::
	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer,
				CBuffer<TOutput>& OutputBuffer)
{
	/* OUTPUT-DRIVEN modul implementation in the transmitter ---------------- */
	/* Look in output buffer if data is requested */
	if (OutputBuffer.GetRequestFlag() == TRUE)
	{
		/* Check, if enough input data is available */
		if (InputBuffer.GetFillLevel() < this->iInputBlockSize)
		{
			/* Set request flag */
			InputBuffer.SetRequestFlag(TRUE);

			return FALSE;
		}

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);

		/* Query vector from output transfer-buffer for writing */
		this->pvecOutputData = OutputBuffer.QueryWriteBuffer();

		/* Copy extended data from vectors */
		(*(this->pvecOutputData)).
			SetExData((*(this->pvecInputData)).GetExData());

		/* Call the underlying processing-routine */
		this->ProcessDataInternal(Parameter);

		/* Write processed data from internal memory in transfer-buffer */
		OutputBuffer.Put(this->iOutputBlockSize);

		/* Data was provided, clear data request */
		OutputBuffer.SetRequestFlag(FALSE);
	}

	return TRUE;
}

template<class TInput, class TOutput>
void CTransmitterModul<TInput, TOutput>::
	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer,
				CBuffer<TInput>& InputBuffer2,
				CBuffer<TInput>& InputBuffer3,
				CBuffer<TOutput>& OutputBuffer)
{
	/* OUTPUT-DRIVEN modul implementation in the transmitter ---------------- */
	/* Look in output buffer if data is requested */
	if (OutputBuffer.GetRequestFlag() == TRUE)
	{
		/* Check, if enough input data is available from all sources */
		if (InputBuffer.GetFillLevel() < this->iInputBlockSize)
		{
			/* Set request flag */
			InputBuffer.SetRequestFlag(TRUE);

			return;
		}
		if (InputBuffer2.GetFillLevel() < iInputBlockSize2)
		{
			/* Set request flag */
			InputBuffer2.SetRequestFlag(TRUE);

			return;
		}
		if (InputBuffer3.GetFillLevel() < iInputBlockSize3)
		{
			/* Set request flag */
			InputBuffer3.SetRequestFlag(TRUE);

			return;
		}

		/* Get vectors from transfer-buffers */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);
		pvecInputData2 = InputBuffer2.Get(iInputBlockSize2);
		pvecInputData3 = InputBuffer3.Get(iInputBlockSize3);

		/* Query vector from output transfer-buffer for writing */
		this->pvecOutputData = OutputBuffer.QueryWriteBuffer();

		/* Call the underlying processing-routine */
		this->ProcessDataInternal(Parameter);

		/* Write processed data from internal memory in transfer-buffer */
		OutputBuffer.Put(this->iOutputBlockSize);

		/* Data was provided, clear data request */
		OutputBuffer.SetRequestFlag(FALSE);
	}
}

template<class TInput, class TOutput>
void CTransmitterModul<TInput, TOutput>::
	ReadData(CParameter& Parameter, CBuffer<TOutput>& OutputBuffer)
{
	/* OUTPUT-DRIVEN modul implementation in the transmitter ---------------- */
	/* Look in output buffer if data is requested */
	if (OutputBuffer.GetRequestFlag() == TRUE)
	{
		/* Read data and write it in the transfer-buffer.
		   Query vector from output transfer-buffer for writing */
		this->pvecOutputData = OutputBuffer.QueryWriteBuffer();

		/* Call the underlying processing-routine */
		this->ProcessDataInternal(Parameter);

		/* Write processed data from internal memory in transfer-buffer */
		OutputBuffer.Put(this->iOutputBlockSize);

		/* Data was provided, clear data request */
		OutputBuffer.SetRequestFlag(FALSE);
	}
}

template<class TInput, class TOutput>
_BOOLEAN CTransmitterModul<TInput, TOutput>::
	WriteData(CParameter& Parameter, CBuffer<TInput>& InputBuffer)
{
	/* OUTPUT-DRIVEN modul implementation in the transmitter */
	/* Check, if enough input data is available */
	if (InputBuffer.GetFillLevel() < this->iInputBlockSize)
	{
		/* Set request flag */
		InputBuffer.SetRequestFlag(TRUE);

		return FALSE;
	}

	/* Get vector from transfer-buffer */
	this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);

	/* Call the underlying processing-routine */
	this->ProcessDataInternal(Parameter);

	return TRUE;
}


/******************************************************************************\
* Receiver modul (CReceiverModul)                                              *
\******************************************************************************/
template<class TInput, class TOutput>
CReceiverModul<TInput, TOutput>::CReceiverModul()
{
	/* Initialize all member variables with zeros */
	iMaxOutputBlockSize2 = 0;
	iMaxOutputBlockSize3 = 0;
	iOutputBlockSize2 = 0;
	iOutputBlockSize3 = 0;
	pvecOutputData2 = NULL;
	pvecOutputData3 = NULL;
	bResetBuf = FALSE;
	bResetBuf2 = FALSE;
	bResetBuf3 = FALSE;
	bDoInit = FALSE;
}

template<class TInput, class TOutput> void
CReceiverModul<TInput, TOutput>::Init(CParameter& Parameter)
{
	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter);
}

template<class TInput, class TOutput> void
CReceiverModul<TInput, TOutput>::Init(CParameter& Parameter,
									  CBuffer<TOutput>& OutputBuffer)
{
	/* Init flag */
	bResetBuf = FALSE;

	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter, OutputBuffer);
}

template<class TInput, class TOutput> void
CReceiverModul<TInput, TOutput>::Init(CParameter& Parameter,
									  CBuffer<TOutput>& OutputBuffer,
									  CBuffer<TOutput>& OutputBuffer2)
{
	/* Init some internal variables */
	iMaxOutputBlockSize2 = 0;
	iOutputBlockSize2 = 0;
	bResetBuf = FALSE;
	bResetBuf2 = FALSE;

	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter, OutputBuffer);

	/* Init output transfer buffers */
	if (iMaxOutputBlockSize2 != 0)
		OutputBuffer2.Init(iMaxOutputBlockSize2);
	else
	{
		if (iOutputBlockSize2 != 0)
			OutputBuffer2.Init(iOutputBlockSize2);
	}
}

template<class TInput, class TOutput> void
CReceiverModul<TInput, TOutput>::Init(CParameter& Parameter,
									  CBuffer<TOutput>& OutputBuffer,
									  CBuffer<TOutput>& OutputBuffer2,
									  CBuffer<TOutput>& OutputBuffer3)
{
	/* Init some internal variables */
	iMaxOutputBlockSize2 = 0;
	iMaxOutputBlockSize3 = 0;
	iOutputBlockSize2 = 0;
	iOutputBlockSize3 = 0;
	bResetBuf = FALSE;
	bResetBuf2 = FALSE;
	bResetBuf3 = FALSE;

	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter, OutputBuffer);

	/* Init output transfer buffers */
	if (iMaxOutputBlockSize2 != 0)
		OutputBuffer2.Init(iMaxOutputBlockSize2);
	else
	{
		if (iOutputBlockSize2 != 0)
			OutputBuffer2.Init(iOutputBlockSize2);
	}

	if (iMaxOutputBlockSize3 != 0)
		OutputBuffer3.Init(iMaxOutputBlockSize3);
	else
	{
		if (iOutputBlockSize3 != 0)
			OutputBuffer3.Init(iOutputBlockSize3);
	}
}

template<class TInput, class TOutput> void
CReceiverModul<TInput, TOutput>::Init(CParameter& Parameter,
									  CBuffer<TOutput>& OutputBuffer,
									  CBuffer<TOutput>& OutputBuffer2,
									  vector< CSingleBuffer<TOutput> >& vecOutputBuffer)
{
	size_t i;
	/* Init some internal variables */
	iMaxOutputBlockSize2 = 0;
	iMaxOutputBlockSize3 = 0;
	veciMaxOutputBlockSize.resize(vecOutputBuffer.size());
    for(i=0; i<veciMaxOutputBlockSize.size(); i++)
		veciMaxOutputBlockSize[i]=0;
	iOutputBlockSize2 = 0;
	iOutputBlockSize3 = 0;
	veciOutputBlockSize.resize(vecOutputBuffer.size());
    for(i=0; i<veciOutputBlockSize.size(); i++)
		veciOutputBlockSize[i]=0;
	bResetBuf = FALSE;
	bResetBuf2 = FALSE;
	bResetBuf3 = FALSE;
	vecbResetBuf.resize(vecOutputBuffer.size());
    for(i=0; i<vecbResetBuf.size(); i++)
		vecbResetBuf[i]=FALSE;

	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter, OutputBuffer);

	/* Init output transfer buffers */
	if (iMaxOutputBlockSize2 != 0)
		OutputBuffer2.Init(iMaxOutputBlockSize2);
	else
	{
		if (iOutputBlockSize2 != 0)
			OutputBuffer2.Init(iOutputBlockSize2);
	}

    for(i=0; i<veciMaxOutputBlockSize.size(); i++)
    {
		if (veciMaxOutputBlockSize[i] != 0)
			vecOutputBuffer[i].Init(veciMaxOutputBlockSize[i]);
		else
		{
			if (veciOutputBlockSize[i] != 0)
				vecOutputBuffer[i].Init(veciOutputBlockSize[i]);
		}
    }
}

template<class TInput, class TOutput> void
CReceiverModul<TInput, TOutput>::Init(CParameter& Parameter,
					vector< CSingleBuffer<TOutput> >& vecOutputBuffer)
{
	size_t i;
	/* Init some internal variables */
	veciMaxOutputBlockSize.resize(vecOutputBuffer.size());
    for(i=0; i<veciMaxOutputBlockSize.size(); i++)
		veciMaxOutputBlockSize[i]=0;
	veciOutputBlockSize.resize(vecOutputBuffer.size());
    for(i=0; i<veciOutputBlockSize.size(); i++)
		veciOutputBlockSize[i]=0;
	vecbResetBuf.resize(vecOutputBuffer.size());
    for(i=0; i<vecbResetBuf.size(); i++)
		vecbResetBuf[i]=FALSE;

	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter);

	/* Init output transfer buffers */
    for(i=0; i<veciMaxOutputBlockSize.size(); i++)
    {
		if (veciMaxOutputBlockSize[i] != 0)
			vecOutputBuffer[i].Init(veciMaxOutputBlockSize[i]);
		else
		{
			if (veciOutputBlockSize[i] != 0)
				vecOutputBuffer[i].Init(veciOutputBlockSize[i]);
		}
    }
}

template<class TInput, class TOutput>
_BOOLEAN CReceiverModul<TInput, TOutput>::
	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer,
				CBuffer<TOutput>& OutputBuffer)
{
	/* Check initialization flag. The initialization must be done OUTSIDE
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we
	   introduced the flag */
	if (bDoInit == TRUE)
	{
		/* Call init routine */
		Init(Parameter, OutputBuffer);

		/* Reset init flag */
		bDoInit = FALSE;
	}

	/* Special case if input block size is zero */
	if (this->iInputBlockSize == 0)
	{
		InputBuffer.Clear();

		return FALSE;
	}

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* This flag shows, if enough data was in the input buffer for processing */
	_BOOLEAN bEnoughData = FALSE;

	/* Check if enough data is available in the input buffer for processing */
	if (InputBuffer.GetFillLevel() >= this->iInputBlockSize)
	{
		bEnoughData = TRUE;

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);

		/* Query vector from output transfer-buffer for writing */
		this->pvecOutputData = OutputBuffer.QueryWriteBuffer();

		/* Copy extended data from vectors */
		(*(this->pvecOutputData)).
			SetExData((*(this->pvecInputData)).GetExData());

		/* Call the underlying processing-routine */
		this->ProcessDataThreadSave(Parameter);

		/* Reset output-buffers if flag was set by processing routine */
		if (bResetBuf == TRUE)
		{
			/* Reset flag and clear buffer */
			bResetBuf = FALSE;
			OutputBuffer.Clear();
		}
		else
		{
			/* Write processed data from internal memory in transfer-buffer */
			OutputBuffer.Put(this->iOutputBlockSize);
		}
	}

	return bEnoughData;
}

template<class TInput, class TOutput>
_BOOLEAN CReceiverModul<TInput, TOutput>::
	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer,
				CBuffer<TOutput>& OutputBuffer,
				CBuffer<TOutput>& OutputBuffer2)
{
	/* Check initialization flag. The initialization must be done OUTSIDE
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we
	   introduced the flag */
	if (bDoInit == TRUE)
	{
		/* Call init routine */
		Init(Parameter, OutputBuffer, OutputBuffer2);

		/* Reset init flag */
		bDoInit = FALSE;
	}

	/* This flag shows, if enough data was in the input buffer for processing */
	_BOOLEAN bEnoughData = FALSE;

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* Check if enough data is available in the input buffer for processing */
	if (InputBuffer.GetFillLevel() >= this->iInputBlockSize)
	{
		bEnoughData = TRUE;

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);

		/* Query vector from output transfer-buffer for writing */
		this->pvecOutputData = OutputBuffer.QueryWriteBuffer();
		pvecOutputData2 = OutputBuffer2.QueryWriteBuffer();

		/* Call the underlying processing-routine */
		this->ProcessDataThreadSave(Parameter);

		/* Reset output-buffers if flag was set by processing routine */
		if (bResetBuf == TRUE)
		{
			/* Reset flag and clear buffer */
			bResetBuf = FALSE;
			OutputBuffer.Clear();
		}
		else
		{
			/* Write processed data from internal memory in transfer-buffer */
			OutputBuffer.Put(this->iOutputBlockSize);
		}

		if (bResetBuf2 == TRUE)
		{
			/* Reset flag and clear buffer */
			bResetBuf2 = FALSE;
			OutputBuffer2.Clear();
		}
		else
		{
			/* Write processed data from internal memory in transfer-buffer */
			OutputBuffer2.Put(iOutputBlockSize2);
		}
	}

	return bEnoughData;
}

template<class TInput, class TOutput>
_BOOLEAN CReceiverModul<TInput, TOutput>::
	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer,
				CBuffer<TOutput>& OutputBuffer,
				CBuffer<TOutput>& OutputBuffer2,
				CBuffer<TOutput>& OutputBuffer3)
{
	/* Check initialization flag. The initialization must be done OUTSIDE
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we
	   introduced the flag */
	if (bDoInit == TRUE)
	{
		/* Call init routine */
		Init(Parameter, OutputBuffer, OutputBuffer2, OutputBuffer3);

		/* Reset init flag */
		bDoInit = FALSE;
	}

	/* This flag shows, if enough data was in the input buffer for processing */
	_BOOLEAN bEnoughData = FALSE;

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* Check if enough data is available in the input buffer for processing */
	if (InputBuffer.GetFillLevel() >= this->iInputBlockSize)
	{
		bEnoughData = TRUE;

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);

		/* Query vector from output transfer-buffer for writing */
		this->pvecOutputData = OutputBuffer.QueryWriteBuffer();
		pvecOutputData2 = OutputBuffer2.QueryWriteBuffer();
		pvecOutputData3 = OutputBuffer3.QueryWriteBuffer();

		/* Call the underlying processing-routine */
		this->ProcessDataThreadSave(Parameter);

		/* Reset output-buffers if flag was set by processing routine */
		if (bResetBuf == TRUE)
		{
			/* Reset flag and clear buffer */
			bResetBuf = FALSE;
			OutputBuffer.Clear();
		}
		else
		{
			/* Write processed data from internal memory in transfer-buffer */
			OutputBuffer.Put(this->iOutputBlockSize);
		}

		if (bResetBuf2 == TRUE)
		{
			/* Reset flag and clear buffer */
			bResetBuf2 = FALSE;
			OutputBuffer2.Clear();
		}
		else
		{
			/* Write processed data from internal memory in transfer-buffer */
			OutputBuffer2.Put(iOutputBlockSize2);
		}

		if (bResetBuf3 == TRUE)
		{
			/* Reset flag and clear buffer */
			bResetBuf3 = FALSE;
			OutputBuffer3.Clear();
		}
		else
		{
			/* Write processed data from internal memory in transfer-buffer */
			OutputBuffer3.Put(iOutputBlockSize3);
		}
	}

	return bEnoughData;
}

template<class TInput, class TOutput>
_BOOLEAN CReceiverModul<TInput, TOutput>::
	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer,
				CBuffer<TOutput>& OutputBuffer,
				CBuffer<TOutput>& OutputBuffer2,
				vector< CSingleBuffer<TOutput> >& vecOutputBuffer)
{
	/* Check initialization flag. The initialization must be done OUTSIDE
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we
	   introduced the flag */
	if (bDoInit == TRUE)
	{
		/* Call init routine */
		Init(Parameter, OutputBuffer, OutputBuffer2, vecOutputBuffer);

		/* Reset init flag */
		bDoInit = FALSE;
	}

	/* This flag shows, if enough data was in the input buffer for processing */
	_BOOLEAN bEnoughData = FALSE;

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* Check if enough data is available in the input buffer for processing */
	if (InputBuffer.GetFillLevel() >= this->iInputBlockSize)
	{
		size_t i;
		bEnoughData = TRUE;

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);

		/* Query vector from output transfer-buffer for writing */
		this->pvecOutputData = OutputBuffer.QueryWriteBuffer();
		pvecOutputData2 = OutputBuffer2.QueryWriteBuffer();
		vecpvecOutputData.resize(vecOutputBuffer.size());
		for(i=0; i<vecOutputBuffer.size(); i++)
		{
			vecpvecOutputData[i] = vecOutputBuffer[i].QueryWriteBuffer();
		}

		/* Call the underlying processing-routine */
		this->ProcessDataThreadSave(Parameter);

		/* Reset output-buffers if flag was set by processing routine */
		if (bResetBuf == TRUE)
		{
			/* Reset flag and clear buffer */
			bResetBuf = FALSE;
			OutputBuffer.Clear();
		}
		else
		{
			/* Write processed data from internal memory in transfer-buffer */
			OutputBuffer.Put(this->iOutputBlockSize);
		}

		if (bResetBuf2 == TRUE)
		{
			/* Reset flag and clear buffer */
			bResetBuf2 = FALSE;
			OutputBuffer2.Clear();
		}
		else
		{
			/* Write processed data from internal memory in transfer-buffer */
			OutputBuffer2.Put(iOutputBlockSize2);
		}

		for(i=0; i<vecOutputBuffer.size(); i++)
		{
			if (vecbResetBuf[i] == TRUE)
			{
				/* Reset flag and clear buffer */
				vecbResetBuf[i] = FALSE;
				vecOutputBuffer[i].Clear();
			}
			else
			{
				/* Write processed data from internal memory in transfer-buffer */
				vecOutputBuffer[i].Put(veciOutputBlockSize[i]);
			}
		}
	}

	return bEnoughData;
}

template<class TInput, class TOutput>
_BOOLEAN CReceiverModul<TInput, TOutput>::
	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer,
				vector< CSingleBuffer<TOutput> >& vecOutputBuffer)
{
	/* Check initialization flag. The initialization must be done OUTSIDE
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we
	   introduced the flag */
	if (bDoInit == TRUE)
	{
		/* Call init routine */
		Init(Parameter, vecOutputBuffer);

		/* Reset init flag */
		bDoInit = FALSE;
	}

	/* This flag shows, if enough data was in the input buffer for processing */
	_BOOLEAN bEnoughData = FALSE;

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* Check if enough data is available in the input buffer for processing */
	if (InputBuffer.GetFillLevel() >= this->iInputBlockSize)
	{
		size_t i;
		bEnoughData = TRUE;

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);

		/* Query vector from output transfer-buffer for writing */
		vecpvecOutputData.resize(vecOutputBuffer.size());
		for(i=0; i<vecOutputBuffer.size(); i++)
		{
			vecpvecOutputData[i] = vecOutputBuffer[i].QueryWriteBuffer();
		}

		/* Call the underlying processing-routine */
		this->ProcessDataThreadSave(Parameter);

		/* Reset output-buffers if flag was set by processing routine */
		for(i=0; i<vecOutputBuffer.size(); i++)
		{
			if (vecbResetBuf[i] == TRUE)
			{
				/* Reset flag and clear buffer */
				vecbResetBuf[i] = FALSE;
				vecOutputBuffer[i].Clear();
			}
			else
			{
				/* Write processed data from internal memory in transfer-buffer */
				vecOutputBuffer[i].Put(veciOutputBlockSize[i]);
			}
		}
	}

	return bEnoughData;
}

template<class TInput, class TOutput>
void CReceiverModul<TInput, TOutput>::
	ReadData(CParameter& Parameter, CBuffer<TOutput>& OutputBuffer)
{
	/* Check initialization flag. The initialization must be done OUTSIDE
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we
	   introduced the flag */
	if (bDoInit == TRUE)
	{
		/* Call init routine */
		Init(Parameter, OutputBuffer);

		/* Reset init flag */
		bDoInit = FALSE;
	}

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* Query vector from output transfer-buffer for writing */
	this->pvecOutputData = OutputBuffer.QueryWriteBuffer();

	/* Call the underlying processing-routine */
	this->ProcessDataThreadSave(Parameter);

	/* Reset output-buffers if flag was set by processing routine */
	if (bResetBuf == TRUE)
	{
		/* Reset flag and clear buffer */
		bResetBuf = FALSE;
		OutputBuffer.Clear();
	}
	else
	{
		/* Write processed data from internal memory in transfer-buffer */
		OutputBuffer.Put(this->iOutputBlockSize);
	}
}

template<class TInput, class TOutput>
_BOOLEAN CReceiverModul<TInput, TOutput>::
	WriteData(CParameter& Parameter, CBuffer<TInput>& InputBuffer)
{
	/* Check initialization flag. The initialization must be done OUTSIDE
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we
	   introduced the flag */
	if (bDoInit == TRUE)
	{
		/* Call init routine */
		Init(Parameter);

		/* Reset init flag */
		bDoInit = FALSE;
	}

	/* Special case if input block size is zero and buffer, too */
	if ((InputBuffer.GetFillLevel() == 0) && (this->iInputBlockSize == 0))
	{
		InputBuffer.Clear();
		return FALSE;
	}

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* This flag shows, if enough data was in the input buffer for processing */
	_BOOLEAN bEnoughData = FALSE;

	/* Check if enough data is available in the input buffer for processing */
	if (InputBuffer.GetFillLevel() >= this->iInputBlockSize)
	{
		bEnoughData = TRUE;

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);

		/* Call the underlying processing-routine */
		this->ProcessDataThreadSave(Parameter);
	}

	return bEnoughData;
}


/******************************************************************************\
* Simulation modul (CSimulationModul)                                          *
\******************************************************************************/
template<class TInput, class TOutput, class TInOut2>
CSimulationModul<TInput, TOutput, TInOut2>::CSimulationModul()
{
	/* Initialize all member variables with zeros */
	iMaxOutputBlockSize2 = 0;
	iOutputBlockSize2 = 0;
	iInputBlockSize2 = 0;
	pvecOutputData2 = NULL;
	pvecInputData2 = NULL;
}

template<class TInput, class TOutput, class TInOut2>
void CSimulationModul<TInput, TOutput, TInOut2>::Init(CParameter& Parameter)
{
	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter);
}

template<class TInput, class TOutput, class TInOut2>
void CSimulationModul<TInput, TOutput, TInOut2>::
	Init(CParameter& Parameter,
		 CBuffer<TOutput>& OutputBuffer)
{
	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter, OutputBuffer);
}

template<class TInput, class TOutput, class TInOut2>
void CSimulationModul<TInput, TOutput, TInOut2>::
	Init(CParameter& Parameter,
		 CBuffer<TOutput>& OutputBuffer,
		 CBuffer<TInOut2>& OutputBuffer2)
{
	/* Init some internal variables */
	iMaxOutputBlockSize2 = 0;
	iOutputBlockSize2 = 0;

	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter, OutputBuffer);

	/* Init output transfer buffers */
	if (iMaxOutputBlockSize2 != 0)
		OutputBuffer2.Init(iMaxOutputBlockSize2);
	else
	{
		if (iOutputBlockSize2 != 0)
			OutputBuffer2.Init(iOutputBlockSize2);
	}
}

template<class TInput, class TOutput, class TInOut2>
void CSimulationModul<TInput, TOutput, TInOut2>::
	TransferData(CParameter& Parameter,
				 CBuffer<TInput>& InputBuffer,
				 CBuffer<TOutput>& OutputBuffer)
{
	/* TransferData needed for simulation */
	/* Check, if enough input data is available */
	if (InputBuffer.GetFillLevel() < this->iInputBlockSize)
	{
		/* Set request flag */
		InputBuffer.SetRequestFlag(TRUE);

		return;
	}

	/* Get vector from transfer-buffer */
	this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);

	/* Query vector from output transfer-buffer for writing */
	this->pvecOutputData = OutputBuffer.QueryWriteBuffer();

	/* Call the underlying processing-routine */
	this->ProcessDataInternal(Parameter);

	/* Write processed data from internal memory in transfer-buffer */
	OutputBuffer.Put(this->iOutputBlockSize);
}

template<class TInput, class TOutput, class TInOut2>
_BOOLEAN CSimulationModul<TInput, TOutput, TInOut2>::
	ProcessDataIn(CParameter& Parameter,
				  CBuffer<TInput>& InputBuffer,
				  CBuffer<TInOut2>& InputBuffer2,
				  CBuffer<TOutput>& OutputBuffer)
{
	/* This flag shows, if enough data was in the input buffer for processing */
	_BOOLEAN bEnoughData = FALSE;

	/* Check if enough data is available in the input buffer for processing */
	if ((InputBuffer.GetFillLevel() >= this->iInputBlockSize) &&
		(InputBuffer2.GetFillLevel() >= iInputBlockSize2))
	{
		bEnoughData = TRUE;

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);
		pvecInputData2 = InputBuffer2.Get(iInputBlockSize2);

		/* Query vector from output transfer-buffer for writing */
		this->pvecOutputData = OutputBuffer.QueryWriteBuffer();

		/* Copy extended data from FIRST input vector (definition!) */
		(*(this->pvecOutputData)).
			SetExData((*(this->pvecInputData)).GetExData());

		/* Call the underlying processing-routine */
		this->ProcessDataInternal(Parameter);

		/* Write processed data from internal memory in transfer-buffer */
		OutputBuffer.Put(this->iOutputBlockSize);
	}

	return bEnoughData;
}

template<class TInput, class TOutput, class TInOut2>
_BOOLEAN CSimulationModul<TInput, TOutput, TInOut2>::
	ProcessDataOut(CParameter& Parameter,
				   CBuffer<TInput>& InputBuffer,
				   CBuffer<TOutput>& OutputBuffer,
				   CBuffer<TInOut2>& OutputBuffer2)
{
	/* This flag shows, if enough data was in the input buffer for processing */
	_BOOLEAN bEnoughData = FALSE;

	/* Check if enough data is available in the input buffer for processing */
	if (InputBuffer.GetFillLevel() >= this->iInputBlockSize)
	{
		bEnoughData = TRUE;

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);

		/* Query vector from output transfer-buffer for writing */
		this->pvecOutputData = OutputBuffer.QueryWriteBuffer();
		pvecOutputData2 = OutputBuffer2.QueryWriteBuffer();

		/* Call the underlying processing-routine */
		this->ProcessDataInternal(Parameter);

		/* Write processed data from internal memory in transfer-buffers */
		OutputBuffer.Put(this->iOutputBlockSize);
		OutputBuffer2.Put(iOutputBlockSize2);
	}

	return bEnoughData;
}

/* Take an input buffer and split it 2 ways */

template<class TInput>
class CSplitModul: public CReceiverModul<TInput, TInput>
{
protected:
	virtual void SetInputBlockSize(CParameter& Parameters) = 0;

	virtual void InitInternal(CParameter& Parameters)
	{
		this->SetInputBlockSize(Parameters);
		this->iOutputBlockSize = this->iInputBlockSize;
		this->iOutputBlockSize2 = this->iInputBlockSize;
	}

	virtual void ProcessDataInternal(CParameter&)
	{
		for (int i = 0; i < this->iInputBlockSize; i++)
		{
			TInput n = (*(this->pvecInputData))[i];
			(*this->pvecOutputData)[i] = n;
			(*this->pvecOutputData2)[i] = n;
		}
	}
};

#endif // !defined(AFX_MODUL_H__41E39CD3_2AEC_400E_907B_148C0EC17A43__INCLUDED_)
