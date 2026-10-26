/************************************************************************
 **
 **  @file   vfoldline.h
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
#ifndef VFOLDLINE_H
#define VFOLDLINE_H

#include <QCoreApplication>
#include <QFont>
#include <QLineF>
#include <QPointF>
#include <QString>
#include <QStringList>
#include <QTransform>
#include <QVector>

#include <functional>

#include "../vmisc/def.h"

class QPainterPath;
class QGraphicsSimpleTextItem;
class VPiece;
class VContainer;
class VAbstractPattern;

struct TextPosData
{
    qreal labelWidth{0};
    qreal labelHeight{0};
    QLineF base{};
};

struct ArrowsTextPosData : public TextPosData
{
    qreal arrowsWidth{0};
    qreal arrowsHeight{0};
};

/// @brief One drawn line of a fold line's caption: the words, and where this line's own origin
/// goes. A caption written in several languages is several lines, one item each.
struct FoldLabelLine
{
    QString text{}; // NOLINT(misc-non-private-member-variables-in-classes)
    QPointF pos{};  // NOLINT(misc-non-private-member-variables-in-classes)
};

struct FoldLabelPosData
{
    QFont font{};                   // NOLINT(misc-non-private-member-variables-in-classes)
    QVector<FoldLabelLine> lines{}; // NOLINT(misc-non-private-member-variables-in-classes)
    qreal angle{0};                 // NOLINT(misc-non-private-member-variables-in-classes)
};

class VFoldLine
{
    Q_DECLARE_TR_FUNCTIONS(VFoldLine) // NOLINT

public:
    VFoldLine() = default;
    VFoldLine(const QLineF &line, FoldLineType type);
    VFoldLine(VFoldLine &&) noexcept = default;
    auto operator=(VFoldLine &&) noexcept -> VFoldLine & = default;
    VFoldLine(const VFoldLine &) = default;
    auto operator=(const VFoldLine &) -> VFoldLine & = default;
    ~VFoldLine() = default;

    /**
     * @brief Create builds the fold line of @a piece exactly as the canvas renders it.
     *
     * Single source of truth for the canvas and the layout path: picks the mirror axis (seam
     * allowance or seam line, depending on "Hide main path"), reads the label font from the
     * application settings, and evaluates the three manual fold formulas.
     *
     * @param piece the piece whose fold line is wanted
     * @param data the pattern container the formulas are evaluated against
     * @param doc the pattern, so the generic caption can be asked for in the piece's own cartouche
     *            languages; nullptr is allowed and falls back to the application's default text
     * @return the fold line, ready to be asked for its paths or its values
     */
    static auto Create(const VPiece &piece, const VContainer *data, VAbstractPattern *doc = nullptr) -> VFoldLine;

    /**
     * @brief Ce que cette bibliothèque ne peut pas savoir et que l'application, elle, sait dire :
     * la légende générique dans les langues du cartouche de la pièce.
     *
     * Une foldline sans texte à elle porte un texte générique. L'application sait l'écrire dans
     * chaque langue d'un cartouche ; quel cartouche, et quelles langues, est une connaissance de
     * l'application et non de cette bibliothèque. Elle est donc injectée, comme les autres
     * résolveurs du projet. Un résolveur qui rend une chaîne vide laisse le texte par défaut
     * s'appliquer.
     */
    using OFoldLineLabelResolver = std::function<QString(const VPiece &piece, VAbstractPattern *doc)>;

    static void SetLabelResolver(OFoldLineLabelResolver resolver);

    /**
     * @brief ResolvedLabel est le texte que la foldline dessine : celui de la pièce quand elle en a
     * un, la légende générique autrement.
     *
     * Public parce que le chemin de mise en page construit sa foldline sans passer par Create.
     */
    static auto ResolvedLabel(const VPiece &piece, VAbstractPattern *doc) -> QString;

    /**
     * @brief ApplyManualFoldValues evaluates the three manual fold formulas of @a piece.
     *
     * Kept apart from Create so a caller that needs only the values — the layout path — does not
     * pay for the mirror axis, which is expensive to compute. A value that is not manual is left
     * at its automatic sentinel: 0 for height and width, 0.5 for the center.
     */
    void ApplyManualFoldValues(const VPiece &piece, const VContainer *data);

    void SetHeight(qreal newHeight);
    void SetWidth(qreal newWidth);
    void SetCenterPosition(qreal newCenter);
    void SetLabel(const QString &newLabel);
    void SetLabelFontItalic(bool newItalic);
    void SetLabelFontBold(bool newBold);
    void SetLabelAlignment(int alignment);
    void SetMatrix(const QTransform &matrix);
    void SetVerticallyFlipped(bool value);
    void SetHorizontallyFlipped(bool value);
    void SetOutlineFont(const QFont &font);

    auto Type() const -> FoldLineType;
    auto Label() const -> QString;
    auto OutlineFont() const -> QFont;

    /// Whether @a type draws a text at all. Only TwoArrowsTextAbove does: TwoArrows draws the
    /// arrows alone and None draws nothing. Single source of the predicate, shared by the
    /// exporters and by the dock — the dock needs it for a type it has not built an object from.
    static auto TypeHasLabel(FoldLineType type) -> bool;

    /**
     * @brief HasLabel tells whether this fold line type draws a text at all.
     *
     * See TypeHasLabel.
     */
    auto HasLabel() const -> bool;

    /// Height as stored: the manual value in pixels, or 0 when the value is automatic.
    auto Height() const -> qreal;
    /// Width as stored: the manual value in pixels, or 0 when the value is automatic.
    auto Width() const -> qreal;
    /// Center as stored: a unitless ratio in [0, 1], 0.5 by default.
    auto CenterPosition() const -> qreal;

    /**
     * @brief ResolvedHeight is the number currently in effect for the height field.
     *
     * The manual value when one is set, the automatic one otherwise (1 cm). This is NOT the
     * rendered arrow height: for TwoArrowsTextAbove the drawn arrows are shorter than the value
     * the field drives.
     */
    auto ResolvedHeight() const -> qreal;
    /// @brief ResolvedWidth is the number in effect for the width field: the manual value, or
    /// 0.9 × the axis length. Not clamped — the renderer clamps to the axis length itself.
    auto ResolvedWidth() const -> qreal;
    /// @brief ResolvedCenter is the number in effect for the center field (0.5 by default).
    auto ResolvedCenter() const -> qreal;

    auto FoldLinePath() const -> QVector<QPainterPath>;
    auto FoldLineMarkPoints() const -> QVector<QVector<QPointF>>;

    auto LabelPosition(bool &ok) const -> FoldLabelPosData;

    /**
     * @brief LabelLineCount is how many text items this fold line's caption needs.
     *
     * A QGraphicsSimpleTextItem draws a single line, so a caption written in several languages is
     * several items. Zero when this fold line draws no caption at all.
     */
    auto LabelLineCount() const -> vsizetype;

    /**
     * @brief UpdateFoldLineLabel places one line of the caption on @a item.
     *
     * The caller owns the items and asks for as many as LabelLineCount() announces, one call per
     * line. How a line is placed — where it sits, at what angle, and which way a mirrored piece
     * turns it — is the fold line's business and not the caller's.
     *
     * @param item the item that carries line @a lineIndex
     * @param lineIndex the line's rank, 0 being the one farthest from the fold line
     */
    void UpdateFoldLineLabel(QGraphicsSimpleTextItem *item, vsizetype lineIndex) const;

    static auto DefaultFoldLineLabel() -> QString;

    /// The font the caption is drawn with, built from the application settings. One place only: the
    /// Details canvas, the layout and the preview of the setting all call it, so neither can drift.
    static auto DefaultLabelFont() -> QFont;

