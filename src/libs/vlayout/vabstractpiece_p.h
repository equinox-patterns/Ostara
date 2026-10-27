/************************************************************************
 **
 **  @file
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   9 11, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2016 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 26/10/2026.
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

#ifndef VABSTRACTPIECE_P_H
#define VABSTRACTPIECE_P_H

#include <QCoreApplication>
#include <QSharedData>
#include <QString>
#include <QUuid>

#include "../vmisc/defglobal.h"

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Weffc++")
QT_WARNING_DISABLE_GCC("-Wnon-virtual-dtor")

class VAbstractPieceData : public QSharedData
{
    Q_DECLARE_TR_FUNCTIONS(VAbstractPieceData) // NOLINT

public:
    VAbstractPieceData() = default;
    VAbstractPieceData(const VAbstractPieceData &piece) = default;
    ~VAbstractPieceData() = default;


    QString m_name{tr("Detail")}; // NOLINT (misc-non-private-member-variables-in-classes)
    bool m_forceFlipping{false};                             // NOLINT (misc-non-private-member-variables-in-classes)
    bool m_symmetricalCopy{true};                            // NOLINT (misc-non-private-member-variables-in-classes)
    bool m_seamAllowance{false};                             // NOLINT (misc-non-private-member-variables-in-classes)
    bool m_hideMainPath{false};                              // NOLINT (misc-non-private-member-variables-in-classes)
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    bool m_secondPassmarkOnSeamLine{true};
    qreal m_width{0};                                        // NOLINT (misc-non-private-member-variables-in-classes)
    qreal m_mx{0};                                           // NOLINT (misc-non-private-member-variables-in-classes)
    qreal m_my{0};                                           // NOLINT (misc-non-private-member-variables-in-classes)
    uint m_priority{0};                                      // NOLINT (misc-non-private-member-variables-in-classes)
    QUuid m_uuid{QUuid::createUuid()};                       // NOLINT (misc-non-private-member-variables-in-classes)
    bool m_onDrawing{false};                                 // NOLINT (misc-non-private-member-variables-in-classes)
    bool m_showFullPiece{true};                              // NOLINT (misc-non-private-member-variables-in-classes)
    bool m_foldLineLabelFontItalic{false};                   // NOLINT (misc-non-private-member-variables-in-classes)
    bool m_foldLineLabelFontBold{false};                     // NOLINT (misc-non-private-member-variables-in-classes)
    QString m_foldLineLabel{};                               // NOLINT (misc-non-private-member-variables-in-classes)
    int m_foldLineLabelAlignment{Qt::AlignHCenter};          // NOLINT (misc-non-private-member-variables-in-classes)
    bool m_showMirrorLine{true};                             // NOLINT (misc-non-private-member-variables-in-classes)

    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    FoldLineType m_foldLineType{FoldLineType::TwoArrowsTextAbove};

private:
    Q_DISABLE_ASSIGN_MOVE(VAbstractPieceData) // NOLINT
};

QT_WARNING_POP


#endif // VABSTRACTPIECE_P_H
