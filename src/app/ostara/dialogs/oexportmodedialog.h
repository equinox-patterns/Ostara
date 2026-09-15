/************************************************************************
 **
 **  @file   oexportmodedialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/09/2026
 **
 **  @brief  Export mode entry dialog — one tab per thing the mode can be
 **          asked for, and the sheet the run comes out on.
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

#ifndef OEXPORTMODEDIALOG_H
#define OEXPORTMODEDIALOG_H

#include <QDialog>
#include <QSharedPointer>

#include "../core/ogradedsizeranges.h"
#include "../vmisc/def.h"

class OExportPrintWidget;
class QDialogButtonBox;
class QEvent;
class QTabWidget;
class VContainer;
class VMeasurements;

/**
 * @brief Entry dialog of the Export mode.
 *
 * The mode is asked for three different things — a list of runs to fire one after the
 * other, a cutting plan, and a projection — and they have almost nothing in common but
 * the nesting underneath. Hence one tab each rather than one form that grew a second
 * meaning: the tab that describes what the user came for is the only one that shows.
 *
 * <b>Print</b> is the only tab that is built. The other three announce themselves and
 * nothing more — a mode that silently lacked two thirds of what it will do would read as
 * a mode that was finished. The dialog opens on Print, the one tab that can be acted on.
 *
 * The dialog owns no field of its own: every value the run needs comes from the Print
 * tab, and the accessors below are the pass-through. That is deliberate — the same values
 * will have to be read from the other tabs the day they exist, and a dialog that held a
 * second copy of them would be a second place to keep them right.
 */
class OExportModeDialog : public QDialog
{
    Q_OBJECT

public:
    /// @param context the (measurements table, declared grading axis) the pattern is on.
    ///                The dialog cannot resolve it: it holds no document. The range list
    ///                is restricted to it, exactly as the Graded dock restricts its own.
    explicit OExportModeDialog(const VContainer *data, const QSharedPointer<VMeasurements> &measurements,
                               const OGradedSizeRanges::Context &context, QWidget *parent = nullptr);

    /// @brief The width the nesting runs on, in millimetres.
    auto DocumentWidthMm() const -> qreal;

    auto MarginMm() const -> qreal;
    auto GapMm() const -> qreal;

    /// @brief Key of the selected size range. Empty in single-size mode.
    auto SizeRangeName() const -> QString;

    /// @brief True when the user asked for one size instead of a whole range.
    auto IsSingleSize() const -> bool;

    /// @brief Base value of the axis in slot @p index (0 = X, 1 = Y, 2 = W).
    ///        -1 when the table carries no such axis, or the user picked nothing for it.
    auto DimensionBase(int index) const -> qreal;

    /// @brief What the axis in slot @p index reads as, empty when the table carries none.
    auto DimensionLabel(int index) const -> QString;

    /// @brief The token of the print model the run was asked for.
    auto PrintModelToken() const -> QString;

    /// @brief How many sheets side by side; 1 on a roll.
    auto Columns() const -> int;

    /// @brief The unit the run was read in.
    auto DisplayUnit() const -> Unit;

protected:
    void changeEvent(QEvent *event) override;

private:
    /// @brief A tab that exists and does nothing yet, saying so rather than showing
    ///        an empty page.
    static auto MakeComingSoonTab(QWidget *parent) -> QWidget *;

    void Retranslate();

    /// @brief Keep the dialog at least as wide as its tab bar.
    ///
    /// A QTabWidget whose tabs do not fit does not grow: it elides the labels, or grows
    /// scroll arrows beside them. Four tabs the user cannot read are four tabs she cannot
    /// choose between, so the width is imposed from the labels instead — and re-imposed
    /// whenever they change language.
    void AdjustMinimumWidth();

    OExportPrintWidget *m_printWidget{nullptr};
    QTabWidget *m_tabs{nullptr};
    QDialogButtonBox *m_buttonBox{nullptr};

    QWidget *m_listTab{nullptr};
    QWidget *m_cuttingTab{nullptr};
    QWidget *m_printTab{nullptr};
    QWidget *m_projectorTab{nullptr};
};

#endif // OEXPORTMODEDIALOG_H
