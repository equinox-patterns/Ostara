/************************************************************************
 **
 **  @file   mainwindow_graded_annotation.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/09/2026
 **
 **  @brief  Graded mode: interactive annotations on the reference layer (lone pieces + groups).
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
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "core/savegradedrangecontent.h"
#include "mainwindowsnogui.h"
#include "mainwindow_graded_helpers.h"
#include "xml/vpattern.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vformat/vmeasurements.h"
#include "../vmisc/def.h"
#include "../vpatterndb/calculator.h"
#include "../vtools/tools/oannotationitem.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vmisc/vabstractapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/vmaingraphicsscene.h"

#include <QGraphicsItem>
#include <QHash>
#include <QObject>
#include <QStringList>
#include <QtGlobal>
#include <QtMath>
#include <QUndoStack>

#include <algorithm>

//---------------------------------------------------------------------------------------------------------------------
namespace
{
/// Marker of an interactive annotation item (carries the owner piece/group id).
constexpr int kGradedAnnotationRole = Qt::UserRole + 906;


/// Deterministic fingerprint of an annotation list (order + template + text). Used to detect
/// a list change in Details (delete/reorder) so stale positional overrides are not mis-applied.
auto GradedAnnotationsFingerprint(const QList<OAnnotation> &annotations) -> QString
{
    QStringList parts;
    parts.reserve(annotations.size());
    for (const OAnnotation &annotation : annotations)
    {
        parts.append(annotation.templateName + QLatin1Char('#') + annotation.text.trimmed());
    }
    return parts.join(QLatin1Char('|'));
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::StoreGradedAnnotation(quint32 ownerId, bool isGroup, int index, const OAnnotationGeom &geom)
{
    const auto *ranges = pattern->SizeRanges();
    if (ranges == nullptr || !ranges->contains(m_activeGradedRangeName))
    {
        return;
    }
    const OSizerange oldRange = ranges->value(m_activeGradedRangeName);
    OSizerange newRange = oldRange;
    OSizerange::AnnotationOverride override;
    override.valid = true;
    override.geom = geom;

    // Record which annotation list these positional overrides were made against, so a later
    // Details delete/reorder can be detected and the stale overrides dropped.
    QList<OAnnotation> annotationList;
    if (isGroup)
    {
        const auto *groups = pattern->DataPieceGroups();
        if (groups != nullptr && groups->contains(ownerId))
        {
            annotationList = groups->value(ownerId).GetAnnotationData().Annotations();
        }
    }
    else
    {
        const auto *pieces = pattern->DataPieces();
        if (pieces != nullptr && pieces->contains(ownerId))
        {
            annotationList = pieces->value(ownerId).GetAnnotationData().Annotations();
        }
    }
    const QString fp = GradedAnnotationsFingerprint(annotationList);

    if (isGroup)
    {
        newRange.groupContent[ownerId].annotations.insert(index, override);
        newRange.groupContent[ownerId].annotationsFp = fp;
    }
    else
    {
        newRange.pieceContent[ownerId].annotations.insert(index, override);
        newRange.pieceContent[ownerId].annotationsFp = fp;
    }
    VAbstractApplication::VApp()->getUndoStack()->push(
        new SaveGradedRangeContent(doc, pattern, oldRange, newRange));
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::RefreshGradedReferenceAnnotations()
{
    if (m_sceneGraded == nullptr || pattern == nullptr || m_activeGradedRangeName.isEmpty())
    {
        return;
    }
    const auto *ranges = pattern->SizeRanges();
    if (ranges == nullptr || !ranges->contains(m_activeGradedRangeName))
    {
        return;
    }
    const OSizerange &range = ranges->value(m_activeGradedRangeName);

    // Drop previous interactive annotation items, then re-fetch the scene list.
    auto items = m_sceneGraded->items();
    for (auto *owner : items)
    {
        if (GradedPieceId(owner) == 0 && owner->data(kGradedHostGroupIdRole).isNull())
        {
            continue;
        }
        const auto children = owner->childItems();
        for (auto *child : children)
        {
            if (!child->data(kGradedAnnotationRole).isNull())
            {
                delete child;
            }
        }
    }
    items = m_sceneGraded->items();

    const auto *piecesMap = pattern->DataPieces();
    const auto *groups = pattern->DataPieceGroups();
    const OAnnotationStyleResolver &styleResolver = VToolSeamAllowance::AnnotationStyleResolver();

    // Rotation: fixed from the base annotation — evaluated like Details (piece formula via the
    // calculator over the pattern variables; group = literal degrees). Never stored/modified.
    auto RotationDeg = [this](const QString &formula, bool isGroup) -> qreal {
        if (isGroup)
        {
            bool ok = false;
            const qreal value = formula.toDouble(&ok);
            return ok ? value : 0.0;
        }
        if (formula.isEmpty())
        {
            return 0.0;
        }
        try
        {
            Calculator cal;
            const qreal value = cal.EvalFormula(pattern->DataVariables(), formula);
            return (qIsNaN(value) || qIsInf(value)) ? 0.0 : value;
        }
        catch (...)
        {
            return 0.0; // malformed rotation formula → fall back like Details (invalid → 0°)
        }
    };

    auto BuildOwnerAnnotations = [this, &range, &styleResolver, &RotationDeg](
                                     QGraphicsItem *owner, quint32 ownerId, bool isGroup,
                                     const QList<OAnnotation> &annotations) {
        const QPointF center = owner->boundingRect().center();
        const QString storedFp = isGroup ? range.groupContent.value(ownerId).annotationsFp
                                         : range.pieceContent.value(ownerId).annotationsFp;
        // If the annotation list changed in Details since the overrides were stored, drop them
        // (reset to Details positions) instead of mis-applying them by stale index.
        const bool overridesStale =
            !storedFp.isEmpty() && storedFp != GradedAnnotationsFingerprint(annotations);

        for (int i = 0; i < annotations.size(); ++i)
        {
            const OAnnotation &annotation = annotations.at(i);
            if (annotation.text.trimmed().isEmpty())
            {
                continue; // Details hides empty-text annotations
            }

            OSizerange::AnnotationOverride override;
            if (!overridesStale)
            {
                if (isGroup)
                {
                    const auto it = range.groupContent.constFind(ownerId);
                    if (it != range.groupContent.cend())
                    {
                        override = it->annotations.value(i);
                    }
                }
                else
                {
                    const auto it = range.pieceContent.constFind(ownerId);
                    if (it != range.pieceContent.cend())
                    {
                        override = it->annotations.value(i);
                    }
                }
            }

            OAnnotationGeom geom = annotation.geom;
            if (override.valid)
            {
                geom = override.geom;
            }
            if (qFuzzyIsNull(geom.pos.x()) && qFuzzyIsNull(geom.pos.y()))
            {
                geom.pos = center; // "never moved" sentinel → owner centre (Details rule)
            }

            OAnnotationRenderStyle style = styleResolver ? styleResolver(annotation.templateName)
                                                         : OAnnotationRenderStyle();
            const qreal rotationDeg = RotationDeg(annotation.rotationFormula, isGroup);

            auto *item = new OAnnotationItem(i, owner);
            item->setData(kGradedAnnotationRole, ownerId);
            item->SetIndex(i);
            item->SetContent(annotation.text, style);
            item->SetAnnotationGeom(geom, rotationDeg);
            item->show();

            connect(item, &OAnnotationItem::Moved, this,
                    [this, ownerId, isGroup, i, annotations](int, const QPointF &p) {
                        const auto *r = pattern->SizeRanges();
                        if (r == nullptr || !r->contains(m_activeGradedRangeName))
                        {
                            return;
                        }
                        const OAnnotationGeom baseGeom = annotations.value(i).geom;
                        const OSizerange cur = r->value(m_activeGradedRangeName);
                        const OSizerange::AnnotationOverride ov =
                            isGroup ? cur.groupContent.value(ownerId).annotations.value(i)
                                    : cur.pieceContent.value(ownerId).annotations.value(i);
                        OAnnotationGeom g = ov.valid ? ov.geom : baseGeom;
                        g.pos = p;
                        StoreGradedAnnotation(ownerId, isGroup, i, g);
                    });
            connect(item, &OAnnotationItem::Resized, this,
                    [this, item, ownerId, isGroup, i, annotations](int, qreal newW, qreal newH, qreal origW,
                                                                  qreal origH) {
                        const auto *r = pattern->SizeRanges();
                        if (r == nullptr || !r->contains(m_activeGradedRangeName))
                        {
                            return;
                        }
                        const OAnnotationGeom baseGeom = annotations.value(i).geom;
                        const OSizerange cur = r->value(m_activeGradedRangeName);
                        const OSizerange::AnnotationOverride ov =
                            isGroup ? cur.groupContent.value(ownerId).annotations.value(i)
                                    : cur.pieceContent.value(ownerId).annotations.value(i);
                        OAnnotationGeom g = ov.valid ? ov.geom : baseGeom;
                        g.pos = item->pos();
                        g.boxWidth = newW;
                        g.boxHeight = newH;
                        // fontScale only on a proportional (corner) scale — Details rule.
                        const qreal fontScale = (ov.valid && ov.geom.fontScale > 0.0) ? ov.geom.fontScale
                                                                                      : baseGeom.fontScale;
                        g.fontScale = fontScale > 0.0 ? fontScale : 1.0;
                        if (origW > 0.0 && qFuzzyCompare(newW / origW, newH / origH))
                        {
                            g.fontScale *= newW / origW; // proportional (corner) scale only
                        }
                        StoreGradedAnnotation(ownerId, isGroup, i, g);
                    });
        }
    };

    // Lone (non-grouped) reference pieces.
    for (auto *item : items)
    {
        if (!GradedIsReference(item))
        {
            continue;
        }
        const quint32 pieceId = GradedPieceId(item);
        if (pieceId == 0 || piecesMap == nullptr || !piecesMap->contains(pieceId))
        {
            continue;
        }
        if (pattern->GetPieceGroupId(pieceId) != NULL_ID)
        {
            continue; // grouped → the group hosts the (single) set of annotations
        }
        const VPiece &piece = piecesMap->value(pieceId);
        BuildOwnerAnnotations(item, pieceId, false, piece.GetAnnotationData().Annotations());
    }

    // Groups.
    for (auto *item : items)
    {
        if (item->data(kGradedHostGroupIdRole).isNull() || groups == nullptr)
        {
            continue;
        }
        const quint32 groupId = item->data(kGradedHostGroupIdRole).toUInt();
        if (!groups->contains(groupId))
        {
            continue;
        }
        const OPieceGroup &group = groups->value(groupId);
        BuildOwnerAnnotations(item, groupId, true, group.GetAnnotationData().Annotations());
    }
}
