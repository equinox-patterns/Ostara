/************************************************************************
 **
 **  @file   MeasurementWindow.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/07/2026
 **
 **  @brief  Modal window wrapping MeasurementWorkspace for editing from Draw
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

#ifndef MEASUREMENTWINDOW_H
#define MEASUREMENTWINDOW_H

#include <QMainWindow>
#include <memory>

#include "MeasurementWorkspace.h"
class OOmtDocument;
class OOtuDocument;

class MeasurementWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MeasurementWindow(QWidget *parent = nullptr);
    ~MeasurementWindow() override;

    /// Set the origin mode — controls close button label and return behavior.
    void SetOrigin(MeasurementWorkspace::Origin origin);

    /// Configure the window for modeless use (no modality, full-screen-ish size).
    void SetModeless(bool modeless);

    /// Create a new empty document.
    void NewDocument(MeasurementsType type, Unit unit = Unit::Cm);

    /// Set the table name on the workspace panels.
    void SetTableName(const QString &name);

    /// Load a document from a file path (.omt, .otu, .vst, .vit).
    /// Returns true on success, false on failure.
    bool LoadFile(const QString &filePath);

    /// Load a multi-size document into the internal workspace.
    void LoadDocument(std::unique_ptr<OOmtDocument> doc);

    /// Load a single-size document into the internal workspace.
    void LoadDocument(std::unique_ptr<OOtuDocument> doc);

signals:
    /// Emitted when the user saved and closed the window.
    /// filePath is the saved path; empty if the user discarded changes.
    void measurementsSaved(const QString &filePath);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void OnCloseRequested();

private:
    bool HandleClose();
    void UpdateWindowTitle();

    MeasurementWorkspace *m_workspace{nullptr};
    bool m_isModeless{false};
};

#endif // MEASUREMENTWINDOW_H
