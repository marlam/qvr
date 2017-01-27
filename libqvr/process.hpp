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

#ifndef QVR_PROCESS_HPP
#define QVR_PROCESS_HPP

#include <QProcess>

#include "manager.hpp"

class QVRWindowConfig;
class QVRProcessConfig;

/*!
 * \brief Application process.
 *
 * A process is connected to exactly one Qt display, which typically represents
 * one GPU. A display can contain multiple screens, which typically correspond
 * to multiple monitors or projectors.
 *
 * A process can drive multiple \a QVRWindow windows with different screens,
 * positions, and sizes.
 *
 * The first process started by the user is the master process and has index 0.
 * Slave processes, if configured in the QVR configuration, are launched by the
 * \a QVRManager.
 *
 * A process is configured via \a QVRProcessConfig.
 */
class QVRProcess : public QProcess
{
private:
    int _index;

    // functions for the master to manage slave processes
    bool launch(const QString& prg, const QStringList& args);
    bool exit();

    /*! \cond
     * This is internal information. */
    friend class QVRManager;
    /*! \endcond */

public:
    /*! \brief Constructor for the process with the given \a index in the QVR configuration. */
    QVRProcess(int index);
    /*! \brief Destructor. */
    ~QVRProcess();

    /*! \brief Returns the index of the process in the QVR configuration.
     *
     * The process with index 0 is the master process.
     */
    int index() const;

    /*! \brief Returns the unique id. */
    const QString& id() const;
    /*! \brief Returns the configuration. */
    const QVRProcessConfig& config() const;

    /*! \brief Returns the unique id of the window with the given index. */
    const QString& windowId(int windowIndex) const;
    /*! \brief Returns the configuration of the window with the given index. */
    const QVRWindowConfig& windowConfig(int windowIndex) const;
};

#endif
