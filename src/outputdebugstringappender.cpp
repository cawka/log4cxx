/*
 * Copyright 2003,2004 The Apache Software Foundation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if defined(_WIN32)
#include <log4cxx/nt/outputdebugstringappender.h>
#include <log4cxx/helpers/transcoder.h>

#include "windows.h"

using namespace log4cxx;
using namespace log4cxx::helpers;
using namespace log4cxx::nt;

IMPLEMENT_LOG4CXX_OBJECT(OutputDebugStringAppender)

OutputDebugStringAppender::OutputDebugStringAppender() {
}

void OutputDebugStringAppender::append(const spi::LoggingEventPtr& event, Pool& p)
{
        LogString buf;
        layout->format(buf, event, p);
        std::wstring wstr;
        log4cxx::helpers::Transcoder::encode(buf, wstr);
        ::OutputDebugStringW(wstr.c_str());
}

#endif

