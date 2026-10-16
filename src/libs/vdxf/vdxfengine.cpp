/************************************************************************
**
**  @file   vdxfengine.cpp
**  @author Valentina Zhuravska <zhuravska19(at)gmail.com>
**  @date   12 8, 2015
**
**  @brief
**  @copyright
**  This source code is part of the Valentina project, a pattern making
**  program, whose allow create and modeling patterns of clothing.
**  Copyright (C) 2013-2015 Valentina project
**  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
**
**  Valentina is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  Valentina is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
**
*************************************************************************/

#include "vdxfengine.h"

#include <QApplication>
#include <QByteArray>
#include <QColor>
#include <QDateTime>
#include <QFlag>
#include <QFlags>
#include <QFont>
#include <QLineF>
#include <QList>
#include <QLoggingCategory>
#include <QMessageLogger>
#include <QPaintEngineState>
#include <QPainterPath>
#include <QPen>
#include <QPolygonF>
#include <QTextItem>
#include <QtDebug>
#include <QtMath>
#include <memory>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include "../vmisc/vtextcodec.h"
using QTextCodec = VTextCodec;
#else
#include <QTextCodec>
#endif // QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

#include "../vgeometry/vgeometrydef.h"
#include "../vgeometry/vlayoutplacelabel.h"
#include "../vlayout/vfoldline.h"
#include "../vlayout/vlayoutpiece.h"
#include "../vlayout/vlayoutpiecepath.h"
#include "../vlayout/vlayoutpoint.h"
#include "../vmisc/def.h"
#include "dxiface.h"
#include "libdxfrw/drw_entities.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wmissing-prototypes")
QT_WARNING_DISABLE_INTEL(1418)

Q_LOGGING_CATEGORY(vDxf, "v.undo") // NOLINT

QT_WARNING_POP

namespace
{
const qreal AAMATextHeight = 2.5;

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wunused-member-function")

Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer0, (UTF8STRING("0"))) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer1, (UTF8STRING("1"))) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer2, (UTF8STRING("2"))) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer3, (UTF8STRING("3"))) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer4, (UTF8STRING("4"))) // NOLINT
// Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer5, (UTF8STRING("5"))) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer6, (UTF8STRING("6"))) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer7, (UTF8STRING("7"))) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer8, (UTF8STRING("8"))) // NOLINT
// Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer9, (UTF8STRING("9"))) // NOLINT
// Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer10, (UTF8STRING("10"))) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer11, (UTF8STRING("11"))) // NOLINT
// Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer12, (UTF8STRING("12"))) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer13, (UTF8STRING("13"))) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer14, (UTF8STRING("14"))) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer15, (UTF8STRING("15"))) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer19, (UTF8STRING("19"))) // NOLINT
// Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer26, (UTF8STRING("26"))) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer80, (UTF8STRING("80"))) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer81, (UTF8STRING("81"))) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer82, (UTF8STRING("82"))) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer83, (UTF8STRING("83"))) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer84, (UTF8STRING("84"))) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer85, (UTF8STRING("85"))) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer86, (UTF8STRING("86"))) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const UTF8STRING, layer87, (UTF8STRING("87"))) // NOLINT

QT_WARNING_POP

//---------------------------------------------------------------------------------------------------------------------
inline auto svgEngineFeatures() -> QPaintEngine::PaintEngineFeatures
{
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_CLANG("-Wsign-conversion")
    QT_WARNING_DISABLE_INTEL(68)

    return {QPaintEngine::AllFeatures & ~QPaintEngine::PatternBrush & ~QPaintEngine::PerspectiveTransform
            & ~QPaintEngine::ConicalGradientFill & ~QPaintEngine::PorterDuff};

    QT_WARNING_POP
}
} // namespace


//---------------------------------------------------------------------------------------------------------------------
VDxfEngine::VDxfEngine()
  : QPaintEngine(svgEngineFeatures()),
    m_textBuffer(new DRW_Text())
{
}

