/************************************************************************
 **
 **  @file   MeasurementWindow.cpp
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

#include "MeasurementWindow.h"
#include "xml/oomtdocument.h"
#include "xml/ootudocument.h"
#include "xml/oomtconverter.h"
#include "vabstractvalapplication.h"
#include "vvalentinasettings.h"

#include <QCloseEvent>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNodeList>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMessageBox>
#include <QRect>
#include <QScreen>

MeasurementWindow::MeasurementWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_workspace = new MeasurementWorkspace();
    setCentralWidget(m_workspace);

    // Application-modal: blocks input to all parent windows while open
    setWindowModality(Qt::ApplicationModal);

    resize(950, 700);

    // Center on parent window
    if (parent != nullptr)
    {
        QWidget *topLevel = parent->window();
        QRect parentRect = topLevel->geometry();
        move(parentRect.center() - rect().center());
    }

    // Close button in the workspace toolbar
    connect(m_workspace, &MeasurementWorkspace::CloseRequested,
            this, &MeasurementWindow::OnCloseRequested);

    // Update window title when document is modified
    connect(m_workspace, &MeasurementWorkspace::DocumentModified,
            this, &MeasurementWindow::UpdateWindowTitle);

    UpdateWindowTitle();
    setMinimumSize(800, 600);
}

MeasurementWindow::~MeasurementWindow() = default;

void MeasurementWindow::UpdateWindowTitle()
{
    QString title = m_workspace->FilePath().isEmpty()
        ? tr("New measurement table")
        : QFileInfo(m_workspace->FilePath()).fileName();
    setWindowTitle(title + QStringLiteral(" - Ostara"));
}

//---------------------------------------------------------------------------------------------------------------------
void MeasurementWindow::changeEvent(QEvent *event)
{
    // ⚠️ **Cette fenêtre est modeless sur quatre sites : elle peut rester ouverte pendant une
    // bascule de langue, et rien d'autre ne la reprend.** `MainWindow::RetranslateAll()` ne la
    // connaît pas — aucun pointeur n'est conservé de lui à elle. C'est donc elle qui se reprend, et
    // elle reprend l'espace de travail qu'elle héberge.
    if (event->type() == QEvent::LanguageChange)
    {
        UpdateWindowTitle();
        if (m_workspace != nullptr)
        {
            m_workspace->RetranslateUI();
        }
    }
    QMainWindow::changeEvent(event);
}

void MeasurementWindow::SetOrigin(MeasurementWorkspace::Origin origin)
{
    m_workspace->SetOrigin(origin);
}

void MeasurementWindow::SetModeless(bool modeless)
{
    if (modeless)
    {
        setWindowModality(Qt::NonModal);
        // Modeless windows from MainWindow use "Close", not "Back to pattern"
        m_workspace->SetOrigin(MeasurementWorkspace::Origin::FromWindow);

        // Restore saved geometry, or default to near-fullscreen on the active window's screen
        const QByteArray savedGeom = VAbstractValApplication::VApp()
            ->ValentinaSettings()->value(QStringLiteral("measurementWindowGeometry")).toByteArray();
        if (!savedGeom.isEmpty())
        {
            restoreGeometry(savedGeom);
        }
        else
        {
            // Use the screen where the parent (active) window is, not necessarily primary
            QScreen *screen = nullptr;
            if (parentWidget() != nullptr)
            {
                screen = parentWidget()->window()->screen();
            }
            if (screen == nullptr)
            {
                screen = QGuiApplication::primaryScreen();
            }
            if (screen != nullptr)
            {
                const QRect avail = screen->availableGeometry();
                resize(qRound(avail.width() * 0.90), qRound(avail.height() * 0.85));
                move(avail.center() - rect().center());
            }
        }

        m_isModeless = true;
    }
}

void MeasurementWindow::NewDocument(MeasurementsType type, Unit unit)
{
    m_workspace->NewDocument(type, unit);
    UpdateWindowTitle();
}

void MeasurementWindow::SetTableName(const QString &name)
{
    m_workspace->SetTableName(name);
}

auto MeasurementWindow::ImportStructure(const QString &filePath) -> int
{
    return m_workspace->ImportStructure(filePath);
}

bool MeasurementWindow::LoadFile(const QString &filePath)
{
    // Check file type
    const QFileInfo fi(filePath);
    const QString suffix = fi.suffix().toLower();

    if (suffix == QStringLiteral("omt"))
    {
        auto doc = std::make_unique<OOmtDocument>();
        doc->setXMLContent(filePath);
        m_workspace->SetFilePath(filePath);
        LoadDocument(std::move(doc));
        return true;
    }

    if (suffix == QStringLiteral("otu"))
    {
        auto doc = std::make_unique<OOtuDocument>();
        doc->setXMLContent(filePath);
        m_workspace->SetFilePath(filePath);
        LoadDocument(std::move(doc));
        return true;
    }

    // Legacy conversion from .vst/.vit
    if (suffix == QStringLiteral("vst"))
    {
        OOmtConverter converter;
        auto result = converter.Convert(filePath);
        if (!result.document)
        {
            QMessageBox::warning(this, tr("Conversion error"),
                                 tr("Unable to convert .vst file."));
            return false;
        }
        // Load the document first so the workspace is on Step 2 when the message appears
        m_workspace->SetFilePath(QString());
        LoadDocument(std::move(result.document));
        // Set the table name from the source file name (user can change it)
        m_workspace->SetTableName(QFileInfo(filePath).completeBaseName());
        // Now show the message on top of Step 2
        QString msg = tr("This legacy .vst file has been converted to .omt format.\n\n"
                         "Ostara calculates measurements differently from Tape, "
                         "so the converted values may differ from the original file.\n\n"
                         "Please verify the measurement values and adjust if needed before saving.");
        if (!result.unresolvedMeasurements.isEmpty())
        {
            msg += QStringLiteral("\n\n") +
                   tr("Unresolved measurements: %1")
                       .arg(result.unresolvedMeasurements.join(QStringLiteral(", ")));
        }
        QMessageBox::information(this, tr("Conversion complete"), msg);
        return true;
    }

    if (suffix == QStringLiteral("vit"))
    {
        // .vit → .otu : parse the .vit XML and map to OOtuDocument
        QFile file(filePath);
        if (!file.open(QFile::ReadOnly | QFile::Text))
        {
            QMessageBox::warning(this, tr("Read error"),
                                 tr("Unable to open file %1.").arg(filePath));
            return false;
        }
        QDomDocument vitDoc;
        const QDomDocument::ParseResult parseResult = vitDoc.setContent(&file);
        file.close();
        if (!parseResult)
        {
            QMessageBox::warning(this, tr("XML error"),
                                 tr("Invalid .vit file: %1").arg(parseResult.errorMessage));
            return false;
        }

        auto doc = std::make_unique<OOtuDocument>();
        doc->CreateEmptyFile();
        const QDomElement root = vitDoc.documentElement();
        // Read unit
        const QDomElement unitEl = root.firstChildElement(QStringLiteral("unit"));
        if (!unitEl.isNull())
        {
            const QString unitStr = unitEl.text().trimmed();
            doc->SetUnits(StrToUnits(unitStr));
        }
        // Read measurements
        const QDomElement bmEl = root.firstChildElement(QStringLiteral("body-measurements"));
        const QDomNodeList mList = bmEl.elementsByTagName(QStringLiteral("m"));
        for (int i = 0; i < mList.count(); ++i)
        {
            const QDomElement mEl = mList.at(i).toElement();
            const QString name = mEl.attribute(QStringLiteral("name"));
            const QString fullName = mEl.attribute(QStringLiteral("fullName"));
            const QString description = mEl.attribute(QStringLiteral("description"));
            const QString type = mEl.attribute(QStringLiteral("type"));
            const bool specialUnits = mEl.attribute(QStringLiteral("specialUnits")) == QStringLiteral("true");
            // For individual measurements, the value is a formula or number
            const QString valueStr = mEl.attribute(QStringLiteral("value"));
            double value = 0.0;
            // Try to parse as number; formulas from .vit are not supported in .otu
            bool ok = false;
            const double v = valueStr.toDouble(&ok);
            if (ok) { value = v; }
            doc->AddM(name, fullName, description, value, type, specialUnits);
        }
        m_workspace->SetFilePath(QString());
        LoadDocument(std::move(doc));
        // Set the table name from the source file name (user can change it)
        m_workspace->SetTableName(QFileInfo(filePath).completeBaseName());
        // Show message after LoadDocument so the workspace is on Step 2
        QMessageBox::information(this, tr("Conversion complete"),
            tr("This legacy .vit file has been converted to .otu format.\n\n"
               "Ostara calculates measurements differently from Tape, "
               "so the converted values may differ from the original file.\n\n"
               "Please verify the measurement values and adjust if needed before saving."));
        return true;
    }

    QMessageBox::warning(this, tr("Unsupported format"),
                         tr("File %1 is not a recognized format.").arg(filePath));
    return false;
}

void MeasurementWindow::LoadDocument(std::unique_ptr<OOmtDocument> doc)
{
    m_workspace->SetOrigin(MeasurementWorkspace::Origin::FromWindow);
    m_workspace->LoadDocument(std::move(doc));
    UpdateWindowTitle();
}

void MeasurementWindow::LoadDocument(std::unique_ptr<OOtuDocument> doc)
{
    m_workspace->SetOrigin(MeasurementWorkspace::Origin::FromWindow);
    m_workspace->LoadDocument(std::move(doc));
    UpdateWindowTitle();
}

void MeasurementWindow::closeEvent(QCloseEvent *event)
{
    if (HandleClose())
    {
        // Save geometry for modeless windows so the user's size/position is restored next time
        if (m_isModeless)
        {
            VAbstractValApplication::VApp()->ValentinaSettings()
                ->setValue(QStringLiteral("measurementWindowGeometry"), saveGeometry());
        }
        QMainWindow::closeEvent(event);
    }
    else
    {
        event->ignore();
    }
}

void MeasurementWindow::OnCloseRequested()
{
    close(); // Triggers closeEvent → HandleClose
}

bool MeasurementWindow::HandleClose()
{
    // Case 1: document with no file path (converted .vst/.vit or new — never saved)
    if (m_workspace->FilePath().isEmpty())
    {
        auto result = QMessageBox::warning(this,
            tr("Unsaved table"),
            tr("The measurement table was not saved.\n"
               "If you close without saving, data will be lost."),
            QMessageBox::Discard | QMessageBox::Save,
            QMessageBox::Save);

        if (result == QMessageBox::Discard)
        {
            // Close without saving — don't emit measurementsSaved
            return true;
        }
        else
        {
            // Open save dialog
            m_workspace->OnSave();

            // If still no file path, save was cancelled or failed — stay open
            if (m_workspace->FilePath().isEmpty())
            {
                return false;
            }
        }
    }
    // Case 2: modified document with existing file path — ask
    else if (m_workspace->IsModified())
    {
        auto result = QMessageBox::question(this,
            tr("Unsaved changes"),
            tr("The measurement table has been modified.\n"
               "Do you want to save changes before closing?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (result == QMessageBox::Cancel)
        {
            return false;
        }

        if (result == QMessageBox::Save)
        {
            m_workspace->OnSave();
            if (m_workspace->IsModified())
            {
                return false; // Save failed or was cancelled
            }
        }
        // Discard: fall through, emit nothing
    }

    // Emit the saved file path (empty if discarded/no path)
    const QString savedPath = m_workspace->FilePath();
    if (!savedPath.isEmpty())
    {
        emit measurementsSaved(savedPath);
    }

    return true;
}
