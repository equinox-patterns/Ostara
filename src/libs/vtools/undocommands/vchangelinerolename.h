/************************************************************************
 **
 **  @file   vchangelinerolename.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   11 8, 2026
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2026 Valentina project
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
#ifndef VCHANGELINEROLENAME_H
#define VCHANGELINEROLENAME_H

#include <QString>
#include <QtGlobal>

#include "vundocommand.h"

class VChangeLineRoleName : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    VChangeLineRoleName(VAbstractPattern *doc,
                        QString pairKey,
                        QString oldSuffix,
                        QString newSuffix,
                        QUndoCommand *parent = nullptr);
    ~VChangeLineRoleName() override = default;

    void undo() override;
    void redo() override;

private:
    Q_DISABLE_COPY_MOVE(VChangeLineRoleName) // NOLINT

    void UpdateDom(const QString &oldSuffix, const QString &newSuffix);

    QString m_pairKey;
    QString m_oldSuffix;
    QString m_newSuffix;
};

#endif // VCHANGELINEROLENAME_H
