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

#include <qorganizeritemdetails.h>
#include <QtQml/qqmlinfo.h>
#include "qdeclarativeorganizermodel_p.h"
#include <qorganizermanager.h>
#include <qversitorganizerimporter.h>
#include <qversitorganizerexporter.h>
#include "qdeclarativeorganizercollection_p.h"
#include <QFile>
#include <QUrl>

#include <qorganizeritemrequests.h>
#include <QtCore/qmath.h>


QTVERSITORGANIZER_USE_NAMESPACE

QTORGANIZER_BEGIN_NAMESPACE

// TODO:
// - Improve handling of itemsModified signal. Instead of fetching all items each time the
//   signal is received, only modified items should be fetched. Item based fetching allows easier
//   use of any item id based caches backends might have.
// - Full update is not needed every time some model property changes. Collections should
//   be updated only if collections have been changed while autoUpdate is off.
// - Changing the time period is by far the most common use case and should be optimized.
//   If new time period overlaps with the old, no need to fetch all the items in new time period.
// - All changes happening during autoUpdate is off should be monitored. If only timePeriod changes
//   there is no need for full update after switching autoUpdate on.


static QString urlToLocalFileName(const QUrl& url)
{
   if (!url.isValid()) {
      return url.toString();
   } else if (url.scheme() == "qrc") {
      return url.toString().remove(0, 5).prepend(':');
   } else {
      return url.toLocalFile();
   }

}


class QDeclarativeOrganizerModelPrivate
{
public:
    QDeclarativeOrganizerModelPrivate()
        :m_manager(0),
        m_fetchHint(0),
        m_filter(0),
        m_fetchRequest(0),
        m_occurrenceFetchRequest(0),
        m_reader(0),
        m_writer(0),
        m_startPeriod(QDateTime::currentDateTime()),
        m_endPeriod(QDateTime::currentDateTime()),
        m_storageLocations(QOrganizerAbstractRequest::UserDataStorage),
        m_error(QOrganizerManager::NoError),
        m_autoUpdate(true),
        m_updatePending(false),
        m_componentCompleted(false),
        m_initialUpdate(false),
        m_lastRequestId(0)
    {
    }
    ~QDeclarativeOrganizerModelPrivate()
    {
        if (m_manager)
            delete m_manager;
        if (m_filter)
            delete m_filter;
        delete m_reader;
        delete m_writer;
}

    QList<QDeclarativeOrganizerItem*> m_items;
    QHash<QString, QDeclarativeOrganizerItem *> m_itemIdHash;
    QOrganizerManager* m_manager;
    QDeclarativeOrganizerItemFetchHint* m_fetchHint;
    QList<QOrganizerItemSortOrder> m_sortOrders;
    QList<QDeclarativeOrganizerItemSortOrder*> m_declarativeSortOrders;
    QDeclarativeOrganizerItemFilter* m_filter;
    QOrganizerItemFetchRequest* m_fetchRequest;
    QSet<QOrganizerItemId> m_addedItemIds;
    QMap<QOrganizerAbstractRequest*, QSet<QOrganizerItemId> > m_notifiedItems;
    QOrganizerItemOccurrenceFetchRequest* m_occurrenceFetchRequest;
    QStringList m_importProfiles;
    QVersitReader *m_reader;
    QVersitWriter *m_writer;
    QDateTime m_startPeriod;
    QDateTime m_endPeriod;
    QList<QDeclarativeOrganizerCollection*> m_collections;
    QOrganizerAbstractRequest::StorageLocations m_storageLocations;

    QOrganizerManager::Error m_error;

    bool m_autoUpdate;
    bool m_updatePending;
    bool m_componentCompleted;
    bool m_initialUpdate;

    QAtomicInt m_lastRequestId;
    QHash<QOrganizerAbstractRequest *, int> m_requestIdHash;
    QUrl m_lastExportUrl;
    QUrl m_lastImportUrl;
};

/*!
    \qmlclass OrganizerModel QDeclarativeOrganizerModel
    \brief The OrganizerModel element provides access to organizer items from the organizer store.
    \inqmlmodule QtOrganizer
    \ingroup qml-organizer-main

    OrganizerModel provides a model of organizer items from the organizer store.
    The contents of the model can be specified with \l filter, \l sortOrders and \l fetchHint properties.
    Whether the model is automatically updated when the store or \l {C++ organizer} item changes, can be
    controlled with \l OrganizerModel::autoUpdate property.

    There are two ways of accessing the organizer item data: via the model by using views and delegates,
    or alternatively via \l items list property. Of the two, the model access is preferred.
    Direct list access (i.e. non-model) is not guaranteed to be in order set by \l sortOrder.

    At the moment the model roles provided by OrganizerModel are \c display and \c item.
    Through the \c item role can access any data provided by the OrganizerItem element.


    \note Both the \c startPeriod and \c endPeriod are set by default to the current time (when the OrganizerModel was created). 
     In most cases, both (or at least one) of the startPeriod and endPeriod should be set; otherwise, the OrganizerModel will contain 
     zero items because the \c startPeriod and \c endPeriod are the same value. For example, if only \c endPeriod is provided, 
     the OrganizerModel will contain all items from now (the time of the OrganizerModel's creation) to the \c endPeriod time.

    \sa OrganizerItem, {QOrganizerManager}
*/

QDeclarativeOrganizerModel::QDeclarativeOrganizerModel(QObject *parent) :
    QAbstractListModel(parent),
    d_ptr(new QDeclarativeOrganizerModelPrivate)
{
    QHash<int, QByteArray> roleNames;
    roleNames = QAbstractItemModel::roleNames();
    roleNames.insert(OrganizerItemRole, "item");
    setRoleNames(roleNames);

    connect(this, SIGNAL(managerChanged()), SLOT(doUpdate()));
    connect(this, SIGNAL(filterChanged()), SLOT(doUpdate()));
    connect(this, SIGNAL(fetchHintChanged()), SLOT(doUpdate()));
    connect(this, SIGNAL(sortOrdersChanged()), SLOT(doUpdate()));
    connect(this, SIGNAL(startPeriodChanged()), SLOT(doUpdate()));
    connect(this, SIGNAL(endPeriodChanged()), SLOT(doUpdate()));
    connect(this, SIGNAL(storageLocationsChanged()), SLOT(doUpdate()));//FIXME; optimize with handling only the delta instead of full update
}

QDeclarativeOrganizerModel::~QDeclarativeOrganizerModel()
{
}

/*!
  \qmlproperty string OrganizerModel::manager

  This property holds the manager name or manager uri of the organizer backend engine.
  The manager uri format: qtorganizer:<managerid>:<key>=<value>&<key>=<value>.

  For example, memory organizer engine has an optional id parameter, if user want to
  share the same memory engine with multiple OrganizerModel instances, the manager property
  should declared like this:
  \code
    model : OrganizerModel {
       manager:"qtorganizer:memory:id=organizer1
    }
  \endcode

  instead of just the manager name:
  \code
    model : OrganizerModel {
       manager:"memory"
    }
  \endcode

  \sa QOrganizerManager::fromUri()
  */
QString QDeclarativeOrganizerModel::manager() const
{
    Q_D(const QDeclarativeOrganizerModel);
    if (d->m_manager)
        return d->m_manager->managerUri();
    return QString();
}

/*!
  \qmlproperty string OrganizerModel::managerName

  This property holds the manager name of the organizer backend engine.
  This property is read only.
  \sa QOrganizerManager::fromUri()
  */
QString QDeclarativeOrganizerModel::managerName() const
{
    Q_D(const QDeclarativeOrganizerModel);
    if (d->m_manager)
        return d->m_manager->managerName();
    return QString();
}

/*!
  \qmlproperty list<string> OrganizerModel::availableManagers

  This property holds the list of available manager names.
  This property is read only.
  */
QStringList QDeclarativeOrganizerModel::availableManagers() const
{
    return QOrganizerManager::availableManagers();
}

