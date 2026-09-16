/************************************************************************
 **
 **  @file   oexporttemplateeditor.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/09/2026
 **
 **  @brief  Export template editor — one tab per template family.
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

#ifndef OEXPORTTEMPLATEEDITOR_H
#define OEXPORTTEMPLATEEDITOR_H

#include <QList>
#include <QWidget>

class QLabel;
class QListWidget;
class QShowEvent;
class QSplitter;
class QStackedWidget;
class QTabWidget;
class OExportTemplatePreview;

/**
 * @brief The Export template editor, in the shape of OInfoBlockEditor.
 *
 * Two tabs, one per template family:
 *
 *  - tab 0 « Cover page »      the nomenclature. Not implemented — see
 *                              Modifications/Ostara/ExportMode/PDFTemplateEditor.md §6.
 *  - tab 1 « Assembly marks »  what is added to the sheets of a tiled plan: the frame,
 *                              the marks and the pagination (§3.4).
 *
 * Tab 1 is a fixed-width menu on the left and a stack of pages on the right, as in
 * OInfoBlockEditor: pages are built once in the constructor and never rebuilt, and
 * navigating only calls setCurrentIndex().
 *
 * The shell is what exists here. The pages of the menu, the form of the Edition pane
 * and the marks themselves are not written yet, and say so on screen.
 */
class OExportTemplateEditor : public QWidget
{
    Q_OBJECT

public:
    explicit OExportTemplateEditor(QWidget *parent = nullptr);

    /** @brief Open on the given tab (0 = cover page, 1 = assembly marks). */
    void OpenOnTab(int tab);

    /** @brief Re-apply every user-visible string, after a language change. */
    void RetranslateUI();

protected:
    void showEvent(QShowEvent *event) override;

private:
    QWidget *MakeAssemblyMarksTab();
    QWidget *MakeCreateTemplatePage();
    QWidget *MakeEditionPane();
    QLabel *MakePlaceholderLabel();
    QWidget *MakePlaceholderPage();
    void ShowMenuPage(int row);

    QTabWidget *m_tabs{nullptr};
    QListWidget *m_menuList{nullptr};
    QStackedWidget *m_stack{nullptr};
    OExportTemplatePreview *m_preview{nullptr};
    QSplitter *m_splitter{nullptr};

    /** The 50/50 split is asked for once, when there is a real width to divide. */
    bool m_splitterSized{false};

    /** Placeholders, kept only so a language change can reach their text. */
    QList<QLabel *> m_placeholders;
};

#endif // OEXPORTTEMPLATEEDITOR_H
