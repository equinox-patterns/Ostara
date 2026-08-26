/************************************************************************
 **
 **  @file   mainwindow_measurements.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Measurement logic methods extracted from mainwindow.cpp
 **
 **  @note   Extracted from mainwindow.cpp by Sophie Denys <contact@sophiedenys.com>
 **          as part of Ostara refactoring.
 **
 **  @copyright
 **  Copyright (C) 2013-2015 Valentina project
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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "widgets/TopBar.h"
#include "vmisc/FactoryWidgets.h"
#include "mainwindow_helpers.h"
#include "vtheme.h"
#include "../vformat/vmeasurements.h"
#include "../vpatterndb/variables/vmeasurement.h"
#include "../vpatterndb/variables/vincrement.h"
#include "../vmisc/qxtcsvmodel.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vvalentinasettings.h"
#include "../ifc/exception/vexceptionemptyparameter.h"
#include "../ifc/xml/utils.h"
#include "../vmisc/vsysexits.h"
#include "core/vapplication.h"
#include <QComboBox>
#include <QFontMetrics>
#include <QLabel>
#include <QTimer>
#include <QFileInfo>
#include <QDir>

using namespace Qt::Literals::StringLiterals;


//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::LoadMeasurements(const QString &patternPath, QString &path) -> bool
{
    m_m = OpenMeasurementFile(patternPath, path);

    if (m_m.isNull())
    {
        return false;
    }

    try
    {
        VAbstractValApplication::VApp()->SetMeasurementsType(m_m->Type());
        if (m_m->Type() == MeasurementsType::Individual)
        {
            VAbstractValApplication::VApp()->SetCustomerName(m_m->Customer());
            VAbstractValApplication::VApp()->SetCustomerBirthDate(m_m->BirthDate());
            VAbstractValApplication::VApp()->SetCustomerEmail(m_m->Email());
        }
        else if (m_m->Type() == MeasurementsType::Multisize)
        {
            auto DimensionBase = [](qreal current, qreal tableBase)
            { return not VFuzzyComparePossibleNulls(current, -1) ? current : tableBase; };

            m_currentDimensionA = DimensionBase(doc->GetDimensionAValue(), m_m->DimensionABase());
            m_currentDimensionB = DimensionBase(doc->GetDimensionBValue(), m_m->DimensionBBase());
            m_currentDimensionC = DimensionBase(doc->GetDimensionCValue(), m_m->DimensionCBase());
        }
        UpdateMeasurementState();
        SetDimensionBases();
        pattern->ClearVariables({VarType::Measurement, VarType::MeasurementSeparator});
        m_m->StoreNames(false);
        m_m->ReadMeasurements(m_currentDimensionA, m_currentDimensionB, m_currentDimensionC);
    }
    catch (VExceptionEmptyParameter &e)
    {
        qCCritical(vMainWindow, "%s\n\n%s\n\n%s", qUtf8Printable(tr("File error.")), qUtf8Printable(e.ErrorMessage()),
                   qUtf8Printable(e.DetailedInformation()));
        if (not VApplication::IsGUIMode())
        {
            QCoreApplication::exit(V_EX_NOINPUT);
        }
        return false;
    }

    StoreDimensions();

    return true;
}


//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::UpdateMeasurements(const QString &patternPath, QString &path, qreal baseA, qreal baseB, qreal baseC) -> bool
{
    return UpdateMeasurements(OpenMeasurementFile(patternPath, path), baseA, baseB, baseC);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ReadMeasurements(qreal baseA, qreal baseB, qreal baseC, bool rebuildGradation)
{
    pattern->ClearVariables({VarType::Measurement, VarType::MeasurementSeparator});

    // Rebuilding the dimension labels and gradation combo boxes is only needed when the measurement
    // file itself (re)loads. On a plain size switch the gradation lists are unchanged and rebuilding
    // them (with per-item QFontMetrics work) is redundant synchronous UI work on the hot path.
    if (rebuildGradation && not m_m->Dimensions().isEmpty())
    {
        const QList<MeasurementDimension_p> dimensions = m_m->Dimensions().values();

        auto InitDimensionLabel =
            [this](const MeasurementDimension_p &dimension, QPointer<QLabel> &name, QPointer<QComboBox> &control)
        {
            if (dimension.isNull())
            {
                return;
            }

            if (name == nullptr)
            {
                name = new QLabel(dimension->Name() + ':'_L1);
            }
            else
            {
                name->setText(dimension->Name() + ':'_L1);
            }
            name->setToolTip(VAbstartMeasurementDimension::DimensionToolTip(dimension, m_m->IsFullCircumference()));

            if (control == nullptr)
            {
                control = MakeComboBox(this);
                control->setSizeAdjustPolicy(QComboBox::AdjustToContents);
            }
        };

        MeasurementDimension_p dimension = dimensions.at(0);
        InitDimensionLabel(dimension, m_dimensionALabel, m_dimensionA);
        InitDimensionGradation(0, dimension, m_dimensionA);

        if (dimensions.size() > 1)
        {
            dimension = dimensions.at(1);
            InitDimensionLabel(dimension, m_dimensionBLabel, m_dimensionB);
            InitDimensionGradation(1, dimension, m_dimensionB);

            if (dimensions.size() > 2)
            {
                dimension = dimensions.at(2);
                InitDimensionLabel(dimension, m_dimensionCLabel, m_dimensionC);
                InitDimensionGradation(2, dimension, m_dimensionC);
            }
        }
    }

    m_m->StoreNames(false);
    m_m->ReadMeasurements(baseA, baseB, baseC);
    if (m_m->Type() == MeasurementsType::Individual)
    {
        VAbstractValApplication::VApp()->SetCustomerName(m_m->Customer());
        VAbstractValApplication::VApp()->SetCustomerBirthDate(m_m->BirthDate());
        VAbstractValApplication::VApp()->SetCustomerEmail(m_m->Email());
    }
}



//---------------------------------------------------------------------------------------------------------------------
void MainWindow::StoreMultisizeMDimensions()
{
    VAbstractValApplication::VApp()->SetMeasurementsUnits(m_m->Units());

    QList<MeasurementDimension_p> const dimensions = m_m->Dimensions().values();

    if (not dimensions.isEmpty() && not m_dimensionALabel.isNull())
    {
        m_dimensionALabel->setText(dimensions.at(0)->Name() + ':'_L1);
    }

    if (dimensions.size() > 1 && not m_dimensionBLabel.isNull())
    {
        m_dimensionBLabel->setText(dimensions.at(1)->Name() + ':'_L1);
    }

    if (dimensions.size() > 2 && not m_dimensionCLabel.isNull())
    {
        m_dimensionCLabel->setText(dimensions.at(2)->Name() + ':'_L1);
    }

    StoreMultisizeMDimension(dimensions, 0, m_currentDimensionA);
    StoreMultisizeMDimension(dimensions, 1, m_currentDimensionB);
    StoreMultisizeMDimension(dimensions, 2, m_currentDimensionC);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::StoreIndividualMDimensions()
{
    QMap<QString, QSharedPointer<VMeasurement>> const measurements = pattern->DataMeasurements();

    StoreIndividualMDimension(measurements, IMD::X);
    StoreIndividualMDimension(measurements, IMD::Y);
    StoreIndividualMDimension(measurements, IMD::W);
    StoreIndividualMDimension(measurements, IMD::Z);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::SetDimensionBases()
{
    const QList<MeasurementDimension_p> dimensions = m_m->Dimensions().values();

    auto SetBase = [dimensions](int index, const QPointer<QComboBox> &control, double &value)
    {
        if (dimensions.size() > index)
        {
            SCASSERT(control != nullptr)

            const QSignalBlocker blocker(control);

            if (const qint32 i = control->findData(value); i != -1)
            {
                control->setCurrentIndex(i);
            }
            else
            {
                value = control->currentData().toDouble();
            }
        }
    };

    SetBase(0, m_dimensionA, m_currentDimensionA);
    SetBase(1, m_dimensionB, m_currentDimensionB);
    SetBase(2, m_dimensionC, m_currentDimensionC);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::StoreDimensions()
{
    if (m_m->Type() == MeasurementsType::Multisize)
    {
        StoreMultisizeMDimensions();

        doc->SetPatternWasChanged(true);
        emit doc->UpdatePatternLabel();
    }
    else if (m_m->Type() == MeasurementsType::Individual)
    {
        StoreIndividualMDimensions();

        doc->SetPatternWasChanged(true);
        emit doc->UpdatePatternLabel();
    }
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::DimensionABaseChanged()
{
    if (m_m.isNull())
    {
        return;
    }

    const qreal oldValue = m_currentDimensionA;
    m_currentDimensionA = m_dimensionA->currentData().toDouble();
    doc->SetDimensionAValue(m_currentDimensionA);

    if (not VFuzzyComparePossibleNulls(oldValue, m_currentDimensionA))
    {
        if (const QList<MeasurementDimension_p> dimensions = m_m->Dimensions().values(); dimensions.size() > 1)
        {
            MeasurementDimension_p dimension = dimensions.at(1);
            InitDimensionGradation(1, dimension, m_dimensionB);

            if (dimensions.size() > 2)
            {
                dimension = dimensions.at(2);
                InitDimensionGradation(2, dimension, m_dimensionC);
            }
        }

        m_gradation->start();
    }
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::DimensionBBaseChanged()
{
    if (m_m.isNull())
    {
        return;
    }

    const qreal oldValue = m_currentDimensionB;
    m_currentDimensionB = m_dimensionB->currentData().toDouble();
    doc->SetDimensionBValue(m_currentDimensionB);

    if (not VFuzzyComparePossibleNulls(oldValue, m_currentDimensionB))
    {
        if (const QList<MeasurementDimension_p> dimensions = m_m->Dimensions().values(); dimensions.size() > 2)
        {
            const MeasurementDimension_p &dimension = dimensions.at(2);
            InitDimensionGradation(2, dimension, m_dimensionC);
        }

        m_gradation->start();
    }
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::DimensionCBaseChanged()
{
    const qreal oldValue = m_currentDimensionC;
    m_currentDimensionC = m_dimensionC->currentData().toDouble();
    doc->SetDimensionCValue(m_currentDimensionC);

    if (not VFuzzyComparePossibleNulls(oldValue, m_currentDimensionC))
    {
        m_gradation->start();
    }
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::GradationChanged()
{
    m_gradation->stop();

    // Only a freshly (re)opened measurement file needs the dimension gradation combo boxes rebuilt.
    // A plain size switch keeps the same gradation lists, so skip that redundant work.
    bool reopened = false;
    if (m_m.isNull())
    {
        const QString patternPath = VAbstractValApplication::VApp()->GetPatternPath();
        QString mPath = AbsoluteMPath(patternPath, doc->MPath());
        m_m = OpenMeasurementFile(patternPath, mPath);
        reopened = true;
    }

    if (UpdateMeasurements(m_m, m_currentDimensionA, m_currentDimensionB, m_currentDimensionC, reopened))
    {
        doc->LiteParseTree(Document::FullLiteParse);
        StoreDimensions();
        // A size switch is a discrete user action: refresh piece geometry immediately instead of
        // waiting for the debounce timer started by the parse. RefreshPieceGeometry() also refreshes
        // the piece labels, so the previous eager DimensionsChanged() emit is no longer needed.
        doc->RefreshPieceGeometry();
    }
    else
    {
        qCWarning(vMainWindow, "%s", qUtf8Printable(tr("Couldn't update measurements.")));
    }
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::InitDimensionControls()
{
    delete m_dimensionA.data();
    delete m_dimensionALabel.data();
    delete m_dimensionB.data();
    delete m_dimensionBLabel.data();
    delete m_dimensionC.data();
    delete m_dimensionCLabel.data();

    const bool multisize = m_m && VAbstractValApplication::VApp()->GetMeasurementsType() == MeasurementsType::Multisize;
    m_topBar->SetDimensionSeparatorVisible(multisize);

    if (multisize)
    {
        const QList<MeasurementDimension_p> dimensions = m_m->Dimensions().values();

        auto InitControl = [this, dimensions](int index, QPointer<QLabel> &name, QPointer<QComboBox> &control)
        {
            if (dimensions.size() > index)
            {
                const MeasurementDimension_p &dimension = dimensions.at(index);

                if (name.isNull())
                {
                    name = new QLabel(dimension->Name() + ':'_L1);
                }
                else
                {
                    name->setText(dimension->Name() + ':'_L1);
                }
                name->setToolTip(VAbstartMeasurementDimension::DimensionToolTip(dimension, m_m->IsFullCircumference()));

                if (control.isNull())
                {
                    control = MakeComboBox(this);
                    control->setSizeAdjustPolicy(QComboBox::AdjustToContents);
                }

                InitDimensionGradation(index, dimension, control);

                m_topBar->InsertDimensionWidgets(name, control);
            }
        };

        InitControl(0, m_dimensionALabel, m_dimensionA);
        InitControl(1, m_dimensionBLabel, m_dimensionB);
        InitControl(2, m_dimensionCLabel, m_dimensionC);

        if (not m_dimensionA.isNull())
        {
            connect(m_dimensionA.data(), QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    &MainWindow::DimensionABaseChanged);
        }

        if (not m_dimensionB.isNull())
        {
            connect(m_dimensionB.data(), QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    &MainWindow::DimensionBBaseChanged);
        }

        if (not m_dimensionC.isNull())
        {
            connect(m_dimensionC.data(), QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    &MainWindow::DimensionCBaseChanged);
        }
    }
}


//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::CheckPathToMeasurements(const QString &patternPath, const QString &path) -> QString
{
    if (path.isEmpty())
    {
        return path;
    }

    auto FindLocation = [this](const QString &filter, const QString &dirPath, const QString &selectedName)
    {
        QFileDialog dialog(this, tr("Open file"), dirPath, filter);
        dialog.selectFile(selectedName);
        dialog.setFileMode(QFileDialog::ExistingFile);
        dialog.setOption(QFileDialog::DontUseNativeDialog,
                         VAbstractApplication::VApp()->Settings()->IsDontUseNativeDialog());

        QString mPath;
        if (dialog.exec() == QDialog::Accepted)
        {
            mPath = dialog.selectedFiles().value(0);
        }

        return mPath;
    };

    QFileInfo const table(path);
    if (table.exists())
    {
        return path;
    }

    if (not VApplication::IsGUIMode())
    {
        return {}; // console mode doesn't support fixing path to a measurement file
    }

    const QString text = tr("The measurements file <br/><br/> <b>%1</b> <br/><br/> could not be found. Do you "
                            "want to update the file location?")
                             .arg(path);

    if (QMessageBox::StandardButton const res = QMessageBox::question(
            this, tr("Loading measurements file"), text, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        res == QMessageBox::No)
    {
        return {};
    }

    MeasurementsType patternType;
    if (table.suffix() == "omt"_L1 || table.suffix() == "vst"_L1)
    {
        patternType = MeasurementsType::Multisize;
    }
    else if (table.suffix() == "otu"_L1 || table.suffix() == "vit"_L1)
    {
        patternType = MeasurementsType::Individual;
    }
    else
    {
        patternType = MeasurementsType::Unknown;
    }

    auto DirPath = [patternPath, table](const QString &defPath, QString &selectedName)
    {
        QString dirPath;
        const QDir patternDir = QFileInfo(patternPath).absoluteDir();
        if (QString measurements = table.fileName(); patternDir.exists(measurements))
        {
            selectedName = measurements;
            dirPath = patternDir.absolutePath();
        }
        else if (patternDir.exists(measurements.replace(' '_L1, '_'_L1)))
        {
            selectedName = measurements.replace(' '_L1, '_'_L1);
            dirPath = patternDir.absolutePath();
        }
        else
        {
            dirPath = defPath;
        }
        return dirPath;
    };

    QString mPath;
    if (patternType == MeasurementsType::Multisize)
    {
        const QString filter =
            tr("Multisize measurements") + " (*.omt *.vst);;"_L1 + tr("Individual measurements") + " (*.otu *.vit)"_L1;
        // Use standard path to multisize measurements
        QString selectedName;
        const QString dirPath =
            DirPath(VAbstractValApplication::VApp()->ValentinaSettings()->GetPathMultisizeMeasurements(), selectedName);
        mPath = FindLocation(filter, dirPath, selectedName);
        if (!mPath.isEmpty())
        {
            VAbstractValApplication::VApp()->ValentinaSettings()->SetPathMultisizeMeasurements(mPath);
        }
    }
    else
    {
        const QString filter =
            tr("Individual measurements") + " (*.otu *.vit);;"_L1 + tr("Multisize measurements") + " (*.omt *.vst)"_L1;
        // Use standard path to individual measurements
        QString selectedName;
        const QString dirPath = DirPath(
            VAbstractValApplication::VApp()->ValentinaSettings()->GetPathIndividualMeasurements(), selectedName);
        mPath = FindLocation(filter, dirPath, selectedName);
        if (!mPath.isEmpty())
        {
            VAbstractValApplication::VApp()->ValentinaSettings()->SetPathIndividualMeasurements(mPath);
        }
    }

    if (mPath.isEmpty())
    {
        return mPath;
    }

    QScopedPointer<VMeasurements> const m(new VMeasurements(pattern));
    m->setXMLContent(mPath);

    patternType = m->Type();

    if (patternType == MeasurementsType::Unknown)
    {
        throw VException(tr("Measurement file has unknown format."));
    }

    VAbstractValApplication::VApp()->SetMeasurementsType(patternType);
    doc->SetMPath(RelativeMPath(patternPath, mPath));
    return mPath;
}


//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::UpdateMeasurements(const QSharedPointer<VMeasurements> &mFile, qreal baseA, qreal baseB, qreal baseC,
                                    bool rebuildGradation) -> bool
{
    if (mFile.isNull())
    {
        qCWarning(vMainWindow, "UpdateMeasurements called with null measurement file; keeping existing data.");
        return false;
    }

    m_m = mFile;

    if (VAbstractValApplication::VApp()->GetMeasurementsType() != m_m->Type())
    {
        qCCritical(vMainWindow, "%s", qUtf8Printable(tr("Measurement files types have not match.")));
        if (not VApplication::IsGUIMode())
        {
            QCoreApplication::exit(V_EX_DATAERR);
        }
        return false;
    }

    try
    {
        ReadMeasurements(baseA, baseB, baseC, rebuildGradation);
    }
    catch (VExceptionEmptyParameter &e)
    {
        qCCritical(vMainWindow, "%s\n\n%s\n\n%s", qUtf8Printable(tr("File error.")), qUtf8Printable(e.ErrorMessage()),
                   qUtf8Printable(e.DetailedInformation()));
        if (not VApplication::IsGUIMode())
        {
            QCoreApplication::exit(V_EX_NOINPUT);
        }
        return false;
    }

    return true;
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::StoreMultisizeMDimension(const QList<MeasurementDimension_p> &dimensions, int index, qreal currentBase)
{
    if (dimensions.size() > index)
    {
        const MeasurementDimension_p &dimension = dimensions.at(index);
        const DimesionLabels labels = dimension->Labels();

        switch (dimension->Type())
        {
            case MeasurementDimension::X:
                VAbstractValApplication::VApp()->SetDimensionHeight(currentBase);
                VAbstractValApplication::VApp()->SetDimensionHeightLabel(
                    VFuzzyValue(labels, currentBase, QString::number(currentBase)));
                break;
            case MeasurementDimension::Y:
            {
                const bool fc = m_m->IsFullCircumference();
                VAbstractValApplication::VApp()->SetDimensionSize(fc ? currentBase * 2 : currentBase);
                VAbstractValApplication::VApp()->SetDimensionSizeLabel(
                    VFuzzyValue(labels, currentBase, QString::number(fc ? currentBase * 2 : currentBase)));
                const bool measurement = dimension->IsBodyMeasurement();
                VAbstractValApplication::VApp()->SetDimensionSizeUnits(measurement ? m_m->Units()
                                                                                   : Unit::LAST_UNIT_DO_NOT_USE);
                break;
            }
            case MeasurementDimension::W:
            {
                const bool fc = m_m->IsFullCircumference();
                VAbstractValApplication::VApp()->SetDimensionWaist(fc ? currentBase * 2 : currentBase);
                VAbstractValApplication::VApp()->SetDimensionWaistLabel(
                    VFuzzyValue(labels, currentBase, QString::number(fc ? currentBase * 2 : currentBase)));
                break;
            }
            case MeasurementDimension::Z:
            {
                const bool fc = m_m->IsFullCircumference();
                VAbstractValApplication::VApp()->SetDimensionHip(fc ? currentBase * 2 : currentBase);
                VAbstractValApplication::VApp()->SetDimensionHipLabel(
                    VFuzzyValue(labels, currentBase, QString::number(fc ? currentBase * 2 : currentBase)));
                break;
            }
            default:
                break;
        }
    }
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::StoreIndividualMDimension(const QMap<QString, QSharedPointer<VMeasurement>> &measurements, IMD type)
{
    const QString name = VAbstractApplication::VApp()->TrVars()->VarToUser(m_m->MeasurementForDimension(type));
    const bool valid = not name.isEmpty() && measurements.contains(name);
    const qreal value = valid ? *measurements.value(name)->GetValue() : 0;
    switch (type)
    {
        case IMD::X:
            VAbstractValApplication::VApp()->SetDimensionHeight(value);
            VAbstractValApplication::VApp()->SetDimensionHeightLabel(QString::number(value));
            break;
        case IMD::Y:
            VAbstractValApplication::VApp()->SetDimensionSize(value);
            VAbstractValApplication::VApp()->SetDimensionSizeLabel(QString::number(value));
            break;
        case IMD::W:
            VAbstractValApplication::VApp()->SetDimensionWaist(value);
            VAbstractValApplication::VApp()->SetDimensionWaistLabel(QString::number(value));
            break;
        case IMD::Z:
            VAbstractValApplication::VApp()->SetDimensionHip(value);
            VAbstractValApplication::VApp()->SetDimensionHipLabel(QString::number(value));
            break;
        case IMD::N:
        default:
            break;
    }
}


//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::DimensionRestrictedValues(int index, const MeasurementDimension_p &dimension) -> QVector<qreal>
{
    VDimensionRestriction restriction;
    if (index == 0)
    {
        restriction = m_m->Restriction(0);
    }
    else if (index == 1)
    {
        restriction = m_m->Restriction(m_currentDimensionA);
    }
    else
    {
        restriction = m_m->Restriction(m_currentDimensionA, m_currentDimensionB);
    }

    const QVector<qreal> bases = dimension->ValidBases();

    qreal min = VFuzzyIndexOf(bases, restriction.GetMin()) != -1 ? restriction.GetMin() : dimension->MinValue();
    qreal max = VFuzzyIndexOf(bases, restriction.GetMax()) != -1 ? restriction.GetMax() : dimension->MaxValue();

    if (min > max)
    {
        min = dimension->MinValue();
        max = dimension->MaxValue();
    }

    return VAbstartMeasurementDimension::ValidBases(min, max, dimension->Step(), restriction.GetExcludeValues());
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::InitDimensionGradation(int index, const MeasurementDimension_p &dimension,
                                        const QPointer<QComboBox> &control)
{
    SCASSERT(control != nullptr)

    qreal current = -1;
    if (control->currentIndex() != -1)
    {
        current = control->currentData().toDouble();
    }

    QSignalBlocker blocker(control);
    control->clear();

    const QVector<qreal> bases = DimensionRestrictedValues(index, dimension);
    const DimesionLabels labels = dimension->Labels();

    if (dimension->Type() == MeasurementDimension::X)
    {
        InitDimensionXGradation(bases, labels, control);
    }
    else if (dimension->Type() == MeasurementDimension::Y || dimension->Type() == MeasurementDimension::W ||
             dimension->Type() == MeasurementDimension::Z)
    {
        InitDimensionYWZGradation(bases, labels, control, dimension->IsBodyMeasurement());
    }

    // Calculate the width of the largest item using QFontMetrics
    QFontMetrics const fontMetrics(control->font());
    int maxWidth = 0;
    for (int i = 0; i < control->count(); ++i)
    {
        int const itemWidth = fontMetrics.horizontalAdvance(control->itemText(i));
        if (itemWidth > maxWidth)
        {
            maxWidth = itemWidth;
        }
    }

    // Set the minimum width of the view to the largest item width
    control->view()->setMinimumWidth(maxWidth);

    // after initialization the current index is 0. The signal for changing the index will not be triggered if not make
    // it invalid first
    control->setCurrentIndex(-1);

    int const i = control->findData(current);
    if (i != -1)
    {
        control->setCurrentIndex(i);
        blocker.unblock();
    }
    else
    {
        blocker.unblock();
        control->setCurrentIndex(0);
    }
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::InitDimensionXGradation(const QVector<qreal> &bases, const DimesionLabels &labels,
                                         const QPointer<QComboBox> &control)
{
    const QString unit = UnitsToStr(VAbstractValApplication::VApp()->MeasurementsUnits(), true);

    for (auto base : bases)
    {
        if (VFuzzyContains(labels, base) && not VFuzzyValue(labels, base).isEmpty())
        {
            control->addItem(VFuzzyValue(labels, base), base);
        }
        else
        {
            control->addItem(QStringLiteral("%1 %2").arg(base).arg(unit), base);
        }
    }
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::InitDimensionYWZGradation(const QVector<qreal> &bases, const DimesionLabels &labels,
                                           const QPointer<QComboBox> &control, bool bodyMeasurement)
{
    const bool fc = m_m->IsFullCircumference();
    const QString unit = UnitsToStr(VAbstractValApplication::VApp()->MeasurementsUnits(), true);

    for (auto base : bases)
    {
        if (VFuzzyContains(labels, base) && not VFuzzyValue(labels, base).isEmpty())
        {
            control->addItem(VFuzzyValue(labels, base), base);
        }
        else
        {
            if (bodyMeasurement)
            {
                control->addItem(QStringLiteral("%1 %2").arg(fc ? base * 2 : base).arg(unit), base);
            }
            else
            {
                control->addItem(QString::number(base), base);
            }
        }
    }
}


//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::SetDimensionA(int value) -> bool
{
    if (not VApplication::IsGUIMode())
    {
        if (this->isWindowModified() || not VAbstractValApplication::VApp()->GetPatternPath().isEmpty())
        {
            if (VAbstractValApplication::VApp()->GetMeasurementsType() == MeasurementsType::Multisize)
            {
                const qint32 index = m_dimensionA ? m_dimensionA->findData(value) : -1;
                if (index != -1)
                {
                    m_dimensionA->setCurrentIndex(index);
                }
                else
                {
                    qCCritical(
                        vMainWindow, "%s",
                        qUtf8Printable(tr("Not supported dimension A value '%1' for this pattern file.").arg(value)));
                    return false;
                }
            }
            else
            {
                qCCritical(vMainWindow, "%s",
                           qUtf8Printable(tr("Couldn't set dimension A. Need a file with multisize measurements.")));
                return false;
            }
        }
        else
        {
            qCCritical(vMainWindow, "%s", qUtf8Printable(tr("Couldn't set dimension A. File wasn't opened.")));
            return false;
        }
    }
    else
    {
        qCWarning(vMainWindow, "%s", qUtf8Printable(tr("The method %1 does nothing in GUI mode").arg(Q_FUNC_INFO)));
        return false;
    }
    return true;
}


//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::SetDimensionB(int value) -> bool
{
    if (not VApplication::IsGUIMode())
    {
        if (this->isWindowModified() || not VAbstractValApplication::VApp()->GetPatternPath().isEmpty())
        {
            if (VAbstractValApplication::VApp()->GetMeasurementsType() == MeasurementsType::Multisize)
            {
                const qint32 index = m_dimensionB ? m_dimensionB->findData(value) : -1;
                if (index != -1)
                {
                    m_dimensionB->setCurrentIndex(index);
                }
                else
                {
                    qCCritical(
                        vMainWindow, "%s",
                        qUtf8Printable(tr("Not supported dimension B value '%1' for this pattern file.").arg(value)));
                    return false;
                }
            }
            else
            {
                qCCritical(vMainWindow, "%s",
                           qUtf8Printable(tr("Couldn't set dimension B. Need a file with multisize measurements.")));
                return false;
            }
        }
        else
        {
            qCCritical(vMainWindow, "%s", qUtf8Printable(tr("Couldn't set dimension B. File wasn't opened.")));
            return false;
        }
    }
    else
    {
        qCWarning(vMainWindow, "%s", qUtf8Printable(tr("The method %1 does nothing in GUI mode").arg(Q_FUNC_INFO)));
        return false;
    }
    return true;
}


//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::SetDimensionC(int value) -> bool
{
    if (not VApplication::IsGUIMode())
    {
        if (this->isWindowModified() || not VAbstractValApplication::VApp()->GetPatternPath().isEmpty())
        {
            if (VAbstractValApplication::VApp()->GetMeasurementsType() == MeasurementsType::Multisize)
            {
                const qint32 index = m_dimensionC ? m_dimensionC->findData(value) : -1;
                if (index != -1)
                {
                    m_dimensionC->setCurrentIndex(index);
                }
                else
                {
                    qCCritical(
                        vMainWindow, "%s",
                        qUtf8Printable(tr("Not supported dimension C value '%1' for this pattern file.").arg(value)));
                    return false;
                }
            }
            else
            {
                qCCritical(vMainWindow, "%s",
                           qUtf8Printable(tr("Couldn't set dimension C. Need a file with multisize measurements.")));
                return false;
            }
        }
        else
        {
            qCCritical(vMainWindow, "%s", qUtf8Printable(tr("Couldn't set dimension C. File wasn't opened.")));
            return false;
        }
    }
    else
    {
        qCWarning(vMainWindow, "%s", qUtf8Printable(tr("The method %1 does nothing in GUI mode").arg(Q_FUNC_INFO)));
        return false;
    }
    return true;
}

