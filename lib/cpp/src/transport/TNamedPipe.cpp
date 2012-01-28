/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "TNamedPipe.h"
#include "TTransportException.h"

namespace apache { namespace thrift { namespace transport {

TNamedPipe::TNamedPipe(std::string pipeName)
{
	pipeName_ = pipeName;
}

TNamedPipe::~TNamedPipe(void)
{
	close();
}

bool TNamedPipe::isOpen() 
{
	return hPipe_ != INVALID_HANDLE_VALUE;
}

bool TNamedPipe::peek()
{
	if (!isOpen())
		throw TTransportException(TTransportException::NOT_OPEN, "Called peek on non-open pipe");
	
	if(hPipe_ != INVALID_HANDLE_VALUE)
	{
		DWORD bytesAvailable = 0;
		int peekReturn = PeekNamedPipe(hPipe_, NULL, 0, NULL, &bytesAvailable, NULL); 
		
		return (peekReturn != 0 && bytesAvailable > 0);
	}

	return false;
}

void TNamedPipe::open()
{
	hPipe_ = CreateFile( 
		pipeName_.c_str(),   // pipe name 
		GENERIC_READ |  // read and write access 
		GENERIC_WRITE, 
		0,              // no sharing 
		NULL,           // default security attributes
		OPEN_EXISTING,  // opens existing pipe 
		0,              // default attributes 
		NULL);          // no template file 
	
	if (hPipe_ != INVALID_HANDLE_VALUE) 
		return; 
	
	if (GetLastError() != ERROR_PIPE_BUSY) 
    {
		throw TTransportException(TTransportException::NOT_OPEN, "Unable to open pipe");
	}
	
	// TODO: Implement retry/timeout logic
	//if (!WaitNamedPipe(pipeName_.c_str(), 500)) 
	//{
	//	throw TTransportException(TTransportException::TIMED_OUT, "Unable to open pipe");
	//} 
}
	
void TNamedPipe::close()
{
	if (isOpen())
	{
		CloseHandle(hPipe_); 		
	}

	hPipe_ = INVALID_HANDLE_VALUE;
}
	
uint32_t TNamedPipe::read(uint8_t* buf, uint32_t len)
{
	if (!isOpen())
		throw TTransportException(TTransportException::NOT_OPEN, "Called read on non-open pipe");
	DWORD bytesRead;
	
	BOOL success = ReadFile(
		hPipe_,    // pipe handle 
		buf,    // buffer to receive reply 
		len,  // size of buffer 
		&bytesRead,  // number of bytes read 
		NULL);    // not overlapped 	

	if (!success)
		throw TTransportException(TTransportException::UNKNOWN, "Read not successful");

	return bytesRead;
}
	
void TNamedPipe::write(const uint8_t* buf, uint32_t len)
{
	if (!isOpen())
		throw TTransportException(TTransportException::NOT_OPEN, "Called write on non-open pipe");

	DWORD bytesWritten;
	
	BOOL success = WriteFile( 
      hPipe_,                  // pipe handle 
      buf,             // message 
      len,              // message length 
      &bytesWritten,             // bytes written 
      NULL);                  // not overlapped 
	
	if (!success) 
		throw TTransportException(TTransportException::UNKNOWN, "Write not successful");
}

}}} // apache::thrift::transport