/*!
  \qmlproperty bool OrganizerModel::autoUpdate

  This property indicates whether or not the organizer model should be updated automatically, default value is true.

  \sa OrganizerModel::update()
  */
void QDeclarativeOrganizerModel::setAutoUpdate(bool autoUpdate)
{
    Q_D(QDeclarativeOrganizerModel);
    if (autoUpdate == d->m_autoUpdate)
        return;
    d->m_autoUpdate = autoUpdate;
    emit autoUpdateChanged();
}

bool QDeclarativeOrganizerModel::autoUpdate() const
{
    Q_D(const QDeclarativeOrganizerModel);
    return d->m_autoUpdate;
}

/*!
  \qmlmethod OrganizerModel::update()

  Manually update the organizer model content.

  \sa OrganizerModel::autoUpdate
  */
void QDeclarativeOrganizerModel::update()
{
    Q_D(QDeclarativeOrganizerModel);
    if (!d->m_componentCompleted || d->m_updatePending)
        return;
    d->m_updatePending = true; // Disallow possible duplicate request triggering
    QMetaObject::invokeMethod(this, "fetchCollections", Qt::QueuedConnection);
}

void QDeclarativeOrganizerModel::doUpdate()
{
    Q_D(QDeclarativeOrganizerModel);
    if (d->m_autoUpdate)
        update();
}

/*!
  \qmlmethod OrganizerModel::cancelUpdate()

  Cancel the running organizer model content update request.

  \sa OrganizerModel::autoUpdate  OrganizerModel::update
  */
void QDeclarativeOrganizerModel::cancelUpdate()
{
    Q_D(QDeclarativeOrganizerModel);
    if (d->m_fetchRequest) {
        d->m_fetchRequest->cancel();
        d->m_fetchRequest->deleteLater();
        d->m_fetchRequest = 0;
        d->m_updatePending = false;
    }
}
/*!
  \qmlproperty date OrganizerModel::startPeriod

  This property holds the start date and time period used by the organizer model to fetch organizer items.
  The default value is the datetime of OrganizerModel creation.
  */
QDateTime QDeclarativeOrganizerModel::startPeriod() const
{
    Q_D(const QDeclarativeOrganizerModel);
    return d->m_startPeriod;
}
void QDeclarativeOrganizerModel::setStartPeriod(const QDateTime& start)
{
    Q_D(QDeclarativeOrganizerModel);
    if (start != d->m_startPeriod) {
        d->m_startPeriod = start;
        emit startPeriodChanged();
    }
}

/*!
  \qmlproperty date OrganizerModel::endPeriod

  This property holds the end date and time period used by the organizer model to fetch organizer items.
  The default value is the datetime of OrganizerModel creation.
  */
QDateTime QDeclarativeOrganizerModel::endPeriod() const
{
    Q_D(const QDeclarativeOrganizerModel);
    return d->m_endPeriod;
}
void QDeclarativeOrganizerModel::setEndPeriod(const QDateTime& end)
{
    Q_D(QDeclarativeOrganizerModel);
    if (end != d->m_endPeriod) {
        d->m_endPeriod = end;
        emit endPeriodChanged();
    }
}

/*!
  \qmlproperty enumeration OrganizerModel::StorageLocation

  Defines the different storage locations for saving items and model population purposes.

  \list
  \li OrganizerModel::UserDataStorage        A storage location where user data is usually stored.
  \li OrganizerModel::SystemStorage          A storage location where system files are usually stored.
  \endlist

  Depending on the platform, the access rights for different storage locations might vary.

  \sa OrganizerModel::storageLocations
  \sa OrganizerModel::saveCollection()
  \sa OrganizerModel::saveItem()
*/

/*!
  \qmlsignal OrganizerModel::storageLocationsChanged()

  This signal is emitted, when \l OrganizerModel::storageLocations property changes.

  \sa OrganizerModel::storageLocations
 */

/*!
  \qmlproperty int OrganizerModel::storageLocations

  This property indicates which storage locations are used to populate the declarative model. As the type
  of property is int, it can be utilized as flag with several values.

  storageLocations is a backend specific feature. Some backends support it and some might just ignore it. If backend
  is having some specific requirements and they're not met, backend returns StorageLocationsNotExistingError.

  \sa OrganizerModel::StorageLocation
*/
int QDeclarativeOrganizerModel::storageLocations() const
{
    Q_D(const QDeclarativeOrganizerModel);
    return int(d->m_storageLocations);
}

void QDeclarativeOrganizerModel::setStorageLocations(int storageLocationsFlag)
{
    Q_D(QDeclarativeOrganizerModel);
    if (storageLocations() != storageLocationsFlag) {
        d->m_storageLocations = QOrganizerAbstractRequest::StorageLocations(storageLocationsFlag);
        emit storageLocationsChanged();
    }
}

/*!
  \qmlmethod OrganizerModel::importItems(url url, list<string> profiles)

  Import organizer items from a vcalendar by the given \a url and optional \a profiles.
  Only one import operation can be active at a time.
 */
void QDeclarativeOrganizerModel::importItems(const QUrl& url, const QStringList &profiles)
{
    Q_D(QDeclarativeOrganizerModel);

    ImportError importError = ImportNotReadyError;

    // Reader is capable of handling only one request at the time.
  if (!d->m_reader || (d->m_reader->state() != QVersitReader::ActiveState)) {

        d->m_importProfiles = profiles;

        //TODO: need to allow download vcard from network
        QFile *file = new QFile(urlToLocalFileName(url));
        if (file->open(QIODevice::ReadOnly)) {
            if (!d->m_reader) {
                d->m_reader = new QVersitReader;
                connect(d->m_reader, SIGNAL(stateChanged(QVersitReader::State)), this, SLOT(startImport(QVersitReader::State)));
            }

            d->m_reader->setDevice(file);
            if (d->m_reader->startReading()) {
                d->m_lastImportUrl = url;
                return;
            }
            importError = QDeclarativeOrganizerModel::ImportError(d->m_reader->error());
        } else {
            importError = ImportIOError;
        }
  }

    // If cannot startReading because already running then report the import error now
    emit importCompleted(importError, url);
}

/*!
  \qmlmethod OrganizerModel::exportItems(url url, list<string> profiles)
  Export organizer items into a vcalendar file to the given \a url by optional \a profiles.
  At the moment only the local file url is supported in export method.
  */
void QDeclarativeOrganizerModel::exportItems(const QUrl &url, const QStringList &profiles)
{
    Q_D(QDeclarativeOrganizerModel);
    ExportError exportError = ExportNotReadyError;

    // Writer is capable of handling only one request at the time.
    if (!d->m_writer || (d->m_writer->state() != QVersitWriter::ActiveState)) {

        QString profile = profiles.isEmpty() ? QString() : profiles.at(0);

        QVersitOrganizerExporter exporter(profile);
        QList<QOrganizerItem> items;
        foreach (QDeclarativeOrganizerItem *di, d->m_items)
            items.append(di->item());

        exporter.exportItems(items, QVersitDocument::ICalendar20Type);
        QVersitDocument document = exporter.document();
        QFile *file = new QFile(urlToLocalFileName(url));
        if (file->open(QIODevice::ReadWrite)) {
            if (!d->m_writer) {
                d->m_writer = new QVersitWriter;
                connect(d->m_writer, SIGNAL(stateChanged(QVersitWriter::State)), this, SLOT(itemsExported(QVersitWriter::State)));
            }
            d->m_writer->setDevice(file);
            if (d->m_writer->startWriting(document)) {
                d->m_lastExportUrl = url;
                return;
            }
            exportError = QDeclarativeOrganizerModel::ExportError(d->m_writer->error());
        } else {
            exportError = ExportIOError;
        }
    }
    emit exportCompleted(exportError, url);
}

void QDeclarativeOrganizerModel::itemsExported(QVersitWriter::State state)
{
    Q_D(QDeclarativeOrganizerModel);
    if (state == QVersitWriter::FinishedState || state == QVersitWriter::CanceledState) {
         emit exportCompleted(QDeclarativeOrganizerModel::ExportError(d->m_writer->error()), d->m_lastExportUrl);
         delete d->m_writer->device();
         d->m_writer->setDevice(0);
    }
}

int QDeclarativeOrganizerModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    Q_D(const QDeclarativeOrganizerModel);
    return d->m_items.count();
}

void QDeclarativeOrganizerModel::setManager(const QString& managerName)
{
    Q_D(QDeclarativeOrganizerModel);
    if (d->m_manager && (managerName == d->m_manager->managerName() || managerName == d->m_manager->managerUri()))
        return;

    if (d->m_manager) {
        cancelUpdate();
        d->m_updatePending = false;
        delete d->m_manager;
    }

    if (managerName.startsWith("qtorganizer:")) {
        d->m_manager = QOrganizerManager::fromUri(managerName, this);
    } else {
        d->m_manager = new QOrganizerManager(managerName, QMap<QString, QString>(), this);
    }

    connect(d->m_manager, SIGNAL(dataChanged()), this, SLOT(doUpdate()));
    connect(d->m_manager, SIGNAL(itemsModified(QList<QPair<QOrganizerItemId, QOrganizerManager::Operation> >)), this, SLOT(onItemsModified(QList<QPair<QOrganizerItemId, QOrganizerManager::Operation> >)));
    connect(d->m_manager, SIGNAL(collectionsAdded(QList<QOrganizerCollectionId>)), this, SLOT(fetchCollections()));
    connect(d->m_manager, SIGNAL(collectionsChanged(QList<QOrganizerCollectionId>)), this, SLOT(fetchCollections()));
    connect(d->m_manager, SIGNAL(collectionsRemoved(QList<QOrganizerCollectionId>)), this, SLOT(fetchCollections()));

    if (d->m_error != QOrganizerManager::NoError) {
        d->m_error = QOrganizerManager::NoError;
        emit errorChanged();
    }

    emit managerChanged();
}

void QDeclarativeOrganizerModel::componentComplete()
{
    Q_D(QDeclarativeOrganizerModel);
    d->m_componentCompleted = true;
    if (!d->m_manager)
        setManager(QString());

    if (d->m_autoUpdate) {
        d->m_initialUpdate = true;
        update();
    } else {
        emit modelChanged();
    }
}
/*!
  \qmlproperty Filter OrganizerModel::filter

  This property holds the filter instance used by the organizer model.

  \sa Filter
  */
QDeclarativeOrganizerItemFilter* QDeclarativeOrganizerModel::filter() const
{
    Q_D(const QDeclarativeOrganizerModel);
    return d->m_filter;
}

void QDeclarativeOrganizerModel::setFilter(QDeclarativeOrganizerItemFilter* filter)
{
    Q_D(QDeclarativeOrganizerModel);
    if (filter != d->m_filter) {
        d->m_filter = filter;
        if (d->m_filter)
            connect(d->m_filter, SIGNAL(filterChanged()), this, SIGNAL(filterChanged()));
        emit filterChanged();
    }
}

/*!
  \qmlproperty FetchHint OrganizerModel::fetchHint

  This property holds the fetch hint instance used by the organizer model.

  \sa FetchHint
  */
QDeclarativeOrganizerItemFetchHint* QDeclarativeOrganizerModel::fetchHint() const
{
    Q_D(const QDeclarativeOrganizerModel);
    return d->m_fetchHint;
}

void QDeclarativeOrganizerModel::setFetchHint(QDeclarativeOrganizerItemFetchHint* fetchHint)
{
    Q_D(QDeclarativeOrganizerModel);
    if (fetchHint && fetchHint != d->m_fetchHint) {
        if (d->m_fetchHint)
            delete d->m_fetchHint;
        d->m_fetchHint = fetchHint;
        connect(d->m_fetchHint, SIGNAL(fetchHintChanged()), this, SIGNAL(fetchHintChanged()));
        emit fetchHintChanged();
    }
}
/*!
  \qmlproperty int OrganizerModel::itemCount

  This property holds the size of organizer items the OrganizerModel currently holds.

  This property is read only.
  */
int QDeclarativeOrganizerModel::itemCount() const
{
    Q_D(const QDeclarativeOrganizerModel);
    return d->m_items.size();
}
/*!
  \qmlproperty string OrganizerModel::error

  This property holds the latest error code returned by the organizer manager.

  This property is read only.
  */
QString QDeclarativeOrganizerModel::error() const
{
    Q_D(const QDeclarativeOrganizerModel);
    if (d->m_manager) {
        switch (d->m_error) {
        case QOrganizerManager::DoesNotExistError:
            return QLatin1String("DoesNotExist");
        case QOrganizerManager::AlreadyExistsError:
            return QLatin1String("AlreadyExists");
        case QOrganizerManager::InvalidDetailError:
            return QLatin1String("InvalidDetail");
        case QOrganizerManager::InvalidCollectionError:
            return QLatin1String("InvalidCollection");
        case QOrganizerManager::LockedError:
            return QLatin1String("LockedError");
        case QOrganizerManager::DetailAccessError:
            return QLatin1String("DetailAccessError");
        case QOrganizerManager::PermissionsError:
            return QLatin1String("PermissionsError");
        case QOrganizerManager::OutOfMemoryError:
            return QLatin1String("OutOfMemory");
        case QOrganizerManager::NotSupportedError:
            return QLatin1String("NotSupported");
        case QOrganizerManager::BadArgumentError:
            return QLatin1String("BadArgument");
        case QOrganizerManager::UnspecifiedError:
            return QLatin1String("UnspecifiedError");
        case QOrganizerManager::LimitReachedError:
            return QLatin1String("LimitReached");
        case QOrganizerManager::InvalidItemTypeError:
            return QLatin1String("InvalidItemType");
        case QOrganizerManager::InvalidOccurrenceError:
            return QLatin1String("InvalidOccurrence");
        case QOrganizerManager::StorageLocationsNotExistingError:
            return QLatin1String("StorageLocationsNotExistingError");
        default:
            break;
        }
    }
    return QLatin1String("NoError");
}

/*!
  \qmlproperty list<SortOrder> OrganizerModel::sortOrders

  This property holds a list of sort orders used by the organizer model.

  \sa SortOrder
  */
QQmlListProperty<QDeclarativeOrganizerItemSortOrder> QDeclarativeOrganizerModel::sortOrders()
{
    return QQmlListProperty<QDeclarativeOrganizerItemSortOrder>(this,
                                                                        0,
                                                                        sortOrder_append,
                                                                        sortOrder_count,
                                                                        sortOrder_at,
                                                                        sortOrder_clear);
}

void QDeclarativeOrganizerModel::startImport(QVersitReader::State state)
{
    Q_D(QDeclarativeOrganizerModel);
    if (state == QVersitReader::FinishedState || state == QVersitReader::CanceledState) {
        if (!d->m_reader->results().isEmpty()) {
            QVersitOrganizerImporter importer;
            importer.importDocument(d->m_reader->results().at(0));
            QList<QOrganizerItem> items = importer.items();
            delete d->m_reader->device();
            d->m_reader->setDevice(0);

            if (d->m_manager && !d->m_manager->saveItems(&items) && d->m_error != d->m_manager->error()) {
                d->m_error = d->m_manager->error();
                emit errorChanged();
            }
        }
        emit importCompleted(QDeclarativeOrganizerModel::ImportError(d->m_reader->error()), d->m_lastImportUrl);
    }
}

bool QDeclarativeOrganizerModel::itemHasRecurrence(const QOrganizerItem& oi) const
{
    if (oi.type() == QOrganizerItemType::TypeEvent || oi.type() == QOrganizerItemType::TypeTodo) {
        QOrganizerItemRecurrence recur = oi.detail(QOrganizerItemDetail::TypeRecurrence);
        return !recur.recurrenceDates().isEmpty() || !recur.recurrenceRules().isEmpty();
    }

    return false;
}

