/************************************************************************
 **
 **  @file   odialogfit.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   10/11/2026
 **
 **  @brief  One rule for every dialog of the application: never taller than
 **          the screen, never stretched inside.
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

#include "odialogfit.h"

#include <QAbstractButton>
#include <QAbstractScrollArea>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGraphicsView>
#include <QHash>
#include <QLayout>
#include <QLoggingCategory>
#include <QPointer>
#include <QVector>
#include <QScreen>
#include <QScrollArea>
#include <QSplitter>
#include <QStyle>
#include <QTabWidget>
#include <QTextEdit>
#include <QTimer>
#include <QWindow>

Q_LOGGING_CATEGORY(vDialogFit, "vDialogFit")

namespace
{
// A frame smaller than this has nothing to protect: no dialog of the application is that short,
// and shrinking below it would be an answer to a screen that does not exist.
constexpr int kMinFrameHeight = 160;

// Nor is a dialog narrower than this: a screen that narrow does not exist, and shrinking to it would
// answer a question nobody asked.
constexpr int kMinFrameWidth = 200;

// The frame margins reported by the window system are right to a pixel or two; rounding them the
// wrong way is the difference between "the buttons are visible" and "they are not".
constexpr int kFitSlack = 2;

const char kCapProperty[] = "oDialogFitCap";
const char kCapWidthProperty[] = "oDialogFitCapWidth";
const char kPendingProperty[] = "oDialogFitPending";
const char kWrappedProperty[] = "oDialogFitWrapped";

// Every dialog the policy has met, with what it asked for and what it got. Written out once, when the
// application quits: one block to read, instead of a hunt through the log for the ones that went wrong.
struct FitRecord
{
    QSize needed;
    QSize room;
    ODialogFit::Verdict verdict{ODialogFit::Verdict::Skipped};
};

auto Inventory() -> QHash<QString, FitRecord> &
{
    static QHash<QString, FitRecord> inventory;
    return inventory;
}

//---------------------------------------------------------------------------------------------------------------------
auto VerdictName(ODialogFit::Verdict verdict) -> QString
{
    switch (verdict)
    {
        case ODialogFit::Verdict::Fits:
            return QStringLiteral("fits");
        case ODialogFit::Verdict::Scrolls:
            return QStringLiteral("scrolls");
        case ODialogFit::Verdict::Overflows:
            return QStringLiteral("OVERFLOWS");
        case ODialogFit::Verdict::Skipped:
            break;
    }
    return QStringLiteral("skipped");
}

//---------------------------------------------------------------------------------------------------------------------
void DumpInventory()
{
    for (const QString &line : ODialogFit::InventoryReport())
    {
        qCInfo(vDialogFit) << line;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogKey(const QWidget *dialog) -> QString
{
    // A test, and more than one dialog of a program, may carry the same class name: the object name says
    // which one when it is set.
    const QString named = dialog->objectName();
    return named.isEmpty() ? QString::fromLatin1(dialog->metaObject()->className()) : named;
}

auto AvailableFor(const QWidget *widget) -> QRect
{
    const QScreen *screen = widget->screen();
    return screen != nullptr ? screen->availableGeometry() : QRect();
}

// The bar of buttons is the part that must never scroll away: it is the only way out of the
// dialog, and a frame that runs past the edge of the screen takes it first. Some dialogs build it
// with a QDialogButtonBox, others with plain buttons in a row of their own — both are recognised,
// and so is any last row that holds buttons and nothing to fill in.
auto HoldsButtons(QLayoutItem *item) -> bool
{
    if (item == nullptr)
    {
        return false;
    }

    if (QWidget *widget = item->widget())
    {
        return qobject_cast<QAbstractButton *>(widget) != nullptr
               || qobject_cast<QDialogButtonBox *>(widget) != nullptr
               || !widget->findChildren<QAbstractButton *>().isEmpty();
    }

    if (QLayout *layout = item->layout())
    {
        for (int i = 0; i < layout->count(); ++i)
        {
            if (HoldsButtons(layout->itemAt(i)))
            {
                return true;
            }
        }
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto HoldsInput(QLayoutItem *item) -> bool
{
    if (item == nullptr)
    {
        return false;
    }

    if (QWidget *widget = item->widget())
    {
        return qobject_cast<QDialogButtonBox *>(widget) == nullptr
               && (qobject_cast<QLineEdit *>(widget) != nullptr || qobject_cast<QComboBox *>(widget) != nullptr
                   || qobject_cast<QAbstractSpinBox *>(widget) != nullptr
                   || qobject_cast<QAbstractScrollArea *>(widget) != nullptr
                   || !widget->findChildren<QLineEdit *>().isEmpty());
    }

    if (QLayout *layout = item->layout())
    {
        for (int i = 0; i < layout->count(); ++i)
        {
            if (HoldsInput(layout->itemAt(i)))
            {
                return true;
            }
        }
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto ContainsButtonBar(QLayoutItem *item) -> bool
{
    return qobject_cast<QDialogButtonBox *>(item != nullptr ? item->widget() : nullptr) != nullptr
           || (HoldsButtons(item) && !HoldsInput(item));
}

// One cap, applied or left alone. `previous` is the cap this class set last time (zero when it never
// did): a dialog that locked its own size keeps it, because only a cap that would let it run past the
// screen has the right to overrule a decision.
template <typename Getter, typename Setter>
void CapTo(QWidget *dialog, const char *property, Getter getter, Setter setter, int cap)
{
    const int previous = dialog->property(property).toInt();
    const int current = (dialog->*getter)();
    // "Ours" means we have already written this property, and it still holds what we wrote. On the very
    // first pass nothing is ours: a dialog that locked its own size below the screen keeps it, and only a
    // maximum that would let the frame run past the screen is overruled.
    const bool ours = previous != 0 && current == previous;
    if (current != cap && (ours || current > cap))
    {
        (dialog->*setter)(cap);
    }
    dialog->setProperty(property, cap);
}

auto MakeScrollArea(QWidget *parent) -> QScrollArea *
{
    auto *scroll = new QScrollArea(parent);
    scroll->setObjectName(QStringLiteral("oDialogFitScrollArea"));
    // The content keeps its own width and takes the height it is given; the bars appear only when
    // the content is genuinely larger than the viewport, never as a decoration.
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    return scroll;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
ODialogFit::ODialogFit(QObject *parent)
  : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFit::Install()
{
    static ODialogFit *filter = nullptr;
    if (filter == nullptr)
    {
        filter = new ODialogFit(qApp);
        qApp->installEventFilter(filter);
        QObject::connect(qApp, &QCoreApplication::aboutToQuit, qApp, &DumpInventory);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogFit::Apply(QWidget *dialog, const QRect &available) -> Verdict
{
    // Applying the policy rewrites size policies and may insert a scroll area, both of which ask
    // for another layout pass — and that pass comes back here. One application at a time.
    static bool applying = false;
    if (applying || dialog == nullptr || available.isEmpty())
    {
        return Verdict::Skipped;
    }

    applying = true;
    struct Reset
    {
        Reset() = default;
        ~Reset() { applying = false; }
    } const reset;

    // Invariant 2 first: compacting a form row changes how tall the dialog wants to be, so it has
    // to happen before anything is measured.
    CompactFormRows(dialog);

    const QMargins chrome = FrameChrome(dialog);
    const int limitH = available.height() - chrome.top() - chrome.bottom();
    const int limitW = available.width() - chrome.left() - chrome.right();
    if (limitH < kMinFrameHeight || limitW < kMinFrameWidth)
    {
        return Verdict::Skipped;
    }

    bool wrapped = dialog->property(kWrappedProperty).toBool();
    const QSize neededBefore = dialog->minimumSizeHint();
    if (!wrapped && (neededBefore.height() > limitH || neededBefore.width() > limitW))
    {
        // What the content would have taken had the screen been large enough. Wrapping it makes the
        // dialog's own minimum collapse to a few rows, so without this it would open cramped.
        const QSize preferred(qMax(dialog->sizeHint().width(), dialog->width()),
                              qMax(dialog->sizeHint().height(), dialog->height()));

        if (QWidget *content = ContentToScroll(dialog); content != nullptr)
        {
            wrapped = WrapInScrollArea(content);
        }

        if (!wrapped)
        {
            // Either there was no single widget worth scrolling, or the one found could not be replaced
            // where it stood. The whole content can still be moved out of the frame.
            wrapped = WrapContent(dialog);
        }

        if (wrapped)
        {
            dialog->setProperty(kWrappedProperty, true);
            const int wantedW = qMin(preferred.width(), limitW);
            const int wantedH = qMin(preferred.height(), limitH);
            if (dialog->width() < wantedW || dialog->height() < wantedH)
            {
                dialog->resize(qMax(dialog->width(), wantedW), qMax(dialog->height(), wantedH));
            }
            qCInfo(vDialogFit) << "content made scrollable to fit" << available.width() << "×"
                               << available.height() << "px" << dialog->metaObject()->className();
        }
    }

    const QSize needed = dialog->minimumSizeHint();
    const int capH = qMax(limitH, needed.height());
    const int capW = qMax(limitW, needed.width());

    // A dialog may have locked its own size (a "new pattern" form does): that lock is a decision, not
    // a leftover, and only a cap that would let it run past the screen has the right to touch it.
    CapTo(dialog, kCapProperty, &QWidget::maximumHeight, &QWidget::setMaximumHeight, capH);
    CapTo(dialog, kCapWidthProperty, &QWidget::maximumWidth, &QWidget::setMaximumWidth, capW);

    if (dialog->height() > capH || dialog->width() > capW)
    {
        // Never below what the layout needs: a dialog that has not been sized yet reports zero, and
        // "smaller than its own minimum" is not a size, it is a crash waiting for a layout pass.
        const QSize floor = dialog->minimumSizeHint();
        dialog->resize(qMax(floor.width(), qMin(dialog->width(), capW)),
                       qMax(floor.height(), qMin(dialog->height(), capH)));
    }

    const Verdict verdict = (needed.height() > limitH + kFitSlack || needed.width() > limitW + kFitSlack)
                                ? Verdict::Overflows
                                : (wrapped ? Verdict::Scrolls : Verdict::Fits);

    Inventory().insert(DialogKey(dialog), FitRecord{needed, QSize(limitW, limitH), verdict});

    if (verdict == Verdict::Overflows)
    {
        // Nothing left to give: the content cannot be made smaller than this — a fixed-size
        // widget, a layout whose minimum is the screen and then some. Reported, never broken.
        qCInfo(vDialogFit) << "cannot fit" << needed.width() << "×" << needed.height() << "px of content into"
                           << limitW << "×" << limitH << "px" << dialog->metaObject()->className();
    }

    return verdict;
}

//---------------------------------------------------------------------------------------------------------------------
bool ODialogFit::eventFilter(QObject *watched, QEvent *event)
{
    auto *widget = qobject_cast<QWidget *>(watched);
    if (widget == nullptr || !widget->isWindow() || qobject_cast<QDialog *>(widget) == nullptr)
    {
        return QObject::eventFilter(watched, event);
    }

    switch (event->type())
    {
        case QEvent::Show:
            Apply(widget, AvailableFor(widget));
            // A dialog that opens filled from a file, a pattern or a table has not built its last
            // row when it is shown; the fit has to be checked once the event loop has run them.
            ScheduleRefit(widget);
            break;
        case QEvent::LayoutRequest:
        case QEvent::ScreenChangeInternal:
            if (widget->property(kCapProperty).isValid())
            {
                ScheduleRefit(widget);
            }
            break;
        default:
            break;
    }

    return QObject::eventFilter(watched, event);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogFit::InventoryReport() -> QStringList
{
    const QHash<QString, FitRecord> &inventory = Inventory();
    QStringList names = inventory.keys();
    names.sort();

    QStringList report;
    report << QStringLiteral("inventory — %1 dialogues seen").arg(names.size());
    for (const QString &name : names)
    {
        const FitRecord &record = inventory.value(name);
        report << QStringLiteral("  %1: %2×%3 px asked, %4×%5 px available, %6")
                      .arg(name)
                      .arg(record.needed.width())
                      .arg(record.needed.height())
                      .arg(record.room.width())
                      .arg(record.room.height())
                      .arg(VerdictName(record.verdict));
    }
    return report;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFit::KeepOnScreen(QWidget *dialog)
{
    if (dialog == nullptr)
    {
        return;
    }

    const QScreen *current = dialog->screen();
    if (current == nullptr)
    {
        return;
    }

    const QRect available = current->availableGeometry();
    const int margin = 20;

    if (dialog->height() > available.height() - 2 * margin)
    {
        dialog->resize(dialog->width(), available.height() - 2 * margin);
    }

    const QRect frame = dialog->frameGeometry();
    QRect target = frame;
    if (target.left() < available.left())
    {
        target.moveLeft(available.left());
    }
    if (target.top() < available.top())
    {
        target.moveTop(available.top());
    }
    if (target.right() > available.right())
    {
        target.moveRight(available.right());
    }
    if (target.bottom() > available.bottom())
    {
        target.moveBottom(available.bottom());
    }

    if (target.topLeft() == frame.topLeft())
    {
        return; // it is already whole, and where its reader put it
    }

    // move() places the client, frameGeometry() speaks of the frame: the offset is what the window
    // manager added around us, and taking it out is what puts the frame where it was computed.
    const QPoint clientOffset = dialog->pos() - frame.topLeft();
    dialog->move(target.topLeft() + clientOffset);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFit::CenterOnScreen(QWidget *dialog)
{
    if (dialog == nullptr)
    {
        return;
    }

    const QScreen *current = dialog->screen();
    if (current == nullptr)
    {
        return;
    }

    const QRect available = current->availableGeometry();
    const int margin = 20;

    // First, it must be able to fit: a frame taller than the area cannot be centred whole, so the frame is
    // brought back inside before anything is placed.
    const int maxWidth = qMax(1, available.width() - 2 * margin);
    const int maxHeight = qMax(1, available.height() - 2 * margin);
    if (dialog->width() > maxWidth || dialog->height() > maxHeight)
    {
        dialog->resize(qMin(dialog->width(), maxWidth), qMin(dialog->height(), maxHeight));
    }

    const QRect frame = dialog->frameGeometry();
    QPoint target = available.center() - QPoint(frame.width() / 2, frame.height() / 2);

    // Centred, but whole: what would hang under the bottom is raised, and what would run past the top is
    // pinned to the margin. A window taller than the area keeps its top visible — its bottom is what the
    // content can scroll to.
    target.setX(qBound(available.left() + margin, target.x(), available.right() - frame.width() - margin + 1));
    target.setY(qBound(available.top() + margin, target.y(), available.bottom() - frame.height() - margin + 1));

    const QPoint clientOffset = dialog->pos() - frame.topLeft();
    dialog->move(target + clientOffset);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFit::ScheduleRefit(QWidget *dialog)
{
    if (dialog->property(kPendingProperty).toBool())
    {
        return;
    }

    dialog->setProperty(kPendingProperty, true);

    const QPointer<QWidget> guard(dialog);
    QTimer::singleShot(0, dialog,
                       [guard]()
                       {
                           if (guard == nullptr)
                           {
                               return;
                           }

                           guard->setProperty(kPendingProperty, false);
                           Apply(guard, AvailableFor(guard));
                       });
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogFit::ContentToScroll(QWidget *dialog) -> QWidget *
{
    QLayout *layout = dialog->layout();
    if (layout == nullptr)
    {
        return nullptr;
    }

    // A tab widget gives every page the height of the tallest one, so it is the one widget whose
    // own scrolling gives the frame back to the screen. The bar of buttons, when it lives outside
    // the tab widget, stays where it is — which is the whole point.
    QWidget *deepest = nullptr;
    int deepestArea = 0;

    for (int i = 0; i < layout->count(); ++i)
    {
        QWidget *item = layout->itemAt(i)->widget();
        if (item == nullptr)
        {
            continue;
        }

        if (auto *tabs = qobject_cast<QTabWidget *>(item))
        {
            return tabs;
        }

        if (auto *tabs = item->findChild<QTabWidget *>())
        {
            const QSize hint = tabs->sizeHint();
            const int area = hint.width() * hint.height();
            if (area > deepestArea)
            {
                deepestArea = area;
                deepest = tabs;
            }
        }
    }

    return deepest;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogFit::WrapInScrollArea(QWidget *content) -> bool
{
    QWidget *host = content->parentWidget();
    if (host == nullptr)
    {
        return false;
    }

    QLayout *layout = host->layout();
    if (layout == nullptr)
    {
        return false;
    }

    QScrollArea *scroll = MakeScrollArea(host);
    QLayoutItem *replaced = layout->replaceWidget(content, scroll);
    if (replaced == nullptr)
    {
        delete scroll;
        return false;
    }
    delete replaced;

    scroll->setWidget(content);
    content->show();
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogFit::WrapContent(QWidget *dialog) -> bool
{
    auto *box = qobject_cast<QBoxLayout *>(dialog->layout());
    if (box == nullptr || box->direction() != QBoxLayout::TopToBottom)
    {
        // A grid, or a bar of columns: moving its items elsewhere would lose the very positions
        // the layout was written for. Such a dialog is reported by the audit rather than broken.
        return false;
    }

    auto *content = new QWidget(dialog);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);

    const int itemCount = box->count();
    QVector<QLayoutItem *> pinned;
    int index = 0;
    while (box->count() > 0)
    {
        QLayoutItem *item = box->takeAt(0);
        const bool last = ++index == itemCount;
        if (item == nullptr)
        {
            continue;
        }

        // A bar of buttons, or the last row when it holds buttons and nothing to fill in: it stays
        // in the frame, where a click can still reach it.
        if (ContainsButtonBar(item) || (last && HoldsButtons(item) && !HoldsInput(item)))
        {
            pinned.append(item);
        }
        else if (QWidget *widget = item->widget())
        {
            contentLayout->addWidget(widget);
            delete item;
        }
        else
        {
            contentLayout->addItem(item);
        }
    }

    QScrollArea *scroll = MakeScrollArea(dialog);
    scroll->setWidget(content);
    box->addWidget(scroll, 1);

    for (QLayoutItem *item : pinned)
    {
        box->addItem(item);
    }

    content->show();
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFit::CompactFormRows(QWidget *dialog)
{
    const QList<QFormLayout *> forms = dialog->findChildren<QFormLayout *>();
    for (QFormLayout *form : forms)
    {
        if (form == nullptr)
        {
            continue;
        }

        for (int row = 0; row < form->rowCount(); ++row)
        {
            // Only the rows that span the whole width: a labelled row is a field, and a field is
            // as tall as it needs to be. A full-width row is a block, and a block that grows is
            // the dead stretch the dialog cannot afford.
            QLayoutItem *item = form->itemAt(row, QFormLayout::SpanningRole);
            if (item == nullptr)
            {
                continue;
            }

            QWidget *block = item->widget();
            if (block == nullptr || block->layout() == nullptr || KeepsItsGrowth(block))
            {
                continue;
            }

            if (!(block->layout()->expandingDirections() & Qt::Vertical))
            {
                continue;
            }

            if (block->sizePolicy().verticalPolicy() == QSizePolicy::Maximum)
            {
                continue;
            }

            block->setSizePolicy(block->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
            qCDebug(vDialogFit) << "full-width row no longer grows:" << block;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogFit::KeepsItsGrowth(const QWidget *widget) -> bool
{
    // Content that is meant to fill what it is given. Capping these would not remove empty space,
    // it would remove the content.
    const auto fills = [](const QWidget *candidate) -> bool
    {
        return qobject_cast<const QAbstractScrollArea *>(candidate) != nullptr
               || qobject_cast<const QTextEdit *>(candidate) != nullptr
               || qobject_cast<const QTabWidget *>(candidate) != nullptr
               || qobject_cast<const QSplitter *>(candidate) != nullptr
               || qobject_cast<const QGraphicsView *>(candidate) != nullptr
               || qobject_cast<const QDialogButtonBox *>(candidate) != nullptr;
    };

    if (fills(widget))
    {
        return true;
    }

    // A group box is a plain container: capping it from its own size policy would squeeze whatever
    // table, list or text area it holds, and a table that cannot scroll is worse than a gap. If the
    // content that needs the room is inside, the box keeps the room.
    for (const QWidget *child : widget->findChildren<QWidget *>())
    {
        if (fills(child))
        {
            return true;
        }
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogFit::FrameChrome(const QWidget *dialog) -> QMargins
{
    if (dialog->windowHandle() != nullptr)
    {
        const QMargins margins = dialog->windowHandle()->frameMargins();
        if (margins.top() + margins.bottom() > 0)
        {
            return margins;
        }
    }

    // Before the window exists the window system has nothing to say: the style still knows what a
    // title bar costs, and overestimating it keeps the bar of buttons inside the screen.
    const QStyle *style = dialog->style();
    const int border = style->pixelMetric(QStyle::PM_DefaultFrameWidth);
    return QMargins(border, style->pixelMetric(QStyle::PM_TitleBarHeight) + border, border, border);
}
