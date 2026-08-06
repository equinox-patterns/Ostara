/************************************************************************
 **
 **  @file   DialogImportStructure.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   23/07/2026
 **
 **  @brief  Dialog to pick a measurement file from the library for import.
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

#ifndef DIALOGIMPORTSTRUCTURE_H
#define DIALOGIMPORTSTRUCTURE_H

#include <QDialog>
#include <QStringList>
#include <QList>

#include "../MeasurementLibraryScanner.h"

class QListWidget;
class QListWidgetItem;
class QPushButton;

class DialogImportStructure : public QDialog
{
    Q_OBJECT

public:
    explicit DialogImportStructure(const QList<LibraryFileEntry> &entries,
                                    QStringList favorites,
                                    QWidget *parent = nullptr);
    ~DialogImportStructure() override = default;

    auto selectedFilePath() const -> QString;
    auto favorites() const -> QStringList;

private slots:
    void OnSelectionChanged();
    void OnItemDoubleClicked(QListWidgetItem *item);
    void OnToggleFavorite();

private:
    void SetupUI();
    void PopulateList();
    void UpdateImportButton();
    void RefreshFavoriteStars();

    struct EntryItem
    {
        LibraryFileEntry entry;
        QListWidgetItem *item{nullptr};
    };

    QList<EntryItem> m_entryItems;
    QStringList m_favorites;
    QListWidget *m_listWidget{nullptr};
    QPushButton *m_importBtn{nullptr};
    QPushButton *m_favoriteBtn{nullptr};
};

#endif // DIALOGIMPORTSTRUCTURE_H
