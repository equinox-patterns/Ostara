/************************************************************************
 **
 **  @file   olibraryfolderdialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/10/2026
 **
 **  @brief  A folder made in a library, named and created the same way everywhere.
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

#ifndef OLIBRARYFOLDERDIALOG_H
#define OLIBRARYFOLDERDIALOG_H

#include <QDialog>
#include <QString>

class QDialogButtonBox;
class QLabel;
class QLineEdit;

/**
 * @brief A new folder in a library: one name, and the folder is made under the folder the caller names.
 *
 * The library of pieces sorts its pieces into folders the way the library of patterns does, and the same question
 * is asked in the window, in the save dialog and in the transfer: the answer is a name, and the refusal is the
 * same three rules everywhere — empty, holding a separator, or already taken. The rules themselves live in
 * OLibraryStorage, so this dialog only asks.
 */
class OLibraryFolderDialog : public QDialog
{
    Q_OBJECT // NOLINT

public:
    /// @param parentPath the folder the new one is made in — the folder the library is standing in.
    explicit OLibraryFolderDialog(const QString &parentPath, QWidget *parent = nullptr);
    ~OLibraryFolderDialog() override = default;

    /// @brief The folder that was made, absolute — empty while nothing was, so a caller never guesses a path.
    auto CreatedPath() const -> QString;

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void Retranslate();
    void OnAccept();

private:
    Q_DISABLE_COPY_MOVE(OLibraryFolderDialog) // NOLINT

    void SetupUI();

    QString m_parentPath{};
    QString m_createdPath{};

    QLabel *m_nameLabel{nullptr};
    QLineEdit *m_nameEdit{nullptr};
    QDialogButtonBox *m_buttonBox{nullptr};
};

#endif // OLIBRARYFOLDERDIALOG_H
