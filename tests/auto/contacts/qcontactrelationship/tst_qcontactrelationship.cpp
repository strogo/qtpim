/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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

//TESTED_COMPONENT=src/contacts

#include <QtTest/QtTest>

#include <QtContacts/qcontacts.h>

QTCONTACTS_USE_NAMESPACE

static inline QContactId makeId(const QString &managerName, uint id)
{
    return QContactId(QStringLiteral("qtcontacts:%1:").arg(managerName), QString::number(id));
}


class tst_QContactRelationship: public QObject
{
Q_OBJECT

public:
    tst_QContactRelationship();
    virtual ~tst_QContactRelationship();

private slots:
    void operations();
    void emptiness();
    void hash();
    void datastream();
};

tst_QContactRelationship::tst_QContactRelationship()
{
}

tst_QContactRelationship::~tst_QContactRelationship()
{
}

void tst_QContactRelationship::operations()
{
    QContactRelationship r1;

    QContact contact1, contact2;

    QVERIFY(r1.first() == QContactId());
    QVERIFY(r1.second() == QContactId());
    QVERIFY(r1.relationshipType() == QString());

    r1.setFirst(contact1.id());
    QVERIFY(r1.first() == contact1.id());
    r1.setSecond(contact2.id());
    QVERIFY(r1.second() == contact2.id());
    QVERIFY(r1.first() == contact1.id());
    QVERIFY(r1.relationshipType() == QString());

    r1.setRelationshipType(QContactRelationship::HasSpouse());
    QVERIFY(r1.relationshipType() == QContactRelationship::HasSpouse());
}

void tst_QContactRelationship::emptiness()
{
    QContactRelationship r1, r2, r3;

    QContact contact1, contact2, contact3;
    contact1.setId(makeId("test", 1));
    contact2.setId(makeId("test", 2));
    contact3.setId(makeId("test", 3));

    QVERIFY(r1.first() == QContactId());
    QVERIFY(r1.second() == QContactId());
    QVERIFY(r1.relationshipType() == QString());

    r1.setFirst(contact1.id());
    QVERIFY(r1.first() == contact1.id());
    r1.setSecond(contact2.id());
    QVERIFY(r1.second() == contact2.id());
    QVERIFY(r1.first() == contact1.id());
    QVERIFY(r1.relationshipType() == QString());

    r1.setRelationshipType(QContactRelationship::HasSpouse());
    QVERIFY(r1.relationshipType() == QContactRelationship::HasSpouse());

    r2 = r1;
    QVERIFY(r1 == r2);
    QVERIFY(r1 != r3);
    QVERIFY(r2 != r3);

    r3.setFirst(contact3.id());
    r3.setSecond(contact2.id());
    r3.setRelationshipType(QContactRelationship::HasAssistant());

    r2.setFirst(contact3.id());
    QVERIFY(r1 != r2);
    QVERIFY(r2 != r3);
    QVERIFY(r3 != r1);
}

void tst_QContactRelationship::hash()
{
    QContactRelationship r1;
    QContact contact1;
    contact1.setId(makeId("a", 1));
    r1.setFirst(contact1.id());
    QContact contact2;
    contact2.setId(makeId("b", 2));
    r1.setSecond(contact2.id());
    r1.setRelationshipType(QContactRelationship::HasMember());

    QContactRelationship r2;
    r2.setFirst(contact1.id());
    r2.setSecond(contact2.id());
    r2.setRelationshipType(QContactRelationship::HasMember());

    QContactRelationship r3;
    r3.setFirst(contact1.id());
    QContact contact3;
    contact1.setId(makeId("c", 3));
    r3.setSecond(contact3.id());
    r3.setRelationshipType(QContactRelationship::HasMember());

    QVERIFY(qHash(r1) == qHash(r2));
    QVERIFY(qHash(r1) != qHash(r3));

}

void tst_QContactRelationship::datastream()
{
    QByteArray buffer;
    QDataStream stream1(&buffer, QIODevice::WriteOnly);
    QContactRelationship relationshipIn;
    QContact contact1;
    contact1.setId(makeId("a", 1));
    relationshipIn.setFirst(contact1.id());
    QContact contact2;
    contact2.setId(makeId("a", 2));
    relationshipIn.setSecond(contact2.id());
    relationshipIn.setRelationshipType(QContactRelationship::HasMember());
    stream1 << relationshipIn;

    QVERIFY(buffer.size() > 0);

    QDataStream stream2(buffer);
    QContactRelationship relationshipOut;
    stream2 >> relationshipOut;
    QCOMPARE(relationshipOut, relationshipIn);
}

QTEST_MAIN(tst_QContactRelationship)
#include "tst_qcontactrelationship.moc"
