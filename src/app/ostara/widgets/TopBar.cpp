/************************************************************************
 **
 **  @file   TopBar.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   09/08/2026
 **
 **  @brief  Unified fixed top bar replacing individual QToolBars.
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
#include "TopBar.h"

#include "vtheme.h"

#include <QAction>
#include <QComboBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QToolButton>

//---------------------------------------------------------------------------------------------------------------------
TopBar::TopBar(QWidget *parent)
    : QWidget(parent)
{
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setContentsMargins(4, 2, 4, 2);
    m_mainLayout->setSpacing(4);

    // --- Left zone --------------------------------------------------------
    m_leftLayout = new QHBoxLayout();
    m_leftLayout->setContentsMargins(0, 0, 0, 0);
    m_leftLayout->setSpacing(2);
    m_mainLayout->addLayout(m_leftLayout);

    // --- Center zone (centered between left and right) -------------------
    // Balanced stretch on both sides of the center content ensures it is
    // horizontally centred regardless of left/right content widths.
    m_mainLayout->addStretch(1);

    m_centerLayout = new QHBoxLayout();
    m_centerLayout->setContentsMargins(0, 0, 0, 0);
    m_centerLayout->setSpacing(4);
    m_mainLayout->addLayout(m_centerLayout);

    m_mainLayout->addStretch(1);

    // --- Right zone -------------------------------------------------------
    m_rightLayout = new QHBoxLayout();
    m_rightLayout->setContentsMargins(0, 0, 0, 0);
    m_rightLayout->setSpacing(4);
    m_mainLayout->addLayout(m_rightLayout);
}

//---------------------------------------------------------------------------------------------------------------------
auto TopBar::MakeModeButton(QAction *action) -> QToolButton *
{
    auto *btn = new QToolButton(this);
    btn->setDefaultAction(action);
    btn->setAutoRaise(true);
    btn->setCheckable(true);
    btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    btn->setIconSize(QSize(24, 24));
    return btn;
}

//---------------------------------------------------------------------------------------------------------------------
void TopBar::SetModeActions(QAction *draw, QAction *details, QAction *graded, QAction *exportMode)
{
    m_btnDraw    = MakeModeButton(draw);
    m_btnDetails = MakeModeButton(details);
    m_btnGraded  = MakeModeButton(graded);
    m_btnExport  = MakeModeButton(exportMode);

    auto MakeSeparator = [&]() {
        auto *lbl = new QLabel(this);
        const bool isDark = VTheme::ColorSheme() == VColorSheme::Dark;
        const auto theme  = QString(isDark ? QStringLiteral("dark") : QStringLiteral("light"));
        const auto path = QStringLiteral(":/icons/%1/line-vertical_%1.svg").arg(theme);
        QPixmap pix(path);
        if (!pix.isNull())
        {
            lbl->setPixmap(pix.scaledToHeight(14, Qt::SmoothTransformation));
        }
        else
        {
            lbl->setText(QStringLiteral("|"));
        }
        return lbl;
    };

    m_sepDrawDetails   = MakeSeparator();
    m_sepDetailsGraded = MakeSeparator();
    m_sepGradedExport  = MakeSeparator();

    m_leftLayout->addWidget(m_btnDraw);
    m_leftLayout->addWidget(m_sepDrawDetails);
    m_leftLayout->addWidget(m_btnDetails);
    m_leftLayout->addWidget(m_sepDetailsGraded);
    m_leftLayout->addWidget(m_btnGraded);
    m_leftLayout->addWidget(m_sepGradedExport);
    m_leftLayout->addWidget(m_btnExport);

    UpdateModeButtonIcons();
}

//---------------------------------------------------------------------------------------------------------------------
void TopBar::SetPatternPieceWidgets(QLabel *pieceLabel, QComboBox *combo, QAction *newPieceAction,
                                    QAction *savePieceAction)
{
    m_pieceLabel = pieceLabel;
    pieceLabel->setParent(this);
    m_centerLayout->addWidget(pieceLabel);

    m_comboDraws = combo;
    combo->setParent(this);
    m_centerLayout->addWidget(combo);

    m_btnNewPiece = new QToolButton(this);
    m_btnNewPiece->setDefaultAction(newPieceAction);
    m_btnNewPiece->setAutoRaise(true);
    m_btnNewPiece->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_btnNewPiece->setIconSize(QSize(24, 24));
    m_centerLayout->addWidget(m_btnNewPiece);

    // Right next to the piece it saves: the piece the combo shows is the piece the button writes, so the
    // two are read as one gesture — choose, then keep.
    m_btnSavePiece = new QToolButton(this);
    m_btnSavePiece->setDefaultAction(savePieceAction);
    m_btnSavePiece->setAutoRaise(true);
    m_btnSavePiece->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_btnSavePiece->setIconSize(QSize(24, 24));
    m_centerLayout->addWidget(m_btnSavePiece);

    // Separator between pattern piece and measurement sections
    m_sepPatternMeasure = new QLabel(this);
    const bool isDark = VTheme::ColorSheme() == VColorSheme::Dark;
    const auto theme = QString(isDark ? QStringLiteral("dark") : QStringLiteral("light"));
    QPixmap pix(QStringLiteral(":/icons/%1/line-vertical_%1.svg").arg(theme));
    if (!pix.isNull())
    {
        m_sepPatternMeasure->setPixmap(pix.scaledToHeight(14, Qt::SmoothTransformation));
    }
    else
    {
        m_sepPatternMeasure->setText(QStringLiteral("|"));
    }
    m_centerLayout->addWidget(m_sepPatternMeasure);
}

//---------------------------------------------------------------------------------------------------------------------
void TopBar::SetMeasurementWidgets(QAction *loadAction, QLabel *fileNameLabel,
                                    QLabel *separator, QAction *tableAction)
{
    m_btnLoadTable = new QToolButton(this);
    m_btnLoadTable->setDefaultAction(loadAction);
    m_btnLoadTable->setAutoRaise(true);
    m_btnLoadTable->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_btnLoadTable->setIconSize(QSize(24, 24));
    m_centerLayout->addWidget(m_btnLoadTable);

    fileNameLabel->setParent(this);
    m_measurementFileNameLabel = fileNameLabel;
    m_centerLayout->addWidget(fileNameLabel);

    // Dim separator (line-vertical, only visible when multisize)
    m_dimSeparator = new QLabel(this);
    m_dimSeparator->setVisible(false);
    m_centerLayout->addWidget(m_dimSeparator);

    // Vertical separator icon (always visible, between dims and table button)
    m_sepAfterDims = separator;
    separator->setParent(this);
    m_centerLayout->addWidget(separator);

    m_btnTable = new QToolButton(this);
    m_btnTable->setDefaultAction(tableAction);
    m_btnTable->setAutoRaise(true);
    m_btnTable->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_btnTable->setIconSize(QSize(24, 24));
    m_centerLayout->addWidget(m_btnTable);
}

//---------------------------------------------------------------------------------------------------------------------
void TopBar::SetZoomWidget(QWidget *zoomBlock)
{
    zoomBlock->setParent(this);
    m_rightLayout->addWidget(zoomBlock);
}

//---------------------------------------------------------------------------------------------------------------------
void TopBar::ClearDimensionWidgets()
{
    // Dimension widgets are children of this TopBar; the caller (InitDimensionControls)
    // deletes the old widgets first, so the tracked pointers are reset by their QPointer.
    m_dimWidgets.clear();
    m_gradedChrome.visibility.clear();
    m_exportChrome.visibility.clear();
}

//---------------------------------------------------------------------------------------------------------------------
void TopBar::InsertDimensionWidgets(QLabel *label, QComboBox *combo)
{
    if (!label || !combo || m_centerLayout->count() < 2)
    {
        return;
    }

    label->setParent(this);
    combo->setParent(this);

    m_dimWidgets.append(label);
    m_dimWidgets.append(combo);

    // Insert before the vertical separator (second-to-last item in center layout),
    // which is right before the table button.
    const int lastIdx = m_centerLayout->count() - 1; // table button
    const int insertAt = lastIdx - 1;                 // before separator

    m_centerLayout->insertWidget(insertAt, label);
    m_centerLayout->insertWidget(insertAt + 1, combo);
}

//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
void TopBar::SetPatternPieceVisible(bool visible)
{
    if (m_pieceLabel)
    {
        m_pieceLabel->setVisible(visible);
    }
    if (m_comboDraws)
    {
        m_comboDraws->setVisible(visible);
    }
    if (m_btnNewPiece)
    {
        m_btnNewPiece->setVisible(visible);
    }
    if (m_btnSavePiece)
    {
        m_btnSavePiece->setVisible(visible);
    }
    if (m_sepPatternMeasure)
    {
        m_sepPatternMeasure->setVisible(visible);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TopBar::SetDimensionSeparatorVisible(bool visible)
{
    if (m_dimSeparator)
    {
        if (visible)
        {
            const bool isDark = VTheme::ColorSheme() == VColorSheme::Dark;
            const auto theme  = QString(isDark ? QStringLiteral("dark") : QStringLiteral("light"));
            const auto path = QStringLiteral(":/icons/%1/line-vertical_%1.svg").arg(theme);
            QPixmap pix(path);
            if (!pix.isNull())
            {
                m_dimSeparator->setPixmap(pix.scaledToHeight(14, Qt::SmoothTransformation));
            }
        }
        m_dimSeparator->setVisible(visible);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TopBar::SetDimensionWidgetsVisible(bool visible)
{
    for (const auto &w : m_dimWidgets)
    {
        if (w)
        {
            w->setVisible(visible);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TopBar::ApplyCenterChrome(CenterChromeSnapshot &snapshot, bool hidden)
{
    auto CenterWidgets = [this]() {
        QList<QWidget *> widgets;
        if (m_btnLoadTable)
        {
            widgets.append(m_btnLoadTable);
        }
        if (m_dimSeparator)
        {
            widgets.append(m_dimSeparator);
        }
        if (m_sepAfterDims)
        {
            widgets.append(m_sepAfterDims);
        }
        if (m_btnTable)
        {
            widgets.append(m_btnTable);
        }
        for (const auto &w : m_dimWidgets)
        {
            if (w)
            {
                widgets.append(w);
            }
        }
        return widgets;
    };

    if (hidden)
    {
        snapshot.visibility.clear();
        const auto widgets = CenterWidgets();
        for (QWidget *w : widgets)
        {
            snapshot.visibility.insert(w, w->isVisible());
            w->hide();
        }
        SetPatternPieceVisible(false);
    }
    else
    {
        SetDimensionWidgetsVisible(true);
        if (m_btnLoadTable)
        {
            m_btnLoadTable->setVisible(snapshot.visibility.value(m_btnLoadTable, true));
        }
        if (m_sepAfterDims)
        {
            m_sepAfterDims->setVisible(snapshot.visibility.value(m_sepAfterDims, true));
        }
        if (m_btnTable)
        {
            m_btnTable->setVisible(snapshot.visibility.value(m_btnTable, true));
        }
        if (m_dimSeparator)
        {
            m_dimSeparator->setVisible(snapshot.visibility.value(m_dimSeparator, false));
        }
        snapshot.visibility.clear();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TopBar::SetGradedChrome(bool graded)
{
    ApplyCenterChrome(m_gradedChrome, graded);
}

//---------------------------------------------------------------------------------------------------------------------
void TopBar::SetExportChrome(bool exportMode)
{
    ApplyCenterChrome(m_exportChrome, exportMode);
}

//---------------------------------------------------------------------------------------------------------------------
void TopBar::ReloadIcons()
{
    UpdateModeButtonIcons();

    const bool isDark = VTheme::ColorSheme() == VColorSheme::Dark;
    const auto theme  = QString(isDark ? QStringLiteral("dark") : QStringLiteral("light"));
    const auto path = QStringLiteral(":/icons/%1/line-vertical_%1.svg").arg(theme);

    auto ReloadSep = [&](QLabel *lbl) {
        if (!lbl)
        {
            return;
        }
        QPixmap pix(path);
        if (!pix.isNull())
        {
            lbl->setPixmap(pix.scaledToHeight(14, Qt::SmoothTransformation));
        }
    };

    ReloadSep(m_sepDrawDetails);
    ReloadSep(m_sepDetailsGraded);
    ReloadSep(m_sepGradedExport);
    ReloadSep(m_sepPatternMeasure);

    // The save button wears the only icon the mode set does not: it is not a mode, it is the piece.
    if (m_btnSavePiece)
    {
        m_btnSavePiece->setIcon(QIcon(QStringLiteral(":/icons/%1/floppy-disk-fill_%1.svg").arg(theme)));
    }

    // Refresh dim separator if visible
    if (m_dimSeparator && m_dimSeparator->isVisible())
    {
        QPixmap pix(path);
        if (!pix.isNull())
        {
            m_dimSeparator->setPixmap(pix.scaledToHeight(14, Qt::SmoothTransformation));
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TopBar::UpdateModeButtonIcons()
{
    const bool isDark = VTheme::ColorSheme() == VColorSheme::Dark;
    const auto theme  = QString(isDark ? QStringLiteral("dark") : QStringLiteral("light"));

    auto SetIcon = [&](QToolButton *btn, const QString &name) {
        if (btn)
        {
            btn->setIcon(QIcon(QStringLiteral(":/icons/%1/%2-fill_%1.svg").arg(theme, name)));
        }
    };

    SetIcon(m_btnDraw,    QStringLiteral("polygon"));
    SetIcon(m_btnDetails, QStringLiteral("sliders"));
    SetIcon(m_btnExport,  QStringLiteral("printer"));

    // Graded is disabled when the pattern's measurements table is single-size (.otu): the mode
    // needs a multi-size table. The disabled look comes from the theme's disabled set (Icons.md
    // §2) — Qt's automatic greying fades a fill icon into something unreadable.
    if (m_btnGraded)
    {
        QIcon graded(QStringLiteral(":/icons/%1/stack-fill_%1.svg").arg(theme));
        graded.addFile(QStringLiteral(":/icons/%1/stack-fill_disabled.svg").arg(theme), QSize(),
                       QIcon::Disabled, QIcon::Off);
        m_btnGraded->setIcon(graded);
    }
}
