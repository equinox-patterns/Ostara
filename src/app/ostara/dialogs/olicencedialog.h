/************************************************************************
 **
 **  @file   olicencedialog.h
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

#ifndef OLICENCEDIALOG_H
#define OLICENCEDIALOG_H

#include <QDialog>

class QTextBrowser;

/// « Textes de licence » — les textes entiers, pas un résumé.
///
/// **Les textes voyagent dans le binaire** (`:/licenses/…`, voir `licenses.qrc`), pas dans le
/// dossier `Resources` : une copie de l'application dont ce dossier a été vidé doit encore pouvoir
/// dire ce qu'elle doit. Le MIT et l'OFL ne valent que complets ; les autres n'ont de sens que
/// lisibles, et c'est la même fenêtre qui les montre.
///
/// **L'ordre n'est pas décoratif :** le texte du programme vient en premier (GPLv3 §4), les
/// composants ensuite, chacun avec le nom de sa licence dans son titre.
class OLicenceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OLicenceDialog(QWidget *parent = nullptr);

private:
    QTextBrowser *m_body = nullptr;
};

#endif // OLICENCEDIALOG_H
