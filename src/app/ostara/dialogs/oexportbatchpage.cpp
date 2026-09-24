/************************************************************************
 **
 **  @file   oexportbatchpage.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Implementation d'une page de l'export en lot.
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

#include "oexportbatchpage.h"

#include <QCheckBox>
#include <QEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------
OExportBatchPage::OExportBatchPage(const QVector<Row> &rows, const QString &suffix,
                                   const QString &directory, bool withLayoutLayer, QWidget *parent)
  : QWidget(parent),
    m_rows(rows),
    m_suffix(suffix)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    // ── Le dossier ──
    // Un champ par page, et pas un pour tout le dialogue : une catégorie est un format, et les
    // fichiers d'un format vont ensemble. C'est ce qui permet d'écrire les PDF ici et les SVG
    // ailleurs, dans la même session, sans re-taper quoi que ce soit.
    auto *folderLine = new QHBoxLayout();
    m_pathLabel = new QLabel(this);
    m_pathEdit = new QLineEdit(directory, this);
    m_browseButton = new QPushButton(this);
    connect(m_browseButton, &QPushButton::clicked, this, &OExportBatchPage::OnBrowse);
    folderLine->addWidget(m_pathLabel);
    folderLine->addWidget(m_pathEdit, 1);
    folderLine->addWidget(m_browseButton);
    layout->addLayout(folderLine);

    // ── L'option, juste sous le chemin ──
    if (withLayoutLayer)
    {
        m_layoutLayerCheck = new QCheckBox(this);
        // OFF, comme dans l'export PDF d'une seule génération, et pour la même raison : un plan dont
        // le décor est un calque peut s'imprimer sans son cartouche et paraître normal. Le défaut sûr
        // est celui qui n'enlève rien, et la case se demande.
        m_layoutLayerCheck->setChecked(false);
        // Elle vaut pour TOUT le lot : un décor est une propriété du document écrit, pas d'une ligne.
        layout->addWidget(m_layoutLayerCheck);
    }

    // ── Les générations ──
    if (m_rows.isEmpty())
    {
        m_emptyLabel = new QLabel(this);
        m_emptyLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(m_emptyLabel);
    }
    else
    {
        for (int row = 0; row < m_rows.size(); ++row)
        {
            auto *line = new QHBoxLayout();

            // Une vraie case à cocher, et non une icône peinte : la QSS dessine déjà
            // QCheckBox::indicator avec les icônes de l'application, et un widget qui EST une case
            // répond au clavier et dit ce qu'il est sans une ligne de code de plus.
            auto *tick = new QCheckBox(this);
            // Toutes cochées : on demande un lot quand on en veut la plupart, et décocher deux
            // lignes coûte moins que d'en cocher quatre. La liste se lit avant que rien ne s'écrive.
            tick->setChecked(true);

            // Un champ, et un champ à sa taille — une table lui donnerait la hauteur d'une cellule.
            auto *name = new QLineEdit(m_rows.at(row).name + m_suffix, this);

            line->addWidget(tick);
            line->addWidget(name, 1);

            m_ticks.append(tick);
            m_names.append(name);
            layout->addLayout(line);
        }
    }

    // Les lignes restent en haut de la page ; l'onglet peut être plus haut qu'elles.
    layout->addStretch();

    Retranslate();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportBatchPage::Directory() const -> QString
{
    return m_pathEdit != nullptr ? m_pathEdit->text().trimmed() : QString();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportBatchPage::NameOf(int row) const -> QString
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

    // L'extension suit le format, d'où que la frappe l'ait laissée : un fichier livré sous un nom
    // qui annonce l'autre format est un nom qui ment sur son contenu.
    if (!name.endsWith(m_suffix, Qt::CaseInsensitive))
    {
        name += m_suffix;
    }

    return name;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportBatchPage::Chosen() const -> QVector<Row>
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
auto OExportBatchPage::AddLayoutLayer() const -> bool
{
    return m_layoutLayerCheck != nullptr && m_layoutLayerCheck->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportBatchPage::HasRows() const -> bool
{
    return !m_rows.isEmpty();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportBatchPage::FocusDirectory()
{
    if (m_pathEdit != nullptr)
    {
        m_pathEdit->setFocus();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportBatchPage::FocusFirstName()
{
    if (!m_names.isEmpty() && m_names.constFirst() != nullptr)
    {
        m_names.constFirst()->setFocus();
        m_names.constFirst()->selectAll();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportBatchPage::OnBrowse()
{
    const QString chosen = QFileDialog::getExistingDirectory(this, tr("Folder"), Directory());
    if (!chosen.isEmpty())
    {
        m_pathEdit->setText(chosen);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportBatchPage::Retranslate()
{
    if (m_pathLabel != nullptr)
    {
        m_pathLabel->setText(tr("Folder:"));
    }

    if (m_browseButton != nullptr)
    {
        m_browseButton->setText(tr("Browse..."));
    }

    if (m_layoutLayerCheck != nullptr)
    {
        // Le libellé de l'export PDF d'une seule génération, au mot près — deux orthographes de la
        // même case seraient deux traductions à tenir.
        m_layoutLayerCheck->setText(tr("Add Layout layer"));
    }

    if (m_emptyLabel != nullptr)
    {
        m_emptyLabel->setText(tr("There is no output to export here."));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportBatchPage::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QWidget::changeEvent(event);
}
