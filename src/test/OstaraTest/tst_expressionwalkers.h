/************************************************************************
 **
 **  @file   tst_expressionwalkers.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/10/2026
 **
 **  @brief  Les attributs qu'un patron peut écrire sous forme de formule doivent tous figurer dans
 **          `ListExpressions()`. Ce n'est pas une commodité : c'est cette liste qui fait qu'un incrément renommé
 **          met à jour les formules qui le citent, que le test « utilisé » le voit, et que les mesures requises
 **          d'une pièce sont complètes. Un attribut oublié se tait — la formule garde l'ancien nom et meurt à la
 **          première évaluation.
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

#ifndef TST_EXPRESSIONWALKERS_H
#define TST_EXPRESSIONWALKERS_H

#include <QObject>

class TST_ExpressionWalkers : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit TST_ExpressionWalkers(QObject *parent = nullptr);

private slots:
    void EveryFormulaAttributeIsListed();
};

#endif // TST_EXPRESSIONWALKERS_H
