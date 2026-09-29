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
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QScreen>
#include <QShowEvent>

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
    :QDialog(parent), ui(new Ui::DialogNewPattern), m_data(data)
{
    ui->setupUi(this);


    VAbstractValApplication::VApp()->ValentinaSettings()->GetOsSeparator() ? setLocale(QLocale())
                                                                           : setLocale(QLocale::c());

    // Centring happens in showEvent, not here: see the comment there.
    ui->lineEditName->setText(patternPieceName);

    // The combo comes from the .ui, so the factory cannot have built it — and without its Qt-styled popup
    // the native macOS one ignores the stylesheet. rules.md §3.4.
    ApplyComboBoxView(ui->comboBoxUnits);

    InitUnits();

    if (creation == Creation::Piece)
    {
        // Same question, one more piece: the window says which of the two it is asking, and the unit — which
        // belongs to the pattern — is not part of it.
        setWindowTitle(tr("New pattern piece"));
        ui->formLayout->setRowVisible(kUnitRow, false);
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
