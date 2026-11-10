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

#include <QFileInfo>
#include <QImage>
#include <QImageReader>
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

/**
 * @brief The vector logo @p path, drawn @p heightPx tall, its width from its own proportions.
 *
 * A file that declares no size of its own is taken as square, which is the only guess that keeps
 * it visible rather than a sliver.
 */
auto VectorPixmap(const QString &path, double heightPx) -> QPixmap
{
    QSvgRenderer renderer(path);
    if (!renderer.isValid())
    {
        return {}; // a file that has gone, or one that is not a drawing
    }

    const QSizeF intrinsic = renderer.defaultSize();
    double widthPx = heightPx;
    if (intrinsic.isValid() && intrinsic.width() > 0 && intrinsic.height() > 0)
    {
        widthPx = heightPx * (intrinsic.width() / intrinsic.height());
    }

    QPixmap pixmap(qCeil(widthPx), qCeil(heightPx));
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    renderer.render(&painter);
    painter.end();
    return pixmap;
}

/**
 * @brief The raster logo @p path, sampled @p heightPx tall, its width from its own proportions.
 *
 * **The proportions are read from the picture that came out, not from the file's header.** A
 * photograph is rotated by its EXIF tag as it is decoded, so the size the header announces is the
 * one lying on its side: a logo turned upright that way would have been given the width of its
 * height. Decoding first costs the full-size image in memory once, and it is the only way not to
 * be wrong about a photograph taken with a telephone.
 */
auto RasterPixmap(const QString &path, double heightPx) -> QPixmap
{
    QImageReader reader(path);
    // Whatever says which way is up — a scanner or a telephone — is obeyed.
    reader.setAutoTransform(true);

    const QImage image = reader.read();
    if (image.isNull() || image.width() <= 0 || image.height() <= 0)
    {
        return {}; // a file that has gone, or one this build cannot decode
    }

    const double widthPx = heightPx * (static_cast<double>(image.width()) / image.height());
    const QImage scaled = image.scaled(QSize(qCeil(widthPx), qCeil(heightPx)), Qt::IgnoreAspectRatio,
                                       Qt::SmoothTransformation);
    return QPixmap::fromImage(scaled);
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------

auto OLogoRender::IsVector(const QString &path) -> bool
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    // Empty counts as vector: the library held SVG alone long enough that no other reading of a
    // file without a suffix could be meant. `.svgz` is the compressed drawing, and it is drawn.
    return suffix.isEmpty() || suffix == QLatin1String("svg") || suffix == QLatin1String("svgz");
}

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
    const double heightPx = HeightFor(sizeCm);

    // One answer either way: a drawing that could not be made is an empty Drawing, never a pixmap
    // with no height beside it — the callers drop the line on `pixmap.isNull()` and read the height
    // only once it is not.
    const QPixmap pixmap = IsVector(path) ? VectorPixmap(path, heightPx) : RasterPixmap(path, heightPx);
    if (pixmap.isNull())
    {
        return drawing;
    }

    drawing.pixmap = pixmap;
    drawing.heightPx = heightPx;
    return drawing;
}
