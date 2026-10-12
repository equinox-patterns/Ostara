/************************************************************************
 **
 **  @file   opiecedocument.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/10/2026
 **
 **  @brief  A piece of the library: the `.opiece` file, held the way a pattern holds its `.ostara`.
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

#ifndef OPIECEDOCUMENT_H
#define OPIECEDOCUMENT_H

#include "xml/vdomdocument.h"

#include <QDomElement>
#include <QList>
#include <QString>

/**
 * @brief A piece of the library: one block of a pattern, its increments, and what it needs from elsewhere.
 *
 * The document *is* the piece — there is no second representation to keep in step. A piece is a `<draw>`
 * block copied out of a pattern, in a small envelope, with the increments that block uses and the values
 * it reads from another piece.
 *
 * What the envelope leaves out is the point of it: a piece carries no cartouche, no watermark, no size
 * ranges, no measurements — nothing that belongs to a pattern rather than to a piece. The pattern that
 * receives the block validates it against its own schema at the next save, so the block's own content is
 * accepted here as it stands rather than described a second time.
 */
class OPieceDocument : public VDomDocument
{
    Q_OBJECT // NOLINT

public:
    /**
     * @brief A value the piece reads but does not declare — a line or a curve of another piece.
     *
     * @p origin is the block the value comes from, and it is always known: a piece can be cut from a block
     * that is not itself a library piece. @p libraryId is that block's identity when it *is* one, and stays
     * empty otherwise; a name would not survive a rename in the library, an identity does.
     */
    struct Dependency
    {
        QString token{};
        QString origin{};
        QString libraryId{};
        QString category{};
    };

    explicit OPieceDocument(QObject *parent = nullptr);

    auto SaveSchema() const -> QString override;

    /** @brief The empty envelope: the format version, and the unit the piece's lengths are written in. */
    void Create(const QString &unit);

    auto Unit() const -> QString;

    /** @brief The block, copied in, named, and given the identity that will follow it wherever it goes. */
    void SetDraw(const QDomElement &source, const QString &name, const QString &libraryId);

    auto DrawElement() const -> QDomElement;
    auto PieceName() const -> QString;
    auto LibraryId() const -> QString;

    /**
     * @brief The two increment tables the piece uses, and the groups they are filed under.
     *
     * Only the rows matter: the sections are copied row by row, so a piece carries its increments and
     * nothing of the table it came from. An empty element leaves the section out.
     */
    void SetIncrements(const QDomElement &increments,
                       const QDomElement &incrementGroups,
                       const QDomElement &previewCalculations,
                       const QDomElement &previewCalculationGroups);

    auto Increments() const -> QDomElement;
    auto IncrementGroups() const -> QDomElement;
    auto PreviewCalculations() const -> QDomElement;
    auto PreviewCalculationGroups() const -> QDomElement;

    void SetDependencies(const QList<Dependency> &dependencies);
    auto Dependencies() const -> QList<Dependency>;

private:
    /** @brief The section @p tag if it is there. */
    auto ExistingSection(const QString &tag) const -> QDomElement;

    /**
     * @brief The section @p tag, created in its place in the sequence if it is not there yet.
     *
     * `<piece>` is a sequence, so a section appended at the end would sit after the block and the file
     * would not pass its own schema.
     */
    auto Section(const QString &tag) -> QDomElement;
};

#endif // OPIECEDOCUMENT_H
