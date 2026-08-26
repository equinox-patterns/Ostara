/************************************************************************
 **
 **  @file   OInfoBlockTemplatesPage.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/08/2026
 **
 **  @brief  Cartouche template library — search, single favorite, open, delete.
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

#ifndef OINFOBLOCKTEMPLATESPAGE_H
#define OINFOBLOCKTEMPLATESPAGE_H

#include <QList>
#include <QString>
#include <QStringList>
#include <QWidget>

class QLineEdit;
class QPushButton;
class QTableWidget;

/**
 * @brief The « Existing templates » page of the Information block editor.
 *
 * A table with a search bar, a single favorite (the default cartouche used by
 * the future Piece tool dialog), open / edit, delete and an "Open folder"
 * button. Pattern: DialogManageMeasurements, minus the dates and the export.
 * The Languages column shows each template's block languages (flag + native
 * name).
 */
class OInfoBlockTemplatesPage : public QWidget
{
    Q_OBJECT

public:
    explicit OInfoBlockTemplatesPage(QWidget *parent = nullptr);

    /// Re-scan the templates folder and repopulate the table.
    void Refresh();

    /// Re-translate the fixed literals (called on QEvent::LanguageChange).
    void RetranslateUI();

signals:
    /// The user wants to open (edit) this template.
    void OpenTemplate(const QString &filePath);

private:
    struct Entry
    {
        QString name;        // file base name (also the favorite key)
        QString path;        // absolute path
        QStringList languages;
    };

    void SetupUI();
    void PopulateTable();
    void FilterTable(const QString &filter);
    void OnOpenFolder();
    void OnCellClicked(int row, int column);

    QLineEdit *m_searchEdit{nullptr};
    QTableWidget *m_table{nullptr};
    QPushButton *m_openFolderBtn{nullptr};
    QList<Entry> m_entries;
    QString m_defaultName;
};

#endif // OINFOBLOCKTEMPLATESPAGE_H
