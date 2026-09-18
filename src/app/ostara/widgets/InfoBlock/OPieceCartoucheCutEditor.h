/************************************************************************
 **
 **  @file   OPieceCartoucheCutEditor.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  The Cut information's control: how many of each fabric, and whether on the fold.
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

#ifndef OPIECECARTOUCHECUTEDITOR_H
#define OPIECECARTOUCHECUTEDITOR_H

#include "core/otemplateoptioneditor.h"

#include <QList>

class QCheckBox;
class QSpinBox;

/**
 * @brief The Cut information's control — one tab per fabric.
 *
 * **The cartouche's own, and it says so by living here.** The shared section editor builds it
 * because the catalogue's field asked for it, not because it recognises a cut: what a fabric is,
 * which keys hold its quantity, and how many tabs there are is knowledge of this family alone.
 *
 * One information covers all three fabrics, so the three tabs are three fabrics rather than three
 * informations — a piece is cut in fabric, in interfacing and in lining, and none of them is worth
 * a separate entry in a menu the user reads to decide what to show.
 */
class OPieceCartoucheCutEditor : public OTemplateOptionEditor
{
    Q_OBJECT

public:
    explicit OPieceCartoucheCutEditor(const OTemplateOptionContext &context, QWidget *parent = nullptr);

    /**
     * @brief Re-reads the three quantities.
     *
     * Nothing outside this control changes them — they belong to one information and one piece —
     * but the interface is the same for every composite, and a control that ignored it would be
     * the one that breaks when something else learns to write them.
     */
    void Refresh(const OTemplateOptionContext &context) override;

private:
    /// One fabric's tab: its quantity, whether it is cut on the fold, and the keys they live under.
    struct FabricTab
    {
        QSpinBox *quantity{nullptr};
        QCheckBox *onFold{nullptr};
        QString quantityKey;
        QString foldKey;
    };

    QList<FabricTab> m_tabs;
};

#endif // OPIECECARTOUCHECUTEDITOR_H
