/************************************************************************
 **
 **  @file   opiecedialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   28 August, 2026
 **
 **  @brief  New piece creation dialog (two steps): collects the contour
 **          (headless, via OPieceAutomation) then gathers pattern- and
 **          piece-level information. Replaces DialogSeamAllowance on the
 **          creation path only; the legacy dialog stays on Details editing.
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

#ifndef OPIECEDIALOG_H
#define OPIECEDIALOG_H

#include <QStackedWidget>

#include "../vtools/dialogs/tools/dialogtool.h"
#include "../vtools/dialogs/tools/piece/opieceautomation.h"
#include "../vpatterndb/vpiece.h"

class QHBoxLayout;
class QPushButton;
class OPiecePatternStep;
class OPiecePieceStep;

/**
 * @brief Two-step piece creation dialog.
 *
 * Hidden while the contour is collected (the automation drives the preview),
 * then shown on Enter: two steps for the first piece (pattern, then piece),
 * a single piece step for the following ones. No main path editing, no Reverse
 * button here — the sense is automatic.
 */
class OPieceDialog final : public DialogTool
{
    Q_OBJECT

public:
    explicit OPieceDialog(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent = nullptr);

    auto GetPiece() const -> VPiece;

    void ShowDialog(bool click) override;
    void SetPatternDoc(VAbstractPattern *doc) override;
    void CheckDependencyTreeComplete() override;

public slots:
    void ChosenObject(quint32 id, const SceneObject &type) override;
    void DialogAccepted() override;

protected:
    auto IsValid() const -> bool override;

private:
    void BuildPreviewPiece();
    void UpdateButtons();
    void OnPatternNext();
    void OnPatternSkip();

    OPieceAutomation m_automation{&data};
    OPiecePatternStep *m_patternStep{nullptr};
    OPiecePieceStep *m_pieceStep{nullptr};
    QStackedWidget *m_stack{nullptr};
    QPushButton *m_skipButton{nullptr};
    QPushButton *m_nextButton{nullptr};
    QPushButton *m_okButton{nullptr};
    bool m_patternCommitted{false};
};

#endif // OPIECEDIALOG_H
