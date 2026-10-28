/************************************************************************
 **
 **  @file   otoolmovebatch.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   04/10/2026
 **
 **  @brief  The move by batch: N moves, each one carried by the objects of one cut line.
 **
 **          The tool is a move replayed N times. From an ordered chain of objects and N pairs (start, pivot),
 **          it applies the moves cumulatively -- in memory -- and materializes only the final position of every
 **          object. No intermediate state ever enters the pattern.
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

#ifndef OTOOLMOVEBATCH_H
#define OTOOLMOVEBATCH_H

#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/def.h"
#include "omovebatchrule.h"
#include "vabstractoperation.h"

template <class T> class QSharedPointer;
class QDomElement;
class QGraphicsSceneContextMenuEvent;

/** @brief One copy a point needs at an intermediate state: the object created for it, the point it comes
 *  from, and how many moves it has been through there. See OMoveBatchRule::IntermediateStates. */
struct OMoveBatchIntermediate
{
    quint32 point{NULL_ID};
    quint32 sourceId{NULL_ID};
    int state{0};
    QString name{};
};

struct OToolMoveBatchInitData : VAbstractOperationInitData
{
    QVector<OMoveBatchMove> moves{};
    QVector<OMoveBatchIntermediate> intermediates{};
};

class OToolMoveBatch : public VAbstractOperation
{
    Q_OBJECT // NOLINT

public:
    ~OToolMoveBatch() override = default;

    void SetDialog() override;

    static auto Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                       VContainer *data) -> OToolMoveBatch *;
    static auto Create(OToolMoveBatchInitData &initData) -> OToolMoveBatch *;

    static const QString ToolType;

    static const QString TagMoves;
    static const QString AttrStart;
    static const QString AttrPivot;

    static const QString TagIntermediates;
    static const QString AttrPoint;
    static const QString AttrSource;
    static const QString AttrState;

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Tool::MoveBatch)
    };

    auto Moves() const -> const QVector<OMoveBatchMove> &;
    void SetMoves(const QVector<OMoveBatchMove> &moves);

    void ShowVisualization(bool show) override;

    /** @brief Open the options dialog, exactly as the context menu's "Options" entry does. The options dock
     *  offers a button for it: the moves are a list, and the dock has no editor for one. */
    void OpenOptionsDialog();

    static void SaveMoves(VAbstractPattern *doc, QDomElement &tag, const QVector<OMoveBatchMove> &moves);
    static auto ExtractMoves(const QDomElement &domElement) -> QVector<OMoveBatchMove>;

    /** The copies a point needs at the intermediate states of the chain, written beside the moves: one entry
     *  per object, holding the id the file gave it, the point it comes from, its state and its name. */
    static void SaveIntermediates(VAbstractPattern *doc, QDomElement &tag,
                                  const QVector<OMoveBatchIntermediate> &intermediates);
    static auto ExtractIntermediates(const QDomElement &domElement) -> QVector<OMoveBatchIntermediate>;

protected slots:
    void ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id = NULL_ID) override;

protected:
    void SetVisualization() override;
    void SaveDialog(QDomElement &domElement) override;
    void ReadToolAttributes(const QDomElement &domElement) override;
    void SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj) override;
    auto MakeToolTip() const -> QString override;
    void ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement) override;

    /** @brief The points the operation materializes at an intermediate state, so the framework builds their items
     *  exactly as it builds the copies'. */
    auto ExtraPointIds() const -> QVector<quint32> override;

    /** @brief The objects of the figure a layer has to answer for: the copies, and the intermediate points. */
    auto DisplayObjectIds(const QDomElement &domElement) const -> QSet<quint32> override;

private:
    Q_DISABLE_COPY_MOVE(OToolMoveBatch) // NOLINT

    QVector<OMoveBatchMove> m_moves{};
    QVector<OMoveBatchIntermediate> m_intermediates{};

    explicit OToolMoveBatch(const OToolMoveBatchInitData &initData, QGraphicsItem *parent = nullptr);

    /** @brief Materialize, before the tool exists, every copy a point needs at an intermediate state.
     *
     *  Called once the destinations are made, from the very place they are: the figure must be in the container
     *  before InitOperatedObjects() asks for the objects whose items it has to build. */
    static void BuildIntermediates(OToolMoveBatchInitData &initData);

    auto SyncDestination(const QVector<SourceItem> &source) -> QVector<DestinationItem>;
};

#endif // OTOOLMOVEBATCH_H