//---------------------------------------------------------------------------------------------------------------------
VDxfEngine::~VDxfEngine()
{
    delete m_textBuffer;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::begin(QPaintDevice *pdev) -> bool
{
    Q_UNUSED(pdev)

    if (isActive())
    {
        qCWarning(vDxf) << qUtf8Printable("VDxfEngine::begin(), the engine was alredy activated"_L1);
        return false;
    }

    if (not m_size.isValid())
    {
        qCWarning(vDxf) << qUtf8Printable("VDxfEngine::begin(), size is not valid"_L1);
        return false;
    }

    m_input = QSharedPointer<dx_iface>(
        new dx_iface(m_fileName.toUtf8().toStdString(), m_version, m_varMeasurement, m_varInsunits));
    m_input->AddDefHeaderData();
    m_input->AddQtLTypes();
    m_input->AddDefLayers();
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::end() -> bool
{
    return m_input->fileExport(m_binary);
}

//---------------------------------------------------------------------------------------------------------------------
// cppcheck-suppress unusedFunction
void VDxfEngine::updateState(const QPaintEngineState &state)
{
    QPaintEngine::DirtyFlags flags = state.state();

    // always stream full gstate, which is not required, but...
    flags |= QPaintEngine::AllDirty;

    if (flags & QPaintEngine::DirtyTransform)
    {
        m_matrix = state.transform(); // Save new matrix for moving paths
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::drawPath(const QPainterPath &path)
{
    const QList<QPolygonF> subpaths = path.toSubpathPolygons(m_matrix);

    for (const auto &polygon : subpaths)
    {
        if (polygon.isEmpty())
        {
            continue;
        }

        if (m_version > DRW::AC1009)
        { // Use lwpolyline
            auto *poly = new DRW_LWPolyline();
            poly->layer = *layer0;
            poly->color = GetPenColor();
            poly->lWeight = DRW_LW_Conv::widthByLayer;
            poly->lineType = GetPenStyle();

            if (polygon.size() > 1 && polygon.constFirst() == polygon.constLast())
            {
                poly->flags |= 0x1; // closed NOLINT(hicpp-signed-bitwise)
            }

            poly->flags |= 0x80; // plinegen NOLINT(hicpp-signed-bitwise)

            for (auto p : polygon)
            {
                poly->addVertex(DRW_Vertex2D(FromPixel(p.x(), m_varInsunits),
                                             FromPixel(GetSize().height() - p.y(), m_varInsunits), 0));
            }

            m_input->AddEntity(poly);
        }
        else
        { // Use polyline
            auto *poly = new DRW_Polyline();
            poly->layer = *layer0;
            poly->color = GetPenColor();
            poly->lWeight = DRW_LW_Conv::widthByLayer;
            poly->lineType = GetPenStyle();
            if (polygon.size() > 1 && polygon.constFirst() == polygon.constLast())
            {
                poly->flags |= 0x1; // closed NOLINT(hicpp-signed-bitwise)
            }

            poly->flags |= 0x80; // plinegen NOLINT(hicpp-signed-bitwise)

            for (auto p : polygon)
            {
                poly->addVertex(DRW_Vertex(FromPixel(p.x(), m_varInsunits),
                                           FromPixel(GetSize().height() - p.y(), m_varInsunits), 0, 0));
            }

            m_input->AddEntity(poly);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::drawLines(const QLineF *lines, int lineCount)
{
    for (int i = 0; i < lineCount; ++i)
    {
        const QPointF p1 = m_matrix.map(lines[i].p1()); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        const QPointF p2 = m_matrix.map(lines[i].p2()); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

        auto *line = new DRW_Line();
        line->basePoint =
            DRW_Coord(FromPixel(p1.x(), m_varInsunits), FromPixel(GetSize().height() - p1.y(), m_varInsunits), 0);
        line->secPoint =
            DRW_Coord(FromPixel(p2.x(), m_varInsunits), FromPixel(GetSize().height() - p2.y(), m_varInsunits), 0);
        line->layer = *layer0;
        line->color = GetPenColor();
        line->lWeight = DRW_LW_Conv::widthByLayer;
        line->lineType = GetPenStyle();

        m_input->AddEntity(line);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode)
{
    Q_UNUSED(mode)

    if (pointCount <= 0)
    {
        return;
    }

    if (m_version > DRW::AC1009)
    { // Use lwpolyline
        auto *poly = new DRW_LWPolyline();
        poly->layer = *layer0;
        poly->color = GetPenColor();
        poly->lWeight = DRW_LW_Conv::widthByLayer;
        poly->lineType = GetPenStyle();

        if (pointCount > 1 &&
            points[0] == points[pointCount]) // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        {
            poly->flags |= 0x1; // closed NOLINT(hicpp-signed-bitwise)
        }

        poly->flags |= 0x80; // plinegen NOLINT(hicpp-signed-bitwise)

        for (int i = 0; i < pointCount; ++i)
        {
            const QPointF p = m_matrix.map(points[i]); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            poly->addVertex(
                DRW_Vertex2D(FromPixel(p.x(), m_varInsunits), FromPixel(GetSize().height() - p.y(), m_varInsunits), 0));
        }

        m_input->AddEntity(poly);
    }
    else
    { // Use polyline
        auto *poly = new DRW_Polyline();
        poly->layer = *layer0;
        poly->color = GetPenColor();
        poly->lWeight = DRW_LW_Conv::widthByLayer;
        poly->lineType = GetPenStyle();

        if (pointCount > 1 &&
            points[0] == points[pointCount]) // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        {
            poly->flags |= 0x1; // closed NOLINT(hicpp-signed-bitwise)
        }

        poly->flags |= 0x80; // plinegen NOLINT(hicpp-signed-bitwise)

        for (int i = 0; i < pointCount; ++i)
        {
            const QPointF p = m_matrix.map(points[i]); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            poly->addVertex(DRW_Vertex(FromPixel(p.x(), m_varInsunits),
                                       FromPixel(GetSize().height() - p.y(), m_varInsunits), 0, 0));
        }

        m_input->AddEntity(poly);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::drawEllipse(const QRectF &rect)
{
    const QRectF newRect = m_matrix.mapRect(rect);
    const double rotationAngle = atan(m_matrix.m12() / m_matrix.m11());

    // distanse between center and endpoint of the major axis
    double majorX = 0;
    double majorY = 0;
    double ratio = 0; // ratio of minor axis to major axis
    if (rect.width() <= rect.height())
    {
        majorX = (rect.top() - rect.center().y()) * sin(rotationAngle) * m_matrix.m11() / cos(rotationAngle);
        // major axis * sin(rotation angle) * x-scale-factor
        majorY = (rect.top() - rect.center().y()) * m_matrix.m22();
        // major axis * cos(rotation angle) * y-scale-factor, where y-scale-factor = matrix.m22()/cos(rotationAngle)
        ratio = rect.width() / rect.height();
    }
    else
    {
        majorX = (rect.right() - rect.center().x()) * m_matrix.m11();
        // major axis * cos(rotation angle) * x-scale-factor, where y-scale-factor = matrix.m22()/cos(rotationAngle)
        majorY = (rect.right() - rect.center().x()) * sin(rotationAngle) * m_matrix.m22() / cos(rotationAngle);
        // major axis * sin(rotation angle) * y-scale-factor
        ratio = rect.height() / rect.width();
    }

    auto *ellipse = new DRW_Ellipse();
    ellipse->basePoint = DRW_Coord(FromPixel(newRect.center().x(), m_varInsunits),
                                   FromPixel(GetSize().height() - newRect.center().y(), m_varInsunits), 0);
    ellipse->secPoint = DRW_Coord(FromPixel(majorX, m_varInsunits), FromPixel(majorY, m_varInsunits), 0);
    ellipse->ratio = ratio;
    ellipse->staparam = 0;
    ellipse->endparam = 2 * M_PI;

    ellipse->layer = *layer0;
    ellipse->color = GetPenColor();
    ellipse->lWeight = DRW_LW_Conv::widthByLayer;
    ellipse->lineType = GetPenStyle();

    m_input->AddEntity(ellipse);
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::drawTextItem(const QPointF &p, const QTextItem &textItem)
{
    if (m_textBuffer->text.empty())
    {
        const QPointF startPoint = m_matrix.map(p);
        const double rotationAngle = qRadiansToDegrees(qAtan2(m_matrix.m12(), m_matrix.m11()));

        const QFont f = textItem.font();
        const UTF8STRING fontStyle = m_input->AddFont(f);

        m_textBuffer->basePoint = DRW_Coord(FromPixel(startPoint.x(), m_varInsunits),
                                            FromPixel(GetSize().height() - startPoint.y(), m_varInsunits), 0);
        m_textBuffer->secPoint = DRW_Coord(FromPixel(startPoint.x(), m_varInsunits),
                                           FromPixel(GetSize().height() - startPoint.y(), m_varInsunits), 0);
        m_textBuffer->height = FromPixel(QFontMetrics(f).height(), m_varInsunits);

        m_textBuffer->style = fontStyle;
        m_textBuffer->angle = -rotationAngle;

        m_textBuffer->layer = *layer0;
        m_textBuffer->color = GetPenColor();
        m_textBuffer->lWeight = DRW_LW_Conv::widthByLayer;
        m_textBuffer->lineType = GetPenStyle();
    }

    /* Because QPaintEngine::drawTextItem doesn't pass whole string per time we mark end of each string by adding
     * special placholder. */
    QString t = textItem.text();
    const bool foundEndOfString = t.contains(endStringPlaceholder);

    if (foundEndOfString)
    {
        t.replace(endStringPlaceholder, QString());
    }

    m_textBuffer->text += t.toStdString();

    if (foundEndOfString)
    {
        m_input->AddEntity(m_textBuffer);
        m_textBuffer = new DRW_Text();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::type() const -> QPaintEngine::Type
{
    return QPaintEngine::User;
}

//---------------------------------------------------------------------------------------------------------------------
// cppcheck-suppress unusedFunction
void VDxfEngine::drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr)
{
    Q_UNUSED(r)
    Q_UNUSED(pm)
    Q_UNUSED(sr)
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::GetSize() const -> QSize
{
    return m_size;
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::SetSize(const QSize &value)
{
    Q_ASSERT(not isActive());
    m_size = value;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::GetResolution() const -> double
{
    return m_resolution;
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::SetResolution(double value)
{
    Q_ASSERT(not isActive());
    m_resolution = value;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::GetFileName() const -> QString
{
    return m_fileName;
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::SetFileName(const QString &value)
{
    Q_ASSERT(not isActive());
    m_fileName = value;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::GetVersion() const -> DRW::Version
{
    return m_version;
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::SetVersion(DRW::Version version)
{
    Q_ASSERT(not isActive());
    m_version = version;
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::SetBinaryFormat(bool binary)
{
    m_binary = binary;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::IsBinaryFormat() const -> bool
{
    return m_binary;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::GetPenStyle() -> std::string
{
    switch (state->pen().style())
    {
        case Qt::DashLine:
            return "DASHED";
        case Qt::DotLine:
            return "DOT";
        case Qt::DashDotLine:
            return "DASHDOT2";
        case Qt::DashDotDotLine:
            return "DIVIDE2";
        case Qt::SolidLine:
        default:
            return "BYLAYER";
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::GetPenColor() -> int
{
    QColor const color = state->pen().color();

    if (color == Qt::black)
    {
        return DRW::black;
    }

    if (color == Qt::white)
    {
        return DRW::white;
    }

    if (color == Qt::darkGray)
    {
        return DRW::gray;
    }

    if (color == Qt::gray)
    {
        return DRW::l_gray;
    }

    if (color == Qt::darkMagenta)
    {
        return DRW::magenta;
    }

    if (color == Qt::magenta)
    {
        return DRW::l_magenta;
    }

    if (color == Qt::cyan)
    {
        return DRW::l_cyan;
    }

    if (color == Qt::darkCyan)
    {
        return DRW::cyan;
    }

    if (color == Qt::blue)
    {
        return DRW::l_blue;
    }

    if (color == Qt::darkBlue)
    {
        return DRW::blue;
    }

    if (color == Qt::darkGreen)
    {
        return DRW::green;
    }

    if (color == Qt::green)
    {
        return DRW::l_green;
    }

    if (color == Qt::darkRed)
    {
        return DRW::red;
    }

    if (color == Qt::red)
    {
        return DRW::l_red;
    }

    if (color == Qt::yellow)
    {
        return DRW::yellow;
    }

    return DRW::ColorByLayer;
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::SetMeasurement(const VarMeasurement &var)
{
    Q_ASSERT(not isActive());
    m_varMeasurement = var;
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::SetInsunits(const VarInsunits &var)
{
    Q_ASSERT(not isActive());
    m_varInsunits = var;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::GetXScale() const -> qreal
{
    return m_xscale;
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::SetXScale(const qreal &xscale)
{
    Q_ASSERT(not isActive());
    m_xscale = xscale;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::GetYScale() const -> qreal
{
    return m_yscale;
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::SetYScale(const qreal &yscale)
{
    Q_ASSERT(not isActive());
    m_yscale = yscale;
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::SetBoundaryTogetherWithNotches(bool value)
{
    Q_ASSERT(not isActive());
    m_togetherWithNotches = value;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::IsBoundaryTogetherWithNotches() const -> bool
{
    return m_togetherWithNotches;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::DxfApparelCompatibility() const -> DXFApparelCompatibility
{
    return m_compatibilityMode;
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::SetDxfApparelCompatibility(DXFApparelCompatibility mode)
{
    m_compatibilityMode = mode;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::IsShowMainPath() const -> bool
{
    return m_showMainPath;
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::SetShowMainPath(bool value)
{
    m_showMainPath = value;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::ErrorString() const -> QString
{
    return QString::fromStdString(m_input->ErrorString());
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::FromPixel(double pix, const VarInsunits &unit) const -> double
{
    switch (unit)
    {
        case VarInsunits::Millimeters:
            return pix / m_resolution * 25.4;
        case VarInsunits::Centimeters:
            return pix / m_resolution * 25.4 / 10.0;
        case VarInsunits::Inches:
            return pix / m_resolution;
        default:
            return pix;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::ToPixel(double val, const VarInsunits &unit) const -> double
{
    switch (unit)
    {
        case VarInsunits::Millimeters:
            return (val / 25.4) * m_resolution;
        case VarInsunits::Centimeters:
            return ((val * 10.0) / 25.4) * m_resolution;
        case VarInsunits::Inches:
            return val * m_resolution;
        default:
            return val;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::ExportToAAMA(const QVector<VLayoutPiece> &details) -> bool
{
    if (not m_size.isValid())
    {
        qCWarning(vDxf) << qUtf8Printable("VDxfEngine::ExportToAAMA(), size is not valid"_L1);
        return false;
    }

    m_input = QSharedPointer<dx_iface>::create(m_fileName.toUtf8().toStdString(),
                                               m_version,
                                               m_varMeasurement,
                                               m_varInsunits);
    m_input->AddXSpaceBlock(false);
    m_input->AddAAMAHeaderData();
    if (m_version > DRW::AC1009)
    {
        m_input->AddDefLayers();
    }
    m_input->AddAAMALayers();

    for (auto detail : details)
    {
        // Use custom deleter function to lose ownership after adding the block
        bool deleteBlock = true;
        auto NoOpDeleter = [&deleteBlock](dx_ifaceBlock *block)
        {
            if (deleteBlock)
            {
                delete block;
            }
        };

        auto detailBlock = QSharedPointer<dx_ifaceBlock>(new dx_ifaceBlock, NoOpDeleter);

        QString blockName = detail.GetName();
        if (m_version <= DRW::AC1009)
        {
            blockName.replace(' '_L1, '_'_L1);
        }

        detailBlock->name = blockName.toStdString();
        detailBlock->flags = 64;
        detailBlock->layer = *layer1;

        detail.Scale(m_xscale, m_yscale);

        if (m_showMainPath)
        {
            // Export option: write the main path (the sew line) even for a piece that hides it, and send
            // that sew line to its own layer (14) rather than to the Draw line (8). 'detail' is a copy,
            // so neither the pattern nor the canvas is touched.
            detail.SetHideMainPath(false);
            detail.SetSewLineOnDrawing(false);
        }

        ExportAAMAOutline(detailBlock, detail);
        ExportAAMADraw(detailBlock, detail);
        ExportAAMAIntcut(detailBlock, detail);
        ExportAAMANotch(detailBlock, detail);
        ExportAAMAGrainline(detailBlock, detail);
        ExportAAMADrill(detailBlock, detail);
        ExportAnnotationText(detailBlock, detail, *layer19);

        m_input->AddBlock(detailBlock.data());

        auto insert = std::make_unique<DRW_Insert>();
        insert->name = blockName.toStdString();
        insert->layer = *layer1;

        m_input->AddEntity(insert.release());

        deleteBlock = false; // lose ownership
    }

    return m_input->fileExport(m_binary);
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::ExportAAMAOutline(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail)
{
    QVector<VLayoutPoint> points = PieceOutline(detail);

    if (m_togetherWithNotches)
    {
        points = VAbstractPiece::RemoveDublicates(points, false);
    }

    if (DRW_Entity *e = AAMAPolygon(points, *layer1, true))
    {
        detailBlock->ent.push_back(e);
    }

    ExportTurnAndCurvePoints(detailBlock, points);
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::ExportAAMADraw(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail)
{
    ExportAAMADrawSewLine(detailBlock, detail);
    ExportAAMADrawInternalPaths(detailBlock, detail);
    ExportAAMADrawPlaceLabels(detailBlock, detail);
    ExportAAMADrawFoldLine(detailBlock, detail);

    if (!detail.IsShowFullPiece())
    {
        return;
    }

    if (QLineF const mirrorLine = detail.GetMappedCorrectedMirrorLine(m_togetherWithNotches);
        not mirrorLine.isNull() && detail.IsShowMirrorLine())
    {
        if (DRW_Entity *e = AAMALine(mirrorLine, *layer8))
        {
            e->lineType = dx_iface::QtPenStyleToString(Qt::DashDotLine);
            detailBlock->ent.push_back(e);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::ExportAAMADrawSewLine(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail)
{
    // The sew line is pattern data, not drawing data: hiding the main path is a display option and must
    // not remove the sew line from the DXF. Style3D rebuilds the couturage from it — without it the piece
    // has nothing to stitch.
    //
    // ⚠️ **La règle du 15-10-2026 — « une pièce sans valeur de couture reste sans calque 14 » — tient,
    // sauf en Clo3D/Style3D, qui est un mode de compatibilité.** Là, la pièce n'a plus de marge DU
    // TOUT : son contour de coupe EST son couturage, et le calque 1 le porte déjà. Mais Style3D lit la
    // couture sur le 14, et son propre DXF écrit les deux calques avec la même courbe — mesuré sur son
    // fichier de référence : douze polylignes fermées de part et d'autre, les mêmes à 0,1 mm près. En
    // mode Clo3D la couture s'écrit donc aussi, et sans marge elle se confond avec le contour.
    if (!detail.IsSeamAllowance() && !IsClo3DCompatibility())
    {
        return;
    }

    // ⚠️ **Le contour de la couture porte les mêmes découpes que celui du calque 1** — voir
    // ContourWithCutPaths : une pince coupée appartient aux deux, sans quoi la pièce serait coupée
    // d'un côté et cousue de l'autre.
    QVector<VLayoutPoint> const points =
        ContourWithCutPaths(detail, detail.GetMappedFullContourPoints(m_togetherWithNotches, true, false));
    if (points.isEmpty())
    {
        return;
    }

    // Sans valeur de couture en mode Clo3D, le contour EST le couturage : il va sur le 14 quel que soit
    // le réglage d'affichage de la pièce — c'est le calque que Style3D lit, et un contour envoyé sur le
    // 8 lui laisserait une pièce sans couture.
    const bool onDrawing = detail.IsSewLineOnDrawing() && (detail.IsSeamAllowance() || !IsClo3DCompatibility());

    if (const UTF8STRING &layer = not onDrawing ? *layer14 : *layer8;
        DRW_Entity *e = AAMAPolygon(points, layer, true))
    {
        detailBlock->ent.push_back(e);
    }

    // No turn or curve point here: layers 2 and 3 describe the outline of the piece, which is the
    // boundary written on layer 1. Writing the sew line points there as well puts points 10 mm away from
    // the outline into the very same layers, and a reader that rebuilds the edge mixes them: the piece
    // then comes back with a couple of points and a deformed seam (a collar, typically).
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::ExportAAMADrawInternalPaths(const QSharedPointer<dx_ifaceBlock> &detailBlock,
                                             const VLayoutPiece &detail)
{
    auto DrawPolygon = [this, detailBlock](const QVector<VLayoutPoint> &points, Qt::PenStyle style, bool forceClosed)
    {
        if (DRW_Entity *e = AAMAPolygon(points, *layer8, forceClosed))
        {
            e->lineType = dx_iface::QtPenStyleToString(style);
            detailBlock->ent.push_back(e);
        }

        ExportTurnAndCurvePoints(detailBlock, points);
    };

    const QVector<VLayoutPiecePath> drawIPaths = InternalPaths(detail, false);
    for (const auto &iPath : drawIPaths)
    {
        QVector<VLayoutPoint> points = InternalPathPoints(iPath);
        DrawPolygon(points, iPath.PenStyle(), false);

        if (!iPath.IsNotMirrored() && detail.IsShowFullPiece() && !detail.GetMappedSeamMirrorLine().isNull())
        {
            const QTransform matrix = VGObject::FlippingMatrix(detail.GetMappedSeamMirrorLine());
            std::transform(points.begin(), points.end(), points.begin(),
                           [&matrix](const VLayoutPoint &point) { return VAbstractPiece::MapPoint(point, matrix); });
            DrawPolygon(points, iPath.PenStyle(), false);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::ExportAAMADrawPlaceLabels(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail)
{
    auto DrawShape = [this, detailBlock](const PlaceLabelImg &shape, bool forceClosed)
    {
        for (const auto &points : shape)
        {
            if (DRW_Entity *e = AAMAPolygon(points, *layer8, forceClosed))
            {
                detailBlock->ent.push_back(e);
            }

            ExportTurnAndCurvePoints(detailBlock, points);
        }
    };

    const QVector<VLayoutPlaceLabel> labels = detail.GetPlaceLabels();
    for (const auto &label : labels)
    {
        if (label.Type() != PlaceLabelType::Doubletree && label.Type() != PlaceLabelType::Button &&
            label.Type() != PlaceLabelType::Circle)
        {
            DrawShape(detail.MapPlaceLabelShape(VAbstractPiece::PlaceLabelShape(label)), false);

            if (!label.IsNotMirrored() && detail.IsShowFullPiece() && !detail.GetMappedSeamMirrorLine().isNull())
            {
                PlaceLabelImg shape = detail.MapPlaceLabelShape(VAbstractPiece::PlaceLabelShape(label));
                const QTransform matrix = VGObject::FlippingMatrix(detail.GetMappedSeamMirrorLine());
                for (auto &points : shape)
                {
                    std::transform(points.begin(), points.end(), points.begin(),
                                   [&matrix](const VLayoutPoint &point)
                                   { return VAbstractPiece::MapPoint(point, matrix); });
                }

                DrawShape(shape, false);
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::ExportAAMAIntcut(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail)
{
    // ── Le calque d'une découpe qui n'entre PAS dans le contour ──────────────────────────────────
    //
    // En AAMA un « Cut on fabric » est un chemin INTERNE : il s'écrit sur le 11. En mode Clo3D/Style3D
    // il s'écrit sur le 1 — mais **seulement s'il n'est pas entré dans le contour** : une découpe dont
    // les deux extrémités tombent sur le contour est une **pince coupée**, et c'est la polyligne du
    // contour qui la porte (voir ContourWithCutPaths). Elle est donc écartée ici, sans quoi le fichier
    // la porterait deux fois.
    //
    // ⚠️ Ses points de retournement et de courbe suivent (voir `ExportTurnAndCurvePoints` plus bas) :
    // la découpe qui reste une entité à part s'écrit avec les points du contour, comme avant.
    const UTF8STRING &cutLayer = IsClo3DCompatibility() ? *layer1 : *layer11;

    auto DrawPolygon = [this, detailBlock, &cutLayer](const QVector<VLayoutPoint> &points, bool forceClosed)
    {
        if (DRW_Entity *e = AAMAPolygon(points, cutLayer, forceClosed))
        {
            detailBlock->ent.push_back(e);
        }

        ExportTurnAndCurvePoints(detailBlock, points);
    };

    // Les chemins BRUTS : `InternalPaths` efface les drapeaux d'accroche au contour en préparant une
    // pince à tracer, et c'est justement sur eux que se décide la fusion.
    QVector<VLayoutPiecePath> const drawIntCut = detail.MappedInternalPathsForCut(true);
    for (const auto &intCut : drawIntCut)
    {
        if (IsMergedInContour(detail, intCut))
        {
            continue;
        }

        QVector<VLayoutPoint> points = InternalPathPoints(intCut);
        DrawPolygon(points, false);

        if (!intCut.IsNotMirrored() && detail.IsShowFullPiece() && !detail.GetMappedSeamMirrorLine().isNull())
        {
            const QTransform matrix = VGObject::FlippingMatrix(detail.GetMappedSeamMirrorLine());
            std::transform(points.begin(), points.end(), points.begin(),
                           [&matrix](const VLayoutPoint &point) { return VAbstractPiece::MapPoint(point, matrix); });
            DrawPolygon(points, false);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::ExportAAMANotch(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail)
{
    auto ExportNotch = [this, detailBlock](QPointF center, qreal length, qreal angle)
    {
        auto notch = std::make_unique<DRW_ASTMNotch>();

        notch->basePoint =
            DRW_Coord(FromPixel(center.x(), m_varInsunits), FromPixel(GetSize().height() - center.y(), m_varInsunits),
                      FromPixel(length, m_varInsunits));
        notch->angle = angle;
        notch->layer = *layer4;

        detailBlock->ent.push_back(notch.release());
    };

    // **Un cran s'écrit qu'il y ait une valeur de couture ou non** (décidé le 15-10-2026) : sans elle,
    // le cran est porté par le main path (voir VLayoutPiece::ConvertPassmarks) et **se pose sur la
    // ligne de coupe** — la marge avec, le main path sans — c'est ce que le mode Details montre, et le
    // fichier est ce que l'écran montre.
    //
    // L'autre moitié de la condition, elle, ne bouge pas : `Boundary together with notches` demande que
    // les crans soient DANS la polyligne du contour, donc pas d'entités de cran à côté.
    if (!m_togetherWithNotches)
    {
        const QLineF mirrorLine = detail.GetMappedSeamMirrorLine();
        const QVector<VLayoutPassmark> passmarks = detail.GetMappedPassmarks();
        for (const auto &passmark : passmarks)
        {
            ExportNotch(passmark.baseLine.p1(), passmark.baseLine.length(), passmark.baseLine.angle());

            if (!mirrorLine.isNull() && detail.IsShowFullPiece()
                && !IsPointOnLineviaPDP(passmark.baseLine.p1(), mirrorLine.p1(), mirrorLine.p2())
                && !passmark.notMirrored)
            {
                const QTransform matrix = VGObject::FlippingMatrix(mirrorLine);
                QLineF const baseLine = matrix.map(passmark.baseLine);
                ExportNotch(baseLine.p1(), baseLine.length(), baseLine.angle());
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::ExportAAMAGrainline(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail)
{
    const QLineF grainlineMainLine = detail.GetMappedGrainlineMainLine();
    if (not grainlineMainLine.isNull())
    {
        if (DRW_Entity *e = AAMALine(grainlineMainLine, *layer7))
        {
            detailBlock->ent.push_back(e);
        }
    }
}

void VDxfEngine::ExportAnnotationText(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail,
                                      const UTF8STRING &layer)
{
    VFoldLine const fLine = detail.FoldLine();

    // A fold type that draws no text has nothing to export here. Stated explicitly because this
    // function is called for every piece and its only other guard is LabelPosition's flag — that
    // is what used to let a default "Cut On Fold" text slip into files for pieces marked "No text"
    // or "Invisible fold line".
    if (not fLine.HasLabel())
    {
        return;
    }

    bool ok = false;
    FoldLabelPosData const labelData = fLine.LabelPosition(ok);

    if (not ok)
    {
        return;
    }

    QFontMetrics const fm(labelData.font);
    qreal const height = fm.height() * qMin(detail.GetXScale(), detail.GetYScale());

    // One TEXT entity per line: a DXF text is a single line by nature, so a caption that wraps —
    // because it carries several languages, or because the fold line is short — is several
    // entities. Nothing else changes: each line is laid out exactly as the single one used to be.
    for (const FoldLabelLine &line : labelData.lines)
    {
        QPointF pos = line.pos;
        qreal const width = fm.horizontalAdvance(line.text);

        QLineF base(pos, QPointF(pos.x() + 100, pos.y()));
        base.setAngle(base.angle() - 180);
        base.setLength(height);
        pos = base.p2();

        QTransform matrix;
        if ((detail.IsVerticallyFlipped() && !detail.IsHorizontallyFlipped()) ||
            (!detail.IsVerticallyFlipped() && detail.IsHorizontallyFlipped()))
        {
            matrix.translate(pos.x(), pos.y());
            matrix.rotate(180);
            matrix.translate(-pos.x(), -pos.y());
            matrix.translate(-width, fm.height());
        }

        matrix *= detail.GetMatrix();

        QLineF angleLine(QPointF(), QPointF(1000, 0));
        angleLine.setAngle(labelData.angle);
        angleLine = matrix.map(angleLine);

        const QPointF textPos = matrix.map(pos);

        auto *text = new DRW_Text();
        text->basePoint = DRW_Coord(FromPixel(textPos.x(), m_varInsunits),
                                    FromPixel(GetSize().height() - textPos.y(), m_varInsunits), 0);
        text->secPoint = DRW_Coord(FromPixel(textPos.x(), m_varInsunits),
                                   FromPixel(GetSize().height() - textPos.y(), m_varInsunits), 0);
        text->height = FromPixel(height / 2, m_varInsunits);
        text->layer = layer;
        text->text = line.text.toStdString();
        text->style = m_input->AddFont(labelData.font);
        text->angle = angleLine.angle();

        detailBlock->ent.push_back(text);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::ExportAAMADrill(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail)
{
    const QVector<VLayoutPlaceLabel> labels = detail.GetPlaceLabels();

    for (const auto &label : labels)
    {
        if (label.Type() == PlaceLabelType::Doubletree || label.Type() == PlaceLabelType::Button ||
            label.Type() == PlaceLabelType::Circle)
        {
            QPointF center = detail.GetMatrix().map(label.Center());
            detailBlock->ent.push_back(AAMAPoint(center, *layer13));

            if (!label.IsNotMirrored() && detail.IsShowFullPiece() && !detail.GetMappedSeamMirrorLine().isNull())
            {
                const QTransform matrix = VGObject::FlippingMatrix(detail.GetMappedSeamMirrorLine());
                center = VAbstractPiece::MapPoint(center, matrix);
                detailBlock->ent.push_back(AAMAPoint(center, *layer13));
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::ExportAAMADrawFoldLine(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail)
{
    if (detail.IsShowFullPiece() && !detail.IsShowMirrorLine())
    {
        return;
    }

    VFoldLine const fLine = detail.FoldLine();
    QVector<QVector<QPointF>> points = fLine.FoldLineMarkPoints();
    if (points.isEmpty())
    {
        return;
    }

    switch (detail.GetFoldLineType())
    {
        case FoldLineType::TwoArrows:
        case FoldLineType::TwoArrowsTextAbove:
            points.removeAt(1);
            AAMADrawFoldLineTwoArrows(points, detailBlock);
            break;
        case FoldLineType::LAST_ONE_DO_NOT_USE:
            Q_UNREACHABLE();
            break;
        case FoldLineType::None:
        default:
            break;
    };
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::ExportToASTM(const QVector<VLayoutPiece> &details) -> bool
{
    if (not m_size.isValid())
    {
        qCWarning(vDxf) << qUtf8Printable("VDxfEngine::ExportToASTM(), size is not valid"_L1);
        return false;
    }

    m_input = QSharedPointer<dx_iface>(
        new dx_iface(m_fileName.toUtf8().toStdString(), m_version, m_varMeasurement, m_varInsunits));
    m_input->AddXSpaceBlock(false);
    m_input->AddAAMAHeaderData();
    if (m_version > DRW::AC1009)
    {
        m_input->AddDefLayers();
    }
    m_input->AddASTMLayers();

    for (auto detail : details)
    {
        // Use custom deleter function to lose ownership after adding the block
        bool deleteBlock = true;
        auto NoOpDeleter = [&deleteBlock](dx_ifaceBlock *block)
        {
            if (deleteBlock)
            {
                delete block;
            }
        };
        auto detailBlock = QSharedPointer<dx_ifaceBlock>(new dx_ifaceBlock, NoOpDeleter);

        QString blockName = detail.GetName();
        if (m_version <= DRW::AC1009)
        {
            blockName.replace(' '_L1, '_'_L1);
        }

        detailBlock->name = blockName.toStdString();
        detailBlock->layer = *layer1;

        detail.Scale(m_xscale, m_yscale);

        if (m_showMainPath)
        {
            // Export option: write the main path (the sew line) even for a piece that hides it, and send
            // that sew line to its own layer (14) rather than to the Draw line (8). 'detail' is a copy,
            // so neither the pattern nor the canvas is touched.
            detail.SetHideMainPath(false);
            detail.SetSewLineOnDrawing(false);
        }

        ExportASTMPieceBoundary(detailBlock, detail);
        ExportASTMSewLine(detailBlock, detail);
        ExportASTMDrawInternalPaths(detailBlock, detail);
        ExportASTMDrawPlaceLabels(detailBlock, detail);
        ExportASTMInternalCutout(detailBlock, detail);
        ExportASTMNotches(detailBlock, detail);
        ExportAAMAGrainline(detailBlock, detail);
        ExportASTMDrill(detailBlock, detail);
        ExportASTMAnnotationText(detailBlock, detail);
        ExportASTMMirrorLine(detailBlock, detail);
        ExportASTMDrawFoldLine(detailBlock, detail);
        ExportAnnotationText(detailBlock, detail, *layer15);

        m_input->AddBlock(detailBlock.data());

        auto insert = std::make_unique<DRW_Insert>();
        insert->name = blockName.toStdString();
        insert->layer = *layer1;

        m_input->AddEntity(insert.release());

        deleteBlock = false; // lose ownership
    }

    return m_input->fileExport(m_binary);
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::ExportASTMPieceBoundary(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail)
{
    QVector<VLayoutPoint> pieceBoundary = PieceOutline(detail);
    if (m_togetherWithNotches)
    {
        pieceBoundary = VAbstractPiece::RemoveDublicates(pieceBoundary, false);
    }

    // Piece boundary
    if (DRW_Entity *e = AAMAPolygon(pieceBoundary, *layer1, true))
    {
        detailBlock->ent.push_back(e);
    }

    ExportTurnAndCurvePoints(detailBlock, pieceBoundary);

    // Piece boundary quality validation curves
    if (DRW_Entity *q = AAMAPolygon(pieceBoundary, *layer84, true))
    {
        detailBlock->ent.push_back(q);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::ExportASTMSewLine(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail)
{
    // The sew line is pattern data, not drawing data: hiding the main path is a display option and must
    // not remove the sew line from the DXF. Style3D rebuilds the couturage from it — without it the piece
    // has nothing to stitch.
    //
    // Et la même exception qu'en AAMA — voir ExportAAMADrawSewLine : en mode Clo3D/Style3D une pièce
    // sans valeur de couture garde quand même son calque 14, parce que c'est là que Style3D la lit.
    if (!detail.IsSeamAllowance() && !IsClo3DCompatibility())
    {
        return;
    }

    // Idem AAMA — voir ContourWithCutPaths : la pince coupée est entrée dans le contour, la couture
    // la porte donc aussi.
    QVector<VLayoutPoint> const sewLine =
        ContourWithCutPaths(detail, detail.GetMappedFullContourPoints(m_togetherWithNotches, true, false));

    // Sew lines
    if (DRW_Entity *e = AAMAPolygon(sewLine, *layer14, true))
    {
        detailBlock->ent.push_back(e);
    }

    // No turn or curve point here: layers 2 and 3 describe the outline of the piece. See
    // ExportAAMADrawSewLine().

    // Sew lines quality validation curves
    if (DRW_Entity *e = AAMAPolygon(sewLine, *layer87, true))
    {
        detailBlock->ent.push_back(e);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::ExportASTMDrawInternalPaths(const QSharedPointer<dx_ifaceBlock> &detailBlock,
                                             const VLayoutPiece &detail)
{
    auto DrawPolygon =
        [this, detailBlock](const QVector<VLayoutPoint> &points, Qt::PenStyle style, bool notMirrored, bool forceClosed)
    {
        // Internal line
        if (DRW_Entity *e = AAMAPolygon(points, *layer8, forceClosed))
        {
            e->lineType = dx_iface::QtPenStyleToString(style);
            detailBlock->ent.push_back(e);
        }

        if (notMirrored && !points.isEmpty())
        {
            QPointF const pos(points.constFirst().x(),
                              points.constFirst().y() - ToPixel(AAMATextHeight, m_varInsunits));
            detailBlock->ent.push_back(AAMAText(pos, QStringLiteral("NM"), *layer8));
        }

        ExportTurnAndCurvePoints(detailBlock, points);

        // Internal lines quality validation curves
        if (DRW_Entity *e = AAMAPolygon(points, *layer85, forceClosed))
        {
            detailBlock->ent.push_back(e);
        }
    };

    const QVector<VLayoutPiecePath> drawIPaths = InternalPaths(detail, false);
    for (const auto &iPath : drawIPaths)
    {
        QVector<VLayoutPoint> points = InternalPathPoints(iPath);
        DrawPolygon(points, iPath.PenStyle(), iPath.IsNotMirrored(), false);

        if (!iPath.IsNotMirrored() && detail.IsShowFullPiece() && !detail.GetMappedSeamMirrorLine().isNull())
        {
            const QTransform matrix = VGObject::FlippingMatrix(detail.GetMappedSeamMirrorLine());
            std::transform(points.begin(), points.end(), points.begin(),
                           [&matrix](const VLayoutPoint &point) { return VAbstractPiece::MapPoint(point, matrix); });
            DrawPolygon(points, iPath.PenStyle(), iPath.IsNotMirrored(), false);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::ExportASTMDrawPlaceLabels(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail)
{
    auto DrawShape = [this, detailBlock](const PlaceLabelImg &shape, bool notMirrored, bool forceClosed)
    {
        for (const auto &points : shape)
        {
            // Internal line (placelabel)
            if (DRW_Entity *e = AAMAPolygon(points, *layer8, forceClosed))
            {
                detailBlock->ent.push_back(e);
            }

            if (notMirrored && !points.isEmpty())
            {
                QPointF const pos(points.constFirst().x(),
                                  points.constFirst().y() - ToPixel(AAMATextHeight, m_varInsunits));
                detailBlock->ent.push_back(AAMAText(pos, QStringLiteral("NM"), *layer8));
            }

            ExportTurnAndCurvePoints(detailBlock, points);

            // Internal lines quality validation curves
            if (DRW_Entity *e = AAMAPolygon(points, *layer85, false))
            {
                detailBlock->ent.push_back(e);
            }
        }
    };

    const QVector<VLayoutPlaceLabel> labels = detail.GetPlaceLabels();
    for (const auto &label : labels)
    {
        if (label.Type() != PlaceLabelType::Doubletree && label.Type() != PlaceLabelType::Button &&
            label.Type() != PlaceLabelType::Circle)
        {
            PlaceLabelImg shape = detail.MapPlaceLabelShape(VAbstractPiece::PlaceLabelShape(label));
            DrawShape(shape, label.IsNotMirrored(), false);

            if (!label.IsNotMirrored() && detail.IsShowFullPiece() && !detail.GetMappedSeamMirrorLine().isNull())
            {
                const QTransform matrix = VGObject::FlippingMatrix(detail.GetMappedSeamMirrorLine());
                for (auto &points : shape)
                {
                    std::transform(points.begin(), points.end(), points.begin(),
                                   [&matrix](const VLayoutPoint &point)
                                   { return VAbstractPiece::MapPoint(point, matrix); });
                }

                DrawShape(shape, label.IsNotMirrored(), false);
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::ExportASTMInternalCutout(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail)
{
    // Même règle qu'en AAMA — voir ExportAAMAIntcut : en mode Clo3D/Style3D la découpe appartient au
    // contour et s'écrit donc sur le 1. Les deux voisines du calque 11, elles, ne bougent pas : le
    // repère « NM » PARLE des découpes internes sans en être une, et la courbe du 86 est une copie de
    // validation ASTM, pas la ligne de coupe.
    const UTF8STRING &cutLayer = IsClo3DCompatibility() ? *layer1 : *layer11;

    auto DrawPolygon = [this, detailBlock, &cutLayer](const QVector<VLayoutPoint> &points, bool notMirrored,
                                                      bool forceClosed)
    {
        // Internal cutout
        if (DRW_Entity *e = AAMAPolygon(points, cutLayer, forceClosed))
        {
            detailBlock->ent.push_back(e);
        }

        if (notMirrored && !points.isEmpty())
        {
            QPointF const pos(points.constFirst().x(),
                              points.constFirst().y() - ToPixel(AAMATextHeight, m_varInsunits));
            detailBlock->ent.push_back(AAMAText(pos, QStringLiteral("NM"), *layer11));
        }

        ExportTurnAndCurvePoints(detailBlock, points);

        // Internal cutouts quality validation curves
        if (DRW_Entity *e = AAMAPolygon(points, *layer86, forceClosed))
        {
            detailBlock->ent.push_back(e);
        }
    };

    // Les chemins BRUTS, et la même règle qu'en AAMA : une découpe entrée dans le contour n'est pas
    // réécrite ici — voir ExportAAMAIntcut et ContourWithCutPaths.
    QVector<VLayoutPiecePath> const drawIntCut = detail.MappedInternalPathsForCut(true);
    for (const auto &intCut : drawIntCut)
    {
        if (IsMergedInContour(detail, intCut))
        {
            continue;
        }

        QVector<VLayoutPoint> points = InternalPathPoints(intCut);
        DrawPolygon(points, intCut.IsNotMirrored(), false);

        if (!intCut.IsNotMirrored() && detail.IsShowFullPiece() && !detail.GetMappedSeamMirrorLine().isNull())
        {
            const QTransform matrix = VGObject::FlippingMatrix(detail.GetMappedSeamMirrorLine());
            std::transform(points.begin(), points.end(), points.begin(),
                           [&matrix](const VLayoutPoint &point) { return VAbstractPiece::MapPoint(point, matrix); });
            DrawPolygon(points, intCut.IsNotMirrored(), false);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::ExportASTMAnnotationText(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail)
{
    QString const name = detail.GetName();
    QPointF const textPos = detail.VLayoutPiece::MappedDetailBoundingRect().center();

    QPointF const pos(textPos.x(), textPos.y() - ToPixel(AAMATextHeight, m_varInsunits));
    detailBlock->ent.push_back(AAMAText(pos, name, *layer15));
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::ExportASTMDrill(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail)
{
    const QVector<VLayoutPlaceLabel> labels = detail.GetPlaceLabels();

    auto ExportPoint = [this, detailBlock](QPointF center, qreal diameter)
    {
        auto point = std::make_unique<DRW_Point>();
        point->basePoint =
            DRW_Coord(FromPixel(center.x(), m_varInsunits), FromPixel(GetSize().height() - center.y(), m_varInsunits),
                      FromPixel(diameter, m_varInsunits));
        point->layer = *layer13;
        detailBlock->ent.push_back(point.release());

        // TODO. Investigate drill category
        // QPointF pos(center.x(), center.y() - ToPixel(AAMATextHeight, m_varInsunits));
        // detailBlock->ent.push_back(AAMAText(pos, category, *layer13));
    };

    for (const auto &label : labels)
    {
        if (label.Type() == PlaceLabelType::Doubletree || label.Type() == PlaceLabelType::Button ||
            label.Type() == PlaceLabelType::Circle)
        {
            QPointF center = detail.GetMatrix().map(label.Center());
            const QLineF diameter(label.Box().bottomLeft(), label.Box().topRight());

            ExportPoint(center, diameter.length());

            if (!label.IsNotMirrored() && detail.IsShowFullPiece() && !detail.GetMappedSeamMirrorLine().isNull())
            {
                const QTransform matrix = VGObject::FlippingMatrix(detail.GetMappedSeamMirrorLine());
                center = VAbstractPiece::MapPoint(center, matrix);
                ExportPoint(center, diameter.length());
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::ExportASTMNotches(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail)
{
    // Même règle que le chemin AAMA : le cran s'écrit avec ou sans valeur de couture (15-10-2026), et
    // seul « Boundary together with notches » l'écarte — il l'a déjà mis dans la polyligne du contour.
    if (m_togetherWithNotches)
    {
        return;
    }

    auto ExportPassmark = [this, detailBlock, &detail](const VLayoutPassmark &passmark)
    {
        DRW_ASTMNotch *notch = ExportASTMNotch(passmark);
        DRW_ATTDEF *attdef = ExportASTMNotchDataDependecy(passmark, notch->layer, detail);
        detailBlock->ent.push_back(notch);

        if (attdef != nullptr)
        {
            detailBlock->ent.push_back(attdef);
        }
    };

    const QVector<VLayoutPassmark> passmarks = detail.GetMappedPassmarks();
    for (const auto &passmark : passmarks)
    {
        ExportPassmark(passmark);

        const QLineF mirrorLine = detail.GetMappedSeamMirrorLine();
        if (!mirrorLine.isNull() && detail.IsShowFullPiece()
            && !IsPointOnLineviaPDP(passmark.baseLine.p1(), mirrorLine.p1(), mirrorLine.p2()) && !passmark.notMirrored)
        {
            const QTransform matrix = VGObject::FlippingMatrix(mirrorLine);
            const VLayoutPassmark mirroredPassmark = VLayoutPiece::MapPassmark(passmark, matrix, false);

            ExportPassmark(mirroredPassmark);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::ExportASTMMirrorLine(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail)
{
    // The axis belongs to the unfolded view: once the piece is folded, its fold edge is already part
    // of the boundary. Every other mirror-axis site in the application draws it on exactly this
    // condition, and the AAMA export below does the same. The guard used to be inverted here, which
    // made the whole function dead code and left the ASTM export with no mirror line at all.
    if (not detail.IsShowFullPiece())
    {
        return;
    }

    if (QLineF const mirrorLine = detail.GetMappedCorrectedMirrorLine(m_togetherWithNotches);
        not mirrorLine.isNull() && detail.IsShowMirrorLine())
    {
        // Layer 6 is the ASTM layer for the mirror line ("Mirror line — symmetry of fold"); layer 8
        // is "internal line(s) — graphic annotation of placement, not cut". AAMA puts its own axis on
        // 8 because its table does not declare layer 6. The two formats have different layer tables,
        // and following each one's own is the point — on 8 an ASTM reader would take the axis for an
        // internal line.
        if (DRW_Entity *e = AAMALine(mirrorLine, *layer6))
        {
            e->lineType = dx_iface::QtPenStyleToString(Qt::DashDotLine);
            detailBlock->ent.push_back(e);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::ExportASTMDrawFoldLine(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail)
{
    if (detail.IsShowFullPiece() && !detail.IsShowMirrorLine())
    {
        return;
    }

    VFoldLine const fLine = detail.FoldLine();
    QVector<QVector<QPointF>> points = fLine.FoldLineMarkPoints();
    if (points.isEmpty())
    {
        return;
    }

    switch (detail.GetFoldLineType())
    {
        case FoldLineType::TwoArrows:
        case FoldLineType::TwoArrowsTextAbove:
            points.removeAt(1);
            ASTMDrawFoldLineTwoArrows(points, detailBlock);
            break;
        case FoldLineType::LAST_ONE_DO_NOT_USE:
            Q_UNREACHABLE();
            break;
        case FoldLineType::None:
        default:
            break;
    };
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::ExportASTMNotch(const VLayoutPassmark &passmark) -> DRW_ASTMNotch *
{
    auto *notch = new DRW_ASTMNotch();
    const QPointF center = passmark.baseLine.p1();

    notch->basePoint =
        DRW_Coord(FromPixel(center.x(), m_varInsunits), FromPixel(GetSize().height() - center.y(), m_varInsunits),
                  FromPixel(passmark.baseLine.length(), m_varInsunits));

    notch->angle = passmark.baseLine.angle();

    PassmarkLineType type = passmark.type;
    if ((m_compatibilityMode == DXFApparelCompatibility::RPCADV08 ||
         m_compatibilityMode == DXFApparelCompatibility::RPCADV09 ||
         m_compatibilityMode == DXFApparelCompatibility::RPCADV10) &&
        (type == PassmarkLineType::ExternalVMark || type == PassmarkLineType::InternalVMark))
    {
        type = PassmarkLineType::CheckMark;
    }

    switch (type)
    {
        case PassmarkLineType::OneLine:
        case PassmarkLineType::TwoLines:
        case PassmarkLineType::ThreeLines:
            // Slit notch
            notch->layer = *layer4;
            break;
        case PassmarkLineType::ExternalVMark:
        case PassmarkLineType::InternalVMark:
        { // V-Notch
            const QLineF boundaryLine(passmark.lines.constFirst().p1(), passmark.lines.constLast().p2());
            notch->thickness = FromPixel(boundaryLine.length(), m_varInsunits); // width
            notch->layer = *layer4;
            break;
        }
        case PassmarkLineType::TMark:
            // T-Notch
            notch->thickness = FromPixel(passmark.lines.constLast().length(), m_varInsunits); // width
            notch->layer = *layer80;
            break;
        case PassmarkLineType::BoxMark:
        { // Castle Notch
            QPointF const start = passmark.lines.constFirst().p1();
            QPointF const end = passmark.lines.constLast().p2();

            notch->layer = *layer81;
            notch->thickness = FromPixel(QLineF(start, end).length(), m_varInsunits); // width
            break;
        }
        case PassmarkLineType::UMark:
        { // U-Notch
            QPointF const start = passmark.lines.constFirst().p1();
            QPointF const end = passmark.lines.constLast().p2();

            notch->thickness = FromPixel(QLineF(start, end).length(), m_varInsunits); // width

            notch->layer = *layer83;
            break;
        }
        case PassmarkLineType::CheckMark:
        { // Check Notch
            const QLineF &line1 = passmark.lines.constFirst();
            const QLineF &line2 = passmark.lines.constLast();

            qreal width = QLineF(line1.p1(), line2.p2()).length();

            if (not passmark.isClockwiseOpening)
            { // a counter clockwise opening
                width *= -1;
            }

            notch->thickness = FromPixel(width, m_varInsunits); // width
            notch->layer = *layer82;
            break;
        }
        case PassmarkLineType::LAST_ONE_DO_NOT_USE:
            Q_UNREACHABLE();
            break;
        default:
            break;
    };

    return notch;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::ExportASTMNotchDataDependecy(const VLayoutPassmark &passmark, const UTF8STRING &notchLayer,
                                              const VLayoutPiece &detail) -> DRW_ATTDEF *
{
    QVector<VLayoutPoint> const boundary = !passmark.isBuiltIn ? detail.GetMappedSeamAllowancePoints()
                                                               : detail.GetMappedContourPoints();

    const QPointF center = passmark.baseLine.p1();
    QPointF referencePoint;
    if (not NotchPrecedingPoint(boundary, center, referencePoint))
    {
        return nullptr;
    }

    auto *attdef = new DRW_ATTDEF();
    // Le calque du contour dont le cran dépend : la valeur de couture s'écrit sur le 1, le main path sur
    // le 14. ⚠️ **Sauf quand la pièce n'a pas de valeur de couture** (15-10-2026) : son main path EST le
    // contour de coupe, écrit sur le **1** — le 14 ne porte alors rien, et un attribut qui y renverrait
    // désignerait un calque vide. C'est la même bascule que pour `boundary`, deux lignes plus haut.
    attdef->layer = (!passmark.isBuiltIn || not detail.IsSeamAllowance()) ? *layer1 : *layer14;
    attdef->basePoint = DRW_Coord(FromPixel(referencePoint.x(), m_varInsunits),
                                  FromPixel(GetSize().height() - referencePoint.y(), m_varInsunits), 0);
    attdef->adjustmentPoint =
        DRW_Coord(FromPixel(center.x(), m_varInsunits), FromPixel(GetSize().height() - center.y(), m_varInsunits), 0);
    attdef->height = 3.0;
    attdef->text = "Link:" + notchLayer;
    attdef->name = "Dependency";
    attdef->flags |= 0x2;             // this is a constant attribute
    attdef->horizontalAdjustment = 3; // aligned (if vertical alignment = 0)

    return attdef;
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::ExportTurnPoints(const QSharedPointer<dx_ifaceBlock> &detailBlock,
                                  const QVector<VLayoutPoint> &points) const
{
    for (const auto &p : std::as_const(points))
    {
        if (p.TurnPoint())
        {
            detailBlock->ent.push_back(AAMAPoint(p, *layer2));
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::ExportTurnAndCurvePoints(const QSharedPointer<dx_ifaceBlock> &detailBlock,
                                          const QVector<VLayoutPoint> &points) const
{
    for (const auto &p : std::as_const(points))
    {
        if (p.TurnPoint())
        {
            detailBlock->ent.push_back(AAMAPoint(p, *layer2));
        }
        else if (p.CurvePoint())
        {
            detailBlock->ent.push_back(AAMAPoint(p, *layer3));
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::AAMAPolygon(const QVector<VLayoutPoint> &polygon, const UTF8STRING &layer, bool forceClosed)
    -> DRW_Entity *
{
    if (polygon.isEmpty())
    {
        return nullptr;
    }

    if (m_version > DRW::AC1009)
    { // Use lwpolyline
        return CreateAAMAPolygon<DRW_LWPolyline, DRW_Vertex2D>(polygon, layer, forceClosed);
    }

    // Use polyline
    return CreateAAMAPolygon<DRW_Polyline, DRW_Vertex>(polygon, layer, forceClosed);
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::AAMALine(const QLineF &line, const UTF8STRING &layer) -> DRW_Entity *
{
    auto *lineEnt = new DRW_Line();
    lineEnt->basePoint = DRW_Coord(FromPixel(line.p1().x(), m_varInsunits),
                                   FromPixel(GetSize().height() - line.p1().y(), m_varInsunits), 0);
    lineEnt->secPoint = DRW_Coord(FromPixel(line.p2().x(), m_varInsunits),
                                  FromPixel(GetSize().height() - line.p2().y(), m_varInsunits), 0);
    lineEnt->layer = layer;

    return lineEnt;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::AAMAText(const QPointF &pos, const QString &text, const UTF8STRING &layer) -> DRW_Entity *
{
    auto *textLine = new DRW_Text();
    textLine->basePoint =
        DRW_Coord(FromPixel(pos.x(), m_varInsunits), FromPixel(GetSize().height() - pos.y(), m_varInsunits), 0);
    textLine->secPoint =
        DRW_Coord(FromPixel(pos.x(), m_varInsunits), FromPixel(GetSize().height() - pos.y(), m_varInsunits), 0);
    textLine->height = AAMATextHeight;
    textLine->layer = layer;
    textLine->text = text.toStdString();

    return textLine;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::AAMAPoint(const QPointF &pos, const UTF8STRING &layer) const -> DRW_Point *
{
    auto *point = new DRW_Point();
    point->basePoint =
        DRW_Coord(FromPixel(pos.x(), m_varInsunits), FromPixel(GetSize().height() - pos.y(), m_varInsunits), 0);
    point->layer = layer;
    return point;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::AAMACircle(const QPointF &pos, const std::string &layer, qreal radius) const -> DRW_Circle *
{
    auto *circle = new DRW_Circle();
    circle->basePoint =
        DRW_Coord(FromPixel(pos.x(), m_varInsunits), FromPixel(GetSize().height() - pos.y(), m_varInsunits), 0);
    circle->layer = layer;
    circle->radious = FromPixel(radius, m_varInsunits);
    return circle;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::NotchPrecedingPoint(const QVector<VLayoutPoint> &boundary, QPointF notchBase, QPointF &point) -> bool
{
    if (boundary.count() < 2)
    {
        return false;
    }

    if (VFuzzyComparePoints(boundary.constFirst(), notchBase))
    {
        point = boundary.constFirst().ToQPointF();
        return true;
    }

    if (VFuzzyComparePoints(boundary.constLast(), notchBase))
    {
        point = boundary.constLast().ToQPointF();
        return true;
    }

    QPointF candidatePoint;
    qreal bestDistance = INT_MAX;
    bool found = false;

    for (qint32 i = 0; i < boundary.count() - 1; ++i)
    {
        const QPointF cPoint = VGObject::ClosestPoint(QLineF(boundary.at(i), boundary.at(i + 1)), notchBase);

        if (IsPointOnLineSegment(cPoint, boundary.at(i), boundary.at(i + 1)))
        {
            const qreal length = QLineF(notchBase, cPoint).length();
            if (length < bestDistance)
            {
                candidatePoint = boundary.at(i).ToQPointF();
                bestDistance = length;
                found = true;
            }
        }
    }

    if (found)
    {
        point = candidatePoint;
        return true;
    }

    return found;
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::AAMADrawFoldLineTwoArrows(const QVector<QVector<QPointF>> &points,
                                           const QSharedPointer<dx_ifaceBlock> &detailBlock)
{
    QVector<VLayoutPoint> shape;
    for (const auto &subShape : points)
    {
        for (const auto &point : subShape)
        {
            VLayoutPoint p(point);
            p.SetTurnPoint(true);
            shape.append(p);
        }
    }

    if (DRW_Entity *e = AAMAPolygon(shape, *layer8, false))
    {
        detailBlock->ent.push_back(e);
    }

    ExportTurnPoints(detailBlock, shape);
}

//---------------------------------------------------------------------------------------------------------------------
void VDxfEngine::ASTMDrawFoldLineTwoArrows(const QVector<QVector<QPointF>> &points,
                                           const QSharedPointer<dx_ifaceBlock> &detailBlock)
{
    QVector<VLayoutPoint> shape;
    for (const auto &subShape : points)
    {
        for (const auto &point : subShape)
        {
            VLayoutPoint p(point);
            p.SetTurnPoint(true);
            shape.append(p);
        }
    }

    if (DRW_Entity *e = AAMAPolygon(shape, *layer8, false))
    {
        detailBlock->ent.push_back(e);
    }

    if (!shape.isEmpty())
    {
        QPointF const pos(shape.constFirst().x(), shape.constFirst().y() - ToPixel(AAMATextHeight, m_varInsunits));
        detailBlock->ent.push_back(AAMAText(pos, QStringLiteral("NM"), *layer8));
    }

    ExportTurnPoints(detailBlock, shape);

    if (DRW_Entity *e = AAMAPolygon(shape, *layer85, false))
    {
        detailBlock->ent.push_back(e);
    }
}


//---------------------------------------------------------------------------------------------------------------------
template <class P, class V, class C>
auto VDxfEngine::CreateAAMAPolygon(const QVector<C> &polygon, const UTF8STRING &layer, bool forceClosed) -> P *
{
    auto *poly = new P();
    poly->layer = layer;

    if (forceClosed)
    {
        poly->flags |= 0x1; // closed NOLINT(hicpp-signed-bitwise)
    }
    else
    {
        if (polygon.size() > 1 && polygon.constFirst() == polygon.constLast())
        {
            poly->flags |= 0x1; // closed NOLINT(hicpp-signed-bitwise)
        }
    }

    for (const auto &p : polygon)
    {
        V vertex(FromPixel(p.x(), m_varInsunits), FromPixel(GetSize().height() - p.y(), m_varInsunits));

        if constexpr (std::is_same_v<V, DRW_Vertex>)
        {
            if (p.CurvePoint())
            {
                vertex.flags |= 0x1;
            }
        }

        poly->addVertex(vertex);
    }

    return poly;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::PieceOutline(const VLayoutPiece &detail) const -> QVector<VLayoutPoint>
{
    if (detail.IsSeamAllowance())
    {
        return detail.GetMappedFullSeamAllowancePoints(m_togetherWithNotches);
    }
    return ContourWithCutPaths(detail, detail.GetMappedFullContourPoints(m_togetherWithNotches));
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::IsClo3DCompatibility() const -> bool
{
    return m_compatibilityMode == DXFApparelCompatibility::CLO3D;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::IsMergedInContour(const VLayoutPiece &detail, const VLayoutPiecePath &path) const -> bool
{
    // ── La pince coupée ───────────────────────────────────────────────────────────────────────────
    //
    // ⚠️ **C'est la règle de Sophie, mesurée sur le DXF qu'elle a fait à la main** : en mode
    // Clo3D/Style3D, un chemin « Cut on fabric » dont les deux extrémités tombent sur le contour de
    // coupe n'est pas un trait posé dans la pièce — il **appartient au contour**. Style3D ne lit une
    // découpe que là : partout ailleurs, sur quelque calque que ce soit, il en fait une ligne interne.
    // Le 11 (la convention AAMA de la découpe interne) et le 14 ont été essayés sur son propre fichier,
    // l'un après l'autre : les deux échouent, et il ne reste que le contour.
    //
    // Deux conditions, et pas une de plus : la pièce est **sans valeur de couture** — c'est ce qui met
    // les deux extrémités du chemin SUR le contour, puisque le contour EST le main path — et le chemin
    // demande à y aller **des deux côtés** (les deux cases de sa dialogue).
    return IsClo3DCompatibility() && not detail.IsSeamAllowance() && path.IsCutPath()
           && path.IsFirstToCuttingContour() && path.IsLastToCuttingContour();
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::SplicePathInContour(QVector<VLayoutPoint> &contour, const QVector<VLayoutPoint> &points) -> bool
{
    if (contour.size() < 3 || points.size() < 3)
    {
        return false;
    }

    // Le sommet du contour qui porte @p point ; à défaut, le segment sur lequel il tombe — le point
    // d'accroche d'une découpe est SUR le contour par construction, mais pas forcément sur un sommet :
    // la polyligne a pu être dédupliquée ou lissée entre-temps.
    const auto AnchorIndex = [&contour](const VLayoutPoint &point) -> int
    {
        for (int i = 0; i < contour.size(); ++i)
        {
            if (VFuzzyComparePoints(contour.at(i), point))
            {
                return i;
            }
        }

        for (int i = 0; i < contour.size(); ++i)
        {
            const int next = (i + 1) % contour.size();
            if (IsPointOnLineSegment(point, contour.at(i), contour.at(next)))
            {
                contour.insert(i + 1, point);
                return i + 1;
            }
        }

        return -1;
    };

    const int first = AnchorIndex(points.constFirst());
    const int last = AnchorIndex(points.constLast());
    if (first < 0 || last < 0)
    {
        // Une accroche qui ne tombe nulle part n'arrive pas — les deux extrémités d'une découpe sont
        // posées sur le contour par `VPiecePath::PathPoints`. Si elle arrivait quand même, le dire est
        // la seule chose honnête : la pièce sortirait avec un contour qui ignore cette découpe.
        qWarning() << "VDxfEngine: a cut path does not meet the outline, it is left out of it.";
        return false;
    }

    if (first == last)
    {
        return false;
    }

    // Les deux arcs entre les accroches ; **c'est le COURT qui est l'ouverture de la pince**, et c'est
    // lui que le chemin remplace. L'autre est le reste de la pièce, et il ne bouge pas.
    const int n = static_cast<int>(contour.size());
    QVector<int> forward;
    for (int i = first; i != last; i = (i + 1) % n)
    {
        forward.append(i);
    }
    forward.append(last);

    const auto ArcLength = [&contour](const QVector<int> &arc) -> qreal
    {
        qreal length = 0.0;
        for (int i = 0; i + 1 < arc.size(); ++i)
        {
            length += QLineF(contour.at(arc.at(i)), contour.at(arc.at(i + 1))).length();
        }
        return length;
    };

    QVector<int> backward;
    for (int i = last; i != first; i = (i + 1) % n)
    {
        backward.append(i);
    }
    backward.append(first);

    const QVector<int> &shortArc = ArcLength(forward) <= ArcLength(backward) ? forward : backward;

    // Le chemin va de son premier point à son dernier : dans le sens de l'arc court, ou contre lui.
    QVector<VLayoutPoint> splice = points;
    if (shortArc.constFirst() != first)
    {
        std::reverse(splice.begin(), splice.end());
    }

    // On rebâtit le contour : les sommets de l'arc court disparaissent, remplacés par le chemin entier
    // (ses deux accroches comprises, qui sont les deux premiers et dernier points de `splice`).
    const QSet<int> dropped(shortArc.cbegin() + 1, shortArc.cend() - 1);
    QVector<VLayoutPoint> merged;
    merged.reserve(contour.size() + splice.size());
    for (int i = 0; i < n; ++i)
    {
        if (i == shortArc.constFirst())
        {
            merged += splice;
        }
        else if (not dropped.contains(i))
        {
            merged.append(contour.at(i));
        }
    }

    contour = merged;
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::ContourWithCutPaths(const VLayoutPiece &detail, QVector<VLayoutPoint> contour) const
    -> QVector<VLayoutPoint>
{
    if (not IsClo3DCompatibility() || detail.IsSeamAllowance())
    {
        return contour;
    }

    const QVector<VLayoutPiecePath> cutPaths = detail.MappedInternalPathsForCut(true);
    for (const VLayoutPiecePath &path : cutPaths)
    {
        if (not IsMergedInContour(detail, path))
        {
            continue;
        }

        SplicePathInContour(contour, path.Points());

        // ⚠️ **Et son miroir avec elle.** Une pièce dépliée porte la pince deux fois — l'originale et
        // son reflet — et c'est ainsi que `ExportAAMAIntcut` les écrivait : les deux moitiés du
        // contour doivent donc recevoir la leur, sans quoi la pièce sortirait coupée d'un seul côté.
        if (not path.IsNotMirrored() && detail.IsShowFullPiece() && not detail.GetMappedSeamMirrorLine().isNull())
        {
            const QTransform matrix = VGObject::FlippingMatrix(detail.GetMappedSeamMirrorLine());
            QVector<VLayoutPoint> mirrored = path.Points();
            std::transform(mirrored.begin(), mirrored.end(), mirrored.begin(),
                           [&matrix](const VLayoutPoint &point) { return VAbstractPiece::MapPoint(point, matrix); });
            SplicePathInContour(contour, mirrored);
        }
    }

    return contour;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::InternalPathPoints(const VLayoutPiecePath &path) const -> QVector<VLayoutPoint>
{
    QVector<VLayoutPoint> points = path.Points();

    if (not IsClo3DCompatibility())
    {
        return points;
    }

    // An internal path that reaches the cutting contour carries one extra point per end, where it crosses
    // the seam allowance. Style3D reads a dart on its own and needs both branches to end on the outline:
    // with that continuation they end outside of it, and the dart cannot be recognised any more.
    //
    // ⛔ **Sauf une DÉCOUPE — et c'est ce qui la rendait méconnaissable.** Un chemin « Cut on fabric »
    // n'est pas une pince à coudre : c'est une ligne de coupe, et ses deux extrémités DOIVENT tomber sur
    // le contour de coupe — c'est ce qui en fait une fente dans la pièce, et ce qui la rattache au
    // contour. Les lui retirer laissait deux segments de deux points, posés à quelques millimètres à
    // l'intérieur, que Style3D ne pouvait lire que comme des lignes internes errantes — ou pas du tout.
    if (path.IsCutPath())
    {
        return points;
    }

    if (path.IsFirstToCuttingContour() && points.size() > 2)
    {
        points.removeFirst();
    }

    if (path.IsLastToCuttingContour() && points.size() > 2)
    {
        points.removeLast();
    }

    return points;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::DeepestPointIndex(const QVector<VLayoutPoint> &points) -> int
{
    // A dart is a V: its apex is the point of the path the furthest from the line joining both ends.
    if (points.size() < 3)
    {
        return -1;
    }

    const QLineF ends(points.constFirst(), points.constLast());
    if (qFuzzyIsNull(ends.length()))
    {
        return -1;
    }

    int deepest = -1;
    qreal maxDistance = 0;
    for (int i = 1; i < points.size() - 1; ++i)
    {
        const QLineF side(points.constFirst(), points.at(i));
        const qreal distance = qAbs(side.dx() * ends.dy() - side.dy() * ends.dx()) / ends.length();
        if (distance > maxDistance)
        {
            maxDistance = distance;
            deepest = i;
        }
    }
    return deepest;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDxfEngine::InternalPaths(const VLayoutPiece &detail, bool cut) const -> QVector<VLayoutPiecePath>
{
    const QVector<VLayoutPiecePath> paths = detail.MappedInternalPathsForCut(cut);

    if (not IsClo3DCompatibility())
    {
        return paths;
    }

    // The continuation to the cutting contour does not belong to the path itself: Style3D traces a dart
    // from its own points, which must end on the outline. A dart is recognised here by that continuation.
    auto MakePath = [](const VLayoutPiecePath &path, const QVector<VLayoutPoint> &points)
    {
        VLayoutPiecePath made = path;
        made.SetPoints(points);
        made.SetFirstToCuttingContour(false);
        made.SetLastToCuttingContour(false);
        return made;
    };

    QVector<VLayoutPiecePath> result;
    result.reserve(paths.size());
    for (const auto &path : paths)
    {
        const QVector<VLayoutPoint> points = InternalPathPoints(path);

        // ⚠️ **Une découpe n'est pas une pince à tracer** : elle garde son chemin entier, pointe
        // comprise — voir `InternalPathPoints`, qui lui rend justement ses deux extrémités. La couper
        // en deux baselines à sa pointe ferait d'une ligne de coupe deux morceaux qui ne se touchent
        // plus, et c'est exactement ce que Style3D recevait.
        const bool dart = !path.IsCutPath()
                          && (path.IsFirstToCuttingContour() || path.IsLastToCuttingContour());

        // Style3D traces a dart from exactly two baselines sharing their apex vertex ("Trace as Dart").
        // A single polyline carries both branches and is refused — the dart stays closed and unstitched.
        const int apex = dart ? DeepestPointIndex(points) : -1;
        if (apex > 0 && apex < points.size() - 1)
        {
            result.append(MakePath(path, points.mid(0, apex + 1)));
            result.append(MakePath(path, points.mid(apex)));
        }
        else
        {
            result.append(MakePath(path, points));
        }
    }
    return result;
}
