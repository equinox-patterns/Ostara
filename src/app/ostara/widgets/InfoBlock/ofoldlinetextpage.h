/************************************************************************
 **
 **  @file   ofoldlinetextpage.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/10/2026
 **
 **  @brief  Typography of the fold line caption, edited from the Information block window.
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

#ifndef OFOLDLINETEXTPAGE_H
#define OFOLDLINETEXTPAGE_H

#include <QWidget>

class QComboBox;
class QFont;
class QFontComboBox;
class QFormLayout;
class QGroupBox;
class QSpinBox;
class OFoldLineTextPreview;

/**
 * @brief The typography of the fold line caption — « Cut on fold ».
 *
 * A family, a face and a size, and nothing else: this is the font `VFoldLine` builds for the caption
 * wherever it is drawn (the Details canvas, the layout, every export). The text itself belongs to the
 * piece; only its typography is set here.
 *
 * ⚠️ **The preferences are the state.** Every control writes its setting the moment it changes, as
 * the Miniature page does: there is nothing to save and nothing to cancel. It also means these
 * settings are the application's and not a template's — the page edits them wherever it is opened.
 */
class OFoldLineTextPage : public QWidget
{
    Q_OBJECT

public:
    explicit OFoldLineTextPage(QWidget *parent = nullptr);

    /// Reload every control from the preferences — call on entry.
    void Show();

    /// Re-translate the fixed wording (called on QEvent::LanguageChange).
    void RetranslateUI();

signals:
    /** @brief A setting changed: whatever draws a fold line has to be rebuilt to show it. */
    void StylesChanged();

private:
    Q_DISABLE_COPY_MOVE(OFoldLineTextPage) // NOLINT

    void WritePreferences();
    void OnFamilyChanged(const QFont &font);

    OFoldLineTextPreview *m_preview{nullptr};
    QGroupBox *m_textGroup{nullptr};
    QFormLayout *m_form{nullptr};
    QFontComboBox *m_family{nullptr};
    QComboBox *m_face{nullptr};
    QSpinBox *m_size{nullptr};

    /// True while the controls are being filled from the preferences: a widget answering a write is
    /// not the user answering a control, and it must not write back.
    bool m_loading{false};
};

#endif // OFOLDLINETEXTPAGE_H
