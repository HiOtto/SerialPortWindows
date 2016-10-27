/* Serial Port Class for Windows
 *
 * https://github.com/HiOtto/SerialPortWindows
 *
 * Copyright (c) 2014 Siqi Liu
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
 
#include "StdAfx.h"
#include "SerialPort.h"
#include <process.h>
#include <iostream>

/** Exit flag */ 
bool CSerialPort::s_bExit = false;
/** Query interval */ 
const UINT SLEEP_TIME_INTERVAL = 5;
/** File **/
std::ofstream fileOut;

CSerialPort::CSerialPort(void)
: m_hListenThread(INVALID_HANDLE_VALUE)
{
	m_hComm = INVALID_HANDLE_VALUE;
	m_hListenThread = INVALID_HANDLE_VALUE;

	InitializeCriticalSection(&m_csCommunicationSync);

}

CSerialPort::~CSerialPort(void)
{
	CloseListenTread();
	ClosePort();
	DeleteCriticalSection(&m_csCommunicationSync);
}

bool CSerialPort::InitPort( UINT portNo /*= 1*/,UINT baud /*= CBR_9600*/,char parity /*= 'N'*/,
						    UINT databits /*= 8*/, UINT stopsbits /*= 1*/,DWORD dwCommEvents /*= EV_RXCHAR*/ )
{

	/** Temporary var, to construct DCS */ 
	char szDCBparam[50];
	sprintf_s(szDCBparam, "baud=%d parity=%c data=%d stop=%d", baud, parity, databits, stopsbits);

 
	if (!openPort(portNo))
	{
		return false;
	}


	EnterCriticalSection(&m_csCommunicationSync);


	BOOL bIsSuccess = TRUE;

    /**  Set the input and output buffer size */
	 
	/*if (bIsSuccess )
	{
		bIsSuccess = SetupComm(m_hComm,10,10);
	}*/

	COMMTIMEOUTS  CommTimeouts;
	CommTimeouts.ReadIntervalTimeout         = 0;
	CommTimeouts.ReadTotalTimeoutMultiplier  = 0;
	CommTimeouts.ReadTotalTimeoutConstant    = 0;
	CommTimeouts.WriteTotalTimeoutMultiplier = 0;
	CommTimeouts.WriteTotalTimeoutConstant   = 0; 
	if ( bIsSuccess)
	{
		bIsSuccess = SetCommTimeouts(m_hComm, &CommTimeouts);
	}

	DCB  dcb;
	if ( bIsSuccess )
	{
		/** Transfer ANSI to UNICODE */
		DWORD dwNum = MultiByteToWideChar (CP_ACP, 0, szDCBparam, -1, NULL, 0);
		wchar_t *pwText = new wchar_t[dwNum] ;
	//	char *pwText = new char[dwNum] ;
	/*	if (!MultiByteToWideChar (CP_ACP, 0, szDCBparam, -1, pwText, dwNum))
		{
			bIsSuccess = TRUE;
		}
		*/
		/** Get current parameter and construct DCB*/ 
		bIsSuccess = GetCommState(m_hComm, &dcb) && BuildCommDCB(szDCBparam, &dcb) ;
		/** Start RTS flow control */ 
		dcb.fRtsControl = RTS_CONTROL_ENABLE; 

		/** Release */ 
		delete [] pwText;
	}

	if ( bIsSuccess )
	{
		/** Use DCB to setup COM state*/ 
		bIsSuccess = SetCommState(m_hComm, &dcb);
	}
		
	/**  Purge COM buffer */
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	
	LeaveCriticalSection(&m_csCommunicationSync);

	return bIsSuccess==TRUE;
}

bool CSerialPort::InitPort( UINT portNo ,const LPDCB& plDCB )
{

	if (!openPort(portNo))
	{
		return false;
	}
	

	EnterCriticalSection(&m_csCommunicationSync);


	if (!SetCommState(m_hComm, plDCB))
	{
		return false;
	}


	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);


	LeaveCriticalSection(&m_csCommunicationSync);

	return true;
}

void CSerialPort::ClosePort()
{
	/** If the COM is opened, then close it */
	if( m_hComm != INVALID_HANDLE_VALUE )
	{
		CloseHandle( m_hComm );
		m_hComm = INVALID_HANDLE_VALUE;
	}
}

