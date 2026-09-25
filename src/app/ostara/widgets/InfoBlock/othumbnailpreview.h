/************************************************************************
 **
 **  @file   othumbnailpreview.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   25/09/2026
 **
 **  @brief  What a miniature says, drawn without the piece: one value
 **          and the legend that names its unit.
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

#ifndef OTHUMBNAILPREVIEW_H
#define OTHUMBNAILPREVIEW_H

#include <QStringList>
#include <QWidget>

/**
 * @brief The wording of a miniature, at the size it will be printed.
 *
 * The Thumbnail page sets two texts and two units; this is the only place that shows what they come
 * to. It draws **no drawing at all** — a piece belongs to a pattern, and the block editor has none —
 * so what is previewed is the wording: one seam-allowance value, and the legend beneath it, one line
 * per language, exactly as the canvas writes them.
 *
 * ⚠️ **It formats nothing.** The value comes from @ref OThumbnailTextBuilder::ValueText, the legend
 * from @ref OThumbnailTextBuilder::Legend, and the fonts from @ref OThumbnailItem::ValuesFont — the
 * same three calls the canvas and the export make. A preview that resolved any of them itself would
 * be a second answer to a question already answered, and the day it drifted it would lie about the
 * drawing while looking authoritative.
 *
 * It reads the preferences on every paint and keeps no copy of them: the page writes a setting, this
 * shows it, and there is no third state to keep in step.
 */
class OThumbnailPreview : public QWidget
{
    Q_OBJECT

public:
    explicit OThumbnailPreview(QWidget *parent = nullptr);

    /// The languages the legend is written in — one line each. Empty is the application's own.
    void SetLanguages(const QStringList &languages);

    /// Re-read the preferences and repaint.
    void Refresh();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QStringList m_languages;
};

#endif // OTHUMBNAILPREVIEW_H
