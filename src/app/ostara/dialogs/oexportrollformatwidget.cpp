/************************************************************************
 **
 **  @file   oexportrollformatwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  Implementation of the Roll shape.
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

#include "oexportrollformatwidget.h"

#include "oexportrowiconbutton.h"

#include "../core/oexportrollpages.h"
#include "../core/oexportunitformat.h"

#include <QCheckBox>
#include <QEvent>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QStringList>

#include <algorithm>

namespace
{
/// Une largeur de rouleau qui n'aurait pas de sens : le mode sait nidifier plus large, mais aucun
/// rouleau de machine ne l'est, et une valeur aberrante ne serait découverte qu'au nesting.
constexpr qreal kMinWidthMm = 100.0;
constexpr qreal kMaxWidthMm = 2000.0;

/// Une page plus courte que cela ne porte aucune pièce : la valeur est un garde-fou de saisie, pas
/// une contrainte du moteur.
constexpr qreal kMinPageLengthMm = 100.0;

/// Les largeurs de rouleaux les plus courantes en impression, dans l'ordre de la largeur.
///
/// Les valeurs sont celles SOUS LESQUELLES les rouleaux sont vendus, pas les conversions exactes :
/// un 36 pouces fait 914,4 mm, il se vend en 914. C'est le chiffre que l'utilisatrice trouvera chez
/// son imprimeur, et c'est celui-là qu'elle doit pouvoir retaper.
///
/// Sources vérifiées le 10-11-2026 : le tableau des rouleaux du Canon imagePROGRAF PRO-6000
/// (841, 914,4, 1066,8, 1117,6, 1524 mm) et le guide ArQuipu sur les largeurs de rouleaux
/// (24, 36 et 44 pouces les plus courantes ; 17, 42 et 60 existent aussi).
struct RollWidth
{
    const char *label;
    qreal mm;
};

const QVector<RollWidth> kRollWidths{
    {"17 in", 432.0}, {"A1", 594.0}, {"24 in", 610.0}, {"A0", 841.0},
    {"36 in", 914.0}, {"42 in", 1067.0}, {"44 in", 1118.0}, {"60 in", 1524.0},
};
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OExportRollFormatWidget::OExportRollFormatWidget(QWidget *parent)
  : QWidget(parent)
{
    auto *layout = new QFormLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // ── La largeur, et ce qu'elle peut être ──────────────────────────────────────────────────────
    //
    // Un champ, et non le libellé encadré d'hier : la largeur d'un rouleau n'est pas une norme, elle
    // dépend du rouleau CHARGÉ. L'icône d'info est à côté du champ, pas dans une ligne à part : elle
    // répond à la seule question que la saisie pose — « quelles largeurs existent ? ».
    auto *widthRow = new QWidget(this);
    auto *widthLayout = new QHBoxLayout(widthRow);
    widthLayout->setContentsMargins(0, 0, 0, 0);

    m_widthEdit = new QLineEdit(widthRow);
    connect(m_widthEdit, &QLineEdit::editingFinished, this, &OExportRollFormatWidget::CommitWidth);

    m_infoButton = new OExportRowIconButton(QStringLiteral("info"), widthRow);
    connect(m_infoButton, &QPushButton::clicked, this, &OExportRollFormatWidget::ShowRollWidths);

    widthLayout->addWidget(m_widthEdit, 1);
    widthLayout->addWidget(m_infoButton);

    m_widthLabel = new QLabel(this);
    layout->addRow(m_widthLabel, widthRow);

    // ── La longueur maximale d'une page ──────────────────────────────────────────────────────────
    //
    // Vide = 500 cm, le plafond du PDF. La valeur plus petite est la limite de la machine à plier de
    // l'imprimante : aucune n'est universelle, donc c'est à l'utilisatrice de la donner.
    m_maxLengthLabel = new QLabel(this);
    m_maxLengthEdit = new QLineEdit(this);
    connect(m_maxLengthEdit, &QLineEdit::editingFinished, this,
            &OExportRollFormatWidget::CommitMaxLength);
    layout->addRow(m_maxLengthLabel, m_maxLengthEdit);

    // ── L'export hors marges droite/gauche ───────────────────────────────────────────────────────
    //
    // La case est seule sur sa ligne, et ce qu'elle change est écrit dessous : la largeur exportée
    // n'est nulle part ailleurs, et c'est le chiffre que l'imprimante facture. La précision « haut et
    // bas conservés » est sur la même ligne, en second — c'est la question suivante que la case pose.
    m_trimCheck = new QCheckBox(this);
    connect(m_trimCheck, &QCheckBox::toggled, this, [this](bool)
    {
        Refresh();
        emit Changed();
    });
    layout->addRow(m_trimCheck);

    m_trimHint = new QLabel(this);
    layout->addRow(m_trimHint);

    Retranslate();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRollFormatWidget::SetWidthMm(qreal widthMm)
{
    m_widthMm = widthMm;
    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRollFormatWidget::SetMarginMm(qreal marginMm)
{
    m_marginMm = marginMm;
    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRollFormatWidget::SetUnit(const Unit &unit)
{
    m_unit = unit;
    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportRollFormatWidget::WidthMm() const -> qreal
{
    return m_widthMm;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportRollFormatWidget::MaxPageLengthMm() const -> qreal
{
    return m_maxPageLengthMm;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportRollFormatWidget::TrimSideMargins() const -> bool
{
    return m_trimCheck->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRollFormatWidget::CommitWidth()
{
    bool ok = false;
    const qreal mm = OExportUnitFormat::Parse(m_widthEdit->text(), m_unit, &ok);

    // Une saisie illisible ne change rien : le champ est réécrit avec la longueur gardée, comme les
    // deux champs de marge de l'onglet. Une largeur hors bornes est ignorée de la même façon — une
    // valeur corrigée en silence serait une valeur qu'on croit avoir saisie.
    if (ok && mm >= kMinWidthMm && mm <= kMaxWidthMm)
    {
        m_widthMm = mm;
    }

    Refresh();
    emit Changed();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRollFormatWidget::CommitMaxLength()
{
    const QString text = m_maxLengthEdit->text().trimmed();

    if (text.isEmpty())
    {
        m_maxPageLengthMm = 0.0; // 500 cm, le plafond du PDF
    }
    else
    {
        bool ok = false;
        const qreal mm = OExportUnitFormat::Parse(text, m_unit, &ok);
        if (ok && mm > 0.0)
        {
            m_maxPageLengthMm = std::clamp(mm, kMinPageLengthMm, OExportRollPages::PdfPageMaxMm);
        }
        else if (ok)
        {
            m_maxPageLengthMm = 0.0; // zéro est la même réponse que vide
        }
    }

    Refresh();
    emit Changed();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRollFormatWidget::Refresh()
{
    m_widthEdit->setText(OExportUnitFormat::Format(m_widthMm, m_unit));

    m_maxLengthEdit->setText(m_maxPageLengthMm > 0.0
                                 ? OExportUnitFormat::Format(m_maxPageLengthMm, m_unit)
                                 : QString());
    // **Le placeholder dit le vide, dans l'unité du moment.** 500 cm, 5000 mm, 196.85 in : une
    // valeur fixe en centimètres mentirait dès que l'onglet passe en pouces.
    m_maxLengthEdit->setPlaceholderText(OExportUnitFormat::Format(OExportRollPages::PdfPageMaxMm, m_unit));

    const QString unitText = UnitsToStr(m_unit, true);
    const QString pdfMax = OExportUnitFormat::Format(OExportRollPages::PdfPageMaxMm, m_unit);

    m_infoButton->setToolTip(RollWidthsBody());
    m_maxLengthEdit->setToolTip(
        tr("Empty = %1 %2, the biggest page a PDF can hold. A smaller value is the limit of your "
           "printer's folding machine.")
            .arg(pdfMax, unitText));

    // Le repère n'est pas une seconde réponse à « Document width: » : celle-là dit la largeur du
    // PAPIER, celui-ci la largeur du FICHIER, et c'est celle que l'imprimante facture.
    const bool trim = m_trimCheck->isChecked();
    m_trimHint->setVisible(trim);
    if (trim)
    {
        const qreal exported = std::max<qreal>(0.0, m_widthMm - 2.0 * m_marginMm);
        m_trimHint->setText(
            tr("Exported width: %1 %2 — left and right only.\nThe top and bottom margins are kept.")
                .arg(OExportUnitFormat::Format(exported, m_unit), unitText));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRollFormatWidget::Retranslate()
{
    m_widthLabel->setText(tr("Width:"));
    m_maxLengthLabel->setText(tr("Max page length:"));
    m_trimCheck->setText(tr("Export the PDF without side margins"));
    m_trimCheck->setToolTip(
        tr("Print at 100% and, if your printer offers it, center the page on the roll. Otherwise the "
           "5 mm are not guaranteed on both sides."));

    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportRollFormatWidget::RollWidthsBody() const -> QString
{
    // Une seule liste, deux lectures : le tooltip et la dialogue. Deux listes écrites côte à côte
    // finiraient par ne plus dire la même chose.
    QStringList lines;
    lines << tr("The most common roll widths in printing:");
    lines << QString();

    for (const RollWidth &width : kRollWidths)
    {
        lines << QStringLiteral("%1 — %2 mm (%3 %4)")
                     .arg(QLatin1String(width.label), QString::number(width.mm, 'f', 0),
                          OExportUnitFormat::Format(width.mm, Unit::Cm), UnitsToStr(Unit::Cm, true));
    }

    return lines.join(QLatin1Char('\n'));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRollFormatWidget::ShowRollWidths()
{
    QMessageBox::information(this, tr("Standard printing roll widths"), RollWidthsBody());
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRollFormatWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QWidget::changeEvent(event);
}
