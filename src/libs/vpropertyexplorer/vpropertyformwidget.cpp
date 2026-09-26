/************************************************************************
 **
 **  @file   vpropertyformwidget.cpp
 **  @author hedgeware <internal(at)hedgeware.net>
 **  @date
 **
 **  @brief
 **  @copyright
 **  All rights reserved. This program and the accompanying materials
 **  are made available under the terms of the GNU Lesser General Public License
 **  (LGPL) version 2.1 which accompanies this distribution, and is available at
 **  http://www.gnu.org/licenses/lgpl-2.1.html
 **
 **  This library is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 **  Lesser General Public License for more details.
 **
 *************************************************************************/

#include "vpropertyformwidget.h"

#include <QApplication>
#include <QEvent>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QLayoutItem>
#include <QMargins>
#include <QMouseEvent>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QStyleOptionViewItem>
#include <QVariant>
#include <QWidget>

#include <functional>

#include "plugins/vwidgetproperty.h"
#include "vproperty.h"
#include "vpropertyformwidget_p.h"

namespace
{
// A flat, non-button collapsible header: "Title ▾/▴" where clicking toggles the callback.
class CollapsibleHeader : public QFrame
{
public:
    CollapsibleHeader(const QString &text, const QPixmap &expandPixmap, const QPixmap &collapsePixmap,
                      bool initiallyCollapsed, std::function<void(bool)> onToggle, QWidget *parent)
      : QFrame(parent),
        m_onToggle(onToggle),
        m_collapsed(initiallyCollapsed),
        m_expandPixmap(expandPixmap),
        m_collapsePixmap(collapsePixmap)
    {
        setFrameStyle(QFrame::NoFrame);
        {
            auto p = palette();
            p.setColor(QPalette::Window, qApp->palette().button().color());
            setPalette(p);
            setAutoFillBackground(true);
        }

        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);

        auto *title = new QLabel(text, this);
        title->setAttribute(Qt::WA_TransparentForMouseEvents);

        m_caret = new QLabel(this);
        m_caret->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_caret->setPixmap(m_collapsed ? m_expandPixmap : m_collapsePixmap);

        layout->addWidget(title);
        layout->addWidget(m_caret);
        layout->addStretch();
    }

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        m_collapsed = !m_collapsed;
        m_caret->setPixmap(m_collapsed ? m_expandPixmap : m_collapsePixmap);
        m_onToggle(m_collapsed);
        QWidget::mousePressEvent(event);
    }

private:
    std::function<void(bool)> m_onToggle;
    bool m_collapsed;
    QLabel *m_caret;
    QPixmap m_expandPixmap;
    QPixmap m_collapsePixmap;
};
} // namespace

VPE::VPropertyFormWidget::VPropertyFormWidget(const QString &title, const QString &description,
                                              const QList<VProperty *> &properties, QWidget *parent)
  : QGroupBox(title, parent),
    vproperty_d_ptr(new VPropertyFormWidgetPrivate(properties))
{
    VPE::VPropertyFormWidget::build();
    setToolTip(description);
    setWhatsThis(description);
}

VPE::VPropertyFormWidget::VPropertyFormWidget(VProperty *parent_property, QWidget *parent)
  : QGroupBox(parent),
    vproperty_d_ptr(new VPropertyFormWidgetPrivate())
{
    if (parent_property)
    {
        vproperty_d_ptr->Properties = parent_property->getChildren();
        VPE::VPropertyFormWidget::build();
        setTitle(parent_property->getName());
        setToolTip(parent_property->getDescription());
        setWhatsThis(parent_property->getDescription());
    }
}

VPE::VPropertyFormWidget::VPropertyFormWidget(VPropertyFormWidgetPrivate *d_pointer, QWidget *parent,
                                              const QString &title, const QString &description)
  : QGroupBox(title, parent),
    vproperty_d_ptr(d_pointer)
{
    VPE::VPropertyFormWidget::build();
    setToolTip(description);
    setWhatsThis(description);
}

