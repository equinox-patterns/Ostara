/************************************************************************
 **
 **  @file   oomtdocument.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/07/2026
 **
 **  @brief  In-memory model for .omt (Ostara Multi-Tailles) measurement files.
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

#ifndef OOMTDOCUMENT_H
#define OOMTDOCUMENT_H

#include "vdomdocument.h"
#include "omeasurementtypes.h"
#include <QDomElement>
#include <QMap>
#include <QString>
#include <QStringList>
#include <memory>
#include <optional>

/**
 * @brief In-memory model for .omt (Ostara Multi-Tailles) measurement files.
 *
 * Stores both the editing recipe (anchor value + increments per axis group)
 * and the computed result grid for O(1) loading.
 * Inherits ODomDocument for XML I/O and provides typed accessors.
 *
 * Forward compatibility: unknown top-level elements are preserved in
 * m_unknownElements and serialized back on save.
 */
class OOmtDocument : public VDomDocument
{
public:
    // --- Format version ---
    static constexpr const char *CurrentFormatVersion = "0.1.0";

    // --- Root ---
    static const QString TagOmt;
    static const QString AttrVersion;
    static const QString TagName;

    // --- Unit ---
    static const QString TagUnit;
    static const QString AttrValue;

    // --- System ---
    static const QString TagSystem;

    // --- Size groups ---
    static const QString TagSizeGroups;
    static const QString TagSizeGroup;
    static const QString AttrName;
    static const QString AttrValues;

    // --- Anchor ---
    static const QString TagAnchor;
    static const QString TagBind;
    static const QString AttrGroup;

    // --- Measurements ---
    static const QString TagMeasurements;
    static const QString TagM;
    static const QString TagValue;  // child element, not attribute
    static const QString TagInc;
    static const QString AttrFullName;
    static const QString AttrDescription;
    static const QString AttrSpecialUnits;
    static const QString AttrCollapsed;

    // --- Range groups ---
    static const QString TagRangeGroups;
    static const QString TagRangeGroup;
    static const QString TagRange;

    // --- Grid ---
    static const QString TagGrid;
    static const QString TagCell;
    static const QString AttrM;
    static const QString AttrSg;
    static const QString AttrV;

    explicit OOmtDocument(QObject *parent = nullptr);
    ~OOmtDocument() override;

    // --- Factory ---
    /// Creates a minimal empty .omt document with root, unit, and empty sections.
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

    // --- Size groups ---
    auto GetSizeGroupCount() const -> int;
    auto GetSizeGroupNames() const -> QStringList;
    auto GetSizeGroupValues(const QString &groupName) const -> QStringList;
    void AddSizeGroup(const QString &name, const QStringList &values);
    void RemoveSizeGroup(const QString &name);
    void ClearSizeGroups();

    // --- Anchor ---
    auto GetAnchorBindings() const -> QMap<QString, QString>;
    auto GetAnchorValue(const QString &groupName) const -> QString;
    void SetAnchorBinding(const QString &groupName, const QString &value);
    void ClearAnchor();

    // --- Measurements (editing recipe) ---
    auto GetMCount() const -> int;
    auto GetAllMNames() const -> QStringList;
    auto GetMFullName(const QString &name) const -> QString;
    auto GetMDescription(const QString &name) const -> QString;
    auto GetMType(const QString &name) const -> QString;
    auto GetMValue(const QString &name) const -> double;
    auto GetMIncrements(const QString &name) const -> QMap<QString, GroupIncrement>;
    auto GetMIncrement(const QString &name, const QString &group) const -> double;
    auto GetMSpecialUnits(const QString &name) const -> bool;
    auto GetMCollapsed(const QString &name) const -> bool;
    void SetMCollapsed(const QString &name, bool collapsed);
    void AddM(const QString &name, const QString &fullName, const QString &description,
              double anchorValue, const QMap<QString, GroupIncrement> &increments,
              const QString &type = QStringLiteral("measurement"), bool specialUnits = false);
    void SetMValue(const QString &name, double value);
    void SetMIncrement(const QString &name, const QString &group, double inc);
    void RemoveM(const QString &name);

    // --- Range groups (document-level, per axis group) ---
    auto GetRangeGroups(const QString &axisGroupName) const -> QVector<RangeGroup>;
    void AddRangeGroup(const QString &axisGroupName, const RangeGroup &rg);
    void RemoveRangeGroup(const QString &axisGroupName, const QString &rgName);
    void UpdateRangeGroup(const QString &axisGroupName, const RangeGroup &rg);
    void ClearRangeGroups(const QString &axisGroupName);

    // --- Grid ---
    /// Returns the computed value for a given measurement and size group coordinate.
    /// Coordinate format: "v1,v2,v3" (comma-separated values matching group order).
    ///
    /// **Nullopt is the honest answer for a size the grid does not describe** — a table saved
    /// before the grid existed, or a coordinate the file does not declare. Returning 0.0 for
    /// both would be a value nothing can tell apart from a real zero, and the callers that show
    /// this figure to a user would then print one that was never computed.
    auto GetGridCell(const QString &mName, const QString &coord) const -> std::optional<qreal>;
    void SetGridCell(const QString &mName, const QString &coord, double value);
    void ClearGrid();

    // --- Computation ---
    /// Compute all grid cells from the editing recipe and populate the <grid> section.
    /// Cascade rule: value = anchorValue + sum(inc[axis] × stepsFromAnchor[axis])
    void ComputeGrid();

    /// Compute the value for a single measurement at a specific size combination.
    /// @param anchorValue  The measurement's base value at the reference size.
    /// @param increments   Per-group increment values (group name → increment).
    /// @param groupValues  Ordered group names and their value lists.
    /// @param anchorBindings  Reference value per group (group name → anchor value).
    /// @param selectedValues  Currently selected value per group (group name → selected).
    /// @return Computed value = anchorValue + Σ inc[i] × (idxSelected[i] − idxAnchor[i]).
    static auto ComputeValueForCombination(double anchorValue,
                                           const QMap<QString, GroupIncrement> &increments,
                                           const QMap<QString, QStringList> &groupValues,
                                           const QMap<QString, QString> &anchorBindings,
                                           const QMap<QString, QString> &selectedValues,
                                           const QMap<QString, QVector<RangeDef>> &rangeLookup) -> double;

    // --- Forward compatibility ---
    void AdoptUnknownElement(const QDomElement &el);
    auto UnknownElements() const -> const QList<QDomElement> &;

    // --- Utilities ---
    auto RootElement() const -> QDomElement;

private:
    Q_DISABLE_COPY_MOVE(OOmtDocument)

    /// Helper: get or create a child element of parent by tag name.
    auto EnsureChildElement(QDomElement &parent, const QString &tagName) -> QDomElement;

    /// Helper: set text content of a simple child element (creates if missing).
    void SetChildText(QDomElement &parent, const QString &tagName, const QString &text);

    /// Helper: get the measurements parent element.
    auto MeasurementsElement() const -> QDomElement;

    /// Helper: find a <bind> element by group name.
    auto FindBindElement(const QString &groupName) const -> QDomElement;

    /// Helper: find an <m> element by name.
    auto FindMElement(const QString &name) const -> QDomElement;

    /// Helper: find an <inc> element by group name inside an <m>.
    auto FindIncElement(const QDomElement &mEl, const QString &group) const -> QDomElement;

    /// Helper: parse comma-separated values string into a list.
    static auto ParseValuesString(const QString &csv) -> QStringList;

    /// Unknown top-level elements preserved for forward compatibility.
    QList<QDomElement> m_unknownElements;
};

#endif // OOMTDOCUMENT_H
