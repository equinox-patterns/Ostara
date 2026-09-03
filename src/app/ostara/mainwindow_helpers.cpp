/************************************************************************
 **
 **  @file   mainwindow_helpers.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Helper function implementations extracted from mainwindow.cpp
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

#include "mainwindow_helpers.h"

#include <QApplication>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNodeList>
#include <QFileInfo>
#include <QPointer>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>

#include "vtheme.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vexception/vexception.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../vwidgets/vabstractsimple.h"
#include "../vwidgets/vgraphicssimpletextitem.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vsegmentlabel.h"
#include "../vwidgets/scalesceneitems.h"
#include "../vwidgets/vcurvepathitem.h"
#include "../vwidgets/vsimplecurve.h"
#include "../vwidgets/vsimplepoint.h"
#include "../vwidgets/vscenepoint.h"
#include "../vtools/tools/vabstracttool.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "mainwindowsnogui.h"

#include <QtDebug>

using namespace Qt::Literals::StringLiterals;

//---------------------------------------------------------------------------------------------------------------------
auto AutosavePrefix() -> const QString &
{
    static const QString prefix = QStringLiteral(".autosave");
    return prefix;
}

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wmissing-prototypes")

Q_LOGGING_CATEGORY(vMainWindow, "v.mainwindow") // NOLINT

QT_WARNING_POP

//---------------------------------------------------------------------------------------------------------------------
void LogPatternToolUsed(bool checked, const QString &toolName)
{
    // VGAnalyticsLib removed — no tracking
    Q_UNUSED(checked)
    Q_UNUSED(toolName)
}

//---------------------------------------------------------------------------------------------------------------------
auto SortDetailsForLayout(const QHash<quint32, VPiece> *allDetails, const VContainer *data,
                          const QString &nameRegex) -> QVector<DetailForLayout>
{
    QVector<DetailForLayout> details;
    details.reserve(allDetails->size());

    const auto isInLayout = [data](const VPiece &piece, quint32 id) -> bool
    {
        // A member piece's in-layout state follows its GROUP (a group behaves like one piece):
        // toggling the group to "x" excludes the whole group from export.
        const quint32 groupId = data != nullptr ? data->GetPieceGroupId(id) : NULL_ID;
        if (groupId != NULL_ID)
        {
            const auto *groups = data->DataPieceGroups();
            return groups != nullptr && groups->contains(groupId) && groups->value(groupId).IsInLayout();
        }
        return piece.IsInLayout();
    };

    if (auto i = allDetails->constBegin(); nameRegex.isEmpty())
    {
        while (i != allDetails->constEnd())
        {
            if (isInLayout(i.value(), i.key()))
            {
                details.append({.id = i.key(), .piece = i.value()});
            }

            ++i;
        }
    }
    else
    {
        const QRegularExpression nameRe(nameRegex);
        while (i != allDetails->constEnd())
        {
            if (nameRe.match(i.value().GetName()).hasMatch())
            {
                details.append({.id = i.key(), .piece = i.value()});
            }

            ++i;
        }
    }

    return details;
}

//---------------------------------------------------------------------------------------------------------------------
void WarningNotUniquePieceName(const QHash<quint32, VPiece> *allDetails)
{
    auto i = allDetails->constBegin();
    QSet<QString> uniqueNames;

    while (i != allDetails->constEnd())
    {
        if (const QString pieceName = i.value().GetName(); not uniqueNames.contains(pieceName))
        {
            uniqueNames.insert(pieceName);
        }
        else
        {
            const QString errorMsg = QObject::tr("Piece name '%1' is not unique.").arg(pieceName);
            VAbstractApplication::VApp()->IsPedantic()
                ? throw VException(errorMsg)
                : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
        }

        ++i;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ResolveToolCursorResource(const QString &resource, const QString &cursor) -> QString
{
    auto cursorResource = VTheme::GetResourceName(resource, cursor);

    if (qApp->devicePixelRatio() >= 2) // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
    {
        const auto hiDPICursor = QString(cursor).replace(".png"_L1, "@2x.png"_L1);
        const auto hiDPIResource = VTheme::GetResourceName(resource, hiDPICursor);
        if (QFileInfo::exists(hiDPIResource))
        {
            cursorResource = hiDPIResource;
        }
    }

    return cursorResource;
}

//---------------------------------------------------------------------------------------------------------------------
auto ResolveRubberBandTarget(const QGraphicsItem *item) -> QGraphicsItem *
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    auto *target = const_cast<QGraphicsItem *>(item);

    // Redirect label text items to their parent
    if (item->type() == VGraphicsSimpleTextItem::Type)
    {
        target = item->parentItem();
        if (target == nullptr)
        {
            return nullptr;
        }
    }

    // Skip leader lines and other non-selectable VSegmentLabel children.
    if (target->type() != VSimplePoint::Type && target->type() != VSimpleCurve::Type &&
        target->type() != VSegmentLabel::Type)
    {
        if (const QGraphicsItem *parent = target->parentItem();
            parent != nullptr && parent->type() == VSegmentLabel::Type)
        {
            return nullptr;
        }
    }

    return target;
}

//---------------------------------------------------------------------------------------------------------------------
auto MakeRubberBandEntry(QGraphicsItem *target) -> std::pair<quint32, TriggerFn>
{
    if (target->type() == VSimplePoint::Type || target->type() == VSimpleCurve::Type)
    {
        auto *simple = dynamic_cast<VAbstractSimple *>(target);
        if (simple == nullptr)
        {
            return {};
        }
        QPointer<VAbstractSimple> const ptr(simple);
        return {simple->GetId(), [ptr]() -> void { if (ptr) { ptr->TriggerChoosed(); } }};
    }

    if (target->type() == VSegmentLabel::Type)
    {
        auto *segLabel = static_cast<VSegmentLabel *>(target);
        QPointer<VSegmentLabel> const ptr(segLabel);
        return {segLabel->GetSegmentId(), [ptr]() -> void { if (ptr) { ptr->TriggerChoosed(); } }};
    }

    if (auto *tool = dynamic_cast<VAbstractTool *>(target))
    {
        QPointer<VAbstractTool> const ptr(tool);
        return {tool->getId(), [ptr]() -> void { if (ptr) { ptr->TriggerChoosed(); } }};
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
void HandleRubberBandSelection(const VMainGraphicsScene *scene, const QRectF &sceneRect,
                               const std::shared_ptr<QMap<quint32, TriggerFn>> &prevBandTriggers)
{
    QMap<quint32, TriggerFn> currentTriggers;

    for (const QGraphicsItem *item : scene->items(sceneRect, Qt::IntersectsItemShape))
    {
        QGraphicsItem *target = ResolveRubberBandTarget(item);
        if (target == nullptr)
        {
            continue;
        }

        auto [id, trigger] = MakeRubberBandEntry(target);
        if (id != NULL_ID && not currentTriggers.contains(id))
        {
            currentTriggers.insert(id, trigger);
        }
    }

    // Fire trigger for items newly entering the rubber band
    for (auto it = currentTriggers.cbegin(); it != currentTriggers.cend(); ++it)
    {
        if (not prevBandTriggers->contains(it.key()))
        {
            it.value()();
        }
    }

    // Fire trigger for items that left the rubber band (toggles them back off)
    for (auto it = prevBandTriggers->cbegin(); it != prevBandTriggers->cend(); ++it)
    {
        if (not currentTriggers.contains(it.key()))
        {
            it.value()();
        }
    }

    *prevBandTriggers = std::move(currentTriggers);
}

//---------------------------------------------------------------------------------------------------------------------
auto StripLabelElements(QDomDocument *doc) -> bool
{
    if (doc == nullptr)
    {
        return false;
    }

    bool foundLabels = false;
    QDomElement root = doc->documentElement();
    if (root.isNull())
    {
        return false;
    }

    // 1. Remove <patternLabel> element (and its <line> children, dateFormat, timeFormat)
    const QDomNodeList patternLabels = root.elementsByTagName(QStringLiteral("patternLabel"));
    if (not patternLabels.isEmpty())
    {
        foundLabels = true;
        // Remove all matching elements (iterate backwards to avoid index issues)
        for (int i = patternLabels.size() - 1; i >= 0; --i)
        {
            QDomNode node = patternLabels.at(i);
            if (node.parentNode() == root)
            {
                root.removeChild(node);
            }
        }
    }

    // 2. Remove <pieceLabel> element
    const QDomNodeList pieceLabels = root.elementsByTagName(QStringLiteral("pieceLabel"));
    if (not pieceLabels.isEmpty())
    {
        foundLabels = true;
        for (int i = pieceLabels.size() - 1; i >= 0; --i)
        {
            QDomNode node = pieceLabels.at(i);
            if (node.parentNode() == root)
            {
                root.removeChild(node);
            }
        }
    }

    // 5. Remove gradationLabel attribute from <detail> elements
    const QDomNodeList details = root.elementsByTagName(QStringLiteral("detail"));
    for (int i = 0; i < details.size(); ++i)
    {
        QDomElement detail = details.at(i).toElement();
        if (detail.isNull())
        {
            continue;
        }
        if (detail.hasAttribute(QStringLiteral("gradationLabel")))
        {
            foundLabels = true;
            detail.removeAttribute(QStringLiteral("gradationLabel"));
        }

        // 6. Remove <data> elements (piece label data). Valentina serializes an empty, invisible <data> on
        //    every piece, so presence alone must NOT flag the warning — only a block carrying real label text
        //    (template <line>s, visible, or a non-empty letter/annotation) does.
        const QDomNodeList dataElements = detail.elementsByTagName(QStringLiteral("data"));
        for (int j = dataElements.size() - 1; j >= 0; --j)
        {
            const QDomElement dataElement = dataElements.at(j).toElement();
            const bool hasLabelText = not dataElement.isNull() &&
                                      (not dataElement.elementsByTagName(QStringLiteral("line")).isEmpty() ||
                                       dataElement.attribute(QStringLiteral("visible")) == QStringLiteral("true") ||
                                       not dataElement.attribute(QStringLiteral("letter")).isEmpty() ||
                                       not dataElement.attribute(QStringLiteral("annotation")).isEmpty());
            if (hasLabelText)
            {
                foundLabels = true;
            }
            detail.removeChild(dataElements.at(j));
        }

        // 7. Remove <patternInfo> elements (pattern info labels) — likewise only flagged when actually visible.
        const QDomNodeList patternInfos = detail.elementsByTagName(QStringLiteral("patternInfo"));
        for (int j = patternInfos.size() - 1; j >= 0; --j)
        {
            const QDomElement infoElement = patternInfos.at(j).toElement();
            if (not infoElement.isNull() && infoElement.attribute(QStringLiteral("visible")) == QStringLiteral("true"))
            {
                foundLabels = true;
            }
            detail.removeChild(patternInfos.at(j));
        }
    }

    // 8. Migrate obsolete "showLabel"/"showLabel1"/"showLabel2" point attributes (renamed "showPointName" in Ostara).
    //    These only control whether a point's name is displayed — they are not label text — so stripping them must NOT
    //    flag the labels-not-supported dialog: an old .val with shown point names would otherwise warn on a label-free
    //    pattern (old Valentina files serialize showLabel="true" on ordinary points).
    for (const QString &tag : {QStringLiteral("point"), QStringLiteral("item")})
    {
        const QDomNodeList nodes = root.elementsByTagName(tag);
        for (int i = 0; i < nodes.size(); ++i)
        {
            QDomElement elem = nodes.at(i).toElement();
            if (elem.isNull())
            {
                continue;
            }
            if (elem.hasAttribute(QStringLiteral("showLabel")) || elem.hasAttribute(QStringLiteral("showLabel1")) ||
                elem.hasAttribute(QStringLiteral("showLabel2")))
            {
                elem.removeAttribute(QStringLiteral("showLabel"));
                elem.removeAttribute(QStringLiteral("showLabel1"));
                elem.removeAttribute(QStringLiteral("showLabel2"));
            }
        }
    }

    return foundLabels;
}

//---------------------------------------------------------------------------------------------------------------------
void RefreshSceneItemPens(const VMainGraphicsScene *scene)
{
    // Scale- and theme-dependent item state must be refreshed here, not inside paint(). Since Qt 6
    // mutating geometry/visibility/selection during paint() crashes the paint pipeline on Windows.
    for (QGraphicsItem *item : scene->items())
    {
        // VGraphicsSimpleTextItem and VToolSeamAllowance have a single fixed type(), so a cheap
        // int compare + static_cast is enough. VScenePoint is a base class whose subclasses
        // override type(), so it can only be matched with dynamic_cast.
        if (item->type() == VGraphicsSimpleTextItem::Type)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            static_cast<VGraphicsSimpleTextItem *>(item)->RefreshScale();
        }
        else if (item->type() == VToolSeamAllowance::Type)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            static_cast<VToolSeamAllowance *>(item)->RefreshScale();
        }
        else if (auto *line = dynamic_cast<VScaledLine *>(item))
        {
            line->RefreshPen();
        }
        else if (auto *ellipse = dynamic_cast<VScaledEllipse *>(item))
        {
            ellipse->RefreshScale();
        }
        else if (auto *curve = dynamic_cast<VCurvePathItem *>(item))
        {
            curve->RefreshScale();
        }
        else if (auto *point = dynamic_cast<VScenePoint *>(item))
        {
            point->RefreshScale();
        }
    }
}