VPE::VPropertyFormWidget::~VPropertyFormWidget()
{
    delete vproperty_d_ptr;
}

void VPE::VPropertyFormWidget::setDarkTheme(bool dark)
{
    m_dark = dark;
}

void VPE::VPropertyFormWidget::build()
{
    // Clear the old content, delete old widgets
    vproperty_d_ptr->EditorWidgets.clear();
    if (layout() != nullptr)
    {
        while (layout()->count() > 0)
        {
            if (QLayoutItem *const child = layout()->takeAt(0)) // NOLINT(misc-const-correctness)
            {
                delete child->widget();
                delete child;
            }
        }
        delete layout();
    }

    // Create new content
    if (vproperty_d_ptr->Properties.isEmpty())
    {
        return; //... only if there are properties
    }

    auto *tmpFormLayout = new QFormLayout(this);
    tmpFormLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(tmpFormLayout);

    for (int i = 0; i < vproperty_d_ptr->Properties.count(); ++i)
    {
        // Get the current property
        VProperty *tmpProperty = vproperty_d_ptr->Properties.value(i, nullptr);
        if (!tmpProperty)
        {
            continue;
        }

        if (tmpProperty->getRowCount() > 0)
        {
            if (tmpProperty->propertyType() == Property::Complex)
            {
                buildEditor(tmpProperty, tmpFormLayout, Property::Complex);
                // Plain QWidget does not paint backgrounds — it lacks the
                // paintEvent required to render QSS or autoFill (doc-rendu §05).
                // Use QFrame (which has a proper paintEvent) instead.
                // Force QPalette::Window to DockBody (the viewport's DockTitle
                // palette cascades down through the parent chain).
                auto *group = new QFrame(this);
                group->setFrameStyle(QFrame::NoFrame);
                {
                    auto p = group->palette();
                    p.setColor(QPalette::Window, qApp->palette().button().color());
                    group->setPalette(p);
                    group->setAutoFillBackground(true);
                }
                tmpFormLayout->addRow(group);

                auto *subFormLayout = new QFormLayout(group);
                subFormLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
                QMargins margins = subFormLayout->contentsMargins();
                margins.setTop(0);
                margins.setLeft(18);
                margins.setRight(0);
                subFormLayout->setContentsMargins(margins);

                group->setLayout(subFormLayout);
                QList<VProperty *> children = tmpProperty->getChildren();
                for (int j = 0; j < children.size(); ++j)
                {
                    buildEditor(children[j], subFormLayout);
                    connect(children[j], &VProperty::childChanged, tmpProperty, &VProperty::ValueChildChanged,
                            Qt::UniqueConnection);
                    ++i;
                    vproperty_d_ptr->Properties.insert(i, children[j]);
                }
                if (QWidget *bottom = tmpProperty->bottomWidget(); bottom != nullptr)
                {
                    bottom->setParent(group);
                    subFormLayout->addWidget(bottom);
                }
            }
            else if (tmpProperty->isCollapsible())
            {
                // Collapsible group: a flat header (text + caret) toggles a hideable content frame.
                const QString theme = m_dark ? QStringLiteral("dark") : QStringLiteral("light");
                const QPixmap expandPixmap(QStringLiteral(":/icons/%1/caret-down_%2.svg").arg(theme, theme));
                const QPixmap collapsePixmap(QStringLiteral(":/icons/%1/caret-up_%2.svg").arg(theme, theme));

                auto *content = new QFrame(this);
                content->setObjectName(QStringLiteral("propertyCollapseContent"));
                auto *subFormLayout = new QFormLayout(content);
                subFormLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
                QMargins margins = subFormLayout->contentsMargins();
                margins.setTop(0);
                margins.setLeft(18);
                margins.setRight(0);
                subFormLayout->setContentsMargins(margins);
                content->setLayout(subFormLayout);

                // The collapsible group occupies a row in Properties (its header) but has no
                // editor of its own. Append a placeholder so EditorWidgets stays index-aligned
                // with Properties — commitData()/loadData() pair them by row index.
                vproperty_d_ptr->EditorWidgets.append(VPropertyFormWidgetPrivate::SEditorWidget());

                QList<VProperty *> children = tmpProperty->getChildren();
                for (int j = 0; j < children.size(); ++j)
                {
                    VProperty *const child = children[j];
                    if (child->propertyType() == Property::Complex)
                    {
                        // A Complex child (e.g. VFormulaProperty) renders as a bold row plus an
                        // indented sub-frame holding its own children (the "Formula" field).
                        buildEditor(child, subFormLayout, Property::Complex);
                        auto *childGroup = new QFrame(content);
                        childGroup->setFrameStyle(QFrame::NoFrame);
                        {
                            auto p = childGroup->palette();
                            p.setColor(QPalette::Window, qApp->palette().button().color());
                            childGroup->setPalette(p);
                            childGroup->setAutoFillBackground(true);
                        }
                        subFormLayout->addRow(childGroup);
                        auto *childSubLayout = new QFormLayout(childGroup);
                        childSubLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
                        QMargins childMargins = childSubLayout->contentsMargins();
                        childMargins.setTop(0);
                        childMargins.setLeft(18);
                        childMargins.setRight(0);
                        childSubLayout->setContentsMargins(childMargins);
                        childGroup->setLayout(childSubLayout);
                        ++i;
                        vproperty_d_ptr->Properties.insert(i, child);
                        const QList<VProperty *> grandchildren = child->getChildren();
                        for (int k = 0; k < grandchildren.size(); ++k)
                        {
                            buildEditor(grandchildren[k], childSubLayout);
                            connect(grandchildren[k], &VProperty::childChanged, child,
                                    &VProperty::ValueChildChanged, Qt::UniqueConnection);
                            ++i;
                            vproperty_d_ptr->Properties.insert(i, grandchildren[k]);
                        }
                        if (QWidget *bottom = child->bottomWidget(); bottom != nullptr)
                        {
                            bottom->setParent(childGroup);
                            childSubLayout->addWidget(bottom);
                        }
                    }
                    else
                    {
                        buildEditor(child, subFormLayout);
                        ++i;
                        vproperty_d_ptr->Properties.insert(i, child);
                    }
                }

                content->setVisible(tmpProperty->isExpandedByDefault());

                auto *header = new CollapsibleHeader(tmpProperty->getName(), expandPixmap, collapsePixmap,
                                                     !tmpProperty->isExpandedByDefault(),
                                                     [content](bool collapsed) { content->setVisible(!collapsed); }, this);
                header->setObjectName(QStringLiteral("propertyCollapseHeader"));

                tmpFormLayout->addRow(header);
                tmpFormLayout->addRow(content);
            }
            else
            {
                // Child properties, the property itself is not being added
                auto *tmpNewFormWidget = new VPropertyFormWidget(tmpProperty, this);
                tmpFormLayout->addRow(tmpNewFormWidget);
                vproperty_d_ptr->EditorWidgets.append(VPropertyFormWidgetPrivate::SEditorWidget(tmpNewFormWidget));
                tmpNewFormWidget->setCommitBehaviour(vproperty_d_ptr->UpdateEditors);
            }
        }
        else if (tmpProperty->type() == "widget")
        {
            auto *tmpWidgetProperty = static_cast<VWidgetProperty *>(tmpProperty);
            tmpFormLayout->addRow(tmpWidgetProperty->getWidget());
            vproperty_d_ptr->EditorWidgets.append(VPropertyFormWidgetPrivate::SEditorWidget(tmpWidgetProperty->getWidget()));
        }
        else
        {
            buildEditor(tmpProperty, tmpFormLayout);
        }
    }
}

