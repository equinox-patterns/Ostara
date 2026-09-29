/************************************************************************
 **
 **  @file   otooltransfert.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   29/09/2026
 **
 **  @brief  The transfer tool: a portion of one pattern part is grafted onto another one, as a copy
 **          recomputed from its sources at every parse. The anchor of the portion is a point X of the
 **          source part that fuses with a point A of the host part: X is referenced, never copied — the
 **          translation X → A would have placed its copy exactly on A, so A *is* its copy.
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

#ifndef OTOOLTRANSFERT_H
#define OTOOLTRANSFERT_H

#include <QHash>
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

struct OToolTransfertInitData : VAbstractOperationInitData
{
    /** @brief X, the merge point of the source part. It is referenced — its copy is the target — so it
     *  carries no destination object of its own, while keeping its place in the source list. */
    quint32 origin{NULL_ID};

    /** @brief A, the merge point of the host part: where X lands, hence where the whole portion lands. */
    quint32 target{NULL_ID};

    /** @brief The layer of the host part the copies are written into. NULL_ID with a name asks for a new
     *  layer of that name; NULL_ID without one leaves the copies to the host part's active layer. */
    quint32 layer{NULL_ID};
    QString layerName{};
};

class OToolTransfert : public VAbstractOperation
{
    Q_OBJECT // NOLINT

public:
    ~OToolTransfert() override = default;

    void SetDialog() override;

    static auto Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                       VContainer *data) -> OToolTransfert *;
    static auto Create(OToolTransfertInitData &initData) -> OToolTransfert *;

    static const QString ToolType;

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Tool::Transfert)
    };

    /** @brief The merge points by name — the only two things this tool says about itself, since it has no
     *  formula of its own: everything else is the portion, and the portion is recalculated. */
    auto OriginPointName() const -> QString;
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
    Q_DISABLE_COPY_MOVE(OToolTransfert) // NOLINT

    quint32 m_origPointId{NULL_ID};
    quint32 m_targetPointId{NULL_ID};

    explicit OToolTransfert(const OToolTransfertInitData &initData, QGraphicsItem *parent = nullptr);

    auto SyncDestination(const QVector<SourceItem> &source) -> QVector<DestinationItem>;
};

#endif // OTOOLTRANSFERT_H
