/************************************************************************
 **
 **  @file   opatternfolderdialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/10/2026
 **
 **  @brief  A folder made in the pattern library — and, when asked, the client or the collection of the same name.
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

#ifndef OPATTERNFOLDERDIALOG_H
#define OPATTERNFOLDERDIALOG_H

#include <QDialog>
#include <QString>

class QCheckBox;
class QDialogButtonBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;

/**
 * @brief A new folder in the library of patterns, and the two bridges that go with it.
 *
 * A reader who sorts her patterns by client — or by collection — makes the same thing twice: a folder on the
 * disk, and an entry in one of the two libraries of the Information block. This dialog makes both at once when
 * she asks for it, and the fields are the ones those pages hold: e-mail, phone and information for a client;
 * year, period and description for a collection.
 *
 * When the name is already taken **on the other side**, the reader is told and offered to link to what exists:
 * a second "Dupont" would be a second line no one could tell from the first. If she refuses, the name has to
 * change — nothing is created meanwhile, so refusing costs nothing.
 */
class OPatternFolderDialog : public QDialog
{
    Q_OBJECT // NOLINT

public:
    /// @param parentPath the folder the new one is made in — the folder the library window is standing in.
    explicit OPatternFolderDialog(const QString &parentPath, QWidget *parent = nullptr);
    ~OPatternFolderDialog() override = default;

    /// @brief The folder that was made, absolute — empty while nothing was, so a caller never guesses a path.
    auto CreatedPath() const -> QString;

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void Retranslate();
    void OnAddToClientToggled(bool checked);
    void OnAddToCollectionToggled(bool checked);

    /// @brief Everything the dialog promised: the folder, and the client or the collection when asked for.
    void OnAccept();

private:
    Q_DISABLE_COPY_MOVE(OPatternFolderDialog) // NOLINT

    void SetupUI();
    void ShowClientFields(bool visible);
    void ShowCollectionFields(bool visible);

    /// @brief The id of the client @p name already names, empty when no client does — case-insensitively, so
    /// two entries a combo could not tell apart are one entry here.
    static auto ClientNamed(const QString &name) -> QString;

    /// @brief The same, for the collections.
    static auto CollectionNamed(const QString &name) -> QString;

    QString m_parentPath{};
    QString m_createdPath{};

    QLabel *m_nameLabel{nullptr};
    QLineEdit *m_nameEdit{nullptr};

    QCheckBox *m_addClientCheck{nullptr};
    QGroupBox *m_clientGroup{nullptr};
    QLabel *m_clientMailLabel{nullptr};
    QLabel *m_clientPhoneLabel{nullptr};
    QLabel *m_clientInfoLabel{nullptr};
    QLineEdit *m_clientMailEdit{nullptr};
    QLineEdit *m_clientPhoneEdit{nullptr};
    QPlainTextEdit *m_clientInfoEdit{nullptr};

    QCheckBox *m_addCollectionCheck{nullptr};
    QGroupBox *m_collectionGroup{nullptr};
    QLabel *m_collectionYearLabel{nullptr};
    QLabel *m_collectionPeriodLabel{nullptr};
    QLabel *m_collectionDescriptionLabel{nullptr};
    QSpinBox *m_collectionYearSpin{nullptr};
    QLineEdit *m_collectionPeriodEdit{nullptr};
    QPlainTextEdit *m_collectionDescriptionEdit{nullptr};

    QDialogButtonBox *m_buttonBox{nullptr};
};

#endif // OPATTERNFOLDERDIALOG_H
