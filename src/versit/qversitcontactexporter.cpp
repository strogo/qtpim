/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtVersit module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qversitcontactexporter.h"
#include "qversitcontactexporter_p.h"

QT_BEGIN_NAMESPACE_VERSIT

/*!
  \deprecated
  \class QVersitContactExporterDetailHandler
  \brief The QVersitContactExporterDetailHandler class is the legacy interface for clients wishing
  to implement custom export behaviour for certain contact details.

  This interface is replaced by QVersitContactExporterDetailHandlerV2.  For general information on
  extending Qt Versit, see the document on \l{Qt Versit Plugins}.


  \sa QVersitContactExporter
 */

/*!
  \fn QVersitContactExporterDetailHandler::~QVersitContactExporterDetailHandler()
  Frees any memory in use by this handler.
 */

/*!
  \fn bool QVersitContactExporterDetailHandler::preProcessDetail(const QContact& contact, const QContactDetail& detail, QVersitDocument* document)
  Process \a detail and update \a document with the corresponding QVersitProperty(s).
  \a contact provides the context within which the detail was found.

  Returns true if the detail has been handled and requires no further processing, false otherwise.

  This function is called on every QContactDetail encountered during an export.  Supply this
  function and return true to implement custom export behaviour.
 */

/*!
  \fn bool QVersitContactExporterDetailHandler::postProcessDetail(const QContact& contact, const QContactDetail& detail, bool alreadyProcessed, QVersitDocument* document)
  Process \a detail and update \a document with the corresponding QVersitProperty(s).
  \a contact provides the context within which the detail was found.
  \a alreadyProcessed is true if the detail has already been processed either by
  \l preProcessDetail() or by QVersitContactExporter itself.

  Returns true if the detail has been handled, false otherwise.

  This function is called on every \l QContactDetail encountered during an export.  This can be
  used to implement support for QContactDetails not supported by QVersitContactExporter.
 */

/*!
  \class QVersitContactExporterDetailHandlerV2
  \brief The QVersitContactExporterDetailHandlerV2 class is an interface for specifying custom
  export behaviour for certain contact details.
  \ingroup versit-extension
  \inmodule QtVersit

  This interface supercedes QVersitContactExporterDetailHandler.  For general information on
  extending Qt Versit, see the document on \l{Qt Versit Plugins}.


  \sa QVersitContactExporter
 */

/*!
  \fn QVersitContactExporterDetailHandlerV2::~QVersitContactExporterDetailHandlerV2()
  Frees any memory in use by this handler.
 */

/*!
  \fn void QVersitContactExporterDetailHandlerV2::detailProcessed(const QContact& contact, const QContactDetail& detail, const QVersitDocument& document, QSet<int>* processedFields, QList<QVersitProperty>* toBeRemoved, QList<QVersitProperty>* toBeAdded)

  Process \a detail and provide a list of updated \l{QVersitProperty}{QVersitProperties} by
  modifying the \a toBeRemoved and \a toBeAdded lists.

  This function is called on every QContactDetail encountered during an export, after the detail has
  been processed by the QVersitContactExporter.  An implementation of this function can be made to
  provide support for QContactDetails not supported by QVersitContactExporter.

  The supplied \a contact is the container for the \a detail.  \a processedFields contains a list of
  fields in the \a detail that were considered by the QVersitContactExporter or another handler in
  processing the detail.  \a document holds the state of the document before the detail was
  processed by the exporter.

  \a toBeRemoved and \a toBeAdded are initially filled with a list of properties that the exporter
  will remove from and add to the document.  These lists can be modified (by removing, modifying or
  adding properties) by the handler to control the changes that will actually be made to the
  document.  If a property is to be modified in the document, the old version will appear in the
  \a toBeRemoved list and the new version will appear in the \a toBeAdded list.  When the handler
  uses a field from the detail, it should update the processedFields set to reflect this to inform
  later handlers that the field has already been processed.

  After the handler returns control back to the exporter, the properties in the \a toBeRemoved
  list will be removed and the properties in the \a toBeAdded list will be appended to the document.
 */

/*!
  \fn void QVersitContactExporterDetailHandlerV2::contactProcessed(const QContact& contact, QVersitDocument* document)
  Perform any final processing on the \a document generated by the \a contact.  This can be
  implemented by the handler to clear any internal state before moving onto the next contact.

  This function is called after all QContactDetails have been handled by the
  QVersitContactExporter.
*/

