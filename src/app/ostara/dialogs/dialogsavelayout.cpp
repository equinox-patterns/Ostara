/************************************************************************
 **
 **  @file   dialogsavelayout.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   22 1, 2015
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
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

#include "dialogsavelayout.h"
#include "../core/vapplication.h"
#include "vexception.h"
#include "../vlayout/vlayoutexporter.h"
#include "../vmisc/vvalentinasettings.h"
#include "vmisc/FactoryWidgets.h"
#include "vtheme.h"
#include "ui_dialogsavelayout.h"

#include <QDir>
#include <QFileDialog>
#include <QGlobalStatic>
#include <QMessageBox>
#include <QProcess>
#include <QRegularExpression>
#include <QtDebug>

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

namespace
{
QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wunused-member-function")

#ifndef Q_OS_WIN
Q_GLOBAL_STATIC_WITH_ARGS(const QString, baseFilenameRegExp, ("^[^\\/]+$"_L1)) // NOLINT
#else
Q_GLOBAL_STATIC_WITH_ARGS(const QString, baseFilenameRegExp, ("^[^\\:?\"*|\\/<>]+$"_L1)) // NOLINT
#endif

QT_WARNING_POP
} // namespace

//---------------------------------------------------------------------------------------------------------------------
DialogSaveLayout::DialogSaveLayout(int count, const QString &fileName, QWidget *parent)
  : VAbstractLayoutDialog(parent),
    ui(new Ui::DialogSaveLAyout),
    m_count(count)
{
    ui->setupUi(this);

#if defined(Q_OS_MAC)
    setWindowFlags(Qt::Window);
#endif


    VAbstractApplication::VApp()->Settings()->GetOsSeparator() ? setLocale(QLocale()) : setLocale(QLocale::c());

    QPushButton *bOk = ui->buttonBox->button(QDialogButtonBox::Ok);
    SCASSERT(bOk != nullptr)
    bOk->setEnabled(false);

    ui->lineEditFileName->setValidator(new QRegularExpressionValidator(QRegularExpression(*baseFilenameRegExp), this));

    if (VApplication::IsGUIMode())
    {
        ui->lineEditFileName->setText(fileName);
    }
    else
    {
        if (QRegularExpression(*baseFilenameRegExp).match(fileName).hasMatch())
        {
            ui->lineEditFileName->setText(fileName);
        }
        else
        {
            throw VException(tr("The base filename does not match a regular expression."));
        }
    }

    // The combos come from the .ui, so they take ApplyComboBoxView instead of the MakeComboBox factory:
    // both give the combo the application's own popup view, because the native macOS popup ignores the
    // stylesheet (Modifications/Ostara/rules.md §3.4).
    ApplyComboBoxView(ui->comboBoxFormat);
    ApplyComboBoxView(ui->comboBoxFormatType);
    ApplyComboBoxView(ui->comboBoxDxfCompatibility);

    InitFileFormats();
    connect(ui->comboBoxFormatType,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &DialogSaveLayout::ShowExample);
    connect(ui->comboBoxFormat,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this](int index)
            {
                if (index == -1)
                {
                    ui->comboBoxFormatType->clear();
                    return;
                }

                {
                    const QSignalBlocker blocker(ui->comboBoxFormatType);
                    InitFileFormatTypes(static_cast<LayoutExportFileFormat>(ui->comboBoxFormat->currentData().toInt()));
                    ui->comboBoxFormatType->setCurrentIndex(-1);
                }

                ui->comboBoxFormatType->setCurrentIndex(0);
            });

    InitDxfCompatibility();

    connect(bOk, &QPushButton::clicked, this, &DialogSaveLayout::Save);
    connect(ui->lineEditFileName, &QLineEdit::textChanged, this, &DialogSaveLayout::ShowExample);
    connect(ui->pushButtonBrowse, &QPushButton::clicked, this,
            [this]()
            {
                VValentinaSettings  const*settings = VAbstractValApplication::VApp()->ValentinaSettings();
                const QString dirPath = settings->GetPathLayoutExport();

                const QString dir = QFileDialog::getExistingDirectory(
                    this, tr("Select folder"), dirPath,
                    VAbstractApplication::VApp()->NativeFileDialog(QFileDialog::ShowDirsOnly |
                                                                   QFileDialog::DontResolveSymlinks));
                if (not dir.isEmpty())
                { // If paths equal the signal will not be called, we will do this manually
                    dir == ui->lineEditPath->text() ? PathChanged(dir) : ui->lineEditPath->setText(dir);

                    settings->SetPathLayoutExport(dir);
                }
            });
    connect(ui->lineEditPath, &QLineEdit::textChanged, this, &DialogSaveLayout::PathChanged);

    ui->lineEditPath->setText(VAbstractValApplication::VApp()->ValentinaSettings()->GetPathLayoutExport());

    connect(ui->toolButtonScaleConnected, &QToolButton::clicked, this, &DialogSaveLayout::ToggleScaleConnection);
    UpdateScaleConnectionIcon();

    connect(ui->doubleSpinBoxHorizontalScale, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &DialogSaveLayout::HorizontalScaleChanged);
    connect(ui->doubleSpinBoxVerticalScale, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &DialogSaveLayout::VerticalScaleChanged);

    ReadSettings();

    ShowExample(); // Show example for current format.
}

//---------------------------------------------------------------------------------------------------------------------

void DialogSaveLayout::SelectFormat(LayoutExportFormats format)
{
    if (static_cast<int>(format) < 0 || format >= LayoutExportFormats::COUNT)
    {
        throw VException(tr("Tried to use out of range file format type number."));
    }

    const LayoutExportFileFormat fileFormat = VLayoutExporter::LayoutExportFileFormat(format);

    int i = ui->comboBoxFormat->findData(static_cast<int>(fileFormat));
    if (i < 0)
    {
        throw VException(tr("Selected not present file format."));
    }

    {
        const QSignalBlocker blocker(ui->comboBoxFormat);
        ui->comboBoxFormat->setCurrentIndex(i);
    }

    {
        const QSignalBlocker blocker(ui->comboBoxFormatType);
        InitFileFormatTypes(fileFormat);
        ui->comboBoxFormatType->setCurrentIndex(-1);
    }

    i = ui->comboBoxFormatType->findData(static_cast<int>(format));
    if (i < 0)
    {
        throw VException(tr("Selected not present file format type."));
    }
    ui->comboBoxFormatType->setCurrentIndex(i);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSaveLayout::SetBinaryDXFFormat(bool binary)
{
    switch (Format())
    {
        case LayoutExportFormats::DXF_AC1006_Flat:
        case LayoutExportFormats::DXF_AC1009_Flat:
        case LayoutExportFormats::DXF_AC1012_Flat:
        case LayoutExportFormats::DXF_AC1014_Flat:
        case LayoutExportFormats::DXF_AC1015_Flat:
        case LayoutExportFormats::DXF_AC1018_Flat:
        case LayoutExportFormats::DXF_AC1021_Flat:
        case LayoutExportFormats::DXF_AC1024_Flat:
        case LayoutExportFormats::DXF_AC1027_Flat:
        case LayoutExportFormats::DXF_AAMA:
        case LayoutExportFormats::DXF_ASTM:
            ui->checkBoxBinaryDXF->setChecked(binary);
            break;
        default:
            ui->checkBoxBinaryDXF->setChecked(false);
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSaveLayout::IsBinaryDXFFormat() const -> bool
{
    switch (Format())
    {
        case LayoutExportFormats::DXF_AC1006_Flat:
        case LayoutExportFormats::DXF_AC1009_Flat:
        case LayoutExportFormats::DXF_AC1012_Flat:
        case LayoutExportFormats::DXF_AC1014_Flat:
        case LayoutExportFormats::DXF_AC1015_Flat:
        case LayoutExportFormats::DXF_AC1018_Flat:
        case LayoutExportFormats::DXF_AC1021_Flat:
        case LayoutExportFormats::DXF_AC1024_Flat:
        case LayoutExportFormats::DXF_AC1027_Flat:
        case LayoutExportFormats::DXF_AAMA:
        case LayoutExportFormats::DXF_ASTM:
            return ui->checkBoxBinaryDXF->isChecked();
        default:
            return false;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSaveLayout::DxfCompatibility() const -> DXFApparelCompatibility
{
    switch (Format())
    {
        case LayoutExportFormats::DXF_AAMA:
        case LayoutExportFormats::DXF_ASTM:
            return static_cast<DXFApparelCompatibility>(ui->comboBoxDxfCompatibility->currentData().toInt());
        default:
            return DXFApparelCompatibility::STANDARD;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSaveLayout::SetDxfCompatibility(DXFApparelCompatibility type)
{
    switch (Format())
    {
        case LayoutExportFormats::DXF_AAMA:
        case LayoutExportFormats::DXF_ASTM:
        {
            if (static_cast<int>(type) < 0 || type >= DXFApparelCompatibility::COUNT)
            {
                break;
            }

            const int i = ui->comboBoxDxfCompatibility->findData(static_cast<int>(type));
            if (i < 0)
            {
                break;
            }
            ui->comboBoxDxfCompatibility->setCurrentIndex(i);
            break;
        }
        default:
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSaveLayout::IsShowMainPath() const -> bool
{
    return ui->checkBoxShowMainPath->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSaveLayout::SetShowMainPath(bool value)
{
    ui->checkBoxShowMainPath->setChecked(value);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSaveLayout::SetShowGrainline(bool show)
{
    switch (Format())
    {
        case LayoutExportFormats::DXF_AC1006_Flat:
        case LayoutExportFormats::DXF_AC1009_Flat:
        case LayoutExportFormats::DXF_AC1012_Flat:
        case LayoutExportFormats::DXF_AC1014_Flat:
        case LayoutExportFormats::DXF_AC1015_Flat:
        case LayoutExportFormats::DXF_AC1018_Flat:
        case LayoutExportFormats::DXF_AC1021_Flat:
        case LayoutExportFormats::DXF_AC1024_Flat:
        case LayoutExportFormats::DXF_AC1027_Flat:
        case LayoutExportFormats::SVG:
        case LayoutExportFormats::PDF:
        case LayoutExportFormats::PDFTiled:
        case LayoutExportFormats::PNG:
        case LayoutExportFormats::PS:
        case LayoutExportFormats::EPS:
        case LayoutExportFormats::TIF:
        case LayoutExportFormats::HPGL:
        case LayoutExportFormats::HPGL2:
        case LayoutExportFormats::HPGL_PLT:
        case LayoutExportFormats::HPGL2_PLT:
            ui->checkBoxShowGrainline->setChecked(show);
            break;
        default:
            ui->checkBoxShowGrainline->setChecked(true);
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSaveLayout::IsShowGrainline() const -> bool
{
    switch (Format())
    {
        case LayoutExportFormats::DXF_AC1006_Flat:
        case LayoutExportFormats::DXF_AC1009_Flat:
        case LayoutExportFormats::DXF_AC1012_Flat:
        case LayoutExportFormats::DXF_AC1014_Flat:
        case LayoutExportFormats::DXF_AC1015_Flat:
        case LayoutExportFormats::DXF_AC1018_Flat:
        case LayoutExportFormats::DXF_AC1021_Flat:
        case LayoutExportFormats::DXF_AC1024_Flat:
        case LayoutExportFormats::DXF_AC1027_Flat:
        case LayoutExportFormats::SVG:
        case LayoutExportFormats::PDF:
        case LayoutExportFormats::PDFTiled:
        case LayoutExportFormats::PNG:
        case LayoutExportFormats::PS:
        case LayoutExportFormats::EPS:
        case LayoutExportFormats::TIF:
        case LayoutExportFormats::HPGL:
        case LayoutExportFormats::HPGL2:
        case LayoutExportFormats::HPGL_PLT:
        case LayoutExportFormats::HPGL2_PLT:
            return ui->checkBoxShowGrainline->isChecked();
        default:
            return true;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSaveLayout::MakeHelpFormatList() -> QString
{
    auto out = QStringLiteral("\n");
    const auto formats = VLayoutExporter::AllLayoutExportFormats();
    for (int i = 0; i < formats.size(); ++i)
    {
        out += "\t* "_L1 + VLayoutExporter::ExportFormatDescription(formats.at(i)) + " = "_L1
               + QString::number(static_cast<int>(formats.at(i)));

        if (i < formats.size() - 1)
        {
            out += ",\n"_L1;
        }
        else
        {
            out += ".\n"_L1;
        }
    }
    return out;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSaveLayout::MakeHelpDxfApparelCompatibilityList() -> QString
{
    QString out("\n"_L1);
    out += QStringLiteral("\t* %1 = %2,\n")
               .arg(tr("By standard"))
               .arg(static_cast<int>(DXFApparelCompatibility::STANDARD));
    out += QStringLiteral("\t* Richpeace CAD V8 = %1,\n").arg(static_cast<int>(DXFApparelCompatibility::RPCADV08));
    out += QStringLiteral("\t* Richpeace CAD V9 = %1,\n").arg(static_cast<int>(DXFApparelCompatibility::RPCADV09));
    out += QStringLiteral("\t* Richpeace CAD V10 = %1.\n").arg(static_cast<int>(DXFApparelCompatibility::RPCADV10));
    out += QStringLiteral("\t* Clo3D/Style3D = %1.\n").arg(static_cast<int>(DXFApparelCompatibility::CLO3D));

    return out;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSaveLayout::SetDestinationPath(const QString &cmdDestinationPath)
{
    QString path;
    if (cmdDestinationPath.isEmpty())
    {
        path = QDir::currentPath();
    }
    else if (QDir(cmdDestinationPath).isAbsolute())
    {
        path = cmdDestinationPath;
    }
    else
    {
        QDir dir;
        if (not dir.cd(cmdDestinationPath))
        {
            throw VException(tr("The destination directory doesn't exists or is not readable."));
        }
        path = dir.absolutePath();
    }

    qDebug() << "Output full path: " << path << "\n";
    ui->lineEditPath->setText(path);
}

//---------------------------------------------------------------------------------------------------------------------
DialogSaveLayout::~DialogSaveLayout()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSaveLayout::Path() const -> QString
{
    return ui->lineEditPath->text();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSaveLayout::FileName() const -> QString
{
    const QString fileName = ui->lineEditFileName->text();
    return m_count > 1 && Format() != LayoutExportFormats::PDFTiled ? fileName + '_'_L1 : fileName;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSaveLayout::Format() const -> LayoutExportFormats
{
    return static_cast<LayoutExportFormats>(ui->comboBoxFormatType->currentData().toInt());
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSaveLayout::Save()
{
    WriteSettings();

    for (int i = 0; i < m_count; ++i)
    {
        QString name;

        if (m_count > 1)
        {
            name = Path() + '/' + FileName() + QString::number(i + 1) + VLayoutExporter::ExportFormatSuffix(Format());
        }
        else
        {
            name = Path() + '/' + FileName() + VLayoutExporter::ExportFormatSuffix(Format());
        }

        if (QFile::exists(name))
        {
            if (QMessageBox::StandardButton const res = QMessageBox::question(
                    this, tr("Name conflict"),
                    tr("Folder already contain file with name %1. Rewrite all conflict file names?").arg(name),
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
                res == QMessageBox::No)
            {
                reject();
                return;
            }

            break;
        }
    }

    if (QFile::exists(Path()))
    {
        VAbstractValApplication::VApp()->ValentinaSettings()->SetPathLayoutExport(Path());
    }

    accept();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSaveLayout::PathChanged(const QString &text)
{
    QPushButton *bOk = ui->buttonBox->button(QDialogButtonBox::Ok);
    SCASSERT(bOk != nullptr)

    QPalette palette = ui->lineEditPath->palette();

    QDir dir(text);
    dir.setPath(text);
    if (not text.isEmpty() && dir.exists(text))
    {
        bOk->setEnabled(true);
        const QPalette defaultPalette = QApplication::palette(ui->lineEditPath);
        palette.setColor(ui->lineEditPath->foregroundRole(), defaultPalette.color(QPalette::Text));
    }
    else
    {
        bOk->setEnabled(false);
        palette.setColor(ui->lineEditPath->foregroundRole(), Qt::red);
    }

    ui->lineEditPath->setPalette(palette);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSaveLayout::ShowExample()
{
    const bool hasValues = ui->comboBoxFormatType->currentIndex() != -1;
    ui->groupBoxFormatOptions->setEnabled(hasValues);
    ui->groupBoxScale->setEnabled(hasValues);

    QPushButton *bOk = ui->buttonBox->button(QDialogButtonBox::Ok);
    SCASSERT(bOk != nullptr)
    bOk->setEnabled(hasValues);

    if (!hasValues)
    {
        FitHeightToContent();
        return;
    }

    const LayoutExportFormats currentFormat = Format();
    QString example;

    if (m_count > 1 && currentFormat != LayoutExportFormats::PDFTiled)
    {
        example = tr("Example:") + FileName() + '1'_L1 + VLayoutExporter::ExportFormatSuffix(currentFormat);
    }
    else
    {
        example = tr("Example:") + FileName() + VLayoutExporter::ExportFormatSuffix(currentFormat);
    }
    ui->labelExample->setText(example);

    ui->labelOptionsNotAvailable->setVisible(false);
    ui->checkBoxBinaryDXF->setVisible(false);
    ui->checkBoxTextAsPaths->setVisible(false);
    ui->checkBoxTextAsPaths->setEnabled(true);
    ui->checkBoxShowGrainline->setVisible(false);
    ui->checkBoxTogetherWithNotches->setVisible(false);
    ui->labelDxfCompatibility->setVisible(false);
    ui->comboBoxDxfCompatibility->setVisible(false);
    ui->checkBoxShowMainPath->setVisible(false);

    switch (currentFormat)
    {
        case LayoutExportFormats::DXF_AAMA:
        case LayoutExportFormats::DXF_ASTM:
            ui->checkBoxBinaryDXF->setVisible(true);
            ui->checkBoxTogetherWithNotches->setVisible(true);
            ui->labelDxfCompatibility->setVisible(true);
            ui->comboBoxDxfCompatibility->setVisible(true);
            ui->checkBoxShowMainPath->setVisible(true);
            break;
        case LayoutExportFormats::PDFTiled:
            // Tiled PDF is out of the dialog's file formats (see InitFileFormats), so it is only reachable
            // from the command line: the page format, the margins and the orientation then come from the
            // settings, exactly like they used to come from the fields the dialog no longer carries.
            ui->checkBoxTextAsPaths->setVisible(true);
            ui->checkBoxShowGrainline->setVisible(true);
            ui->checkBoxTogetherWithNotches->setVisible(true);
            break;
        case LayoutExportFormats::HPGL:
        case LayoutExportFormats::HPGL2:
        case LayoutExportFormats::HPGL_PLT:
        case LayoutExportFormats::HPGL2_PLT:
            ui->checkBoxShowGrainline->setVisible(true);
            ui->checkBoxTogetherWithNotches->setVisible(true);
            break;
        case LayoutExportFormats::SVG:
        case LayoutExportFormats::PDF:
        case LayoutExportFormats::PS:
        case LayoutExportFormats::EPS:
        case LayoutExportFormats::PNG:
        case LayoutExportFormats::TIF:
            ui->checkBoxTextAsPaths->setVisible(true);
            ui->checkBoxShowGrainline->setVisible(true);
            ui->checkBoxTogetherWithNotches->setVisible(true);
            break;
        case LayoutExportFormats::DXF_AC1006_Flat:
        case LayoutExportFormats::DXF_AC1009_Flat:
        case LayoutExportFormats::DXF_AC1012_Flat:
        case LayoutExportFormats::DXF_AC1014_Flat:
        case LayoutExportFormats::DXF_AC1015_Flat:
        case LayoutExportFormats::DXF_AC1018_Flat:
        case LayoutExportFormats::DXF_AC1021_Flat:
        case LayoutExportFormats::DXF_AC1024_Flat:
        case LayoutExportFormats::DXF_AC1027_Flat:
            ui->checkBoxBinaryDXF->setVisible(true);
            ui->checkBoxTextAsPaths->setVisible(true);
            ui->checkBoxShowGrainline->setVisible(true);
            ui->checkBoxTogetherWithNotches->setVisible(true);
            break;
        default:
            ui->labelOptionsNotAvailable->setVisible(true);
            break;
    }

    FitHeightToContent();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSaveLayout::ToggleScaleConnection()
{
    m_scaleConnected = not m_scaleConnected;
    UpdateScaleConnectionIcon();
}

//---------------------------------------------------------------------------------------------------------------------
/// The proportions lock: a whole chain when the two scales are bound, a broken one when they are not.
void DialogSaveLayout::UpdateScaleConnectionIcon()
{
    ui->toolButtonScaleConnected->setIcon(
        VTheme::ThemedIcon(m_scaleConnected ? QStringLiteral("link-simple") : QStringLiteral("link-break")));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSaveLayout::HorizontalScaleChanged(double d)
{
    if (m_scaleConnected)
    {
        const QSignalBlocker blocker(ui->doubleSpinBoxVerticalScale);
        ui->doubleSpinBoxVerticalScale->setValue(d);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSaveLayout::VerticalScaleChanged(double d)
{
    if (m_scaleConnected)
    {
        const QSignalBlocker blocker(ui->doubleSpinBoxHorizontalScale);
        ui->doubleSpinBoxHorizontalScale->setValue(d);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSaveLayout::IsTextAsPaths() const -> bool
{
    return ui->checkBoxTextAsPaths->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSaveLayout::SetTextAsPaths(bool textAsPaths)
{
    ui->checkBoxTextAsPaths->setChecked(textAsPaths);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSaveLayout::SetTiledExportMode(bool tiledExportMode)
{
    m_tiledExportMode = tiledExportMode;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSaveLayout::SetTiledMargins(QMarginsF margins)
{
    // The command line export is the only caller: the value is carried until the pages are generated.
    m_tiledMargins = margins;
    m_tiledMarginsSet = true;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSaveLayout::GetTiledMargins() const -> QMarginsF
{
    if (m_tiledMarginsSet)
    {
        return m_tiledMargins;
    }

    // The dialog carries no margin field any more: the margins are the ones remembered in the settings.
    return VAbstractValApplication::VApp()->ValentinaSettings()->GetTiledPDFMargins(Unit::Mm);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSaveLayout::SetTiledPageFormat(PaperSizeTemplate format)
{
    // The command line export is the only caller, exactly like SetTiledMargins.
    m_tiledPageFormat = format;
    m_tiledPageFormatSet = true;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSaveLayout::GetTiledPageFormat() const -> VAbstractLayoutDialog::PaperSizeTemplate
{
    if (m_tiledPageFormatSet)
    {
        return m_tiledPageFormat;
    }

    // The dialog carries no paper field any more: recognize the paper size remembered in the settings.
    VValentinaSettings  const*settings = VAbstractValApplication::VApp()->ValentinaSettings();
    const QSizeF size = RoundTemplateSize(settings->GetTiledPDFPaperWidth(Unit::Mm),
                                          settings->GetTiledPDFPaperHeight(Unit::Mm), Unit::Mm);

    const auto max = static_cast<int>(PaperSizeTemplate::Custom);
    for (int i = 0; i < max; ++i)
    {
        if (size == GetTemplateSize(static_cast<PaperSizeTemplate>(i), Unit::Mm))
        {
            return static_cast<PaperSizeTemplate>(i);
        }
    }

    return PaperSizeTemplate::A0;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSaveLayout::SetTiledPageOrientation(PageOrientation orientation)
{
    // The command line export is the only caller, exactly like SetTiledMargins.
    m_tiledOrientation = orientation;
    m_tiledOrientationSet = true;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSaveLayout::GetTiledPageOrientation() const -> PageOrientation
{
    if (m_tiledOrientationSet)
    {
        return m_tiledOrientation;
    }

    // The dialog carries no orientation button any more: use the one remembered in the settings.
    return VAbstractValApplication::VApp()->ValentinaSettings()->GetTiledPDFOrientation();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSaveLayout::SetXScale(qreal scale)
{
    ui->doubleSpinBoxHorizontalScale->setValue(scale * 100.);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSaveLayout::GetXScale() const -> qreal
{
    return ui->doubleSpinBoxHorizontalScale->value() / 100.;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSaveLayout::SetYScale(qreal scale)
{
    ui->doubleSpinBoxVerticalScale->setValue(scale * 100.);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSaveLayout::GetYScale() const -> qreal
{
    return ui->doubleSpinBoxVerticalScale->value() / 100.;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSaveLayout::SetBoundaryTogetherWithNotches(bool value)
{
    ui->checkBoxTogetherWithNotches->setChecked(value);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSaveLayout::IsBoundaryTogetherWithNotches() const -> bool
{
    return ui->checkBoxTogetherWithNotches->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSaveLayout::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    if (event->spontaneous())
    {
        return;
    }

    FitHeightToContent();
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief FitHeightToContent locks the height of the dialog on the height of the rows it shows. Those rows depend on
/// the file format (see ShowExample), so the height is recomputed every time they change: a height locked once and for
/// all left an empty strip under a format that showed fewer rows, and cut off the bottom of a taller one.
void DialogSaveLayout::FitHeightToContent()
{
    if (layout() == nullptr)
    {
        return;
    }

    // Showing or hiding a row marks only the innermost layout dirty: the layouts in between keep the height of
    // the previous state until the next full layout pass. Invalidate the whole tree and deliver the requests Qt
    // has already posted — without that, the window keeps the height of the format that showed the most rows,
    // which is exactly the empty strip this method exists to remove.
    for (QLayout *childLayout : findChildren<QLayout *>())
    {
        childLayout->invalidate();
    }
    QApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);

    layout()->activate();
    setFixedHeight(layout()->minimumSize().height());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ReadSettings reads the values of the variables needed for the save layout dialog, for instance
 * the file format of the previous export. Then sets the corresponding elements of the dialog to these values.
 */
