/************************************************************************
 **
 **  @file   ofoldlinetextpage.cpp
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

#include "ofoldlinetextpage.h"
#include "ofoldlinetextpreview.h"

#include "core/ofontfaces.h"
#include "vabstractvalapplication.h"
#include "vmisc/FactoryWidgets.h"
#include "vvalentinasettings.h"

#include <QFont>
#include <QFontComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------
OFoldLineTextPage::OFoldLineTextPage(QWidget *parent) : QWidget(parent)
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(8, 8, 8, 8);
    outer->setSpacing(8);

    m_textGroup = new QGroupBox();
    m_form = new QFormLayout(m_textGroup);
    m_form->setContentsMargins(8, 8, 8, 8);
    m_form->setSpacing(6);

    m_family = new QFontComboBox();
    m_form->addRow(QString(), m_family);

    // Only the faces the family really has, italics excluded — see FillFontFaces: a number cannot
    // name a face, and an italic would be drawn upright by the export without saying so.
    m_face = MakeComboBox(this);
    m_form->addRow(QString(), m_face);

    m_size = new QSpinBox();
    m_size->setRange(5, 200);
    m_size->setSuffix(QStringLiteral(" px"));
    m_form->addRow(QString(), m_size);

    outer->addWidget(m_textGroup);

    m_preview = new OFoldLineTextPreview();
    outer->addWidget(m_preview);
    outer->addStretch();

    RetranslateUI();
    Show();

    // **Every control writes its own setting, at once.** The preferences ARE the state: the drawing
    // reads them back on its next rebuild, so there is no copy of them kept here to drift.
    const auto commit = [this]() {
        if (m_loading)
        {
            return;
        }
        WritePreferences();
        m_preview->Refresh();
        emit StylesChanged();
    };

    connect(m_family, &QFontComboBox::currentFontChanged, this,
            [this, commit](const QFont &font) {
                OnFamilyChanged(font);
                commit();
            });
    connect(m_face, &QComboBox::currentIndexChanged, this, [commit](int) { commit(); });
    connect(m_size, &QSpinBox::valueChanged, this, [commit](int) { commit(); });
}

//---------------------------------------------------------------------------------------------------------------------
void OFoldLineTextPage::OnFamilyChanged(const QFont &font)
{
    if (m_loading)
    {
        return;
    }
    // **The faces belong to the family, so they change with it.** The rule lives in FillFontFaces and
    // nowhere else: the face of that name is kept when the new family has one, the family's Regular
    // otherwise.
    FillFontFaces(m_face, font.family(), QString());
}

//---------------------------------------------------------------------------------------------------------------------
void OFoldLineTextPage::WritePreferences()
{
    VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
    settings->SetFoldLineLabelFontFamily(m_family->currentFont().family());
    settings->SetFoldLineLabelFontFace(m_face->currentData().toString());
    settings->SetFoldLineLabelFontPixelSize(m_size->value());
}

//---------------------------------------------------------------------------------------------------------------------
void OFoldLineTextPage::Show()
{
    m_loading = true;
    const VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();

    const QString family = settings->GetFoldLineLabelFontFamily();
    m_family->setCurrentFont(QFont(family));
    m_size->setValue(settings->GetFoldLineLabelFontPixelSize());

    // ⚠️ The face list is refilled from the preference and NOT from what the control was showing:
    // `Show()` is a reload, and a face left over from a previous visit is not the stored answer.
    // `FillFontFaces` keeps the current selection, so it is dropped first.
    m_face->setCurrentIndex(-1);
    FillFontFaces(m_face, family, settings->GetFoldLineLabelFontFace());

    m_loading = false;
    m_preview->Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
void OFoldLineTextPage::RetranslateUI()
{
    m_textGroup->setTitle(tr("Foldline text"));

    const auto relabel = [](QFormLayout *form, QWidget *field, const QString &text) {
        if (auto *label = qobject_cast<QLabel *>(form->labelForField(field)))
        {
            label->setText(text);
        }
    };
    relabel(m_form, m_family, tr("Font:"));
    relabel(m_form, m_face, tr("Weight:"));
    relabel(m_form, m_size, tr("Size:"));
}
