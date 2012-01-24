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

#include "TTransportException.h"
#include "TPipe.h"
#include <stdio.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

namespace apache { namespace thrift { namespace transport {

using namespace std;

/**
* TPipe implementation.
*/

//---- Constructors ----
TPipe::TPipe(int hpipe) :
  pipename_(""),
  hPipe_(hpipe),
  TimeoutSeconds_(3),
  isAnonymous(false),
  fPipe_(NULL)
{
  // --- This doesn't work on Windows ---
  //int t = _open_osfhandle(hpipe, O_BINARY);
  //fPipe_ = fdopen(t, "w");
  //errno_t z;
  //  _get_errno(&z);
  //  GlobalOutput.perror("Pipe open error ", z);
}

TPipe::TPipe(FILE* pfile) :
  pipename_(""),
  hPipe_(-1),
  TimeoutSeconds_(3),
  isAnonymous(false),
  fPipe_(pfile)
{}

TPipe::TPipe(string pipename) :
  pipename_(pipename),
  hPipe_(-1),
  TimeoutSeconds_(3),
  isAnonymous(false),
  fPipe_(NULL)
{}

TPipe::TPipe(int hPipeRd, int hPipeWrt) :
  pipename_(""),
  hPipe_(hPipeRd),
  hPipeWrt_(hPipeWrt),
  TimeoutSeconds_(3),
  isAnonymous(true)
{}

TPipe::TPipe() :
  pipename_(""),
  hPipe_(-1),
  TimeoutSeconds_(3)
{}

//---- Destructor ----
TPipe::~TPipe() {
  close();
}


bool TPipe::isOpen() {
#ifdef _WIN32
  return (hPipe_ != -1);
#else
  return (fPipe_ != NULL);
#endif
}

//---------------------------------------------------------
// Transport callbacks
//---------------------------------------------------------

bool TPipe::peek() {
  if (!isOpen()) {
    return false;
  }

  if(hPipe_ != -1)
  {
    DWORD bytesavail = 0;
    int  PeekRet = 0;
    PeekRet = PeekNamedPipe((HANDLE)hPipe_, NULL, 0, NULL, &bytesavail, NULL); 
    return (PeekRet != 0 && bytesavail > 0);
  }
  else if(fPipe_ != NULL)
  {
    long sz = ftell (fPipe_);
    return (sz > 0);
  }
}

void TPipe::open() {
  if (isOpen()) {
    return;
  }

  int SleepInterval = 500; //ms
  int retries = TimeoutSeconds_ * 1000 / SleepInterval;
  for(int i=0; i<retries; i++)
  {
    fPipe_ = fopen(pipename_.c_str(), "r+b");
    if (fPipe_ != NULL) {
      break;
    }
    sleep(SleepInterval);
  }
  if (fPipe_ == NULL)
    throw TTransportException(TTransportException::NOT_OPEN, "Unable to open pipe");

#ifdef _WIN32
  //Convert FILE* to (int)HANDLE
  int cPipe = _fileno(fPipe_);
  hPipe_ = _get_osfhandle(cPipe);
#endif
}


void TPipe::close() {
  if (isOpen())
  {
    if(fPipe_ != NULL) {
      fclose(fPipe_);
      fPipe_ = NULL;
    }
#ifdef _WIN32
    if(hPipe_ != -1) {
      CloseHandle((HANDLE)hPipe_);
      hPipe_ = -1;
    }
#endif
  }
}

uint32_t TPipe::read(uint8_t* buf, uint32_t len) {
  if (!isOpen())
    throw TTransportException(TTransportException::NOT_OPEN, "Called read on non-open pipe");

#ifndef _WIN32
  len = ftell (fPipe_);
#endif
  uint32_t bytesread = fread(buf, 1, len, fPipe_);

  return bytesread;
}

void TPipe::write(const uint8_t* buf, uint32_t len) {
  if (!isOpen())
    throw TTransportException(TTransportException::NOT_OPEN, "Called write on non-open pipe");

  size_t byteswritten = fwrite(buf, 1, len, fPipe_);
  if(byteswritten < len)
  {
    GlobalOutput.perror("TPipe write() GLE ", GetLastError()); //?
    throw TTransportException(TTransportException::NOT_OPEN, "Write to pipe failed");
  }
}

//---------------------------------------------------------
// Windows-only R/W implementation code.
// These wrapper functions are required because stdio's 
// fread/fwrite do not work properly on named pipes.
//---------------------------------------------------------
#ifdef _WIN32
size_t TPipe::fread(void * _DstBuf, size_t _ElementSize, size_t _Count, FILE * _File)
{
  DWORD cbRead;
  if(hPipe_ != -1)
  {
    int fSuccess = ReadFile( 
          (HANDLE)hPipe_,        // pipe handle 
          _DstBuf,               // buffer to receive reply 
          _Count * _ElementSize, // size of buffer 
          &cbRead,               // number of bytes read 
          NULL);                 // not overlapped 

    if ( !fSuccess && GetLastError() != ERROR_MORE_DATA )
    cbRead = 0; // No more data, possibly because client disconnected.
  }
  return cbRead;
}

size_t TPipe::fwrite(const void * _Str, size_t _Size, size_t _Count, FILE * _File)
{
  HANDLE WritePipe = isAnonymous? (HANDLE)hPipeWrt_: (HANDLE)hPipe_;
  DWORD  cbWritten;
  int fSuccess = WriteFile( 
        WritePipe,      // pipe handle 
        _Str,           // message 
        _Size * _Count, // message length 
        &cbWritten,     // bytes written 
        NULL);          // not overlapped 
  if ( !fSuccess)
  {
    GlobalOutput.perror("TPipe WriteFile() GLE ", GetLastError());
    throw TTransportException(TTransportException::NOT_OPEN, "Write to pipe failed");
  }
  return cbWritten;
}

#endif

//---------------------------------------------------------
// Accessors
//---------------------------------------------------------

string TPipe::getPipename() {
  return pipename_;
}

void TPipe::setPipename(std::string pipename) {
  pipename_ = pipename;
}

int TPipe::getPipeHandle() {
  return hPipe_;
}

void TPipe::setPipeHandle(int pipehandle) {
  hPipe_ = pipehandle;
}

int TPipe::getWrtPipeHandle() {
  return hPipeWrt_;
}

void TPipe::setWrtPipeHandle(int pipehandle) {
  hPipeWrt_ = pipehandle;
}

long TPipe::getConnectTimeout() {
  return TimeoutSeconds_;
}

void TPipe::setConnectTimeout(long seconds) {
  TimeoutSeconds_ = seconds;
}

}}} // apache::thrift::transport

