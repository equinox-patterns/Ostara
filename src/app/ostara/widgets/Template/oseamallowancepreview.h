/************************************************************************
 **
 **  @file   oseamallowancepreview.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/09/2026
 **
 **  @brief  Shows what one margin of the library says: its name, and the
 **          width it answers to.
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

#ifndef OSEAMALLOWANCEPREVIEW_H
#define OSEAMALLOWANCEPREVIEW_H

#include "core/oseamallowancelibrary.h"

#include <QWidget>

#include "../vmisc/def.h"

/**
 * @brief One margin, as the library means it.
 *
 * **What is shown is the name and the rule, not a finished line of the page.** How a page writes a
 * margin — in which units, with which separator, in which typeface — is a setting of the template,
 * and the library knows nothing of it: a preview that invented a line here would be showing a page
 * that does not exist. What the user is checking on this pane is that the name reads well and that
 * the rule picks the width it was meant to.
 */
class OSeamAllowancePreview : public QWidget
{
    Q_OBJECT

public:
    explicit OSeamAllowancePreview(QWidget *parent = nullptr);

    /// Shows one margin. @p mm is the width it answers to, and is not read when @p byDefault.
    void SetEntry(const QString &name, bool byDefault,
                  OSeamAllowanceLibrary::Comparison comparison, qreal mm, const Unit &unit);

    /// Nothing to show: no margin is selected.
    void Clear();

    void RetranslateUI();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    bool m_hasEntry{false};
    QString m_name;
    bool m_byDefault{false};
    OSeamAllowanceLibrary::Comparison m_comparison{OSeamAllowanceLibrary::Comparison::Equal};
    qreal m_mm{0.0};
    Unit m_unit{Unit::Cm};
};

#endif // OSEAMALLOWANCEPREVIEW_H