QDeclarativeOrganizerItem* QDeclarativeOrganizerModel::createItem(const QOrganizerItem& item)
{
    QDeclarativeOrganizerItem* di;
    if (item.type() == QOrganizerItemType::TypeEvent)
        di = new QDeclarativeOrganizerEvent(this);
    else if (item.type() == QOrganizerItemType::TypeEventOccurrence)
        di = new QDeclarativeOrganizerEventOccurrence(this);
    else if (item.type() == QOrganizerItemType::TypeTodo)
        di = new QDeclarativeOrganizerTodo(this);
    else if (item.type() == QOrganizerItemType::TypeTodoOccurrence)
        di = new QDeclarativeOrganizerTodoOccurrence(this);
    else if (item.type() == QOrganizerItemType::TypeJournal)
        di = new QDeclarativeOrganizerJournal(this);
    else if (item.type() == QOrganizerItemType::TypeNote)
        di = new QDeclarativeOrganizerNote(this);
    else
        di = new QDeclarativeOrganizerItem(this);
    di->setItem(item);
    return di;
}

void QDeclarativeOrganizerModel::checkError(const QOrganizerAbstractRequest *request)
{
    Q_D(QDeclarativeOrganizerModel);
    if (request && d->m_error != request->error()) {
        d->m_error = request->error();
        emit errorChanged();
    }
}

/*!
    \qmlsignal OrganizerModel::onItemsFetched(int requestId, list<OrganizerItem> fetchedItems)

    This handler is called when request of the given \a requestId is finished with the \a fetchedItems.

    \sa fetchItems
 */

/*!
    \qmlmethod int OrganizerModel::fetchItems(stringlist itemIds)

    Starts a request to fetch items by the given \a itemIds, and returns the unique ID of this request.
    -1 is returned if the request can't be started.

    Note that the items fetched won't be added to the model, but can be accessed through the onItemsFetched
    handler.

    \sa onItemsFetched
  */
int QDeclarativeOrganizerModel::fetchItems(const QStringList &itemIds)
{
    Q_D(QDeclarativeOrganizerModel);
    if (itemIds.isEmpty())
        return -1;

    QOrganizerItemFetchByIdRequest *fetchRequest = new QOrganizerItemFetchByIdRequest(this);
    connect(fetchRequest, SIGNAL(stateChanged(QOrganizerAbstractRequest::State)),
            this, SLOT(onFetchItemsRequestStateChanged(QOrganizerAbstractRequest::State)));
    fetchRequest->setManager(d->m_manager);

    QList<QOrganizerItemId> ids;
    foreach (const QString &itemId, itemIds)
        ids.append(QOrganizerItemId::fromString(itemId));
    fetchRequest->setIds(ids);
    if (fetchRequest->start()) {
        int requestId(d->m_lastRequestId.fetchAndAddOrdered(1));
        d->m_requestIdHash.insert(fetchRequest, requestId);
        return requestId;
    } else {
        return -1;
    }
}

/*!
    \internal
 */
void QDeclarativeOrganizerModel::onFetchItemsRequestStateChanged(QOrganizerAbstractRequest::State state)
{
    Q_D(QDeclarativeOrganizerModel);
    if (state != QOrganizerAbstractRequest::FinishedState || !sender())
        return;

    QOrganizerItemFetchByIdRequest *request = qobject_cast<QOrganizerItemFetchByIdRequest *>(sender());
    if (!request)
        return;

    checkError(request);

    int requestId(d->m_requestIdHash.value(request, -1));
    QVariantList list;
    if (request->error() == QOrganizerManager::NoError) {
        QList<QOrganizerItem> items(request->items());
        QDeclarativeOrganizerItem *declarativeItem(0);
        foreach (const QOrganizerItem &item, items) {
            switch (item.type()) {
            case QOrganizerItemType::TypeEvent:
                declarativeItem = new QDeclarativeOrganizerEvent(this);
                break;
            case QOrganizerItemType::TypeEventOccurrence:
                declarativeItem = new QDeclarativeOrganizerEventOccurrence(this);
                break;
            case QOrganizerItemType::TypeTodo:
                declarativeItem = new QDeclarativeOrganizerTodo(this);
                break;
            case QOrganizerItemType::TypeTodoOccurrence:
                declarativeItem = new QDeclarativeOrganizerTodoOccurrence(this);
                break;
            default:
                declarativeItem = new QDeclarativeOrganizerItem(this);
                break;
            }
            declarativeItem->setItem(item);
            list.append(QVariant::fromValue((QObject *)declarativeItem));
        }
    }
    emit itemsFetched(requestId, list);

    request->deleteLater();
}

/*!
    \qmlmethod list<bool> OrganizerModel::containsItems(date start, date end, int interval)

    Returns a list of booleans telling if there is any item falling in the given time range.

    For example, if the \a start time is 2011-12-08 14:00:00, the \a end time is 2011-12-08 20:00:00,
    and the \a interval is 3600 (seconds), a list of size 6 is returned, telling if there is any item
    falling in the range of 14:00:00 to 15:00:00, 15:00:00 to 16:00:00, ..., 19:00:00 to 20:00:00.
 */
QList<bool> QDeclarativeOrganizerModel::containsItems(const QDateTime &start, const QDateTime &end, int interval)
{
    Q_D(QDeclarativeOrganizerModel);
    QList<bool> list;
    if (start.isValid() && end.isValid() && start < end && interval > 0) {
        int count = qCeil(start.secsTo(end) / static_cast<double>(interval));
        list.reserve(count);
        int i(0);
        for (; i < count; ++i)
            list.append(false);

        QList<QDateTime> dateTime;
        dateTime.reserve(count + 1);
        dateTime.append(start);
        for (i = 1; i < count; ++i)
            dateTime.append(dateTime.at(i - 1).addSecs(interval));
        dateTime.append(end);

        QDateTime startTime;
        QDateTime endTime;
        foreach (QDeclarativeOrganizerItem *item, d->m_items) {
            for (i = 0; i < count; ++i) {
                if (list.at(i))
                    continue;

                startTime = item->itemStartTime();
                endTime = item->itemEndTime();
                if ((startTime.isValid() && startTime <= dateTime.at(i) && endTime >= dateTime.at(i + 1))
                    || (startTime >= dateTime.at(i) && startTime <= dateTime.at(i + 1))
                    || (endTime >= dateTime.at(i) && endTime <= dateTime.at(i + 1))) {
                    list[i] = true;
                }
            }
        }
    }
    return list;
}

/*!
  \qmlmethod bool OrganizerModel::containsItems(date start, date end)

  Returns true if there is at least one OrganizerItem between the given date range.
  Both the \a start and  \a end parameters are optional, if no \a end parameter, returns true
  if there are item(s) after \a start, if neither start nor end date time provided, returns true if
  items in the current model is not empty, otherwise return false.

  \sa itemIds()
  */
bool QDeclarativeOrganizerModel::containsItems(const QDateTime &start, const QDateTime &end)
{
    return !itemIds(start, end).isEmpty();
}

/*!
    \qmlmethod list<OrganizerItem> OrganizerModel::itemsByTimePeriod(date start, date end)

    Returns the list of organizer items between the given \a start and \a end period.
 */
QVariantList QDeclarativeOrganizerModel::itemsByTimePeriod(const QDateTime &start, const QDateTime &end)
{
    Q_D(QDeclarativeOrganizerModel);
    QVariantList list;

    if (start.isValid() && end.isValid()) {
        QDateTime startTime;
        QDateTime endTime;
        foreach (QDeclarativeOrganizerItem *item, d->m_items) {
            startTime = item->itemStartTime();
            endTime = item->itemEndTime();
            if ((startTime.isValid() && startTime <= start && endTime >= end)
                || (startTime >= start && startTime <= end)
                || (endTime >= start && endTime <= end)) {
                list.append(QVariant::fromValue((QObject *)item));
            }
        }
    } else if (start.isValid()) {
        foreach (QDeclarativeOrganizerItem *item, d->m_items) {
            if (item->itemEndTime() >= start)
                list.append(QVariant::fromValue((QObject *)item));
        }
    } else if (end.isValid()) {
        foreach (QDeclarativeOrganizerItem *item, d->m_items) {
            if (item->itemStartTime() <= end)
                list.append(QVariant::fromValue((QObject *)item));
        }
    } else {
        foreach (QDeclarativeOrganizerItem *item, d->m_items)
            list.append(QVariant::fromValue((QObject *)item));
    }

    return list;
}

