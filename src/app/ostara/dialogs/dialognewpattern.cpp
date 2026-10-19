/************************************************************************
 **
 **  @file   dialognewpattern.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   22 2, 2014
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#include "dialognewpattern.h"
#include "ui_dialognewpattern.h"
#include "../core/opiecelibrary.h"
#include "../core/opieceimport.h"
#include "../ifc/ifcdef.h"
#include "../vmisc/FactoryWidgets.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vtools/dialogs/dialogtoolbox.h"

#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QScreen>
#include <QShowEvent>
#include <QSignalBlocker>

namespace
{
// The .ui's second row, "Units:" — the one a piece added to an existing pattern does not ask.
constexpr int kUnitRow = 1;

// The line-style items carry an ICON ONLY, with no text to fall back on: without the thumbnail size Qt
// shrinks them to an unreadable 16x16. 80x14 is the size the tool dialogs declare in their .ui
// (DialogEndLine, DialogGroup), with the same min 80 / max 110.
constexpr int kPenIconWidth = 80;
constexpr int kPenIconHeight = 14;
constexpr int kColorIconSize = 20;

// 80 % of the row: 8 parts of rule against 1 part of air on each side (welcomeSeparator,
// templateLanguageSeparator). Stretch factors, not a fixed width, so the rule follows the dialog.
constexpr int kRuleStretch = 8;
constexpr int kAirStretch = 1;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
DialogNewPattern::DialogNewPattern(VContainer *data, const QString &patternPieceName, Creation creation,
                                   QWidget *parent)
    :QDialog(parent), ui(new Ui::DialogNewPattern), m_data(data), m_creation(creation)
{
    ui->setupUi(this);


    VAbstractValApplication::VApp()->ValentinaSettings()->GetOsSeparator() ? setLocale(QLocale())
                                                                           : setLocale(QLocale::c());

    // Centring happens in showEvent, not here: see the comment there.
    ui->lineEditName->setText(patternPieceName);

    // The combos come from the .ui, so the factory cannot have built them — and without their Qt-styled popup
    // the native macOS one ignores the stylesheet. rules.md §3.4.
    ApplyComboBoxView(ui->comboBoxUnits);
    ApplyComboBoxView(ui->comboBoxFolder);

    InitUnits();

    if (creation == Creation::Piece)
    {
        // Same question, one more piece: the window says which of the two it is asking, and the unit — which
        // belongs to the pattern — is not part of it.
        setWindowTitle(tr("New pattern piece"));
        ui->formLayout->setRowVisible(kUnitRow, false);
    }

    // What OK does is said ON the button, because the two groups ask two different things and nothing else in
    // the window tells them apart: with a piece chosen, the name and the layer above do not apply.
    if (QPushButton *bOk = ui->buttonBox->button(QDialogButtonBox::Ok); bOk != nullptr)
    {
        m_okText = bOk->text();
    }

    auto insertPiece = [this](QListWidgetItem *item) -> void
    {
        if (item != nullptr && not item->isHidden() && (item->flags() & Qt::ItemIsEnabled) != 0)
        {
            m_importedPath = item->data(Qt::UserRole).toString();
            accept();
        }
    };
    connect(ui->listPieces, &QListWidget::itemDoubleClicked, this, insertPiece);
    // Enter, too: the gesture of the dialog that opens a pattern, and the one a reader tries first.
    connect(ui->listPieces, &QListWidget::itemActivated, this, insertPiece);
    connect(ui->listPieces, &QListWidget::itemSelectionChanged, this, &DialogNewPattern::CheckState);

    // The folder decides what the list holds; the search only hides rows of what is already read.
    connect(ui->comboBoxFolder, &QComboBox::currentIndexChanged, this, &DialogNewPattern::OnFolderChanged);
    connect(ui->lineEditSearch, &QLineEdit::textChanged, this, &DialogNewPattern::OnSearchChanged);

    // The unit is part of the question when a pattern is being born, and only then: changing it changes which
    // pieces the library can offer, so the list is read again.
    if (creation == Creation::Pattern)
    {
        connect(ui->comboBoxUnits, &QComboBox::currentIndexChanged, this, [this]() -> void { FillLibrary(); });
    }

    BuildLayerFields();

    CheckState();
    connect(ui->lineEditName, &QLineEdit::textChanged, this, &DialogNewPattern::CheckState);

    // No adjustSize() here, deliberately: it would shrink the dialog to its contents and throw away the
    // .ui's 440 px width, which is what gives the name field its room. Qt clamps the height to the layout's
    // minimum anyway, so the four added rows are laid out in full — measured on a probe: 440x209 at
    // showEvent, last row's bottom 50 px short of the edge.
}

//---------------------------------------------------------------------------------------------------------------------
DialogNewPattern::~DialogNewPattern()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNewPattern::BuildLayerFields()
{
    auto *ruleBar = new QHBoxLayout();
    ruleBar->setContentsMargins(0, 0, 0, 0);
    auto *rule = new QFrame(this);
    rule->setObjectName(QStringLiteral("newPatternLayerSeparator"));
    rule->setFixedHeight(1);
    ruleBar->addStretch(kAirStretch);
    ruleBar->addWidget(rule, kRuleStretch);
    ruleBar->addStretch(kAirStretch);

    m_layerName = new QLineEdit(tr("Construction"), this);

    m_layerPenStyle = MakeComboBox(this);
    m_layerPenStyle->setMinimumWidth(80);
    m_layerPenStyle->setMaximumWidth(110);
    m_layerPenStyle->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_layerPenStyle->setIconSize(QSize(kPenIconWidth, kPenIconHeight));
    // The pen a layer starts on, said rather than inherited from the combo's order: a plain solid line.
    FillComboBoxTypeLine(m_layerPenStyle,
                         LineStylesPics(m_layerPenStyle->palette().color(QPalette::Base),
                                        m_layerPenStyle->palette().color(QPalette::Text)),
                         TypeLineLine);

    m_layerColor = MakeComboBox(this);
    m_layerColor->setMinimumWidth(80);
    m_layerColor->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_layerColor->setIconSize(QSize(kColorIconSize, kColorIconSize));
    InitColorCombo(m_layerColor);
    SetColorComboCurrentName(m_layerColor, ColorBlack);

    ui->formLayout->addRow(ruleBar);
    ui->formLayout->addRow(tr("First layer name:"), m_layerName);
    ui->formLayout->addRow(tr("Pen style:"), m_layerPenStyle);
    ui->formLayout->addRow(tr("Color:"), m_layerColor);

    connect(m_layerName, &QLineEdit::textChanged, this, &DialogNewPattern::CheckState);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNewPattern::SetTakenNames(const QStringList &names)
{
    m_takenNames = names;
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNewPattern::SetLibraryDirectory(const QString &directory)
{
    m_libraryDirectory = directory;
    PopulateFolders();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNewPattern::PopulateFolders()
{
    const QSignalBlocker blocker(ui->comboBoxFolder);

    ui->comboBoxFolder->clear();

    if (m_libraryDirectory.isEmpty())
    {
        FillLibrary();
        return;
    }

    // The library's own folder is a line of its own here, like in the dialog that opens a pattern: a destination
    // that cannot be named would make the pieces at the root of the library unreachable.
    ui->comboBoxFolder->addItem(tr("The library's own folder"), m_libraryDirectory);
    for (const OPieceLibrary::Folder &folder : OPieceLibrary::Folders(m_libraryDirectory))
    {
        ui->comboBoxFolder->addItem(folder.label, folder.path);
    }

    // The library's own folder is what one looks in first; the search then looks in the folder chosen, not in
    // the whole library.
    ui->comboBoxFolder->setCurrentIndex(0);

    FillLibrary();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogNewPattern::CurrentFolder() const -> QString
{
    const QString chosen = ui->comboBoxFolder->currentData().toString();
    return chosen.isEmpty() ? m_libraryDirectory : chosen;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNewPattern::OnFolderChanged()
{
    FillLibrary();
    ui->listPieces->setFocus();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNewPattern::OnSearchChanged()
{
    FilterLibrary();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNewPattern::SetLibraryUnit(const Unit &unit)
{
    m_libraryUnit = unit;
    FillLibrary();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogNewPattern::WantedUnit() const -> Unit
{
    // A pattern being born has no file yet: the unit *is* the question this dialog asks, so the combo answers it.
    // One more piece in a pattern that exists inherits its unit, and the caller says which.
    return m_creation == Creation::Pattern ? PatternUnit() : m_libraryUnit;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNewPattern::FillLibrary()
{
    ui->listPieces->clear();
    m_libraryCount = 0;
    m_libraryLeftOut = 0;

    ui->labelImportHidden->clear();
    ui->labelImportHidden->setVisible(false);

    if (m_libraryDirectory.isEmpty())
    {
        ShowLibraryState();
        return;
    }

    const Unit wanted = WantedUnit();

    // **Only the folder the combo points at.** Choosing a folder is the gesture that says where to look; the
    // search then filters that list. A piece in another folder is reached by choosing that folder — the gesture
    // of the dialog that opens a pattern, and the reason a reader sorts her pieces into folders at all.
    const QList<OPieceLibrary::Summary> summaries = OPieceLibrary::Summaries(CurrentFolder());
    m_libraryCount = static_cast<int>(summaries.size());

    for (const OPieceLibrary::Summary &summary : summaries)
    {
        // **Only the pieces this pattern could take.** A piece is written in one unit and a pattern holds one from
        // end to end, so a piece of another unit is not a choice — it is a refusal three clicks from here. A file
        // that cannot be read declares no unit at all, and is left out with the same count.
        if (OPieceImport::PieceUnit(summary.unit) != wanted)
        {
            ++m_libraryLeftOut;
            continue;
        }

        // A line has no column header above it here, so what the row says has to be a sentence: the window's
        // "Needs" column becomes "needs Buste".
        QString line = summary.name;
        if (const QString needs = OPieceLibrary::Needs(summary); not needs.isEmpty())
        {
            line = tr("%1 - needs %2").arg(summary.name, needs);
        }

        auto *item = new QListWidgetItem(line, ui->listPieces);
        // The path is in the tooltip, under the details: the file is where a reader goes to look.
        item->setToolTip(OPieceLibrary::Details(summary) + QLatin1Char('\n') + summary.path);
        item->setData(Qt::UserRole, summary.path);
    }

    if (m_libraryLeftOut > 0)
    {
        // Said rather than hidden: a reader who saved a piece in another unit and does not see it here must not go
        // looking for a mistake of her own.
        ui->labelImportHidden->setText(
            tr("%n piece(s) not shown: another unit, or unreadable.", "", m_libraryLeftOut));
        ui->labelImportHidden->setVisible(true);
    }
    else
    {
        ui->labelImportHidden->clear();
        ui->labelImportHidden->setVisible(false);
    }

    FilterLibrary();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNewPattern::FilterLibrary()
{
    const QString needle = ui->lineEditSearch->text().trimmed();

    for (int row = 0; row < ui->listPieces->count(); ++row)
    {
        QListWidgetItem *item = ui->listPieces->item(row);
        item->setHidden(not needle.isEmpty() && not item->text().contains(needle, Qt::CaseInsensitive));
    }

    // The list has just changed under the selection: a row hidden by the search is not a choice the button may
    // offer, and the hint has to say which of the empty states the reader is looking at.
    CheckState();
    ShowLibraryState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNewPattern::ShowLibraryState()
{
    if (m_libraryDirectory.isEmpty())
    {
        ui->labelImportHint->clear();
        return;
    }

    if (ui->listPieces->count() == 0)
    {
        // An empty folder and a folder whose pieces are all in another unit are not the same thing to a reader:
        // the second one she can act on (another folder, another unit), the first one she cannot.
        ui->labelImportHint->setText(m_libraryCount == 0
                                         ? tr("This folder holds no piece.")
                                         : tr("No piece in the library is in %1.")
                                               .arg(UnitsToStr(WantedUnit(), true)));
        return;
    }

    int visible = 0;
    for (int row = 0; row < ui->listPieces->count(); ++row)
    {
        if (not ui->listPieces->item(row)->isHidden())
        {
            ++visible;
        }
    }

    ui->labelImportHint->setText(visible == 0
                                     ? tr("No piece matches the search.")
                                     : tr("Double-click a piece to insert it, or select one and accept."));
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogNewPattern::SelectedLibraryPiece() const -> QString
{
    if (const QListWidgetItem *item = ui->listPieces->currentItem(); item != nullptr && not item->isHidden())
    {
        return item->data(Qt::UserRole).toString();
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogNewPattern::ImportedPath() const -> QString
{
    // The double-click path is kept as it is: the dialog is already accepted by the time anyone asks, and the
    // list may have been emptied since.
    return m_importedPath.isEmpty() ? SelectedLibraryPiece() : m_importedPath;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogNewPattern::LayerName() const -> QString
{
    return m_layerName->text();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogNewPattern::LayerColor() const -> QString
{
    return GetColorComboCurrentName(m_layerColor);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogNewPattern::LayerPenStyle() const -> QString
{
    return GetComboBoxCurrentData(m_layerPenStyle, TypeLineLine);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogNewPattern::PatternUnit() const -> Unit
{
    const qint32 index = ui->comboBoxUnits->currentIndex();
    return StrToUnits(ui->comboBoxUnits->itemData(index).toString());
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNewPattern::CheckState()
{
    QPushButton *bOk = ui->buttonBox->button(QDialogButtonBox::Ok);
    SCASSERT(bOk != nullptr)

    // A piece chosen in the library is a whole answer on its own: it brings its name, its layers and its
    // increments, so the name and the layer fields above have nothing to say — and the button says so, rather
    // than letting the reader wonder which of the two groups is about to be read.
    if (not SelectedLibraryPiece().isEmpty())
    {
        bOk->setText(tr("Insert the piece"));
        bOk->setEnabled(true);
        return;
    }

    bOk->setText(m_okText);

    // The piece needs a name no other piece of this pattern wears, and the layer needs one too: both are
    // nameable things that end up written, so the button says what is missing by staying down.
    const QString pieceName = ui->lineEditName->text();
    bOk->setEnabled(not pieceName.isEmpty() && not m_takenNames.contains(pieceName)
                    && m_layerName != nullptr && not m_layerName->text().isEmpty());
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNewPattern::showEvent(QShowEvent *event)
{
    QDialog::showEvent( event );
    if ( event->spontaneous() )
    {
        return;
    }

    if (m_isInitialized)
    {
        return;
    }
    // do your init stuff here

    // Centred HERE, and on the frame rather than on the client size — the two reasons the export template
    // window spells out (oexporttemplatewindow.cpp:97-107), each enough on its own to look off-centre:
    // a resize in a constructor is only a REQUEST, so the dialog grows from a position computed for a size
    // it never got; and `move()` places the FRAME, title bar included, while `resize()` sizes the client
    // area, so centring the client size leaves the window half a title bar too low. frameGeometry() has
    // both settled by now, and the size the layout asked for is the one locked just below.
    if (const QScreen *screen = parentWidget() ? parentWidget()->screen() : QGuiApplication::primaryScreen())
    {
        const QRect available = screen->availableGeometry();
        QRect frame = frameGeometry();
        frame.moveCenter(available.center());

        // A dialog larger than the screen cannot be centred on it: pinned to the top and the left, so what
        // it loses is the far edge rather than both.
        if (frame.height() > available.height())
        {
            frame.moveTop(available.top());
        }
        if (frame.width() > available.width())
        {
            frame.moveLeft(available.left());
        }

        move(frame.topLeft());
    }

    setMaximumSize(size());
    setMinimumSize(size());

    m_isInitialized = true;//first show windows are held
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNewPattern::InitUnits()
{
    ui->comboBoxUnits->addItem(tr("Centimeters"), QVariant(UnitsToStr(Unit::Cm)));
    ui->comboBoxUnits->addItem(tr("Millimiters"), QVariant(UnitsToStr(Unit::Mm)));
    ui->comboBoxUnits->addItem(tr("Inches"), QVariant(UnitsToStr(Unit::Inch)));

    // set default unit
    const qint32 indexUnit = ui->comboBoxUnits->findData(
                VAbstractValApplication::VApp()->ValentinaSettings()->GetUnit());
    if (indexUnit != -1)
    {
        ui->comboBoxUnits->setCurrentIndex(indexUnit);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogNewPattern::name() const -> QString
{
    return ui->lineEditName->text();
}

