/************************************************************************
 **
 **  @file   ocovergradedaxiseditor.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/09/2026
 **
 **  @brief  The block that edits how a cover page draws the axis it grades along.
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

#ifndef OCOVERGRADEDAXISEDITOR_H
#define OCOVERGRADEDAXISEDITOR_H

#include <QHash>
#include <QString>

#include "core/ocovergradedaxis.h"
#include "core/otemplateoptioneditor.h"

class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QVBoxLayout;
class QWidget;

/**
 * @brief The names the page gives the sizes, and how long the sample beside each one runs.
 *
 * **Every value of the axis, not only the ones the range holds.** A page is designed once and
 * printed from many ranges — the same template, run against 36–44 today and 46–52 next month — and
 * the names written here are the names it shows then. Offering only what the current range happens
 * to list would mean renaming the page again for every range it is ever printed from, which is the
 * work the alias exists to save.
 *
 * Where the sizes *come from* is asked elsewhere, by the block that stands above this one: a range
 * is one question, and how it is drawn is another. The catalogue is what says this block waits for
 * that answer.
 */
class OCoverGradedAxisEditor : public OTemplateOptionEditor
{
    Q_OBJECT

public:
    explicit OCoverGradedAxisEditor(const OTemplateOptionContext &context,
                                    QWidget *parent = nullptr);

    void Refresh(const OTemplateOptionContext &context) override;

private:
    void BuildUi();
    void ApplyNames(const QHash<QString, QString> &names);
    void Write(const QString &key, const QVariant &value);
    void RebuildNameRows(const OCoverGradedAxis &range);

    OTemplateOptionContext m_context;

    QDoubleSpinBox *m_traceLength{nullptr};
    QWidget *m_namesHost{nullptr};
    QVBoxLayout *m_namesLayout{nullptr};
};

#endif // OCOVERGRADEDAXISEDITOR_H
