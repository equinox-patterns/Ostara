/************************************************************************
 **
 **  @file   oaboutdialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/10/2026
 **
 **  @brief  What Ostara is, who wrote it, what it owes, and the versions
 **          it is made of.
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

#include "oaboutdialog.h"

#include "olicencedialog.h"
#include "../vmisc/projectversion.h"

#include <QApplication>
#include <QClipboard>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollBar>
#include <QTextBrowser>
#include <QVBoxLayout>

namespace
{
/// Un texte brut, replié — les lignes techniques, et rien d'autre.
auto Block(const QString &text) -> QString
{
    return QStringLiteral("<p style=\"white-space:pre-wrap;\">%1</p>").arg(text.toHtmlEscaped());
}

/// Une ligne de la liste des composants : ce que c'est, puis à qui et sous quelle licence.
auto Line(const QString &label, const QString &value) -> QString
{
    return QStringLiteral("<p><b>%1</b> %2</p>").arg(label.toHtmlEscaped(), value.toHtmlEscaped());
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OAboutDialog::OAboutDialog(QWidget *parent)
  : QDialog(parent)
{
    setWindowTitle(tr("About Ostara"));
    setModal(true);
    resize(640, 600);
    setMinimumSize(540, 460);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(28, 24, 28, 20);
    layout->setSpacing(6);

    // Le mot seul, à la manière de la page d'accueil : Ostara a une marque, mais une marque dans un
    // dialogue d'informations est une décoration, et l'icône de l'application est créditée plus bas.
    m_wordmark = new QLabel(QStringLiteral("Ostara"), this);
    m_wordmark->setObjectName(QStringLiteral("aboutWordmark"));
    m_wordmark->setAlignment(Qt::AlignHCenter);
    layout->addWidget(m_wordmark);

    m_version = new QLabel(this);
    m_version->setObjectName(QStringLiteral("aboutVersion"));
    m_version->setAlignment(Qt::AlignHCenter);
    layout->addWidget(m_version);

    // Le copyright du logiciel, tel qu'il est : un nom et une année qui ne se traduisent pas.
    m_copyright = new QLabel(QStringLiteral("© 2026 Sophie Denys"), this);
    m_copyright->setAlignment(Qt::AlignHCenter);
    layout->addWidget(m_copyright);

    m_legal = new QLabel(this);
    m_legal->setObjectName(QStringLiteral("aboutLegal"));
    m_legal->setAlignment(Qt::AlignHCenter);
    m_legal->setWordWrap(true);
    layout->addWidget(m_legal);
    layout->addSpacing(12);

    m_body = new QTextBrowser(this);
    m_body->setObjectName(QStringLiteral("aboutBody"));
    m_body->setOpenExternalLinks(true);
    m_body->setFrameShape(QFrame::NoFrame);
    layout->addWidget(m_body, 1);

    auto *buttons = new QHBoxLayout;
    buttons->setContentsMargins(0, 8, 0, 0);

    m_copy = new QPushButton(tr("Copy the technical details"), this);
    m_copy->setCursor(Qt::PointingHandCursor);
    buttons->addWidget(m_copy);

    // Les textes obligatoires ne sont pas dans cette fiche : ils ont leur fenêtre, à un clic.
    m_licences = new QPushButton(tr("Licence texts"), this);
    m_licences->setCursor(Qt::PointingHandCursor);
    buttons->addWidget(m_licences);

    buttons->addStretch();

    m_close = new QPushButton(tr("Close"), this);
    m_close->setCursor(Qt::PointingHandCursor);
    m_close->setDefault(true);
    buttons->addWidget(m_close);

    layout->addLayout(buttons);

    connect(m_copy, &QPushButton::clicked, this,
            []() { QApplication::clipboard()->setText(TechnicalText()); });
    connect(m_licences, &QPushButton::clicked, this,
            [this]()
            {
                OLicenceDialog dialog(this);
                dialog.exec();
            });
    connect(m_close, &QPushButton::clicked, this, &QDialog::accept);

    Fill();
}

//---------------------------------------------------------------------------------------------------------------------
auto OAboutDialog::TechnicalText() -> QString
{
    // Ce qui reste utile quand quelque chose ne va pas : la version d'Ostara, et le Qt qui
    // l'exécute. Ni la révision, ni la date de compilation, ni le système — ils n'apprennent rien
    // qui ne se voie déjà sur la machine.
    return tr("Ostara %1").arg(AppDisplayVersion()) + QLatin1Char('\n')
           + tr("Qt: built with %1, running %2")
                 .arg(QString::fromLatin1(QT_VERSION_STR), QString::fromLatin1(qVersion()));
}

//---------------------------------------------------------------------------------------------------------------------
void OAboutDialog::Fill()
{
    m_version->setText(tr("Version %1").arg(AppDisplayVersion()));
    m_legal->setText(tr("A free program, published under the GNU General Public License, version 3 "
                        "or later. It comes with no warranty of any kind."));

    // L'origine en tête, et nommée : un fork garde les avis de copyright du projet dont il vient, et
    // dit qu'il l'a modifié. Le reste de la fiche n'aurait aucun sens sans cette phrase.
    const QString origin =
        QStringLiteral("<h3>%1</h3><p>%2</p>")
            .arg(tr("Origin"),
                 tr("Ostara is a fork of Valentina, © 2013-2026 the Valentina project — Roman "
                    "Telezhynskyi and its contributors — published under the GNU General Public "
                    "License, version 3. Ostara © 2026 Sophie Denys, under the same licence."));

    // Une ligne par composant : ce qu'il fait, qui l'a écrit, sous quelle licence. Les icônes sont
    // détaillées parce qu'elles viennent de deux endroits différents, et que les confondre serait
    // attribuer à l'un le travail de l'autre. L'icône de l'application n'y est pas : elle est de
    // Sophie, comme le reste du programme, et le copyright trois lignes plus haut le dit déjà.
    const QString components =
        QStringLiteral("<h3>%1</h3>").arg(tr("Components"))
        + Line(tr("Interface icons:"), tr("Phosphor Icons — Tobias Fried and Helena Zhang, MIT"))
        + Line(tr("Tool icons:"), tr("Valentina project — Roman Telezhynskyi and contributors, "
                                     "HiDPI redraw by Benjamin Nauck, 2016"))
        + Line(tr("Typeface:"), tr("Inter — Rasmus Andersson (The Inter Project Authors), "
                                   "SIL Open Font License 1.1"))
        + Line(tr("Nesting:"), tr("Clipper2 — Angus Johnson, Boost Software License 1.0"))
        + Line(tr("Nesting engine:"), tr("Trame — Sophie Denys, GNU GPL v3"))
        + Line(tr("Graph algorithms:"), tr("graaflib — Bob Luppes, MIT"))
        + Line(tr("Property editor:"), tr("VPropertyExplorer — the Valentina project, LGPLv3"))
        + Line(tr("XML:"), tr("Xerces-C++ — The Apache Software Foundation, Apache License 2.0"))
        + Line(tr("DXF:"), tr("libdxfrw — Rallaz and contributors, GNU GPL v2 or later"))
        + Line(tr("Expression parser:"), tr("muParser — Ingo Berg, MIT"))
        + Line(tr("Compression:"), tr("zlib — Jean-loup Gailly and Mark Adler, zlib licence"))
        + Line(tr("Framework:"),
               tr("Qt %1 — The Qt Company, LGPLv3").arg(QString::fromLatin1(QT_VERSION_STR)));

    const QString details = QStringLiteral("<h3>%1</h3>%2")
                                .arg(tr("Technical details"), Block(TechnicalText()));

    m_body->setHtml(origin + components + details);
    m_body->verticalScrollBar()->setValue(0);
}
