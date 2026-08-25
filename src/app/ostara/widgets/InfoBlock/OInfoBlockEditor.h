/************************************************************************
 **
 **  @file   OInfoBlockEditor.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/08/2026
 **
 **  @brief  Information block editor — the template editing widget (3 zones).
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

#ifndef OINFOBLOCKEDITOR_H
#define OINFOBLOCKEDITOR_H

#include "OPieceCartoucheTable.h"
#include "OPieceCartoucheTemplate.h"

#include <QWidget>

class QLineEdit;
class QListWidget;
class QPushButton;
class QStackedWidget;
class OInfoBlockPreview;
class OInfoBlockSectionEditor;

/**
 * @brief Information block editor — the template editing widget.
 *
 * Left menu (Existing templates · Create new · Libraries) + a stacked content
 * area. The editor page holds the 3 zones: top bar (name + « + Add information »
 * dropdown + order arrows + save), ordered section list + data-driven edit zone
 * (left), live preview (right). Pattern: MeasurementWorkspace::SetupUI.
 */
class OInfoBlockEditor : public QWidget
{
    Q_OBJECT

public:
    explicit OInfoBlockEditor(QWidget *parent = nullptr);

    auto IsModified() const -> bool;

public slots:
    void OnSave();
    void OnSaveAs();

signals:
    void DocumentModified();
    void StatusMessage(const QString &message);

private:
    void SetupUI();
    void SetupEditorPage();
    void SetupTemplatesPage();
    void NewTemplate();
    void ShowTemplatesList();
    void LoadFile(const QString &filePath);
    void OnAddSection(const QString &typeId);
    void RefreshTemplateList();
    void RefreshSectionList();
    void SelectSection(int index);
    void RefreshPreview();
    void DoSave(const QString &filePath);
    void SetModified(bool modified);

    OPieceCartoucheTemplate m_template;
    OPieceCartoucheTable m_table;  // read access to the loaded .omt (Sizes / Measurement)
    QString m_filePath;
    bool m_modified{false};

    QListWidget *m_menuList{nullptr};
    QStackedWidget *m_stack{nullptr};

    // Editor page
    QLineEdit *m_nameEdit{nullptr};
    QPushButton *m_addBtn{nullptr};
    QListWidget *m_sectionList{nullptr};
    OInfoBlockSectionEditor *m_sectionEditor{nullptr};
    OInfoBlockPreview *m_preview{nullptr};

    // Templates page
    QListWidget *m_templatesList{nullptr};
};

#endif // OINFOBLOCKEDITOR_H
