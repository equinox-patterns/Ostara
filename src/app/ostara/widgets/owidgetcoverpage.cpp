/************************************************************************
 **
 **  @file   owidgetcoverpage.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/09/2026
 **
 **  @brief  The Cover page dock: what a pattern contributes to its own
 **          cover page, gathered where the pattern is.
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

#include "owidgetcoverpage.h"

#include "core/ocoverseamallowances.h"
#include "core/olengthunits.h"
#include "core/oseamallowancevalues.h"

#include "FactoryWidgets.h"
#include "vabstractapplication.h"
#include "vabstractvalapplication.h"
#include "vcommonsettings.h"
#include "vmisc/def.h"
#include "xml/vabstractpattern.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>

namespace
{
/// How long a text being typed waits before it is written into the document.
constexpr int kCommitDelayMs = 600;

/**
 * @brief An empty row.
 *
 * **A frame, and not a bare widget.** A plain QWidget paints no background at all — the stylesheet
 * is ignored silently, and under a dock's scroll area it shows whatever is behind it. Every row
 * built at run time is a frame for that reason; see SubtiliteCode, rendering of backgrounds.
 */
auto MakeRow(QWidget *parent) -> QFrame *
{
    auto *row = new QFrame(parent);
    row->setFrameStyle(QFrame::NoFrame);
    return row;
}

/**
 * @brief A line saying where one group of settings ends and the next begins.
 *
 * One pixel of the border colour, **and not the full width**: a rule that reached both edges would
 * read as a box around what follows, and what it says is only that the next block is another
 * subject. The same line the template editor draws between its own groups.
 */
auto MakeSectionSeparator(QWidget *parent) -> QWidget *
{
    auto *host = MakeRow(parent);
    auto *layout = new QHBoxLayout(host);
    layout->setContentsMargins(0, 4, 0, 4);
    layout->setSpacing(0);

    auto *rule = new QFrame(host);
    rule->setObjectName(QStringLiteral("coverPageSectionRule"));
    rule->setFixedHeight(1);
    rule->setFrameShape(QFrame::NoFrame);

    // Eight parts of ten, the rest left empty on either side — a proportion rather than a width, so
    // the line follows the dock when it is widened or narrowed.
    layout->addStretch(1);
    layout->addWidget(rule, 8);
    layout->addStretch(1);
    return host;
}

/// @brief Removes and destroys everything a layout holds, rows built at run time included.
void ClearLayout(QLayout *layout)
{
    while (QLayoutItem *item = layout->takeAt(0))
    {
        if (QWidget *widget = item->widget())
        {
            // **Hidden before it is scheduled for deletion, and the order is the whole point.** A
            // deferred deletion is not performed while the events are merely pumped — Qt,
            // QObject::deleteLater(), and the export flow relies on it, which is why it flushes
            // DeferredDelete by hand before it starts. A row taken out of its layout and left
            // waiting is therefore still on screen, at the place it last held, while the row
            // replacing it is drawn in the same spot: the same text laid down twice, which reads
            // as one line in bold. Hiding it takes it off the screen at once, and the deletion
            // still happens whenever the loop that posted it comes round.
            widget->hide();
            widget->deleteLater();
        }
        delete item;
    }
}

/// @brief What a symbol is called in a list: what its legend would print, or failing that, its marks.
auto SymbolLabel(const OSymbolLibrary::Entry &entry, const QString &locale) -> QString
{
    QString text = entry.texts.value(locale);
    if (text.isEmpty() && !entry.texts.isEmpty())
    {
        text = entry.texts.constBegin().value();
    }
    if (!text.isEmpty())
    {
        return text;
    }

    QStringList names;
    names.reserve(entry.shapes.size());
    for (const OSymbolShapes::Shape &shape : entry.shapes)
    {
        names.append(OSymbolShapes::Name(shape));
    }
    return names.join(QStringLiteral(", "));
}

/// @brief The margin the application is set to, in millimetres — what the default entry answers.
auto DefaultSeamAllowanceMm() -> qreal
{
    auto *settings = VAbstractApplication::VApp()->Settings();
    if (settings == nullptr)
    {
        return 0.0;
    }
    return UnitConvertor(settings->GetDefaultSeamAllowance(), StrToUnits(settings->GetUnit()),
                         Unit::Mm);
}

