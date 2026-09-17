/************************************************************************
 **
 **  @file   oexporttemplatespage.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/09/2026
 **
 **  @brief  The « Existing templates » page of the Export template editor.
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

#ifndef OEXPORTTEMPLATESPAGE_H
#define OEXPORTTEMPLATESPAGE_H

#include <QList>
#include <QString>
#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

/**
 * @brief The list of saved templates: a search bar, and one row per template.
 *
 * Four columns — the star of the single favourite, the name, edit, delete — and an « Open folder »
 * button under them. The Information block's page, one column shorter: a cartouche template is
 * written in a language and says so, a template of assembly marks is not and does not.
 *
 * **Opening a row hands a path over and touches nothing else.** The page knows the library and
 * nothing about the editor it sits in; the editor loads the file and fills itself in.
 */
class OExportTemplatesPage : public QWidget
{
    Q_OBJECT

public:
    explicit OExportTemplatesPage(QWidget *parent = nullptr);

    /** @brief Re-scans the templates folder and rebuilds the table. */
    void Refresh();

    /** @brief Re-applies every user-visible string, after a language change. */
    void RetranslateUI();

signals:
    /** @brief The user wants to open this template in the editor. */
    void OpenTemplate(const QString &filePath);

private:
    struct Entry
    {
        QString name; ///< the file's base name, which is the template's identity
        QString path; ///< absolute path
    };

    void SetupUI();
    void PopulateTable();
    void FilterTable(const QString &filter);
    void OnOpenFolder();
    void OnCellClicked(int row, int column);

    QLabel *m_title{nullptr};
    QLineEdit *m_searchEdit{nullptr};
    QTableWidget *m_table{nullptr};
    QPushButton *m_openFolderBtn{nullptr};
    QList<Entry> m_entries;
    QString m_defaultName;
};

#endif // OEXPORTTEMPLATESPAGE_H
