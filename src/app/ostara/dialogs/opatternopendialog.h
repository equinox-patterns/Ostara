/************************************************************************
 **
 **  @file   opatternopendialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/10/2026
 **
 **  @brief  Open a pattern: from the pattern library, or from anywhere on the computer.
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

#ifndef OPATTERNOPENDIALOG_H
#define OPATTERNOPENDIALOG_H

#include "core/opatternlibrary.h"

#include <QDialog>
#include <QList>
#include <QString>

class QComboBox;
class QDialogButtonBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;

/**
 * @brief Opening a pattern: the library first, the whole computer second.
 *
 * The welcome screen's « Open pattern… » used to drop straight into the system's file panel, which knows nothing
 * about the library — the folders a reader made by hand, one per client, were invisible there. This asks the
 * question the way she thinks about it: **a folder of the library, then a pattern**; and only if it is not in
 * the library, a folder of the computer.
 *
 * The list shows the pattern's **own name** and the day it was last written — the file name goes to the tooltip.
 * Two patterns can carry the same name in two folders, and a name alone would not say which is the newer one.
 * The search narrows the folder's list as she types: a folder of thirty patterns is read faster that way than
 * by scrolling.
 */
class OPatternOpenDialog : public QDialog
{
    Q_OBJECT // NOLINT

public:
    explicit OPatternOpenDialog(const QString &libraryDirectory, QWidget *parent = nullptr);
    ~OPatternOpenDialog() override = default;

    /// @brief The file to open — empty when she changed her mind.
    auto SelectedPath() const -> QString;

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void Retranslate();
    void OnFolderChanged();
    void OnSearchChanged();
    void OnBrowse();
    void OnItemActivated(QListWidgetItem *item);

private:
    Q_DISABLE_COPY_MOVE(OPatternOpenDialog) // NOLINT

    void SetupUI();
    void PopulateFolders();
    void PopulateList();
    void ShowEmptyState();

    /// @brief The folder the list is reading right now — the library's own when nothing is chosen.
    auto CurrentFolder() const -> QString;

    QString m_library{};
    QList<OPatternLibrary::Entry> m_entries{};
    QString m_selectedPath{};

    QGroupBox *m_libraryGroup{nullptr};
    QLabel *m_folderLabel{nullptr};
    QComboBox *m_folderCombo{nullptr};
    QLabel *m_searchLabel{nullptr};
    QLineEdit *m_searchEdit{nullptr};
    QListWidget *m_list{nullptr};
    QLabel *m_hint{nullptr};
    QGroupBox *m_otherGroup{nullptr};
    QPushButton *m_browseButton{nullptr};
    QLabel *m_browseHint{nullptr};
    QDialogButtonBox *m_buttonBox{nullptr};
};

#endif // OPATTERNOPENDIALOG_H
