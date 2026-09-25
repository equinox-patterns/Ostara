/************************************************************************
 **
 **  @file   othumbnailstylespage.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   25/09/2026
 **
 **  @brief  The Thumbnail page of the information block editor: what the
 **          words of a miniature are set in, and the units they state.
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

#ifndef OTHUMBNAILSTYLESPAGE_H
#define OTHUMBNAILSTYLESPAGE_H

#include <QFont>
#include <QStringList>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QFontComboBox;
class QFormLayout;
class QGroupBox;
class QLabel;
class QSpinBox;
class OThumbnailPreview;

/**
 * @brief The typography and the units of every miniature.
 *
 * Two texts are set here and nothing else: the seam-allowance values a piece is cut with, and the
 * legend that names their unit. Each carries a family, a face and a size; the two units are a pair of
 * switches. A miniature draws no drawing on this page — a piece belongs to a pattern, and the block
 * editor has none — so the preview below shows the wording alone (see @ref OThumbnailPreview).
 *
 * ⚠️ **The preferences are the state.** Every control writes its setting the moment it changes, and
 * the preview reads the preferences back: no copy of them is kept here, so the page and the drawing
 * cannot come to hold two different answers. It also means these settings are the application's and
 * not a template's — the page edits them wherever it is opened, and the miniatures already drawn
 * obey them as the ones drawn later do.
 */
class OThumbnailStylesPage : public QWidget
{
    Q_OBJECT

public:
    explicit OThumbnailStylesPage(QWidget *parent = nullptr);

    /// Reload every control from the preferences — call on entry, and after any outside change.
    void Show();

    /// The languages the legend is written in — the edited template's own, one line each.
    void SetLanguages(const QStringList &languages);

    /// Re-translate the fixed wording (called on QEvent::LanguageChange).
    void RetranslateUI();

private:
    /// One text's three controls: the values', or the legend's.
    struct FontControls
    {
        QFontComboBox *family{nullptr};
        QComboBox *face{nullptr};
        QSpinBox *size{nullptr};
    };

    auto MakeFontGroup(const QString &title, FontControls &controls, QFormLayout *&form) -> QGroupBox *;
    void OnFamilyChanged(FontControls &controls, const QFont &font);
    void OnUnitToggled(bool centimeters, bool checked);
    void WritePreferences();
    void RefreshPreview();

    FontControls m_values;
    FontControls m_legend;
    QGroupBox *m_valuesGroup{nullptr};
    QGroupBox *m_legendGroup{nullptr};
    QGroupBox *m_unitsGroup{nullptr};
    QFormLayout *m_valuesForm{nullptr};
    QFormLayout *m_legendForm{nullptr};
    QCheckBox *m_centimetersCheck{nullptr};
    QCheckBox *m_inchesCheck{nullptr};
    QLabel *m_hint{nullptr};
    OThumbnailPreview *m_preview{nullptr};

    /// True while the controls are being filled from the preferences: a widget answering a write is
    /// not the user answering a control, and it must not write back.
    bool m_loading{false};
};

#endif // OTHUMBNAILSTYLESPAGE_H
