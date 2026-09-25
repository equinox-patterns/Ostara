/************************************************************************
 **
 **  @file   MeasurementWorkspace.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/07/2026
 **
 **  @brief  Measurement workspace — container for size grid config + measurement table.
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

#ifndef MEASUREMENTWORKSPACE_H
#define MEASUREMENTWORKSPACE_H

#include <QWidget>
#include "vmisc/def.h"

#include <memory>

class OOmtDocument;
class OOtuDocument;
class SizeGridPanel;
class Step2Container;
class QLabel;
class QPushButton;
class QStackedWidget;
class QToolButton;

class MeasurementWorkspace : public QWidget
{
    Q_OBJECT

public:
    /// Where the workspace was opened from — controls the close button label and return destination.
    enum class Origin
    {
        FromHome,   ///< Opened from WelcomeScreen — close returns to home.
        FromDraw,   ///< Opened from PatternWorkspace — close returns to Draw.
        FromWindow  ///< Opened as standalone modal window — close button says "Fermer".
    };

    explicit MeasurementWorkspace(QWidget *parent = nullptr);
    ~MeasurementWorkspace() override;

    bool eventFilter(QObject *obj, QEvent *event) override;

    /// Re-apply every text of the workspace, and of its two pages, in the installed language.
    void RetranslateUI();

    /// Load an existing multi-size document for editing.
    void LoadDocument(std::unique_ptr<OOmtDocument> doc);

    /// Load an existing single-size document for editing.
    void LoadDocument(std::unique_ptr<OOtuDocument> doc);

    /// Set the file path (when opening an existing file).
    void SetFilePath(const QString &path);

    /// Current file path (empty for new unsaved documents).
    auto FilePath() const -> QString;

    /// Create a new empty document.
    void NewDocument(Unit unit = Unit::Cm);

    /// Create a new empty document with a specific type.
    void NewDocument(MeasurementsType type, Unit unit = Unit::Cm);

    /// Set the table name on the currently active panel.
    void SetTableName(const QString &name);

    /// Get the current document type.
    auto DocumentType() const -> MeasurementsType;

    /// Has unsaved changes?
    auto IsModified() const -> bool;

    /// Set the origin (controls close button label and return destination).
    void SetOrigin(Origin origin);

    /// Where the workspace was opened from.
    auto GetOrigin() const -> Origin { return m_origin; }

signals:
    void DocumentModified();
    void CloseRequested();

public slots:
    void OnSave();
    void OnSaveAs();

private slots:
    void OnNextStep();
    void OnPrevStep();
    void OnDataChanged();
    void OnClose();

private:
    void SetupUI();
    void UpdateStepIndicator();
    void UpdateTitle();
    void SyncDocumentFromPanels();
    void SyncPanelsFromDocument();
    void DoSave(const QString &filePath);
    bool ConfirmOverwrite(const QString &filePath) const;

    QStackedWidget *m_stackedWidget{nullptr};
    SizeGridPanel *m_sizeGridPanel{nullptr};
    Step2Container *m_step2Container{nullptr};

    // Navigation
    QLabel *m_stepIndicator{nullptr};
    QPushButton *m_prevBtn{nullptr};
    QPushButton *m_nextBtn{nullptr};

    // Toolbar
    QToolButton *m_saveBtn{nullptr};
    QToolButton *m_saveAsBtn{nullptr};
    QToolButton *m_closeBtn{nullptr};

    // Title
    QLabel *m_titleLabel{nullptr};

    Origin m_origin{Origin::FromHome};
    std::unique_ptr<OOmtDocument> m_omtDoc;
    std::unique_ptr<OOtuDocument> m_otuDoc;
    MeasurementsType m_measurementType{MeasurementsType::Multisize};
    QString m_currentFilePath;
    bool m_modified{false};
    bool m_loading{false}; // true during LoadDocument — suppresses OnDataChanged
    int m_currentStep{0}; // 0 = size grid, 1 = measurement table
};

#endif // MEASUREMENTWORKSPACE_H