/// @brief The unit the pattern is being worked in, which is the one a width is stated in.
auto WorkingUnit() -> Unit
{
    auto *settings = VAbstractApplication::VApp()->Settings();
    return settings == nullptr ? Unit::Cm : StrToUnits(settings->GetUnit());
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------

OWidgetCoverPage::OWidgetCoverPage(QWidget *parent) : QWidget(parent)
{
    BuildUi();

    // **A text is written once it stops being typed.** Every letter written into the document would
    // be a document change, and a page redrawn for each; the delay is the one the pattern's own
    // information page waits, so a text behaves the same wherever it is written.
    m_commitTimer = new QTimer(this);
    m_commitTimer->setSingleShot(true);
    m_commitTimer->setInterval(kCommitDelayMs);
    connect(m_commitTimer, &QTimer::timeout, this, [this]() {
        if (m_loading || m_doc == nullptr)
        {
            return;
        }
        m_page.freeText = m_freeTextEdit->toPlainText();
        WritePage();
    });
}

//---------------------------------------------------------------------------------------------------------------------

void OWidgetCoverPage::BuildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(6);

    // ── The name the pattern answers to ──
    m_nameLabel = new QLabel(this);
    root->addWidget(m_nameLabel);

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setObjectName(QStringLiteral("coverPagePatternName"));
    connect(m_nameEdit, &QLineEdit::editingFinished, this, [this]() {
        if (m_loading || m_doc == nullptr)
        {
            return;
        }
        // **Through the setter, never through the document directly.** The name is cached by the
        // pattern, and one written behind its back would leave the toolbar, the window title and the
        // cover page showing the old one until the file was opened again.
        m_doc->SetPatternName(m_nameEdit->text().trimmed());

        // The pattern's name is an information of the page — so a page on screen has just gone
        // stale, and it is the second and last write this dock makes. See CoverPageChanged.
        emit CoverPageChanged();
    });
    root->addWidget(m_nameEdit);

    root->addWidget(MakeSectionSeparator(this));

    // ── The symbols this pattern chose ──
    m_symbolsLabel = new QLabel(this);
    root->addWidget(m_symbolsLabel);

    m_symbolsHint = new QLabel(this);
    m_symbolsHint->setObjectName(QStringLiteral("coverPageSymbolsHint"));
    m_symbolsHint->setWordWrap(true);
    root->addWidget(m_symbolsHint);

    auto *symbolsHost = MakeRow(this);
    m_symbolRows = new QVBoxLayout(symbolsHost);
    m_symbolRows->setContentsMargins(0, 0, 0, 0);
    m_symbolRows->setSpacing(4);
    root->addWidget(symbolsHost);

    m_addSymbolButton = new QPushButton(this);
    m_addSymbolButton->setObjectName(QStringLiteral("coverPageAddSymbol"));
    connect(m_addSymbolButton, &QPushButton::clicked, this, [this]() {
        if (m_doc == nullptr)
        {
            return;
        }
        // **The button adds a row, and the row holds the choice.** A symbol starts unchosen, and
        // stays out of the file until one is picked: a row nobody answered says nothing, and a
        // pattern should not carry a question that was never answered.
        m_page.symbols.append(QString());
        RebuildSymbolRows();
    });
    root->addWidget(m_addSymbolButton, 0, Qt::AlignLeft);

    root->addWidget(MakeSectionSeparator(this));

    // ── The text a template left to be filled in ──
    m_freeTextLabel = new QLabel(this);
    root->addWidget(m_freeTextLabel);

    m_freeTextEdit = new QPlainTextEdit(this);
    m_freeTextEdit->setObjectName(QStringLiteral("coverPageFreeText"));
    // **Two lines, fixed, and it scrolls rather than growing.** A pattern's note is a sentence or
    // two, and a field that stretched with what was written would push the margins block out of the
    // dock every time it was used.
    //
    // **Fixed, and not a minimum.** A minimum changes nothing here: a text box carries a size hint
    // several lines tall, and in a layout that hint is what the widget is given — the box stayed as
    // tall as it liked whatever floor was put under it.
    m_freeTextEdit->setFixedHeight(m_freeTextEdit->fontMetrics().lineSpacing() * 2
                                   + m_freeTextEdit->frameWidth() * 2
                                   + static_cast<int>(m_freeTextEdit->document()->documentMargin()) * 2);
    connect(m_freeTextEdit, &QPlainTextEdit::textChanged, this, [this]() {
        if (m_loading || m_doc == nullptr)
        {
            return;
        }
        m_commitTimer->start();
    });
    root->addWidget(m_freeTextEdit);

    root->addWidget(MakeSectionSeparator(this));

    // ── The names this pattern gives its own margins ──
    m_marginsLabel = new QLabel(this);
    root->addWidget(m_marginsLabel);

    m_marginsHint = new QLabel(this);
    m_marginsHint->setObjectName(QStringLiteral("coverPageMarginsHint"));
    m_marginsHint->setWordWrap(true);
    root->addWidget(m_marginsHint);

    auto *marginsHost = MakeRow(this);
    m_marginRows = new QVBoxLayout(marginsHost);
    m_marginRows->setContentsMargins(0, 0, 0, 0);
    m_marginRows->setSpacing(4);
    root->addWidget(marginsHost);

    root->addStretch(1);
    RetranslateUI();
}

