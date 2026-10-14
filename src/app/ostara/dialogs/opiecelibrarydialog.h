/************************************************************************
 **
 **  @file   opiecelibrarydialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/10/2026
 **
 **  @brief  The pieces saved in the library: what each of them asks for, what the reader wrote about it, and the
 **          four gestures that manage them — insert one, copy one out, take one in, or delete it.
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
#ifndef OPIECELIBRARYDIALOG_H
#define OPIECELIBRARYDIALOG_H

#include "core/opiecelibrary.h"

#include <QDialog>
#include <QList>
#include <QString>

class QDialogButtonBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;
class QTimer;

/**
 * @brief The library of pieces: what it holds, what each piece needs, and what was written about it.
 *
 * The list is what a folder cannot say — a piece that reads a value it does not declare needs another piece, and
 * that other piece is named here before the piece is ever imported. Beside it, the same selection tells **what the
 * piece reads** in full and **what the reader noted** about it, and the note is written here as well as read: a
 * piece is managed from this window, so this is where it is described.
 *
 * Four gestures live here, and none of them is a way of drawing: **Insert** gives the chosen piece to the caller,
 * **Export** copies it out of the library under another name, **Import** takes a piece in from elsewhere, and
 * **Delete** removes it. Import reads a piece before it copies it, and Delete asks.
 */
class OPieceLibraryDialog : public QDialog
{
    Q_OBJECT // NOLINT

public:
    /// @param directory the folder the library is read from — given, never guessed, so the window can be
    ///        pointed at any folder the pieces of a test live in, and so the settings are read once, by
    ///        the caller, where the library's location is decided.
    explicit OPieceLibraryDialog(const QString &directory, QWidget *parent = nullptr);
    ~OPieceLibraryDialog() override = default;

    /// @brief The piece the reader chose, as an absolute path — empty until she chooses one.
    auto SelectedPath() const -> QString;

protected:
    void changeEvent(QEvent *event) override;

    /// The note being typed is written before the window goes: a pending save is not a reason to lose it.
    void done(int result) override;

private slots:
    void OnSearchChanged(const QString &text);
    void ShowDetails();

    /// @brief A click on one of the two action columns: the piece of that row is exported or deleted.
    void OnCellClicked(int row, int column);

    /// @brief The library folder, opened where the system keeps its files.
    void OnOpenFolder();

    /// @brief A piece taken from elsewhere on the computer, read, and copied into the library.
    void OnImportPiece();

    /// @brief The note the reader is typing, remembered once she stops.
    void OnNotesChanged();
    void CommitNotes();

private:
    Q_DISABLE_COPY_MOVE(OPieceLibraryDialog) // NOLINT

    void SetupUI();
    void Retranslate();
    void Refresh();
    void Populate();
    void Filter(const QString &filter);

    /// @brief Follow the selection: remember the piece chosen and say whether it can be inserted at all.
    void FollowSelection();

    /// @brief The row the selection is on, or -1 — a hidden row counts as no row at all.
    auto SelectedRow() const -> int;

    void ExportPiece(int row);
    void DeletePiece(int row);

    /// @brief Writes the note of the selected piece, and says so when the file refuses it.
    void SaveNotes(int row);

    /// @brief The file the imported piece would be saved under, asking for another name while that one is taken.
    /// @return the path to copy to, or empty when the reader gave up.
    auto FreeImportPath(const QString &directory, const QString &baseName) -> QString;

    QLabel *m_searchLabel{nullptr};
    QLineEdit *m_searchEdit{nullptr};
    QTableWidget *m_table{nullptr};

    QGroupBox *m_needsGroup{nullptr};
    QLabel *m_needsLabel{nullptr};
    QGroupBox *m_notesGroup{nullptr};
    QPlainTextEdit *m_notesEdit{nullptr};

    QPushButton *m_openFolderButton{nullptr};
    QPushButton *m_importButton{nullptr};
    QDialogButtonBox *m_buttonBox{nullptr};
    QPushButton *m_insertButton{nullptr};

    /// The note is written a moment after the typing stops, never on every keystroke: a file written per letter
    /// would be a file validated per letter.
    QTimer *m_notesTimer{nullptr};

    /// True while the note field is being filled from the selection: what is written then is not the reader's.
    bool m_loadingNotes{false};

    /// The row the note on screen belongs to — not the row selected now: a pending save must go to the piece it
    /// was typed for, never to the one that just took its place.
    int m_notesRow{-1};

    QList<OPieceLibrary::Summary> m_summaries{};
    QString m_directory{};
    QString m_selectedPath{};
};

#endif // OPIECELIBRARYDIALOG_H
