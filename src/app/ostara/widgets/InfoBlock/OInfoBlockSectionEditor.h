/************************************************************************
 **
 **  @file   OInfoBlockSectionEditor.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/08/2026
 **
 **  @brief  Data-driven edit zone for one template section.
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

#ifndef OINFOBLOCKSECTIONEDITOR_H
#define OINFOBLOCKSECTIONEDITOR_H

#include "OPieceCartoucheTemplate.h"

#include <QVBoxLayout>
#include <QWidget>

class OAxisRenameEditor;
class OPieceCartoucheTable;

/**
 * @brief Edit zone for the currently selected template section.
 *
 * The SOLID core: the widget does not know the section types — it reads the
 * option schema of the selected type and draws one control per field
 * (Open/Closed: a new section type does not change this widget). It also shows
 * the section description (the complementary information, spec §7).
 */
class OInfoBlockSectionEditor : public QWidget
{
    Q_OBJECT

public:
    explicit OInfoBlockSectionEditor(QWidget *parent = nullptr);

    /// Edit section `index` of `tpl` (lifetime owned by the caller). -1 clears.
    /// `table` gives read access to the loaded .omt for the table-driven controls.
    void SetSection(OPieceCartoucheTemplate *tpl, int index, const OPieceCartoucheTable *table);

    /// Re-translate the dynamic content (called on QEvent::LanguageChange).
    void RetranslateUI();

signals:
    /// Fired when any option changes (the editor marks the template modified).
    void OptionsChanged();

private:
    void Rebuild();
    void UpdateAxisRenameEditor();

    QVBoxLayout *m_root{nullptr};
    QWidget *m_content{nullptr};
    OPieceCartoucheTemplate *m_template{nullptr};
    const OPieceCartoucheTable *m_table{nullptr};
    int m_index{-1};
    OAxisRenameEditor *m_axisRenameEditor{nullptr};
};

#endif // OINFOBLOCKSECTIONEDITOR_H
