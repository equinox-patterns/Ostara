/************************************************************************
 **
 **  @file   oannotationdialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  Add / edit dialog of one annotation (see oannotationdialog.h).
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

#include "oannotationdialog.h"

#include "../vmisc/FactoryWidgets.h"
#include "../vmisc/def.h"
#include "../vmisc/literals.h"
#include "../vpatterndb/vcontainer.h"
#include "../vtools/dialogs/widgets/angleformulawidget.h"
#include "widgets/InfoBlock/OPieceCartoucheStorage.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------
OAnnotationDialog::OAnnotationDialog(const VContainer *data, QWidget *parent)
  : QDialog(parent), m_data(data)
{
    setWindowTitle(tr("Annotation"));
    setModal(true);

    auto *layout = new QVBoxLayout(this);

    auto *form = new QFormLayout();
    m_templateCombo = MakeComboBox(this);
    // "Default" first (the built-in real style, fallback key of the render);
    // the data is empty for Default (templateName "" = Default).
    m_templateCombo->addItem(tr("Default"), QString());  // no-style fallback (default render)
    const QList<OPieceCartoucheStorage::AnnotationStyleEntry> styles =
        OPieceCartoucheStorage::LoadAnnotationStyles();
    for (const OPieceCartoucheStorage::AnnotationStyleEntry &style : styles)
    {
        m_templateCombo->addItem(style.name, style.name);
    }
    m_templateLabel = new QLabel(tr("Template:"), this);
    form->addRow(m_templateLabel, m_templateCombo);

    m_textEdit = new QPlainTextEdit();
    m_textEdit->setPlaceholderText(tr("Annotation text"));
    m_textLabel = new QLabel(tr("Content:"), this);
    form->addRow(m_textLabel, m_textEdit);

    layout->addLayout(form);

    // Rotation — same composite as the EndLine tool (§5): evaluation debounce
    // against the pattern variables, green/red label, fx button, presets.
    m_formulaAngle = new AngleFormulaWidget(this);
    m_formulaAngle->setData(m_data);
    m_formulaAngle->setToolId(NULL_ID);  // no drawing tool behind a plain annotation
    m_formulaAngle->setLabelText(tr("Angle:"));
    m_formulaAngle->setPostfix(degreeSymbol);
    m_formulaAngle->setDialogTitle(tr("Edit angle"));
    connect(m_formulaAngle, &AngleFormulaWidget::valueChanged, this, [this](bool) { CheckState(); });
    // A fresh annotation is not rotated: 0° by default.
    m_formulaAngle->setFormula(QStringLiteral("0"));
    layout->addWidget(m_formulaAngle);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        SaveData();
        accept();
    });
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(m_buttonBox);

    // Disable OK until the rotation formula is valid (spec §5).
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
auto OAnnotationDialog::IsValid() const -> bool
{
    return m_formulaAngle != nullptr && m_formulaAngle->isValid();
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationDialog::CheckState()
{
    if (m_buttonBox != nullptr)
    {
        QPushButton *ok = m_buttonBox->button(QDialogButtonBox::Ok);
        ok->setEnabled(IsValid());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationDialog::SaveData()
{
    m_templateName = m_templateCombo->currentData().toString();
    m_text = m_textEdit->toPlainText();
    // The widget converts the user formula to the internal form (formula()).
    m_rotationFormula = m_formulaAngle->formula();
}

//---------------------------------------------------------------------------------------------------------------------
auto OAnnotationDialog::TemplateName() const -> QString
{
    return m_templateName;
}

//---------------------------------------------------------------------------------------------------------------------
auto OAnnotationDialog::Text() const -> QString
{
    return m_text;
}

//---------------------------------------------------------------------------------------------------------------------
auto OAnnotationDialog::RotationFormula() const -> QString
{
    return m_rotationFormula;
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationDialog::SetTemplateName(const QString &name)
{
    m_templateName = name;
    if (m_templateCombo == nullptr)
    {
        return;
    }
    m_templateCombo->blockSignals(true);
    int index = m_templateCombo->findData(name);
    if (index < 0)
    {
        if (name.isEmpty())
        {
            index = m_templateCombo->findData(QString());
        }
        else
        {
            // The referenced style was deleted/renamed since: keep a synthetic
            // item so an edit never silently restyles the annotation (pattern
            // PreferredTemplateIndex).
            m_templateCombo->addItem(name, name);
            index = m_templateCombo->count() - 1;
        }
    }
    m_templateCombo->setCurrentIndex(index);
    m_templateCombo->blockSignals(false);
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationDialog::SetText(const QString &text)
{
    m_text = text;
    if (m_textEdit != nullptr)
    {
        m_textEdit->setPlainText(text);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationDialog::SetRotationFormula(const QString &formula)
{
    m_rotationFormula = formula;
    if (m_formulaAngle != nullptr)
    {
        m_formulaAngle->setFormula(formula);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        setWindowTitle(tr("Annotation"));
        m_templateLabel->setText(tr("Template:"));
        m_textLabel->setText(tr("Content:"));
        m_textEdit->setPlaceholderText(tr("Annotation text"));
    }
    QDialog::changeEvent(event);
}