//---------------------------------------------------------------------------------------------------------------------

void OWidgetCoverPage::RetranslateUI()
{
    m_nameLabel->setText(tr("Pattern name"));
    m_symbolsLabel->setText(tr("Symbols"));
    m_symbolsHint->setText(
        tr("Symbols this pattern adds to the legend. The permanent ones are already there."));
    m_addSymbolButton->setText(tr("Add a symbol"));
    m_freeTextLabel->setText(tr("Free text"));
    m_marginsLabel->setText(tr("Margins"));
    m_marginsHint->setText(tr("Margins this pattern names differently from the library."));
}

//---------------------------------------------------------------------------------------------------------------------

void OWidgetCoverPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------

void OWidgetCoverPage::Refresh()
{
    QPointer<VAbstractPattern> doc;
    if (VAbstractApplication::VApp() != nullptr)
    {
        doc = VAbstractValApplication::VApp()->getCurrentDocument();
    }

    if (doc != m_doc)
    {
        // The document is a single object that is emptied and reloaded rather than replaced, so
        // what changes is which one is open — and the one left behind must stop talking to us.
        if (m_doc != nullptr)
        {
            disconnect(m_doc, &VAbstractPattern::FullUpdateFromFile, this, &OWidgetCoverPage::OnFullUpdate);
        }
        m_doc = doc;
        if (m_doc != nullptr)
        {
            connect(m_doc, &VAbstractPattern::FullUpdateFromFile, this, &OWidgetCoverPage::OnFullUpdate);
        }
    }

    OnFullUpdate();
}

//---------------------------------------------------------------------------------------------------------------------

void OWidgetCoverPage::OnFullUpdate()
{
    // **Anything still waiting to be written belongs to the document that is going away.** A text
    // typed a moment before another pattern was opened would otherwise be written into that one.
    if (m_commitTimer != nullptr)
    {
        m_commitTimer->stop();
    }

    Populate();
}

//---------------------------------------------------------------------------------------------------------------------

void OWidgetCoverPage::Populate()
{
    m_loading = true;

    if (m_doc == nullptr)
    {
        m_page = VCoverPageData();
        m_nameEdit->clear();
        m_freeTextEdit->clear();
    }
    else
    {
        m_page = m_doc->GetCoverPage();
        m_nameEdit->setText(m_doc->GetPatternName());
        m_freeTextEdit->setPlainText(m_page.freeText);
    }

    // **The libraries are read when the dock is filled, and not held across sessions.** A symbol
    // added or a margin rule rewritten belongs to the library page, and the dock has no way of
    // hearing about it — reading them here is what makes a change visible the next time it is shown.
    m_symbolsLibrary = OSymbolLibrary::Load();
    m_marginLibrary = OSeamAllowanceLibrary::Load();

    RebuildSymbolRows();
    RebuildMarginRows();

    m_loading = false;

    // Nothing to gather without a pattern, and a field that cannot be written into should not look
    // as though it could.
    setEnabled(m_doc != nullptr);
}

//---------------------------------------------------------------------------------------------------------------------

