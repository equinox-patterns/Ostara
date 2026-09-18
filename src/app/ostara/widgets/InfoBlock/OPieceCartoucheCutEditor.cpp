/************************************************************************
 **
 **  @file   OPieceCartoucheCutEditor.cpp
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

#include "OPieceCartoucheCutEditor.h"

#include "OPieceCartoucheOptions.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

namespace
{
/// One fabric: what it is called on the tab, and the keys its two values are written under.
struct FabricTabDef
{
    const char *name;
    const QString &quantityKey;
    const QString &foldKey;
};
} // namespace

//---------------------------------------------------------------------------------------------------------------------

OPieceCartoucheCutEditor::OPieceCartoucheCutEditor(const OTemplateOptionContext &context, QWidget *parent)
  : OTemplateOptionEditor(parent)
{
    static const FabricTabDef kFabrics[] = {
        {QT_TRANSLATE_NOOP("OPieceCartoucheCutEditor", "Fabric"), OPieceCartoucheCut::kFabricQty,
         OPieceCartoucheCut::kFabricOnFold},
        {QT_TRANSLATE_NOOP("OPieceCartoucheCutEditor", "Interfacing"), OPieceCartoucheCut::kInterfacingQty,
         OPieceCartoucheCut::kInterfacingOnFold},
        {QT_TRANSLATE_NOOP("OPieceCartoucheCutEditor", "Lining"), OPieceCartoucheCut::kLiningQty,
         OPieceCartoucheCut::kLiningOnFold},
    };

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *tabs = new QTabWidget(this);
    for (const FabricTabDef &fabric : kFabrics)
    {
        auto *page = new QWidget(tabs);
        auto *pageLayout = new QHBoxLayout(page);
        pageLayout->setContentsMargins(6, 6, 6, 6);

        pageLayout->addWidget(new QLabel(tr("Cut"), page));

        FabricTab tab;
        tab.quantityKey = fabric.quantityKey;
        tab.foldKey = fabric.foldKey;

        tab.quantity = new QSpinBox(page);
        tab.quantity->setRange(0, 99);
        connect(tab.quantity, &QSpinBox::valueChanged, this, [context, key = tab.quantityKey](int value) {
            context.apply(key, value);
        });
        pageLayout->addWidget(tab.quantity);

        tab.onFold = new QCheckBox(tr("On fold"), page);
        connect(tab.onFold, &QCheckBox::toggled, this, [context, key = tab.foldKey](bool on) {
            context.apply(key, on);
        });
        pageLayout->addWidget(tab.onFold);

        pageLayout->addStretch();
        tabs->addTab(page, tr(fabric.name));

        m_tabs.append(tab);
    }

    layout->addWidget(tabs);

    Refresh(context);
}

//---------------------------------------------------------------------------------------------------------------------

void OPieceCartoucheCutEditor::Refresh(const OTemplateOptionContext &context)
{
    for (const FabricTab &tab : m_tabs)
    {
        // Silenced: a refresh reads what the section holds, it does not write it back. Left
        // connected, setting the same value would travel through the connection above and tell the
        // editor the template had been changed — which it has not.
        const QSignalBlocker blockQuantity(tab.quantity);
        const QSignalBlocker blockFold(tab.onFold);
        tab.quantity->setValue(context.options.value(tab.quantityKey, 0).toInt());
        tab.onFold->setChecked(context.options.value(tab.foldKey, false).toBool());
    }
}
