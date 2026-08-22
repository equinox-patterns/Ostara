/************************************************************************
 **
 **  @file   odetailssavedmarginsdialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/08/2026
 **
 **  @brief  Saved custom seam allowances management dialog
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

#include "odetailssavedmarginsdialog.h"

#include "../vwidgets/osamarginpalette.h"
#include "osavedmarginrowwidget.h"

#include "../vmisc/def.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vvalentinasettings.h"

#include <QPalette>

#include <QComboBox>
#include <QFrame>
#include <QLocale>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QScrollArea>
#include <QVBoxLayout>

namespace
{
[[nodiscard]] auto ParseSavedValue(const QString &text, bool *ok) -> double
{
    QString normalized = text;
    normalized.replace(QLatin1Char(','), QLatin1Char('.'));
    return QLocale::c().toDouble(normalized, ok);
}

const QRegularExpression kSavedDecimalRegex(QStringLiteral("[0-9]*[\\.,]?[0-9]*"));
} // namespace

//---------------------------------------------------------------------------------------------------------------------
ODetailsSavedMarginsDialog::ODetailsSavedMarginsDialog(QWidget *parent)
  : QDialog(parent)
{
    setWindowTitle(tr("Saved custom seam allowances"));
    resize(520, 300);

    auto *layout = new QVBoxLayout(this);

    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_listContainer = new QWidget();
    m_listLayout = new QVBoxLayout(m_listContainer);
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    m_listLayout->setSpacing(0);
    m_scrollArea->setWidget(m_listContainer);
    layout->addWidget(m_scrollArea);

    // The scroll-area viewport paints a flat NATIVE background (white on macOS) that
    // overrides global QSS and palette. Only an inline dot-selector with an OPAQUE
    // color works (SubtiliteCode §13); `transparent` would let the native white show
    // through. Use the Window color (#232323 dark / #F9F9F9 light), read from the
    // palette so it always follows the current theme.
    m_scrollArea->viewport()->setStyleSheet(
        QStringLiteral(".QWidget { background: %1; }").arg(palette().color(QPalette::Window).name()));

    const auto saved = VAbstractValApplication::VApp()->ValentinaSettings()->GetSavedSeamAllowances();
    for (const auto &s : saved)
    {
        AddRow(s);
    }

    // The widgetResizable scroll area stretches the container to the viewport height;
    // without this, the spare space would be spread into gaps between the rows. Absorb
    // it at the end so the rows stay stacked one after another at the top.
    m_listLayout->addStretch(1);

    auto *closeBtn = new QPushButton(tr("Close"));
    // Real default button of the dialog — Enter closes it.
    closeBtn->setDefault(true);
    layout->addWidget(closeBtn);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsSavedMarginsDialog::AddRow(const SavedSeamAllowance &s)
{
    auto *row = new OSavedMarginRowWidget(m_listContainer);
    // Vertical Fixed: rows keep their sizeHint and stack one after another instead
    // of being stretched by the container layout's spare space.
    row->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_listLayout->addWidget(row);

    row->nameEdit()->setText(s.name);

    const Unit patternUnit = VAbstractValApplication::VApp()->patternUnits();
    const qreal displayVal = UnitConvertor(s.value, Unit::Mm, patternUnit);
    row->valueEdit()->setText(QString::number(displayVal, 'f', 1));
    row->valueEdit()->setValidator(new QRegularExpressionValidator(kSavedDecimalRegex, this));

    const int colorIdx = (s.colorIndex >= 0 && s.colorIndex < OSAMarginPalette::kColorCount) ? s.colorIndex : 0;
    row->colorCombo()->setCurrentIndex(colorIdx);

    connect(row->nameEdit(), &QLineEdit::editingFinished, this, &ODetailsSavedMarginsDialog::Persist);
    connect(row->valueEdit(), &QLineEdit::editingFinished, this, &ODetailsSavedMarginsDialog::Persist);
    connect(row->colorCombo(), QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ODetailsSavedMarginsDialog::Persist);
    connect(row->deleteButton(), &QPushButton::clicked, this, [this, row]()
    {
        delete row;
        Persist();
    });
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsSavedMarginsDialog::Persist()
{
    QVector<SavedSeamAllowance> list;
    const Unit patternUnit = VAbstractValApplication::VApp()->patternUnits();
    for (int i = 0; i < m_listLayout->count(); ++i)
    {
        auto *row = qobject_cast<OSavedMarginRowWidget *>(m_listLayout->itemAt(i)->widget());
        if (row == nullptr)
        {
            continue;
        }

        SavedSeamAllowance s;
        s.name = row->nameEdit()->text().trimmed();
        bool ok = false;
        const double v = ParseSavedValue(row->valueEdit()->text(), &ok);
        s.value = UnitConvertor(ok ? v : 0, patternUnit, Unit::Mm);
        s.colorIndex = row->colorCombo()->currentIndex();
        list.append(s);
    }
    VAbstractValApplication::VApp()->ValentinaSettings()->SetSavedSeamAllowances(list);
}
