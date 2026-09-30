/************************************************************************
 **
 **  @file   MeasurementWorkspace.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/07/2026
 **
 **  @brief  Measurement workspace — container widget implementation.
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

#include "MeasurementWorkspace.h"
#include "SizeGridPanel.h"
#include "Step2Container.h"
#include "MeasurementLibraryScanner.h"
#include "dialogues/DialogSaveMeasurements.h"
#include "xml/oomtdocument.h"
#include "xml/ootudocument.h"
#include "vtheme.h"
#include "core/otranslation.h"

#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include "vcommonsettings.h"
#include "vabstractapplication.h"

namespace
{
    auto IconResPath(const QString &baseName, bool dark) -> QString
    {
        return QStringLiteral(":/icons/%1/%2_%1.svg")
            .arg(dark ? QStringLiteral("dark") : QStringLiteral("light"), baseName);
    }

    auto MakeToolIcon(const QString &baseName, bool dark) -> QIcon
    {
        return QIcon(IconResPath(baseName, dark));
    }
} // anonymous namespace

MeasurementWorkspace::MeasurementWorkspace(QWidget *parent)
    : QWidget(parent)
{
    SetupUI();
}

MeasurementWorkspace::~MeasurementWorkspace() = default;

bool MeasurementWorkspace::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Enter)
    {
        auto *btn = qobject_cast<QToolButton *>(obj);
        if (btn && btn->property("hoverIcon").isValid())
        {
            btn->setIcon(btn->property("hoverIcon").value<QIcon>());
        }
    }
    else if (event->type() == QEvent::Leave)
    {
        auto *btn = qobject_cast<QToolButton *>(obj);
        if (btn && btn->property("normalIcon").isValid())
        {
            btn->setIcon(btn->property("normalIcon").value<QIcon>());
        }
    }
    return QWidget::eventFilter(obj, event);
}

void MeasurementWorkspace::SetupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Title bar with toolbar
    auto *titleBar = new QHBoxLayout();
    titleBar->setContentsMargins(16, 16, 16, 8);

    // ⚠️ **Le titre n'est PAS confié au poseur**, et c'est délibéré : il porte soit « New measurement
    // table », soit le **nom du fichier** ouvert (voir `UpdateTitle()`). L'enregistrer auprès du
    // moteur écraserait le nom de fichier à chaque bascule de langue. Il est donc repris par
    // `RetranslateUI()`, qui sait lequel des deux écrire.
    m_titleLabel = new QLabel();
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    titleBar->addWidget(m_titleLabel);

    // Step indicator
    m_stepIndicator = new QLabel();
    m_stepIndicator->setObjectName(QStringLiteral("stepIndicator"));
    titleBar->addStretch();
    titleBar->addWidget(m_stepIndicator);

    titleBar->addStretch();

    // Toolbar — icon-only buttons with tooltips (SVG icons from QRC)
    // QToolButton avoids QPushButton's child-widget QSS limitations.

    auto makeToolBtn = [&](const QString &baseName, const QString &tooltip) -> QToolButton *
    {
        bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);

        auto *btn = new QToolButton();
        btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        btn->setToolTip(tooltip);
        btn->setObjectName(QStringLiteral("menuBtn"));
        btn->setAutoRaise(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setIconSize(QSize(24, 24));
        btn->setIcon(MakeToolIcon(baseName, dark));

        return btn;
    };

    m_saveBtn = makeToolBtn(QStringLiteral("floppy-disk"), tr("Save"));
    OTranslation::SetToolTip(this, m_saveBtn, QT_TRANSLATE_NOOP("MeasurementWorkspace", "Save"));
    connect(m_saveBtn, &QToolButton::clicked, this, &MeasurementWorkspace::OnSave);
    titleBar->addWidget(m_saveBtn);

    // No "Save As" button: a measurement table is saved into the Ostara library, and only there. Naming the
    // table is still asked for the first save — that is what Save does.

    // Son infobulle dépend de l'origine (« Home » ou « Close ») : c'est `SetOrigin()` qui la pose,
    // et `RetranslateUI()` le rappelle.
    m_closeBtn = makeToolBtn(QStringLiteral("house-fill"), tr("Home"));
    connect(m_closeBtn, &QToolButton::clicked, this, &MeasurementWorkspace::OnClose);
    titleBar->addWidget(m_closeBtn);

    mainLayout->addLayout(titleBar);

    // Stacked widget
    m_stackedWidget = new QStackedWidget();

    // Page 0: Size grid
    m_sizeGridPanel = new SizeGridPanel();
    connect(m_sizeGridPanel, &SizeGridPanel::GroupsChanged, this, &MeasurementWorkspace::OnDataChanged);
    m_stackedWidget->addWidget(m_sizeGridPanel); // Index 0

    // Page 1: Step 2 container (size selectors + table + detail)
    m_step2Container = new Step2Container();
    connect(m_step2Container, &Step2Container::MeasurementsChanged, this, &MeasurementWorkspace::OnDataChanged);
    m_stackedWidget->addWidget(m_step2Container); // Index 1

    mainLayout->addWidget(m_stackedWidget, 1);

    // Navigation bar
    auto *navBar = new QHBoxLayout();
    navBar->setContentsMargins(16, 8, 16, 8);

    m_prevBtn = new QPushButton();
    OTranslation::SetText(this, m_prevBtn, QT_TRANSLATE_NOOP("MeasurementWorkspace", "< Back"));
    m_prevBtn->setEnabled(false);
    connect(m_prevBtn, &QPushButton::clicked, this, &MeasurementWorkspace::OnPrevStep);
    navBar->addWidget(m_prevBtn);

    navBar->addStretch();

    m_nextBtn = new QPushButton();
    OTranslation::SetText(this, m_nextBtn, QT_TRANSLATE_NOOP("MeasurementWorkspace", "Next >"));
    connect(m_nextBtn, &QPushButton::clicked, this, &MeasurementWorkspace::OnNextStep);
    navBar->addWidget(m_nextBtn);

    mainLayout->addLayout(navBar);

    UpdateStepIndicator();
    UpdateTitle();
}

//---------------------------------------------------------------------------------------------------------------------
void MeasurementWorkspace::RetranslateUI()
{
    // Ce qui dépend d'un état est repris ici, parce que le moteur ne peut pas le deviner : le titre
    // (nom de fichier **ou** libellé), l'indicateur d'étape, et l'infobulle du bouton de fermeture,
    // qui vaut « Home » ou « Close » selon l'origine.
    UpdateTitle();
    UpdateStepIndicator();
    SetOrigin(m_origin);

    // Et la seconde page, qui porte des textes dépendant de son état (l'indice d'aide, l'infobulle
    // du bouton de suppression). La première — la grille de tailles — n'a que des textes fixes :
    // le poseur les couvre, elle n'a rien à reprendre ici.
    if (m_step2Container != nullptr)
    {
        m_step2Container->RetranslateUI();
    }
}

void MeasurementWorkspace::UpdateStepIndicator()
{
    if (m_measurementType == MeasurementsType::Individual)
    {
        m_stepIndicator->setText(tr("Measurements"));
    }
    else
    {
        QString step0 = (m_currentStep == 0) ? QStringLiteral("●") : QStringLiteral("○");
        QString step1 = (m_currentStep == 1) ? QStringLiteral("●") : QStringLiteral("○");
        m_stepIndicator->setText(tr("Step %1: %2 Sizes  %3 Measurements")
                                    .arg(m_currentStep + 1)
                                    .arg(step0, step1));
    }

    // Like Tape: once Step 1 is validated, there's no going back.
    // The "Retour" button is never shown.
    m_prevBtn->setVisible(false);

    bool isStep1 = (m_currentStep == 1);
    m_saveBtn->setVisible(isStep1);
    m_nextBtn->setVisible(!isStep1);
}

void MeasurementWorkspace::UpdateTitle()
{
    if (m_currentFilePath.isEmpty())
    {
        m_titleLabel->setText(tr("New measurement table"));
    }
    else
    {
        m_titleLabel->setText(QFileInfo(m_currentFilePath).fileName());
    }
}

void MeasurementWorkspace::SetFilePath(const QString &path)
{
    m_currentFilePath = path;
    UpdateTitle();
}

auto MeasurementWorkspace::FilePath() const -> QString
{
    return m_currentFilePath;
}

void MeasurementWorkspace::OnNextStep()
{
    if (m_currentStep == 0)
    {
        // Validate step 1
        if (!m_sizeGridPanel->IsValid())
        {
            return;
        }

        // Pass axis groups to step 2 container
        QStringList groupNames = m_sizeGridPanel->GetActiveGroupNames();
        QList<QStringList> groupValues;
        QMap<QString, QString> anchorBindings;
        for (const QString &name : groupNames)
        {
            groupValues.append(m_sizeGridPanel->GetGroupValues(name));
            anchorBindings.insert(name, m_sizeGridPanel->GetBaseBinding(name));
        }
        m_step2Container->SetAxisGroups(groupNames, groupValues, anchorBindings);
        m_step2Container->SetTableName(m_sizeGridPanel->GetTableName());

        // If loading an existing document, keep records; otherwise start fresh
        if (m_omtDoc && !m_omtDoc->GetAllMNames().isEmpty() && m_step2Container->GetRecords().isEmpty())
        {
            SyncPanelsFromDocument();
        }

        m_currentStep = 1;
        m_stackedWidget->setCurrentIndex(1);
    }

    UpdateStepIndicator();
}

void MeasurementWorkspace::OnPrevStep()
{
    // Dead code: like Tape, once Step 1 is validated there is no going back.
    // The "Retour" button is always hidden (see UpdateStepIndicator).
}

void MeasurementWorkspace::OnDataChanged()
{
    if (m_loading)
    {
        return;
    }
    if (!m_modified)
    {
        m_modified = true;
        emit DocumentModified();
    }
}

void MeasurementWorkspace::OnSave()
{
    SyncDocumentFromPanels();

    // A table that already lives in the library is overwritten in place, silently — that is what "Save"
    // means. Any other case goes through SaveToLibrary: the table is named (or keeps its name) and the
    // library gets a copy, while the file it was opened from stays untouched where it is.
    if (!m_currentFilePath.isEmpty() && MeasurementLibraryScanner::IsInLibrary(LibraryPath(), m_currentFilePath))
    {
        DoSave(m_currentFilePath);
        return;
    }

    SaveToLibrary();
}

void MeasurementWorkspace::SaveToLibrary()
{
    SyncDocumentFromPanels();

    QString defaultName;
    QString extension;
    if (m_measurementType == MeasurementsType::Individual && m_otuDoc)
    {
        defaultName = m_otuDoc->GetTableName();
        extension = QStringLiteral(".otu");
    }
    else if (m_omtDoc)
    {
        defaultName = m_omtDoc->GetTableName();
        extension = QStringLiteral(".omt");
    }
    else
    {
        return;
    }

    if (defaultName.isEmpty())
    {
        defaultName = tr("table");
    }

    DialogSaveMeasurements dlg(extension, defaultName, m_currentFilePath, this);
    if (dlg.exec() != QDialog::Accepted)
    {
        return;
    }

    const QString libPath =
        QDir::cleanPath(LibraryPath() + QDir::separator() + dlg.fileName());
    QDir().mkpath(QFileInfo(libPath).absolutePath());
    if (!ConfirmOverwrite(libPath))
    {
        return; // the existing library table is kept, and the current file path does not move
    }

    DoSave(libPath); // sets m_currentFilePath to the library copy on success
}

auto MeasurementWorkspace::LibraryPath() -> QString
{
    return VAbstractApplication::VApp()->Settings()->GetPathMeasurementLibrary();
}

void MeasurementWorkspace::SetOrigin(Origin origin)
{
    m_origin = origin;
    bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);

    if (origin == Origin::FromHome)
    {
        m_closeBtn->setIcon(MakeToolIcon(QStringLiteral("house-fill"), dark));
        m_closeBtn->setToolTip(tr("Home"));
    }
    else // FromDraw or FromWindow
    {
        m_closeBtn->setIcon(MakeToolIcon(QStringLiteral("x"), dark));
        m_closeBtn->setToolTip(tr("Close"));
    }
}

void MeasurementWorkspace::OnClose()
{
    emit CloseRequested();
}

void MeasurementWorkspace::DoSave(const QString &filePath)
{
    // OnSave overwrites a table that already lives in the library without asking: that is the point of
    // "Save". SaveToLibrary asks first, because there the name may be a new one.

    QString error;
    bool ok = false;

    if (m_measurementType == MeasurementsType::Individual && m_otuDoc)
    {
        ok = m_otuDoc->SaveDocument(filePath, error);
    }
    else if (m_omtDoc)
    {
        m_omtDoc->ComputeGrid();
        ok = m_omtDoc->SaveDocument(filePath, error);
    }

    if (!ok)
    {
        QMessageBox::critical(this,
            tr("Save error"),
            tr("Unable to save file.\n\n%1").arg(error));
        return;
    }

    m_currentFilePath = filePath;
    m_modified = false;
    UpdateTitle();
}

bool MeasurementWorkspace::ConfirmOverwrite(const QString &filePath) const
{
    if (!QFileInfo::exists(filePath))
    {
        return true;
    }

    auto result = QMessageBox::question(
        const_cast<MeasurementWorkspace *>(this),
        tr("File already exists"),
        tr("A file named \"%1\" already exists at this location.\n"
           "Do you want to overwrite it?").arg(QFileInfo(filePath).fileName()),
        QMessageBox::Yes | QMessageBox::No);

    return result == QMessageBox::Yes;
}

void MeasurementWorkspace::LoadDocument(std::unique_ptr<OOmtDocument> doc)
{
    m_loading = true;
    m_omtDoc = std::move(doc);
    m_otuDoc.reset();
    m_measurementType = MeasurementsType::Multisize;
    m_modified = false;
    SyncPanelsFromDocument();

    // Skip Step 1 if the document already has data (existing file or converted).
    // The size grid is loaded from the file and frozen (no "Retour" button).
    if (!m_currentFilePath.isEmpty() || m_omtDoc->GetSizeGroupCount() > 0)
    {
        m_currentStep = 1;
        m_stackedWidget->setCurrentIndex(1);
    }
    else
    {
        m_currentStep = 0;
        m_stackedWidget->setCurrentIndex(0);
    }

    UpdateStepIndicator();
    UpdateTitle();
    m_loading = false;
}

void MeasurementWorkspace::LoadDocument(std::unique_ptr<OOtuDocument> doc)
{
    m_loading = true;
    m_otuDoc = std::move(doc);
    m_omtDoc.reset();
    m_measurementType = MeasurementsType::Individual;
    m_modified = false;

    // Solo mode: go directly to Step 2, no size grid
    m_currentStep = 1;
    m_stackedWidget->setCurrentIndex(1);

    // Setup step 2 with empty groups (no size selectors, no intervals)
    m_step2Container->SetAxisGroups({}, {}, {});
    m_step2Container->SetTableName(m_otuDoc->GetTableName());

    SyncPanelsFromDocument();
    UpdateStepIndicator();
    UpdateTitle();
    m_loading = false;
}

void MeasurementWorkspace::NewDocument(Unit unit)
{
    NewDocument(MeasurementsType::Multisize, unit);
}

void MeasurementWorkspace::NewDocument(MeasurementsType type, Unit unit)
{
    if (type == MeasurementsType::Individual)
    {
        auto doc = std::make_unique<OOtuDocument>();
        doc->CreateEmptyFile(unit);
        LoadDocument(std::move(doc));
    }
    else
    {
        auto doc = std::make_unique<OOmtDocument>();
        doc->CreateEmptyFile(unit);
        LoadDocument(std::move(doc));
    }
}

void MeasurementWorkspace::SetTableName(const QString &name)
{
    if (name.isEmpty())
    {
        return;
    }
    if (m_currentStep == 0)
    {
        m_sizeGridPanel->SetTableName(name);
    }
    else
    {
        m_step2Container->SetTableName(name);
    }
}

auto MeasurementWorkspace::DocumentType() const -> MeasurementsType
{
    return m_measurementType;
}

auto MeasurementWorkspace::IsModified() const -> bool
{
    return m_modified;
}

void MeasurementWorkspace::SyncDocumentFromPanels()
{
    QString tableName = m_currentStep == 0
                        ? m_sizeGridPanel->GetTableName()
                        : m_step2Container->GetTableName();
    QVector<MeasurementRecord> records = m_step2Container->GetRecords();

    if (m_measurementType == MeasurementsType::Individual && m_otuDoc)
    {
        // Solo: no size groups, no anchor, no increments, no range groups, no grid
        m_otuDoc->SetTableName(tableName);

        QStringList existingM = m_otuDoc->GetAllMNames();
        for (const QString &name : existingM)
        {
            m_otuDoc->RemoveM(name);
        }

        for (const MeasurementRecord &rec : records)
        {
            if (rec.isSeparator)
            {
                m_otuDoc->AddM(rec.name, QString(), QString(), 0.0,
                               QStringLiteral("separator"));
            }
            else
            {
                m_otuDoc->AddM(rec.name, rec.fullName, rec.description,
                               rec.anchorValue, QStringLiteral("measurement"));
            }
        }

        // Sync collapse state
        for (const MeasurementRecord &rec : records)
        {
            if (rec.isSeparator)
            {
                m_otuDoc->SetMCollapsed(rec.name, rec.isCollapsed);
            }
        }
    }
    else if (m_omtDoc)
    {
        // Multi: sync size groups, anchor, increments, range groups
        m_omtDoc->ClearSizeGroups();
        QStringList groupNames = m_sizeGridPanel->GetActiveGroupNames();
        for (const QString &name : groupNames)
        {
            m_omtDoc->AddSizeGroup(name, m_sizeGridPanel->GetGroupValues(name));
        }

        m_omtDoc->ClearAnchor();
        for (const QString &name : groupNames)
        {
            QString baseVal = m_sizeGridPanel->GetBaseBinding(name);
            if (!baseVal.isEmpty())
            {
                m_omtDoc->SetAnchorBinding(name, baseVal);
            }
        }

        m_omtDoc->SetTableName(tableName);

        QStringList existingM = m_omtDoc->GetAllMNames();
        for (const QString &name : existingM)
        {
            m_omtDoc->RemoveM(name);
        }

        for (const MeasurementRecord &rec : records)
        {
            if (rec.isSeparator)
            {
                m_omtDoc->AddM(rec.name, QString(), QString(), 0.0, {},
                               QStringLiteral("separator"));
            }
            else
            {
                m_omtDoc->AddM(rec.name, rec.fullName, rec.description,
                               rec.anchorValue, rec.increments,
                               QStringLiteral("measurement"));
            }
        }

        // Sync range groups
        QMap<QString, QVector<RangeGroup>> rangeGroups = m_step2Container->GetRangeGroups();
        for (auto it = rangeGroups.cbegin(); it != rangeGroups.cend(); ++it)
        {
            m_omtDoc->ClearRangeGroups(it.key());
            for (const auto &rg : it.value())
            {
                m_omtDoc->AddRangeGroup(it.key(), rg);
            }
        }

        // Sync collapse state
        for (const MeasurementRecord &rec : records)
        {
            if (rec.isSeparator)
            {
                m_omtDoc->SetMCollapsed(rec.name, rec.isCollapsed);
            }
        }
    }
}

void MeasurementWorkspace::SyncPanelsFromDocument()
{
    if (m_measurementType == MeasurementsType::Individual && m_otuDoc)
    {
        // Solo: no size groups, no increments, no range groups
        m_step2Container->SetAxisGroups({}, {}, {});
        m_step2Container->SetTableName(m_otuDoc->GetTableName());
        m_step2Container->SetUnit(m_otuDoc->GetUnits());

        QStringList mNames = m_otuDoc->GetAllMNames();
        QStringList fullNames;
        QStringList descriptions;
        QList<double> values;
        QList<QMap<QString, GroupIncrement>> increments;  // always empty
        QStringList types;

        for (const QString &name : mNames)
        {
            fullNames.append(m_otuDoc->GetMFullName(name));
            descriptions.append(m_otuDoc->GetMDescription(name));
            values.append(m_otuDoc->GetMValue(name));
            increments.append(QMap<QString, GroupIncrement>{});  // no increments in solo mode
            types.append(m_otuDoc->GetMType(name));
        }

        m_step2Container->LoadFromDocument(mNames, fullNames, descriptions,
                                           values, increments, types);

        // Restore collapse state
        auto records = m_step2Container->GetRecords();
        for (int i = 0; i < records.size(); ++i)
        {
            if (records[i].isSeparator)
            {
                bool collapsed = m_otuDoc->GetMCollapsed(records[i].name);
                m_step2Container->SetGroupCollapsed(i, collapsed);
            }
        }

        // No range groups in solo
        m_step2Container->SetRangeGroups({});
    }
    else if (m_omtDoc)
    {
        // Multi: sync size groups, measurements, increments, range groups
        QStringList groupNames = m_omtDoc->GetSizeGroupNames();
        QList<QStringList> groupValues;

        for (const QString &name : groupNames)
        {
            groupValues.append(m_omtDoc->GetSizeGroupValues(name));
        }

        m_sizeGridPanel->LoadFromDocument(groupNames, groupValues, m_omtDoc->GetAnchorBindings());
        m_sizeGridPanel->SetTableName(m_omtDoc->GetTableName());

        QMap<QString, QString> anchorBindings = m_omtDoc->GetAnchorBindings();
        m_step2Container->SetAxisGroups(groupNames, groupValues, anchorBindings);
        m_step2Container->SetTableName(m_omtDoc->GetTableName());
        m_step2Container->SetUnit(m_omtDoc->GetUnits());

        QStringList mNames = m_omtDoc->GetAllMNames();
        QStringList fullNames;
        QStringList descriptions;
        QList<double> anchorValues;
        QList<QMap<QString, GroupIncrement>> increments;
        QStringList types;

        for (const QString &name : mNames)
        {
            fullNames.append(m_omtDoc->GetMFullName(name));
            descriptions.append(m_omtDoc->GetMDescription(name));
            anchorValues.append(m_omtDoc->GetMValue(name));
            increments.append(m_omtDoc->GetMIncrements(name));
            types.append(m_omtDoc->GetMType(name));
        }

        m_step2Container->LoadFromDocument(mNames, fullNames, descriptions,
                                           anchorValues, increments, types);

        // Restore collapse state
        auto records = m_step2Container->GetRecords();
        for (int i = 0; i < records.size(); ++i)
        {
            if (records[i].isSeparator)
            {
                bool collapsed = m_omtDoc->GetMCollapsed(records[i].name);
                m_step2Container->SetGroupCollapsed(i, collapsed);
            }
        }

        // Load range groups
        QMap<QString, QVector<RangeGroup>> rangeGroups;
        for (const QString &gn : groupNames)
        {
            rangeGroups[gn] = m_omtDoc->GetRangeGroups(gn);
        }
        m_step2Container->SetRangeGroups(rangeGroups);
    }

    m_modified = false;
}
