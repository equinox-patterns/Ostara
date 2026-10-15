/************************************************************************
 **
 **  @file   oexportbatchdialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Implementation de l'export en lot.
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

#include "oexportbatchdialog.h"

#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

#include "oexportcuttingfileoptions.h"

namespace
{

/// L'ordre des onglets, tel qu'il est posé et tel qu'il se relit — trois constantes plutôt que
/// trois nombres, parce que la catégorie se lit de l'index courant.
constexpr int kCuttingTab = 0;
constexpr int kPrintTab = 1;
constexpr int kPlotterTab = 2;

constexpr auto kPdfSuffix = ".pdf";
constexpr auto kSvgSuffix = ".svg";

} // namespace

//---------------------------------------------------------------------------------------------------------------------
OExportBatchDialog::OExportBatchDialog(const QVector<OExportBatchPage::Row> &cuttingRows,
                                       const QVector<OExportBatchPage::Row> &printRows,
                                       const QVector<OExportBatchPage::Row> &plotterRows,
                                       const QString &directory, QWidget *parent)
  : QDialog(parent)
{
    setModal(true);
    setMinimumWidth(560);

    auto *layout = new QVBoxLayout(this);

    m_tabs = new QTabWidget(this);
    // Un onglet qui manque de place raccourcit ses titres ou cache les suivants derrière des
    // flèches : ni l'un ni l'autre ne se lit. Trois titres courts, et l'élision reste au repos.
    m_tabs->setElideMode(Qt::ElideNone);
    m_tabs->setUsesScrollButtons(false);

    // ── Le plan de coupe : son réglage, puis ses générations ──
    //
    // Le réglage est AU-DESSUS des lignes, et il vaut pour toutes : un lot est un format — comme la
    // case « mise en page » d'un lot de PDF vaut pour tous ses PDF. Qui veut deux jeux d'options
    // fait deux lots.
    auto *cuttingTab = new QWidget(this);
    auto *cuttingLayout = new QVBoxLayout(cuttingTab);

    m_cuttingOptions = new OExportCuttingFileOptions(cuttingTab);
    cuttingLayout->addWidget(m_cuttingOptions);

    m_cuttingPage = new OExportBatchPage(cuttingRows, m_cuttingOptions->Suffix(), directory, false, cuttingTab);
    cuttingLayout->addWidget(m_cuttingPage);
    cuttingLayout->addStretch();

    // L'extension des lignes suit le format choisi : c'est la seule chose que le réglage impose aux
    // noms, et c'est ce qui empêche un `.dxf` d'annoncer un `.plt`.
    connect(m_cuttingOptions, &OExportCuttingFileOptions::FormatChanged, this,
            [this] { m_cuttingPage->SetSuffix(m_cuttingOptions->Suffix()); });

    // ── Print et Plotter : une page chacun, et leur propre format ──
    m_printPage = new OExportBatchPage(printRows, QLatin1String(kPdfSuffix), directory, true, this);
    m_plotterPage = new OExportBatchPage(plotterRows, QLatin1String(kSvgSuffix), directory, false, this);

    m_tabs->addTab(cuttingTab, QString());
    m_tabs->addTab(m_printPage, QString());
    m_tabs->addTab(m_plotterPage, QString());

    // On ouvre sur une page qui a quelque chose à montrer : Print d'abord, Plotter quand il n'y a que
    // du plotter, le plan de coupe quand il est le seul à en avoir — une page vide s'ouvrirait sur
    // une question à laquelle il n'y a pas de réponse.
    int opening = kPrintTab;
    if (printRows.isEmpty() && !plotterRows.isEmpty())
    {
        opening = kPlotterTab;
    }
    else if (printRows.isEmpty() && plotterRows.isEmpty() && !cuttingRows.isEmpty())
    {
        opening = kCuttingTab;
    }
    m_tabs->setCurrentIndex(opening);
    connect(m_tabs, &QTabWidget::currentChanged, this, &OExportBatchDialog::OnTabChanged);

    layout->addWidget(m_tabs);

    m_buttonBox = new QDialogButtonBox(this);
    m_exportButton = m_buttonBox->addButton(QString(), QDialogButtonBox::AcceptRole);
    m_exportButton->setAutoDefault(false);
    m_buttonBox->addButton(QDialogButtonBox::Cancel);
    // Affirmé en dernier, une fois la barre complète : la boîte répartit son bouton par défaut à
    // mesure que les boutons arrivent, et Entrée fermerait le dialogue au lieu d'exporter.
    m_exportButton->setDefault(true);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &OExportBatchDialog::OnAccept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    layout->addWidget(m_buttonBox);

    Retranslate();
    UpdateExportButton();

    // Sur le premier nom de la page ouverte : c'est le seul champ qui soit là pour être touché, le
    // dossier venant d'être proposé et les cases d'être cochées.
    if (OExportBatchPage *page = CurrentPage(); page != nullptr)
    {
        page->FocusFirstName();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportBatchDialog::CurrentPage() const -> OExportBatchPage *
{
    switch (m_tabs->currentIndex())
    {
        case kCuttingTab:
            return m_cuttingPage;
        case kPrintTab:
            return m_printPage;
        case kPlotterTab:
            return m_plotterPage;
        default:
            return nullptr;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportBatchDialog::Category() const -> OutputCategory
{
    switch (m_tabs->currentIndex())
    {
        case kPrintTab:
            return OutputCategory::Print;
        case kPlotterTab:
            return OutputCategory::Plotter;
        default:
            return OutputCategory::Cutting;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportBatchDialog::Directory() const -> QString
{
    const OExportBatchPage *page = CurrentPage();
    return page != nullptr ? page->Directory() : QString();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportBatchDialog::Chosen() const -> QVector<OExportBatchPage::Row>
{
    const OExportBatchPage *page = CurrentPage();
    return page != nullptr ? page->Chosen() : QVector<OExportBatchPage::Row>();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportBatchDialog::AddLayoutLayer() const -> bool
{
    const OExportBatchPage *page = CurrentPage();
    return page != nullptr && page->AddLayoutLayer();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportBatchDialog::CuttingOptions() const -> OExportCuttingFile::Options
{
    return m_cuttingOptions != nullptr ? m_cuttingOptions->Options() : OExportCuttingFile::Options();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportBatchDialog::OnTabChanged(int index)
{
    Q_UNUSED(index)
    UpdateExportButton();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportBatchDialog::UpdateExportButton()
{
    if (m_exportButton == nullptr)
    {
        return;
    }

    const OExportBatchPage *page = CurrentPage();
    const bool canExport = page != nullptr && page->HasRows();

    m_exportButton->setEnabled(canExport);
    m_exportButton->setToolTip(canExport ? tr("Export the ticked generations")
                                         : tr("There is no output to export here."));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportBatchDialog::OnAccept()
{
    OExportBatchPage *page = CurrentPage();
    if (page == nullptr)
    {
        // Un index d'onglet hors liste — jamais courant, mais la question « quel format ? » doit
        // avoir une réponse même là.
        return;
    }

    // Le dossier d'abord : c'est la seule erreur qui ne se voit pas dans la liste, et la corriger
    // demande d'aller au champ.
    const QFileInfo folder(page->Directory());
    if (!folder.isDir())
    {
        QMessageBox::warning(this, tr("Export"), tr("That folder does not exist."));
        page->FocusDirectory();
        return;
    }

    const QVector<OExportBatchPage::Row> chosen = page->Chosen();
    if (chosen.isEmpty())
    {
        QMessageBox::warning(this, tr("Export"), tr("No generation is ticked."));
        return;
    }

    for (const OExportBatchPage::Row &row : chosen)
    {
        if (row.name.isEmpty())
        {
            QMessageBox::warning(this, tr("Export"), tr("Every ticked generation needs a name."));
            return;
        }
    }

    accept();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportBatchDialog::Retranslate()
{
    setWindowTitle(tr("Export several outputs"));

    m_tabs->setTabText(kCuttingTab, tr("Cutting plan"));
    m_tabs->setTabText(kPrintTab, tr("Print"));
    m_tabs->setTabText(kPlotterTab, tr("Plotter"));

    const QList<QAbstractButton *> buttons = m_buttonBox->buttons();
    for (QAbstractButton *button : buttons)
    {
        switch (m_buttonBox->buttonRole(button))
        {
            case QDialogButtonBox::AcceptRole:
                button->setText(tr("Export"));
                break;
            case QDialogButtonBox::RejectRole:
                button->setText(tr("Cancel"));
                break;
            default:
                break;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportBatchDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
        UpdateExportButton();
    }

    QDialog::changeEvent(event);
}
