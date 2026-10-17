/************************************************************************
 **
 **  @file   tst_oomtconverter.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/10/2026
 **
 **  @brief  Un `.vst` antérieur au schéma 0.5.0 n'a pas d'élément `<dimensions>` : sa taille et sa hauteur y sont
 **          deux valeurs de base, et ses mesures portent `size_increase` / `height_increase` au lieu de
 **          `shiftA`/`shiftB`/`shiftC`. Le lire tel quel ne trouve donc ni dimensions ni mesures — c'est le défaut
 **          qui empêchait de convertir un ancien `.vst` en `.omt`. Le convertisseur doit d'abord le mettre à niveau,
 **          exactement comme le chargeur de patron.
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

#ifndef TST_OOMTCONVERTER_H
#define TST_OOMTCONVERTER_H

#include <QObject>

class TST_OomtConverter : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit TST_OomtConverter(QObject *parent = nullptr);

private slots:
    void LegacyVstIsUpgradedBeforeParsing();
};

#endif // TST_OOMTCONVERTER_H
