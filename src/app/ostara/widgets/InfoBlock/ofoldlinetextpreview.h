/************************************************************************
 **
 **  @file   ofoldlinetextpreview.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/10/2026
 **
 **  @brief  The fold line caption as it will be drawn.
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

#ifndef OFOLDLINETEXTPREVIEW_H
#define OFOLDLINETEXTPREVIEW_H

#include <QWidget>

/**
 * @brief The fold line caption, in the font the page sets.
 *
 * It draws the caption at the size it will be drawn with, and nothing else — the fold line itself
 * belongs to a piece, and the block editor has none.
 *
 * ⚠️ **It resolves nothing.** The font comes from @ref VFoldLine::DefaultLabelFont and the words from
 * @ref VFoldLine::DefaultFoldLineLabel — the two calls the canvas, the layout and the exports make. A
 * preview that built its own font would be a second answer to a question already answered, and the
 * day it drifted it would lie about the drawing while looking authoritative.
 *
 * It reads the preferences on every paint and keeps no copy of them.
 */
class OFoldLineTextPreview : public QWidget
{
    Q_OBJECT

public:
    explicit OFoldLineTextPreview(QWidget *parent = nullptr);

    /// Re-read the preferences and repaint.
    void Refresh();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Q_DISABLE_COPY_MOVE(OFoldLineTextPreview) // NOLINT
};

#endif // OFOLDLINETEXTPREVIEW_H
