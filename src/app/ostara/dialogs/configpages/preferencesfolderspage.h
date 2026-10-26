/************************************************************************
 **
 **  @file   preferencesfolderspage.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   12 4, 2017
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2017 Valentina project
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

#ifndef PREFERENCESFOLDERSPAGE_H
#define PREFERENCESFOLDERSPAGE_H

#include <QVector>
#include <QWidget>

#include <functional>

class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QToolButton;

/**
 * @brief The folders page — every place where Ostara stores the user's own files.
 *
 * One row per folder under `~/Ostara`: the twelve libraries the application reads and writes
 * (measurements, pieces, patterns, collections, clients, logos, symbols, seam allowances,
 * annotation styles, cartouche templates, PDF templates, cover page templates).
 *
 * A change is written only by Apply(), never by the file dialog, so Cancel leaves both the
 * settings AND the files exactly as they were: the move proposed at pick time is remembered and
 * carried out after the new path is stored.
 */
class PreferencesFoldersPage : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit PreferencesFoldersPage(QWidget *parent = nullptr);
    ~PreferencesFoldersPage() override;

    auto Apply() -> QStringList;

protected:
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(PreferencesFoldersPage) // NOLINT

    struct Folder
    {
        Folder(const char *label, const char *hint, const QString &name, std::function<QString()> get,
               std::function<void(const QString &)> set, std::function<void()> afterChange = nullptr)
          : label(label),
            hint(hint),
            name(name),
            get(std::move(get)),
            set(std::move(set)),
            afterChange(std::move(afterChange))
        {
        }

        const char *label; // source label, retranslated on language change
        const char *hint;  // what the folder holds, shown as the row's tooltip
        QString name;      // subfolder name under ~/Ostara, which gives the default path
        std::function<QString()> get;
        std::function<void(const QString &)> set;

        /// Runs after the new path is stored: a folder that the application seeds or caches
        /// cannot simply be repointed.
        std::function<void()> afterChange;

        QLabel *nameLabel{nullptr};
        QLineEdit *pathEdit{nullptr};
        QToolButton *changeButton{nullptr};

        QString pending; // chosen but not yet written; empty = untouched
        bool moveContents{false};
    };

    QVector<Folder> m_folders;
    QGroupBox *m_group{nullptr};
    QPushButton *m_defaultsButton{nullptr};

    void BuildFolders();
    void BuildRows();
    void Reload();
    void Choose(int row);
    void RestoreDefaults();
    void RetranslateUi();

    auto DefaultPath(const Folder &folder) const -> QString;
    auto DisplayPath(const QString &path) const -> QString;
    void OpenInFinder(int row);
    void RowChanged(int row, const QString &path);
};

#endif // PREFERENCESFOLDERSPAGE_H
