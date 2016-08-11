/****************************************************************************
**
** Copyright (C) 2019 Andre Hartmann <aha_1980@gmx.de>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtSerialBus module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "libsocketcan.h"

#include <QtCore/qloggingcategory.h>

#if QT_CONFIG(library)
#   include <QtCore/qlibrary.h>
#endif

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QT_CANBUS_PLUGINS_SOCKETCAN)

#define GENERATE_SYMBOL(returnType, symbolName, ...) \
    typedef returnType (*fp_##symbolName)(__VA_ARGS__); \
    static fp_##symbolName symbolName = nullptr;

#define RESOLVE_SYMBOL(symbolName) \
    symbolName = reinterpret_cast<fp_##symbolName>(library->resolve(#symbolName)); \
    if (!symbolName) \
        return false;

struct can_bittiming {
    quint32 bitrate = 0;      /* Bit-rate in bits/second */
    quint32 sample_point = 0; /* Sample point in one-tenth of a percent */
    quint32 tq = 0;           /* Time quanta (TQ) in nanoseconds */
    quint32 prop_seg = 0;     /* Propagation segment in TQs */
    quint32 phase_seg1 = 0;   /* Phase buffer segment 1 in TQs */
    quint32 phase_seg2 = 0;   /* Phase buffer segment 2 in TQs */
    quint32 sjw = 0;          /* Synchronization jump width in TQs */
    quint32 brp = 0;          /* Bit-rate prescaler */
};

GENERATE_SYMBOL(int, can_do_restart, const char * /* name */)
GENERATE_SYMBOL(int, can_do_stop, const char * /* name */)
GENERATE_SYMBOL(int, can_do_start, const char * /* name */)
GENERATE_SYMBOL(int, can_set_bitrate, const char * /* name */, quint32 /* bitrate */)
GENERATE_SYMBOL(int, can_get_bittiming, const char * /* name */, struct can_bittiming * /* bt */)

LibSocketCan::LibSocketCan(QString *errorString)
{
#if QT_CONFIG(library)
    auto resolveSymbols = [](QLibrary *library) {
        if (!library->isLoaded()) {
            library->setFileName(QStringLiteral("socketcan"));
            if (!library->load())
                return false;
        }

        RESOLVE_SYMBOL(can_do_start);
        RESOLVE_SYMBOL(can_do_stop);
        RESOLVE_SYMBOL(can_do_restart);
        RESOLVE_SYMBOL(can_set_bitrate);
        RESOLVE_SYMBOL(can_get_bittiming);

        return true;
    };

    QLibrary lib;
    if (Q_UNLIKELY(!resolveSymbols(&lib))) {
        qCWarning(QT_CANBUS_PLUGINS_SOCKETCAN, "%ls", qUtf16Printable(lib.errorString()));
        if (errorString)
            *errorString = lib.errorString();
    }
#else
    const QString error =
            QObject::tr("Cannot load library libsocketcan as Qt was built without QLibrary.");
    qCWarning(QT_CANBUS_PLUGINS_SOCKETCAN, "%ls", qUtf16Printable(error));
    if (errorString)
        *errorString = error;
#endif
}

/*!
    Brings the CAN \a interface up.

    \internal
    \note Requires appropriate permissions.
*/
bool LibSocketCan::start(const QString &interface)
{
    if (!::can_do_start) {
        qCWarning(QT_CANBUS_PLUGINS_SOCKETCAN, "Function can_do_start() is not available.");
        return false;
    }

    return ::can_do_start(interface.toLatin1().constData()) == 0;
}

/*!
    Brings the CAN \a interface down.

    \internal
    \note Requires appropriate permissions.
*/
bool LibSocketCan::stop(const QString &interface)
{
    if (!::can_do_stop) {
        qCWarning(QT_CANBUS_PLUGINS_SOCKETCAN, "Function can_do_stop() is not available.");
        return false;
    }

    return ::can_do_stop(interface.toLatin1().constData()) == 0;
}

/*!
    Performs a CAN controller reset on the CAN \a interface.

    \internal
    \note Reset can only be triggerd if the controller is in bus off
    and the auto restart not turned on.
    \note Requires appropriate permissions.
 */
bool LibSocketCan::restart(const QString &interface)
{
    if (!::can_do_restart) {
        qCWarning(QT_CANBUS_PLUGINS_SOCKETCAN, "Function can_do_restart() is not available.");
        return false;
    }

    return ::can_do_restart(interface.toLatin1().constData()) == 0;
}

/*!
    Returns the configured bitrate for \a interface.
    \internal
*/
quint32 LibSocketCan::bitrate(const QString &interface) const
{
    if (!::can_get_bittiming) {
        qCWarning(QT_CANBUS_PLUGINS_SOCKETCAN, "Function can_get_bittiming() is not available.");
        return 0;
    }

    struct can_bittiming bt;
    if (::can_get_bittiming(interface.toLatin1().constData(), &bt) == 0)
        return bt.bitrate;

    return 0;
}

/*!
    Sets the bitrate for the CAN \a interface.

    \internal
    \note Requires appropriate permissions.
 */
bool LibSocketCan::setBitrate(const QString &interface, quint32 bitrate)
{
    if (!::can_set_bitrate) {
        qCWarning(QT_CANBUS_PLUGINS_SOCKETCAN, "Function can_set_bitrate() is not available.");
        return false;
    }

    return ::can_set_bitrate(interface.toLatin1().constData(), bitrate) == 0;
}

QT_END_NAMESPACE
