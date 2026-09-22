/************************************************************************
 **
 **  @file   oexporttemplate.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/09/2026
 **
 **  @brief  One saved assembly-mark template — a name, and the three blocks it draws with.
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

#ifndef OEXPORTTEMPLATE_H
#define OEXPORTTEMPLATE_H

#include "oexportframesettings.h"
#include "oexportmarksettings.h"
#include "oexportpaginationsettings.h"

#include <QDomElement>
#include <QString>

class QDomDocument;

/**
 * @brief A template: what it is called, and everything it draws on the sheets of a tiled plan.
 *
 * **It carries no sheet and no format.** One template decorates any tiled plan — A3, A4, Letter,
 * Tabloid, or either of the two framed formats — so nothing here names one, and nothing chosen
 * in the format row is saved (§10.1).
 *
 * **A default-constructed one is what a new template is.** Its three blocks keep the values their
 * own headers declare, which is the state the editor opens in; the template shipped with the
 * installation is a *file*, written by OExportTemplateStorage, and is not what this starts from.
 * Sophie's decision, 17-09-2026.
 *
 * Serialised as XML against `://schema/assemblymark_template/v1.0.0.xsd`, in a `.oamt` file under
 * ~/Ostara/PDFTemplate — the cartouche's arrangement, one format further on.
 */
struct OExportTemplate
{
    /** What the template is called. The file it is saved in carries the same name. */
    QString name;

    OExportFrameSettings frame;
    OExportMarkSettings marks;
    OExportPaginationSettings pagination;

    /** @brief This template as an XML element, ready to be appended to a document. */
    auto ToXml(QDomDocument &doc) const -> QDomElement;

    /**
     * @brief Reads a template back off an XML element, and says whether it was one.
     *
     * Every attribute is read over a default-constructed template, so a file written by an older
     * build — or a hand-edited one missing a setting — opens with that setting at its default
     * rather than at nothing. Returns false, and changes nothing, when the element is not a
     * template element at all.
     */
    auto LoadFromXml(const QDomElement &root) -> bool;
};

#endif // OEXPORTTEMPLATE_H