private:
    /// Le résolveur installé par l'application, s'il y en a un. Statique parce qu'il dit ce que
    /// l'application sait, et non ce qu'une foldline particulière est.
    static OFoldLineLabelResolver s_labelResolver;

    QLineF m_foldLine{};
    FoldLineType m_type{FoldLineType::TwoArrowsTextAbove};
    qreal m_height{0};
    qreal m_width{0};
    qreal m_center{0.5};
    QString m_label{};
    bool m_italic{false};
    bool m_bold{false};
    int m_alignment{Qt::AlignHCenter};
    QTransform m_matrix{};
    bool m_verticallyFlipped{false};
    bool m_horizontallyFlipped{false};
    QFont m_outlineFont{};

    auto FoldLineTwoArrowsPath() const -> QVector<QPainterPath>;
    auto FoldLineTwoArrowsTextAbovePath() const -> QVector<QPainterPath>;

    auto TrueCenter(const QLineF &base, qreal width) const -> QPointF;

    auto FoldLineLabel() const -> QString;

    auto OutlineFontLabel(const QLineF &base, qreal width) const -> QPainterPath;

    auto LabelTextHeight() const -> qreal;
    auto LabelOutlineFont() const -> QFont;

    auto TwoArrowsPath(qreal width, qreal height) const -> QPainterPath;

    auto TwoArrowsPoints(qreal width, qreal height) const -> QVector<QVector<QPointF>>;

    /// Automatic width: 0.9 × the axis length. Single definition of the 0.9 factor.
    auto AutoWidth() const -> qreal;
    /// Width the drawing uses: the manual value clamped to the axis length, or the automatic one.
    auto EffectiveWidth() const -> qreal;

    auto TwoArrowsData() const -> ArrowsTextPosData;
    auto TwoArrowsTextAboveData() const -> ArrowsTextPosData;
};

#endif // VFOLDLINE_H
