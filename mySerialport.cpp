/* This is a serial port communication demo for Windows platform
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


// mySerialport.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "SerialPort.h"
#include <iostream>
#include <fstream>
#include <string>

int _tmain(int argc, _TCHAR* argv[])
{

	CSerialPort mySerialPort;

	if (!mySerialPort.InitPort(2,115200))
	{
		std::cout << "initPort fail !" << std::endl;
	}
	else
	{
		std::cout << "initPort success !" << std::endl;
	}

	if (!mySerialPort.OpenListenThread())
	{
		std::cout << "OpenListenThread fail !" << std::endl;
	}
	else
	{
		std::cout << "OpenListenThread success !" << std::endl;
	}

	char key = 0;
	std::cin>>key;

	if (!mySerialPort.CloseListenTread())
	{
		std::cout << "CloseListenThread fail !" << std::endl;
	}
	else
	{
		std::cout << "CloseListenThread success !" << std::endl;
	}
	

	while(1);
	return 0;
}

