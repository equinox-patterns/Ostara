/************************************************************************
 **
 **  @file   otooldartcap.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  the cap of a dart: the point that closes it on the other side
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

#ifndef OTOOLDARTCAP_H
#define OTOOLDARTCAP_H

#include <QDomElement>
#include <QGraphicsItem>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QSharedPointer>
#include <QString>
#include <QtGlobal>

#include "../../../../dialogs/tools/dialogtool.h"
#include "toollinepoint/vtoollinepoint.h"

class VMainGraphicsScene;
class VPointF;
class VScaledLine;

//! What the tool needs to build a cap: the three points of the dart, the point of the line to extend, and
//! the two names of the tracés it adds.
struct OToolDartCapInitData : VToolLinePointInitData
{
    quint32 apexId{NULL_ID};
    quint32 firstPointId{NULL_ID};
    quint32 secondPointId{NULL_ID};
    quint32 extendPointId{NULL_ID};
    QString secondLineRoleName{};
    bool drawCap{true};
};

/**
 * @brief The cap of a dart: the point C of the triangle P1-C-P2, isosceles on the opening of the dart
 * P1-P-P2, found where the line of the pattern (shoulder line, side curve) meets the axis of the dart.
 *
 * The rule itself lives in ODartCapRule, shared with the dialog so that the two cannot disagree. What this
 * class adds is the dart cap as a pattern object: the point, plus the two tracés that make it readable —
 * C→P1 (the inherited mainLine) and C→P2 (m_secondLine), one style and one colour for both.
 */
class OToolDartCap : public VToolLinePoint
{
    Q_OBJECT // NOLINT

public:
    ~OToolDartCap() override = default;

    void SetDialog() override;

    static auto Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                       VContainer *data) -> OToolDartCap *;
    static auto Create(OToolDartCapInitData initData) -> OToolDartCap *;

    static const QString ToolType;

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Tool::DartCap)
    };

    //! The four points the cap is built from, by name, for the dialog and the tooltip.
    auto ApexName() const -> QString;
    auto FirstPointName() const -> QString;
    auto SecondPointName() const -> QString;
    auto ExtendPointName() const -> QString;

    //! The name of the tracé that goes from the cap point to one of the two points of the opening. The pair
    //! is read cap point first, in the direction of the tracé: Line_C_P1 and Line_C_P2.
    auto GetLine1RoleName() const -> QString;
    void SetLine1RoleName(const QString &suffix);
    auto GetLine2RoleName() const -> QString;
    void SetLine2RoleName(const QString &suffix);

    //! Whether the two tracés of the cap are drawn: false when the user asked for the point alone.
    auto IsCapDrawn() const -> bool;
    void SetCapDrawn(bool drawn);

    void ShowVisualization(bool show) override;

protected slots:
    void ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id = NULL_ID) override;

protected:
    void SaveDialog(QDomElement &domElement) override;
    void SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj) override;
    void ReadToolAttributes(const QDomElement &domElement) override;
    void SetVisualization() override;
    void RefreshGeometry() override;
    auto MakeToolTip() const -> QString override;
    void ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement) override;

private:
    Q_DISABLE_COPY_MOVE(OToolDartCap) // NOLINT

    quint32 m_apexId{NULL_ID};
    quint32 m_firstPointId{NULL_ID};
    quint32 m_secondPointId{NULL_ID};
    quint32 m_extendPointId{NULL_ID};

    /** @brief m_drawCap whether the two tracés are drawn. Held in the file: without it the choice would be
     *  forgotten as soon as the pattern is reopened. */
    bool m_drawCap{true};

    /** @brief m_secondLine the tracé C→P2. The first one, C→P1, is the mainLine inherited from
     *  VToolLinePoint. */
    VScaledLine *m_secondLine{nullptr};

    explicit OToolDartCap(const OToolDartCapInitData &initData, QGraphicsItem *parent = nullptr);

    //! The pair of points that names a tracé, cap point first: "C_P1" and "C_P2".
    auto Line1PairKey() const -> QString;
    auto Line2PairKey() const -> QString;
    void SetLineRoleNameForPair(const QString &pairKey, const QString &suffix);

    //! Give the two tracés their place, their one pen, and hide them when the cap is not drawn.
    void RefreshCapGeometry();
};

#endif // OTOOLDARTCAP_H
