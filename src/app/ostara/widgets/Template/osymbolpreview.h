/************************************************************************
 **
 **  @file   osymbolpreview.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/09/2026
 **
 **  @brief  One line of a legend — the mark at the size it prints at, and
 **          the words beside it.
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

#ifndef OSYMBOLPREVIEW_H
#define OSYMBOLPREVIEW_H

#include <QString>
#include <QWidget>

#include "core/osymbolshapes.h"

/**
 * @brief One line of a legend: the mark, at the size it will print at, and the words beside it.
 *
 * **A symbol is small, and it is judged small.** It stands on a page a few centimetres wide, where
 * it is a centimetre or two tall; a pane that blew it up to fill itself would show a symbol nobody
 * will ever see, and would hide the only question worth asking here — does this mark still read at
 * the size it is printed at.
 *
 * So the marks are drawn at their own size and the words stand beside them, which is how the legend
 * will be read: a mark and a word have to hold together, and a word twice the mark's height is a
 * line that will not fit on a page either.
 *
 * **A symbol standing for several marks shows them all, on one line.** Three notches named "notches"
 * are one line of a legend and are drawn as one: side by side, in the order they were listed, with
 * the word after the last of them.
 *
 * The mark is drawn in the pane's text colour rather than in black, so that it stays legible in
 * either theme: it stands on the base colour, not on paper.
 */
class OSymbolPreview : public QWidget
{
    Q_OBJECT

public:
    explicit OSymbolPreview(QWidget *parent = nullptr);

    /// Show one legend line: @p shapes as @p text names them. Clearing shows a placeholder instead.
    void SetSymbol(const QVector<OSymbolShapes::Shape> &shapes, const QString &text);
    void Clear();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<OSymbolShapes::Shape> m_shapes;
    QString m_text;
    bool m_hasSymbol = false;
};

#endif // OSYMBOLPREVIEW_H
