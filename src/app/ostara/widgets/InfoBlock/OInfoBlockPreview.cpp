/************************************************************************
 **
 **  @file   OInfoBlockPreview.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/08/2026
 **
 **  @brief  Live preview of a cartouche template — one stacked line per section.
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

#include "OInfoBlockPreview.h"

#include "OPieceCartoucheCatalogue.h"
#include "OPieceCartoucheOptions.h"
#include "core/osizetable.h"

#include "core/ologorender.h"

#include "vwidgets/ocartouchepainter.h"

#include "def.h"

#include <QCoreApplication>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>

#include <algorithm>

namespace
{
// Compose "name [separator] value", e.g. "Stature : 168" or "Tour de poitrine 89 cm".
QString ComposeLine(const QString &name, const QString &value, const QString &separator)
{
    if (value.isEmpty())
    {
        return name;
    }
    if (separator.isEmpty())
    {
        return name + QLatin1Char(' ') + value;
    }
    return name + QLatin1Char(' ') + separator + QLatin1Char(' ') + value;
}
}  // namespace

OInfoBlockPreview::OInfoBlockPreview(QWidget *parent) : QWidget(parent)
{
    setMinimumWidth(180);
}

void OInfoBlockPreview::SetTemplate(const OPieceCartoucheTemplate *tpl)
{
    m_template = tpl;
    update();
}

void OInfoBlockPreview::SetTable(const OSizeTable *table)
{
    m_table = table;
    update();
}

void OInfoBlockPreview::Refresh()
{
    update();
}

void OInfoBlockPreview::RetranslateUI()
{
    // paintEvent re-resolves tr() and the catalogue DisplayName() on every paint.
    update();
}

void OInfoBlockPreview::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.fillRect(rect(), palette().base());

    if (m_template == nullptr || m_template->IsEmpty())
    {
        painter.setPen(palette().placeholderText().color());
        painter.drawText(rect(), Qt::AlignCenter, tr("No sections yet."));
        return;
    }

    // Block display language(s). The software-managed sections (cut, piece
    // number) render one line per language, in combo order; the other sections
    // render their sample in the main language (combo 1). Each string is
    // translated through the app's own catalogues; an empty locale means the
    // app's UI language (no language chosen).
    const QStringList languages = m_template->Languages();
    const QStringList renderLanguages = languages.isEmpty() ? QStringList{QString()} : languages;
    const QString mainLocale = renderLanguages.constFirst();
    const auto Tl = [](const QString &locale, const char *context, const char *source) -> QString {
        if (locale.isEmpty())
        {
            return QCoreApplication::translate(context, source);
        }
        return TranslateToLocale(locale, context, source);
    };

    QVector<OCartoucheRenderLine> lines;
    const QFont baseFont = font();  // widget's own font — reset per line, no style cascade
    for (int i = 0; i < m_template->SectionCount(); ++i)
    {
        const auto &type = OPieceCartoucheCatalogue::Section(m_template->SectionTypeId(i));
        if (!type.IsValid())
        {
            continue;
        }

        const auto &options = m_template->SectionAt(i).options;
        const QFont font = OCartouchePainter::SectionFont(options, baseFont);
        const int lineSpacing = options.value(QStringLiteral("lineSpacing"), 0).toInt();

        // Text sections — the sample line is the section's source name in the
        // main language (the catalogue translates it for the UI elsewhere).
        QString line = Tl(mainLocale, "OTemplateSection", qPrintable(type.SourceName()));
        // The separator values are language-neutral keys ("of", "-", "/"); translate
        // the word ones at render ("of" → "sur"). Main-language separator for the
        // table-driven sections (axis / measurement).
        const QString separator =
            Tl(mainLocale, "OInfoBlockPreview", qPrintable(options.value(QStringLiteral("separator")).toString()));
        if (type.Id() == QStringLiteral("axis"))
        {
            if (m_table == nullptr || !m_table->HasMultiSizeTable())
            {
                continue;  // dead data: no multi-size table
            }
            const QString axis = options.value(QStringLiteral("axis")).toString();
            if (axis.isEmpty() || !m_table->AxisNames().contains(axis))
            {
                continue;  // dead data: axis not in the current table
            }

            // Per-language display aliases of the axis and its values: the
            // anchor value is looked up by its position in the axis values.
            const QString anchorValue = m_table->AnchorValue(axis);
            const QStringList axisValues = m_table->AxisValues(axis);
            const int anchorIndex = static_cast<int>(axisValues.indexOf(anchorValue));
            const QMap<QString, OAxisRename> renames =
                ParseAxisRenames(options.value(OPieceCartoucheAxisRename::kRenames));
            const bool oneLine = options.value(OPieceCartoucheAxisRename::kOneLine, false).toBool();

            QStringList languageLines;
            for (const QString &locale : renderLanguages)
            {
                const OAxisRename rename = renames.value(locale);
                const QString displayName = rename.name.isEmpty() ? axis : rename.name;
                QString displayValue = anchorValue;
                if (anchorIndex >= 0 && anchorIndex < rename.values.size() && !rename.values.at(anchorIndex).isEmpty())
                {
                    displayValue = rename.values.at(anchorIndex);
                }
                const QString sep =
                    Tl(locale, "OInfoBlockPreview", qPrintable(options.value(QStringLiteral("separator")).toString()));
                languageLines.append(ComposeLine(displayName, displayValue, sep));
            }

            if (oneLine)
            {
                // Every block language on its own line.
                for (const QString &text : languageLines)
                {
                    lines.append(OCartouchePainter::TextLine(text, font, lineSpacing));
                }
            }
            else
            {
                // All languages on a single line, joined by " - ".
                lines.append(OCartouchePainter::TextLine(languageLines.join(QStringLiteral(" - ")), font, lineSpacing));
            }
            continue;
        }
        else if (type.Id() == QStringLiteral("measure"))
        {
            if (m_table == nullptr || !m_table->HasTable())
            {
                continue;  // dead data: no measurement table (.omt or .otu)
            }
            const QString storedFull = options.value(QStringLiteral("measureName")).toString();
            if (storedFull.isEmpty())
            {
                continue;
            }
            const QString name = m_table->MeasurementNameFromFullName(storedFull);
            if (name.isEmpty())
            {
                continue;  // dead data: measurement not in the current table
            }
            const QString value = OSizeTable::FormatValue(m_table->MeasurementValue(name))
                                  + QLatin1Char(' ') + m_table->UnitsStr();

            // Per-language display aliases of the measurement name (the value is
            // computed from the active size, it is not renamed).
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
                // Every block language on its own line.
                for (int i = 0; i < names.size(); ++i)
                {
                    const QString sep = Tl(renderLanguages.at(i), "OInfoBlockPreview",
                                           qPrintable(options.value(QStringLiteral("separator")).toString()));
                    lines.append(OCartouchePainter::TextLine(ComposeLine(names.at(i), value, sep), font, lineSpacing));
                }
            }
            else
            {
                // Main language first, the other languages in parentheses.
                QString display = names.isEmpty() ? storedFull : names.constFirst();
                if (names.size() > 1)
                {
                    display += QStringLiteral(" (") + names.mid(1).join(QStringLiteral(", ")) + QLatin1Char(')');
                }
                const QString sep = Tl(mainLocale, "OInfoBlockPreview",
                                       qPrintable(options.value(QStringLiteral("separator")).toString()));
                lines.append(OCartouchePainter::TextLine(ComposeLine(display, value, sep), font, lineSpacing));
            }
            continue;
        }
        else if (type.Id() == QStringLiteral("pieceNumber"))
        {
            // Sample "1 of 1" / "01 of 01": the preview goes through the very same formatter as
            // the canvas, so the style shown here is the one the cartouche will render. The total
            // is an example — it is computed automatically from the pieces in the layout.
            const QString style = options.value(OPieceCartoucheNumber::kStyle).toString();
            line = ComposeLine(FormatPieceNumber(1, style), FormatPieceNumber(1, style), separator);
        }
        else if (type.Id() == QStringLiteral("cut"))
        {
            // One line per filled fabric tab; an empty tab is skipped (§4.3).
            struct CutTab
            {
                QString qtyKey;
                QString foldKey;
                const char *display;
            };
            static const CutTab kCutTabs[] = {
                {OPieceCartoucheCut::kFabricQty, OPieceCartoucheCut::kFabricOnFold,
                 QT_TRANSLATE_NOOP("OInfoBlockPreview", "fabric")},
                {OPieceCartoucheCut::kInterfacingQty, OPieceCartoucheCut::kInterfacingOnFold,
                 QT_TRANSLATE_NOOP("OInfoBlockPreview", "interfacing")},
                {OPieceCartoucheCut::kLiningQty, OPieceCartoucheCut::kLiningOnFold,
                 QT_TRANSLATE_NOOP("OInfoBlockPreview", "lining")},
            };

            bool any = false;
            // Software-managed: one line per filled tab AND per block language,
            // in combo order (e.g. "Cut 1 on fold (fabric)" / "Couper 1 fois au
            // pli (tissu)").
            for (const QString &locale : renderLanguages)
            {
                for (const CutTab &tab : kCutTabs)
                {
                    const int qty = options.value(tab.qtyKey, 0).toInt();
                    if (qty <= 0)
                    {
                        continue;  // empty tab → no line
                    }
                    any = true;
                    const bool onFold = options.value(tab.foldKey, false).toBool();
                    QString text = Tl(locale, "OInfoBlockPreview",
                                      QT_TRANSLATE_NOOP("OInfoBlockPreview", "Cut %1"))
                                       .arg(qty);
                    if (onFold)
                    {
                        text += Tl(locale, "OInfoBlockPreview",
                                   QT_TRANSLATE_NOOP("OInfoBlockPreview", " on fold"));
                    }
                    text += Tl(locale, "OInfoBlockPreview", QT_TRANSLATE_NOOP("OInfoBlockPreview", " (%1)"))
                                .arg(Tl(locale, "OInfoBlockPreview", tab.display));
                    lines.append(OCartouchePainter::TextLine(text, font, lineSpacing));
                }
            }
            if (!any)
            {
                continue;  // dead data: no fabric tab filled
            }
            continue;
        }
        else if (type.Id() == QStringLiteral("logo"))
        {
            const QString stored = options.value(QStringLiteral("logo")).toString();
            if (stored.isEmpty())
            {
                continue;  // dead data: no logo chosen
            }
            // Drawing the logo is the same six steps wherever it is done, so it is done in one
            // place — see OLogoRender. A preview that rasterised it its own way could come out
            // disagreeing with the export about how big a logo is, which is a preview that lies.
            const double sizeCm = options.value(QStringLiteral("logoSize"), 2.0).toDouble();
            const OLogoRender::Drawing drawing = OLogoRender::Draw(stored, sizeCm);
            if (drawing.pixmap.isNull())
            {
                continue;  // dead data: missing or invalid SVG
            }
            lines.append(OCartouchePainter::LogoLine(drawing.pixmap, drawing.heightPx, lineSpacing));
            continue;
        }

        lines.append(OCartouchePainter::TextLine(line, font, lineSpacing));
    }

    painter.setPen(palette().text().color());
    OCartouchePainter::Paint(painter, rect(), lines);
}
