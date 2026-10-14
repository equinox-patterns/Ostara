/************************************************************************
 **
 **  @file   dialogindividualmeasurements.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   22 2, 2014
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#ifndef DIALOGINDIVIDUALMEASUREMENTS_H
#define DIALOGINDIVIDUALMEASUREMENTS_H

#include <QDialog>
#include <QStringList>

#include "../vmisc/def.h"

class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class VContainer;

namespace Ui
{
    class DialogNewPattern;
}

class DialogNewPattern : public QDialog
{
    Q_OBJECT // NOLINT
public:
    /// Which creation this dialog is answering. The unit belongs to the pattern, so it is only part of the
    /// question when the pattern itself is being born; a piece added to an existing pattern inherits it.
    enum class Creation
    {
        Pattern, ///< The first piece, and with it the pattern.
        Piece    ///< One more piece inside a pattern that already exists.
    };

    DialogNewPattern(VContainer *data, const QString &patternPieceName, Creation creation,
                     QWidget *parent = nullptr);
    ~DialogNewPattern() override;
    auto name() const -> QString;
    auto PatternUnit() const -> Unit;

    /// What the reader wants to say about the piece — free text, and the only thing a piece says besides its
    /// name. It is written on the block, so it travels into the library with the piece and comes back with an
    /// instance imported from there.
    auto Notes() const -> QString;

    /// The piece names this pattern already uses. A piece cannot share its name with another one, and the
    /// dialog says so by keeping OK down rather than by asking the question again after the fact.
    void SetTakenNames(const QStringList &names);

    /// The library to offer in the second group. **Given** by the caller, never read from the settings here: the
    /// caller is what knows which library it is talking about, and giving it is also what makes this dialog
    /// measurable.
    void SetLibraryDirectory(const QString &directory);

    /// The unit a piece of the library must be in to be offered — the unit of the pattern it would enter. Read
    /// from the unit combo instead when the pattern itself is being born: there the unit is the question, and
    /// changing it re-reads the library.
    void SetLibraryUnit(const Unit &unit);

    /// The piece the reader chose to import, as an absolute path — empty when she chose to create one instead.
    auto ImportedPath() const -> QString;

    /// The layer the piece starts in — the two are born together, since no object may live outside a layer.
    auto LayerName() const -> QString;
    auto LayerColor() const -> QString;
    auto LayerPenStyle() const -> QString;

protected:
    void showEvent(QShowEvent *event) override;
private slots:
    void CheckState();
private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(DialogNewPattern) // NOLINT

    /// The rule and the first layer's three fields, as extra rows of the .ui's form. Built here because the
    /// pen combos are code-built — MakeComboBox — and because a piece added to an existing pattern must not
    /// show the unit row that sits above them.
    void BuildLayerFields();

    Ui::DialogNewPattern *ui;
    VContainer *m_data;
    bool m_isInitialized{false};
    QStringList m_takenNames;
    QPlainTextEdit *m_notes{nullptr};
    QLineEdit *m_layerName{nullptr};
    QComboBox *m_layerPenStyle{nullptr};
    QComboBox *m_layerColor{nullptr};
    void InitUnits();

    /// Fill the second group from the library the caller gave: one line per piece, saying what it reads
    /// elsewhere — that is what decides whether it can enter this pattern at all. A file that cannot be read
    /// keeps its place and says why, unselectable, rather than disappearing from the list without a word.
    void FillLibrary();

    /// The unit a piece must be in to be offered: the combo's when the pattern is being born, the one the caller
    /// gave otherwise. Every other unit is left out of the list, because the insertion would refuse it.
    auto WantedUnit() const -> Unit;

    /// The piece selected in the library list, or empty. What the dialog is about to do hangs on it.
    auto SelectedLibraryPiece() const -> QString;

    /// The kind of creation this dialog asks about — what tells whether the unit is the combo's or the caller's.
    Creation m_creation{Creation::Pattern};

    QString m_libraryDirectory{};
    QString m_importedPath{};
    Unit m_libraryUnit{Unit::LAST_UNIT_DO_NOT_USE};
    /// The OK button's own text, kept so it can be given back when the choice goes back to creating a piece.
    QString m_okText{};
};

#endif // DIALOGINDIVIDUALMEASUREMENTS_H
