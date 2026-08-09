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
void TopBar::SetModeActions(QAction *draw, QAction *details, QAction *layout)
{
    m_btnDraw    = MakeModeButton(draw);
    m_btnDetails = MakeModeButton(details);
    m_btnLayout  = MakeModeButton(layout);

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

    m_sepDrawDetails = MakeSeparator();
    m_sepDetailsLayout = MakeSeparator();

    m_leftLayout->addWidget(m_btnDraw);
    m_leftLayout->addWidget(m_sepDrawDetails);
    m_leftLayout->addWidget(m_btnDetails);
    m_leftLayout->addWidget(m_sepDetailsLayout);
    m_leftLayout->addWidget(m_btnLayout);
}

//---------------------------------------------------------------------------------------------------------------------
void TopBar::SetPatternPieceWidgets(QLabel *pieceLabel, QComboBox *combo, QAction *newPieceAction)
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
    auto *btnRuler = new QToolButton(this);
    btnRuler->setDefaultAction(loadAction);
    btnRuler->setAutoRaise(true);
    btnRuler->setToolButtonStyle(Qt::ToolButtonIconOnly);
    btnRuler->setIconSize(QSize(24, 24));
    m_centerLayout->addWidget(btnRuler);

    fileNameLabel->setParent(this);
    m_measurementFileNameLabel = fileNameLabel;
    m_centerLayout->addWidget(fileNameLabel);

    // Dim separator (line-vertical, only visible when multisize)
    m_dimSeparator = new QLabel(this);
    m_dimSeparator->setVisible(false);
    m_centerLayout->addWidget(m_dimSeparator);

    // Vertical separator icon (always visible, between dims and table button)
    separator->setParent(this);
    m_centerLayout->addWidget(separator);

    auto *btnTable = new QToolButton(this);
    btnTable->setDefaultAction(tableAction);
    btnTable->setAutoRaise(true);
    btnTable->setToolButtonStyle(Qt::ToolButtonIconOnly);
    btnTable->setIconSize(QSize(24, 24));
    m_centerLayout->addWidget(btnTable);
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
    // Dimension widgets are children of this TopBar; remove them from the layout
    // by reparenting to nullptr (they'll be deleted by the caller via QPointer).
    // We rely on the caller (InitDimensionControls) to delete the old widgets first.
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
    ReloadSep(m_sepDetailsLayout);
    ReloadSep(m_sepPatternMeasure);

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
    SetIcon(m_btnLayout,  QStringLiteral("printer"));
}
