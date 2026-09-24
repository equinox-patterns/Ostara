/************************************************************************
 **
 **  @file   oexportfolder.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Implementation du dossier d'export.
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

#include "oexportfolder.h"

#include "core/oexportoutput.h"

#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vcommonsettings.h"

#include <QDir>
#include <QStandardPaths>

namespace
{

/// @brief Le repli, quand le dossier de la gamme et celui de l'application ne mènent nulle part.
auto DocumentsFolder() -> QString
{
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
}

} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto ExportFolder(const OExportOutput &output) -> QString
{
    QString directory = output.hasRange ? output.range.exportPath : QString();
    if (directory.isEmpty() || !QDir(directory).exists())
    {
        directory = VAbstractValApplication::VApp()->Settings()->GetPathLayoutExport();
    }

    if (directory.isEmpty() || !QDir(directory).exists())
    {
        directory = DocumentsFolder();
    }

    return directory;
}

//---------------------------------------------------------------------------------------------------------------------
auto LastExportFolder() -> QString
{
    QString directory = VAbstractValApplication::VApp()->Settings()->GetPathLayoutExport();
    if (directory.isEmpty() || !QDir(directory).exists())
    {
        directory = DocumentsFolder();
    }

    return directory;
}

//---------------------------------------------------------------------------------------------------------------------
void RememberExportFolder(const QString &directory)
{
    if (!directory.isEmpty() && QDir(directory).exists())
    {
        VAbstractValApplication::VApp()->Settings()->SetPathLayoutExport(directory);
    }
}
