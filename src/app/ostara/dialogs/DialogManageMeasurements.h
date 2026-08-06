/************************************************************************
 **
 **  @file   DialogManageMeasurements.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   23/07/2026
 **
 **  @brief  Dialog to browse, search, favorite, modify, export and delete
 **          .omt/.otu measurement files in the Ostara library.
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

#ifndef DIALOGMANAGEMEASUREMENTS_H
#define DIALOGMANAGEMEASUREMENTS_H

#include <QDateTime>
#include <QDialog>
#include <QList>
#include <QString>
#include <QStringList>

class QLineEdit;
class QPushButton;
class QTableWidget;
class QTableWidgetItem;
struct LibraryFileEntry;

class DialogManageMeasurements : public QDialog
{
    Q_OBJECT

public:
    explicit DialogManageMeasurements(QWidget *parent = nullptr);
    ~DialogManageMeasurements() override = default;

    /// Returns the file path the user wants to open (double-click or Modifier),
    /// or an empty string if the dialog was cancelled.
    auto selectedPath() const -> QString;

private slots:
    void OnSearchChanged(const QString &text);
    void OnDoubleClick(int row, int column);
    void OnToggleFavorite();
    void OnOpenFolder();

private:
    void SetupUI();
    void RefreshData();
    void PopulateTable();
    void FilterTable(const QString &filter);

    QLineEdit *m_searchEdit{nullptr};
    QTableWidget *m_table{nullptr};
    QPushButton *m_openFolderBtn{nullptr};

    QList<LibraryFileEntry> m_entries;
    QStringList m_favorites;
    QString m_selectedPath;
};

#endif // DIALOGMANAGEMEASUREMENTS_H
