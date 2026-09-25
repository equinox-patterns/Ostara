/************************************************************************
 **
 **  @file   oexportpaginationsettings.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/09/2026
 **
 **  @brief  The tables the pagination combo boxes are built from.
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

#include "oexportpaginationsettings.h"

#include <QCoreApplication>

namespace
{
/**
 * The two forms, in the order the combo shows them. In the enum's order too, which is what lets
 * the combo be read back by index.
 *
 * The macro is QT_TRANSLATE_NOOP so that lupdate sees the context; the translation itself happens
 * at display time, with QCoreApplication::translate on the same context.
 */
const QVector<OExportPaginationFormModel> kForms{
    {QT_TRANSLATE_NOOP("OExportPaginationSettings", "Number"), "number"},
    {QT_TRANSLATE_NOOP("OExportPaginationSettings", "Number + letter"), "number+letter"},
};

} // namespace

auto OExportPaginationForms() -> const QVector<OExportPaginationFormModel> &
{
    return kForms;
}
