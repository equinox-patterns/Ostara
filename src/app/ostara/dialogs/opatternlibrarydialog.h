/************************************************************************
 **
 **  @file   opatternlibrarydialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/10/2026
 **
 **  @brief  The library of patterns: the folders they are sorted into, the patterns of the chosen folder, and
 **          what the selected one says about itself.
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

#ifndef OPATTERNLIBRARYDIALOG_H
#define OPATTERNLIBRARYDIALOG_H

#include "core/opatternlibrary.h"

#include <QDialog>
#include <QList>
#include <QString>

class QDialogButtonBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QTimer;
class QTreeWidget;
class QTreeWidgetItem;

/**
 * @brief The library of patterns: where they are sorted, what each of them says, and the three gestures that
 *        manage them.
 *
 * The window **does not open a pattern** — that is what the folders of the application are for. It reads what
 * each `.ostara` says about itself, shows it, and offers what a file manager would: **Transfer** a pattern to
 * another folder of the library, **Export** it elsewhere on the computer, **Delete** it. Creating a folder is
 * the fourth gesture, and it can make the client or the collection of the same name at the same time.
 *
 * A folder is a place: the tree on the left is the library's own tree, the library's folder is **not** shown as
 * a level of its own — it is what the window is standing in — and choosing nothing means being at that root.
 * The search is the one gesture that looks everywhere at once, and it waits for the typing to stop.
 *
 * **Nothing watches the folder.** The list is read from the disk when the window is opened, when a folder is
 * chosen, and when the search runs; a pattern renamed or moved in the Finder shows up here at the next read, and
 * not before. That is the same bargain the four older libraries make, and it is deliberate: a watcher would have
 * to decide what to do with a file that vanished under the reader's cursor.
 */
class OPatternLibraryDialog : public QDialog
{
    Q_OBJECT // NOLINT

public:
    /// @param directory the folder the library is read from — given, never guessed, so the window can be
    ///        pointed at any folder a test's patterns live in, and the settings are read once, by the caller.
    explicit OPatternLibraryDialog(const QString &directory, QWidget *parent = nullptr);
    ~OPatternLibraryDialog() override = default;

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void OnSearchChanged(const QString &text);

    /// @brief The search, run once the typing has stopped: it reads the whole library, headers and all.
    void OnSearchTimeout();

    /// @brief The folder the list is showing — nothing chosen means the library's own folder.
    void OnFolderSelected();

    void OnCreateFolder();

    /// @brief A click on one of the three action columns: the pattern of that row is moved, exported or deleted.
    void OnCellClicked(int row, int column);

    void ShowDetails();

    /// @brief The folder being read, opened where the system keeps its files.
    void OnOpenFolder();

private:
    Q_DISABLE_COPY_MOVE(OPatternLibraryDialog) // NOLINT

    void SetupUI();
    void Retranslate();
    void Refresh();
    void PopulateTree();
    void Populate();

    /// @brief The folder the list reads — the chosen one, or the library's own when none is chosen.
    auto ActiveFolder() const -> QString;

    /// @brief The row the selection is on, or -1.
    auto SelectedRow() const -> int;

    void TransferEntry(int row);

    /// @brief Asks where the pattern goes: the library's own folder, or one of its folders.
    /// @return the chosen folder, or an empty string when she changed her mind.
    auto AskTransferFolder(const QString &patternName, const QStringList &labels, const QStringList &paths)
        -> QString;
    void ExportEntry(int row);
    void DeleteEntry(int row);

    /// @brief Whether two paths name the same file, as the file system spells them.
    static auto SameFile(const QString &left, const QString &right) -> bool;

    QLabel *m_searchLabel{nullptr};
    QLineEdit *m_searchEdit{nullptr};
    QPushButton *m_createFolderButton{nullptr};

    QTreeWidget *m_tree{nullptr};
    QTableWidget *m_table{nullptr};

    QLabel *m_stateLabel{nullptr};
    QLabel *m_fileNameLabel{nullptr};
    QGroupBox *m_descriptionGroup{nullptr};
    QGroupBox *m_notesGroup{nullptr};
    QGroupBox *m_fabricGroup{nullptr};
    QLabel *m_descriptionLabel{nullptr};
    QLabel *m_notesLabel{nullptr};
    QLabel *m_fabricLabel{nullptr};

    QPushButton *m_openFolderButton{nullptr};
    QDialogButtonBox *m_buttonBox{nullptr};

    QTimer *m_searchTimer{nullptr};

    QString m_directory{};

    /// The folder chosen in the tree; empty means the library's own folder — the window is standing in it.
    QString m_activeFolder{};

    QList<OPatternLibrary::Entry> m_entries{};
};

#endif // OPATTERNLIBRARYDIALOG_H