/*!
    \qmlmethod OrganizerItem OrganizerModel::item(string itemId)

    Returns the OrganizerItem object with the given \a itemId.
 */
QDeclarativeOrganizerItem *QDeclarativeOrganizerModel::item(const QString &itemId)
{
    Q_D(QDeclarativeOrganizerModel);
    if (itemId.isEmpty())
        return 0;

    return d->m_itemIdHash.value(itemId, 0);
}

/*!
  \qmlmethod list<string> OrganizerModel::itemIds(date start, date end)

  Returns the list of organizer item ids between the given date range \a start and \a end,
  excluding generated occurrences. Both the \a start and \a end parameters are optional,
  if no \a end parameter, returns all item ids from \a start, if neither start nor end date
  time provided, returns all item ids in the current model.

  \sa containsItems()
  */
QStringList QDeclarativeOrganizerModel::itemIds(const QDateTime &start, const QDateTime &end)
{
    Q_D(QDeclarativeOrganizerModel);
    //TODO: quick search this
    QStringList ids;
    if (!end.isNull()) {
        // both start date and end date are valid
        foreach (QDeclarativeOrganizerItem* item, d->m_items) {
            if (item->generatedOccurrence())
                continue;
            if ( (item->itemStartTime() >= start && item->itemStartTime() <= end)
                 || (item->itemEndTime() >= start && item->itemEndTime() <= end)
                 || (item->itemEndTime() > end && item->itemStartTime() < start))
                ids << item->itemId();
        }
    } else if (!start.isNull()) {
        // only a valid start date is valid
        foreach (QDeclarativeOrganizerItem* item, d->m_items) {
            if (!item->generatedOccurrence() && item->itemStartTime() >= start)
                ids << item->itemId();
        }
    } else {
        // neither start nor end date is valid
        foreach (QDeclarativeOrganizerItem* item, d->m_items) {
            if (!item->generatedOccurrence())
                ids << item->itemId();
        }
    }
    return ids;
}

void QDeclarativeOrganizerModel::fetchAgain()
{
    Q_D(QDeclarativeOrganizerModel);
    cancelUpdate();

    d->m_fetchRequest  = new QOrganizerItemFetchRequest(this);
    d->m_fetchRequest->setManager(d->m_manager);
    d->m_fetchRequest->setSorting(d->m_sortOrders);
    d->m_fetchRequest->setStartDate(d->m_startPeriod);
    d->m_fetchRequest->setEndDate(d->m_endPeriod);
    d->m_fetchRequest->setStorageLocations(d->m_storageLocations);

    if (d->m_filter){
        d->m_fetchRequest->setFilter(d->m_filter->filter());
    } else {
        d->m_fetchRequest->setFilter(QOrganizerItemFilter());
    }
    d->m_fetchRequest->setFetchHint(d->m_fetchHint ? d->m_fetchHint->fetchHint() : QOrganizerItemFetchHint());

    connect(d->m_fetchRequest, SIGNAL(stateChanged(QOrganizerAbstractRequest::State)), this, SLOT(requestUpdated()));
    d->m_fetchRequest->start();
}

/*
  This slot function is connected with item fetch requests and item occurrence fetch requests,
  so the QObject::sender() must be checked for the right sender type.
  During update() function, the fetchAgain() will be invoked, inside fetchAgain(), a QOrganizerItemFetchRequest object
  is created and started, when this fetch request finished, this requestUpdate() slot will be invoked for the first time.
  Then check each of the organizer items returned by the item fetch request, if the item is a recurrence item,
  a QOrganizerItemOccurrenceFetchRequest object will be created and started. When each of these occurrence fetch requests
  finishes, this requestUpdated() slot will be invoked again and insert the returned occurrence items into the d->m_items
  list.
  */
void QDeclarativeOrganizerModel::requestUpdated()
{
    Q_D(QDeclarativeOrganizerModel);
    QList<QOrganizerItem> items;
    QOrganizerItemFetchRequest* ifr = qobject_cast<QOrganizerItemFetchRequest*>(QObject::sender());
    if (ifr && ifr->isFinished()) {
        items = ifr->items();
        checkError(ifr);
        ifr->deleteLater();
        d->m_fetchRequest = 0;
        d->m_updatePending = false;
    } else {
        return;
    }

    if (!items.isEmpty() || !d->m_items.isEmpty() || d->m_initialUpdate) {
        // full update: first go through new items and check if they
        // existed earlier. if they did, use the existing declarative wrapper.
        // otherwise create new declarative item.
        // for occurrences new declarative item is always created.
        QList<QDeclarativeOrganizerItem *> newList;
        QHash<QString, QDeclarativeOrganizerItem *> newItemIdHash;
        QHash<QString, QDeclarativeOrganizerItem *>::iterator iterator;
        QOrganizerItem item;
        QString idString;
        QDeclarativeOrganizerItem *declarativeItem;
        d->m_initialUpdate = false;

        int i;
        for (i = 0; i < items.size(); i++) {
            item = items[i];
            idString = item.id().toString();
            if (item.id().isNull()) {
                // this is occurrence
                declarativeItem = createItem(item);
            } else {
                iterator = d->m_itemIdHash.find(idString);
                if (iterator != d->m_itemIdHash.end()) {
                    declarativeItem = iterator.value();
                    declarativeItem->setItem(item);
                } else {
                    declarativeItem = createItem(item);
                }
                newItemIdHash.insert(idString, declarativeItem);
            }
            newList.append(declarativeItem);
        }

        // go through old items and delete items, which are not part of the
        // new item set. delete also all old occurrences.
        for (i = 0; i < d->m_items.size(); i++) {
            // FIXME: avoid unnecessary usage of item getter which copies all details...
            if (d->m_items[i]->item().id().isNull()) {
                d->m_items[i]->deleteLater();
            } else {
                iterator = newItemIdHash.find(d->m_items[i]->itemId());
                if (iterator == newItemIdHash.end())
                    d->m_items[i]->deleteLater();
            }
        }
        beginResetModel();
        d->m_items = newList;
        endResetModel();

        d->m_itemIdHash = newItemIdHash;
        emit modelChanged();
    }
}

/*!
  \qmlmethod OrganizerModel::saveItem(OrganizerItem item, StorageLocation storageLocation = UserDataStorage)

  Saves asynchronously the given \a item into the organizer backend. The location for storing item
  can be defined with optional \a storageLocation for new items. If optional \a storageLocation is not given, item
  will be stored to UserDataStorage. When item is updated, ie saved again, \a storageLocation is ignored and
  item is saved to the same location where it was originally saved.

  */
void QDeclarativeOrganizerModel::saveItem(QDeclarativeOrganizerItem* di, QDeclarativeOrganizerModel::StorageLocation storageLocation)
{
    Q_D(QDeclarativeOrganizerModel);
    if (di) {
        QOrganizerItem item = di->item();
        QOrganizerItemSaveRequest* req = new QOrganizerItemSaveRequest(this);
        req->setManager(d->m_manager);
        req->setItem(item);
        req->setStorageLocation(QOrganizerAbstractRequest::StorageLocation(storageLocation));

        connect(req, SIGNAL(stateChanged(QOrganizerAbstractRequest::State)), this, SLOT(onRequestStateChanged(QOrganizerAbstractRequest::State)));

        req->start();
    }
}

/*!
  \qmlmethod OrganizerModel::removeItem(string itemId)
  Removes the organizer item with the given \a itemId from the backend.

  */