/*!
  \class QVersitContactExporter
  \brief The QVersitContactExporter class converts \l {QContact}{QContacts} into
  \l {QVersitDocument}{QVersitDocuments}.
  \ingroup versit
  \inmodule QtVersit

  This class is used to convert lists of \l {QContact}{QContacts} (which may be stored in a
  QContactManager) into lists of \l {QVersitDocument}{QVersitDocuments} (which may be written to
  an I/O device using QVersitReader.  Unless there is an error, there is a one-to-one mapping
  between contacts and Versit documents.  The exporter can be extended by clients by associating
  resource and detail handlers.

  Here is a simple example of how to use QVersitContactExporter:
  \snippet qtversitdocsample/qtversitdocsample.cpp Export example

  \section1 Extension via handlers

  A \l QVersitResourceHandler is associated with the exporter to supply the behaviour for loading
  files from persistent storage.  By default, this is set to a \l QVersitDefaultResourceHandler,
  which supports basic resource loading from the file system.  An alternative resource handler
  can be specified with setResourceHandler().

  By associating a \l QVersitContactExporterDetailHandlerV2 with the exporter using
  setDetailHandler(), the client can pass in a handler to override the processing of details and/or
  handle details that QVersitContactExporter doesn't support.  Also, handlers can be implicitly
  associated to an exporter through the \l{Qt Versit Plugins}{handler plugin mechanism}.  The exporter
  can be constructed with a profile, which gives hints about what kind of handlers should be added
  to it.  For example, the backup profile can be used to instruct the exporter to encode any unknown
  details in the vCard such that it can be reconstructed later (a QVersitContactImporter constructed
  under the backup profile can be used to decode it).  To illustrate, a backup exporter can be
  constructed with:
  \code
  QVersitContactExporter exporter(QVersitContactHandlerFactory::ProfileBackup);
  \endcode
  For more details on how the backup plugin works, see \l{Qt Versit Plugins}

  \section1 Exporting group relationships
  The exporter does not handle QContactRelationships at all.

  Some managers use the \l{QContactRelationship::HasMember()}{HasMember} QContactRelationship along
  with contacts of type \l{QContactType::TypeGroup}{TypeGroup} to indicate categorization of
  contacts.  In vCard, categorization is represented by the CATEGORIES property, which has
  semantics most similar to the QContactTag detail.  For contact manager backends that supports
  groups but not QContactTag, if the categorization information needs to be retained through
  CATEGORIES vCard properties, extra work can be done to convert from group relationships to
  QContactTag before passing the contact list to the exporter.  Below is some example code that
  does this translation.

  \snippet qtversitdocsample/qtversitdocsample.cpp Export relationship example

  \section1 Other implementation notes

  Some Android devices do not properly handle the \c FN vCard property.  If a
  DisplayLabel is set on a contact and this class is used to generate a vCard, the receiving device
  might display this label in an unexpected field (eg. Last Name).
  The vCard specification requires the \c FN property to be present.  However, because of this
  interoperability issue, Qt Versit only generates an FN property if a Displaylabel is set.

  \sa QVersitDocument, QVersitProperty, QVersitResourceHandler, QVersitContactExporterDetailHandlerV2
 */

/*!
  \enum QVersitContactExporter::Error
  This enum specifies an error that occurred during the most recent call to exportContacts()
  \value NoError The most recent operation was successful
  \value EmptyContactError One of the contacts was empty
  \value NoNameError One of the contacts has no QContactName field
  */


/*!
 * Constructs a new contact exporter
 */
QVersitContactExporter::QVersitContactExporter()
    : d(new QVersitContactExporterPrivate())
{
}

/*!
 * Constructs a new exporter for the given \a profile.  The profile strings should be one of those
 * defined by QVersitContactHandlerFactory, or a value otherwise agreed to by a \l{Qt Versit
 * Plugins}{Versit plugin}.
 *
 * The profile determines which plugins will be loaded to supplement the exporter.
 */
QVersitContactExporter::QVersitContactExporter(const QString& profile)
{
    if (profile.isEmpty())
       d = new QVersitContactExporterPrivate(QStringList());
    else
       d = new QVersitContactExporterPrivate(QStringList() << profile);
}

/*!
 * Constructs a new exporter for the given \a profiles.  The profile strings should be one of those
 * defined by QVersitContactHandlerFactory, or a value otherwise agreed to by a \l{Qt Versit
 * Plugins}{Versit plugin}.
 *
 * The profiles determine which plugins will be loaded to supplement the exporter.
 */
