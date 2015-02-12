/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2015 Canonical Ltd
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtContacts module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCONTACTCOLLECTIONFILTER_P_H
#define QCONTACTCOLLECTIONFILTER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtContacts/qcontactcollectionfilter.h>
#include <QtContacts/private/qcontactfilter_p.h>

#include <algorithm>

QT_BEGIN_NAMESPACE_CONTACTS

class QContactCollectionFilterPrivate : public QContactFilterPrivate
{
public:
    QContactCollectionFilterPrivate()
        : QContactFilterPrivate()
    {
    }

    QContactCollectionFilterPrivate(const QContactCollectionFilterPrivate &other)
        : QContactFilterPrivate(other), m_ids(other.m_ids)
    {
    }

    virtual bool compare(const QContactFilterPrivate *other) const
    {
        const QContactCollectionFilterPrivate *od = static_cast<const QContactCollectionFilterPrivate *>(other);
        if (od)
            return m_ids == od->m_ids;
        return false;
    }

#ifndef QT_NO_DATASTREAM
    QDataStream &outputToStream(QDataStream &stream, quint8 formatVersion) const
    {
        if (formatVersion == 1)
            stream << m_ids;
        return stream;
    }

    QDataStream &inputFromStream(QDataStream &stream, quint8 formatVersion)
    {
        if (formatVersion == 1)
            stream >> m_ids;
        return stream;
    }
#endif // QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
    QDebug &debugStreamOut(QDebug &dbg) const
    {
        dbg.nospace() << "QContactCollectionFilter(collectionIds=";
        QList<QContactCollectionId> ids(m_ids.toList());
        std::sort(ids.begin(), ids.end());
        dbg.nospace() << ids;
        dbg.nospace() << ")";
        return dbg.maybeSpace();
    }
#endif // QT_NO_DEBUG_STREAM

    Q_IMPLEMENT_CONTACTFILTER_VIRTUALCTORS(QContactCollectionFilter, QContactFilter::CollectionFilter)

    QSet<QContactCollectionId> m_ids;
};

QT_END_NAMESPACE_CONTACTS

#endif // QCONTACTCOLLECTIONFILTER_P_H