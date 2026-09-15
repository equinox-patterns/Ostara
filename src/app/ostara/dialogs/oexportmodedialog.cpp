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

#include "oexportprintwidget.h"

#include <QDialogButtonBox>
#include <QEvent>
#include <QLabel>
#include <QPushButton>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>

namespace
{
/// Where the tab that can be acted on sits. The dialog opens on it rather than on the
/// first tab: three of the four have nothing to offer yet, and landing on one of those
/// would show an empty page every time the mode is entered.
constexpr int kPrintTabIndex = 2;

/// Below this the Print tab's form is cramped, whatever the tab bar needs.
constexpr int kMinDialogWidth = 560;

/// What the tab widget's frame and margins add around the tab bar itself.
constexpr int kTabBarChrome = 40;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OExportModeDialog::OExportModeDialog(const VContainer *data, const QSharedPointer<VMeasurements> &measurements,
                                     const OGradedSizeRanges::Context &context, QWidget *parent)
  : QDialog(parent)
{
    setWindowTitle(tr("Export"));
    setModal(true);

    auto *layout = new QVBoxLayout(this);

    m_tabs = new QTabWidget(this);
    m_listTab = MakeComingSoonTab(m_tabs);
    m_cuttingTab = MakeComingSoonTab(m_tabs);

    m_printTab = new QWidget(m_tabs);
    auto *printLayout = new QVBoxLayout(m_printTab);
    m_printWidget = new OExportPrintWidget(data, measurements, context, m_printTab);
    printLayout->addWidget(m_printWidget);
    printLayout->addStretch();

    m_projectorTab = MakeComingSoonTab(m_tabs);

    // A tab bar that runs out of room shortens its labels, or hides the surplus behind
    // scroll arrows. Neither is readable, so the dialog is widened to the labels instead
    // — see AdjustMinimumWidth, which Retranslate calls once the titles are known.
    m_tabs->setElideMode(Qt::ElideNone);
    m_tabs->setUsesScrollButtons(false);

    m_tabs->addTab(m_listTab, QString());
    m_tabs->addTab(m_cuttingTab, QString());
    m_tabs->addTab(m_printTab, QString());
    m_tabs->addTab(m_projectorTab, QString());
    m_tabs->setCurrentIndex(kPrintTabIndex);
    layout->addWidget(m_tabs);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(m_buttonBox);

    // The run needs a width and something to nest. Nothing else on the tab can be missing
    // — every other field has a value the moment the model is picked.
    const auto refreshOk = [this]()
    { m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(m_printWidget->IsValid()); };
    connect(m_printWidget, &OExportPrintWidget::Changed, this, refreshOk);

    Retranslate();
    refreshOk();
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
auto OExportModeDialog::DocumentWidthMm() const -> qreal
{
    return m_printWidget->DocumentWidthMm();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::MarginMm() const -> qreal
{
    return m_printWidget->MarginMm();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::GapMm() const -> qreal
{
    return m_printWidget->GapMm();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::SizeRangeName() const -> QString
{
    return m_printWidget->SizeRangeName();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::IsSingleSize() const -> bool
{
    return m_printWidget->IsSingleSize();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::DimensionBase(int index) const -> qreal
{
    return m_printWidget->DimensionBase(index);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::DimensionLabel(int index) const -> QString
{
    return m_printWidget->DimensionLabel(index);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::PrintModelToken() const -> QString
{
    return m_printWidget->PrintModelToken();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::Columns() const -> int
{
    return m_printWidget->Columns();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::DisplayUnit() const -> Unit
{
    return m_printWidget->DisplayUnit();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::Retranslate()
{
    setWindowTitle(tr("Export"));

    m_tabs->setTabText(0, tr("List to generate"));
    m_tabs->setTabText(1, tr("Cutting plan"));
    m_tabs->setTabText(2, tr("Print"));
    m_tabs->setTabText(3, tr("Projector"));

    // Read back off the pages rather than kept beside them: a second copy of the three
    // labels would be one more pair to hold together, and each label is right there.
    // tr() rather than QObject::tr(): the placeholder belongs to this dialog, and a string
    // filed under QObject lands in a context every widget in the application shares.
    for (QWidget *page : {m_listTab, m_cuttingTab, m_projectorTab})
    {
        if (auto *label = page->findChild<QLabel *>(QStringLiteral("comingSoonLabel")))
        {
            label->setText(tr("Coming soon"));
        }
    }

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
