/************************************************************************
 **
 **  @file   OInfoBlockWindow.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/08/2026
 **
 **  @brief  Modeless « Information block » window wrapping the template editor.
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

#ifndef OINFOBLOCKWINDOW_H
#define OINFOBLOCKWINDOW_H

#include <QMainWindow>

class QEvent;
class OInfoBlockEditor;

/**
 * @brief Modeless « Information block » window (pattern: MeasurementWindow).
 *
 * Owns the template editor widget, persists its geometry (settings key
 * "infoBlockWindowGeometry") and prompts for unsaved changes on close.
 * Qt::WA_DeleteOnClose is applied by the caller (the main window).
 */
class OInfoBlockWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit OInfoBlockWindow(QWidget *parent = nullptr);

    /** @brief Open the editor on the given page (0..4), see OInfoBlockEditor::OpenOnPage. */
    void OpenOnPage(int page);

signals:
    /** @brief Relayed from the editor when a template is saved (refresh the canvas blocks). */
    void TemplateSaved();

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    OInfoBlockEditor *m_editor{nullptr};
};

#endif // OINFOBLOCKWINDOW_H
