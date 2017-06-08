/*
 * Copyright (C) 2016, 2017 Computer Graphics Group, University of Siegen
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef QVR_LOGGING_HPP
#define QVR_LOGGING_HPP

#include <cstdio>

#include "manager.hpp"

void QVRSetLogFile(const char* name, bool truncate); /* NULL means stderr */
const char* QVRGetLogFile(); /* NULL means stderr */

void QVRMsg(QVRLogLevel level, const char* s);

#define QVR_MSG_BUFSIZE 1024

#define QVR_MSG(level, ...) { char buf[QVR_MSG_BUFSIZE]; snprintf(buf, QVR_MSG_BUFSIZE, __VA_ARGS__); QVRMsg(level, buf); }
#define QVR_FATAL(...)      { QVR_MSG(QVR_Log_Level_Fatal, __VA_ARGS__); }
#define QVR_WARNING(...)    { if (QVRManager::logLevel() >= QVR_Log_Level_Warning)  { QVR_MSG(QVR_Log_Level_Warning, __VA_ARGS__); } }
#define QVR_INFO(...)       { if (QVRManager::logLevel() >= QVR_Log_Level_Info)     { QVR_MSG(QVR_Log_Level_Info, __VA_ARGS__); } }
#define QVR_DEBUG(...)      { if (QVRManager::logLevel() >= QVR_Log_Level_Debug)    { QVR_MSG(QVR_Log_Level_Debug, __VA_ARGS__); } }
#define QVR_FIREHOSE(...)   { if (QVRManager::logLevel() >= QVR_Log_Level_Firehose) { QVR_MSG(QVR_Log_Level_Firehose, __VA_ARGS__); } }

#endif
