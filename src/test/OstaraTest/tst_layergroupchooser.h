/************************************************************************
 **
 **  @file   tst_layergroupchooser.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/11/2026
 **
 **  @brief  La ligne « dans quel groupe de calques ? » : ce qu'elle accepte comme réponse.
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

#ifndef TST_LAYERGROUPCHOOSER_H
#define TST_LAYERGROUPCHOOSER_H

#include <QObject>

class TST_LayerGroupChooser : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit TST_LayerGroupChooser(QObject *parent = nullptr);
    ~TST_LayerGroupChooser() override = default;

private slots:
    void NoGroupIsAnAnswer();
    void TheNewGroupEntryAloneIsNoAnswer();
    void ANamedNewGroupIsAnAnswer();
    void ANamedNewGroupIsNotAnExistingOne();
    void ANameAnExistingGroupWearsJoinsIt();
    void TypingRechecksTheOkButton();
    void TheEntryIsOnlyOfferedWhenCreatingIsAllowed();
};

#endif // TST_LAYERGROUPCHOOSER_H
