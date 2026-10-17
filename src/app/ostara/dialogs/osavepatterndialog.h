/************************************************************************
 **
 **  @file   osavepatterndialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/10/2026
 **
 **  @brief  Where a pattern is written: in the library, in a folder of it, or somewhere else entirely.
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

#ifndef OSAVEPATTERNDIALOG_H
#define OSAVEPATTERNDIALOG_H

#include <QDialog>
#include <QString>

class QCheckBox;
class QDialogButtonBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTreeWidget;

/**
 * @brief Where a pattern is written: in the library, in a folder of it, or somewhere else on the computer.
 *
 * The library is **offered first, and ticked**: that is where a pattern is kept, and a reader asked the same
 * question twice — "which folder?" — soon stops reading the question at all. Unticking the box leaves the
 * gesture exactly as it was before: the folder is asked for by the save dialog of the system.
 *
 * The name here is the **file's** name. The pattern's own name lives in the Property Browser and is not touched:
 * the two are deliberately not the same thing, and the library shows the file's name only when the pattern has
 * none. The extension is written by this dialog, never typed.
 */
class OSavePatternDialog : public QDialog
{
    Q_OBJECT // NOLINT

public:
    /// @brief What the reader asked for when the dialog closed.
    enum class Destination
    {
        Cancelled, ///< nothing was asked.
        Library,   ///< the library — the path is the one LibraryPath() returns.
        Elsewhere, ///< not the library: the caller asks for a folder the way it always has.
    };

    /// @param libraryDirectory the library's own folder — given, never guessed, like the window's.
    /// @param proposedName the file name to propose, extension included.
    /// @param currentPath the pattern's file, when it already has one: it decides the folder offered first.
    explicit OSavePatternDialog(const QString &libraryDirectory, const QString &proposedName,
                                const QString &currentPath, QWidget *parent = nullptr);
    ~OSavePatternDialog() override = default;

    auto ChosenDestination() const -> Destination;

    /// @brief The file to write — only meaningful when the destination is the library.
    auto LibraryPath() const -> QString;

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void Retranslate();
    void OnLibraryToggled(bool checked);
    void OnCreateFolder();
    void OnAccept();

private:
    Q_DISABLE_COPY_MOVE(OSavePatternDialog) // NOLINT

    void SetupUI();
    void PopulateTree();
    void ShowDestination();

    /// @brief The folder the file would go into: the chosen one, or the library's own when none is chosen.
    auto TargetFolder() const -> QString;

    /// @brief The whole path the file would be written to.
    auto TargetPath() const -> QString;

    /// @brief The name without its extension, and without what a name can never hold.
    static auto CleanName(const QString &name) -> QString;

    QString m_library{};
    QString m_currentPath{};

    /// @brief What she answered — the only thing the caller reads.
    Destination m_destination{Destination::Cancelled};

    QLabel *m_nameLabel{nullptr};
    QLineEdit *m_nameEdit{nullptr};
    QCheckBox *m_libraryCheck{nullptr};
    QGroupBox *m_treeGroup{nullptr};
    QTreeWidget *m_tree{nullptr};
    QPushButton *m_createFolderButton{nullptr};
    QLabel *m_destinationLabel{nullptr};
    QPushButton *m_otherFolderButton{nullptr};

    /// @brief The button that saves — kept, because a button added with a role carries no standard text.
    QPushButton *m_saveButton{nullptr};

    QDialogButtonBox *m_buttonBox{nullptr};
};

#endif // OSAVEPATTERNDIALOG_H
