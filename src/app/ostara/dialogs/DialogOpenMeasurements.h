/************************************************************************
 **
 **  @file   DialogOpenMeasurements.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/07/2026
 **
 **  @brief  Dialog to open a measurement table — library lists + file browser.
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

#ifndef DIALOGOPENMEASUREMENTS_H
#define DIALOGOPENMEASUREMENTS_H

#include <QDialog>
#include <QString>
#include <QStringList>

class QListWidget;

/// Modal dialog to open a measurement table.
/// Section 1 — Multi-tailles (.omt) from library
/// Section 2 — Taille unique (.otu) from library
/// Section 3 — Browse any location (.omt/.otu/.vst/.vit)
class DialogOpenMeasurements : public QDialog
{
    Q_OBJECT

public:
    explicit DialogOpenMeasurements(QWidget *parent = nullptr);
    ~DialogOpenMeasurements() override = default;

    /// The one filter that names every measurement table format Ostara reads: .omt/.otu of its own, .vst/.vit of
    /// Tape. Named here so that the search a reader makes from this dialog and the one she makes when a pattern's
    /// link is unusable cannot come to read different formats.
    static auto MeasurementTablesFilter() -> QString;

    /// Selected file path, or empty if cancelled.
    auto selectedPath() const -> QString;

private slots:
    void OnBrowse();

private:
    void SetupUI();
    void PopulateLists();

    QListWidget *m_omtList{nullptr};
    QListWidget *m_otuList{nullptr};
    QString m_selectedPath;
};

#endif // DIALOGOPENMEASUREMENTS_H
