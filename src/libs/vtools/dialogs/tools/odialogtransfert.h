/************************************************************************
 **
 **  @file   odialogtransfert.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   29/09/2026
 **
 **  @brief  The dialog of the transfer: which part receives the portion, and which of its layers. It is the
 **          one dialog that stays alive after its OK — the gesture ends on the point that receives the
 **          portion, in the part that has just been made active.
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

#ifndef ODIALOGTRANSFERT_H
#define ODIALOGTRANSFERT_H

#include <QPair>
#include <QString>
#include <QtGlobal>

#include "dialogoperationtool.h"

class VAbstractPattern;
class VContainer;

namespace Ui
{
class DialogTransfert;
}

class ODialogTransfert final : public DialogOperationTool
{
    Q_OBJECT // NOLINT

public:
    ODialogTransfert(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent = nullptr);
    ~ODialogTransfert() override;

    /** @brief X, the merge point of the source part. Chosen on the canvas, before the dialog is shown. */
    auto GetOriginPointId() const -> quint32;
    void SetOriginPointId(quint32 value);

    /** @brief A, the merge point of the host part. Chosen on the canvas, after this dialog has been
     *  answered — hence the two steps. */
    auto GetTargetPointId() const -> quint32;
    void SetTargetPointId(quint32 value);

    /** @brief The part that receives the portion, by name. */
    auto GetHostPartName() const -> QString;
    void SetHostPartName(const QString &name);

    /** @brief The layer the copies are written into: an existing layer of the host part, or NULL_ID with
     *  GetHostLayerName() the name of the layer to create. */
    auto GetHostLayerId() const -> quint32;
    auto GetHostLayerName() const -> QString;

    /** @brief The part the dialog was opened on — the one the portion comes from. Empty when an existing
     *  transfer is edited: there is no part to go back to, the copies are already written. */
    auto SourcePart() const -> QString;

    void ShowDialog(bool click) override;
    void ChosenObject(quint32 id, const SceneObject &type) override;
    void DialogAccepted() override;

signals:
    /** @brief The host part is chosen: the main window makes it the active one — and shows it, since the
     *  canvas of the previous part stays where it was. Emitted instead of switching from here: a piece
     *  switch redraws the docks and fits the view, which is the application's business, not a dialog's. */
    void HostPartSelected(const QString &part);

protected:
    auto Widgets() const -> OperationWidgets override;
    auto IsValid() const -> bool override;

    /** @brief Highlight what the portion is made of, as every operation does: the picked objects are the
     *  only thing this dialog shows, since nothing is drawn of the transfer before the tool exists. */
    void ShowVisualization() override;
    void OnSourceObjectsSet() override;
    void SaveData() override;

private slots:
    void HostPartChanged(int index);

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(ODialogTransfert) // NOLINT

    /** @brief Where the gesture stands. The three steps after the selection are answered by clicks on the
     *  canvas, not by the dialog — the dialog is only up during Host. */
    enum class Step
    {
        Selecting, // objects are being picked; Enter ends the selection
        Origin,    // the next point clicked is X, the merge point of the source part
        Host,      // the dialog is up: the user chooses the part and its layer
        Target     // the host part is active: the next point clicked is A
    };

    Ui::DialogTransfert *ui;
    Step m_step{Step::Selecting};
    quint32 m_originId{NULL_ID};
    quint32 m_targetId{NULL_ID};

    /** @brief The active part when the creation started. Choosing the host part hands the window over to it,
     *  and only a created operation makes that permanent: an abandoned creation has to come back here. */
    QString m_sourcePart;

    auto IsEditing() const -> bool;

    /** @brief The chosen layer of the host part: its id when it already exists, NULL_ID with the typed name
     *  when the user is naming a new one. */
    auto HostLayer() const -> QPair<quint32, QString>;

    /** @brief Push the picked objects into the visualization, so the portion is lit while it is being
     *  chosen. */
    void RefreshPreview();

    /** @brief Say which step is expected, in the status line of the dialog. */
    void Say(const QString &step);

    /** @brief Offer every part of the pattern, in file order. The part receiving the portion can be built
     *  before the one it copies from: such a transfer is kept aside and built once the whole file has been
     *  read, so nothing here depends on the order any more. */
    void FillHostParts();

    void FillHostLayers(const QString &partName);

    /** @brief Say what is expected without closing anything: the tooltip line, and a box over the main
     *  window while the dialog is still hidden. */
    void Refuse(const QString &message);
};

#endif // ODIALOGTRANSFERT_H
