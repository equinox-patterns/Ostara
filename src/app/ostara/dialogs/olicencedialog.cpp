/************************************************************************
 **
 **  @file   olicencedialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/10/2026
 **
 **  @brief  Les textes de licence des composants, entiers.
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

#include "olicencedialog.h"

#include <QFile>
#include <QFrame>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollBar>
#include <QTextBrowser>
#include <QVBoxLayout>

namespace
{
/// Un texte de licence, tel quel. Le nom du fichier est aussi le nom du titre : une seule liste,
/// donc un seul endroit où en ajouter un.
///
/// **Des `QString`, pas des `const char *`.** Le titre porte un tiret cadratin, et lire des octets
/// UTF-8 comme du Latin-1 le transforme en « â€ » : la conversion est donc faite par le compilateur,
/// une fois, et plus jamais à l'exécution.
struct Section
{
    QString title;
    QString file;
};

/// Le texte d'une ressource, ou une phrase qui dit qu'elle manque — jamais du vide, qui se
/// lirait comme un composant sans licence.
auto Text(const QString &name) -> QString
{
    QFile file(QStringLiteral(":/licenses/%1").arg(name));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return OLicenceDialog::tr("The licence text is missing from this build.");
    }

    return QString::fromUtf8(file.readAll());
}

/// Un bloc de texte brut, replié : les licences ont des colonnes, et une colonne écrasée ne se lit
/// plus.
auto Block(const QString &text) -> QString
{
    return QStringLiteral("<p style=\"white-space:pre-wrap;\">%1</p>").arg(text.toHtmlEscaped());
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OLicenceDialog::OLicenceDialog(QWidget *parent)
  : QDialog(parent)
{
    setWindowTitle(tr("Licence texts"));
    setModal(true);
    resize(680, 640);
    setMinimumSize(520, 420);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 16);
    layout->setSpacing(8);

    m_body = new QTextBrowser(this);
    m_body->setOpenExternalLinks(true);
    m_body->setFrameShape(QFrame::NoFrame);
    layout->addWidget(m_body, 1);

    auto *buttons = new QHBoxLayout;
    buttons->setContentsMargins(0, 8, 0, 0);
    buttons->addStretch();

    auto *close = new QPushButton(tr("Close"), this);
    close->setCursor(Qt::PointingHandCursor);
    close->setDefault(true);
    buttons->addWidget(close);

    layout->addLayout(buttons);

    connect(close, &QPushButton::clicked, this, &QDialog::accept);

    // Le programme d'abord (GPLv3 §4), les composants ensuite. Chaque entrée porte le nom de sa
    // licence : c'est ce qu'un lecteur cherche, pas un numéro de paragraphe.
    const Section sections[] = {
        {QStringLiteral("Ostara — GNU General Public License 3"), QStringLiteral("GPL-3.0.txt")},
        {QStringLiteral("Phosphor Icons — MIT"), QStringLiteral("phosphor-MIT.txt")},
        {QStringLiteral("Inter — SIL Open Font License 1.1"), QStringLiteral("inter-OFL.txt")},
        {QStringLiteral("Clipper2 — Boost Software License 1.0"),
         QStringLiteral("clipper2-BSL-1.0.txt")},
        {QStringLiteral("Xerces-C++ — Apache License 2.0"),
         QStringLiteral("xerces-Apache-2.0.txt")},
        {QStringLiteral("Xerces-C++ — NOTICE"), QStringLiteral("xerces-NOTICE.txt")},
        {QStringLiteral("libdxfrw — GNU General Public License 2"), QStringLiteral("GPL-2.0.txt")},
        {QStringLiteral("muParser — MIT"), QStringLiteral("muparser-MIT.txt")},
        {QStringLiteral("zlib — zlib License"), QStringLiteral("zlib.txt")},
        {QStringLiteral("graaflib — MIT"), QStringLiteral("graaflib-MIT.txt")},
        {QStringLiteral("VPropertyExplorer — GNU Lesser General Public License 3"),
         QStringLiteral("LGPL-3.0.txt")},
        {QStringLiteral("Trame — GNU General Public License 3"), QStringLiteral("GPL-3.0.txt")},
        {QStringLiteral("Qt — GNU Lesser General Public License 3"),
         QStringLiteral("LGPL-3.0.txt")},
    };

    // Un filet entre deux textes : sans lui, un titre d'ouverture se lit comme la dernière ligne du
    // texte précédent, et une section peut passer inaperçue au défilement.
    QString html;
    for (const Section &section : sections)
    {
        if (!html.isEmpty())
        {
            html += QStringLiteral("<hr>");
        }

        html += QStringLiteral("<h2>%1</h2>%2").arg(section.title, Block(Text(section.file)));
    }

    m_body->setHtml(html);
    m_body->verticalScrollBar()->setValue(0);
}
