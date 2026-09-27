/************************************************************************
 **
 **  @file   odialogrectangle.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   27/09/2026
 **
 **  @brief  the window that describes a rectangle
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

#ifndef ODIALOGRECTANGLE_H
#define ODIALOGRECTANGLE_H

#include <QCheckBox>
#include <QHash>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include "dialogtool.h"

class FormulaInputWidget;
class QComboBox;
class QLabel;
class QLineEdit;

namespace Ui
{
class ODialogRectangle;
}

/**
 * @brief The window of a rectangle: the base point, the two lengths that follow from it, and one line and one
 * box per point it creates.
 *
 * The base point is not offered as a field: it is the point the user clicked in the pattern, and the preview
 * is where it is seen. The window only asks what is left to decide — the two lengths, the names of the two
 * sides they draw, and, for each of the three points created, its name and whether that name is drawn.
 *
 * The two lengths are `FormulaInputWidget`s, the same block every other tool uses: the label and the value it
 * evaluates to on one row, the field and its grow button under it. Each carries the pastille of the side it
 * draws — green for the width, red for the height — which is the only thing that tells the user which is
 * which, and the very pair of colours the preview and the created rectangle draw those two sides in.
 */
class ODialogRectangle final : public DialogTool
{
    Q_OBJECT // NOLINT

public:
    explicit ODialogRectangle(const VContainer *data, VAbstractPattern *doc, quint32 toolId,
                              QWidget *parent = nullptr);
    ~ODialogRectangle() override;

    auto GetBasePointId() const -> quint32;
    void SetBasePointId(quint32 value);

    auto GetFormulaWidth() const -> QString;
    void SetFormulaWidth(const QString &value);

    auto GetFormulaHeight() const -> QString;
    void SetFormulaHeight(const QString &value);

    auto GetPointBName() const -> QString;
    void SetPointBName(const QString &value);

    auto GetPointCName() const -> QString;
    void SetPointCName(const QString &value);

    auto GetPointDName() const -> QString;
    void SetPointDName(const QString &value);

    //! Whether the name of each created point is drawn on the canvas. One box per point, all unchecked.
    auto IsPointBLabelVisible() const -> bool;
    auto IsPointCLabelVisible() const -> bool;
    auto IsPointDLabelVisible() const -> bool;
    void SetShowPointLabels(bool showB, bool showC, bool showD);

    auto GetTypeLine() const -> QString;
    void SetTypeLine(const QString &value);

    auto GetLineColor() const -> QString;
    void SetLineColor(const QString &value);

    /** @brief The name given to each of the two lengths. A named length becomes a variable the rest of the
     *  pattern can use — `Line_<name>` — which is why the window offers it next to its pastille. */
    auto GetWidthLineRoleName() const -> QString;
    void SetWidthLineRoleName(const QString &value);

    auto GetHeightLineRoleName() const -> QString;
    void SetHeightLineRoleName(const QString &value);

    void SetNotes(const QString &notes);
    auto GetNotes() const -> QString;

    void CheckDependencyTreeComplete() override;

    void ShowDialog(bool click) override;

public slots:
    void ChosenObject(quint32 id, const SceneObject &type) override;

protected:
    void ShowVisualization() override;
    void SaveData() override;
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    auto IsValid() const -> bool override;

private:
    Q_DISABLE_COPY_MOVE(ODialogRectangle) // NOLINT

    Ui::ODialogRectangle *ui;

    /** @brief The base point, held in a combo of its own and never shown: validating a picked object is the
     *  base class's job and it reads a combo. The name never reaches the screen. */
    QComboBox *m_comboBasePoint{nullptr};

    FormulaInputWidget *m_formulaWidth{nullptr};
    FormulaInputWidget *m_formulaHeight{nullptr};

    bool m_flagNameB{false};
    bool m_flagNameC{false};
    bool m_flagNameD{false};
    bool m_flagWidthLineRoleName{true};
    bool m_flagHeightLineRoleName{true};

    /** @brief The name each of the two length rows started with. A name already in the container is not
     *  unique, so a row has to be allowed to keep its own — and only its own. */
    QHash<QLineEdit *, QString> m_lineNamesInUse{};

    /** @brief The name each of the three point rows started with, for the same reason: on an existing
     *  rectangle, the name the window opened with is already taken by the point it names. */
    QString m_originalNameB{};
    QString m_originalNameC{};
    QString m_originalNameD{};

    void InitIcons();

    /** @brief Whether a name typed for a created point can name one: shape and uniqueness, and the label of
     *  the row turned green or red with it. */
    void CheckOnePointName(QLineEdit *edit, QLabel *label, bool &flag, const QString &originalName);

    /** @brief Whether a name typed for a length can name one: shape and uniqueness. A named length is the key
     *  of a `Line_` variable, so an empty box is valid and simply leaves the length anonymous. */
    void CheckOneLineRoleName(QLineEdit *edit, QLabel *label, bool &flag);

    /** @brief Push what the window holds into the preview, so that the shape follows the two lengths. */
    void RefreshPreview();
};

#endif // ODIALOGRECTANGLE_H
