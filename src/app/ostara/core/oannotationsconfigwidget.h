/************************************************************************
 **
 **  @file   oannotationsconfigwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  Annotations sub-tab of the Information dock tab.
 **
 **  Lists the annotations of the active piece: inline-editable content plus
 **  Edit / Remove buttons and an "Add annotation" entry. Mutates VPiece
 **  through the same snapshot + undo pipeline as the rest of the dock
 **  (SetModifyPending / ModifyPiece). Inline text commits are debounced and
 **  coalesced by SavePieceOptions::mergeWith.
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

#ifndef OANNOTATIONSCONFIGWIDGET_H
#define OANNOTATIONSCONFIGWIDGET_H

#include <QWidget>

#include <functional>

#include "../vpatterndb/ipieceinfo.h"
#include "../vpatterndb/oannotationdata.h"

class QGroupBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QTimer;
class QVBoxLayout;
class VContainer;

/**
 * @brief Annotations sub-tab of the Information dock tab.
 *
 * No inner scroll area (§4.3): the list lives directly in the widget so it
 * shares the group's $DockBody background.
 */
class OAnnotationsConfigWidget : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit OAnnotationsConfigWidget(QWidget *parent = nullptr);
    ~OAnnotationsConfigWidget() override = default;

    /** Callback that applies a piece-like mutation and commits it to the undo stack. */
    void SetModifyPending(std::function<void(std::function<void(IPieceInfo &)>)> callback);

    void Show(const IPieceInfo &detail, const VContainer *data);
    void Clear();
    void RetranslateUI();

private:
    struct AnnotationRow
    {
        QPlainTextEdit *textEdit{nullptr};
        QTimer *timer{nullptr};
        QVBoxLayout *rowLayout{nullptr};
        QPushButton *editBtn{nullptr};
        QPushButton *removeBtn{nullptr};
    };

    void RebuildList();
    void ClearRows();
    void AddAnnotation();
    void EditAnnotation(int index);
    void RemoveAnnotation(int index);
    void OnRowTextChanged(int index);
    void CommitText(int index);

    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(OAnnotationsConfigWidget) // NOLINT

    std::function<void(std::function<void(IPieceInfo &)>)> m_modifyPending;

    QPushButton *m_addBtn{nullptr};
    QGroupBox *m_listGroup{nullptr};
    QVBoxLayout *m_listLayout{nullptr};
    QLabel *m_emptyLabel{nullptr};

    QList<AnnotationRow> m_rows;
    OAnnotationData m_currentAnnotations;
    const VContainer *m_data{nullptr};
    bool m_loading{false};
    bool m_hasState{false};
};

#endif // OANNOTATIONSCONFIGWIDGET_H
