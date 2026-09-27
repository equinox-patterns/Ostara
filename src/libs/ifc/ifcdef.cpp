/************************************************************************
 **
 **  @file   ifcdef.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   11 6, 2015
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2015 Valentina project
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
// Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>

#include "ifcdef.h"

#include <QBrush>
#include <QIcon>
#include <QMap>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QPair>

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

const QString CustomMSign = QStringLiteral("@");
const QString CustomIncrSign = QStringLiteral("#");

const QString AttrType = QStringLiteral("type");
const QString AttrMx = QStringLiteral("mx");
const QString AttrMy = QStringLiteral("my");
const QString AttrNx = QStringLiteral("nx");
const QString AttrNy = QStringLiteral("ny");
const QString AttrName = QStringLiteral("name");
const QString AttrPoints = QStringLiteral("points");
const QString AttrUUID = QStringLiteral("uuid");
const QString AttrGradedAlignH = QStringLiteral("gradedAlignH");
const QString AttrGradedAlignV = QStringLiteral("gradedAlignV");
const QString AttrGradedAlignPoint = QStringLiteral("gradedAlignPoint");
const QString AttrGradedAlignPointMember = QStringLiteral("gradedAlignPointMember");
const QString AttrGradedAlignOnPoint = QStringLiteral("gradedAlignOnPoint");
const QString AttrPieceNumber = QStringLiteral("pieceNumber");
const QString AttrCutFabricQty = QStringLiteral("cutFabricQty");
const QString AttrCutFabricOnFold = QStringLiteral("cutFabricOnFold");
const QString AttrCutInterfacingQty = QStringLiteral("cutInterfacingQty");
const QString AttrCutInterfacingOnFold = QStringLiteral("cutInterfacingOnFold");
const QString AttrCutLiningQty = QStringLiteral("cutLiningQty");
const QString AttrCutLiningOnFold = QStringLiteral("cutLiningOnFold");
const QString AttrFreeText = QStringLiteral("freeText");
const QString AttrMx1 = QStringLiteral("mx1");
const QString AttrMy1 = QStringLiteral("my1");
const QString AttrName1 = QStringLiteral("name1");
const QString AttrMx2 = QStringLiteral("mx2");
const QString AttrMy2 = QStringLiteral("my2");
const QString AttrName2 = QStringLiteral("name2");
const QString AttrMx3 = QStringLiteral("mx3");
const QString AttrMy3 = QStringLiteral("my3");
const QString AttrName3 = QStringLiteral("name3");
const QString AttrBaseLineP1 = QStringLiteral("baseLineP1");
const QString AttrBaseLineP2 = QStringLiteral("baseLineP2");
const QString AttrDartP1 = QStringLiteral("dartP1");
const QString AttrDartP2 = QStringLiteral("dartP2");
const QString AttrDartP3 = QStringLiteral("dartP3");
const QString AttrX = QStringLiteral("x");
const QString AttrY = QStringLiteral("y");
const QString AttrTypeLine = QStringLiteral("typeLine");
const QString AttrCut = QStringLiteral("cut");
const QString AttrLength = QStringLiteral("length");
const QString AttrBasePoint = QStringLiteral("basePoint");
const QString AttrFirstPoint = QStringLiteral("firstPoint");
const QString AttrSecondPoint = QStringLiteral("secondPoint");
const QString AttrThirdPoint = QStringLiteral("thirdPoint");
const QString AttrCenter = QStringLiteral("center");
const QString AttrRadius = QStringLiteral("radius");
const QString AttrRadius1 = QStringLiteral("radius1");
const QString AttrRadius2 = QStringLiteral("radius2");
const QString AttrAngle = QStringLiteral("angle");
const QString AttrAngle1 = QStringLiteral("angle1");
const QString AttrAngle2 = QStringLiteral("angle2");
const QString AttrMovingPoint = QStringLiteral("movingPoint");
const QString AttrTargetPoint = QStringLiteral("targetPoint");
const QString AttrClosingDivisor = QStringLiteral("closingDivisor");
const QString AttrApex = QStringLiteral("apex");
const QString AttrExtendPoint = QStringLiteral("extendPoint");
const QString AttrDrawCap = QStringLiteral("drawCap");
const QString AttrLength1 = QStringLiteral("length1");
const QString AttrLength2 = QStringLiteral("length2");
const QString AttrP1Line = QStringLiteral("p1Line");
const QString AttrP2Line = QStringLiteral("p2Line");
const QString AttrP1Line1 = QStringLiteral("p1Line1");
const QString AttrP2Line1 = QStringLiteral("p2Line1");
const QString AttrP1Line2 = QStringLiteral("p1Line2");
const QString AttrP2Line2 = QStringLiteral("p2Line2");
const QString AttrPShoulder = QStringLiteral("pShoulder");
const QString AttrPoint1 = QStringLiteral("point1");
const QString AttrPoint2 = QStringLiteral("point2");
const QString AttrPoint3 = QStringLiteral("point3");
const QString AttrPoint4 = QStringLiteral("point4");
const QString AttrPoint5 = QStringLiteral("point5");
const QString AttrPoint6 = QStringLiteral("point6");
const QString AttrPoint7 = QStringLiteral("point7");
const QString AttrCarrurePoint = QStringLiteral("carrurePoint");
const QString AttrKAsm1 = QStringLiteral("kAsm1");
const QString AttrKAsm2 = QStringLiteral("kAsm2");
const QString AttrKCurve = QStringLiteral("kCurve");
const QString AttrDuplicate = QStringLiteral("duplicate");
const QString AttrAScale = QStringLiteral("aScale");
const QString TagPathPoint = QStringLiteral("pathPoint");
const QString AttrPSpline = QStringLiteral("pSpline");
const QString AttrAxisP1 = QStringLiteral("axisP1");
const QString AttrAxisP2 = QStringLiteral("axisP2");
const QString AttrCurve = QStringLiteral("curve");
const QString AttrCurve1 = QStringLiteral("curve1");
const QString AttrCurve2 = QStringLiteral("curve2");
const QString AttrLineColor = QStringLiteral("lineColor");
const QString AttrColor = QStringLiteral("color");
const QString AttrPenStyle = QStringLiteral("penStyle");
const QString AttrFirstArc = QStringLiteral("firstArc");
const QString AttrSecondArc = QStringLiteral("secondArc");
const QString AttrCrossPoint = QStringLiteral("crossPoint");
const QString AttrVCrossPoint = QStringLiteral("vCrossPoint");
const QString AttrHCrossPoint = QStringLiteral("hCrossPoint");
const QString AttrAxisType = QStringLiteral("axisType");
const QString AttrC1Center = QStringLiteral("c1Center");
const QString AttrC2Center = QStringLiteral("c2Center");
const QString AttrC1Radius = QStringLiteral("c1Radius");
const QString AttrC2Radius = QStringLiteral("c2Radius");
const QString AttrCCenter = QStringLiteral("cCenter");
const QString AttrTangent = QStringLiteral("tangent");
const QString AttrCRadius = QStringLiteral("cRadius");
const QString AttrArc = QStringLiteral("arc");
const QString AttrSpline = QStringLiteral("spline");
const QString AttrSuffix = QStringLiteral("suffix");
const QString AttrItem = QStringLiteral("item");
const QString AttrIdObject = QStringLiteral("idObject");
const QString AttrInLayout = QStringLiteral("inLayout");
const QString AttrForceFlipping = QStringLiteral("forceFlipping");
const QString AttrSymmetricalCopy = QStringLiteral("symmetricalCopy");
const QString AttrSewLineOnDrawing = QStringLiteral("sewLineOnDrawing");
const QString AttrRotationAngle = QStringLiteral("rotationAngle");
const QString AttrClosed = QStringLiteral("closed");
// C++ identifier keeps the Valentina name "Label" (= point name); the XML attribute is "showPointName"
// to disambiguate the point-name label from piece/segment labels.
const QString AttrShowLabel = QStringLiteral("showPointName");
const QString AttrShowLabel1 = QStringLiteral("showPointName1");
const QString AttrShowLabel2 = QStringLiteral("showPointName2");
const QString AttrShowLabel3 = QStringLiteral("showPointName3");
const QString AttrWidth = QStringLiteral("width");
const QString AttrHeight = QStringLiteral("height");
const QString AttrPlaceLabelType = QStringLiteral("placeLabelType");
const QString AttrVersion = QStringLiteral("version");
const QString AttrFirstToContour = QStringLiteral("firstToContour");
const QString AttrLastToContour = QStringLiteral("lastToContour");
const QString AttrNotes = QStringLiteral("notes");
const QString AttrAlias = QStringLiteral("alias");
const QString AttrAlias1 = QStringLiteral("alias1");
const QString AttrAlias2 = QStringLiteral("alias2");
const QString AttrCurve1Name1 = QStringLiteral("curve1Name1");
const QString AttrCurve1Name2 = QStringLiteral("curve1Name2");
const QString AttrCurve2Name1 = QStringLiteral("curve2Name1");
const QString AttrCurve2Name2 = QStringLiteral("curve2Name2");
const QString AttrCurve1Alias1 = QStringLiteral("curve1Alias1");
const QString AttrCurve1Alias2 = QStringLiteral("curve1Alias2");
const QString AttrCurve2Alias1 = QStringLiteral("curve2Alias1");
const QString AttrCurve2Alias2 = QStringLiteral("curve2Alias2");
const QString AttrLayoutVersion = QStringLiteral("version");
const QString AttrKMVersion = QStringLiteral("version");
const QString AttrNotMirrored = QStringLiteral("notMirrored");
const QString AttrOffset = QStringLiteral("offset");
const QString AttrDescription = QStringLiteral("description");
const QString AttrCurveName1 = QStringLiteral("curveName1");
const QString AttrCurveName2 = QStringLiteral("curveName2");
const QString AttrSegment1Id = QStringLiteral("segment1Id");
const QString AttrSegment2Id = QStringLiteral("segment2Id");
const QString AttrShowTwoSegments = QStringLiteral("showTwoSegments");
const QString AttrSegment3Id = QStringLiteral("segment3Id");
const QString AttrSegment4Id = QStringLiteral("segment4Id");
const QString AttrSegment1Mx = QStringLiteral("segment1Mx");
const QString AttrSegment1My = QStringLiteral("segment1My");
const QString AttrSegment2Mx = QStringLiteral("segment2Mx");
const QString AttrSegment2My = QStringLiteral("segment2My");
const QString AttrSegment3Mx = QStringLiteral("segment3Mx");
const QString AttrSegment3My = QStringLiteral("segment3My");
const QString AttrSegment4Mx = QStringLiteral("segment4Mx");
const QString AttrSegment4My = QStringLiteral("segment4My");

const QString TypeLineDefault = QStringLiteral("default");
const QString TypeLineNone = QStringLiteral("none");
const QString TypeLineLine = QStringLiteral("hair");
const QString TypeLineDashLine = QStringLiteral("dashLine");
const QString TypeLineDotLine = QStringLiteral("dotLine");
const QString TypeLineDashDotLine = QStringLiteral("dashDotLine");
const QString TypeLineDashDotDotLine = QStringLiteral("dashDotDotLine");

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Styles return list of all line styles.
 * @return list of all line styles.
 */
