/************************************************************************
 **
 **  @file   angleformulawidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/08/2026
 **
 **  @brief  Composite wrapping a FormulaInputWidget with a row of preset
 **          angle buttons (0/90/180/270°) placed below the formula field.
 **          The buttons write the plain numeric value into the formula
 **          (setFormula) and are hidden when the widget is disabled.
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
#ifndef ANGLEFORMULAWIDGET_H
#define ANGLEFORMULAWIDGET_H

#include <QWidget>

class FormulaInputWidget;
class VContainer;

/**
 * @brief A FormulaInputWidget plus a row of preset angle buttons.
 *
 * The preset row is hidden when the widget is disabled (see changeEvent()),
 * mirroring the dynamic enable/disable of angle fields (e.g. DialogSplinePath).
 */
class AngleFormulaWidget : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit AngleFormulaWidget(QWidget *parent = nullptr);
    ~AngleFormulaWidget() override = default;

    /// Set the data container for variable resolution (required before use)
    void setData(const VContainer *data);

    /// Set the tool ID used by DialogEditWrongFormula to filter out self-referencing variables
    void setToolId(quint32 id);

    /// Unit postfix shown after the result value (e.g. "mm", "°")
    void setPostfix(const QString &postfix);

    /// Label text shown at the top-left (e.g. "Length:", "Angle:")
    void setLabelText(const QString &text);

    /// Window title for the DialogEditWrongFormula popup
    void setDialogTitle(const QString &title);

    /// Whether zero is considered an invalid value
    void setCheckZero(bool check);

    /// Whether negative values are considered invalid
    void setCheckLessThanZero(bool check);

    /// Set the formula text (forwarded to the inner FormulaInputWidget)
    void setFormula(const QString &formula);

    /// Returns the current formula text, translated from user to internal format.
    [[nodiscard]] auto formula() const -> QString;

    /// Returns the last evaluated value, or INT_MIN if never evaluated/invalid.
    [[nodiscard]] auto value() const -> qreal;

    /// Whether the last evaluation was successful.
    [[nodiscard]] auto isValid() const -> bool;

signals:
    /// Re-emitted from the inner FormulaInputWidget after each debounced evaluation.
    void valueChanged(bool valid);

protected:
    auto changeEvent(QEvent *event) -> void override;
    auto eventFilter(QObject *object, QEvent *event) -> bool override;

private:
    void buildUi();
    void setPresetAngle(int angle);

    FormulaInputWidget *m_formulaInput{nullptr};
    QWidget *m_presetRow{nullptr};

    Q_DISABLE_COPY_MOVE(AngleFormulaWidget)
};

#endif // ANGLEFORMULAWIDGET_H
