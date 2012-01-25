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

#ifndef _THRIFT_TRANSPORT_TNAMEDPIPE_H_
#define _THRIFT_TRANSPORT_TNAMEDPIPE_H_ 1

#ifdef _WIN32
#include <io.h>
#endif

#include "TTransport.h"
#include "TVirtualTransport.h"

namespace apache { namespace thrift { namespace transport {

class TNamedPipe : public TVirtualTransport<TNamedPipe>
{
public:
	TNamedPipe(std::string pipeName);
	virtual ~TNamedPipe(void);
	
	// Returns whether the pipe is open & valid.
	bool isOpen();

	// Checks whether more data is available in the pipe.
	bool peek();

	// opens the named pipe.
	void open();
	
	// Shuts down communications on the pipe.
	void close();
	
	// Reads from the pipe.
	uint32_t read(uint8_t* buf, uint32_t len);
	
	// Writes to the pipe.
	void write(const uint8_t* buf, uint32_t len);

private:
	std::string pipeName_;
	HANDLE hPipe_;
};

}}} // apache::thrift::transport

#endif // #ifndef _THRIFT_TRANSPORT_TNAMEDPIPE_H_

