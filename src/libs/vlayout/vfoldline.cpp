/************************************************************************
 **
 **  @file   vfoldline.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   20 12, 2023
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2023 Valentina project
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
#include "vfoldline.h"
#include "../vformat/vsinglelineoutlinechar.h"
#include "../vgeometry/vgeometrydef.h"
#include "../vmisc/compatibility.h"
#include "../vmisc/def.h"
#include "vexception.h"
#include "../vmisc/svgfont/svgdef.h"
#include "../vmisc/svgfont/vsvgfont.h"
#include "../vmisc/svgfont/vsvgfontdatabase.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vmisc/vtranslator.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vformula.h"
#include "../vwidgets/vpiecegrainline.h"

#include <QGraphicsSimpleTextItem>
#include <QLineF>
#include <QPainterPath>
#include <QtDebug>
#include <QtMath>
#include <algorithm>

namespace
{
constexpr qreal defLabelMargin = CmToPixel(0.5);
constexpr qreal minArrowsHeight = CmToPixel(0.5);
constexpr qreal defArrowsHeight = CmToPixel(1.);
} // namespace

//---------------------------------------------------------------------------------------------------------------------
VFoldLine::VFoldLine(const QLineF &line, FoldLineType type)
  : m_foldLine(line),
    m_type(type)
{
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::Create(const VPiece &piece, const VContainer *data) -> VFoldLine
{
    QLineF const foldLine = piece.IsHideMainPath() ? piece.SeamAllowanceMirrorLine(data)
                                                   : piece.SeamMirrorLine(data);
    VFoldLine fLine(foldLine, piece.GetFoldLineType());

    const VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
    fLine.SetLabelSvgFontSize(static_cast<unsigned int>(settings->GetFoldLineLabelFontPixelSize()));
    fLine.SetLabel(piece.GetFoldLineLabel());

    {
        QFont font(settings->GetFoldLineLabelFontFamily());
        // **A face NAME, never a number** (decision of Sophie, 22-09-2026). A settings file written
        // before the face was named holds the weight instead, and that is what the fallback reads —
        // the two cannot disagree, because a number is resolved exactly as the editor's Weight control
        // resolves it.
        const QString face = settings->GetFoldLineLabelFontFace();
        if (!face.isEmpty())
        {
            font.setStyleName(face);
        }
        else
        {
            font.setWeight(settings->GetFoldLineLabelFontWeight());
        }
        font.setPixelSize(settings->GetFoldLineLabelFontPixelSize());
        fLine.SetOutlineFont(font);
        fLine.SetSvgFont(settings->GetLabelSVGFont());
    }

    fLine.ApplyManualFoldValues(piece, data);

    return fLine;
}

//---------------------------------------------------------------------------------------------------------------------
void VFoldLine::ApplyManualFoldValues(const VPiece &piece, const VContainer *data)
{
    if (piece.IsManualFoldHeight())
    {
        VFormula formula(piece.GetFormulaFoldHeight(), data);
        formula.setCheckZero(false);
        formula.setCheckLessThanZero(true);
        formula.Eval();

        if (formula.error())
        {
            const QString errorMsg = QObject::tr("Cannot calculate fold line height for piece '%1'. Reason: %2.")
                                         .arg(piece.GetName(), formula.Reason());
            VAbstractApplication::VApp()->IsPedantic()
                ? throw VException(errorMsg)
                : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
        }
        else
        {
            SetHeight(ToPixel(formula.getDoubleValue(), *data->GetPatternUnit()));
        }
    }

    if (piece.IsManualFoldWidth())
    {
        VFormula formula(piece.GetFormulaFoldWidth(), data);
        formula.setCheckZero(false);
        formula.setCheckLessThanZero(true);
        formula.Eval();

        if (formula.error())
        {
            const QString errorMsg = QObject::tr("Cannot calculate fold line width for piece '%1'. Reason: %2.")
                                         .arg(piece.GetName(), formula.Reason());
            VAbstractApplication::VApp()->IsPedantic()
                ? throw VException(errorMsg)
                : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
        }
        else
        {
            SetWidth(ToPixel(formula.getDoubleValue(), *data->GetPatternUnit()));
        }
    }

    if (piece.IsManualFoldCenter())
    {
        VFormula formula(piece.GetFormulaFoldCenter(), data);
        formula.setCheckZero(false);
        formula.setCheckLessThanZero(true);
        formula.Eval();

        if (formula.error())
        {
            const QString errorMsg =
                QObject::tr("Cannot calculate fold line center position for piece '%1'. Reason: %2.")
                    .arg(piece.GetName(), formula.Reason());
            VAbstractApplication::VApp()->IsPedantic()
                ? throw VException(errorMsg)
                : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
        }
        else
        {
            SetCenterPosition(formula.getDoubleValue());
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::Type() const -> FoldLineType
{
    return m_type;
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::Label() const -> QString
{
    return m_label;
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::OutlineFont() const -> QFont
{
    return m_outlineFont;
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::SvgFont() const -> QString
{
    return m_svgFont;
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::LabelSvgFontSize() const -> unsigned int
{
    return m_svgFontSize;
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::TypeHasLabel(FoldLineType type) -> bool
{
    return type == FoldLineType::TwoArrowsTextAbove;
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::HasLabel() const -> bool
{
    return TypeHasLabel(m_type);
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::Height() const -> qreal
{
    return m_height;
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::Width() const -> qreal
{
    return m_width;
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::CenterPosition() const -> qreal
{
    return m_center;
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::ResolvedHeight() const -> qreal
{
    return qFuzzyIsNull(m_height) ? defArrowsHeight : m_height;
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::ResolvedWidth() const -> qreal
{
    return qFuzzyIsNull(m_width) ? AutoWidth() : m_width;
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::ResolvedCenter() const -> qreal
{
    return m_center;
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::AutoWidth() const -> qreal
{
    return m_foldLine.length() * 0.9;
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::EffectiveWidth() const -> qreal
{
    return qFuzzyIsNull(m_width) ? AutoWidth() : qMin(m_width, m_foldLine.length());
}

//---------------------------------------------------------------------------------------------------------------------
void VFoldLine::SetHeight(qreal newHeight)
{
    m_height = qAbs(newHeight);
}

//---------------------------------------------------------------------------------------------------------------------
void VFoldLine::SetWidth(qreal newWidth)
{
    m_width = qAbs(newWidth);
}

//---------------------------------------------------------------------------------------------------------------------
void VFoldLine::SetCenterPosition(qreal newCenter)
{
    m_center = qBound(0., qAbs(newCenter), 1.);
}

//---------------------------------------------------------------------------------------------------------------------
void VFoldLine::SetLabel(const QString &newLabel)
{
    m_label = newLabel;
}

//---------------------------------------------------------------------------------------------------------------------
void VFoldLine::SetLabelSvgFontSize(unsigned int newFontSize)
{
    m_svgFontSize = qMax(newFontSize, static_cast<unsigned int>(VCommonSettings::MinPieceLabelFontPointSize()));
}

//---------------------------------------------------------------------------------------------------------------------
void VFoldLine::SetLabelFontItalic(bool newItalic)
{
    m_italic = newItalic;
}

//---------------------------------------------------------------------------------------------------------------------
void VFoldLine::SetLabelFontBold(bool newBold)
{
    m_bold = newBold;
}

//---------------------------------------------------------------------------------------------------------------------
void VFoldLine::SetLabelAlignment(int alignment)
{
    m_alignment = alignment;
}

//---------------------------------------------------------------------------------------------------------------------
void VFoldLine::SetMatrix(const QTransform &matrix)
{
    m_matrix = matrix;
}

//---------------------------------------------------------------------------------------------------------------------
void VFoldLine::SetVerticallyFlipped(bool value)
{
    m_verticallyFlipped = value;
}

//---------------------------------------------------------------------------------------------------------------------
void VFoldLine::SetHorizontallyFlipped(bool value)
{
    m_horizontallyFlipped = value;
}

//---------------------------------------------------------------------------------------------------------------------
void VFoldLine::SetOutlineFont(const QFont &font)
{
    m_outlineFont = font;
}

//---------------------------------------------------------------------------------------------------------------------
void VFoldLine::SetSvgFont(const QString &font)
{
    m_svgFont = font;
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::FoldLinePath() const -> QVector<QPainterPath>
{
    if (m_foldLine.isNull())
    {
        return {};
    }

    switch (m_type)
    {
        case FoldLineType::TwoArrows:
            return FoldLineTwoArrowsPath();
        case FoldLineType::TwoArrowsTextAbove:
            return FoldLineTwoArrowsTextAbovePath();
        case FoldLineType::LAST_ONE_DO_NOT_USE:
            Q_UNREACHABLE();
            break;
        case FoldLineType::None:
        default:
            return {};
    };
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::FoldLineMarkPoints() const -> QVector<QVector<QPointF>>
{
    if (m_foldLine.isNull())
    {
        return {};
    }

    switch (m_type)
    {
        case FoldLineType::TwoArrows:
        {
            ArrowsTextPosData const data = TwoArrowsData();
            return TwoArrowsPoints(data.arrowsWidth, data.arrowsHeight);
        }
        case FoldLineType::TwoArrowsTextAbove:
        {
            ArrowsTextPosData const data = TwoArrowsTextAboveData();
            return TwoArrowsPoints(data.arrowsWidth, data.arrowsHeight);
        }
        case FoldLineType::LAST_ONE_DO_NOT_USE:
            Q_UNREACHABLE();
            return {};
        case FoldLineType::None:
        default:
            return {};
    };
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::LabelPosition(bool &ok) const -> FoldLabelPosData
{
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_GCC("-Wnoexcept")

    FoldLabelPosData posData;
    std::unique_ptr<TextPosData> data;

    QT_WARNING_POP

    // Only TwoArrowsTextAbove draws a text: TwoArrows draws the arrows alone and None draws
    // nothing at all. Both must report "no label" — the DXF exporters' only guard is this flag,
    // so letting them fall through here is what used to leak a default "Cut On Fold" text into
    // files for pieces that carry no label.
    switch (m_type)
    {
        case FoldLineType::TwoArrowsTextAbove:
            data = std::make_unique<ArrowsTextPosData>(TwoArrowsTextAboveData());
            break;
        case FoldLineType::TwoArrows:
        case FoldLineType::None:
        default:
            ok = false;
            return {};
    }

    if (data->base.isNull())
    {
        ok = false;
        return {};
    }

    // The label always sits the same margin beyond the arrow tips: the height field sizes the arrows
    // and must not move the text.
    qreal const margin = defLabelMargin;

    QPointF const center = TrueCenter(data->base, data->labelWidth);

    QLineF baseLine(center, data->base.p1());
    baseLine.setLength(data->labelWidth / 2);
    Swap(baseLine);
    baseLine.setLength(baseLine.length() * 2);
    baseLine = SimpleParallelLine(baseLine.p1(), baseLine.p2(), -(margin + data->labelHeight));

    posData.font = LabelOutlineFont();
    QFontMetrics const fm(posData.font);
    posData.label = fm.elidedText(FoldLineLabel(), Qt::ElideRight, qFloor(data->labelWidth));

    if (m_alignment & Qt::AlignHCenter) // NOLINT(readability-implicit-bool-conversion)
    {
        qreal const shift = (data->labelWidth - fm.horizontalAdvance(posData.label)) / 2;
        baseLine.setLength(baseLine.length() - shift);
    }
    else if (m_alignment & Qt::AlignRight) // NOLINT(readability-implicit-bool-conversion)
    {
        qreal const shift = data->labelWidth - fm.horizontalAdvance(posData.label);
        baseLine.setLength(baseLine.length() - shift);
    }

    posData.pos = baseLine.p2();
    posData.angle = QLineF(center, data->base.p1()).angle();
    ok = true;

    return posData;
}

//---------------------------------------------------------------------------------------------------------------------
void VFoldLine::UpdateFoldLineLabel(QGraphicsSimpleTextItem *item) const
{
    if (item == nullptr)
    {
        return;
    }

    if (m_foldLine.isNull())
    {
        item->setVisible(false);
        return;
    }

    if (!HasLabel())
    {
        item->setVisible(false);
        return;
    }

    item->setVisible(true);

    bool ok = false;
    FoldLabelPosData const labelPos = LabelPosition(ok);

    if (not ok)
    {
        item->setVisible(false);
        return;
    }

    item->setFont(labelPos.font);
    item->setText(labelPos.label);

    QRectF const rect = item->boundingRect();

    QTransform matrix;
    if (m_verticallyFlipped != m_horizontallyFlipped)
    {
        // Vertical-only and horizontal-only flips need DIFFERENT rotation offsets here, not a
        // shared one -- confirmed empirically (real side-by-side renders), not assumed: a
        // vertically flipped label must land 180 degrees from the non-flipped case (matching
        // "both flipped", which already naturally lands there, since two reflections compose to
        // a pure 180-degree rotation the label just follows), but a horizontally flipped label
        // must match the non-flipped case exactly, 0 degrees extra. This asymmetry traces back to
        // TwoArrowsTextAbove's label offset being an inherently Y-directional ("above" the fold
        // line) concept: a Y-mirror (horizontal flip) interacts with that offset differently than
        // an X-mirror (vertical flip) does. An earlier version of this assumed the two axes were
        // symmetric and unified them onto a single shared offset, which was wrong -- see git
        // history for the two-piece renders that exposed it in each direction independently.
        //
        // The correction's scale still has to be applied AFTER the rotation (outermost), not
        // before it: scale-before-rotate cancels m_matrix's mirror but composes to an
        // uncontrolled, angle-dependent extra rotation instead of a fixed offset.
        //
        // With the scale outermost, the only freedom left is translation, so the anchor is the
        // item's own bounding-rect center rotated (not mirrored) into place: refRectCenter is
        // where the *non-flipped* transform would put that same center point (using the
        // non-flipped rotation, not whatever offset is added below -- the position match holds
        // for any rotation angle used inside the correction, since it only pins down where the
        // rotation's own center lands, verified against an asymmetric test shape, not just a
        // symmetric rect whose 180-degree symmetry can hide an extra-rotation bug that real
        // glyphs would reveal).
        qreal const extraRotationDeg = m_verticallyFlipped ? 180. : 0.;

        QPointF const rectCenter = rect.center();

        QTransform refTransform;
        refTransform.translate(labelPos.pos.x(), labelPos.pos.y());
        refTransform.rotate(-labelPos.angle);
        QPointF const refRectCenter = refTransform.map(rectCenter);

        matrix.translate(refRectCenter.x(), refRectCenter.y());
        matrix.scale(m_verticallyFlipped ? -1 : 1, m_horizontallyFlipped ? -1 : 1);
        matrix.rotate(-labelPos.angle + extraRotationDeg);
        matrix.translate(-rectCenter.x(), -rectCenter.y());
    }
    else
    {
        matrix.translate(labelPos.pos.x(), labelPos.pos.y());
        matrix.rotate(-labelPos.angle);
    }

    matrix *= m_matrix;

    item->setTransform(matrix);
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::DefaultFoldLineLabel() -> QString
{
    QSharedPointer<VTranslator> const phTr = VAbstractApplication::VApp()->GetPlaceholderTranslator();
    return phTr->translate("FoldLine", QT_TRANSLATE_NOOP("FoldLine", "Cut On Fold"));
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::FoldLineTwoArrowsPath() const -> QVector<QPainterPath>
{
    if (m_foldLine.isNull())
    {
        return {};
    }

    ArrowsTextPosData const data = TwoArrowsData();
    return {TwoArrowsPath(data.arrowsWidth, data.arrowsHeight)};
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::FoldLineTwoArrowsTextAbovePath() const -> QVector<QPainterPath>
{
    if (m_foldLine.isNull())
    {
        return {};
    }

    ArrowsTextPosData const data = TwoArrowsTextAboveData();

    QVector<QPainterPath> shape;
    shape.append(TwoArrowsPath(data.arrowsWidth, data.arrowsHeight));

    if (const VCommonSettings *settings = VAbstractApplication::VApp()->Settings(); settings->GetSingleLineFonts())
    {
        shape.append(SVGFontLabel(data.base, data.labelWidth, data.labelHeight));
    }
    else
    {
        shape.append(OutlineFontLabel(data.base, data.labelWidth, data.labelHeight));
    }

    return shape;
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::TrueCenter(const QLineF &base, qreal width) const -> QPointF
{
    if (base.isNull())
    {
        return {};
    }

    width = qMin(width, base.length());

    if (VFuzzyComparePossibleNulls(base.length(), width) || qFuzzyIsNull(width))
    {
        return base.center();
    }

    QLineF seg1 = base;
    seg1.setLength(width / 2.);

    qreal const seg1T = seg1.length() / base.length();

    if (m_center >= 0 && m_center < seg1T)
    {
        return seg1.p2();
    }

    QLineF seg2(base.p2(), base.p1());
    seg2.setLength(width / 2.);

    qreal const seg2T = seg2.length() / base.length();

    if (m_center >= (1 - seg2T) && m_center <= 1)
    {
        return seg2.p2();
    }

    return base.pointAt(m_center);
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::FoldLineLabel() const -> QString
{
    if (!m_label.isEmpty())
    {
        return m_label;
    }

    return DefaultFoldLineLabel();
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::OutlineFontLabel(const QLineF &base, qreal width, qreal textHeight) const -> QPainterPath
{
    if (base.isNull())
    {
        return {};
    }

    // Same rule as LabelPosition: the arrow height does not move the text.
    qreal const margin = defLabelMargin;

    QPointF const center = TrueCenter(base, width);

    QLineF baseLine(center, base.p1());
    baseLine.setAngle(baseLine.angle() + 90);
    baseLine.setLength(margin);
    Swap(baseLine);
    baseLine.setAngle(baseLine.angle() - 90 - QLineF(base.p2(), base.p1()).angle());
    baseLine.setLength(width / 2);
    QPointF const rotationCenter = baseLine.p1();
    Swap(baseLine);
    baseLine.setLength(width);
    Swap(baseLine);

    QFont const font = LabelOutlineFont();
    QFontMetrics const fm(font);
    QString const label = fm.elidedText(FoldLineLabel(), Qt::ElideRight, qFloor(width));

    if (m_alignment & Qt::AlignHCenter) // NOLINT(readability-implicit-bool-conversion)
    {
        qreal const shift = (width - fm.horizontalAdvance(label)) / 2;
        baseLine.setLength(baseLine.length() - shift);
    }
    else if (m_alignment & Qt::AlignRight) // NOLINT(readability-implicit-bool-conversion)
    {
        qreal const shift = width - fm.horizontalAdvance(label);
        baseLine.setLength(baseLine.length() - shift);
    }

    QPainterPath labelPath;

    if (const VCommonSettings *settings = VAbstractApplication::VApp()->Settings();
        settings->GetSingleStrokeOutlineFont())
    {
        VSingleLineOutlineChar const corrector(font);
        if (!corrector.IsPopulated())
        {
            corrector.LoadCorrections(settings->GetPathFontCorrections());
        }

        int w = 0;
        for (auto c : std::as_const(label))
        {
            labelPath.addPath(corrector.DrawChar(w, static_cast<qreal>(fm.ascent()), c));
            w += fm.horizontalAdvance(c);
        }

        QTransform matrix;

        if (m_verticallyFlipped != m_horizontallyFlipped)
        {
            // Vertical-only and horizontal-only flips need DIFFERENT rotation offsets here, not
            // a shared one -- see UpdateFoldLineLabel() for the full reasoning (confirmed via
            // real side-by-side renders, not assumed) and why the scale must be outermost.
            qreal const extraRotationDeg = m_verticallyFlipped ? 180. : 0.;

            QPointF const rectCenter = labelPath.boundingRect().center();

            QTransform refTransform;
            refTransform.translate(rotationCenter.x(), rotationCenter.y());
            refTransform.rotate(-QLineF(center, base.p1()).angle());
            refTransform.translate(-rotationCenter.x(), -rotationCenter.y());
            refTransform.translate(baseLine.p2().x(), baseLine.p2().y() - textHeight);
            QPointF const refRectCenter = refTransform.map(rectCenter);

            matrix.translate(refRectCenter.x(), refRectCenter.y());
            matrix.scale(m_verticallyFlipped ? -1 : 1, m_horizontallyFlipped ? -1 : 1);
            matrix.rotate(-QLineF(center, base.p1()).angle() + extraRotationDeg);
            matrix.translate(-rectCenter.x(), -rectCenter.y());
        }
        else
        {
            matrix.translate(rotationCenter.x(), rotationCenter.y());
            matrix.rotate(-QLineF(center, base.p1()).angle());
            matrix.translate(-rotationCenter.x(), -rotationCenter.y());
            matrix.translate(baseLine.p2().x(), baseLine.p2().y() - textHeight);
        }

        matrix *= m_matrix;
        labelPath = matrix.map(labelPath);
    }
    else
    {
        Swap(baseLine);
        baseLine.setAngle(baseLine.angle() + 90);
        baseLine.setLength(fm.descent() + CmToPixel(0.2));
        labelPath.addText(QPointF(), font, label);

        QTransform matrix;

        if (m_verticallyFlipped != m_horizontallyFlipped)
        {
            // See the single-stroke branch above for why the scale must be outermost, the
            // rotation offset differs by flip axis, and the anchor must be the glyph's own
            // bounding-rect center mapped through the non-flipped transform.
            qreal const extraRotationDeg = m_verticallyFlipped ? 180. : 0.;

            QPointF const rectCenter = labelPath.boundingRect().center();

            QTransform refTransform;
            refTransform.translate(rotationCenter.x(), rotationCenter.y());
            refTransform.rotate(-QLineF(center, base.p1()).angle());
            refTransform.translate(-rotationCenter.x(), -rotationCenter.y());
            refTransform.translate(baseLine.p2().x(), baseLine.p2().y());
            QPointF const refRectCenter = refTransform.map(rectCenter);

            matrix.translate(refRectCenter.x(), refRectCenter.y());
            matrix.scale(m_verticallyFlipped ? -1 : 1, m_horizontallyFlipped ? -1 : 1);
            matrix.rotate(-QLineF(center, base.p1()).angle() + extraRotationDeg);
            matrix.translate(-rectCenter.x(), -rectCenter.y());
        }
        else
        {
            matrix.translate(rotationCenter.x(), rotationCenter.y());
            matrix.rotate(-QLineF(center, base.p1()).angle());
            matrix.translate(-rotationCenter.x(), -rotationCenter.y());
            matrix.translate(baseLine.p2().x(), baseLine.p2().y());
        }

        matrix *= m_matrix;
        labelPath = matrix.map(labelPath);
    }

    return labelPath;
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::SVGFontLabel(const QLineF &base, qreal width, qreal textHeight) const -> QPainterPath
{
    if (base.isNull())
    {
        return {};
    }

    VSvgFontEngine engine;
    if (!LabelSVGFontEngine(engine))
    {
        return OutlineFontLabel(base, width, textHeight);
    }

    // Same rule as LabelPosition: the arrow height does not move the text.
    qreal const margin = defLabelMargin;

    QPointF const center = TrueCenter(base, width);

    QLineF baseLine(center, base.p1());
    baseLine.setAngle(baseLine.angle() + 90);
    baseLine.setLength(margin);
    Swap(baseLine);
    baseLine.setAngle(baseLine.angle() - 90 - QLineF(base.p2(), base.p1()).angle());
    baseLine.setLength(width / 2);
    QPointF const rotationCenter = baseLine.p1();
    Swap(baseLine);
    baseLine.setLength(width);
    Swap(baseLine);

    QString const label = engine.ElidedText(FoldLineLabel(), SVGTextElideMode::ElideRight, qFloor(width));

    if (m_alignment & Qt::AlignHCenter) // NOLINT(readability-implicit-bool-conversion)
    {
        qreal const shift = (width - engine.TextWidth(label)) / 2;
        baseLine.setLength(baseLine.length() - shift);
    }
    else if (m_alignment & Qt::AlignRight) // NOLINT(readability-implicit-bool-conversion)
    {
        qreal const shift = width - engine.TextWidth(label);
        baseLine.setLength(baseLine.length() - shift);
    }

    QPainterPath labelPath = engine.DrawPath(QPointF(), label);

    QTransform matrix;

    if (m_verticallyFlipped != m_horizontallyFlipped)
    {
        // See OutlineFontLabel's single-stroke branch for the full reasoning: vertical-only and
        // horizontal-only flips need DIFFERENT rotation offsets (confirmed via real side-by-side
        // renders, not assumed), and the correction's scale has to be applied AFTER the rotation
        // (outermost) -- scale-before-rotate cancels m_matrix's mirror but composes to an
        // uncontrolled, angle-dependent extra rotation instead of a fixed offset. With scale
        // outermost, the anchor is the glyph's own bounding-rect center mapped through the
        // non-flipped transform, which keeps the footprint exactly where the non-flipped
        // footprint mirrors to.
        qreal const extraRotationDeg = m_verticallyFlipped ? 180. : 0.;

        QPointF const rectCenter = labelPath.boundingRect().center();

        QTransform refTransform;
        refTransform.translate(rotationCenter.x(), rotationCenter.y());
        refTransform.rotate(-QLineF(center, base.p1()).angle());
        refTransform.translate(-rotationCenter.x(), -rotationCenter.y());
        refTransform.translate(baseLine.p2().x(), baseLine.p2().y() - textHeight);
        QPointF const refRectCenter = refTransform.map(rectCenter);

        matrix.translate(refRectCenter.x(), refRectCenter.y());
        matrix.scale(m_verticallyFlipped ? -1 : 1, m_horizontallyFlipped ? -1 : 1);
        matrix.rotate(-QLineF(center, base.p1()).angle() + extraRotationDeg);
        matrix.translate(-rectCenter.x(), -rectCenter.y());
    }
    else
    {
        matrix.translate(rotationCenter.x(), rotationCenter.y());
        matrix.rotate(-QLineF(center, base.p1()).angle());
        matrix.translate(-rotationCenter.x(), -rotationCenter.y());
        matrix.translate(baseLine.p2().x(), baseLine.p2().y() - textHeight);
    }

    matrix *= m_matrix;
    labelPath = matrix.map(labelPath);

    return labelPath;
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::LabelTextHeight() const -> qreal
{
    if (const VCommonSettings *settings = VAbstractApplication::VApp()->Settings(); settings->GetSingleLineFonts())
    {
        if (VSvgFontEngine engine; LabelSVGFontEngine(engine))
        {
            return engine.FontHeight();
        }
    }

    return QFontMetrics(LabelOutlineFont()).height();
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::LabelSVGFontEngine(VSvgFontEngine &engine) const -> bool
{
    VSvgFontDatabase  const*db = VAbstractApplication::VApp()->SVGFontDatabase();
    VSvgFontEngine const svgEngine =
        db->FontEngine(m_svgFont, SVGFontStyle::Normal, SVGFontWeight::Normal, static_cast<int>(m_svgFontSize));
    VSvgFont svgFont = svgEngine.Font();
    if (!svgFont.IsValid())
    {
        QString const errorMsg = tr("Invalid SVG font '%1'. Fallback to outline font.").arg(svgFont.Name());
        VAbstractApplication::VApp()->IsPedantic()
            ? throw VException(errorMsg)
            : qWarning() << VAbstractApplication::warningMessageSignature + errorMsg;
        return false;
    }

    svgFont.SetBold(m_bold);
    svgFont.SetItalic(m_italic);

    engine = db->FontEngine(svgFont);
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::LabelOutlineFont() const -> QFont
{
    QFont font = m_outlineFont;
    font.setItalic(m_italic);
    if (m_bold)
    {
        font.setBold(true);
    }
    return font;
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::TwoArrowsPath(qreal width, qreal height) const -> QPainterPath
{
    QVector<QVector<QPointF>> const shape = TwoArrowsPoints(width, height);

    if (shape.size() != 3)
    {
        return {};
    }

    QPainterPath path;
    path.addPolygon(QPolygonF(shape.constFirst()));

    if (shape.at(1).size() == 2)
    {
        path.moveTo(shape.at(1).constFirst());
        path.lineTo(shape.at(1).constLast());
    }

    path.addPolygon(QPolygonF(shape.constLast()));
    path.setFillRule(Qt::OddEvenFill);

    return path;
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::TwoArrowsPoints(qreal width, qreal height) const -> QVector<QVector<QPointF>>
{
    if (m_foldLine.isNull())
    {
        return {};
    }

    QPointF const center = TrueCenter(m_foldLine, width);

    QLineF base1(center, m_foldLine.p1());
    base1.setLength(width / 2.);
    Swap(base1);
    base1.setAngle(base1.angle() - 90);
    base1.setLength(height);

    QLineF base2(center, m_foldLine.p2());
    base2.setLength(width / 2.);
    Swap(base2);
    base2.setAngle(base2.angle() + 90);
    base2.setLength(height);

    VPieceGrainline const arrow1(base1, GrainlineArrowDirection::oneWayDown);
    VPieceGrainline const arrow2(base2, GrainlineArrowDirection::oneWayDown);

    GrainlineShape const shape1 = arrow1.Shape();
    GrainlineShape const shape2 = arrow2.Shape();
    if (shape1.isEmpty() || shape2.isEmpty())
    {
        return {};
    }

    QVector<QPointF> arrow1Shape = shape1.constFirst();
    QVector<QPointF> arrow2Shape = shape2.constFirst();
    QLineF line(arrow1Shape.constLast(), arrow2Shape.constLast());
    std::reverse(arrow2Shape.begin(), arrow2Shape.end());

    std::transform(arrow1Shape.begin(), arrow1Shape.end(), arrow1Shape.begin(),
                   [this](const QPointF &point) { return m_matrix.map(point); });
    line = m_matrix.map(line);
    std::transform(arrow2Shape.begin(), arrow2Shape.end(), arrow2Shape.begin(),
                   [this](const QPointF &point) { return m_matrix.map(point); });

    return {arrow1Shape, {line.p1(), line.p2()}, arrow2Shape};
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::TwoArrowsData() const -> ArrowsTextPosData
{
    ArrowsTextPosData data{};
    data.arrowsWidth = EffectiveWidth();
    data.arrowsHeight = ResolvedHeight();
    return data;
}

//---------------------------------------------------------------------------------------------------------------------
auto VFoldLine::TwoArrowsTextAboveData() const -> ArrowsTextPosData
{
    const qreal defLabelHeight = LabelTextHeight();

    QT_WARNING_PUSH
    QT_WARNING_DISABLE_GCC("-Wnoexcept")

    ArrowsTextPosData data;

    QT_WARNING_POP

    data.arrowsWidth = EffectiveWidth();
    data.labelWidth = data.arrowsWidth;
    // The height is the height of the ARROWS, and nothing else — the same meaning whether it was set
    // or left automatic. It used to be read as a total (arrows + label) on the manual path only, so
    // typing the value the field already displayed produced a SMALLER marker than leaving it alone.
    // The floor only exists to keep the glyph from collapsing on its fixed arrowhead.
    data.arrowsHeight = qFuzzyIsNull(m_height) ? defArrowsHeight : qMax(m_height, minArrowsHeight);
    data.labelHeight = defLabelHeight;
    data.base = SimpleParallelLine(m_foldLine.p1(), m_foldLine.p2(), -data.arrowsHeight);
    return data;
}

