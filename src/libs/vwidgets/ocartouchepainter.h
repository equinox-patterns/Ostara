/************************************************************************
 **
 **  @file   ocartouchepainter.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   31 August, 2026
 **
 **  @brief  Shared cartouche rendering formula — one stacked line per
 **          section, fonts from the template options. Used by the editor
 **          live preview and by the on-piece information blocks so the
 **          two always render identically.
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

#ifndef OCARTOUCHEPAINTER_H
#define OCARTOUCHEPAINTER_H

#include <QFont>
#include <QHash>
#include <QPainter>
#include <QPixmap>
#include <QRectF>
#include <QString>
#include <QSizeF>
#include <QVariant>
#include <QVector>
#include <QtGlobal>

/**
 * @brief One rendered line of an information block (text or logo).
 *
 * The font is fully styled (family / weight / pixel size) before being
 * stored; the painter only lays the line out.
 */
struct OCartoucheRenderLine
{
    enum class Kind
    {
        Text,
        Logo
    };

    Kind kind = Kind::Text;
    QString text;         // Kind::Text — the line text.
    QFont font;           // Kind::Text — fully styled font.
    int lineSpacing = 0;  // Kind::Text — extra px below the line (0 = default).
    QPixmap logo;         // Kind::Logo — pre-rendered logo pixmap.
    qreal logoHeight = 0; // Kind::Logo — rendered height in px.
};

/**
 * @brief Shared cartouche painting helpers.
 *
 * The line formula is the single source of truth for the editor preview and
 * the canvas blocks: line height = font height + 6px + line spacing option,
 * text centered horizontally in the block.
 */
namespace OCartouchePainter
{
constexpr qreal kMargin = 16.0;
constexpr qreal kLineGap = 6.0;

/** @brief Section font from the template options (fontFamily / fontWeight / fontSize). */
QFont SectionFont(const QHash<QString, QVariant> &options, const QFont &baseFont);

/** @brief Build a text render line. */
OCartoucheRenderLine TextLine(const QString &text, const QFont &font, int lineSpacing);

/** @brief Build a logo render line (pre-rendered pixmap). */
OCartoucheRenderLine LogoLine(const QPixmap &logo, qreal logoHeight, int lineSpacing);

/**
 * @brief Vertical advance of a text line: font height + (6px + line spacing) × scale.
 *
 * The font is expected to already carry the scale (pre-scaled pixel size); the
 * scale factor scales the fixed gap and the line-spacing option so a resized
 * block stays fully proportional.
 */
qreal LineHeight(const QFont &font, int lineSpacing, qreal scale = 1.0);

/** @brief Vertical advance of a logo line (scales the logo height and the gap). */
qreal LogoLineHeight(qreal logoHeight, int lineSpacing, qreal scale = 1.0);

/** @brief Natural size of a stack of lines (width = widest line + margins). */
QSizeF NaturalSize(const QVector<OCartoucheRenderLine> &lines);

/**
 * @brief Paint the lines stacked, each centered. Uses painter.pen() for text.
 * @param scale proportional content scale (block fontScale) — scales the
 *        inter-line gap, the line-spacing option and the logo draw size.
 */
void Paint(QPainter &painter, const QRectF &rect, const QVector<OCartoucheRenderLine> &lines, qreal scale = 1.0);
}  // namespace OCartouchePainter

#endif // OCARTOUCHEPAINTER_H