void OWidgetCoverPage::RebuildSymbolRows()
{
    ClearLayout(m_symbolRows);

    const QString locale = DefaultUiLanguage();
    const bool hasChoice = m_doc != nullptr && !m_page.symbols.isEmpty();
    m_symbolsHint->setVisible(m_doc != nullptr && !hasChoice);
    m_addSymbolButton->setEnabled(m_doc != nullptr);

    for (int row = 0; row < m_page.symbols.size(); ++row)
    {
        const QString chosen = m_page.symbols.at(row);

        auto *host = MakeRow(this);
        auto *layout = new QHBoxLayout(host);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);

        auto *combo = MakeComboBox(host);
        combo->setObjectName(QStringLiteral("coverPageSymbol"));
        // The open question first, as the catalogue's own choices do: an entry that is worded rather
        // than blank, so a combo nobody has answered reads as a question and not as an empty one.
        combo->addItem(tr("Choose…"), QString());

        for (const OSymbolLibrary::Entry &entry : m_symbolsLibrary.entries)
        {
            // **The permanent ones are not offered.** They are on every page that uses the template
            // without being asked for, so choosing one would add nothing and say that it had.
            if (entry.permanent || entry.id.isEmpty())
            {
                continue;
            }
            combo->addItem(SymbolLabel(entry, locale), entry.id);
        }

        if (!chosen.isEmpty())
        {
            int index = combo->findData(chosen);
            if (index < 0)
            {
                // **A symbol the library no longer holds is kept, and shown as itself.** The choice
                // was made and is in the file: dropping it here would lose it the moment anything
                // else on this dock was touched, and the library may be put back.
                combo->addItem(chosen, chosen);
                index = combo->count() - 1;
            }
            combo->setCurrentIndex(index);
        }

        connect(combo, &QComboBox::currentIndexChanged, this, [this, row, combo](int) {
            if (m_loading || m_doc == nullptr || row >= m_page.symbols.size())
            {
                return;
            }
            m_page.symbols[row] = combo->currentData().toString();
            WritePage();
        });

        auto *remove = new QPushButton(tr("Remove"), host);
        remove->setObjectName(QStringLiteral("coverPageRemoveSymbol"));
        connect(remove, &QPushButton::clicked, this, [this, row]() {
            if (m_doc == nullptr || row >= m_page.symbols.size())
            {
                return;
            }
            m_page.symbols.removeAt(row);
            WritePage();
            RebuildSymbolRows();
        });

        layout->addWidget(combo, 1);
        layout->addWidget(remove);
        m_symbolRows->addWidget(host);
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OWidgetCoverPage::RebuildMarginRows()
{
    ClearLayout(m_marginRows);

    const QVector<qreal> widths =
        m_doc == nullptr ? QVector<qreal>() : OSeamAllowanceValues::Collect(m_doc);
    const QVector<qreal> ambiguous =
        OCoverSeamAllowances::Ambiguous(m_marginLibrary, widths, DefaultSeamAllowanceMm());

    // **The block is empty unless a rule has to speak for two widths.** Everything else is named by
    // its own rule, once, and there is nothing for anyone to decide; showing it would be asking a
    // question the library has already answered.
    const bool nothingToSay = m_doc == nullptr || ambiguous.isEmpty();
    m_marginsHint->setVisible(nothingToSay);

    for (const qreal mm : ambiguous)
    {
        auto *host = MakeRow(this);
        auto *layout = new QHBoxLayout(host);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);

        // **The width is what tells the rows apart, so it is read and not written.** It is the
        // pattern's own fact, and the one thing a reader of the page can measure.
        auto *widthLabel = new QLabel(OLengthUnits::Format(mm, WorkingUnit()), host);
        widthLabel->setObjectName(QStringLiteral("coverPageMarginWidth"));
        layout->addWidget(widthLabel);

        // **One field per language the margin library is written in**, which is the only authority
        // on which languages a margin name has — the cover page's own languages belong to a template
        // this dock knows nothing about, and a name written in one nobody prints would be lost.
        for (const QString &locale : m_marginLibrary.languages)
        {
            auto *edit = new QLineEdit(host);
            edit->setObjectName(QStringLiteral("coverPageMarginName"));
            edit->setText(CoverPageMarginNameFor(m_page.marginNames, mm, locale));
            // The name that would be printed if nobody renamed anything, offered but not written:
            // leaving the field empty is how a margin is given back to the library.
            edit->setPlaceholderText(
                OCoverSeamAllowances::LibraryNameFor(m_marginLibrary, mm, DefaultSeamAllowanceMm(), locale));

            connect(edit, &QLineEdit::textEdited, this, [this, mm, locale](const QString &text) {
                if (m_loading || m_doc == nullptr)
                {
                    return;
                }
                SetMarginName(mm, locale, text);
                WritePage();
            });

            layout->addWidget(edit, 1);
        }

        m_marginRows->addWidget(host);
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OWidgetCoverPage::SetMarginName(qreal mm, const QString &locale, const QString &text)
{
    const qint64 wanted = CoverPageWidthKey(mm);
    for (VCoverPageMarginName &margin : m_page.marginNames)
    {
        if (CoverPageWidthKey(margin.mm) == wanted)
        {
            margin.texts.insert(locale, text);
            return;
        }
    }

    VCoverPageMarginName margin;
    margin.mm = mm;
    margin.texts.insert(locale, text);
    m_page.marginNames.append(margin);
}

//---------------------------------------------------------------------------------------------------------------------

void OWidgetCoverPage::WritePage()
{
    if (m_doc == nullptr || m_loading)
    {
        return;
    }
    m_doc->SetCoverPage(m_page);

    // **The page has changed, and whoever draws it is told.** The write above cannot say it: the
    // document announces a change it cannot qualify, and the Export mode reads that as "the pattern
    // may have moved" — see CoverPageChanged.
    emit CoverPageChanged();
}
