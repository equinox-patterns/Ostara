/************************************************************************
 **
 **  @file   oexportmodedialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/09/2026
 **
 **  @brief  Implementation of the Export mode entry dialog.
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

#include "oexportmodedialog.h"

#include "oexportgenerationlistwidget.h"
#include "oexportprintwidget.h"

#include "../core/oexportnamebuilder.h"

#include <QAbstractButton>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>

#include <utility>

namespace
{
/// The tab that holds the runs described but not fired.
constexpr int kListTabIndex = 0;

/// Where the tab that can be acted on sits. The dialog opens on it rather than on the
/// first tab: two of the four have nothing to offer yet, and landing on one of those
/// would show an empty page every time the mode is entered.
constexpr int kPrintTabIndex = 2;

/// Below this the Print tab's form is cramped, whatever the tab bar needs.
constexpr int kMinDialogWidth = 560;

/// What the tab widget's frame and margins add around the tab bar itself.
constexpr int kTabBarChrome = 40;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OExportModeDialog::OExportModeDialog(const VContainer *data, const QSharedPointer<VMeasurements> &measurements,
                                     const OGradedSizeRanges::Context &context, const QStringList &takenNames,
                                     QWidget *parent)
  : QDialog(parent),
    m_data(data),
    m_measurements(measurements),
    m_context(context),
    m_takenNames(takenNames)
{
    setWindowTitle(tr("Export"));
    setModal(true);

    auto *layout = new QVBoxLayout(this);

    m_tabs = new QTabWidget(this);

    m_listWidget = new OExportGenerationListWidget(m_tabs);
    connect(m_listWidget, &OExportGenerationListWidget::RemoveRequested, this,
            [this](int index)
            {
                // The queue first, then the row: the row is a view of it, and a view that
                // ran ahead would be showing a line the batch could still walk into.
                m_queue.RemoveAt(index);
                m_listWidget->RemoveRow(index);
                RefreshGenerateButtons();
            });

    m_cuttingTab = MakeComingSoonTab(m_tabs);

    m_printTab = new QWidget(m_tabs);
    m_printLayout = new QVBoxLayout(m_printTab);
    // The stretch is what keeps the form at the top of the page; ResetPrintTab inserts the
    // form at index 0, before it.
    m_printLayout->addStretch();

    m_projectorTab = MakeComingSoonTab(m_tabs);

    // A tab bar that runs out of room shortens its labels, or hides the surplus behind
    // scroll arrows. Neither is readable, so the dialog is widened to the labels instead
    // — see AdjustMinimumWidth, which Retranslate calls once the titles are known.
    m_tabs->setElideMode(Qt::ElideNone);
    m_tabs->setUsesScrollButtons(false);

    m_tabs->addTab(m_listWidget, QString());
    m_tabs->addTab(m_cuttingTab, QString());
    m_tabs->addTab(m_printTab, QString());
    m_tabs->addTab(m_projectorTab, QString());
    m_tabs->setCurrentIndex(kPrintTabIndex);
    layout->addWidget(m_tabs);

    // The bar sits on the right with the last thing said on its left: what was added is
    // reported where the user is already looking when she clicks, not in a status line
    // somewhere else in the window.
    auto *barRow = new QHBoxLayout();
    m_addedLabel = new QLabel(this);
    m_addedLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_addedLabel->setObjectName(QStringLiteral("exportAddedLabel"));
    barRow->addWidget(m_addedLabel, 1);

    m_buttonBox = new QDialogButtonBox(this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    barRow->addWidget(m_buttonBox, 0);

    layout->addLayout(barRow);

    connect(m_tabs, &QTabWidget::currentChanged, this, [this](int) { OnTabChanged(); });

    // After the page it goes on, and before Retranslate, which builds the bar from the tab
    // in force — the Print tab, whose buttons read the form this creates.
    ResetPrintTab();

    Retranslate();
    RefreshGenerateButtons();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::MakeComingSoonTab(QWidget *parent) -> QWidget *
{
    auto *page = new QWidget(parent);
    auto *layout = new QVBoxLayout(page);

    auto *label = new QLabel(page);
    label->setAlignment(Qt::AlignCenter);
    label->setObjectName(QStringLiteral("comingSoonLabel"));
    label->setText(OExportModeDialog::tr("Coming soon"));
    layout->addWidget(label);

    return page;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::ResetPrintTab()
{
    delete m_printWidget;

    m_printWidget = new OExportPrintWidget(m_data, m_measurements, m_context, m_printTab);
    m_printLayout->insertWidget(0, m_printWidget);

    connect(m_printWidget, &OExportPrintWidget::Changed, this, &OExportModeDialog::OnPrintChanged);

    RefreshGenerateButtons();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::CurrentRequest() const -> OExportRunRequest
{
    OExportRunRequest request;
    request.documentWidthMm = m_printWidget->DocumentWidthMm();
    request.unit = m_printWidget->DisplayUnit();
    request.rangeName = m_printWidget->SizeRangeName();
    request.singleSize = m_printWidget->IsSingleSize();
    request.baseA = m_printWidget->DimensionBase(0);
    request.baseB = m_printWidget->DimensionBase(1);
    request.baseC = m_printWidget->DimensionBase(2);
    request.marginMm = m_printWidget->MarginMm();
    request.gapMm = m_printWidget->GapMm();
    request.printModelToken = m_printWidget->PrintModelToken();
    request.columns = m_printWidget->Columns();
    return request;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::CurrentName() const -> QString
{
    const OExportRunRequest request = CurrentRequest();

    // A single size is named by the axes it sits on, read as the user reads them — the
    // label the combo shows ("160", "32"), never the base underneath. A range is named by
    // the range. An axis the table does not carry contributes nothing and is skipped.
    QStringList bases;
    if (request.singleSize)
    {
        for (int slot = 0; slot < 3; ++slot)
        {
            bases.append(m_printWidget->DimensionLabel(slot));
        }
    }

    const QString base = OExportNameBuilder::Base(request.singleSize ? QString() : request.rangeName,
                                                  bases, request.printModelToken);

    // The lines of the list count as taken just as much as the generations the dock lists:
    // a line is named when it is added, so two identical runs added one after the other
    // would otherwise be born carrying the same name.
    QStringList taken = m_takenNames;
    taken += m_queue.Names();

    return OExportNameBuilder::Unique(base, taken);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::SetRunGenerator(RunGenerator generator)
{
    m_generator = std::move(generator);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::HasGenerated() const -> bool
{
    return m_generated;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::OnAddToList()
{
    if (m_batchRunning || not m_printWidget->IsValid())
    {
        return;
    }

    // Named before it is filed, and filed under that name: the list shows it, and what the
    // list shows is what the run will be called.
    const QString name = CurrentName();

    OExportQueueItem item;
    item.name = name;
    item.request = CurrentRequest();

    m_queue.Append(std::move(item));
    m_listWidget->AppendRow(name);
    RefreshGenerateButtons();

    // The form is spent: what it described is in the list now, and the next run starts
    // where this one started. Reset before the message, never after — rebuilding the form
    // raises Changed, and Changed is what clears the message.
    ResetPrintTab();
    ShowAddedMessage(name);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::OnGenerateAll()
{
    if (m_batchRunning || not m_generator || m_queue.IsEmpty())
    {
        return;
    }

    SetBatchRunning(true);

    for (int index = 0; index < m_queue.Count(); ++index)
    {
        // Read out before the call, never held across it: the line belongs to the queue,
        // and the generator below runs an event loop of its own while the engine works.
        const OExportQueueItem item = m_queue.At(index);

        m_queue.SetState(index, OExportRunState::Running);
        m_listWidget->SetState(index, OExportRunState::Running);
        // The list is what the user watches while this takes its time. Without a repaint
        // here nothing of the batch would be drawn until the whole of it was over.
        QCoreApplication::processEvents();

        const bool done = m_generator(item.request, item.name);

        // A line the engine refused does not stop the walk: the other runs were asked for
        // too, and the one that failed says so on its own line.
        m_queue.SetState(index, done ? OExportRunState::Done : OExportRunState::Failed);
        m_listWidget->SetState(index, done ? OExportRunState::Done : OExportRunState::Failed);
        m_generated = m_generated || done;
    }

    SetBatchRunning(false);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::ShowAddedMessage(const QString &name)
{
    m_addedName = name;
    m_addedLabel->setText(tr("%1 was added to the list.").arg(m_addedName));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::OnPrintChanged()
{
    // The message reports one add. The first field touched after it makes it stale — the
    // user is describing something else now.
    m_addedName.clear();
    m_addedLabel->clear();

    RefreshGenerateButtons();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::OnTabChanged()
{
    // The confirmation belongs to the page it was made on.
    m_addedName.clear();
    m_addedLabel->clear();

    RebuildButtonBox();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::RebuildButtonBox()
{
    for (QAbstractButton *button : m_buttonBox->buttons())
    {
        // removeButton hands the button back unparented: deleting it is ours to do, and not
        // doing it would stack one tab's buttons on the next one's.
        m_buttonBox->removeButton(button);
        delete button;
    }

    m_generateButton = nullptr;
    m_addToListButton = nullptr;
    m_generateAllButton = nullptr;

    switch (m_tabs->currentIndex())
    {
        case kPrintTabIndex:
            m_addToListButton = m_buttonBox->addButton(tr("Add to list"), QDialogButtonBox::ActionRole);
            m_addToListButton->setAutoDefault(false);
            connect(m_addToListButton, &QPushButton::clicked, this, &OExportModeDialog::OnAddToList);

            m_generateButton = m_buttonBox->addButton(tr("Generate"), QDialogButtonBox::AcceptRole);

            m_buttonBox->addButton(QDialogButtonBox::Cancel);
            break;

        case kListTabIndex:
            m_generateAllButton = m_buttonBox->addButton(tr("Generate the whole list"),
                                                         QDialogButtonBox::ActionRole);
            m_generateAllButton->setAutoDefault(false);
            connect(m_generateAllButton, &QPushButton::clicked, this, &OExportModeDialog::OnGenerateAll);

            m_buttonBox->addButton(tr("Close"), QDialogButtonBox::RejectRole);
            break;

        default:
            // Nothing on these pages can be acted on: the way out is the only honest thing
            // to offer.
            m_buttonBox->addButton(tr("Close"), QDialogButtonBox::RejectRole);
            break;
    }

    // Asserted last, once every button is in: the box lays its buttons out as they arrive
    // and can hand the default to the one added after Generate — Return would then close
    // the dialog instead of nesting what the tab describes.
    if (m_generateButton != nullptr)
    {
        m_generateButton->setDefault(true);
    }

    m_buttonBox->setEnabled(!m_batchRunning);
    RefreshGenerateButtons();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::SetBatchRunning(bool running)
{
    m_batchRunning = running;

    // The list is locked but kept legible, not greyed: it is what the batch writes to, and
    // its states are what the user is watching. Everything else can go grey — while a run
    // is in the engine, nothing on the Print tab can be acted on.
    m_listWidget->SetBusy(running);
    m_printWidget->setEnabled(!running);
    // The bar as well: mid-batch there is nothing to confirm, nothing to add and nowhere to
    // go — and reject() refuses the close anyway, so an enabled button would be a button
    // that lies.
    m_buttonBox->setEnabled(!running);

    RefreshGenerateButtons();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::RefreshGenerateButtons()
{
    const bool valid = m_printWidget != nullptr && m_printWidget->IsValid() && not m_batchRunning;

    // Each is only alive on its own tab: the bar carries one tab's buttons at a time.
    if (m_generateButton != nullptr)
    {
        m_generateButton->setEnabled(valid);
    }
    if (m_addToListButton != nullptr)
    {
        m_addToListButton->setEnabled(valid);
    }
    if (m_generateAllButton != nullptr)
    {
        m_generateAllButton->setEnabled(not m_queue.IsEmpty() && not m_batchRunning);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::reject()
{
    if (m_batchRunning)
    {
        return;
    }

    QDialog::reject();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::Retranslate()
{
    setWindowTitle(tr("Export"));

    m_tabs->setTabText(kListTabIndex, tr("List to generate"));
    m_tabs->setTabText(1, tr("Cutting plan"));
    m_tabs->setTabText(kPrintTabIndex, tr("Print"));
    m_tabs->setTabText(3, tr("Projector"));

    // Read back off the pages rather than kept beside them: a second copy of the two
    // labels would be one more pair to hold together, and each label is right there.
    // tr() rather than QObject::tr(): the placeholder belongs to this dialog, and a string
    // filed under QObject lands in a context every widget in the application shares.
    for (QWidget *page : {m_cuttingTab, m_projectorTab})
    {
        if (auto *label = page->findChild<QLabel *>(QStringLiteral("comingSoonLabel")))
        {
            label->setText(tr("Coming soon"));
        }
    }

    m_addedLabel->setText(m_addedName.isEmpty() ? QString() : tr("%1 was added to the list.").arg(m_addedName));

    // The bar is rebuilt rather than rewritten: its buttons belong to the tab in force, and
    // the three that exist at any one time are not the same three in every language.
    RebuildButtonBox();

    // Last, because it reads the titles written just above — and because the tab bar only
    // knows how wide it wants to be once it carries them.
    AdjustMinimumWidth();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::AdjustMinimumWidth()
{
    const QTabBar *bar = m_tabs->tabBar();
    const int tabBarWidth = bar != nullptr ? bar->sizeHint().width() : 0;

    setMinimumWidth(qMax(kMinDialogWidth, tabBarWidth + kTabBarChrome));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QDialog::changeEvent(event);
}
