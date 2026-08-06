/************************************************************************
 **
 **  @file   DialogSaveMeasurements.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   22/07/2026
 **
 **  @brief  Save dialog for measurement tables — library + custom location.
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

#ifndef DIALOGSAVEMEASUREMENTS_H
#define DIALOGSAVEMEASUREMENTS_H

#include <QDialog>

class QCheckBox;
class QLineEdit;
class QPushButton;

/// Save dialog replacing Enregistrer/Enregistrer sous with a single modal
/// offering library and/or custom location destinations.
class DialogSaveMeasurements : public QDialog
{
    Q_OBJECT

public:
    /// @param extension   File extension including dot (".omt" or ".otu").
    /// @param defaultName Suggested file name (from table name, already sanitized).
    /// @param currentPath Current file path (empty for new documents).
    /// @param parent      Parent widget.
    explicit DialogSaveMeasurements(const QString &extension,
                                    const QString &defaultName,
                                    const QString &currentPath,
                                    QWidget *parent = nullptr);
    ~DialogSaveMeasurements() override = default;

    /// Full file name including extension (e.g. "32_à_58.omt").
    auto fileName() const -> QString;

    /// Whether the user wants to save to the library folder.
    auto saveToLibrary() const -> bool;

    /// Whether the user wants to save to a custom location.
    auto saveToCustom() const -> bool;

    /// Custom path selected by the user (only valid if saveToCustom()).
    auto customPath() const -> QString;

private slots:
    void OnNameChanged();
    void OnLibraryToggled(bool checked);
    void OnCustomToggled(bool checked);
    void OnBrowse();
    void UpdateSaveButton();

private:
    void SetupUI();

    QString m_extension; // ".omt" or ".otu"

    QLineEdit *m_nameEdit{nullptr};
    QCheckBox *m_libraryCheck{nullptr};
    QCheckBox *m_customCheck{nullptr};
    QLineEdit *m_customPathEdit{nullptr};
    QPushButton *m_browseBtn{nullptr};
    QPushButton *m_saveBtn{nullptr};
};

#endif // DIALOGSAVEMEASUREMENTS_H
