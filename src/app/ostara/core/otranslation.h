/************************************************************************
 **
 **  @file   otranslation.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   25/09/2026
 **
 **  @brief  Retraduction des textes posés en code — un seul mécanisme pour
 **          toute l'application, au lieu d'un `RetranslateUI()` par classe.
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

#ifndef OTRANSLATION_H
#define OTRANSLATION_H

#include <QObject>

class QAbstractButton;
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QTabWidget;
class QTableWidget;
class QWidget;

/**
 * @brief Le mécanisme unique de retraduction des textes construits en code.
 *
 * **Le problème.** Qt traduit au moment où le texte est posé. Un texte écrit une fois dans un
 * constructeur — un `new QLabel(tr("..."))` — reste donc dans la langue du premier affichage pour
 * toute la vie du widget. La doc Qt répond « surchargez `changeEvent()` » dans chaque classe, et
 * chaque classe doit alors savoir **quoi** réappliquer et **où** le retrouver : c'est ce qui a
 * laissé passer, une à une, des surfaces entières du dock Options.
 *
 * **La réponse.** On ne demande plus à aucune classe de se souvenir de rien. Le texte passe par un
 * poseur — `OTTranslation::SetText(...)` — qui **mémorise la chaîne source, son contexte et sa
 * place sur le widget lui-même**, puis pose la traduction courante. Un moteur installé une fois sur
 * l'application écoute `QEvent::LanguageChange`, parcourt tous les widgets et **rejoue ce qui a été
 * mémorisé**, dans la langue qui vient d'être installée.
 *
 * Un widget qui passe par le poseur est couvert définitivement. Le code écrit plus tard l'est
 * aussi, sans que personne ait à y penser.
 *
 * **Contexte.** Le contexte de traduction est celui de la classe **propriétaire** — c'est celui
 * qu'utilise `tr()` à l'endroit où le texte a été écrit. Il est donc passé explicitement : le
 * widget lui-même ne le connaît pas (`metaObject()->className()` d'un `QLabel` rend « QLabel »,
 * pas la classe qui l'a construit).
 *
 * **Un widget peut porter plusieurs textes** — les onglets d'un `QTabWidget`, les items d'un
 * `QComboBox`, les colonnes d'un `QTableWidget`. Chaque pose ajoute donc une entrée à une liste
 * portée par le widget, et le moteur rejoue la liste entière.
 */
namespace OTranslation
{
/**
 * @brief Pose un texte traduit sur un libellé et le rend retraduisible.
 * @param owner classe propriétaire, qui donne le contexte de traduction.
 * @param widget libellé à écrire.
 * @param source chaîne source, telle qu'elle serait passée à `tr()`.
 */
void SetText(const QObject *owner, QLabel *widget, const char *source);

/**
 * @brief Pose un texte traduit sur un bouton (ou une case) et le rend retraduisible.
 */
void SetText(const QObject *owner, QAbstractButton *widget, const char *source);

/**
 * @brief Pose le titre traduit d'un groupe et le rend retraduisible.
 */
void SetTitle(const QObject *owner, QGroupBox *widget, const char *source);

/**
 * @brief Pose une infobulle traduite et la rend retraduisible.
 */
void SetToolTip(const QObject *owner, QWidget *widget, const char *source);

/**
 * @brief Pose le texte traduit d'un onglet et le rend retraduisible.
 * @param index rang de l'onglet dans le widget.
 */
void SetTabText(const QObject *owner, QTabWidget *widget, int index, const char *source);

/**
 * @brief Pose le texte traduit d'un item de liste et le rend retraduisible.
 * @param index rang de l'item, tel qu'il a été ajouté.
 */
void SetItemText(const QObject *owner, QComboBox *widget, int index, const char *source);

/**
 * @brief Pose le texte traduit d'une en-tête de colonne et le rend retraduisible.
 * @param column rang de la colonne.
 */
void SetHeaderText(const QObject *owner, QTableWidget *widget, int column, const char *source);

/**
 * @brief Pose un texte d'invite traduit et le rend retraduisible.
 */
void SetPlaceholderText(const QObject *owner, QLineEdit *widget, const char *source);

/**
 * @brief Rejoue tout ce qui a été mémorisé, dans la langue installée à cet instant.
 *
 * Appelée par le moteur sur `LanguageChange`. Peut l'être à la main sans risque : elle est
 * idempotente.
 */
void ReapplyAll();

/**
 * @brief Installe le moteur sur l'application. Sans effet s'il l'est déjà.
 */
void InstallWatcher();
} // namespace OTranslation

#endif // OTRANSLATION_H