void VPE::VPropertyFormWidget::buildEditor(VProperty *property, QFormLayout *formLayout, Property type)
{
    // Add property (no child properties)
    // Create the editor (if it doesn't work, create empty widget)
    QWidget *tmpEditor = property->createEditor(this, QStyleOptionViewItem(), nullptr);
    if (!tmpEditor)
    {
        tmpEditor = new QWidget(this);
    }

    // set tooltip and whats this
    tmpEditor->setToolTip(property->getDescription());
    tmpEditor->setWhatsThis(property->getDescription());

    // Install event filter
    tmpEditor->installEventFilter(this);

    // Set the editor data
    property->setEditorData(tmpEditor);

    // add new row
    if (type == Property::Complex)
    {
        QString const name = "<b>" + property->getName() + "</b>";
        formLayout->addRow(name, tmpEditor);
    }
    else if (not property->getNameDecoration().isNull())
    {
        // QFrame instead of QWidget — plain QWidget has no paintEvent
        // and does not render backgrounds (SUBTILITES_CODE.md §14).
        auto *labelWidget = new QFrame(this);
        static_cast<QFrame *>(labelWidget)->setFrameStyle(QFrame::NoFrame);
        auto *labelLayout = new QHBoxLayout(labelWidget);
        labelLayout->setContentsMargins(0, 0, 0, 0);
        labelLayout->setSpacing(4);
        auto *dotLabel = new QLabel(labelWidget);
        dotLabel->setPixmap(property->getNameDecoration());
        auto *textLabel = new QLabel(property->getName(), labelWidget);
        labelLayout->addWidget(dotLabel);
        labelLayout->addWidget(textLabel);
        labelLayout->addStretch();
        formLayout->addRow(labelWidget, tmpEditor);
    }
    else if (property->isSpanningRow())
    {
        // The label shares the line with the editor and stays out of the form's label column: the row is
        // full width, flush with the left edge, and its label can never widen the column that pushes the
        // field of every shorter row to the right.
        auto *row = new QFrame(this);
        row->setFrameStyle(QFrame::NoFrame);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(8);
        rowLayout->addWidget(new QLabel(property->getName(), row));
        rowLayout->addWidget(tmpEditor, 1);
        formLayout->addRow(row);
    }
    else
    {
        formLayout->addRow(property->getName(), tmpEditor);
    }

    vproperty_d_ptr->EditorWidgets.append(VPropertyFormWidgetPrivate::SEditorWidget(tmpEditor));
}

