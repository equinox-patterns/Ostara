/************************************************************************
 **
 **  @file   formulainputwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   30/07/2026
 **
 **  @brief  Reusable formula input widget for tool creation/editing dialogs.
 **          Encapsulates: label, VPlainTextEdit (with syntax highlighting),
 **          FX button (opens DialogEditWrongFormula), deploy button
 **          (expand/collapse), result label, debounced evaluation,
 **          and decimal separator event filter.
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
#ifndef FORMULAINPUTWIDGET_H
#define FORMULAINPUTWIDGET_H

#include <QWidget>

#include "../ifc/ifcdef.h"

class QLabel;
class QPushButton;
class QTimer;
class QToolButton;
class VContainer;
class VPlainTextEdit;

class FormulaInputWidget : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit FormulaInputWidget(QWidget *parent = nullptr);
    ~FormulaInputWidget() override;

    /// Set the data container for variable resolution (required before use)
    void setData(const VContainer *data);

    /// Set the tool ID used by DialogEditWrongFormula to filter out self-referencing variables
    /// (0 / NULL_ID = no filtering, e.g. during creation).
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

    /// Set the formula text. Translates from internal to user format,
    /// triggers evaluation automatically.
    void setFormula(const QString &formula);

    /// Returns the current formula text, translated from user to internal format.
    [[nodiscard]] auto formula() const -> QString;

    /// Returns the last evaluated value, or INT_MIN if never evaluated/invalid.
    [[nodiscard]] auto value() const -> qreal;

    /// Whether the last evaluation was successful.
    [[nodiscard]] auto isValid() const -> bool;

    /// The inner editor. Exposed so the owning dialog can put it in the tab order and name it for the
    /// accessibility layer: the widget owns the field, the dialog owns the dialog.
    [[nodiscard]] auto editor() const -> QWidget *;

signals:
    /// Emitted after each debounced evaluation completes.
    /// Connect to CheckState() in the parent dialog.
    void valueChanged(bool valid);

protected:
    auto eventFilter(QObject *object, QEvent *event) -> bool override;

private slots:
    void evaluate();
    void openFxDialog();
    void toggleExpanded();

private:
    void onTextChanged();
    void buildUi();
    void updateHeight();
    void updateExpandIcon();
    auto heightForLines(int lineCount) const -> int;

    QLabel *m_label{nullptr};
    VPlainTextEdit *m_textEdit{nullptr};
    QToolButton *m_fxButton{nullptr};
    QPushButton *m_growButton{nullptr};
    QLabel *m_resultLabel{nullptr};

    QTimer *m_timer{nullptr};
    bool m_expanded{true};
    int m_lineHeight{0};
    quint32 m_toolId{NULL_ID};
    const VContainer *m_data{nullptr};
    QString m_postfix{};
    QString m_dialogTitle{};
    bool m_checkZero{false};
    bool m_checkLessThanZero{false};
    bool m_valid{false};
    qreal m_value{INT_MIN};

    Q_DISABLE_COPY_MOVE(FormulaInputWidget)
};

#endif // FORMULAINPUTWIDGET_H
