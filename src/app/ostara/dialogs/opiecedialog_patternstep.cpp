/************************************************************************
 **
 **  @file   opiecedialog_patternstep.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   28 August, 2026
 **
 **  @brief  Step 1 of the piece tool dialog: pattern-level information.
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

#include "opiecedialog_patternstep.h"

#include "../widgets/InfoBlock/OPieceCartoucheStorage.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vcommonsettings.h"

#include <algorithm>

#include <QCheckBox>
#include <QComboBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------
OPiecePatternStep::OPiecePatternStep(VAbstractPattern *doc, QWidget *parent)
  : QWidget(parent),
    m_doc(doc)
{
    SCASSERT(m_doc != nullptr)

    auto *mainLayout = new QVBoxLayout(this);
    auto *form = new QFormLayout();

    // Cartouche (template) — default from settings, "No cartouche" as an explicit option.
    OPieceCartoucheStorage::EnsureDefaultTemplate();
    m_cartoucheCombo = new QComboBox();
    m_cartoucheCombo->addItem(tr("No cartouche"), QString());
    const QStringList templates = OPieceCartoucheStorage::ScanDirectory(OPieceCartoucheStorage::DefaultDirectory());
    const QString defaultName = VAbstractValApplication::VApp()->Settings()->GetDefaultCartoucheTemplate();
    int defaultIndex = 0;
    for (const QString &path : templates)
    {
        const QString base = QFileInfo(path).completeBaseName();
        m_cartoucheCombo->addItem(base, path);
        if (base == defaultName)
        {
            defaultIndex = m_cartoucheCombo->count() - 1;
        }
    }
    m_cartoucheCombo->setCurrentIndex(defaultIndex);
    form->addRow(tr("Cartouche"), m_cartoucheCombo);

    m_patternNameEdit = new QLineEdit(m_doc->GetPatternName());
    form->addRow(tr("Pattern name"), m_patternNameEdit);

    m_clientEdit = new QLineEdit(m_doc->GetCustomer());
    form->addRow(tr("Client"), m_clientEdit);

    m_collectionEdit = new QLineEdit(m_doc->GetCollection());
    form->addRow(tr("Collection"), m_collectionEdit);

    // Fabric info — grows with its content (3 visible lines minimum).
    m_fabricInfoEdit = new QPlainTextEdit(m_doc->GetFabricInfo());
    m_fabricInfoEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    const auto minFabricHeight = [this]() {
        return m_fabricInfoEdit->fontMetrics().lineSpacing() * 3 + m_fabricInfoEdit->frameWidth() * 2
               + static_cast<int>(m_fabricInfoEdit->document()->documentMargin()) * 2;
    };
    m_fabricInfoEdit->setFixedHeight(minFabricHeight());
    connect(m_fabricInfoEdit->document(), &QTextDocument::contentsChanged, this, [this, minFabricHeight]() {
        const int h = static_cast<int>(m_fabricInfoEdit->document()->size().height())
                      + m_fabricInfoEdit->frameWidth() * 2;
        m_fabricInfoEdit->setFixedHeight(std::max(h, minFabricHeight()));
    });
    form->addRow(tr("Fabric info"), m_fabricInfoEdit);

    mainLayout->addLayout(form);

    m_activateSACheck = new QCheckBox(tr("Activate seam allowances for all pieces"));
    m_activateSACheck->setChecked(m_doc->GetDefaultActivateSA());
    mainLayout->addWidget(m_activateSACheck);

    m_hideMainPathCheck = new QCheckBox(tr("Hide main path for all pieces"));
    m_hideMainPathCheck->setChecked(m_doc->GetDefaultHideMainPath());
    mainLayout->addWidget(m_hideMainPathCheck);

    mainLayout->addStretch();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePatternStep::GetCartouche() const -> QString
{
    return m_cartoucheCombo->currentData().toString();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePatternStep::GetPatternName() const -> QString
{
    return m_patternNameEdit->text().trimmed();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePatternStep::GetClient() const -> QString
{
    return m_clientEdit->text().trimmed();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePatternStep::GetCollection() const -> QString
{
    return m_collectionEdit->text().trimmed();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePatternStep::GetFabricInfo() const -> QString
{
    return m_fabricInfoEdit->toPlainText().trimmed();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePatternStep::IsActivateSA() const -> bool
{
    return m_activateSACheck->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePatternStep::IsHideMainPath() const -> bool
{
    return m_hideMainPathCheck->isChecked();
}
