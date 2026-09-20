/************************************************************************
 **
 **  @file   ocovergradedaxis.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/09/2026
 **
 **  @brief  The graded axis of a cover page's Size(s) information: which sizes the page
 **          lists, what each is called, and how long a trace sample is drawn for it.
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

#ifndef OCOVERGRADEDAXIS_H
#define OCOVERGRADEDAXIS_H

#include <QDomDocument>
#include <QDomElement>
#include <QHash>
#include <QString>
#include <QVariant>
#include <QtGlobal>

/**
 * @brief The sizes a cover page lists, taken from a range the user chose for the preview.
 *
 * **A range, not a size**: one axis of the measurements table runs from @ref minValue to
 * @ref maxValue, and every other axis stands at one value — the table's own reference value, which
 * is what a page states when nothing tells it otherwise. That is what a graded size range is, and
 * the page states it rather than inventing it.
 *
 * **This is the page's own copy, kept to draw the preview with.** The real range an output is
 * graded along belongs to the pattern and lives in the Graded mode's dock; this one is written in
 * the cover page template, so the designer sees the sizes they are designing for, and it never
 * reaches a pattern, a dock, or an export.
 *
 * @ref names holds what each size is *called* on the page. The table's own labels cannot be edited
 * — the .omt is not the page's to rewrite — so a page that wants « XS » where the table says « 32 »
 * says so here. An empty entry means « keep the table's word », exactly as an axis alias does.
 */
struct OCoverGradedAxis
{
    /// The default length of a trace sample, in centimetres.
    static constexpr qreal kDefaultTraceLengthCm = 4.0;

    QString measurement;             ///< absolute path of the table the range was chosen against.
    QString axis;                    ///< the axis that runs — the one being graded.
    QString minValue;                ///< first size, as the table labels it.
    QString maxValue;                ///< last size, as the table labels it.
    QHash<QString, QString> names;   ///< size label -> the word the page shows; empty = the label.
    qreal traceLengthCm = kDefaultTraceLengthCm;

    /**
     * @brief True when the page has a range to draw.
     *
     * All four of the table, the axis and the two ends, because that is what it takes to know which
     * sizes to list: half a range would list something nobody chose.
     */
    auto HasRange() const -> bool;
};

/**
 * @brief How a graded axis is written into a section's options, and read back out of them.
 *
 * The keys are named once, here, for the reason the axis options name theirs: the control that
 * writes them and the resolvers that read them must not come apart. Like the axes, **the field's
 * own key is one of them** — @ref kRange — which is what makes a section's writer reach the branch
 * that writes the rest.
 */
namespace OCoverGradedAxisOptions
{
/// QString — absolute path of the measurements table this range was chosen against.
inline const QString kRange = QStringLiteral("sizeRange");

/// QString — the axis that runs.
inline const QString kAxis = QStringLiteral("sizeRangeAxis");

/// QString — the first size of the range, as the table labels it.
inline const QString kMin = QStringLiteral("sizeRangeMin");

/// QString — the last size of the range, as the table labels it.
inline const QString kMax = QStringLiteral("sizeRangeMax");

/// QVariantMap — size label -> the name the page shows it under.
inline const QString kNames = QStringLiteral("sizeRangeNames");

/// double — how long a trace sample is drawn, in centimetres.
inline const QString kTraceLength = QStringLiteral("traceLength");

/// The XML element the names map is written as.
inline const QString kTagName = QStringLiteral("sizeName");
inline const QString kAttrLabel = QStringLiteral("label");
inline const QString kAttrShown = QStringLiteral("shown");

/// The options a section starts with — no range chosen, and the default sample length.
auto DefaultOptions() -> QHash<QString, QVariant>;

/// Read the whole graded axis out of a section's options.
Q_REQUIRED_RESULT auto FromOptions(const QHash<QString, QVariant> &options) -> OCoverGradedAxis;

/**
 * @brief Write one section's graded axis into its element.
 *
 * The four scalars go out as attributes; the names become child elements, one per size, because a
 * size label is the table's own word and a word may hold anything — a space, a slash, the comma a
 * table separates its values with. An attribute pack would have to invent an escaping for a value
 * that was never meant to hold one.
 */
void Write(QDomDocument &doc, QDomElement &sectionEl, const QHash<QString, QVariant> &options);

/// Read a section's graded axis back into its options.
void Read(const QDomElement &sectionEl, QHash<QString, QVariant> &options);
} // namespace OCoverGradedAxisOptions

#endif // OCOVERGRADEDAXIS_H
