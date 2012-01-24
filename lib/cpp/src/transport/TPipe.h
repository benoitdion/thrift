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

#ifndef _THRIFT_TRANSPORT_TPIPE_H_
#define _THRIFT_TRANSPORT_TPIPE_H_ 1

#include "TTransport.h"
#include "TVirtualTransport.h"

namespace apache { namespace thrift { namespace transport {

/**
 * Windows Pipes implementation of the TTransport interface.
 *
 */
class TPipe : public TVirtualTransport<TPipe> {
 public:

  // Constructs a new pipe object.
  TPipe();
  // Named pipe constructors -
  TPipe(int hPipe);
  TPipe(FILE* pfile);
  TPipe(std::string pipename);
  // Anonymous pipe -
  TPipe(int hPipeRd, int hPipeWrt);

  // Destroys the pipe object, closing it if necessary.
  virtual ~TPipe();

  // Returns whether the pipe is open & valid.
  virtual bool isOpen();

  // Checks whether more data is available in the pipe.
  virtual bool peek();

  // Creates and opens the named/anonymous pipe.
  virtual void open();

  // Shuts down communications on the pipe.
  virtual void close();

  // Reads from the pipe.
  virtual uint32_t read(uint8_t* buf, uint32_t len);

  // Writes to the pipe.
  virtual void write(const uint8_t* buf, uint32_t len);

#ifdef _WIN32
  // Windows-specific implementations
  size_t fread(void * _DstBuf, size_t _ElementSize, size_t _Count, FILE * _File);
  size_t fwrite(const void * _Str, size_t _Size, size_t _Count, FILE * _File);
#endif

  //Accessors
  std::string getPipename();
  void setPipename(std::string pipename);
  int getPipeHandle(); //doubles as the read handle for anon pipe
  void setPipeHandle(int pipehandle);
  int getWrtPipeHandle();
  void setWrtPipeHandle(int pipehandle);
  long getConnectTimeout();
  void setConnectTimeout(long seconds);

 private:
  std::string pipename_;
  FILE* fPipe_;
  //Windows named pipe handles are R/W, while anonymous and *nix pipes are one or the other (half duplex).
  int hPipe_, hPipeWrt_;
  long TimeoutSeconds_;
  bool isAnonymous;

};

}}} // apache::thrift::transport

#endif // #ifndef _THRIFT_TRANSPORT_TPIPE_H_

