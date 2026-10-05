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

class VContainer;
class VAbstractPattern;

class QGraphicsItem;
class QDomDocument;
class VMainGraphicsScene;
class VPiece;

struct DetailForLayout;

Q_DECLARE_LOGGING_CATEGORY(vMainWindow)

using TriggerFn = std::function<void()>;

auto SortDetailsForLayout(const QHash<quint32, VPiece> *allDetails, const VContainer *data,
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

// Strip pin nodes and neutralize pin-anchored grainlines in a pattern DOM.
// Returns true if at least one grainline had to be disabled.
auto StripPinElements(QDomDocument *doc) -> bool;

// Strip the legacy piece "short name" attribute from a pattern DOM. Ostara has no short name: the piece area
// variables are named from the piece name. Only <detail> elements are touched.
// Returns true if the attribute was found.
auto StripShortNameAttributes(QDomDocument *doc) -> bool;

// Strip the legacy piece "forbid flipping" attribute from a pattern DOM. Ostara has no such option: a piece is only
// mirrored when "force flipping" asks for it. Only <detail> elements are touched.
// Returns true if the attribute was found.
auto StripForbidFlippingAttributes(QDomDocument *doc) -> bool;

// Strip the legacy piece "follow grainline" attribute from a pattern DOM. Ostara has no such option: following the
// grainline is an obligation of the cutting plan, not a setting. Only <detail> elements are touched.
// Returns true if the attribute was found.
auto StripFollowGrainlineAttributes(QDomDocument *doc) -> bool;

// Convert legacy "built-in seam allowance" pieces to plain pieces with the seam allowance disabled, and drop the
// attribute. Ostara has no built-in mode: it was exactly a piece without a seam allowance.
// Returns true if at least one piece carried the attribute.
auto StripBuiltInSeamAllowanceAttributes(QDomDocument *doc) -> bool;

// Convert the fold-line types Ostara dropped to the default one, so a file that still carries one stays saveable
// against the narrowed schema. Silent by design — the change is invisible to the user.
// Returns true if at least one value was converted.
auto ConvertRemovedFoldLineTypes(QDomDocument *doc) -> bool;

// Strip the legacy "show copies" attribute of the slash-and-spread from a pattern DOM. Ostara dropped the checkbox,
// and the save schema no longer declares the attribute, so dropping it at load is what keeps a pattern an older Ostara
// build wrote saveable. Only <operation> elements are touched.
// Returns true if the attribute was found.
auto StripShowCopiesAttributes(QDomDocument *doc) -> bool;

// Drop the layer items no tool of the piece can produce: a pair whose tool the file no longer holds, and a pair
// whose OBJECT the tool does not define — the second being anything but inert, since a single point ignores the
// object id it is handed and the pairs are applied in ascending key order, so a stale key outvotes the real one.
// Silent by design, run once on load, BEFORE the layers are normalised so the orphans are counted on clean data.
// Returns true if at least one item was dropped.
auto CleanStaleLayerItems(VAbstractPattern *doc) -> bool;

// Give every object of every piece a layer. An object outside every layer is visible by default and
// unreachable from the Layers dock, so a pattern written before layers existed (a .val, or an .ostara
// carrying objects no layer references) is given ONE automatic layer per piece — the one the dock shows as
// "Unnamed" / "Sans nom" — holding exactly the objects that belong to none yet. It is recognised by its
// marker (VAbstractPattern::AttrAuto), never by its name, which is translated and which the user may change.
// Silent by design — it runs once, on load, for both formats.
// Returns true if at least one layer was created or completed.
auto NormalizeUnlayeredObjects(VAbstractPattern *doc) -> bool;

// Make sure every piece has exactly one active layer — the one the canvas draws into. The layer the file saved as
// active wins; failing that the automatic layer, and failing that the first layer of the piece. Without this a
// pattern written before layers had an active one would leave the canvas with no target to draw into.
// Returns true if a flag was set or cleared.
auto NormalizeActiveLayer(VAbstractPattern *doc) -> bool;

// Refresh scale- and theme-dependent item state (labels, pens) for a whole scene.
void RefreshSceneItemPens(const VMainGraphicsScene *scene);

// Mark (or unmark) an item and everything below it as part of a layer's content: points, lines and curves drawn
// magenta and bold. The state lives on each renderer — Qt's own selection only draws a dashed frame, which is not
// what the layer workflow shows.
void SetLayerObjectsInLayer(QGraphicsItem *item, bool inLayer);

// Mark (or unmark) an item and everything below it as SELECTED for the layer workflow: orange and bold, the colour
// that says what Enter is about to do — add it, or take it out. Orange wins over magenta, because an object can be
// both at once: in the layer being emptied, and picked to leave it.
void SetLayerObjectsSelected(QGraphicsItem *item, bool selected);

#endif // MAINWINDOW_HELPERS_H
