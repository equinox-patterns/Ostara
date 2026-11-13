/************************************************************************
 **
 **  @file   tst_exporttemplate.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/11/2026
 **
 **  @brief  Le modèle de mise en page PDF : la graisse qu'il écrit, et le schéma qui l'accepte.
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

#ifndef TST_EXPORTTEMPLATE_H
#define TST_EXPORTTEMPLATE_H

#include <QObject>

class TST_ExportTemplate : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit TST_ExportTemplate(QObject *parent = nullptr);
    ~TST_ExportTemplate() override = default;

private slots:
    void AFaceNameIsAValidTemplate();
    void AnOldNumberIsStillAValidTemplate();
    void TheFaceSurvivesTheRoundTrip();
    void TheSchemaStillRefusesAbsurdValues();
    void TheSavePathWritesAFaceName();
};

#endif // TST_EXPORTTEMPLATE_H