void QDeclarativeOrganizerModel::removeItem(const QString& id)
{
    QList<QString> ids;
    ids << id;
    removeItems(ids);
}

/*!
  \qmlmethod OrganizerModel::removeItem(OrganizerItem item)
  Removes the given organizer \a item from the backend.
  */
void QDeclarativeOrganizerModel::removeItem(QDeclarativeOrganizerItem *item)
{
    Q_D(QDeclarativeOrganizerModel);
    QOrganizerItemRemoveRequest* req = new QOrganizerItemRemoveRequest(this);
    req->setManager(d->m_manager);
    req->setItem(item->item());
    connect(req, SIGNAL(stateChanged(QOrganizerAbstractRequest::State)), this, SLOT(onRequestStateChanged(QOrganizerAbstractRequest::State)));
    req->start();
}

/*!
  \qmlmethod OrganizerModel::removeItems(list<string> itemId)
  Removes asynchronously the organizer items with the given \a ids from the backend.

  */
void QDeclarativeOrganizerModel::removeItems(const QStringList& ids)
{
    Q_D(QDeclarativeOrganizerModel);
    QOrganizerItemRemoveByIdRequest* req = new QOrganizerItemRemoveByIdRequest(this);
    req->setManager(d->m_manager);
    QList<QOrganizerItemId> oids;

    // FIXME: no special format for occurrence ids
    foreach (const QString& id, ids) {
        if (id.startsWith(QString("qtorganizer:occurrence"))) {
            qmlInfo(this) << tr("Can't remove an occurrence item, please modify the parent item's recurrence rule instead!");
            continue;
        }
        QOrganizerItemId itemId = QOrganizerItemId::fromString(id);
        if (!itemId.isNull()) {
             oids.append(itemId);
        }
    }

    req->setItemIds(oids);

    connect(req, SIGNAL(stateChanged(QOrganizerAbstractRequest::State)), this, SLOT(onRequestStateChanged(QOrganizerAbstractRequest::State)));

    req->start();
}

/*!
  \qmlmethod OrganizerModel::removeItems(list<OrganizerItem> items)
  Removes asynchronously the organizer items in the given \a items list from the backend.
  */
void QDeclarativeOrganizerModel::removeItems(const QList<QDeclarativeOrganizerItem> &items)
{
    Q_D(QDeclarativeOrganizerModel);
    QOrganizerItemRemoveRequest* req = new QOrganizerItemRemoveRequest(this);
    req->setManager(d->m_manager);
    QList<QOrganizerItem> ois;

    for (int i = 0; i < items.size(); i++) {
        ois.append(items[i].item());
    }

    req->setItems(ois);
    connect(req, SIGNAL(stateChanged(QOrganizerAbstractRequest::State)), this, SLOT(onRequestStateChanged(QOrganizerAbstractRequest::State)));
    req->start();
}

/*!
    \internal
 */
void QDeclarativeOrganizerModel::onRequestStateChanged(QOrganizerAbstractRequest::State newState)
{
    if (newState != QOrganizerAbstractRequest::FinishedState || !sender())
        return;

    QOrganizerAbstractRequest *request = qobject_cast<QOrganizerAbstractRequest *>(sender());
    if (!request)
        return;

    checkError(request);
    request->deleteLater();
}

void QDeclarativeOrganizerModel::removeItemsFromModel(const QList<QString> &itemIds)
{
    Q_D(QDeclarativeOrganizerModel);
    bool emitSignal = false;
    bool itemIdFound = false;

    foreach (const QString &itemId, itemIds) {
        itemIdFound = false;
        // generated occurrences are not in m_itemIdHash
        if (d->m_itemIdHash.remove(itemId) > 0)
            itemIdFound = true;
        for (int i = d->m_items.count() - 1; i >= 0; i--) {
            if (itemIdFound) {
                if (d->m_items.at(i)->itemId() == itemId) {
                    beginRemoveRows(QModelIndex(), i, i);
                    d->m_items.removeAt(i);
                    endRemoveRows();
                    emitSignal = true;
                    break;
                }
            } else if (d->m_items.at(i)->generatedOccurrence()) {
                QDeclarativeOrganizerItemDetail *parentDetail = d->m_items.at(i)->detail(QDeclarativeOrganizerItemDetail::Parent);
                if (parentDetail->value(QDeclarativeOrganizerItemParent::FieldParentId).toString() == itemId) {
                    beginRemoveRows(QModelIndex(), i, i);
                    d->m_items.removeAt(i);
                    endRemoveRows();
                    emitSignal = true;
                }
            }
        }
    }
    if (emitSignal)
        emit modelChanged();
}


/*!
    \internal

    It's invoked upon the QOrganizerManager::itemsModified() signal.
 */
void QDeclarativeOrganizerModel::onItemsModified(const QList<QPair<QOrganizerItemId, QOrganizerManager::Operation> > &itemIds)
{
    Q_D(QDeclarativeOrganizerModel);
    if (!d->m_autoUpdate)
        return;

    QSet<QOrganizerItemId> addedAndChangedItems;
    QList<QString> removedItems;
    for (int i = itemIds.size() - 1; i >= 0; i--) {
        if (itemIds[i].second == QOrganizerManager::Remove) {
            // check that item has not been added after removing it
            if (!addedAndChangedItems.contains(itemIds[i].first))
                removedItems.append(itemIds[i].first.toString());
        } else {
            addedAndChangedItems.insert(itemIds[i].first);
        }
    }
    if (!removedItems.isEmpty())
        removeItemsFromModel(removedItems);

    if (!addedAndChangedItems.isEmpty()) {
        // FIXME; to be optimized with fetching only the modified items
        // from the storage locations modified items are on
        QOrganizerItemFetchRequest *fetchRequest = new QOrganizerItemFetchRequest(this);
        connect(fetchRequest, SIGNAL(stateChanged(QOrganizerAbstractRequest::State)),
                this, SLOT(onItemsModifiedFetchRequestStateChanged(QOrganizerAbstractRequest::State)));
        fetchRequest->setManager(d->m_manager);
        fetchRequest->setStartDate(d->m_startPeriod);
        fetchRequest->setEndDate(d->m_endPeriod);
        fetchRequest->setFilter(d->m_filter ? d->m_filter->filter() : QOrganizerItemFilter());
        fetchRequest->setSorting(d->m_sortOrders);
        fetchRequest->setFetchHint(d->m_fetchHint ? d->m_fetchHint->fetchHint() : QOrganizerItemFetchHint());
        fetchRequest->setStorageLocations(d->m_storageLocations);
        d->m_notifiedItems.insert(fetchRequest, addedAndChangedItems);

        fetchRequest->start();
    }
}

/*!
    \internal

    It's invoked by the fetch request from onItemsModified().
 */