QVersitContactExporter::QVersitContactExporter(const QStringList& profiles)
    : d(new QVersitContactExporterPrivate(profiles))
{
}

/*!
 * Frees any memory in use by this contact exporter.
 */
QVersitContactExporter::~QVersitContactExporter()
{
    delete d;
}

/*!
 * Converts \a contacts into a list of corresponding QVersitDocuments, using the format given by
 * \a versitType.
 *
 * Returns true on success.  If any of the contacts could not be exported, false is returned and
 * errorMap() will return a list describing the errors that occurred.  The successfully exported
 * documents will still be available via documents().
 *
 * \sa documents(), errorMap()
 */
bool QVersitContactExporter::exportContacts(
    const QList<QTCONTACTS_PREPEND_NAMESPACE(QContact)>& contacts,
    QVersitDocument::VersitType versitType)
{
    int contactIndex = 0;
    d->mDocuments.clear();
    d->mErrors.clear();
    bool ok = true;
    foreach (const QTCONTACTS_PREPEND_NAMESPACE(QContact)& contact, contacts) {
        if (contact.isEmpty()) {
            d->mErrors[contactIndex] = EmptyContactError;
            ok = false;
            continue;
        }

        QVersitDocument versitDocument;
        versitDocument.setType(versitType);
        versitDocument.setComponentType(QStringLiteral("VCARD"));
        d->exportContact(contact, versitDocument);
        d->mDocuments.append(versitDocument);
        contactIndex++;
    }

    return ok;
}

/*!
 * Returns the documents exported in the most recent call to exportContacts().
 *
 * \sa exportContacts()
 */
QList<QVersitDocument> QVersitContactExporter::documents() const
{
    return d->mDocuments;
}

/*!
 * \obsolete
 *
 * Use \l errorMap() instead.
 */
QMap<int, QVersitContactExporter::Error> QVersitContactExporter::errors() const
{
    return d->mErrors;
}

/*!
 * Returns the map of errors encountered in the most recent call to exportContacts().  The key is
 * the index into the input list of contacts and the value is the error that occurred on that
 * contact.
 *
 * \sa exportContacts()
 */
QMap<int, QVersitContactExporter::Error> QVersitContactExporter::errorMap() const
{
    return d->mErrors;
}

/*!
 * \deprecated
 * Sets \a handler to be the handler for processing QContactDetails, or 0 to have no handler.
 *
 * Does not take ownership of the handler.  The client should ensure the handler remains valid for
 * the lifetime of the exporter.  This function is used for version 1 handlers.
 *
 * Only one detail handler can be set.  If another detail handler (of any version) was
 * previously set, it will no longer be associated with the exporter.
 */
void QVersitContactExporter::setDetailHandler(QVersitContactExporterDetailHandler* handler)
{
    d->mDetailHandlerVersion = 1;
    d->mDetailHandler = handler;
    d->mDetailHandler2 = 0;
}

/*!
 * Sets \a handler to be the handler for processing QContactDetails, or 0 to have no handler.
 *
 * Does not take ownership of the handler.  The client should ensure the handler remains valid for
 * the lifetime of the exporter.  This function is used for version 2 and higher handlers.
 *
 * Only one detail handler can be set.  If another detail handler (of any version) was
 * previously set, it will no longer be associated with the exporter.
 */
void QVersitContactExporter::setDetailHandler(QVersitContactExporterDetailHandlerV2* handler)
{
    d->mDetailHandlerVersion = 2;
    d->mDetailHandler = 0;
    d->mDetailHandler2 = handler;
}

/*!
 * \deprecated
 * Gets the handler for processing QContactDetails.
 */
QVersitContactExporterDetailHandler* QVersitContactExporter::detailHandler() const
{
    return d->mDetailHandler;
}

/*!
 * Sets \a handler to be the handler to load files with, or 0 to have no handler.
 *
 * Does not take ownership of the handler.  The client should ensure the handler remains valid for
 * the lifetime of the exporter.
 */
void QVersitContactExporter::setResourceHandler(QVersitResourceHandler* handler)
{
    d->mResourceHandler = handler;
}

/*!
 * Returns the associated resource handler.
 */
QVersitResourceHandler* QVersitContactExporter::resourceHandler() const
{
    return d->mResourceHandler;
}

QT_END_NAMESPACE_VERSIT