void VPE::VPropertyFormWidget::commitData()
{
    for (int i = 0; i < vproperty_d_ptr->Properties.count(); ++i)
    {
        commitData(i);
    }
}

void VPE::VPropertyFormWidget::loadData()
{
    for (int i = 0; i < vproperty_d_ptr->Properties.count(); ++i)
    {
        loadData(i);
    }
}

void VPE::VPropertyFormWidget::commitData(int row)
{
    if (row < 0 || row >= vproperty_d_ptr->EditorWidgets.count() || row >= vproperty_d_ptr->Properties.count())
    {
        return;
    }

    VPropertyFormWidgetPrivate::SEditorWidget &tmpEditorWidget = vproperty_d_ptr->EditorWidgets[row];
    VProperty *tmpProperty = vproperty_d_ptr->Properties[row];
    if (tmpEditorWidget.FormWidget)
    {
        tmpEditorWidget.FormWidget->commitData();
    }
    else if (tmpEditorWidget.Editor && tmpProperty)
    {
        QVariant const newValue = tmpProperty->getEditorData(tmpEditorWidget.Editor);
        QVariant const oldValue = tmpProperty->data(VProperty::DPC_Data, Qt::EditRole);
        if (oldValue != newValue)
        {
            VProperty *parent = tmpProperty->getParent();
            if (parent != nullptr && parent->propertyType() == Property::Complex)
            {
                tmpProperty->UpdateParent(newValue);
                emit propertyDataSubmitted(parent);
            }
            else if (parent != nullptr && parent->isCollapsible())
            {
                tmpProperty->setValue(newValue);
                emit propertyDataSubmitted(parent);
            }
            else
            {
                tmpProperty->setValue(newValue);
                emit propertyDataSubmitted(tmpProperty);
            }
        }
    }
}

