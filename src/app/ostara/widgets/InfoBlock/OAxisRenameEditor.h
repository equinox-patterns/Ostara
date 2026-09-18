/************************************************************************
 **
 **  @file   OAxisRenameEditor.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/08/2026
 **
 **  @brief  Per-language display aliases of a size axis and its values (Sizes section).
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

#ifndef OAXISRENAMEEDITOR_H
#define OAXISRENAMEEDITOR_H

#include "OPieceCartoucheOptions.h"

#include "core/otemplatesection.h"

#include <QMap>
#include <QStringList>
#include <QWidget>

class QCheckBox;
class QLabel;
class QTabWidget;
class QVBoxLayout;

/**
 * @brief Per-language display aliases of a size axis and its values.
 *
 * One tab per block language. Each tab pre-fills the axis name and its values
 * with the original table names (the .omt is never modified); editing a field
 * creates a display alias for that language, leaving it unchanged keeps the
 * original. A checkbox switches the preview between one line per language and
 * a single line joining the languages with " - ".
 *
 * The widget only builds the controls and emits the edited data — it does not
 * write the template (Open/Closed).
 */
class OAxisRenameEditor : public QWidget
{
    Q_OBJECT

public:
    explicit OAxisRenameEditor(QWidget *parent = nullptr);

    /// The title shown above the tabs ("Rename the axis and its values").
    void SetTitle(const QString &title);

    /// Feed the current state (aliases, one-line flag, languages, original axis
    /// name and values) and rebuild the tabs.
    void SetData(const QMap<QString, OAxisRename> &renames, bool oneLine, const QStringList &languages,
                 const QString &axisName, const QStringList &axisValues);

signals:
    void RenamesChanged(const QMap<QString, OAxisRename> &renames);
    void OneLineChanged(bool on);

private:
    void Rebuild();

    QVBoxLayout *m_root{nullptr};
    QLabel *m_titleLabel{nullptr};
    QTabWidget *m_tabs{nullptr};
    QCheckBox *m_oneLineCheck{nullptr};

    QString m_title;
    QMap<QString, OAxisRename> m_renames;
    bool m_oneLine{false};
    QStringList m_languages;
    QString m_axisName;
    QStringList m_axisValues;
};

#endif // OAXISRENAMEEDITOR_H
