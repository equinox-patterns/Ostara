/************************************************************************
 **
 **  @file   oaxisrenameeditor.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   19/09/2026
 **
 **  @brief  Which size axes an information shows, and what each is called.
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

#ifndef OAXISRENAMEEDITOR_H
#define OAXISRENAMEEDITOR_H

#include "core/otemplateaxisoptions.h"

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QWidget>

class QCheckBox;
class QGridLayout;
class QLabel;
class QTabWidget;
class QVBoxLayout;

/**
 * @brief Which axes an information shows, and how each of them is written in each language.
 *
 * **One control, both families.** A cartouche states the sizes of the piece it decorates, a cover
 * page the sizes of the sheet it heads; what an axis *is* comes from the .omt either way. So the
 * question — *which ones, and what are they called* — is asked once, here, and both editors show
 * the same thing.
 *
 * **Cases à cocher, and not a combo.** A combo says *the* axis, and there is no reason a page
 * should be limited to one when the table declares three: the user who wants height *and*
 * commercial size would have to choose. A box per axis says *the axes*, and the .omt's own ceiling
 * — three — is the only limit there is.
 *
 * One tab per display language. Under each box stands its field, pre-filled with the table's own
 * name, and the values of the axis below it — **in columns, one column per axis, held in the order
 * the table declares them**. An axis with fewer values than its neighbour leaves its cells empty
 * rather than shifting the columns: a value belongs to its axis, and a grid that slid would make it
 * belong to whichever axis happened to be longest.
 *
 * Editing a field creates an alias for that language; leaving it as the table wrote it keeps the
 * original and stores nothing. **The .omt is never modified.**
 *
 * The widget only builds the controls and reports what was edited — it does not write the template
 * (Open/Closed).
 */
class OAxisRenameEditor : public QWidget
{
    Q_OBJECT

public:
    explicit OAxisRenameEditor(QWidget *parent = nullptr);

    /// The title shown above the boxes ("Rename the axis and its values").
    void SetTitle(const QString &title);

    /**
     * @brief Feed the state and rebuild.
     *
     * @param available every axis the table declares, with its values, in table order.
     * @param selected  the axes the information shows, in table order.
     * @param renames   axis name → locale → aliases.
     * @param oneLine   whether every language takes a line of its own.
     * @param languages the display languages, in the order the language row holds them.
     * @param selectable whether the axes can be chosen at all. False for an information that shows
     *                   one thing there is no choosing between — a measurement, say — where a box
     *                   would be asking a question with only one possible answer.
     */
    void SetData(const QList<OAxisChoice> &available, const QStringList &selected,
                 const QMap<QString, OAxisRenames> &renames, bool oneLine,
                 const QStringList &languages, bool selectable = true);

    /**
     * @brief Name the axis this control must not let be ticked.
     *
     * **An axis spoken about elsewhere in the same information is not a value to state beside the
     * others.** A cover page whose sizes come from a range already says everything about the axis
     * that runs — one line per size — and a box beside it would offer to say the same thing a
     * second time, as a single value. The row stays visible, greyed, so the reader can see that the
     * axis is there and that it is accounted for rather than missing.
     *
     * Applied by @ref SetData, which is where the boxes are built.
     */
    void SetReservedAxis(const QString &axisName);

    /**
     * @brief Whether the control offers the « one line per language » choice.
     *
     * **A choice that could do nothing is not offered.** A title is one line whatever happens — the
     * same word repeated down the page would read as so many titles — so an information whose
     * languages are always joined hides the box rather than showing one that changes nothing when it
     * is ticked.
     */
    void SetOneLineOffered(bool offered);

signals:
    /// The axes the user wants shown, in table order.
    void SelectionChanged(const QStringList &axes);
    void RenamesChanged(const QMap<QString, OAxisRenames> &renames);
    void OneLineChanged(bool on);

private:
    /**
     * @brief Rebuilds the boxes — one per axis the table declares.
     *
     * **Never call this from a box's own handler.** A check box that deletes itself while it is
     * handling its own click is a use-after-free: Qt returns from the signal into
     * `QAbstractButton::mouseReleaseEvent` and reads the object it has just destroyed. It happens
     * to be invisible while the layout is built once and never touched, which is why it survived
     * the first build and crashed on the first click.
     */
    void RebuildAxes();

    /// Rebuilds the tabs: the columns are the ticked axes, so a tick redraws these and nothing else.
    void RebuildTabs();

    /// The axes the boxes have ticked, read back in the table's own order.
    auto TickedAxes() const -> QStringList;

    /// Writes one axis's aliases for one language, dropping an entry that came back to the original.
    void StoreAlias(const QString &axis, const QString &locale, int valueIndex, const QString &text);

    QVBoxLayout *m_root{nullptr};
    QLabel *m_titleLabel{nullptr};
    QVBoxLayout *m_axesLayout{nullptr};
    QTabWidget *m_tabs{nullptr};
    QCheckBox *m_oneLineCheck{nullptr};

    QString m_title;
    QList<OAxisChoice> m_available;
    QStringList m_selected;
    /// The axis that may not be ticked here; empty when every axis may. See SetReservedAxis.
    QString m_reservedAxis;
    QMap<QString, OAxisRenames> m_renames;
    bool m_oneLine{false};
    bool m_oneLineOffered{true};
    QStringList m_languages;
    bool m_selectable{true};

    /// Guards the rebuild: ticking a box while the boxes are being built would report a selection
    /// the user never made.
    bool m_building{false};
};

#endif // OAXISRENAMEEDITOR_H