void VPE::VPropertyFormWidget::loadData(int row)
{
    if (row < 0 || row >= vproperty_d_ptr->EditorWidgets.count() || row >= vproperty_d_ptr->Properties.count())
    {
        return;
    }

    VPropertyFormWidgetPrivate::SEditorWidget &tmpEditorWidget = vproperty_d_ptr->EditorWidgets[row];
    VProperty *tmpProperty = vproperty_d_ptr->Properties[row];
    if (tmpEditorWidget.FormWidget)
    {
        tmpEditorWidget.FormWidget->loadData();
    }
    else if (tmpEditorWidget.Editor && tmpProperty)
    {
        tmpProperty->setEditorData(tmpEditorWidget.Editor);
    }
}

void VPE::VPropertyFormWidget::setCommitBehaviour(bool auto_commit)
{
    vproperty_d_ptr->UpdateEditors = auto_commit;

    const QList<VPropertyFormWidget *> tmpChildFormWidgets = getChildPropertyFormWidgets();
    for (auto *tmpChild : tmpChildFormWidgets)
    {
        if (tmpChild)
        {
            tmpChild->setCommitBehaviour(auto_commit);
        }
    }
}

auto VPE::VPropertyFormWidget::getChildPropertyFormWidgets() const -> QList<VPE::VPropertyFormWidget *>
{
    QList<VPropertyFormWidget *> tmpResult;
    for (auto &tmpEditorWidget : vproperty_d_ptr->EditorWidgets)
    {
        if (tmpEditorWidget.FormWidget)
        {
            tmpResult.append(tmpEditorWidget.FormWidget);
        }
    }

    return tmpResult;
}

auto VPE::VPropertyFormWidget::eventFilter(QObject *object, QEvent *event) -> bool
{
    if (!vproperty_d_ptr->UpdateEditors)
    {
        return false;
    }

    QWidget *editor = qobject_cast<QWidget *>(object);
    if (!editor)
    {
        return false;
    }

    if (event->type() == QEvent::KeyPress)
    {
        if (qobject_cast<QPlainTextEdit *>(editor))
        {
            switch (static_cast<QKeyEvent *>(event)->key())
            {
                case Qt::Key_Escape:
                    commitData(editor);
                    event->accept();
                    return true;
                case Qt::Key_Tab:
                case Qt::Key_Backtab:
                case Qt::Key_Enter:
                case Qt::Key_Return:
                default:
                    return false;
            }
        }
        else
        {
            switch (static_cast<QKeyEvent *>(event)->key())
            {
                case Qt::Key_Tab:
                case Qt::Key_Backtab:
                case Qt::Key_Enter:
                case Qt::Key_Return:
                case Qt::Key_Escape:
                    commitData(editor);
                    event->accept();
                    return true;
                default:
                    return false;
            }
        }
        return false;
    }
    if (event->type() == QEvent::FocusOut || (event->type() == QEvent::Hide && editor->isWindow()))
    {
        commitData(editor);
        return false;
    }
    else if (event->type() == QEvent::ShortcutOverride)
    {
        if (static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape)
        {
            commitData(editor);
            event->accept();
            return true;
        }
    }
    else if (event->type() == MyCustomEventType)
    {
        commitData(editor);
        event->accept();
        return true;
    }
    else
    {
        return QGroupBox::eventFilter(object, event);
    }

    // Default:
    return false;
}

void VPE::VPropertyFormWidget::commitData(const QWidget *editor)
{
    if (!editor)
    {
        return;
    }

    for (int i = 0; i < vproperty_d_ptr->EditorWidgets.count(); ++i)
    {
        VPropertyFormWidgetPrivate::SEditorWidget const &tmpEditorWidget = vproperty_d_ptr->EditorWidgets[i];
        if (tmpEditorWidget.Editor == editor)
        {
            commitData(i);
        }
    }
}
