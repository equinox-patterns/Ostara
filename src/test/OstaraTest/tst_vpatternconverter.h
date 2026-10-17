/************************************************************************
 **
 **  @file   tst_vpatternconverter.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/10/2026
 **
 **  @brief  Les noms que la conversion doit réécrire dans un vieux patron — et ceux qu'elle ne doit pas toucher.
 **          Un patron de 1.0.x nommait `Line_P2_H` la ligne d'un point Height ; depuis 1.1.0 la même ligne
 **          s'appelle `Line_H_P2`. Sans réécriture, la formule ne se résout plus à l'ouverture et le fichier
 **          refuse de s'ouvrir avec « Unexpected token ».
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

#ifndef TST_VPATTERNCONVERTER_H
#define TST_VPATTERNCONVERTER_H

#include <QObject>
#include <QTemporaryDir>

class TST_PatternConverter : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit TST_PatternConverter(QObject *parent = nullptr);

private slots:
    void initTestCase();

    void HeightLineNamesAreRewritten();

private:
    auto WriteFile(const QString &name, const QByteArray &content) -> QString;

    QTemporaryDir m_dir;
};

#endif // TST_VPATTERNCONVERTER_H
