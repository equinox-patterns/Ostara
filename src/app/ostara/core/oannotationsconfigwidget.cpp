/************************************************************************
 **
 **  @file   oannotationsconfigwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  Annotations sub-tab of the Information dock tab (see oannotationsconfigwidget.h).
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

#include "oannotationsconfigwidget.h"

#include "../dialogs/oannotationdialog.h"
#include "../vpatterndb/oannotationdata.h"
#include "../vpatterndb/vcontainer.h"
#include "otranslation.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

//---------------------------------------------------------------------------------------------------------------------
OAnnotationsConfigWidget::OAnnotationsConfigWidget(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    // Side margins on the tab content: the add button and the group box are no
    // longer glued to the tab edges.
    layout->setContentsMargins(8, 8, 8, 0);
    layout->setSpacing(12);

    m_addBtn = new QPushButton(tr("Add annotation"), this);
    connect(m_addBtn, &QPushButton::clicked, this, &OAnnotationsConfigWidget::AddAnnotation);
    layout->addWidget(m_addBtn);

    // The list lives DIRECTLY in the group box — no intermediate plain QWidget
    // (a raw QWidget paints no background and lets the $DockTitle viewport show
    // through — SubtiliteCode §14). Same treatment as the Blocks tab: the rows
    // share the group's $DockBody background.
    m_listGroup = new QGroupBox(tr("Active annotations"), this);
    // Less side margin than the theme default (8px) for this group only.
    m_listGroup->setStyleSheet(QStringLiteral("QGroupBox { margin-left: 0px; margin-right: 0px; }"));
    auto *groupLayout = new QVBoxLayout(m_listGroup);
    groupLayout->setSpacing(6);

    m_listLayout = new QVBoxLayout();
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    m_listLayout->setSpacing(8);
    groupLayout->addLayout(m_listLayout);

    m_emptyLabel = new QLabel(tr("No annotations yet. Add one."), m_listGroup);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setWordWrap(true);
    m_listLayout->addWidget(m_emptyLabel);

    groupLayout->addStretch(1);  // keep the rows compact at the top of the group
    layout->addWidget(m_listGroup, 1);
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationsConfigWidget::ClearRows()
{
    for (const AnnotationRow &row : m_rows)
    {
        m_listLayout->removeItem(row.rowLayout);
        delete row.textEdit;  // also deletes the child debounce timer
        delete row.editBtn;
        delete row.removeBtn;
        delete row.rowLayout;  // deletes the inner buttons sublayout
    }
    m_rows.clear();
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationsConfigWidget::SetModifyPending(
    std::function<void(std::function<void(IPieceInfo &)>)> callback)
{
    m_modifyPending = std::move(callback);
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationsConfigWidget::Show(const IPieceInfo &detail, const VContainer *data)
{
    m_currentAnnotations = detail.GetAnnotationData();
    m_data = data;
    m_loading = true;

    const QList<OAnnotation> annotations = detail.GetAnnotationData().Annotations();
    if (m_hasState && m_rows.size() == annotations.size())
    {
        // Same count: update the rows in place so an inline edit keeps focus
        // (the undo refresh after every commit calls Show again). A row with a
        // pending debounce timer is a live edit — never clobber it.
        for (int i = 0; i < m_rows.size(); ++i)
        {
            if (m_rows.at(i).timer->isActive())
            {
                continue;
            }
            QPlainTextEdit *edit = m_rows.at(i).textEdit;
            const QString current = annotations.at(i).text;
            if (edit->toPlainText() != current)
            {
                // Skip the write when unchanged — setPlainText would reset the
                // cursor to position 0 and clear the editor's undo history.
                edit->setPlainText(current);
            }
        }
    }
    else
    {
        RebuildList();
    }

    m_hasState = true;
    m_loading = false;
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationsConfigWidget::Clear()
{
    m_loading = true;
    m_currentAnnotations = OAnnotationData();
    m_data = nullptr;
    ClearRows();
    m_emptyLabel->setVisible(true);
    m_hasState = false;
    m_loading = false;
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationsConfigWidget::RebuildList()
{
    ClearRows();

    const QList<OAnnotation> annotations = m_currentAnnotations.Annotations();
    m_emptyLabel->setVisible(annotations.isEmpty());

    for (int i = 0; i < annotations.size(); ++i)
    {
        // Each annotation is a row layout added directly to the group's list
        // layout — no plain QWidget wrapper (§14), same as the Blocks tab.
        auto *rowLayout = new QVBoxLayout();
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(4);

        auto *textEdit = new QPlainTextEdit(m_listGroup);
        textEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        const auto minHeight = [textEdit]() {
            return textEdit->fontMetrics().lineSpacing() * 3 + textEdit->frameWidth() * 2
                   + static_cast<int>(textEdit->document()->documentMargin()) * 2;
        };
        textEdit->setFixedHeight(minHeight());
        textEdit->setPlainText(annotations.at(i).text);

        auto *timer = new QTimer(textEdit);  // child of the edit — deleted with it
        timer->setSingleShot(true);
        timer->setInterval(600);  // debounce, coalesced by SavePieceOptions::mergeWith

        const int index = i;
        connect(textEdit, &QPlainTextEdit::textChanged, this, [this, index, minHeight]() {
            // Auto-grow (min 3 lines), then debounce the commit.
            QPlainTextEdit *edit = m_rows.at(index).textEdit;
            const int h = static_cast<int>(edit->document()->size().height()) + edit->frameWidth() * 2;
            edit->setFixedHeight((std::max)(h, minHeight()));
            if (!m_loading)
            {
                m_rows.at(index).timer->start();
            }
        });
        connect(timer, &QTimer::timeout, this, [this, index]() {
            m_rows.at(index).timer->stop();
            CommitText(index);
        });

        auto *buttons = new QHBoxLayout();
        buttons->setContentsMargins(0, 0, 0, 0);
        buttons->setSpacing(6);
        // ⚠️ **Les deux boutons de la ligne passent par le poseur.** Ils sont recréés à chaque
        // remplissage de la liste, et `RetranslateUI()` — qui couvre le bouton d'ajout, le titre du
        // groupe et le libellé vide — n'a aucun moyen de les atteindre. Le poseur les couvre tous,
        // quelle que soit la ligne.
        auto *editBtn = new QPushButton(m_listGroup);
        OTranslation::SetText(this, editBtn, QT_TRANSLATE_NOOP("OAnnotationsConfigWidget", "Edit annotation"));
        connect(editBtn, &QPushButton::clicked, this, [this, index]() { EditAnnotation(index); });
        buttons->addWidget(editBtn, 1);
        auto *removeBtn = new QPushButton(m_listGroup);
        OTranslation::SetText(this, removeBtn, QT_TRANSLATE_NOOP("OAnnotationsConfigWidget", "Delete annotation"));
        connect(removeBtn, &QPushButton::clicked, this, [this, index]() { RemoveAnnotation(index); });
        buttons->addWidget(removeBtn, 1);
        // Both buttons stretch equally — the button row spans the same width as
        // the text edit above.

        rowLayout->addWidget(textEdit);
        rowLayout->addLayout(buttons);
        m_listLayout->addLayout(rowLayout);

        AnnotationRow row;
        row.textEdit = textEdit;
        row.timer = timer;
        row.rowLayout = rowLayout;
        row.editBtn = editBtn;
        row.removeBtn = removeBtn;
        m_rows.append(row);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationsConfigWidget::CommitText(int index)
{
    if (m_loading || !m_modifyPending)
    {
        return;
    }
    if (index < 0 || index >= m_rows.size())
    {
        return;
    }
    const QString text = m_rows.at(index).textEdit->toPlainText();
    m_modifyPending([index, text](IPieceInfo &det) {
        OAnnotationData data = det.GetAnnotationData();
        QList<OAnnotation> annotations = data.Annotations();
        if (index >= 0 && index < annotations.size())
        {
            annotations[index].text = text;
            data.SetAnnotations(annotations);
            data.SetSet(true);
            det.SetAnnotationData(data);
        }
    });
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationsConfigWidget::AddAnnotation()
{
    if (m_data == nullptr)
    {
        return;
    }
    OAnnotationDialog dialog(m_data, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        const QString templateName = dialog.TemplateName();
        const QString text = dialog.Text();
        const QString rotationFormula = dialog.RotationFormula();
        m_modifyPending([templateName, text, rotationFormula](IPieceInfo &det) {
            OAnnotationData data = det.GetAnnotationData();
            QList<OAnnotation> annotations = data.Annotations();
            OAnnotation annotation;
            annotation.templateName = templateName;
            annotation.text = text;
            annotation.rotationFormula = rotationFormula;
            annotations.append(annotation);
            data.SetAnnotations(annotations);
            data.SetSet(true);
            det.SetAnnotationData(data);
        });
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationsConfigWidget::EditAnnotation(int index)
{
    if (m_data == nullptr || index < 0 || index >= m_rows.size())
    {
        return;
    }
    // Flush any pending inline edit so it cannot race the modal dialog.
    if (m_rows.at(index).timer->isActive())
    {
        m_rows.at(index).timer->stop();
        CommitText(index);
    }
    const QList<OAnnotation> annotations = m_currentAnnotations.Annotations();
    if (index >= annotations.size())
    {
        return;
    }
    OAnnotation annotation = annotations.at(index);
    // Seed the dialog from the live editor text (a just-flushed pending edit is
    // not yet reflected in the stale snapshot).
    annotation.text = m_rows.at(index).textEdit->toPlainText();

    OAnnotationDialog dialog(m_data, this);
    dialog.SetTemplateName(annotation.templateName);
    dialog.SetText(annotation.text);
    dialog.SetRotationFormula(annotation.rotationFormula);
    if (dialog.exec() == QDialog::Accepted)
    {
        const QString templateName = dialog.TemplateName();
        const QString text = dialog.Text();
        const QString rotationFormula = dialog.RotationFormula();
        m_modifyPending([index, templateName, text, rotationFormula](IPieceInfo &det) {
            OAnnotationData data = det.GetAnnotationData();
            QList<OAnnotation> list = data.Annotations();
            if (index >= 0 && index < list.size())
            {
                list[index].templateName = templateName;
                list[index].text = text;
                list[index].rotationFormula = rotationFormula;
                data.SetAnnotations(list);
                det.SetAnnotationData(data);
            }
        });
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationsConfigWidget::RemoveAnnotation(int index)
{
    if (index < 0 || index >= m_rows.size())
    {
        return;
    }
    // Flush any pending inline edit so it cannot race the confirmation dialog.
    if (m_rows.at(index).timer->isActive())
    {
        m_rows.at(index).timer->stop();
        CommitText(index);
    }
    const QList<OAnnotation> annotations = m_currentAnnotations.Annotations();
    if (index >= annotations.size())
    {
        return;
    }
    const QString live = m_rows.at(index).textEdit->toPlainText();
    const QString preview = live.isEmpty() ? annotations.at(index).text : live;
    const auto answer = QMessageBox::question(
        this, tr("Delete annotation"),
        tr("Delete the annotation \"%1\"?").arg(preview.left(40)));
    if (answer != QMessageBox::Yes)
    {
        return;
    }
    m_modifyPending([index](IPieceInfo &det) {
        OAnnotationData data = det.GetAnnotationData();
        QList<OAnnotation> list = data.Annotations();
        if (index >= 0 && index < list.size())
        {
            list.removeAt(index);
            data.SetAnnotations(list);
            det.SetAnnotationData(data);
        }
    });
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationsConfigWidget::RetranslateUI()
{
    m_addBtn->setText(tr("Add annotation"));
    m_listGroup->setTitle(tr("Active annotations"));
    m_emptyLabel->setText(tr("No annotations yet. Add one."));
    if (m_hasState)
    {
        // The row buttons (Edit/Remove) hold translated literals — rebuild the
        // rows to pick up the new language.
        m_loading = true;
        RebuildList();
        m_loading = false;
    }
}
