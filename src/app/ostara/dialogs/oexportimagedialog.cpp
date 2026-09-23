/************************************************************************
 **
 **  @file   oexportimagedialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   23/09/2026
 **
 **  @brief  Implementation of the image export's first step.
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

#include "oexportimagedialog.h"

#include "../core/oexportunitformat.h"
#include "../vmisc/FactoryWidgets.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>

namespace
{
/// The resolution a raster opens on. The print standard, and more than a plan read on a page
/// ever needs — the field is there to be lowered, not raised.
constexpr int kDefaultDpi = 300;
constexpr int kMinDpi = 72;
constexpr int kMaxDpi = 1200;

using Format = OExportPlanImageWriter::Format;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OExportImageDialog::OExportImageDialog(const QString &directory, const OExportPlanImage::Plan &plan,
                                       bool hasActive, QWidget *parent)
  : QDialog(parent),
    m_plan(plan)
{
    setModal(true);
    setMinimumWidth(520);

    auto *layout = new QGridLayout(this);

    m_pathLabel = new QLabel(this);
    m_pathEdit = new QLineEdit(directory, this);
    m_browseButton = new QPushButton(this);
    connect(m_browseButton, &QPushButton::clicked, this, &OExportImageDialog::OnBrowse);

    m_formatLabel = new QLabel(this);
    m_formatCombo = MakeComboBox(this);
    m_formatCombo->addItem(QString(), static_cast<int>(Format::Svg));
    m_formatCombo->addItem(QString(), static_cast<int>(Format::Png));
    m_formatCombo->setCurrentIndex(0);

    m_planLabel = new QLabel(this);
    m_planValue = new QLabel(this);

    m_axisLabel = new QLabel(this);
    m_axisCombo = MakeComboBox(this);
    m_axisCombo->addItem(QString(), static_cast<int>(OExportPlanImage::Axis::Width));
    m_axisCombo->addItem(QString(), static_cast<int>(OExportPlanImage::Axis::Height));
    m_axisCombo->setCurrentIndex(0);

    m_sizeLabel = new QLabel(this);
    m_sizeEdit = new QLineEdit(this);
    m_unitCombo = MakeComboBox(this);
    for (const Unit &unit : OExportUnitFormat::SelectableUnits())
    {
        m_unitCombo->addItem(QString(), static_cast<int>(unit));
    }
    // Centimetres, like the print tab: the unit a plan is spoken of in, and the one the status
    // label is read in.
    m_unit = Unit::Cm;
    m_unitCombo->setCurrentIndex(m_unitCombo->findData(static_cast<int>(m_unit)));

    m_otherLabel = new QLabel(this);
    m_otherValue = new QLabel(this);

    m_dpiLabel = new QLabel(this);
    m_dpiSpin = new QSpinBox(this);
    m_dpiSpin->setRange(kMinDpi, kMaxDpi);
    m_dpiSpin->setValue(kDefaultDpi);

    // The field opens on the plan's own size, so that the dialog starts at "no reduction" and the
    // user descends from the only figure that is not a guess. The side it measures is the one the
    // axis combo names.
    m_sizeMm = m_plan.widthMm;
    m_sizeEdit->setText(OExportUnitFormat::Format(m_sizeMm, m_unit));

    // Connected once the combos hold their items: adding the first item to an empty combo moves
    // its index, and a handler wired before that would run on a half-built dialog.
    connect(m_formatCombo, &QComboBox::currentIndexChanged, this, &OExportImageDialog::OnFormatChanged);
    connect(m_axisCombo, &QComboBox::currentIndexChanged, this, &OExportImageDialog::OnAxisChanged);
    connect(m_unitCombo, &QComboBox::currentIndexChanged, this, &OExportImageDialog::OnUnitChanged);
    // textEdited and not textChanged: only what the user types has to recompute the other side, so
    // the reformatting this dialog does on a unit change cannot come back as a keystroke.
    connect(m_sizeEdit, &QLineEdit::textEdited, this, &OExportImageDialog::OnSizeEdited);

    layout->addWidget(m_pathLabel, 0, 0);
    layout->addWidget(m_pathEdit, 0, 1);
    layout->addWidget(m_browseButton, 0, 2);
    layout->addWidget(m_formatLabel, 1, 0);
    layout->addWidget(m_formatCombo, 1, 1, 1, 2);
    layout->addWidget(m_axisLabel, 2, 0);
    layout->addWidget(m_axisCombo, 2, 1, 1, 2);
    layout->addWidget(m_planLabel, 3, 0);
    layout->addWidget(m_planValue, 3, 1, 1, 2);
    layout->addWidget(m_sizeLabel, 4, 0);
    layout->addWidget(m_sizeEdit, 4, 1);
    layout->addWidget(m_unitCombo, 4, 2);
    layout->addWidget(m_otherLabel, 5, 0);
    layout->addWidget(m_otherValue, 5, 1, 1, 2);
    layout->addWidget(m_dpiLabel, 6, 0);
    layout->addWidget(m_dpiSpin, 6, 1, 1, 2);

    m_buttonBox = new QDialogButtonBox(this);
    auto *activeButton =
        m_buttonBox->addButton(QString(), QDialogButtonBox::AcceptRole);
    activeButton->setAutoDefault(false);
    activeButton->setEnabled(hasActive);

    auto *listButton = m_buttonBox->addButton(QString(), QDialogButtonBox::ActionRole);
    listButton->setAutoDefault(false);

    m_buttonBox->addButton(QDialogButtonBox::Cancel);

    // Asserted last, once every button is in: the box lays its buttons out as they arrive and can
    // hand the default to the one added after the first — Return would then do something else than
    // what the dialog opened on.
    (hasActive ? activeButton : listButton)->setDefault(true);

    // The two ways out carry their own answer. A shared accepted() would have to guess which
    // button raised it, and a button is the one thing that knows.
    connect(activeButton, &QPushButton::clicked, this, [this]() { Choose(Scope::Active); });
    connect(listButton, &QPushButton::clicked, this, [this]() { Choose(Scope::All); });
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    layout->addWidget(m_buttonBox, 7, 0, 1, 3);

    Retranslate();
    m_sizeEdit->setFocus();
    m_sizeEdit->selectAll();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportImageDialog::Directory() const -> QString
{
    return m_pathEdit->text().trimmed();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportImageDialog::Format() const -> OExportPlanImageWriter::Format
{
    return static_cast<OExportPlanImageWriter::Format>(m_formatCombo->currentData().toInt());
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportImageDialog::Axis() const -> OExportPlanImage::Axis
{
    return static_cast<OExportPlanImage::Axis>(m_axisCombo->currentData().toInt());
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportImageDialog::TargetMm() const -> qreal
{
    return m_sizeMm;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportImageDialog::Dpi() const -> qreal
{
    return m_dpiSpin->value();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportImageDialog::Chosen() const -> OExportImageDialog::Scope
{
    return m_scope;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportImageDialog::RefreshDerived()
{
    const bool raster = (Format() == Format::Png);
    const QString unitText = UnitsToStr(m_unit, true);

    m_planValue->setText(tr("%1 × %2 %3").arg(OExportUnitFormat::Format(m_plan.widthMm, m_unit),
                                              OExportUnitFormat::Format(m_plan.heightMm, m_unit),
                                              unitText));

    // The field's label names the side it measures; the line below names the one that follows. One
    // typed length and one computed — never two fields to hold in agreement.
    const OExportPlanImage::Axis axis = Axis();
    const bool measuresWidth = (axis == OExportPlanImage::Axis::Width);
    m_sizeLabel->setText(measuresWidth ? tr("Width:") : tr("Height:"));
    m_otherLabel->setText(measuresWidth ? tr("Height:") : tr("Width:"));

    const qreal otherMm = OExportPlanImage::OtherSideMm(m_plan, m_sizeMm, axis);
    m_otherValue->setText(otherMm > 0.0
                              ? tr("%1 %2").arg(OExportUnitFormat::Format(otherMm, m_unit), unitText)
                              : QString());

    // An SVG's page IS a physical size and has no resolution to be asked for; a raster has only
    // pixels, and the figure is what gives them a size. Label and field go together — a lone field
    // would read as a setting left over from somewhere else.
    m_dpiLabel->setVisible(raster);
    m_dpiSpin->setVisible(raster);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportImageDialog::OnBrowse()
{
    const QString chosen = QFileDialog::getExistingDirectory(this, tr("Folder"), Directory());
    if (!chosen.isEmpty())
    {
        m_pathEdit->setText(chosen);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportImageDialog::OnFormatChanged()
{
    RefreshDerived();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportImageDialog::OnAxisChanged()
{
    // The figure that was typed stays: the combo says WHAT it measures, not how much — and
    // rewriting the field would throw away a decision the user has already made.
    RefreshDerived();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportImageDialog::OnUnitChanged()
{
    // The field is read in the unit it is WRITTEN in, which is the one being left — so it is
    // parsed first, and the unit changed after. The other order would read "19" as nineteen of the
    // new unit and turn centimetres into inches without a word.
    bool ok = false;
    const qreal typed = OExportUnitFormat::Parse(m_sizeEdit->text(), m_unit, &ok);
    if (ok)
    {
        m_sizeMm = typed;
    }

    m_unit = static_cast<Unit>(m_unitCombo->currentData().toInt());
    m_sizeEdit->setText(OExportUnitFormat::Format(m_sizeMm, m_unit));

    RefreshDerived();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportImageDialog::OnSizeEdited()
{
    bool ok = false;
    const qreal typed = OExportUnitFormat::Parse(m_sizeEdit->text(), m_unit, &ok);
    // Not a number is not zero: zero is a size the user could mean, and a text that is not a
    // number is not. Both leave the other side empty, and Choose refuses on the difference.
    m_sizeMm = ok ? typed : 0.0;

    RefreshDerived();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportImageDialog::Choose(Scope scope)
{
    const QFileInfo folder(Directory());
    if (!folder.isDir())
    {
        QMessageBox::warning(this, tr("Export"), tr("That folder does not exist."));
        m_pathEdit->setFocus();
        return;
    }

    if (m_sizeMm <= 0.0)
    {
        QMessageBox::warning(this, tr("Export"), tr("The size must be a number greater than zero."));
        m_sizeEdit->setFocus();
        m_sizeEdit->selectAll();
        return;
    }

    // The very Target the writer will be handed for the plan on show, computed by the very call it
    // will make: a size this dialog accepts is a size that can be written for the generation the
    // figure was read on. ⚠️ A generation of the batch is measured on its OWN plan, and one far
    // larger than this one can still be refused when its turn comes — the run then says so on
    // that line, which is the only place that knows which generation it was.
    const qreal dpi = (Format() == Format::Png) ? Dpi() : PrintDPI;
    const OExportPlanImage::Target target = OExportPlanImage::SizeFor(m_plan, m_sizeMm, Axis(), dpi);
    if (!target.IsValid())
    {
        QMessageBox::warning(this, tr("Export"), tr("The plan cannot be written at that size."));
        m_sizeEdit->setFocus();
        m_sizeEdit->selectAll();
        return;
    }

    // An SVG carries numbers; a PNG carries pixels, and Qt refuses an allocation it cannot make —
    // with a qWarning, which in Ostara opens a modal box of its own before we could say what
    // happened. Asking beforehand is what keeps that case unreachable.
    const qint64 pixels = static_cast<qint64>(target.sizePx.width()) * target.sizePx.height();
    if (Format() == Format::Png && pixels > OExportPlanImage::kMaxPixels)
    {
        QMessageBox::warning(this, tr("Export"),
                             tr("That image would be too large. Lower the size or the resolution."));
        m_sizeEdit->setFocus();
        m_sizeEdit->selectAll();
        return;
    }

    m_scope = scope;
    accept();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportImageDialog::Retranslate()
{
    setWindowTitle(tr("Export the plan as an image"));

    m_pathLabel->setText(tr("Folder:"));
    m_browseButton->setText(tr("Browse..."));

    m_formatLabel->setText(tr("Format:"));
    m_formatCombo->setItemText(0, QStringLiteral("SVG"));
    m_formatCombo->setItemText(1, QStringLiteral("PNG"));

    m_planLabel->setText(tr("Plan:"));

    m_axisLabel->setText(tr("Give:"));
    m_axisCombo->setItemText(0, tr("Width"));
    m_axisCombo->setItemText(1, tr("Height"));

    for (int i = 0; i < m_unitCombo->count(); ++i)
    {
        m_unitCombo->setItemText(i, UnitsToStr(static_cast<Unit>(m_unitCombo->itemData(i).toInt()), true));
    }

    m_dpiLabel->setText(tr("Resolution:"));
    m_dpiSpin->setSuffix(tr(" dpi"));

    // The two ways out, where their place in the bar already says which is which.
    const QList<QAbstractButton *> buttons = m_buttonBox->buttons();
    for (QAbstractButton *button : buttons)
    {
        switch (m_buttonBox->buttonRole(button))
        {
            case QDialogButtonBox::AcceptRole:
                button->setText(tr("Export the active output"));
                break;
            case QDialogButtonBox::ActionRole:
                button->setText(tr("Export several outputs..."));
                break;
            case QDialogButtonBox::RejectRole:
                button->setText(tr("Cancel"));
                break;
            default:
                break;
        }
    }

    RefreshDerived();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportImageDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QDialog::changeEvent(event);
}
