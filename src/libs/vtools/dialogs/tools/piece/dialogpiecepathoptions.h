/************************************************************************
 **
 **  @file   dialogpiecepathoptions.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   08/09/2026
 **
 **  @brief  Edit dialog for the attribute block of an existing internal path
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

#ifndef DIALOGPIECEPATHOPTIONS_H
#define DIALOGPIECEPATHOPTIONS_H

#include <QDialog>

#include "../dialogtool.h"
#include "../vpatterndb/vpiecepath.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTimer;
class QToolButton;

/**
 * @brief Edits the attributes of an existing internal path (piece path):
 *        Name · Type of pen · continue to the cutting contour · Cut on fabric ·
 *        Not mirrored · Visibility.
 *
 *        Built directly on the QDialog (QGridLayout + button box), exactly like
 *        the "Internal mark" (DialogPlaceLabel) dialog: no QTabWidget container,
 *        so nothing can paint a "fond" behind the content. The path geometry
 *        (nodes) is kept untouched — only the attribute fields are editable.
 */
class DialogPiecePathOptions final : public QDialog
{
    Q_OBJECT // NOLINT

public:
    explicit DialogPiecePathOptions(const VContainer *data, quint32 pathId, QWidget *parent = nullptr);
    ~DialogPiecePathOptions() override = default;

    /** @brief Fill every field from @p path (the nodes are kept for GetPath()). */
    void SetPath(const VPiecePath &path);

    /** @return The edited path: original nodes + attributes read from the fields. */
    auto GetPath() const -> VPiecePath;

private slots:
    void EvalVisible();
    void FXVisible();

private:
    void BuildUi();
    void RetranslateUi();
    void SetFxIcons();
    void SetPenStyle(Qt::PenStyle style);
    auto PenStyle() const -> Qt::PenStyle;
    auto GetFormulaVisible() const -> QString;
    void SetFormulaVisible(const QString &formula);

    const VContainer *m_data{nullptr};
    quint32 m_pathId{0};
    VPiecePath m_path{};

    QLabel *m_nameLabel{nullptr};
    QLineEdit *m_nameEdit{nullptr};
    QLabel *m_penLabel{nullptr};
    QComboBox *m_penCombo{nullptr};
    QCheckBox *m_firstCheck{nullptr};
    QCheckBox *m_lastCheck{nullptr};
    QCheckBox *m_cutCheck{nullptr};
    QCheckBox *m_notMirroredCheck{nullptr};
    QLabel *m_visibleLabel{nullptr};
    QLineEdit *m_visibleEdit{nullptr};
    QToolButton *m_visibleFx{nullptr};
    QLabel *m_visibleHint{nullptr};
    QTimer *m_visibleTimer{nullptr};
    QPushButton *m_okButton{nullptr};
    bool m_flagFormulaVisible{false};

    Q_DISABLE_COPY_MOVE(DialogPiecePathOptions) // NOLINT
};

#endif // DIALOGPIECEPATHOPTIONS_H
