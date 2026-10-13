/************************************************************************
 **
 **  @file   tst_pieceenvelope.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   14/10/2026
 **
 **  @brief  L'enveloppe d'une pièce de bibliothèque : ses sections sont écrites **dans l'ordre du schéma**
 **          (`opiece/v1.0.0.xsd`), quel que soit l'ordre des appels, et les mesures qu'une pièce lit y sont
 **          **nommées**, une fois chacune.
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

#ifndef TST_PIECEENVELOPE_H
#define TST_PIECEENVELOPE_H

#include <QObject>

class TST_PieceEnvelope : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit TST_PieceEnvelope(QObject *parent = nullptr);

private slots:
    void SectionsAreWrittenInTheSchemaOrder();
    void MeasurementsAreNamedOnceAndInOrder();
    void EmptySectionsLeaveTheDocument();
    void TheWrittenEnvelopeIsLeftForTheSchemaToRead();
};

#endif // TST_PIECEENVELOPE_H
