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

/* A named pipe server on Windows supports multiple clients
   simultaneously and thus can be used with TThreadPoolServer
   and other multi-threaded servers.
   However a *nix named pipe can only support one client at
   a time and therefore is unsuitable for use with multi-
   threaded servers.

   It is up to the caller to specify a valid pipe name.
   On Windows platforms it should look something like
     "\\\\.\\pipe\\MyPipe"
   while on *nix use a path such as
     "/tmp/MyPipe"
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <cstring>

#include "TPipe.h"
#include "TPipeServer.h"
#include <boost/shared_ptr.hpp>


namespace apache { namespace thrift { namespace transport {

using namespace std;
using boost::shared_ptr;

//---- Constructors ----
TPipeServer::TPipeServer(string pipename, uint32_t bufsize) :
  pipename_(pipename),
  bufsize_(bufsize),
  Pipe_(-1),
  isAnonymous(false),
  maxconns_(TPIPE_SERVER_MAX_CONNS_DEFAULT)
 {}

TPipeServer::TPipeServer(string pipename, uint32_t bufsize, byte maxconnections) :
  pipename_(pipename),
  bufsize_(bufsize),
  Pipe_(-1),
  isAnonymous(false),
  maxconns_(maxconnections>0? maxconnections : 1)
 {}

TPipeServer::TPipeServer(string pipename) :
  pipename_(pipename),
  bufsize_(1024),
  Pipe_(-1),
  isAnonymous(false),
  maxconns_(TPIPE_SERVER_MAX_CONNS_DEFAULT)
 {}

TPipeServer::TPipeServer(uint32_t bufsize) : 
  pipename_(""),
  bufsize_(bufsize),
  Pipe_(-1),
  isAnonymous(true),
  maxconns_(1)
 {
  //The anonymous pipe needs to be created first so that the server can
  //pass the handles on to the client before the serve (acceptImpl)
  //blocking call.
  if (!TCreateAnonPipe()) {
    GlobalOutput.perror("TPipeServer Create(Anon)Pipe failed, GLE=", GetLastError());
    throw TTransportException(TTransportException::NOT_OPEN, " TPipeServer Create(Anon)Pipe failed");
  }
}

TPipeServer::TPipeServer() : 
  pipename_(""),
  bufsize_(1024),
  Pipe_(-1),
  isAnonymous(true),
  maxconns_(1)
{
  if (!TCreateAnonPipe()) {
    GlobalOutput.perror("TPipeServer Create(Anon)Pipe failed, GLE=", GetLastError());
    throw TTransportException(TTransportException::NOT_OPEN, " TPipeServer Create(Anon)Pipe failed");
  }
}

//---- Destructor ----
TPipeServer::~TPipeServer() {
  close();
}


//---------------------------------------------------------
// Transport callbacks
//---------------------------------------------------------

#ifndef _WIN32  //*nix named pipe

shared_ptr<TTransport> TPipeServer::acceptImpl() {
  shared_ptr<TPipe> client;

  if(isAnonymous)
  { //Anonymous Pipe
    // TODO: Create anon pipe, add blocking read call
  }
  else
  { //Named Pipe
    TCreateNamedPipe();
    // TODO: add blocking read call
  }

  return client;
}

void TPipeServer::interrupt() {
  if(Pipe_ != -1) {
  }
}

void TPipeServer::close() {
  if(!isAnonymous)
  {
    if(Pipe_ != -1) {
    }
  }
  else
  {
  }
}

// Create an instance of the named pipe
bool TPipeServer::TCreateNamedPipe() {
  int retval;

  //"According to POSIX.1-2001, pipes only need to be unidirectional."
  //For better compatibility, create a pair of pipes for both
  //read & write directions (*nix only).
  retval = mkfifo(pipename_.c_str(), O_CREAT | O_RDONLY);
  if ((retval == -1) && (errno != EEXIST))
  {
    int errno_copy = errno;
    GlobalOutput.perror("TPipeServer::TCreateNamedPipe() read pipe ", errno_copy);
    throw TTransportException(TTransportException::NOT_OPEN, "TCreateNamedPipe() read pipe failed", errno_copy);
  }
  retval = mkfifo(pipenameW_.c_str(), O_CREAT | O_WRONLY);
  if ((retval == -1) && (errno != EEXIST))
  {
    int errno_copy = errno;
    GlobalOutput.perror("TPipeServer::TCreateNamedPipe() write pipe ", errno_copy);
    throw TTransportException(TTransportException::NOT_OPEN, "TCreateNamedPipe() write pipe failed", errno_copy);
  }

  return true;
}

#else  //Windows named pipe

shared_ptr<TTransport> TPipeServer::acceptImpl() {
  shared_ptr<TPipe> client;

  if(isAnonymous)
  { //Anonymous Pipe
    //This 0-byte read serves merely as a blocking call.
    byte buf;
    DWORD br;
    int fSuccess = ReadFile( 
          (HANDLE)Pipe_, // pipe handle 
          &buf,   // buffer to receive reply 
          0,      // size of buffer 
          &br,    // number of bytes read 
          NULL);  // not overlapped

    if ( !fSuccess && GetLastError() != ERROR_MORE_DATA ) {
      GlobalOutput.perror("TPipeServer unable to initiate pipe comms, GLE=", GetLastError());
      throw TTransportException(TTransportException::NOT_OPEN, " TPipeServer unable to initiate pipe comms");
    }
	client.reset(new TPipe(Pipe_, PipeW_));
  }
  else
  { //Named Pipe
    int ConnectRet;
    while (true)
    {
      if (!TCreateNamedPipe()) {
        GlobalOutput.perror("TPipeServer CreateNamedPipe failed", -1);
        throw TTransportException(TTransportException::NOT_OPEN, " TPipeServer CreateNamedPipe failed");
      }

      // Wait for the client to connect; if it succeeds, the
      // function returns a nonzero value. If the function returns 
      // zero, GetLastError should return ERROR_PIPE_CONNECTED. 
      ConnectRet = ConnectNamedPipe((HANDLE)Pipe_, NULL) ? 
                    TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

      if (ConnectRet == TRUE)
      {
        GlobalOutput.printf("Client connected.");
        break;
      }
      else
      {
        this->close();
        GlobalOutput.perror("TPipeServer ConnectNamedPipe GLE=", GetLastError());
        throw TTransportException(TTransportException::NOT_OPEN, "TPipeServer: client connection failed");
      }
    }
	client.reset(new TPipe(Pipe_));
  }

  return client;
}

void TPipeServer::interrupt() {
  if(Pipe_ != -1) {
    CancelIo((HANDLE)Pipe_);
  }
}

void TPipeServer::close() {
  if(!isAnonymous)
  {
    if(Pipe_ != -1) {
      CloseHandle((HANDLE)Pipe_);
	  Pipe_ = -1;
    }
  }
  else
  {
    try {
      CloseHandle((HANDLE)Pipe_);
      CloseHandle((HANDLE)PipeW_);
      CloseHandle((HANDLE)ClientAnonRead);
      CloseHandle((HANDLE)ClientAnonWrite);
    }
    catch(...) {
        GlobalOutput.perror("TPipeServer anon close GLE=", GetLastError());
    }
  }
}


// Create an instance of the named pipe
bool TPipeServer::TCreateNamedPipe() {

  //Windows - set security to allow non-elevated apps
  //to access pipes created by elevated apps.
  SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
  PSID everyone_sid = NULL;
  AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &everyone_sid);

  EXPLICIT_ACCESS ea;
  ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
  ea.grfAccessPermissions = SPECIFIC_RIGHTS_ALL | STANDARD_RIGHTS_ALL;
  ea.grfAccessMode = SET_ACCESS;
  ea.grfInheritance = NO_INHERITANCE;
  ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
  ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
  ea.Trustee.ptstrName  = (LPSTR)everyone_sid;

  PACL acl = NULL;
  SetEntriesInAcl(1, &ea, NULL, &acl);

  PSECURITY_DESCRIPTOR sd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR,SECURITY_DESCRIPTOR_MIN_LENGTH);
  InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION);
  SetSecurityDescriptorDacl(sd, TRUE, acl, FALSE);

  SECURITY_ATTRIBUTES sa;
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.lpSecurityDescriptor = sd;
  sa.bInheritHandle = FALSE;

  //Always open pipe in R/W mode.
  HANDLE hPipe_ = CreateNamedPipe( 
        pipename_.c_str(),        // pipe name 
        PIPE_ACCESS_DUPLEX,       // read/write access 
        PIPE_TYPE_BYTE |       // message type pipe 
        PIPE_READMODE_BYTE,    // message-read mode 
        maxconns_,                // max. instances  
        bufsize_,                 // output buffer size 
        bufsize_,                 // input buffer size 
        0,                        // client time-out 
        &sa);                     // security attribute 

  if(hPipe_ == INVALID_HANDLE_VALUE)
  {
    Pipe_ = -1;
    GlobalOutput.perror("TPipeServer::TCreateNamedPipe() GLE=", GetLastError());
    throw TTransportException(TTransportException::NOT_OPEN, "TCreateNamedPipe() failed", GetLastError());
    return false;
  }

  Pipe_ = (int)hPipe_;
  return true;
}

bool TPipeServer::TCreateAnonPipe() {
  SECURITY_ATTRIBUTES sa;
  SECURITY_DESCRIPTOR sd; //security information for pipes

  InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION);
  SetSecurityDescriptorDacl(&sd, true, NULL, false);
  sa.lpSecurityDescriptor = &sd;
  sa.lpSecurityDescriptor = NULL;
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle = true; //allow passing handle to child

  HANDLE ClientAnonReadH, PipeW_H, ClientAnonWriteH, Pipe_H;
  if (!CreatePipe(&ClientAnonReadH,&PipeW_H,&sa,0))   //create stdin pipe
  {
    GlobalOutput.perror("TPipeServer CreatePipe (anon) failed, GLE=", GetLastError());
    return false;
  }
  if (!CreatePipe(&Pipe_H,&ClientAnonWriteH,&sa,0))  //create stdout pipe
  {
    GlobalOutput.perror("TPipeServer CreatePipe (anon) failed, GLE=", GetLastError());
    CloseHandle(ClientAnonReadH);
    CloseHandle(PipeW_H);
    return false;
  }
  ClientAnonRead  = (int)ClientAnonReadH;
  ClientAnonWrite = (int)ClientAnonWriteH;
  Pipe_  = (int)Pipe_H;
  PipeW_ = (int)PipeW_H;

  return true;
}

#endif  //#ifndef _WIN32

//---------------------------------------------------------
// Accessors
//---------------------------------------------------------

string TPipeServer::getPipename() {
  return pipename_;
}

void TPipeServer::setPipename(std::string pipename) {
  pipename_ = pipename;
}

uint32_t  TPipeServer::getBufferSize() {
  return bufsize_;
}

void TPipeServer::setBufferSize(uint32_t bufsize) {
  bufsize_ = bufsize;
}

int TPipeServer::getPipeHandle() {
  return Pipe_;
}

int TPipeServer::getWrtPipeHandle()
{
  return PipeW_;
}

int TPipeServer::getClientRdPipeHandle()
{
  return ClientAnonRead;
}

int TPipeServer::getClientWrtPipeHandle()
{
  return ClientAnonWrite;
}

bool TPipeServer::getAnonymous() {
  return isAnonymous;
}

void TPipeServer::setAnonymous(bool anon) {
  isAnonymous = anon;
}

}}} // apache::thrift::transport

