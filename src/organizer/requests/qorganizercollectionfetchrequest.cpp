/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtOrganizer module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qorganizercollectionfetchrequest.h>
#include <private/qorganizeritemrequests_p.h>

QTORGANIZER_BEGIN_NAMESPACE

/*!
    \class QOrganizerCollectionFetchRequest
    \brief The QOrganizerCollectionFetchRequest class allows a client to asynchronously fetch collections
           from a backend.
    \inmodule QtOrganizer
    \ingroup organizeritems-requests

    This request will fetch all the collections stored in the given backend.
 */

/*!
    Constructs a new organizeritem fetch request whose parent is the specified \a parent.
*/
QOrganizerCollectionFetchRequest::QOrganizerCollectionFetchRequest(QObject *parent)
    : QOrganizerAbstractRequest(new QOrganizerCollectionFetchRequestPrivate, parent)
{
}

/*!
    Frees memory in use by this request.
*/
QOrganizerCollectionFetchRequest::~QOrganizerCollectionFetchRequest()
{
    QOrganizerAbstractRequestPrivate::notifyEngine(this);
}

/*!
    Sets storage locations where the collections are fetched from. \a storageLocations is a flag,
    so you can define multiple locations in it.

    \sa QOrganizerAbstractRequest::StorageLocation
    \sa QOrganizerCollectionFetchRequest::storageLocations()
*/
void QOrganizerCollectionFetchRequest::setStorageLocations(QOrganizerAbstractRequest::StorageLocations storageLocations)
{
    Q_D(QOrganizerCollectionFetchRequest);
    QMutexLocker ml(&d->m_mutex);
    d->m_storageLocations = storageLocations;
}

/*!
    Storage locations where the collections are fetched from.

    \sa QOrganizerAbstractRequest::StorageLocation
    \sa QOrganizerCollectionFetchRequest::setStorageLocations()
*/
QOrganizerAbstractRequest::StorageLocations QOrganizerCollectionFetchRequest::storageLocations() const
{
    Q_D(const QOrganizerCollectionFetchRequest);
    QMutexLocker ml(&d->m_mutex);
    return d->m_storageLocations;
}

/*!
    Returns the collections retrieved by this request.
*/
QList<QOrganizerCollection> QOrganizerCollectionFetchRequest::collections() const
{
    Q_D(const QOrganizerCollectionFetchRequest);
    QMutexLocker ml(&d->m_mutex);
    return d->m_collections;
}

#include "moc_qorganizercollectionfetchrequest.cpp"

QTORGANIZER_END_NAMESPACE
