/************************************************************************
 **
 **  @file   oannotationstylespage.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  Annotation styles page of the cartouche editor: left column
 **          manages the style library (list + edit form), right column
 **          previews the selected style. Persists to `annotationstyles.xml`
 **          through OPieceCartoucheStorage.
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

#ifndef OANNOTATIONSTYLESPAGE_H
#define OANNOTATIONSTYLESPAGE_H

#include <QWidget>

#include "OPieceCartoucheStorage.h"

class QCheckBox;
class QComboBox;
class QFontComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QTreeWidget;
class QTreeWidgetItem;
class OAnnotationPreview;

/**
 * @brief Annotation styles page of the cartouche editor.
 *
 * Mirrors the Logo page structure (list + edit form on the left, preview on
 * the right) but manages the annotation style library: a named font + weight
 * + size + optional background + optional white text. Every edit persists
 * atomically through OPieceCartoucheStorage.
 */
class OAnnotationStylesPage : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit OAnnotationStylesPage(QWidget *parent = nullptr);
    ~OAnnotationStylesPage() override = default;

    /// Reload the library from disk and select the Default style.
    void Show();
    void RetranslateUI();

signals:
    void StatusMessage(const QString &message);

private slots:
    void OnStyleSelected(QTreeWidgetItem *current);
    void OnStyleNameEdited(const QString &text);
    void OnStyleFieldChanged();
    void OnBackgroundTextChanged(const QString &text);
    void OnBackgroundEditingFinished();
    void OnPresetClicked(const QString &hex);
    void OnPreviewTextChanged();
    void AddStyle();
    void RemoveStyle();

private:
    void RefreshStylesList();
    void UpdateRow(QTreeWidgetItem *item, const OPieceCartoucheStorage::AnnotationStyleEntry &entry);
    void PersistStyles();
    void UpdatePreview();
    void ApplyBackgroundValidity();
    void UpdateSwatch();
    void RebuildWeightCombo();
    void SelectStyle(const QString &name);
    static auto NormalizeHex(const QString &text) -> QString;
    static auto IsValidHex(const QString &text) -> bool;

    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(OAnnotationStylesPage) // NOLINT

    QPushButton *m_addStyleBtn{nullptr};
    QPushButton *m_removeStyleBtn{nullptr};
    QTreeWidget *m_stylesTree{nullptr};

    QGroupBox *m_styleEditGroup{nullptr};
    QLineEdit *m_nameEdit{nullptr};
    QFontComboBox *m_fontCombo{nullptr};
    QComboBox *m_weightCombo{nullptr};
    QSpinBox *m_sizeSpin{nullptr};
    QLineEdit *m_backgroundEdit{nullptr};
    QLabel *m_backgroundSwatch{nullptr};
    QPushButton *m_presetWhiteBtn{nullptr};
    QPushButton *m_presetBlackBtn{nullptr};
    QCheckBox *m_whiteTextCheck{nullptr};

    QPlainTextEdit *m_previewTextEdit{nullptr};
    OAnnotationPreview *m_preview{nullptr};

    QList<OPieceCartoucheStorage::AnnotationStyleEntry> m_styleEntries;
    bool m_updatingStyles{false};
};

#endif // OANNOTATIONSTYLESPAGE_H
