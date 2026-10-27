/************************************************************************
 **
 **  @file   owaitdialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/09/2026
 **
 **  @brief  Modal "please wait" dialog: the message, and — when asked — a bar and the seconds.
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

#include "owaitdialog.h"

#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>

namespace
{
/// How often the clock is rewritten. Ten times a second: often enough that a second never looks
/// stuck, seldom enough that the repaint costs nothing.
constexpr int kWaitTickMs = 100;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OWaitDialog::OWaitDialog(const QString &text, QWidget *parent, Presentation presentation)
  : QDialog(parent)
{
    setWindowModality(Qt::ApplicationModal);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);

    auto *label = new QLabel(text, this);
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);

    if (presentation == Presentation::Message)
    {
        return;
    }

    // **La barre tourne au lieu de se remplir.**
    //
    // Le moteur d'imbrication ne compte rien qu'il puisse annoncer : il ne sait ni combien de
    // placements il essaiera, ni combien de temps il lui faudra. Une barre remplie depuis un total
    // inventé serait un mensonge dit à la vitesse du run. Ce qui bouge, c'est l'animation de la
    // barre elle-même — et elle est honnête : quelque chose se passe.
    auto *bar = new QProgressBar(this);
    bar->setRange(0, 0);
    bar->setTextVisible(false);
    bar->setMinimumWidth(320);
    layout->addWidget(bar);

    // **Les secondes, et c'est le point.** Un run d'une demi-minute devant une fenêtre immobile est
    // une fenêtre morte ; un chiffre qui monte est le même run prouvé vivant. Il part de l'ouverture
    // de la fenêtre et non de la progression : ce que l'utilisatrice passe, c'est le temps devant la
    // fenêtre.
    //
    // ⚠️ Ce minuteur ne bat que pendant qu'une boucle d'événements tourne : c'est la boucle imbriquée
    // du run, là-bas, qui tient ce fil occupé à répondre — le travail, lui, est sur un fil
    // d'arrière-plan. Un appelant qui travaillerait sur CE fil laisserait le chiffre figé à 0,0 s.
    m_elapsed = new QLabel(this);
    m_elapsed->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_elapsed);

    // ⚠️ La conversion est écrite : le projet compile en `-Werror`, et un `qint64` divisé par un
    // `double` est une conversion implicite entière → flottante, que le compilateur refuse.
    const auto writeElapsed = [this]
    {
        m_elapsed->setText(tr("%1 s").arg(static_cast<double>(m_clock.elapsed()) / 1000.0, 0, 'f', 1));
    };

    // Started before it is first read: a QElapsedTimer that was never started has no elapsed time,
    // and the first paint would show whatever the invalid state reports.
    m_clock.start();
    writeElapsed();

    auto *tick = new QTimer(this);
    tick->setInterval(kWaitTickMs);
    connect(tick, &QTimer::timeout, this, writeElapsed);
    tick->start();
}
