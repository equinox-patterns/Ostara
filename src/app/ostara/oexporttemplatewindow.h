/************************************************************************
 **
 **  @file   oexporttemplatewindow.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/09/2026
 **
 **  @brief  Modeless « Export templates » window wrapping the template editor.
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

#ifndef OEXPORTTEMPLATEWINDOW_H
#define OEXPORTTEMPLATEWINDOW_H

#include <QMainWindow>

class QCloseEvent;
class QEvent;
class OExportTemplateEditor;

/**
 * @brief Modeless « Export templates » window (pattern: OInfoBlockWindow).
 *
 * Owns the template editor widget. Qt::WA_DeleteOnClose is applied by the caller
 * (the main window).
 *
 * Unlike OInfoBlockWindow this window keeps **no** geometry: it always opens at the size
 * it computes for itself — nine tenths of the screen it opens on. The user may resize it,
 * but a restart starts from that size again, not from theirs.
 *
 * Closing asks before losing work. The editor never saves on its own, so a template that has
 * been changed and not saved would go with the window; the prompt offers to save it, to leave
 * it, or to stay.
 */
class OExportTemplateWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit OExportTemplateWindow(QWidget *parent = nullptr);

    /** @brief Open the editor on the given tab (0 = cover page, 1 = assembly marks). */
    void OpenOnTab(int tab);

protected:
    void changeEvent(QEvent *event) override;

    /** @brief Asks before closing on changes that were never saved. */
    void closeEvent(QCloseEvent *event) override;

private:
    OExportTemplateEditor *m_editor{nullptr};
};

#endif // OEXPORTTEMPLATEWINDOW_H
