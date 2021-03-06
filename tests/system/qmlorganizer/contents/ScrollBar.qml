/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0

Item {
    id: scrollBar
    // The properties that define the scrollbar's state.
    // position and pageSize are in the range 0.0 - 1.0.  They are relative to the
    // height of the page, i.e. a pageSize of 0.5 means that you can see 50%
    // of the height of the view.
    // orientation can be either 'Vertical' or 'Horizontal'
    property real position
    property real pageSize
    property var orientation : "Vertical"
    property alias bgColor: background.color
    property alias fgColor: thumb.color

    // A light, semi-transparent background
    Rectangle {
        id: background
        radius: orientation == 'Vertical' ? (width/2 - 1) : (height/2 - 1)
        color: "white"; opacity: 0.3
        anchors.fill: parent
    }
    // Size the bar to the required size, depending upon the orientation.
    Rectangle {
        id: thumb
        opacity: 0.7
        color: "black"
        radius: orientation == 'Vertical' ? (width/2 - 1) : (height/2 - 1)
        x: orientation == 'Vertical' ? 1 : (scrollBar.position * (scrollBar.width-2) + 1)
        y: orientation == 'Vertical' ? (scrollBar.position * (scrollBar.height-2) + 1) : 1
        width: orientation == 'Vertical' ? (parent.width-2) : (scrollBar.pageSize * (scrollBar.width-2))
        height: orientation == 'Vertical' ? (scrollBar.pageSize * (scrollBar.height-2)) : (parent.height-2)
    }
}
