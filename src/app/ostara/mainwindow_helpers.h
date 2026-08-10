/************************************************************************
 **
 **  @file   mainwindow_helpers.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Helper functions and types extracted from mainwindow.cpp
 **
 **  @note   Extracted from mainwindow.cpp by Sophie Denys <contact@sophiedenys.com>
 **          as part of Ostara refactoring.
 **
 **  @copyright
 **  Copyright (C) 2013-2015 Valentina project
 **
 **  This program is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#ifndef MAINWINDOW_HELPERS_H
#define MAINWINDOW_HELPERS_H

#include <QHash>
#include <QMap>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QtGlobal>
#include <functional>
#include <memory>
#include <utility>

#include "../ifc/xml/utils.h"
#include "../vmisc/def.h"

class QGraphicsItem;
class QDomDocument;
class VMainGraphicsScene;
class VPiece;

struct DetailForLayout;

Q_DECLARE_LOGGING_CATEGORY(vMainWindow)

using TriggerFn = std::function<void()>;

void LogPatternToolUsed(bool checked, const QString &toolName);
auto SortDetailsForLayout(const QHash<quint32, VPiece> *allDetails,
                          const QString &nameRegex = {}) -> QVector<DetailForLayout>;
void WarningNotUniquePieceName(const QHash<quint32, VPiece> *allDetails);
auto ResolveToolCursorResource(const QString &resource, const QString &cursor) -> QString;
auto ResolveRubberBandTarget(const QGraphicsItem *item) -> QGraphicsItem *;
auto MakeRubberBandEntry(QGraphicsItem *target) -> std::pair<quint32, TriggerFn>;
void HandleRubberBandSelection(const VMainGraphicsScene *scene, const QRectF &sceneRect,
                               const std::shared_ptr<QMap<quint32, TriggerFn>> &prevBandTriggers);

// Accessor for autosave prefix used by AutoSavePattern and ReopenFilesAfterCrash
auto AutosavePrefix() -> const QString &;

// Strip Valentina label elements from a pattern DOM. Returns true if any labels were found.
auto StripLabelElements(QDomDocument *doc) -> bool;

#endif // MAINWINDOW_HELPERS_H
