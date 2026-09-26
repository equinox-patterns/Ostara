/************************************************************************
 **
 **  @file   otoolclosedart.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  The tool that closes a dart: a rotation whose angle is not typed but read from the dart itself.
 **
 **          A dart is an isosceles triangle P1-P-P2. Turning P2 around P by the angle the two legs make brings
 **          it exactly onto P1, which is what closing a dart is; the same angle opened up somewhere else is
 **          what moving a dart is. The tool therefore asks for three points — the pivot, the point that
 **          travels, the point it lands on — instead of the formula AngleLine_P_P1 - AngleLine_P_P2, and it
 **          keeps only those three: the angle is derived, never stored, and the length a rotation can be told
 **          by has no meaning here.
 **
 **          A dart is not always closed all the way: a divisor leaves the legs apart, and 1 — the whole
 **          opening — is the ordinary case. That divisor is the only thing the user types.
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

#ifndef OTOOLCLOSEDART_H
#define OTOOLCLOSEDART_H

#include <QMap>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/def.h"
#include "vabstractoperation.h"

template <class T> class QSharedPointer;
class VFormula;

struct OToolCloseDartInitData : VAbstractOperationInitData
{
    quint32 origin{NULL_ID};  // the pivot, the apex of the dart
    quint32 moving{NULL_ID};  // the leg point that travels
    quint32 target{NULL_ID};  // the leg point it lands on
    QString divisor{};        // how much of the opening is closed, as a formula
};

class OToolCloseDart : public VAbstractOperation
{
    Q_OBJECT // NOLINT

public:
    ~OToolCloseDart() override = default;

    void SetDialog() override;

    static auto Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                       VContainer *data) -> OToolCloseDart *;
    static auto Create(OToolCloseDartInitData &initData) -> OToolCloseDart *;

    static const QString ToolType;

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Tool::CloseDart)
    };

    /** @brief How much of the opening is closed: what the dialog and the options dock both read and write.
     *  It is the only value of this tool the user can change, and the only one a pattern keeps. */
    auto GetFormulaDivisor() const -> VFormula;
    void SetFormulaDivisor(const VFormula &value);

    /** @brief The three points of the dart, by name — what the recipe of a pattern names them with.
     *
     * None of them is shown anywhere in the interface: the tool says what it does with the points, not what
     * they are called. */
    auto OriginPointName() const -> QString;
    auto MovingPointName() const -> QString;
    auto TargetPointName() const -> QString;

    void ShowVisualization(bool show) override;

protected slots:
    void ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id = NULL_ID) override;

protected:
    void SetVisualization() override;
    void SaveDialog(QDomElement &domElement) override;
    void ReadToolAttributes(const QDomElement &domElement) override;
    void SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj) override;
    auto MakeToolTip() const -> QString override;
    void ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement) override;

private:
    Q_DISABLE_COPY_MOVE(OToolCloseDart) // NOLINT

    quint32 m_origPointId;
    quint32 m_movingPointId;
    quint32 m_targetPointId;
    QString m_formulaDivisor;

    explicit OToolCloseDart(const OToolCloseDartInitData &initData, QGraphicsItem *parent = nullptr);

    /** @brief The divisor as a number, NaN when the formula does not evaluate: a preview that cannot divide
     *  has no angle to draw, and it must not open a dialog of its own while the mouse is moving. */
    auto Divisor() const -> qreal;

    auto SyncDestination(const QVector<SourceItem> &source, QString &formulaDivisor) -> QVector<DestinationItem>;
};

#endif // OTOOLCLOSEDART_H
