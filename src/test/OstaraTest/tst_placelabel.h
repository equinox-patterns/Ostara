/************************************************************************
 **
 **  @file   tst_placelabel.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/11/2026
 **
 **  @brief  Le repère interne « cercle plein » : la seule forme dont l'encre remplit le tracé.
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

#ifndef TST_PLACELABEL_H
#define TST_PLACELABEL_H

#include <QObject>

class TST_PlaceLabel : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit TST_PlaceLabel(QObject *parent = nullptr);
    ~TST_PlaceLabel() override = default;

private slots:
    void OnlyTheDiscIsFilled();
    void TheDiscIsTheCircle();
    void TheDiscCanBeFilled();
    void EveryTypeDrawsSomething();
    void TheSymbolLibraryShowsTheDiscFilled();
    void TheHandleIsClickableButPaintsNothing();
};

#endif // TST_PLACELABEL_H
