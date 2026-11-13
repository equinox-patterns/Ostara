/************************************************************************
 **
 **  @file   ofullscreenkeeper.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/11/2026
 **
 **  @brief  Le filet qui rend le plein écran du système quand une dialogue l'a fait perdre.
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

#ifndef OFULLSCREENKEEPER_H
#define OFULLSCREENKEEPER_H

#include <QObject>

class QWidget;

/**
 * @brief Ce qui rend le plein écran à la fenêtre principale après une dialogue.
 *
 * Le plein écran de l'application (`ofullscreen.h`) ne peut jamais lui être repris. Celui du système
 * le peut : sous X11 comme sous Wayland, plusieurs gestionnaires de fenêtres retirent le plein écran
 * d'une fenêtre dès qu'une de ses dialogues s'affiche — c'est écrit dans le compositeur, pas dans Qt,
 * et aucune ligne de l'application ne peut l'empêcher.
 *
 * Ce filet ne se bat pas : il constate, et il rend. Quand une dialogue de premier niveau est visible
 * alors que la fenêtre principale était en plein écran — ou quand le plein écran tombe pendant qu'une
 * dialogue est ouverte — il retient le fait, et à la fermeture de la dernière dialogue il remet la
 * fenêtre en plein écran, une fois, sans insister.
 *
 * À la fermeture seulement, jamais pendant : rétablir en cours de route reviendrait à se battre avec
 * le compositeur à chaque événement, et c'est cette course qui fait clignoter les fenêtres.
 *
 * Installé une seule fois par `VApplication`, à côté du cadrage des dialogues. Un filtre
 * d'événements sur `qApp` est le seul endroit qui voit toutes les dialogues — il y a près de cent
 * trente points d'appel à `exec()` dans le code, et aucun ne passe par une fabrique commune.
 */
class OFullScreenKeeper final : public QObject
{
    Q_OBJECT // NOLINT

public:
    /// @brief Installe le filet sur l'application. Sans effet si déjà installé.
    static void Install();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    explicit OFullScreenKeeper(QObject *parent);

    /// @brief La fenêtre à rendre au plein écran, nulle tant que l'application n'en a pas.
    static auto MainWindow() -> QWidget *;

    /// @brief La fenêtre de dialogue que @a watched désigne, s'il en est une de premier niveau.
    ///
    /// Une `QDialog`, et aussi une fenêtre qui porte le **type** dialogue sans en être une : les
    /// éditeurs de cartouche et de mise en page PDF sont des `QMainWindow` que Linux présente comme
    /// des dialogues (voir `OInfoBlockWindow`, `OExportTemplateWindow`). Ce sont des dialogues pour
    /// l'utilisatrice, ils doivent compter comme tels pour le filet.
    static auto TopLevelDialog(QObject *watched) -> QWidget *;

    /// @brief Vrai s'il reste une dialogue visible autre que @a closing.
    static auto AnotherDialogVisible(const QWidget *closing) -> bool;

    /// Vrai quand l'écran doit être rendu à la fermeture de la dernière dialogue : la fenêtre
    /// principale était en plein écran quand une dialogue s'est ouverte, ou le plein écran est tombé
    /// pendant qu'une dialogue était ouverte. Les deux origines mènent au même geste, et rien ne les
    /// distingue ensuite — un seul drapeau suffit, consommé à la première occasion.
    bool m_screenToGiveBack{false};

    /// Empêche le rétablissement de se rappeler lui-même : il change l'état de la fenêtre.
    bool m_restoring{false};
};

#endif // OFULLSCREENKEEPER_H
