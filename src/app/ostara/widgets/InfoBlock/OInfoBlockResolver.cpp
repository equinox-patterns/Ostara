/************************************************************************
 **
 **  @file   OInfoBlockResolver.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   31 August, 2026
 **
 **  @brief  Real-data resolution for the information blocks.
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

#include "OInfoBlockResolver.h"

#include "OPieceCartoucheCatalogue.h"
#include "OPieceCartoucheOptions.h"
#include "core/otemplatesection.h"
#include "core/otemplateseparatoroptions.h"
#include "OPieceCartoucheStorage.h"
#include "core/osizetable.h"

#include "core/ologolibrary.h"
#include "core/ologorender.h"

#include "ifc/xml/vabstractpattern.h"
#include "vmisc/def.h"
#include "vmisc/vabstractvalapplication.h"
#include "vmisc/vcommonsettings.h"
#include "vpatterndb/oinfoblockdata.h"
#include "vpatterndb/vcontainer.h"
#include "vpatterndb/ipieceinfo.h"
#include "vwidgets/ocartouchepainter.h"

#include <QApplication>
#include <QCoreApplication>
#include <QFileInfo>
#include <QPainter>

#include <algorithm>

namespace
{
// « Stature : 168 », « Tour de poitrine 89 cm » — composing such a line is one rule for both
// families of templates, so it lives in the shared vocabulary. It used to be written here and again
// in the preview, which is two places to change the day the rule changes.
using OTemplateSeparatorOptions::Compose;
using OTemplateSeparatorOptions::Join;
using OTemplateSeparatorOptions::kParenthesesSeparator;

QString Translate(const QString &locale, const char *context, const char *source)
{
    if (locale.isEmpty())
    {
        return QCoreApplication::translate(context, source);
    }
    return TranslateToLocale(locale, context, source);
}
}  // namespace

namespace
{
// Effective config + the loaded template, so the caller can reuse it
// instead of loading the small .oct file twice.
struct OInfoBlockResolvedConfig
{
    OInfoBlockEffectiveState state;
    OPieceCartoucheTemplate tpl;
};

OInfoBlockResolvedConfig ResolveConfig(const IPieceInfo &piece, VAbstractPattern *doc, const VContainer *data)
{
    OInfoBlockResolvedConfig config;
    OInfoBlockEffectiveState &state = config.state;
    if (doc == nullptr || data == nullptr)
    {
        return config;
    }

    const OInfoBlockData &info = piece.GetInfoBlockData();

    // Effective activation: explicit per-piece value, else the pattern seed.
    state.active = info.IsSet() ? info.IsActive() : not doc->GetCartoucheTemplate().isEmpty();
    if (not state.active)
    {
        return config;
    }

    // Effective template: per-piece override, else pattern, else favourite/"Default".
    QString templateName = info.IsSet() ? info.Template() : QString();
    if (templateName.isEmpty())
    {
        templateName = doc->GetCartoucheTemplate();
    }
    if (templateName.isEmpty())
    {
        templateName = VAbstractValApplication::VApp()->Settings()->GetDefaultCartoucheTemplate();
    }
    if (templateName.isEmpty())
    {
        templateName = QStringLiteral("Default");
    }

    const QStringList paths = OPieceCartoucheStorage::ScanDirectory(OPieceCartoucheStorage::DefaultDirectory());
    QString filePath;
    for (const QString &path : paths)
    {
        if (QFileInfo(path).completeBaseName() == templateName)
        {
            filePath = path;
            break;
        }
    }
    // Set the effective name before the load: even if the file is gone, the
    // caller (dock tab) keeps it selectable so an edit never overwrites it.
    state.templateName = templateName;
    if (filePath.isEmpty())
    {
        return config;  // template gone -> nothing to render
    }
    QString error;
    if (not OPieceCartoucheStorage::Load(filePath, &config.tpl, &error))
    {
        return config;
    }


    // Effective dispatch: explicit per-piece dispatch, else every section -> block 1.
    if (info.IsSet() && not info.Dispatch().isEmpty())
    {
        state.dispatch = info.Dispatch();
    }
    else
    {
        for (int i = 0; i < config.tpl.SectionCount(); ++i)
        {
            state.dispatch.insert(config.tpl.SectionTypeId(i), 1);
        }
    }
    return config;
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OInfoBlockResolver::ResolveEffectiveConfig(const IPieceInfo &piece, VAbstractPattern *doc, const VContainer *data)
    -> OInfoBlockEffectiveState
{
    return ResolveConfig(piece, doc, data).state;
}

//---------------------------------------------------------------------------------------------------------------------
auto OInfoBlockResolver::ResolveEffectiveState(const IPieceInfo &piece, VAbstractPattern *doc, const VContainer *data)
    -> OInfoBlockEffectiveState
{
    OInfoBlockResolvedConfig config = ResolveConfig(piece, doc, data);
    OInfoBlockEffectiveState &state = config.state;
    if (not state.active || config.tpl.IsEmpty())
    {
        return state;
    }

    // The measurement table is only consumed by the axis/measure sections; load
    // it lazily so text-only templates never pay the .omt parse.
    bool needsTable = false;
    for (int i = 0; i < config.tpl.SectionCount() && !needsTable; ++i)
    {
        const QString typeId = config.tpl.SectionTypeId(i);
        needsTable = typeId == QLatin1String("axis") || typeId == QLatin1String("measure");
    }
    OSizeTable table;
    if (needsTable)
    {
        table.Load();
    }
    const QStringList languages = config.tpl.Languages();
    const QStringList renderLanguages = languages.isEmpty() ? QStringList{QString()} : languages;
    const QString mainLocale = renderLanguages.constFirst();

    for (int i = 0; i < config.tpl.SectionCount(); ++i)
    {
        const OTemplateSection type = OPieceCartoucheCatalogue::Section(config.tpl.SectionTypeId(i));
        if (not type.IsValid())
        {
            continue;
        }
        const int block = state.dispatch.value(type.Id(), 1);
        if (block < 1 || block > 11)
        {
            // "Don't show": the section is asked for nowhere — not block 0, not a block that does
            // not exist. Skipping it here is what makes the information disappear from the piece.
            continue;
        }
        const QList<OCartoucheRenderLine> lines =
            ResolveSectionLines(type, config.tpl.SectionAt(i).options, doc, piece, data, &table, renderLanguages,
                                mainLocale);
        if (lines.isEmpty())
        {
            continue;  // dead data -> no line
        }
        state.blockContent[block].append(lines);
    }

    // A block exists only if at least one section resolved to content.
    for (auto it = state.blockContent.begin(); it != state.blockContent.end();)
    {
        if (it.value().isEmpty())
        {
            it = state.blockContent.erase(it);
        }
        else
        {
            ++it;
        }
    }

    return state;
}

//---------------------------------------------------------------------------------------------------------------------
auto OInfoBlockResolver::ResolveSectionLines(const OTemplateSection &type,
                                             const QHash<QString, QVariant> &options, const VAbstractPattern *doc,
                                             const IPieceInfo &piece, const VContainer *data,
                                             const OSizeTable *table, const QStringList &renderLanguages,
                                             const QString &mainLocale) -> QList<OCartoucheRenderLine>
{
    QList<OCartoucheRenderLine> lines;
    const QFont baseFont = OCartouchePainter::BaseFont();
    const QFont font = OCartouchePainter::SectionFont(options, baseFont);
    const int lineSpacing = options.value(QStringLiteral("lineSpacing"), 0).toInt();
    const QString typeId = type.Id();
    const QString separator =
        Translate(mainLocale, "OInfoBlockPreview", qPrintable(options.value(QStringLiteral("separator")).toString()));

    if (typeId == QLatin1String("axis"))
    {
        if (table == nullptr || not table->HasMultiSizeTable())
        {
            return {};  // dead data: no multi-size table
        }

        // **The axes the user ticked, not the one a combo named.** A file written when an
        // information could show a single axis names it under a key of its own; read as the
        // one-element list it means, so an older cartouche keeps showing what it showed.
        const QString legacyAxis = options.value(QStringLiteral("axis")).toString();
        QStringList axes = options.value(OTemplateAxisOptions::kAxes).toStringList();
        if (axes.isEmpty() && not legacyAxis.isEmpty())
        {
            axes.append(legacyAxis);
        }

        const QMap<QString, OAxisRenames> renamesByAxis =
            ParseAxisRenamesByAxis(options.value(OPieceCartoucheAxisRename::kRenames), legacyAxis);
        const bool oneLine = options.value(OPieceCartoucheAxisRename::kOneLine, false).toBool();

        // One line per axis, in the order the table declares them — which is the order the
        // selection was stored in, whatever order the boxes were ticked in: a cartouche that
        // listed the same three axes in a different order from one file to the next would be
        // describing the same pattern two ways.
        for (const QString &axis : axes)
        {
            if (not table->AxisNames().contains(axis))
            {
                continue;  // dead data: an axis the current table does not declare
            }
            const QString anchorValue = table->AnchorValue(axis);
            const QStringList axisValues = table->AxisValues(axis);
            const int anchorIndex = static_cast<int>(axisValues.indexOf(anchorValue));
            const OAxisRenames renames = renamesByAxis.value(axis);

            QStringList languageLines;
            for (const QString &locale : renderLanguages)
            {
                const OAxisRename rename = renames.value(locale);
                const QString displayName = rename.name.isEmpty() ? axis : rename.name;
                QString displayValue = anchorValue;
                if (anchorIndex >= 0 && anchorIndex < rename.values.size() && not rename.values.at(anchorIndex).isEmpty())
                {
                    displayValue = rename.values.at(anchorIndex);
                }
                const QString sep =
                    Translate(locale, "OInfoBlockPreview", qPrintable(options.value(QStringLiteral("separator")).toString()));
                languageLines.append(Compose(displayName, displayValue, sep));
            }
            if (oneLine)
            {
                for (const QString &text : languageLines)
                {
                    lines.append(OCartouchePainter::TextLines(text, font, lineSpacing));
                }
            }
            else
            {
                lines.append(OCartouchePainter::TextLine(languageLines.join(OTemplateSeparatorOptions::kLanguageJoin), font, lineSpacing));
            }
        }
        return lines;
    }

    if (typeId == QLatin1String("measure"))
    {
        if (table == nullptr || not table->HasTable())
        {
            return {};  // dead data: no measurement table (.omt or .otu)
        }
        const QString storedFull = options.value(QStringLiteral("measureName")).toString();
        if (storedFull.isEmpty())
        {
            return {};
        }
        const QString name = table->MeasurementNameFromFullName(storedFull);
        if (name.isEmpty())
        {
            return {};  // dead data: measurement not in the current table
        }
        const QString value =
            OSizeTable::FormatValue(table->MeasurementValue(name)) + QLatin1Char(' ') + table->UnitsStr();
        const QMap<QString, OAxisRename> renames =
            ParseAxisRenames(options.value(OPieceCartoucheAxisRename::kRenames));
        const bool oneLine = options.value(OPieceCartoucheAxisRename::kOneLine, false).toBool();

        QStringList names;
        for (const QString &locale : renderLanguages)
        {
            const OAxisRename rename = renames.value(locale);
            names.append(rename.name.isEmpty() ? storedFull : rename.name);
        }
        if (oneLine)
        {
            for (int i = 0; i < names.size(); ++i)
            {
                const QString sep = Translate(renderLanguages.at(i), "OInfoBlockPreview",
                                              qPrintable(options.value(QStringLiteral("separator")).toString()));
                lines.append(OCartouchePainter::TextLine(Compose(names.at(i), value, sep), font, lineSpacing));
            }
        }
        else
        {
            // **The languages stand as a title's do**: the first bare, the others bracketed as one
            // aside — « 168 (Height, Grösse) ». The rule lives in one place now, so a cartouche and a
            // cover page cannot come to hold the same two things apart in two different ways.
            QString display = names.isEmpty() ? storedFull : Join(names, kParenthesesSeparator);
            const QString sep = Translate(mainLocale, "OInfoBlockPreview",
                                          qPrintable(options.value(QStringLiteral("separator")).toString()));
            lines.append(OCartouchePainter::TextLine(Compose(display, value, sep), font, lineSpacing));
        }
        return lines;
    }

    if (typeId == QLatin1String("pieceNumber"))
    {
        const uint number = piece.GetPieceNumber();
        if (number == 0)
        {
            return {};  // dead data: no piece number
        }
        // « Un groupe = 1 pièce » : the total must be the effective count, not the raw
        // piece hash (which still includes the members of a group).
        const auto total = static_cast<quint32>(data->GetEffectivePieceCount());
        const QString style = options.value(OPieceCartoucheNumber::kStyle).toString();
        lines.append(OCartouchePainter::TextLine(
            Compose(FormatPieceNumber(number, style), FormatPieceNumber(total, style), separator), font,
            lineSpacing));
        return lines;
    }

    if (typeId == QLatin1String("cut"))
    {
        struct CutTab
        {
            int qty;
            bool onFold;
            const char *display;
        };
        const CutTab kCutTabs[] = {
            {piece.GetCutFabricQty(), piece.IsCutFabricOnFold(),
             QT_TRANSLATE_NOOP("OInfoBlockPreview", "fabric")},
            {piece.GetCutInterfacingQty(), piece.IsCutInterfacingOnFold(),
             QT_TRANSLATE_NOOP("OInfoBlockPreview", "interfacing")},
            {piece.GetCutLiningQty(), piece.IsCutLiningOnFold(),
             QT_TRANSLATE_NOOP("OInfoBlockPreview", "lining")},
        };

        bool any = false;
        for (const QString &locale : renderLanguages)
        {
            for (const CutTab &tab : kCutTabs)
            {
                if (tab.qty <= 0)
                {
                    continue;  // empty tab -> no line
                }
                any = true;
                QString text = Translate(locale, "OInfoBlockPreview",
                                         QT_TRANSLATE_NOOP("OInfoBlockPreview", "Cut %1"))
                                   .arg(tab.qty);
                if (tab.onFold)
                {
                    text += Translate(locale, "OInfoBlockPreview",
                                      QT_TRANSLATE_NOOP("OInfoBlockPreview", " on fold"));
                }
                text += Translate(locale, "OInfoBlockPreview", QT_TRANSLATE_NOOP("OInfoBlockPreview", " (%1)"))
                            .arg(Translate(locale, "OInfoBlockPreview", tab.display));
                lines.append(OCartouchePainter::TextLine(text, font, lineSpacing));
            }
        }
        if (not any)
        {
            return {};  // dead data: no fabric tab filled
        }
        return lines;
    }

    if (typeId == QLatin1String("logo"))
    {
        const QString stored = options.value(QStringLiteral("logo")).toString();
        if (stored.isEmpty())
        {
            return {};  // dead data: no logo chosen
        }
        // Drawing the logo is the same six steps wherever it is done, so it is done in one place
        // — see OLogoRender. What is left here is this family's part: which option holds the file,
        // and what to do when there is nothing to draw.
        const double sizeCm = options.value(QStringLiteral("logoSize"), 2.0).toDouble();
        const OLogoRender::Drawing drawing = OLogoRender::Draw(stored, sizeCm);
        if (drawing.pixmap.isNull())
        {
            return {};  // dead data: missing or invalid SVG
        }
        lines.append(OCartouchePainter::LogoLine(
            drawing.pixmap, drawing.heightPx, lineSpacing,
            OLogoLibrary::Directory() + QLatin1Char('/') + stored));
        return lines;
    }

    // Generic text sections — resolved from the real data, skipped when empty.
    QString text;
    if (typeId == QLatin1String("patternName"))
    {
        text = doc->GetPatternName();
    }
    else if (typeId == QLatin1String("client"))
    {
        text = doc->GetCustomer();
    }
    else if (typeId == QLatin1String("pieceName"))
    {
        text = piece.GetName();
    }
    else if (typeId == QLatin1String("freeText"))
    {
        text = piece.GetFreeText();
    }
    else if (typeId == QLatin1String("collection"))
    {
        text = doc->GetCollection();
    }
    else if (typeId == QLatin1String("fabricInfo"))
    {
        text = doc->GetFabricInfo();
    }
    if (text.isEmpty())
    {
        return {};  // dead data
    }
    // What was typed may run over several lines — the free text of a piece is described as being
    // able to — and one render line is one line on the page. Text without a break comes back as
    // the one line it always was.
    lines.append(OCartouchePainter::TextLines(text, font, lineSpacing));
    return lines;
}
