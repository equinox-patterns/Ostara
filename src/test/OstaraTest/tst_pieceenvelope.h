/************************************************************************
 **
 **  @file   tst_pieceenvelope.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   14/10/2026
 **
 **  @brief  The envelope of a library piece: its sections are written **in the order of the schema**
 **          (`opiece/v1.0.0.xsd`), whatever the order of the calls, and the measurements a piece reads are
 **          **named** there, once each.
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
    void NotesAreCarriedByTheBlock();
    void EmptySectionsLeaveTheDocument();
    void TheWrittenEnvelopeIsLeftForTheSchemaToRead();
};

#endif // TST_PIECEENVELOPE_H
