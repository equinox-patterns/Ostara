/************************************************************************
 **
 **  @file   OInfoBlockResolver.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   31 August, 2026
 **
 **  @brief  Resolves the effective per-piece information block state and
 **          its rendered content from the real pattern / piece / table
 **          data. Consumed by the on-piece blocks (via the app-registered
 **          builder) and by the Blocks dock tab.
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

#ifndef OINFOBLOCKRESOLVER_H
#define OINFOBLOCKRESOLVER_H

#include "vtools/tools/oinfoblockcontent.h"

#include <QMap>

class IPieceInfo;
class VAbstractPattern;
class VContainer;
class OTemplateSection;
class OSizeTable;

/**
 * @brief Real-data resolution for the information blocks.
 *
 * The effective state applies the seed-vs-override semantics: an unset
 * piece follows the pattern-level seed (activation + template), a set
 * piece carries its explicit values. Sections resolve against the real
 * document / piece / measurement table; dead data is skipped (spec §4.3).
 *
 * @par Which size a cartouche describes
 * The table's reference (anchor) size is the default, because that is the size a pattern's
 * measurements are written against. A caller that resolves a cartouche for ANOTHER size — an
 * export run putting its pieces on the size the user asked for — hands over that size as
 * @c sizeAxes: the value each axis stands at, keyed the way the table names its axes. The
 * axis lines print those values and the measure lines read the table's grid at that same
 * size, so a printed cartouche cannot state a size the geometry it sits on does not have.
 */
class OInfoBlockResolver
{
public:
    /** @brief Active / effective template / dispatch only (no content resolution). */
    static auto ResolveEffectiveConfig(const IPieceInfo &piece, VAbstractPattern *doc, const VContainer *data)
        -> OInfoBlockEffectiveState;

    /** @brief Full effective state including the rendered lines of every block. */
    static auto ResolveEffectiveState(const IPieceInfo &piece, VAbstractPattern *doc, const VContainer *data,
                                      const QMap<QString, QString> &sizeAxes = {})
        -> OInfoBlockEffectiveState;

private:
    static auto ResolveSectionLines(const OTemplateSection &type,
                                    const QHash<QString, QVariant> &options, const VAbstractPattern *doc,
                                    const IPieceInfo &piece, const VContainer *data,
                                    const OSizeTable *table, const QStringList &renderLanguages,
                                    const QString &mainLocale, const QMap<QString, QString> &sizeAxes)
        -> QList<OCartoucheRenderLine>;
};

#endif // OINFOBLOCKRESOLVER_H
