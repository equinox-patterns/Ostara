/************************************************************************
 **
 **  @file   OInfoBlockPreview.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/08/2026
 **
 **  @brief  Live preview of a cartouche template — one stacked line per section.
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

#ifndef OINFOBLOCKPREVIEW_H
#define OINFOBLOCKPREVIEW_H

#include "OPieceCartoucheTemplate.h"

#include <QWidget>

class OPieceCartoucheTemplate;

/**
 * @brief Live preview — renders the template as a single stacked block
 * (the default dispatch), one line per section.
 *
 * Per-section font styling is applied from the section options (useDefaults /
 * fontFamily / fontWeight / fontSize). The preview only *reads* the data: it
 * is not a data source itself (spec §3).
 */
class OInfoBlockPreview : public QWidget
{
    Q_OBJECT

public:
    explicit OInfoBlockPreview(QWidget *parent = nullptr);

    /// Point the preview at the template being edited (lifetime owned by the caller).
    void SetTemplate(const OPieceCartoucheTemplate *tpl);

    /// Re-render (call after any template change).
    void Refresh();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    const OPieceCartoucheTemplate *m_template{nullptr};
};

#endif // OINFOBLOCKPREVIEW_H
