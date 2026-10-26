/************************************************************************
 **
 **  @file   oaboutdialog.h
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

#ifndef OABOUTDIALOG_H
#define OABOUTDIALOG_H

#include <QDialog>
#include <QString>

class QLabel;
class QPushButton;
class QTextBrowser;

/// « À propos d'Ostara » — le seul endroit où l'application dit de quoi elle est faite.
///
/// **Deux choses y sont dites et ne peuvent pas être omises.** Ostara est un fork de Valentina :
/// la GPLv3 (§5) impose de garder les avis de copyright du projet d'origine et de dire que le
/// code a été modifié, donc l'origine est nommée en tête, pas en note de bas de page. Et deux des
/// licences des composants ne tiennent que si leur texte entier voyage avec le programme — le MIT
/// des icônes Phosphor et l'OFL de la police Inter — d'où la fenêtre « Textes de licence » à un
/// clic, plutôt qu'un résumé qui ne vaudrait rien.
///
/// **Tout est lu au moment où la fenêtre s'ouvre.** Le dialogue est construit quand l'action part
/// et détruit quand il se ferme : il parle donc toujours la langue en vigueur et dit la vérité sur
/// le binaire qui tourne.
class OAboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OAboutDialog(QWidget *parent = nullptr);

    /// Les versions dont ce binaire est fait : la sienne, et le Qt qui l'exécute.
    ///
    /// Séparé de l'apparence parce que le bouton « Copier » remet exactement ce texte au
    /// presse-papiers, et qu'un rapport de bug se rédige plus vite quand il commence par deux
    /// lignes que personne n'a à demander.
    static auto TechnicalText() -> QString;

private:
    void Fill();

    QLabel *m_wordmark = nullptr;
    QLabel *m_version = nullptr;
    QLabel *m_copyright = nullptr;
    QLabel *m_legal = nullptr;
    QTextBrowser *m_body = nullptr;
    QPushButton *m_copy = nullptr;
    QPushButton *m_licences = nullptr;
    QPushButton *m_close = nullptr;
};

#endif // OABOUTDIALOG_H
