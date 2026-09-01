/************************************************************************
 **
 **  @file   oannotationdialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  Add / edit dialog of one annotation: style template combo, the
 **          multi-line content and the rotation formula (AngleFormulaWidget,
 **          pattern DialogEndLine). The same dialog serves creation and
 **          edition; there is no delete button here (deletion lives in the
 **          Details dock list).
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

#ifndef OANNOTATIONDIALOG_H
#define OANNOTATIONDIALOG_H

#include <QDialog>

class AngleFormulaWidget;
class QComboBox;
class QDialogButtonBox;
class QLabel;
class QPlainTextEdit;
class VContainer;

/**
 * @brief Add / edit dialog of one annotation.
 *
 * The OK button is enabled only when the rotation formula is valid (the
 * AngleFormulaWidget evaluates it against the pattern variables). On accept,
 * SaveData() stores the template name, the content and the rotation formula
 * (internal form), read back through TemplateName()/Text()/RotationFormula().
 */
class OAnnotationDialog : public QDialog
{
    Q_OBJECT // NOLINT

public:
    explicit OAnnotationDialog(const VContainer *data, QWidget *parent = nullptr);
    ~OAnnotationDialog() override = default;

    auto TemplateName() const -> QString;
    auto Text() const -> QString;
    auto RotationFormula() const -> QString;

    void SetTemplateName(const QString &name);
    void SetText(const QString &text);
    void SetRotationFormula(const QString &formula);

protected:
    void changeEvent(QEvent *event) override;

private:
    void CheckState();
    auto IsValid() const -> bool;
    void SaveData();

    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(OAnnotationDialog) // NOLINT

    QComboBox *m_templateCombo{nullptr};
    QLabel *m_templateLabel{nullptr};
    QPlainTextEdit *m_textEdit{nullptr};
    QLabel *m_textLabel{nullptr};
    AngleFormulaWidget *m_formulaAngle{nullptr};
    QDialogButtonBox *m_buttonBox{nullptr};

    QString m_templateName;
    QString m_text;
    QString m_rotationFormula;

    const VContainer *m_data;
};

#endif // OANNOTATIONDIALOG_H
