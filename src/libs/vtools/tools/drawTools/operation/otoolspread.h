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
#include "ospreaddisplay.h"
#include "ospreadrule.h"
#include "vabstractoperation.h"

template <class T> class QSharedPointer;
class QDomElement;
class QGraphicsSceneContextMenuEvent;

struct OToolSpreadInitData : VAbstractOperationInitData
{
    QVector<OSpreadRotation> rotations{};
    OSpreadDisplay display{};
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

    /** @brief The visual representation of the rotations, as the operation holds it: whether it is drawn, the
     *  pen of its lines, and the id of every object it created. */
    auto Display() const -> const OSpreadDisplay &;
    void SetDisplay(const OSpreadDisplay &display);

    auto IsDisplayShown() const -> bool { return m_display.show; }
    void SetDisplayShown(bool show);

    auto DisplayTypeLine() const -> QString { return m_display.typeLine; }
    void SetDisplayTypeLine(const QString &typeLine);

    auto DisplayColor() const -> QString { return m_display.lineColor; }
    void SetDisplayColor(const QString &color);

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
    void RefreshDataInFile() override;

    /** @brief Where the label of the third vertex of a mark stands, kept in the mark that holds it.
     *
     *  The base writes the container object — what the scene draws — and the `<destination>` record of the point that
     *  moved. The vertex is no destination: the record is not found, nothing is written, and the next parse puts the
     *  label back at the place the point it comes from carries. Kept in the entry itself, exactly as OToolDartCap
     *  keeps the place of its own cap and OToolPleat that of its figure. */
    void ChangeLabelPosition(quint32 id, const QPointF &pos) override;
    auto MakeToolTip() const -> QString override;
    void ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement) override;

    auto ExtraPointIds() const -> QVector<quint32> override;
    auto ExtraLineIds() const -> QVector<quint32> override;

    auto DisplayObjectIds(const QDomElement &domElement) const -> QSet<quint32> override;

private:
    Q_DISABLE_COPY_MOVE(OToolSpread) // NOLINT

    QVector<OSpreadRotation> m_rotations{};
    OSpreadDisplay m_display{};

    explicit OToolSpread(const OToolSpreadInitData &initData, QGraphicsItem *parent = nullptr);

    auto SyncDestination(const QVector<SourceItem> &source) -> QVector<DestinationItem>;

    /** @brief Keep in @a m_display the place a mark's vertex was dragged to. False when @a point is not one of the
     *  points the display made — the base has then already written it, or there is nothing to write. */
    auto WriteFigureLabelPlace(quint32 point, const QPointF &pos) -> bool;

    /** @brief Give the display the ids its marks still miss, and drop them when the representation is off: a
     *  mark is only ever created with an id the file carries. */
    void SyncDisplay(const QVector<SourceItem> &source);

    /** @brief Write this tool's element as an option change does, and ask for the full parse the change of a
     *  created object set needs. */
    void SaveOptionsAndReparse();
};

#endif // OTOOLSPREAD_H