auto StylesList() -> QStringList
{
    return {TypeLineNone, TypeLineLine, TypeLineDashLine, TypeLineDotLine, TypeLineDashDotLine, TypeLineDashDotDotLine};
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief LineStyle return pen style for current line style.
 * @return pen style.
 */
auto LineStyleToPenStyle(const QString &typeLine) -> Qt::PenStyle
{
    const QStringList styles = StylesList();
    switch (styles.indexOf(typeLine))
    {
        case 0: // TypeLineNone
            return Qt::NoPen;
        case 2: // TypeLineDashLine
            return Qt::DashLine;
        case 3: // TypeLineDotLine
            return Qt::DotLine;
        case 4: // TypeLineDashDotLine
            return Qt::DashDotLine;
        case 5: // TypeLineDashDotDotLine
            return Qt::DashDotDotLine;
        default:
            return Qt::SolidLine;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto PenStyleToLineStyle(Qt::PenStyle penStyle) -> QString
{
    switch (penStyle)
    {
        case Qt::NoPen:
            return TypeLineNone;
        case Qt::DashLine:
            return TypeLineDashLine;
        case Qt::DotLine:
            return TypeLineDotLine;
        case Qt::DashDotLine:
            return TypeLineDashDotLine;
        case Qt::DashDotDotLine:
            return TypeLineDashDotDotLine;
        default:
            break;
    }

    return TypeLineLine;
}

//---------------------------------------------------------------------------------------------------------------------
auto PenStylePic(QColor backgroundColor, QColor textColor, Qt::PenStyle style) -> QIcon
{
    QPixmap pix(80, 14);
    pix.fill(backgroundColor);

    QPen const pen(textColor, 2.5, style);

    QPainter painter(&pix);
    painter.setPen(pen);
    painter.drawLine(2, 7, 78, 7);

    painter.setPen(QPen(textColor, 0.25));
    painter.drawRect(0, 0, pix.width() - 1, pix.height() - 1);

    return {pix};
}

//---------------------------------------------------------------------------------------------------------------------
auto LineStylesPics(QColor backgroundColor, QColor textColor) -> QMap<QString, QIcon>
{
    QMap<QString, QIcon> map;
    const QStringList styles = StylesList();

    for (const auto &s : styles)
    {
        map.insert(s, PenStylePic(backgroundColor, textColor, LineStyleToPenStyle(s)));
    }
    return map;
}

//---------------------------------------------------------------------------------------------------------------------
auto CurvePenStylesPics(QColor backgroundColor, QColor textColor) -> QMap<QString, QIcon>
{
    QMap<QString, QIcon> map = LineStylesPics(backgroundColor, textColor);
    map.remove(TypeLineNone);
    return map;
}

const QString ColorDefault = QStringLiteral("default");
const QString ColorBlack = QStringLiteral("black");
const QString ColorLightBlue = QStringLiteral("lightBlue");
const QString ColorDarkBlue = QStringLiteral("darkBlue");
const QString ColorPink = QStringLiteral("pink");
const QString ColorPurple = QStringLiteral("purple");
const QString ColorYellow = QStringLiteral("yellow");
const QString ColorOrange = QStringLiteral("orange");
const QString ColorLightGreen = QStringLiteral("lightGreen");
const QString ColorDarkGreen = QStringLiteral("darkGreen");
const QString ColorBrown = QStringLiteral("brown");

// Dark-mode fixed hex values.
const QString ColorBlackDark      = QStringLiteral("#E5E5E5");
const QString ColorLightBlueDark  = QStringLiteral("#38B5EE");
const QString ColorDarkBlueDark   = QStringLiteral("#0062FF");
const QString ColorPinkDark       = QStringLiteral("#FF38A0");
const QString ColorPurpleDark     = QStringLiteral("#A070E0");
const QString ColorYellowDark     = QStringLiteral("#FFD640");
const QString ColorOrangeDark     = QStringLiteral("#FF8838");
const QString ColorLightGreenDark = QStringLiteral("#44D838");
const QString ColorDarkGreenDark  = QStringLiteral("#339300");
const QString ColorBrownDark      = QStringLiteral("#C05038");

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ColorNameToQColor maps a color constant to its Ostara palette hex value.
 * @param colorName one of the ColorXxx constants.
 * @return the mapped QColor.
 */
auto ColorNameToQColor(const QString &colorName) -> QColor
{
    static const QMap<QString, QColor> colorMap = {
        // ── New Ostara palette ──
        {ColorBlack,      QColor(0x00, 0x00, 0x00)},
        {ColorLightBlue,  QColor(0x0E, 0x9C, 0xD8)},
        {ColorDarkBlue,   QColor(0x1C, 0x4D, 0xB5)},
        {ColorPink,       QColor(0xEC, 0x00, 0x78)},
        {ColorPurple,     QColor(0x6C, 0x2D, 0xA8)},
        {ColorYellow,     QColor(0xF5, 0xC4, 0x00)},
        {ColorOrange,     QColor(0xED, 0x5C, 0x00)},
        {ColorLightGreen, QColor(0x28, 0xA8, 0x28)},
        {ColorDarkGreen,  QColor(0x18, 0x71, 0x3C)},
        {ColorBrown,      QColor(0x6E, 0x3E, 0x08)},
        // ── Legacy Valentina → Ostara mapping (backward compat) ──
        {QStringLiteral("green"),           QColor(0x28, 0xA8, 0x28)}, // → light green
        {QStringLiteral("blue"),            QColor(0x0E, 0x9C, 0xD8)}, // → light blue
        {QStringLiteral("darkRed"),         QColor(0xEC, 0x00, 0x78)}, // → pink
        {QStringLiteral("lightsalmon"),     QColor(0xED, 0x5C, 0x00)}, // → orange
        {QStringLiteral("goldenrod"),       QColor(0xF5, 0xC4, 0x00)}, // → yellow
        {QStringLiteral("deeppink"),        QColor(0xEC, 0x00, 0x78)}, // → pink
        {QStringLiteral("violet"),          QColor(0x6C, 0x2D, 0xA8)}, // → purple
        {QStringLiteral("darkviolet"),      QColor(0x1C, 0x4D, 0xB5)}, // → dark blue
        {QStringLiteral("mediumseagreen"),  QColor(0x28, 0xA8, 0x28)}, // → light green
        {QStringLiteral("lime"),            QColor(0x28, 0xA8, 0x28)}, // → light green
        {QStringLiteral("deepskyblue"),     QColor(0x0E, 0x9C, 0xD8)}, // → light blue
        {QStringLiteral("cornflowerblue"),  QColor(0x0E, 0x9C, 0xD8)}, // → light blue
    };
    return colorMap.value(colorName);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ColorNameToQColorForTheme maps a color constant to its light or dark hex value.
 * @param colorName one of the ColorXxx constants.
 * @param dark true for dark-mode hex, false for light-mode hex.
 * @return the mapped QColor.
 */
auto ColorNameToQColorForTheme(const QString &colorName, bool dark) -> QColor
{
    if (!dark)
    {
        return ColorNameToQColor(colorName);
    }

    static const QMap<QString, QColor> darkMap = {
        {ColorBlack,      QColor(0xE5, 0xE5, 0xE5)},
        {ColorLightBlue,  QColor(0x38, 0xB5, 0xEE)},
        {ColorDarkBlue,   QColor(0x00, 0x62, 0xFF)},
        {ColorPink,       QColor(0xFF, 0x38, 0xA0)},
        {ColorPurple,     QColor(0xA0, 0x70, 0xE0)},
        {ColorYellow,     QColor(0xFF, 0xD6, 0x40)},
        {ColorOrange,     QColor(0xFF, 0x88, 0x38)},
        {ColorLightGreen, QColor(0x44, 0xD8, 0x38)},
        {ColorDarkGreen,  QColor(0x33, 0x93, 0x00)},
        {ColorBrown,      QColor(0xC0, 0x50, 0x38)},
    };
    return darkMap.value(colorName);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ColorNameFromQColor reverse-maps a QColor to its color constant name.
 * @param color the QColor to look up.
 * @return the matching ColorXxx constant, or color.name() if no match.
 */
auto ColorNameFromQColor(const QColor &color) -> QString
{
    static const QMap<QRgb, QString> rgbMap = {
        // ── New Ostara palette (light) ──
        {QColor(0x00, 0x00, 0x00).rgb(), ColorBlack},
        {QColor(0x0E, 0x9C, 0xD8).rgb(), ColorLightBlue},
        {QColor(0x1C, 0x4D, 0xB5).rgb(), ColorDarkBlue},
        {QColor(0xEC, 0x00, 0x78).rgb(), ColorPink},
        {QColor(0x6C, 0x2D, 0xA8).rgb(), ColorPurple},
        {QColor(0xF5, 0xC4, 0x00).rgb(), ColorYellow},
        {QColor(0xED, 0x5C, 0x00).rgb(), ColorOrange},
        {QColor(0x28, 0xA8, 0x28).rgb(), ColorLightGreen},
        {QColor(0x18, 0x71, 0x3C).rgb(), ColorDarkGreen},
        {QColor(0x6E, 0x3E, 0x08).rgb(), ColorBrown},
        // ── Dark mode swatches → base name ──
        {QColor(0xE5, 0xE5, 0xE5).rgb(), ColorBlack},
        {QColor(0x38, 0xB5, 0xEE).rgb(), ColorLightBlue},
        {QColor(0x00, 0x62, 0xFF).rgb(), ColorDarkBlue},
        {QColor(0xFF, 0x38, 0xA0).rgb(), ColorPink},
        {QColor(0xA0, 0x70, 0xE0).rgb(), ColorPurple},
        {QColor(0xFF, 0xD6, 0x40).rgb(), ColorYellow},
        {QColor(0xFF, 0x88, 0x38).rgb(), ColorOrange},
        {QColor(0x44, 0xD8, 0x38).rgb(), ColorLightGreen},
        {QColor(0x33, 0x93, 0x00).rgb(), ColorDarkGreen},
        {QColor(0xC0, 0x50, 0x38).rgb(), ColorBrown},
    };
    return rgbMap.value(color.rgb());
}

// variables
const QString measurement_ = QStringLiteral("M_");
const QString increment_ = QStringLiteral("Increment_");
const QString offset_ = QStringLiteral("Offset_");
const QString line_ = QStringLiteral("Line_");
const QString angleLine_ = QStringLiteral("AngleLine_");
const QString spl_ = QStringLiteral(SPL_);
const QString arc_ = QStringLiteral(ARC_);
const QString elarc_ = QStringLiteral(ELARC_);
const QString splPath_V = QStringLiteral("SplPath");
const QString radius_V = QStringLiteral("Radius");
const QString radiusArc_ = radius_V + arc_;
const QString radius1ElArc_ = radius_V + '1'_L1 + elarc_;
const QString radius2ElArc_ = radius_V + '2'_L1 + elarc_;
const QString angle1_V = QStringLiteral("Angle1");
const QString angle2_V = QStringLiteral("Angle2");
const QString c1Length_V = QStringLiteral("C1Length");
const QString c2Length_V = QStringLiteral("C2Length");
const QString c1LengthSpl_ = c1Length_V + spl_;
const QString c2LengthSpl_ = c2Length_V + spl_;
const QString c1LengthSplPath = c1Length_V + splPath_V;
const QString c2LengthSplPath = c2Length_V + splPath_V;
const QString angle1Arc_ = angle1_V + arc_;
const QString angle2Arc_ = angle2_V + arc_;
const QString angle1ElArc_ = angle1_V + elarc_;
const QString angle2ElArc_ = angle2_V + elarc_;
const QString angle1Spl_ = angle1_V + spl_;
const QString angle2Spl_ = angle2_V + spl_;
const QString angle1SplPath = angle1_V + splPath_V;
const QString angle2SplPath = angle2_V + splPath_V;
const QString seg_ = QStringLiteral("Seg_");
const QString currentLength = QStringLiteral("CurrentLength");
const QString currentSeamAllowance = QStringLiteral("CurrentSeamAllowance");
const QString rotation_V = QStringLiteral("Rotation");
const QString rotationElArc_ = rotation_V + elarc_;
const QString pieceArea_ = QStringLiteral("PieceArea_");
const QString pieceSeamLineArea_ = QStringLiteral("PieceSeamLineArea_");

// Simplified prefixes (RoleName step 1)
const QString curve_ = QStringLiteral("Curve_");
const QString angle1Curve_ = QStringLiteral("Angle1Curve_");
const QString angle2Curve_ = QStringLiteral("Angle2Curve_");
const QString handle1Curve_ = QStringLiteral("Handle1Curve_");
const QString handle2Curve_ = QStringLiteral("Handle2Curve_");
const QString radiusSimple_ = QStringLiteral("Radius_");

//---------------------------------------------------------------------------------------------------------------------
QString SimplifiedName(const QString &name)
{
    // Ordered longest-first to avoid partial matches.
    // Each pair: (technical_prefix, simplified_prefix)
    // clang-format off
    static const QList<QPair<QString, QString>> mappings = {
        // SplPath variants (no trailing _) — before Spl variants
        {angle1SplPath, angle1Curve_},
        {angle2SplPath, angle2Curve_},
        {c1LengthSplPath, handle1Curve_},
        {c2LengthSplPath, handle2Curve_},
        {splPath_V, curve_},
        // Simple curve variants (trailing _)
        {angle1Spl_, angle1Curve_},
        {angle2Spl_, angle2Curve_},
        {c1LengthSpl_, handle1Curve_},
        {c2LengthSpl_, handle2Curve_},
        // Radius variants
        {radiusArc_, radiusSimple_},
        {radius1ElArc_, radiusSimple_},
        {radius2ElArc_, radiusSimple_},
        // Simple curve
        {spl_, curve_},
    };
    // clang-format on

    for (const auto &[tech, simple] : mappings)
    {
        if (name.startsWith(tech))
        {
            qsizetype replaceLen = tech.length();
            // When tech prefix doesn't end with _ but simplified does,
            // and the next char is _, consume it to avoid double underscore.
            // "SplPath_A_C" → consume "_" → "Curve_A_C"
            // "SplPath EmmanchureDos" → space, not consumed → "Curve_ EmmanchureDos"
            if (!tech.endsWith(u'_') && simple.endsWith(u'_') && name.length() > replaceLen && name[replaceLen] == u'_')
            {
                replaceLen++;
            }
            QString result = name;
            result.replace(0, replaceLen, simple);
            return result;
        }
    }
    return name; // No simplification needed
}

auto BuilInVariables() -> QStringList
{
    return {measurement_,
            increment_,
            offset_,
            line_,
            angleLine_,
            arc_,
            elarc_,
            spl_,
            splPath_V,
            radiusArc_,
            radius1ElArc_,
            radius2ElArc_,
            angle1Arc_,
            angle2Arc_,
            angle1ElArc_,
            angle2ElArc_,
            angle1Spl_,
            angle2Spl_,
            angle1SplPath,
            angle2SplPath,
            seg_,
            currentLength,
            currentSeamAllowance,
            c1LengthSpl_,
            c2LengthSpl_,
            c1LengthSplPath,
            c2LengthSplPath,
            rotationElArc_,
            pieceArea_,
            pieceSeamLineArea_,
            curve_,
            angle1Curve_,
            angle2Curve_,
            handle1Curve_,
            handle2Curve_,
            radiusSimple_};
}
