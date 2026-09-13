/************************************************************************
 **
 **  @file   ogradedsizetrace.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/09/2026
 **
 **  @brief  The graded size-style pass: the trace color and the pt→px dashes one
 **          size of a graded scene carries, applied to a silhouette subtree.
 **
 **  @copyright
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
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

#include "ogradedsizetrace.h"

#include "mainwindow_graded_helpers.h"

#include "../vlayout/vgraphicsfillitem.h"
#include "../vmisc/defglobal.h"
#include "../vtools/tools/oannotationitem.h"
#include "../vtools/tools/oinfoblockitem.h"
#include "../vwidgets/vgrainlineitem.h"

#include <QBrush>
#include <QGraphicsPathItem>
#include <QGraphicsSimpleTextItem>
#include <QPen>
#include <QVariant>

namespace
{
//---------------------------------------------------------------------------------------------------------------------
// Re-style one layout item subtree (the silhouette tree returned by
// VLayoutPiece::GetItem). A size style (when present) recolors and re-dashes the
// trace items of that size; items excluded from the style (grainline, fold line,
// info blocks, annotations) keep the theme piece color — the style applies to the
// traces, never to the decor. On a scene that outlives the pass, the first override
// of an item stores its native pen (kGradedBasePenRole) so a later "no style /
// gate off" pass can restore it.
void StyleSubtree(QGraphicsItem *item, const QColor &defaultColor, const OGradedSizeStyle *style,
                  bool liveScene)
{
    if (item == nullptr)
    {
        return;
    }
    // Interactive children of a graded piece/host resolve their own colors → leave
    // them untouched (and do not descend: their interior text/fills are theirs).
    const int type = item->type();
    if (type == OInfoBlockItem::Type || type == OAnnotationItem::Type || type == VGrainlineItem::Type)
    {
        return;
    }

    // Grainline / fold line are decor: never recolored by the size style. They still
    // follow the theme (default piece color) like every other trace.
    const bool decorTrace = (type == VGraphicsFillItem::Type || type == VGraphicsFoldLineItem::Type);

    if (auto *path = dynamic_cast<QGraphicsPathItem *>(item))
    {
        QPen pen = path->pen();
        if (pen.style() == Qt::NoPen)
        {
            // nothing drawn by this pen — nothing to restyle
        }
        else if (style != nullptr && style->HasColor() && !decorTrace)
        {
            // Remember the native pen (style/cap/width) before overriding it, once. Only
            // a scene that survives the pass has anything to remember it for: captured
            // items are thrown away with the private scene they were built in.
            if (liveScene && !item->data(kGradedBasePenRole).isValid())
            {
                item->setData(kGradedBasePenRole, pen);
            }

            pen.setColor(style->color);
            if (style->dashes.isEmpty())
            {
                pen.setStyle(Qt::SolidLine); // colored solid line
                pen.setCapStyle(Qt::SquareCap);
            }
            else
            {
                // Dashes are stored in pt (the Illustrator unit the user types);
                // a Qt pen pattern is in units of the pen width, so a dash becomes
                // pt × (px per pt) / width — a 1 pt dash is 1.333 px at scale 1:1.
                const qreal width = pen.widthF() > 0.0 ? pen.widthF() : 1.0;
                const qreal ptToPx = PrintDPI / 72.0;
                QVector<qreal> pattern;
                pattern.reserve(static_cast<int>(style->dashes.size()));
                for (qreal value : style->dashes)
                {
                    pattern.append(value * ptToPx / width);
                }
                pen.setStyle(Qt::CustomDashLine);
                pen.setDashPattern(pattern);
                pen.setCapStyle(Qt::FlatCap); // match the SVG export cap (butt)
            }
            path->setPen(pen);
        }
        else if (liveScene)
        {
            // Default (theme) branch: restore a native pen we may have overridden, then re-tint.
            const QVariant base = item->data(kGradedBasePenRole);
            if (base.isValid())
            {
                pen = base.value<QPen>();
                item->setData(kGradedBasePenRole, QVariant()); // purge: snapshot is self-healing
            }
            pen.setColor(defaultColor);
            path->setPen(pen);
        }
    }
    else if (auto *text = dynamic_cast<QGraphicsSimpleTextItem *>(item); liveScene && text != nullptr)
    {
        // Place-label text follows the theme color; size styles never recolor text.
        text->setBrush(QBrush(defaultColor));
    }

    const auto children = item->childItems();
    for (auto *child : children)
    {
        StyleSubtree(child, defaultColor, style, liveScene);
    }
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void OGradedSizeTrace::ApplyToSubtree(QGraphicsItem *root, const QHash<QString, OGradedSizeStyle> &styles,
                                      Scope scope, const QColor &defaultColor)
{
    if (root == nullptr)
    {
        return;
    }

    // The pointer targets an entry of the caller's @p styles hash, which is never mutated
    // nor rebuilt during this pass — it stays valid for the whole recursion.
    const OGradedSizeStyle *style = nullptr;
    if (!styles.isEmpty())
    {
        const QString sizeValue = root->data(kGradedSizeRole).toString();
        const auto it = styles.constFind(sizeValue);
        if (it != styles.cend() && it->HasColor())
        {
            style = &it.value();
        }
    }

    StyleSubtree(root, defaultColor, style, scope == Scope::LiveScene);
}
