/************************************************************************
 **
 **  @file   opatternversionsdialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/11/2026
 **
 **  @brief  The versions of the pattern: the switch, the names, and the
 **          pieces that would be left without one.
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

#ifndef OPATTERN_VERSIONSDIALOG_H
#define OPATTERN_VERSIONSDIALOG_H

#include <QDialog>
#include <QList>

class QCheckBox;
class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QVBoxLayout;
class VAbstractPattern;

/**
 * @brief The versions of the pattern — the one place they are named and added.
 *
 * Opened from the two places the pattern is configured: the pattern page of the piece tool (first
 * piece only) and the pattern page of the details dock. What it edits is the document, at once: a
 * name is not a form field to submit, and a version the reader has just typed is already part of the
 * pattern every piece will be compared to.
 *
 * **The pieces are the constraint.** A version worn by pieces cannot disappear silently, and the
 * "all versions" entry cannot disappear at all — it is where a piece that belongs to no particular
 * version lives. When a deletion or the switch-off would leave pieces without a version, the dialog
 * says how many, proposes to move them to "all versions", and does nothing if the answer is no.
 */
class OPatternVersionsDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit OPatternVersionsDialog(VAbstractPattern *doc, QWidget *parent = nullptr);

    /**
     * @brief Turns the versions off, moving every piece that wears one to the "all versions" entry.
     *
     * Asked from the three switches — this dialog's, the piece-tool pattern page's and the details
     * dock's — because the refusal must be the same everywhere: a switch that can leave a piece
     * wearing a version nobody shows any more would be a switch that lies.
     *
     * @return false when the reader refused. Nothing is changed then.
     */
    static auto SwitchOffWithReassignment(VAbstractPattern *doc, QWidget *parent) -> bool;

    /** @brief The pieces that wear @p versionId, read from the document. */
    static auto PiecesWearing(const VAbstractPattern &doc, quint32 versionId) -> QList<quint32>;

signals:
    /** @brief A version was added, renamed, deleted, or the mode was switched: reload what shows them. */
    void VersionsChanged();

private:
    void Rebuild();
    void UpdateControls();
    void AddVersion();
    void RemoveVersion(quint32 id);
    void CommitName(quint32 id, const QString &name, QLineEdit *editor);
    void OnMultiVersionToggled(bool on);

    static auto ConfirmReassign(QWidget *parent, const QString &question, const QString &allName) -> bool;
    /// Move @p pieceIds to @p destinationId, one undo step. @p destinationId is one of the pattern's
    /// versions — the entry that covers every version, or one the reader picked.
    static void ReassignPieces(VAbstractPattern *doc, const QList<quint32> &pieceIds, quint32 destinationId);
    auto UniqueName() const -> QString;

    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(OPatternVersionsDialog) // NOLINT

    VAbstractPattern *m_doc{nullptr};
    QCheckBox *m_multiCheck{nullptr};
    QLabel *m_hint{nullptr};
    QScrollArea *m_rowsScroll{nullptr};
    QFrame *m_rowsHost{nullptr};
    QVBoxLayout *m_rowsLayout{nullptr};
    QPushButton *m_addButton{nullptr};
};

#endif // OPATTERN_VERSIONSDIALOG_H
