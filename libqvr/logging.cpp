/*
 * Copyright (C) 2016 Computer Graphics Group, University of Siegen
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

#include <cstdio>

#include "manager.hpp"
#include "logging.hpp"

static FILE* QVRLogStream = NULL;

void QVRSetLogFile(const char* name)
{
    if (!name) {
        QVRLogStream = NULL;
    } else if (!(QVRLogStream = std::fopen(name, "w"))) {
        QVR_WARNING("cannot open log file %s", name);
    }
}

void QVRMsg(const char* s)
{
    // We want to print one complete line with exactly one call to fputs to
    // line-buffered stderr so that the output of different processes is not
    // mangled. Therefore we buffer what we want to print.
    char buf[QVR_MSG_BUFSIZE] = "QVR";
    int bufIndex = 3;
    if (QVRManager::config().processConfigs().size() > 1)
        bufIndex += std::snprintf(buf + bufIndex, QVR_MSG_BUFSIZE - bufIndex, "[%d]", QVRManager::processIndex());
    bufIndex += snprintf(buf + bufIndex, QVR_MSG_BUFSIZE - bufIndex, ": %s", s);
    buf[std::min(bufIndex, QVR_MSG_BUFSIZE - 2)] = '\n';
    bufIndex++;
    buf[std::min(bufIndex, QVR_MSG_BUFSIZE - 1)] = '\0';
    std::fputs(buf, QVRLogStream ? QVRLogStream : stderr);
}