bool CSerialPort::openPort( UINT portNo )
{

	EnterCriticalSection(&m_csCommunicationSync);

 
    char szPort[50];
	sprintf_s(szPort, "COM%d", portNo);

	/** Open the specific COM*/ 
	m_hComm = CreateFileA(szPort,		               
						 GENERIC_READ | GENERIC_WRITE,  
						 0,                             
					     NULL,							 
					     OPEN_EXISTING,					 
						 0,    
						 0);    


	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		LeaveCriticalSection(&m_csCommunicationSync);
		return false;
	}

	LeaveCriticalSection(&m_csCommunicationSync);

	return true;
}

bool CSerialPort::OpenListenThread()
{
	/** Check if the thread is opened*/ 
	if (m_hListenThread != INVALID_HANDLE_VALUE)
	{
		return false;
	}

	s_bExit = false;

	UINT threadId;
	/** Open listen thread */ 
	m_hListenThread = (HANDLE)_beginthreadex(NULL, 0, ListenThread, this, 0, &threadId);
	if (!m_hListenThread)
	{
		return false;
	}
	/** Set the thread priority higner than normal thread*/ 
	if (!SetThreadPriority(m_hListenThread, THREAD_PRIORITY_ABOVE_NORMAL))
	{
		return false;
	}

	/** Open File**/
	fileOut.open("test.txt", std::ios::out, _SH_DENYRW);
	if(fileOut) std::cout<<" file open success! "<<std::endl;

	return true;
}

bool CSerialPort::CloseListenTread()
{	
	if (m_hListenThread != INVALID_HANDLE_VALUE)
	{
	
		s_bExit = true;

		Sleep(10);

		/** Set thread handle invalid */ 
		CloseHandle( m_hListenThread );
		m_hListenThread = INVALID_HANDLE_VALUE;
	}

	/** Close File **/
	fileOut.close();

	return true;
}

UINT CSerialPort::GetBytesInCOM()
{
	DWORD dwError = 0;	
	COMSTAT  comstat;   /** Record the device state information */ 
	memset(&comstat, 0, sizeof(COMSTAT));

	UINT BytesInQue = 0;
	/** Clear COM error befor ReadFile and WiteFile */ 
	if ( ClearCommError(m_hComm, &dwError, &comstat) )
	{
		BytesInQue = comstat.cbInQue; /** 获取在输入缓冲区中的字节数 */ 
	}

	return BytesInQue;
}

UINT WINAPI CSerialPort::ListenThread( void* pParam )
{

	CSerialPort *pSerialPort = reinterpret_cast<CSerialPort*>(pParam);

	/** Polling mode */
	while (!pSerialPort->s_bExit) 
	{
		UINT BytesInQue = pSerialPort->GetBytesInCOM();

		if ( BytesInQue == 0 )
		{
			Sleep(SLEEP_TIME_INTERVAL);
			continue;
		}


		char cRecved[42] = {0x00};
		unsigned char i = 0;
		do
		{
			cRecved[i] = 0x00;
			if(pSerialPort->ReadChar(cRecved[i++]) == true)
			{
				std::cout << cRecved ;
			//	fileOut << cRecved;
				continue;
			}
		}while(--BytesInQue);
	}

	return 0;
}

bool CSerialPort::ReadChar( char &cRecved )
{
	BOOL  bResult     = TRUE;
	DWORD BytesRead   = 0;
	if(m_hComm == INVALID_HANDLE_VALUE)
	{
		return false;
	}


	EnterCriticalSection(&m_csCommunicationSync);

	/** Get one byte for the buffer */ 
	bResult = ReadFile(m_hComm, &cRecved, 1, &BytesRead, NULL);
	if ((!bResult))
	{ 

		DWORD dwError = GetLastError();


		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);



		return false;
	}


	LeaveCriticalSection(&m_csCommunicationSync);

	return (BytesRead == 1);

}

bool CSerialPort::WriteData( unsigned char* pData, unsigned int length )
{
	BOOL   bResult     = TRUE;
	DWORD  BytesToSend = 0;
	if(m_hComm == INVALID_HANDLE_VALUE)
	{
		return false;
	}


	EnterCriticalSection(&m_csCommunicationSync);


	bResult = WriteFile(m_hComm, pData, length, &BytesToSend, NULL);
	if (!bResult)  
	{
		DWORD dwError = GetLastError();

		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);

		return false;
	}

s
	LeaveCriticalSection(&m_csCommunicationSync);

	return true;
}


