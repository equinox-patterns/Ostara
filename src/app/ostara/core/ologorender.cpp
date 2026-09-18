/************************************************************************
 **
 **  @file   ologorender.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  A logo turned into something a painter can draw — the one place it is done.
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

#include "ologorender.h"

#include "ologolibrary.h"

#include "vmisc/def.h"

#include <QPainter>
#include <QSvgRenderer>
#include <QtMath>

namespace
{
/**
 * The floor a logo is never drawn smaller than, in pixels.
 *
 * A logo of a tenth of a millimetre is a mistake rather than a choice, and one drawn at a third
 * of a pixel would come out as nothing at all — which reads as « the logo is broken » rather
 * than as « the size is too small ». Eight pixels is small enough to be nobody's intent and big
 * enough to still be seen.
 */
constexpr double kMinimumHeightPx = 8.0;
} // namespace

//---------------------------------------------------------------------------------------------------------------------

auto OLogoRender::HeightFor(double sizeCm) -> double
{
    return (std::max)(kMinimumHeightPx, ToPixel(sizeCm, Unit::Cm));
}

//---------------------------------------------------------------------------------------------------------------------

auto OLogoRender::Draw(const QString &fileName, double sizeCm) -> Drawing
{
    Drawing drawing;
    if (fileName.isEmpty())
    {
        return drawing;
    }

    const QString path = OLogoLibrary::Directory() + QLatin1Char('/') + fileName;
    QSvgRenderer renderer(path);
    if (!renderer.isValid())
    {
        return drawing; // a file that has gone, or one that is not a drawing
    }

    drawing.heightPx = HeightFor(sizeCm);

    // The width follows the file's own proportions; a file that declares no size of its own is
    // taken as square, which is the only guess that keeps it visible rather than a sliver.
    const QSizeF intrinsic = renderer.defaultSize();
    double widthPx = drawing.heightPx;
    if (intrinsic.isValid() && intrinsic.width() > 0 && intrinsic.height() > 0)
    {
        widthPx = drawing.heightPx * (intrinsic.width() / intrinsic.height());
    }

    drawing.pixmap = QPixmap(qCeil(widthPx), qCeil(drawing.heightPx));
    drawing.pixmap.fill(Qt::transparent);

    QPainter painter(&drawing.pixmap);
    renderer.render(&painter);
    painter.end();

    return drawing;
}
