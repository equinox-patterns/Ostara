/************************************************************************
 **
 **  @file   OPieceCartoucheStorage.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/08/2026
 **
 **  @brief  Scan / load / save of cartouche template files (.oct).
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

#include "OPieceCartoucheStorage.h"

#include "vabstractapplication.h"
#include "xml/vdomdocument.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace
{
/**
 * @brief VDomDocument subclass bound to the cartouche template XSD, so that
 * SaveDocument validates the serialized content before the atomic commit.
 */
class CartoucheTemplateDocument : public VDomDocument
{
public:
    explicit CartoucheTemplateDocument(QObject *parent = nullptr) : VDomDocument(parent)
    {
    }

    auto SaveSchema() const -> QString override
    {
        return QStringLiteral("://schema/cartouche_template/v1.0.0.xsd");
    }
};
}  // namespace

namespace OPieceCartoucheStorage
{
auto DefaultDirectory() -> QString
{
    return VAbstractApplication::VApp()->Settings()->GetPathCartoucheTemplates();
}

auto ScanDirectory(const QString &dirPath) -> QStringList
{
    QDir dir(dirPath);
    if (!dir.exists())
    {
        return {};
    }

    const QStringList files =
        dir.entryList({QStringLiteral("*.oct")}, QDir::Files, QDir::Name | QDir::IgnoreCase);

    QStringList paths;
    paths.reserve(files.size());
    for (const QString &file : files)
    {
        paths.append(dir.absoluteFilePath(file));
    }
    return paths;
}

auto Load(const QString &filePath, OPieceCartoucheTemplate *tpl, QString *error) -> bool
{
    if (tpl == nullptr)
    {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        if (error != nullptr)
        {
            *error = file.errorString();
        }
        return false;
    }

    CartoucheTemplateDocument doc;
    const QDomDocument::ParseResult result = doc.setContent(&file);
    if (!result)
    {
        if (error != nullptr)
        {
            *error = result.errorMessage;
        }
        return false;
    }

    if (!tpl->LoadFromXml(doc.documentElement()))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("Not a cartouche template file.");
        }
        return false;
    }
    return true;
}

auto Save(const OPieceCartoucheTemplate &tpl, const QString &filePath, QString *error) -> bool
{
    // Ensure the destination folder exists (a typed path or a deleted folder
    // must not fail with a raw "No such file or directory").
    QDir().mkpath(QFileInfo(filePath).absolutePath());

    CartoucheTemplateDocument doc;
    doc.appendChild(tpl.ToXml(doc));

    QString err;
    const bool ok = doc.SaveDocument(filePath, err);
    if (!ok && error != nullptr)
    {
        *error = err;
    }
    return ok;
}

void EnsureDefaultTemplate()
{
    const QString dir = DefaultDirectory();
    if (!ScanDirectory(dir).isEmpty())
    {
        return;
    }

    if (!QDir().mkpath(dir))
    {
        qWarning() << "Cannot create cartouche templates folder:" << dir;
        return;
    }

    OPieceCartoucheTemplate tpl(QStringLiteral("Default"));
    tpl.AddSection(QStringLiteral("patternName"));
    tpl.AddSection(QStringLiteral("pieceNumber"));
    tpl.AddSection(QStringLiteral("pieceName"));
    tpl.AddSection(QStringLiteral("cut"));

    const QString path = dir + QLatin1Char('/') + QStringLiteral("Default.oct");
    QString error;
    if (!Save(tpl, path, &error))
    {
        qWarning() << "Cannot create default cartouche template:" << error;
    }
}
}  // namespace OPieceCartoucheStorage
