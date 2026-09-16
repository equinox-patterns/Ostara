/************************************************************************
 **
 **  @file   oexporttemplatepreview.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/09/2026
 **
 **  @brief  Preview of a tiled plan — six white artboards on a contrasting background.
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

#ifndef OEXPORTTEMPLATEPREVIEW_H
#define OEXPORTTEMPLATEPREVIEW_H

#include <QWidget>

/**
 * @brief Draws six artboards — three columns, two rows — as white sheets on a contrasting
 * background.
 *
 * This is the preview pane of the Export template editor. Six sheets rather than one: the
 * marks are placed per sheet and the pagination runs along the rows and the columns, so a
 * single sheet would show neither. What is previewed is the rendering of the settings.
 *
 * The sheets tile edge to edge, the way the plan glues its artboards.
 *
 * Nothing is drawn on the sheets yet — the frame, the marks and the pagination are not
 * implemented. The sheet proportions and the two colours are plain defaults, waiting on the
 * decisions listed in Modifications/Ostara/ExportMode/PDFTemplateEditor.md §7.
 */
class OExportTemplatePreview : public QWidget
{
    Q_OBJECT

public:
    explicit OExportTemplatePreview(QWidget *parent = nullptr);

    /** @brief Sheet proportions, width / height. Portrait A-series by default. */
    void SetSheetRatio(qreal ratio);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    qreal m_sheetRatio{21.0 / 29.7};
};

#endif // OEXPORTTEMPLATEPREVIEW_H
