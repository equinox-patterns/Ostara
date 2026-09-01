/************************************************************************
 **
 **  @file   oannotationpreview.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  Live visual preview of one annotation style: the multi-line text
 **          rendered with the style's font (family/weight/size), text color
 **          and optional background, centered inside the widget.
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

#ifndef OANNOTATIONPREVIEW_H
#define OANNOTATIONPREVIEW_H

#include <QWidget>

#include "OPieceCartoucheStorage.h"

/**
 * @brief Renders one annotation style: the text with the style's font
 * (family/weight/size), text color and optional background, centered.
 */
class OAnnotationPreview : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit OAnnotationPreview(QWidget *parent = nullptr);

    /// Style to render (font + colors); an empty style renders a placeholder.
    void SetStyle(const OPieceCartoucheStorage::AnnotationStyleEntry &style);
    /// Text to render (multi-line).
    void SetText(const QString &text);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(OAnnotationPreview) // NOLINT

    OPieceCartoucheStorage::AnnotationStyleEntry m_style;
    QString m_text;
};

#endif // OANNOTATIONPREVIEW_H
