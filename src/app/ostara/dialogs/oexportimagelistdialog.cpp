/************************************************************************
 **
 **  @file   oexportimagelistdialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   23/09/2026
 **
 **  @brief  Implementation of the image export's second step.
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

#include "oexportimagelistdialog.h"

#include <QAbstractButton>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace
{
constexpr auto kSvgSuffix = ".svg";
constexpr auto kPngSuffix = ".png";
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OExportImageListDialog::OExportImageListDialog(const QVector<Row> &rows,
                                               OExportPlanImageWriter::Format format, QWidget *parent)
  : QDialog(parent),
    m_rows(rows),
    m_format(format)
{
    setModal(true);
    setMinimumWidth(560);

    auto *layout = new QVBoxLayout(this);

    for (int row = 0; row < m_rows.size(); ++row)
    {
        auto *line = new QHBoxLayout();

        // A real checkbox, and not an icon painted somewhere: the stylesheet already draws
        // QCheckBox::indicator with the application's own icons, and a widget that IS a checkbox
        // answers the keyboard and says what it is without a line of code here.
        auto *tick = new QCheckBox(this);
        // All of them, because a batch is asked for when most of them are wanted: unticking two
        // is less work than ticking four, and the list is read before anything is written.
        tick->setChecked(true);

        // A field, and one at its own size — a table would give it the height of a cell.
        auto *name = new QLineEdit(m_rows.at(row).name + Suffix(), this);

        line->addWidget(tick);
        line->addWidget(name, 1);

        m_ticks.append(tick);
        m_names.append(name);
        layout->addLayout(line);
    }

    m_buttonBox = new QDialogButtonBox(this);
    auto *exportButton = m_buttonBox->addButton(QString(), QDialogButtonBox::AcceptRole);
    exportButton->setAutoDefault(false);
    m_buttonBox->addButton(QDialogButtonBox::Cancel);
    // Asserted last, once every button is in: the box lays its buttons out as they arrive and can
    // hand the default to the one added after Export — Return would then cancel the export the
    // user just described.
    exportButton->setDefault(true);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &OExportImageListDialog::OnAccept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    layout->addWidget(m_buttonBox);

    Retranslate();

    // On the first name, since the names are the only thing here that is meant to be touched.
    if (!m_names.isEmpty())
    {
        m_names.constFirst()->setFocus();
        m_names.constFirst()->selectAll();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportImageListDialog::Suffix() const -> QString
{
    return QLatin1String((m_format == OExportPlanImageWriter::Format::Png) ? kPngSuffix : kSvgSuffix);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportImageListDialog::NameOf(int row) const -> QString
{
    if (row < 0 || row >= m_names.size() || m_names.at(row) == nullptr)
    {
        return QString();
    }

    QString name = m_names.at(row)->text().trimmed();
    if (name.isEmpty())
    {
        return name;
    }

    // The suffix follows the format, wherever the typing left it: a delivery named "plan.png"
    // that holds an SVG is a name lying about its own content.
    if (!name.endsWith(Suffix(), Qt::CaseInsensitive))
    {
        name += Suffix();
    }

    return name;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportImageListDialog::Chosen() const -> QVector<Row>
{
    QVector<Row> chosen;

    for (int row = 0; row < m_rows.size(); ++row)
    {
        const QCheckBox *tick = row < m_ticks.size() ? m_ticks.at(row) : nullptr;
        if (tick == nullptr || !tick->isChecked())
        {
            continue;
        }

        chosen.append({m_rows.at(row).id, NameOf(row)});
    }

    return chosen;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportImageListDialog::OnAccept()
{
    const QVector<Row> chosen = Chosen();
    if (chosen.isEmpty())
    {
        QMessageBox::warning(this, tr("Export"), tr("No generation is ticked."));
        return;
    }

    for (const Row &row : chosen)
    {
        if (row.name.isEmpty())
        {
            QMessageBox::warning(this, tr("Export"), tr("Every ticked generation needs a name."));
            return;
        }
    }

    accept();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportImageListDialog::Retranslate()
{
    setWindowTitle(tr("Export the plan as an image"));

    const QList<QAbstractButton *> buttons = m_buttonBox->buttons();
    for (QAbstractButton *button : buttons)
    {
        switch (m_buttonBox->buttonRole(button))
        {
            case QDialogButtonBox::AcceptRole:
                button->setText(tr("Export"));
                break;
            case QDialogButtonBox::RejectRole:
                button->setText(tr("Cancel"));
                break;
            default:
                break;
        }
    }

    // No header, and nothing to say about the boxes and the fields: one says what it is, the
    // other one is a name waiting to be typed. A table's header row would teach neither.
}

//---------------------------------------------------------------------------------------------------------------------
void OExportImageListDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QDialog::changeEvent(event);
}
