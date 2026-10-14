/************************************************************************
 **
 **  @file   oannotationdata.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  Per-piece annotation state: floating multi-line texts on the
 **          pattern with a style, a rotation formula and a geometry.
 **          Annotations are purely per-piece — there is no pattern-level
 **          seed to inherit.
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

#ifndef OANNOTATIONDATA_H
#define OANNOTATIONDATA_H

#include <QList>
#include <QPointF>
#include <QString>
#include <QtGlobal>

#include <functional>

#include "onormalizedanchor.h"

/**
 * @brief Geometry of one annotation, in piece-local scene pixels.
 *
 * boxWidth/boxHeight of 0 mean auto-size to the natural content size. The
 * corner handles scale the whole block (content + box) through fontScale;
 * the edge handles grow only the box, leaving the content scale untouched.
 */
struct OAnnotationGeom
{
    QPointF pos{};        // Piece-local position (scene px).
    qreal boxWidth{0.0};  // Box width in px; 0 = auto (fits the content).
    qreal boxHeight{0.0}; // Box height in px; 0 = auto.
    qreal fontScale{1.0}; // Content scale factor applied by the corner handles.
    ONormalizedAnchor anchor{}; // Normalized placement, keeps the annotation following the active size.
};

/**
 * @brief One annotation: a floating multi-line text on the pattern.
 *
 * The text references a style by name (empty = the built-in Default style)
 * and carries a rotation formula ("90" or "AngleLine_A_B"), evaluated at
 * render time against the pattern variables.
 */
struct OAnnotation
{
    QString templateName;    // Style name ("" = Default).
    QString text;            // Multi-line content.
    QString rotationFormula; // "90" / "AngleLine_A_B" (internal form).
    OAnnotationGeom geom;
};

/**
 * @brief Render-time style of one annotation, resolved from the style library
 * through an app-registered resolver (libs have no app dependency). Empty
 * fields mean "default": default font, transparent background, black text.
 */
struct OAnnotationRenderStyle
{
    QString fontFamily;

    /// The FACE, by NAME — a number names no face (decided on 22-09-2026 : « que les vraies
    /// graisses de la font »). Empty is the family's Regular. A style library written before the faces
    /// were named holds a weight instead, and @ref OPieceCartoucheStorage resolves it **once, at read
    /// time** — so nothing downstream ever sees two ways of saying the same thing.
    QString fontFace;

    int fontSize = 0;
    QString backgroundColor;
    QString textColor;
};

/// App-registered resolver from a style name to its render style (layering
/// bridge, registered once at app startup — pattern OInfoBlockContentBuilder).
using OAnnotationStyleResolver = std::function<OAnnotationRenderStyle(const QString &name)>;

/**
 * @brief Per-piece annotation list.
 *
 * A piece is either "unset" (legacy / never edited) or "set" (the
 * <annotations> element is present in the file). The presence of the element
 * is what marks the piece as configured.
 */
class OAnnotationData
{
public:
    OAnnotationData() = default;

    auto IsSet() const -> bool;
    void SetSet(bool set);

    /** @return The annotation list. */
    auto Annotations() const -> const QList<OAnnotation> &;
    void SetAnnotations(const QList<OAnnotation> &annotations);

    /** @return Number of annotations. */
    auto Count() const -> int;

    void Clear();

private:
    bool m_set{false};
    QList<OAnnotation> m_annotations;
};

#endif // OANNOTATIONDATA_H
