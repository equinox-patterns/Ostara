/************************************************************************
 **
 **  @file   oexportmodedialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/09/2026
 **
 **  @brief  Export mode entry dialog — what to nest, on which bin.
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
#include <QVector>

class QComboBox;
class QDialogButtonBox;
class QEvent;
class QFormLayout;
class QLabel;
class QLineEdit;
class VContainer;
class VMeasurements;

/// @brief Entry dialog of the Export mode.
///
/// Asks for the width of the document and for what to nest, then gets out of the
/// way. The width is typed in the pattern's own unit — the label next to the field
/// follows the pattern, it is never typed by the user.
///
/// What to nest is either a whole size range (multisize) or a single size, chosen
/// here with the same dimension drop-downs Details mode offers in its top bar. The
/// single-size path exists because it is the one Details and Layout already use:
/// the geometry is read as it stands, no measurement is re-read.
///
/// The ranges come from the pattern itself, and the geometry never leaves the
/// application: nothing is exported, nothing is read back.
class OExportModeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OExportModeDialog(const VContainer *data, const QSharedPointer<VMeasurements> &measurements,
                               QWidget *parent = nullptr);

    /// @brief The entered document width, in the pattern's unit.
    auto DocumentWidth() const -> qreal;

    /// @brief Key of the selected size range. Empty in single-size mode.
    auto SizeRangeName() const -> QString;

    /// @brief True when the user asked for one size instead of a whole range.
    auto IsSingleSize() const -> bool;

    /// @brief Base value of dimension @p index (0 = A, 1 = B, 2 = C). -1 when absent.
    auto DimensionBase(int index) const -> qreal;

protected:
    void changeEvent(QEvent *event) override;

private:
    void PopulateSizeRanges();
    void PopulateDimensions();
    void UpdateSourceVisibility();
    void UpdateUnitLabel();
    void CheckState();

    const VContainer *m_data{nullptr};
    QSharedPointer<VMeasurements> m_measurements{};

    QLabel *m_widthLabel{nullptr};
    QLineEdit *m_widthEdit{nullptr};
    QLabel *m_unitLabel{nullptr};

    QFormLayout *m_form{nullptr};

    QLabel *m_sourceLabel{nullptr};
    QComboBox *m_sourceCombo{nullptr};

    QLabel *m_rangeLabel{nullptr};
    QComboBox *m_rangeCombo{nullptr};

    /// Dimension label/combo pairs, in the order of the measurement table.
    QVector<QLabel *> m_dimensionLabels{};
    QVector<QComboBox *> m_dimensionCombos{};

    QDialogButtonBox *m_buttonBox{nullptr};
};

#endif // OEXPORTMODEDIALOG_H
