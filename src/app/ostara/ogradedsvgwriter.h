/************************************************************************
 **
 **  @file   ogradedsvgwriter.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   06/09/2026
 **
 **  @brief  Graded SVG writer: one Illustrator/Inkscape layer per size (grand →
 **          petit in document order, so the smallest ends up on top of the
 **          layers panel) + a final decor layer holding the shared decor
 **          (grainline, fold, info blocks, annotations, miniatures) of the
 **          reference size. Size layers carry geometry only; the decor layer
 **          carries no contour. Text is ALWAYS emitted as editable <text> (never
 **          paths).
 **          The decor layer's <g> is marked ostara:layout="yes" — a backend
 **          marker (namespace "ostara", see ogradedsvgwriter.cpp) that Rabnest
 **          uses to identify the layer. No editor displays it, so the visible
 **          name is free: id = sanitized display name, inkscape:label / <title>
 **          = display name (localized or user-edited). Each decor object wrapper
 **          of the decor layer is marked i:layer="yes" so Rabnest moves it
 **          together with its piece (see WriteGradedSvg).
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

#ifndef OGRADEDSVGWRITER_H
#define OGRADEDSVGWRITER_H

#include "widgets/InfoBlock/oinfoblockexport.h"  // OInfoBlockSvg::Block / Annotation

#include <QDomElement>
#include <QList>
#include <QMarginsF>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QTransform>
#include <QVector>

class QDomDocument;
class QGraphicsItem;

namespace OGradedSvg
{
/** @brief One piece inside a group wrapper (geometry of a group member). */
struct GradedMember
{
    QString name;
    QGraphicsItem *item = nullptr;
};

/** @brief One top-level object drawn in a size layer (lone piece or group). */
struct GradedObject
{
    QString name;
    bool isGroup = false;
    QGraphicsItem *loneItem = nullptr;         // when !isGroup: the piece silhouette root
    QVector<GradedMember> members;             // when isGroup: the member silhouettes

    /// Placement of the object, when its geometry is written at its own coordinates rather
    /// than moved into place. Identity in the graded export, which positions its items in
    /// the scene and lets the writer read them there.
    QTransform transform;
};

/** @brief One size layer: label + its objects, already in grand → petit order. */
struct SizeLayer
{
    QString label;
    QVector<GradedObject> objects;
};

/** @brief One object of the Layout layer (shared decor of the reference size). */
struct LayoutObject
{
    QString name;

    /// The owner's id — a piece's, or a group's — and what a reader matches on.
    ///
    /// ⚠️ **Never the name.** The visible layer gives its carriers unique ids precisely because two
    /// owners may share a name, so a name is not an identity: matching on it would hand one piece's
    /// cartouche to another, silently, and only in the patterns where a name repeats.
    quint32 ownerId = 0;

    /// True when the owner is a group: its decor stands for every member that has none of its own.
    bool isGroup = false;

    /// The transform that carries this owner's ink onto the sheet, in the scene — the identity when
    /// its ink is already in the frame a reader places from.
    ///
    /// ⚠️ **A piece's placement is not an item transform.** The canvas bakes it into the piece's own
    /// matrix (`placed.SetMatrix`), so every path the item holds — the outline, the grain line, the
    /// fold mark — is written already placed. `outline` and `foldAxis`, on the other hand, are mapped
    /// by the piece's own matrix alone. A payload that copied the ink as the scene holds it would
    /// therefore mix two frames, and every piece would arrive on the plan shifted by its own sheet
    /// position. This is the matrix that has to come off — the one the reader's placement does not
    /// expect.
    QTransform ownerPlacement;

    QPointF origin;                            // scene origin used to place blocks/annotations
    QList<QGraphicsItem *> decorChildren;      // grainline / fold / text items (already in scene coords)
    QVector<OInfoBlockSvg::Block> blocks;
    QVector<OInfoBlockSvg::Annotation> annotations;

    /// The owner's miniatures — a piece's own, or the one a group carries per member.
    ///
    /// Records rather than items, for the same reason the blocks and the annotations are: nobody
    /// ever placed a miniature on the export scene, and the frame it is drawn in is the owner's.
    QVector<OInfoBlockSvg::Thumbnail> thumbnails;

    /// Placement of the owner, applied to this object's wrapper — never to its children.
    ///
    /// That is the whole point: a block keeps the rotation it was given, and the wrapper
    /// carries it along with the piece it belongs to. Composing the two rotations into the
    /// block itself would be a different, and wrong, figure. Identity in the graded export.
    QTransform transform;
};

/**
 * @brief Write the graded multi-layer SVG.
 *
 * @param sizeLayers  layers grand → petit (smallest last, = reference).
 * @param layoutLayerName  display name of the final decor layer (topmost) — used
 *                         for its id, inkscape:label and <title>.
 * @param layoutObjects    decor per top-level object, taken from the reference size.
 * @param imageRect   scene content rect (all sizes, after alignment/offsets).
 * @param layered     false writes the objects straight under the root, with no layer of
 *                    their own. A single-size run has nothing to switch between, so a
 *                    layer panel would offer one entry and no choice; the decor is still
 *                    written, it simply has no layer to sit in.
 * @param metadata    what the document was made WITH, written under `<metadata>` — the recipe
 *                    of the format and of the decor, for the reader that rebuilds the paper
 *                    instead of reading it. Null — the default — writes nothing, which is the
 *                    graded SVG export and the plotter's: neither is reopened by anybody.
 *
 *                    It goes in `<metadata>` and not in a layer, a comment or an attribute of
 *                    the root because that is the element SVG reserves for exactly this, and a
 *                    reader that does not know it ignores it. Nothing drawn depends on it.
 */
bool WriteGradedSvg(const QString &fileName, const QList<SizeLayer> &sizeLayers,
                    const QString &layoutLayerName, const QList<LayoutObject> &layoutObjects,
                    const QRectF &imageRect, const QMarginsF &margins, qreal xScale, qreal yScale,
                    const QString &title, const QString &description, bool layered = true,
                    const QDomElement &metadata = QDomElement());

/**
 * @brief One owner's decor as SVG elements, written in the OWNER's own frame.
 *
 * This is the payload's copy of the decor — the one Rabnest reads. It carries the same ink the
 * visible decor layer draws, produced by the same serializer (never a second one), and differs
 * from it in exactly one thing: the frame. The visible layer writes the ink where the scene put
 * it, page transform included, because that is what a reader shows. This writes it back into the
 * owner's frame — the frame the piece's own outline travels in — so that a reader places it with
 * its own placement and computes nothing.
 *
 * @param doc        the payload document that will own the returned element
 * @param object     the owner's decor, as generation resolved it (once, on the reference)
 * @param projection the transform that carries the reference's seat onto THIS size's piece — see
 *                   OExportPieces, which derives it from the two pieces' own extents. The default
 *                   writes the drawing where it was authored, which is right for the reference and
 *                   for nothing else: the sizes do not occupy the same place in the document.
 * @return a `<decor>` element, or a null element when the owner carries no decor at all
 */
auto OwnerDecorFor(QDomDocument &doc, const LayoutObject &object,
                  const QTransform &projection = QTransform(),
                  const QTransform &foldProjection = QTransform()) -> QDomElement;
}  // namespace OGradedSvg

#endif  // OGRADEDSVGWRITER_H
