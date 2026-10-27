/************************************************************************
 **
 **  @file   ootudocument.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   23/07/2026
 **
 **  @brief  In-memory model for .otu (Ostara Taille Unique) measurement files.
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

#ifndef OOTUDOCUMENT_H
#define OOTUDOCUMENT_H

#include "vdomdocument.h"
#include "omeasurementtypes.h"
#include <QDomElement>
#include <QList>
#include <QString>
#include <QStringList>

/**
 * @brief In-memory model for .otu (Ostara Taille Unique) measurement files.
 *
 * Simplified subset of .omt: no size groups, no anchor, no increments, no grid.
 * Each measurement stores a single value. Inherits ODomDocument for XML I/O.
 *
 * Forward compatibility: unknown top-level elements are preserved in
 * m_unknownElements and serialized back on save.
 */
class OOtuDocument : public VDomDocument
{
    Q_OBJECT // NOLINT

public:
    // --- Format version ---
    static constexpr const char *CurrentFormatVersion = "0.1.0";

    // --- Root ---
    static const QString TagOtu;
    static const QString AttrVersion;
    static const QString TagName;

    // --- Unit ---
    static const QString TagUnit;

    // --- System ---
    static const QString TagSystem;

    // --- Measurements ---
    static const QString TagMeasurements;
    static const QString TagM;
    static const QString TagValue;  // child element, not attribute
    static const QString AttrName;
    static const QString AttrFullName;
    static const QString AttrDescription;
    static const QString AttrType;
    static const QString AttrSpecialUnits;
    static const QString AttrCollapsed;
    static const QString AttrValue;

    explicit OOtuDocument(QObject *parent = nullptr);
    ~OOtuDocument() override;

    // --- Factory ---
    /// Creates a minimal empty .otu document with root, unit, and empty measurements.
    auto CreateEmptyFile(Unit unit = Unit::Cm) -> bool;

    // --- I/O overrides ---
    void setXMLContent(const QString &fileName) override;
    auto SaveDocument(const QString &fileName, QString &error) -> bool override;
    auto GetFormatVersionStr() const -> QString override;

    // --- Table name ---
    auto GetTableName() const -> QString;
    void SetTableName(const QString &name);

    // --- Unit ---
    auto GetUnits() const -> Unit;
    void SetUnits(Unit unit);

    // --- System (known measurements) ---
    auto GetSystem() const -> QString;
    void SetSystem(const QString &uuid);

    // --- Measurements ---
    auto GetMCount() const -> int;
    auto GetAllMNames() const -> QStringList;
    auto GetMFullName(const QString &name) const -> QString;
    auto GetMDescription(const QString &name) const -> QString;
    auto GetMType(const QString &name) const -> QString;
    auto GetMValue(const QString &name) const -> double;
    auto GetMSpecialUnits(const QString &name) const -> bool;
    auto GetMCollapsed(const QString &name) const -> bool;
    void SetMCollapsed(const QString &name, bool collapsed);
    void AddM(const QString &name, const QString &fullName, const QString &description,
              double value, const QString &type = QStringLiteral("measurement"),
              bool specialUnits = false);
    void SetMValue(const QString &name, double value);
    void RemoveM(const QString &name);

    // --- Forward compatibility ---
    void AdoptUnknownElement(const QDomElement &el);
    auto UnknownElements() const -> const QList<QDomElement> &;

    // --- Utilities ---
    auto RootElement() const -> QDomElement;

private:
    Q_DISABLE_COPY_MOVE(OOtuDocument)

    /// Helper: get or create a child element of parent by tag name.
    auto EnsureChildElement(QDomElement &parent, const QString &tagName) -> QDomElement;

    /// Helper: set text content of a simple child element (creates if missing).
    void SetChildText(QDomElement &parent, const QString &tagName, const QString &text);

    /// Helper: get the measurements parent element.
    auto MeasurementsElement() const -> QDomElement;

    /// Helper: find an <m> element by name.
    auto FindMElement(const QString &name) const -> QDomElement;

    /// Unknown top-level elements preserved for forward compatibility.
    QList<QDomElement> m_unknownElements;
};

#endif // OOTUDOCUMENT_H