void QDeclarativeOrganizerModel::onItemsModifiedFetchRequestStateChanged(QOrganizerAbstractRequest::State state)
{
    // NOTE: this function assumes the sorting algorithm gives always the same result with
    // same data. E.g. items which have the identical sorting key must be sorted too.

    Q_D(QDeclarativeOrganizerModel);
    if (state != QOrganizerAbstractRequest::FinishedState || !sender())
        return;

    QOrganizerItemFetchRequest *request = qobject_cast<QOrganizerItemFetchRequest *>(sender());
    if (!request)
        return;

    checkError(request);

    QSet<QOrganizerItemId> notifiedItems = d->m_notifiedItems.value(request);
    if (notifiedItems.isEmpty())
        return;

    if (request->error() == QOrganizerManager::NoError) {
        bool emitSignal = false;
        QList<QOrganizerItem> fetchedItems = request->items();
        QOrganizerItem oldItem;
        QOrganizerItem newItem;
        QOrganizerItemParent oldParentDetail;
        QOrganizerItemParent newParentDetail;
        QDeclarativeOrganizerItem *declarativeItem;
        QSet<QOrganizerItemId> removedIds;
        QSet<QOrganizerItemId> addedIds;
        int oldInd = 0;
        int newInd = 0;
        while (newInd < fetchedItems.size()) {
            bool addNewItem = false;
            bool oldItemExists = false;

            newItem = fetchedItems[newInd];
            if (oldInd < d->m_items.size()) {
                // quick check if items are same in old and new event lists
                // FIXME: avoid unnecessary usage of item getter which copies all details
                oldItem = d->m_items[oldInd]->item();
                oldItemExists = true;
                if (!newItem.id().isNull() && !oldItem.id().isNull() && newItem.id() == oldItem.id()) {
                    if (notifiedItems.contains(newItem.id())) {
                        d->m_items[oldInd]->setItem(newItem);
                        emitSignal = true;
                    }
                    newInd++;
                    oldInd++;
                    continue;
                }
            }

            // check should we remove old item
            if (oldItemExists) {
                if (oldItem.id().isNull()) {
                    // this is generated occurrence
                    oldParentDetail = oldItem.detail(QOrganizerItemDetail::TypeParent);
                    if (notifiedItems.contains(oldParentDetail.parentId())) {
                        beginRemoveRows(QModelIndex(), oldInd, oldInd);
                        d->m_items.takeAt(oldInd)->deleteLater();
                        endRemoveRows();
                        emitSignal = true;
                        continue;
                    }
                } else if (notifiedItems.contains(oldItem.id())) {
                    // if notifiedItems contains the oldItem id, it means the item has been
                    // changed and we should reuse the declarative part and only remove
                    // rows from abstract list model
                    // it might also mean that oldItem has been changed so that it does not belong to
                    // the model anymore (e.g. changing fron normal item to recurring item)
                    beginRemoveRows(QModelIndex(), oldInd, oldInd);
                    d->m_items.removeAt(oldInd);
                    endRemoveRows();
                    removedIds.insert(oldItem.id());
                    emitSignal = true;
                    continue;
                }
            }
            // check should we add the new item
            if (newItem.id().isNull() && (newItem.type() == QOrganizerItemType::TypeEventOccurrence || newItem.type() == QOrganizerItemType::TypeTodoOccurrence)) {
                // this is occurrence (generated or exception)
                newParentDetail = newItem.detail(QOrganizerItemDetail::TypeParent);
                if (notifiedItems.contains(newParentDetail.parentId())) {
                    declarativeItem = createItem(newItem);
                    addNewItem = true;
                }
            } else if (notifiedItems.contains(newItem.id())) {
                QHash<QString, QDeclarativeOrganizerItem *>::const_iterator iterator = d->m_itemIdHash.find(newItem.id().toString());
                if (iterator == d->m_itemIdHash.end()) {
                    declarativeItem = createItem(newItem);
                    d->m_itemIdHash.insert(declarativeItem->itemId(), declarativeItem);
                } else {
                    declarativeItem = d->m_itemIdHash.value(newItem.id().toString());
                    addedIds.insert(newItem.id());
                }
                addNewItem = true;
            }

            if (addNewItem) {
                beginInsertRows(QModelIndex(), oldInd, oldInd);
                d->m_items.insert(oldInd, declarativeItem);
                endInsertRows();
                emitSignal = true;
            }
            oldInd++;
            newInd++;
        }
        // remove the rest of the old items
        if (oldInd <= d->m_items.size() - 1) {
            beginRemoveRows(QModelIndex(), oldInd, d->m_items.size() - 1);
            while (oldInd < d->m_items.size()) {
                d->m_itemIdHash.remove(d->m_items[oldInd]->itemId());
                d->m_items.takeAt(oldInd)->deleteLater();
                emitSignal = true;
                oldInd++;
            }
            endRemoveRows();
        }
        // remove items which were changed so that they are no longer part of the model
        // they have been removed from the model earlier, but need to still be removed from the hash
        // and deleted

        removedIds.subtract(addedIds);
        foreach (const QOrganizerItemId &id, removedIds) {
            QDeclarativeOrganizerItem *changedItem = d->m_itemIdHash.take(id.toString());
            if (changedItem) {
                changedItem->deleteLater();
                emitSignal = true;
            }
        }

        if (emitSignal)
            emit modelChanged();
    }
    d->m_notifiedItems.remove(request);
    request->deleteLater();
}

/*!
  \qmlmethod OrganizerModel::fetchCollections()
  Fetch asynchronously a list of organizer collections from the organizer backend.
  */
void QDeclarativeOrganizerModel::fetchCollections()
{
    Q_D(QDeclarativeOrganizerModel);
    // fetchCollections() is used for both direct calls and
    // signals from model. For signal from model, check also the
    // autoupdate-flag.
    if (sender() == d->m_manager && !d->m_autoUpdate) {
        return;
    }

    QOrganizerCollectionFetchRequest* req = new QOrganizerCollectionFetchRequest(this);
    req->setManager(d->m_manager);
    req->setStorageLocations(d->m_storageLocations);

    connect(req,SIGNAL(stateChanged(QOrganizerAbstractRequest::State)), this, SLOT(collectionsFetched()));

    req->start();
}

void QDeclarativeOrganizerModel::collectionsFetched()
{
    Q_D(QDeclarativeOrganizerModel);
    QOrganizerCollectionFetchRequest* req = qobject_cast<QOrganizerCollectionFetchRequest*>(QObject::sender());
    if (req->isFinished() && QOrganizerManager::NoError == req->error()) {
        // prepare tables
        QHash<QString, const QOrganizerCollection*> collections;
        foreach (const QOrganizerCollection& collection, req->collections()) {
            collections.insert(collection.id().toString(), &collection);
        }
        QHash<QString, QDeclarativeOrganizerCollection*> declCollections;
        foreach(QDeclarativeOrganizerCollection* declCollection, d->m_collections) {
            declCollections.insert(declCollection->collection().id().toString(), declCollection);
        }
        // go tables through
        QHashIterator<QString, const QOrganizerCollection*> collIterator(collections);
        while (collIterator.hasNext()) {
            collIterator.next();
            if (declCollections.contains(collIterator.key())) {
                // collection on both sides, update the declarative collection
                declCollections.value(collIterator.key())->setCollection(*collections.value(collIterator.key()));
            } else {
                // new collection, add it to declarative collection list
                QDeclarativeOrganizerCollection* declCollection = new QDeclarativeOrganizerCollection(this);
                declCollection->setCollection(*collections.value(collIterator.key()));
                d->m_collections.append(declCollection);
            }
        }
        QHashIterator<QString, QDeclarativeOrganizerCollection*> declCollIterator(declCollections);
        while (declCollIterator.hasNext()) {
            declCollIterator.next();
            if (!collections.contains(declCollIterator.key())) {
                // collection deleted on the backend side, delete from declarative collection list
                QDeclarativeOrganizerCollection* toBeDeletedColl = declCollections.value(declCollIterator.key());
                d->m_collections.removeOne(toBeDeletedColl);
                delete toBeDeletedColl;
            }
        }
        emit collectionsChanged();
        QMetaObject::invokeMethod(this, "fetchAgain", Qt::QueuedConnection);
        req->deleteLater();
    }
    checkError(req);
}

/*!
  \qmlmethod OrganizerModel::saveCollection(Collection collection, StorageLocation storageLocation = UserDataStorage)

  Saves asynchronously the given \a collection into the organizer backend. The location for storing collection
  can be defined with optional \a storageLocation for new collections. If optional \a storageLocation is not given,
  collection will be stored to UserDataStorage. When collection is updated, ie saved again, \a storageLocation is
  ignored and collection is saved to the same location where it was originally saved.

  */
void QDeclarativeOrganizerModel::saveCollection(QDeclarativeOrganizerCollection* declColl, QDeclarativeOrganizerModel::StorageLocation storageLocation)
{
    Q_D(QDeclarativeOrganizerModel);
    if (declColl) {
        QOrganizerCollection collection = declColl->collection();
        QOrganizerCollectionSaveRequest* req = new QOrganizerCollectionSaveRequest(this);
        req->setManager(d->m_manager);
        req->setCollection(collection);
        req->setStorageLocation(QOrganizerAbstractRequest::StorageLocation(storageLocation));

        connect(req, SIGNAL(stateChanged(QOrganizerAbstractRequest::State)), this, SLOT(onRequestStateChanged(QOrganizerAbstractRequest::State)));

        req->start();
    }
}

