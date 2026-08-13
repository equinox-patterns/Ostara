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
#include "../vmisc/vvalentinasettings.h"
#include "../vexception/vexception.h"
#include "../vpatterndb/vpiece.h"
#include "../vwidgets/vabstractsimple.h"
#include "../vwidgets/vgraphicssimpletextitem.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vsegmentlabel.h"
#include "../vwidgets/vsimplecurve.h"
#include "../vwidgets/vsimplepoint.h"
#include "../vtools/tools/vabstracttool.h"
#include "mainwindowsnogui.h"
#include "core/vapplication.h"

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
auto SortDetailsForLayout(const QHash<quint32, VPiece> *allDetails, const QString &nameRegex)
    -> QVector<DetailForLayout>
{
    QVector<DetailForLayout> details;
    details.reserve(allDetails->size());

    if (auto i = allDetails->constBegin(); nameRegex.isEmpty())
    {
        while (i != allDetails->constEnd())
        {
            if (i.value().IsInLayout())
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

    // 3. Remove labelPrefix attribute from root
    if (root.hasAttribute(QStringLiteral("labelPrefix")))
    {
        foundLabels = true;
        root.removeAttribute(QStringLiteral("labelPrefix"));
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

        // 6. In <data> child of <detail>: remove label-specific attributes and <line> children
        const QDomNodeList dataElements = detail.elementsByTagName(QStringLiteral("data"));
        for (int j = 0; j < dataElements.size(); ++j)
        {
            QDomElement dataEl = dataElements.at(j).toElement();
            if (dataEl.isNull())
            {
                continue;
            }
            // Remove <line> template children
            const QDomNodeList lines = dataEl.elementsByTagName(QStringLiteral("line"));
            for (int k = lines.size() - 1; k >= 0; --k)
            {
                foundLabels = true;
                dataEl.removeChild(lines.at(k));
            }
            // Remove label-specific attributes from <data>
            static const QStringList labelAttrs = {
                QStringLiteral("orientation"), QStringLiteral("rotationWay"),
                QStringLiteral("tilt"), QStringLiteral("foldPosition"),
                QStringLiteral("visible"), QStringLiteral("fontSize"),
                QStringLiteral("mx"), QStringLiteral("my"),
                QStringLiteral("width"), QStringLiteral("height"),
                QStringLiteral("rotation"), QStringLiteral("centerPin"),
                QStringLiteral("topLeftPin"), QStringLiteral("bottomRightPin")
            };
            for (const auto &attr : labelAttrs)
            {
                if (dataEl.hasAttribute(attr))
                {
                    foundLabels = true;
                    dataEl.removeAttribute(attr);
                }
            }
        }

        // 7. Remove <patternInfo> elements (pattern info labels)
        const QDomNodeList patternInfos = detail.elementsByTagName(QStringLiteral("patternInfo"));
        for (int j = patternInfos.size() - 1; j >= 0; --j)
        {
            foundLabels = true;
            detail.removeChild(patternInfos.at(j));
        }
    }

    return foundLabels;
}
