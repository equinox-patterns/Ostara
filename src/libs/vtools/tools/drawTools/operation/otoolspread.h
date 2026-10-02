/************************************************************************
 **
 **  @file   otoolspread.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/10/2026
 **
 **  @brief  The slash-and-spread tool: a batch of rotations that opens a piece along N cut lines.
 **
 **          A slash-and-spread turns an ordered chain of objects with N cumulated rotations and materializes
 **          only the final copy of each object. The N rotations, each a pair (start, pivot) plus an angle,
 **          are replayed in memory at every parse; no intermediate state ever becomes a pattern object.
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

#ifndef OTOOLSPREAD_H
#define OTOOLSPREAD_H

#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/def.h"
#include "ospreadrule.h"
#include "vabstractoperation.h"

template <class T> class QSharedPointer;
class QDomElement;
class QGraphicsSceneContextMenuEvent;

struct OToolSpreadInitData : VAbstractOperationInitData
{
    QVector<OSpreadRotation> rotations{};
};

class OToolSpread : public VAbstractOperation
{
    Q_OBJECT // NOLINT

public:
    ~OToolSpread() override = default;

    void SetDialog() override;

    static auto Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                       VContainer *data) -> OToolSpread *;
    static auto Create(OToolSpreadInitData &initData) -> OToolSpread *;

    static const QString ToolType;

    static const QString TagRotations;
    static const QString AttrStart;
    static const QString AttrPivot;

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Tool::Spread)
    };

    auto Rotations() const -> const QVector<OSpreadRotation> &;
    void SetRotations(const QVector<OSpreadRotation> &rotations);

    void ShowVisualization(bool show) override;

    /** @brief Open the options dialog, exactly as the context menu's "Options" entry does. The options dock
     *  offers a button for it: the rotations are a list, and the dock has no editor for one. */
    void OpenOptionsDialog();

    static void SaveRotations(VAbstractPattern *doc, QDomElement &tag, const QVector<OSpreadRotation> &rotations);
    static auto ExtractRotations(const QDomElement &domElement) -> QVector<OSpreadRotation>;

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
    Q_DISABLE_COPY_MOVE(OToolSpread) // NOLINT

    QVector<OSpreadRotation> m_rotations{};

    explicit OToolSpread(const OToolSpreadInitData &initData, QGraphicsItem *parent = nullptr);

    auto SyncDestination(const QVector<SourceItem> &source) -> QVector<DestinationItem>;
};

#endif // OTOOLSPREAD_H