/*!
  \qmlmethod OrganizerModel::removeCollection(string collectionId)
  Removes asynchronously the organizer collection with the given \a collectionId from the backend.
  */
void QDeclarativeOrganizerModel::removeCollection(const QString &collectionId)
{
    Q_D(QDeclarativeOrganizerModel);
    QOrganizerCollectionRemoveRequest* req = new QOrganizerCollectionRemoveRequest(this);
    req->setManager(d->m_manager);
    req->setCollectionId(QOrganizerCollectionId::fromString(collectionId));

    connect(req, SIGNAL(stateChanged(QOrganizerAbstractRequest::State)), this, SLOT(onRequestStateChanged(QOrganizerAbstractRequest::State)));

    req->start();
}

/*!
  \qmlmethod Collection OrganizerModel::defaultCollection()
  Returns the default Collection object.
  */
QDeclarativeOrganizerCollection* QDeclarativeOrganizerModel::defaultCollection()
{
    Q_D(QDeclarativeOrganizerModel);
    return collection(d->m_manager->defaultCollection().id().toString());
}

/*!
  \qmlmethod Collection OrganizerModel::collection(string collectionId)
  Returns the Collection object which collection id is the given \a collectionId and
  null if collection id is not found.
  */
QDeclarativeOrganizerCollection* QDeclarativeOrganizerModel::collection(const QString &collectionId)
{
    Q_D(QDeclarativeOrganizerModel);
    foreach (QDeclarativeOrganizerCollection* collection, d->m_collections) {
        if (collection->id() == collectionId)
            return collection;
    }
    return 0;
}

QVariant QDeclarativeOrganizerModel::data(const QModelIndex &index, int role) const
{
    Q_D(const QDeclarativeOrganizerModel);
    //Check if QList itme's index is valid before access it, index should be between 0 and count - 1
    if (index.row() < 0 || index.row() >= d->m_items.count()) {
        return QVariant();
    }

    QDeclarativeOrganizerItem* di = d->m_items.at(index.row());
    Q_ASSERT(di);
    QOrganizerItem item = di->item();
    switch(role) {
        case Qt::DisplayRole:
            return item.displayLabel();
        case Qt::DecorationRole:
            //return pixmap for this item type
        case OrganizerItemRole:
            return QVariant::fromValue(di);
    }
    return QVariant();
}

/*!
  \qmlproperty list<OrganizerItem> OrganizerModel::items

  This property holds a list of organizer items in the organizer model.

  \sa OrganizerItem
  */
QQmlListProperty<QDeclarativeOrganizerItem> QDeclarativeOrganizerModel::items()
{
    return QQmlListProperty<QDeclarativeOrganizerItem>(this, 0, item_append, item_count, item_at);
}

/*!
  \qmlproperty list<Collection> OrganizerModel::collections

  This property holds a list of collections in the organizer model.

  \sa Collection
  */
QQmlListProperty<QDeclarativeOrganizerCollection> QDeclarativeOrganizerModel::collections()
{
    return QQmlListProperty<QDeclarativeOrganizerCollection>(this, 0, collection_append, collection_count, collection_at);
}

void QDeclarativeOrganizerModel::item_append(QQmlListProperty<QDeclarativeOrganizerItem> *p, QDeclarativeOrganizerItem *item)
{
    Q_UNUSED(p);
    Q_UNUSED(item);
    qmlInfo(0) << tr("OrganizerModel: appending items is not currently supported");
}

int  QDeclarativeOrganizerModel::item_count(QQmlListProperty<QDeclarativeOrganizerItem> *p)
{
    QDeclarativeOrganizerModel* model = qobject_cast<QDeclarativeOrganizerModel*>(p->object);
    if (model)
        return model->d_ptr->m_items.count();
    return 0;
}

QDeclarativeOrganizerItem * QDeclarativeOrganizerModel::item_at(QQmlListProperty<QDeclarativeOrganizerItem> *p, int idx)
{
    QDeclarativeOrganizerModel* model = qobject_cast<QDeclarativeOrganizerModel*>(p->object);
    if (model && idx >= 0 && idx < model->d_ptr->m_items.size())
        return model->d_ptr->m_items.at(idx);
    return 0;
}

void QDeclarativeOrganizerModel::sortOrder_append(QQmlListProperty<QDeclarativeOrganizerItemSortOrder> *p, QDeclarativeOrganizerItemSortOrder *sortOrder)
{
    QDeclarativeOrganizerModel* model = qobject_cast<QDeclarativeOrganizerModel*>(p->object);
    if (model && sortOrder) {
        QObject::connect(sortOrder, SIGNAL(sortOrderChanged()), model, SIGNAL(sortOrdersChanged()));
        model->d_ptr->m_declarativeSortOrders.append(sortOrder);
        model->d_ptr->m_sortOrders.append(sortOrder->sortOrder());
        emit model->sortOrdersChanged();
    }
}

int  QDeclarativeOrganizerModel::sortOrder_count(QQmlListProperty<QDeclarativeOrganizerItemSortOrder> *p)
{
    QDeclarativeOrganizerModel* model = qobject_cast<QDeclarativeOrganizerModel*>(p->object);
    if (model)
        return model->d_ptr->m_declarativeSortOrders.size();
    return 0;
}
QDeclarativeOrganizerItemSortOrder * QDeclarativeOrganizerModel::sortOrder_at(QQmlListProperty<QDeclarativeOrganizerItemSortOrder> *p, int idx)
{
    QDeclarativeOrganizerModel* model = qobject_cast<QDeclarativeOrganizerModel*>(p->object);

    QDeclarativeOrganizerItemSortOrder* sortOrder = 0;
    if (model) {
        int i = 0;
        foreach (QDeclarativeOrganizerItemSortOrder* s, model->d_ptr->m_declarativeSortOrders) {
            if (i == idx) {
                sortOrder = s;
                break;
            } else {
                i++;
            }
        }
    }
    return sortOrder;
}
void  QDeclarativeOrganizerModel::sortOrder_clear(QQmlListProperty<QDeclarativeOrganizerItemSortOrder> *p)
{
    QDeclarativeOrganizerModel* model = qobject_cast<QDeclarativeOrganizerModel*>(p->object);

    if (model) {
        model->d_ptr->m_sortOrders.clear();
        model->d_ptr->m_declarativeSortOrders.clear();
        emit model->sortOrdersChanged();
    }
}

void QDeclarativeOrganizerModel::collection_append(QQmlListProperty<QDeclarativeOrganizerCollection> *p, QDeclarativeOrganizerCollection *collection)
{
    Q_UNUSED(p);
    Q_UNUSED(collection);
    qmlInfo(0) << tr("OrganizerModel: appending collections is not currently supported");
}

int  QDeclarativeOrganizerModel::collection_count(QQmlListProperty<QDeclarativeOrganizerCollection> *p)
{
    QDeclarativeOrganizerModel* model = qobject_cast<QDeclarativeOrganizerModel*>(p->object);
    return model ? model->d_ptr->m_collections.count() : 0;
}

QDeclarativeOrganizerCollection* QDeclarativeOrganizerModel::collection_at(QQmlListProperty<QDeclarativeOrganizerCollection> *p, int idx)
{
    QDeclarativeOrganizerModel* model = qobject_cast<QDeclarativeOrganizerModel*>(p->object);
    QDeclarativeOrganizerCollection* collection = 0;
    if (model) {
        if (!model->d_ptr->m_collections.isEmpty() && idx >= 0 && idx < model->d_ptr->m_collections.count())
            collection = model->d_ptr->m_collections.at(idx);
    }
    return collection;
}

#include "moc_qdeclarativeorganizermodel_p.cpp"

QTORGANIZER_END_NAMESPACE