void DialogSaveLayout::ReadSettings()
{
    VValentinaSettings  const*settings = VAbstractValApplication::VApp()->ValentinaSettings();

        try
    {
        SelectFormat(static_cast<LayoutExportFormats>(settings->GetDetailExportFormat()));
    }
    catch (VException &e)
    {
        qDebug() << qUtf8Printable(e.ErrorMessage());
        SelectFirstAvailableFormat();
    }


    SetDxfCompatibility(static_cast<DXFApparelCompatibility>(settings->GetDxfCompatibility()));
    SetShowMainPath(settings->IsShowMainPath());
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief SelectFirstAvailableFormat selects the first file format the dialog offers, with its first type. It is the
/// fallback when the format remembered in the settings is not offered any more (PDF...): without it the dialog
/// would open on an empty type list — which looks broken, and leaves the OK button disabled for no visible reason.
void DialogSaveLayout::SelectFirstAvailableFormat()
{
    if (ui->comboBoxFormat->count() == 0)
    {
        return;
    }

    const int index = ui->comboBoxFormat->currentIndex() < 0 ? 0 : ui->comboBoxFormat->currentIndex();

    {
        const QSignalBlocker blocker(ui->comboBoxFormat);
        ui->comboBoxFormat->setCurrentIndex(index);
        InitFileFormatTypes(static_cast<LayoutExportFileFormat>(ui->comboBoxFormat->itemData(index).toInt()));
    }

    ui->comboBoxFormatType->setCurrentIndex(0);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief WriteSettings writes the values of some variables (like the file format of the last export) of the save
 * layout dialog into the settings.
 */
void DialogSaveLayout::WriteSettings() const
{
    if (m_tiledExportMode)
    {
        return;
    }

    VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();

        settings->SetDetailExportFormat(static_cast<qint8>(Format()));


    settings->SetDxfCompatibility(static_cast<qint8>(DxfCompatibility()));
    settings->SetShowMainPath(IsShowMainPath());
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSaveLayout::InitDxfCompatibility()
{
    ui->comboBoxDxfCompatibility->addItem(tr("By standard"),
                                          QVariant(static_cast<int>(DXFApparelCompatibility::STANDARD)));
    ui->comboBoxDxfCompatibility->addItem("Richpeace CAD V8"_L1,
                                          QVariant(static_cast<int>(DXFApparelCompatibility::RPCADV08)));
    ui->comboBoxDxfCompatibility->addItem("Richpeace CAD V9"_L1,
                                          QVariant(static_cast<int>(DXFApparelCompatibility::RPCADV09)));
    ui->comboBoxDxfCompatibility->addItem("Richpeace CAD V10"_L1,
                                          QVariant(static_cast<int>(DXFApparelCompatibility::RPCADV10)));
    ui->comboBoxDxfCompatibility->addItem("Clo3D/Style3D"_L1,
                                          QVariant(static_cast<int>(DXFApparelCompatibility::CLO3D)));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSaveLayout::InitFileFormats()
{
    auto AddItem = [this](const QString &label, LayoutExportFileFormat format)
    { ui->comboBoxFormat->addItem(label, QVariant(static_cast<int>(format))); };

    Q_STATIC_ASSERT_X(static_cast<int>(LayoutExportFileFormat::COUNT) == 9, "Update to cover all cases.");

    // PDF is disconnected from the dialog: no one exports it from here any more. Its pipeline is left
    // untouched, and the console export — which uses this dialog as a plain data holder and never shows
    // it — keeps it, so nothing that worked from the command line stops working.
    const bool guiMode = VApplication::IsGUIMode();

    if (not guiMode)
    {
        AddItem("PDF"_L1, LayoutExportFileFormat::PDF);
    }

    AddItem("SVG"_L1, LayoutExportFileFormat::SVG);
    AddItem("PNG"_L1, LayoutExportFileFormat::PNG);
    // AddItem("OBJ"_L1, LayoutExportFileFormat::OBJ); // Temporarily unavailable
    AddItem("DXF"_L1, LayoutExportFileFormat::DXF);
    // AddItem("NC"_L1, LayoutExportFileFormat::NC);// No support for now
    AddItem("PLT"_L1, LayoutExportFileFormat::PLT);
    AddItem("HPGL"_L1, LayoutExportFileFormat::HPGL);
    AddItem("TIF"_L1, LayoutExportFileFormat::TIF);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSaveLayout::InitFileFormatTypes(LayoutExportFileFormat format)
{
    ui->comboBoxFormatType->clear();
    const QVector<LayoutExportFormats> types = VLayoutExporter::MapLayoutExportFormats(format);

    for (const auto &type : types)
    {
        ui->comboBoxFormatType->addItem(VLayoutExporter::ExportFormatDescription(type),
                                        QVariant(static_cast<int>(type)));
    }

    auto RemoveFormatFromList = [this](LayoutExportFormats exportFormat)
    {
        const int index = ui->comboBoxFormatType->findData(static_cast<int>(exportFormat));
        if (index != -1)
        {
            ui->comboBoxFormatType->removeItem(index);
        }
    };

    //    RemoveFormatFromList(LayoutExportFormats::NC); // No support for now

#ifdef V_NO_ASSERT // Temporarily unavailable
    RemoveFormatFromList(LayoutExportFormats::OBJ);
#endif

        RemoveFormatFromList(LayoutExportFormats::PDFTiled);

}
