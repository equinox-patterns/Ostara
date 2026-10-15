/************************************************************************
 **
 **  @file   oexportcuttingfileoptions.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/10/2026
 **
 **  @brief  Export mode — what a cutting file is written with: format, sub-type, options.
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

#include "oexportcuttingfileoptions.h"

#include "../vlayout/vlayoutexporter.h"
#include "../vmisc/FactoryWidgets.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vvalentinasettings.h"

#include <QCheckBox>
#include <QComboBox>
#include <QEvent>
#include <QGridLayout>
#include <QLabel>

//---------------------------------------------------------------------------------------------------------------------
OExportCuttingFileOptions::OExportCuttingFileOptions(QWidget *parent)
  : QWidget(parent)
{
    auto *grid = new QGridLayout(this);
    grid->setContentsMargins(0, 0, 0, 0);

    m_formatLabel = new QLabel(this);
    m_formatCombo = MakeComboBox(this);
    m_subTypeLabel = new QLabel(this);
    m_subTypeCombo = MakeComboBox(this);

    m_binaryCheck = new QCheckBox(this);
    m_textAsPathsCheck = new QCheckBox(this);
    m_grainlineCheck = new QCheckBox(this);
    m_grainlineCheck->setChecked(true);
    m_notchesCheck = new QCheckBox(this);
    m_mainPathCheck = new QCheckBox(this);
    m_mainPathCheck->setToolTip(tr("Exports the pieces as if their main path were visible: each piece then has both "
                                   "its cutting line and its sew line. Nothing is changed in the pattern."));
    m_compatibilityLabel = new QLabel(this);
    m_compatibilityCombo = MakeComboBox(this);

    grid->addWidget(m_formatLabel, 0, 0);
    grid->addWidget(m_formatCombo, 0, 1);
    grid->addWidget(m_subTypeLabel, 1, 0);
    grid->addWidget(m_subTypeCombo, 1, 1);
    grid->addWidget(m_binaryCheck, 2, 0, 1, 2);
    grid->addWidget(m_textAsPathsCheck, 3, 0, 1, 2);
    grid->addWidget(m_grainlineCheck, 4, 0, 1, 2);
    grid->addWidget(m_notchesCheck, 5, 0, 1, 2);
    grid->addWidget(m_mainPathCheck, 6, 0, 1, 2);
    grid->addWidget(m_compatibilityLabel, 7, 0);
    grid->addWidget(m_compatibilityCombo, 7, 1);

    // Les listes AVANT les connexions : peupler un combo change son index, et l'index annoncerait un
    // changement de format pendant la construction — c'est l'ordre de la dialogue du mode Details, et
    // pour la même raison.
    PopulateFileFormats();
    InitDxfCompatibility();
    PopulateSubTypes();

    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &OExportCuttingFileOptions::FileFormatChanged);
    connect(m_subTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &OExportCuttingFileOptions::SubTypeChanged);

    Retranslate();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingFileOptions::Options() const -> OExportCuttingFile::Options
{
    OExportCuttingFile::Options options;
    options.format = Format();

    // ⚠️ **La visibilité est la règle**, jamais la valeur cachée : une case qu'un format ne montre
    // pas ne doit pas voyager quand même, et un lecteur qui ne verrait que les valeurs ne pourrait
    // pas le deviner.
    options.binary = m_binaryCheck->isVisible() && m_binaryCheck->isChecked();
    options.textAsPaths = m_textAsPathsCheck->isVisible() && m_textAsPathsCheck->isChecked();
    options.togetherWithNotches = m_notchesCheck->isChecked();
    options.showMainPath = m_mainPathCheck->isVisible() && m_mainPathCheck->isChecked();

    // Le droit-fil est TOUJOURS écrit pour AAMA/ASTM — la case n'y est pas offerte, et c'est une
    // constante du format, pas un choix caché.
    options.showGrainline = !m_grainlineCheck->isVisible() || m_grainlineCheck->isChecked();

    options.compatibility = m_compatibilityCombo->isVisible()
                                ? static_cast<DXFApparelCompatibility>(m_compatibilityCombo->currentData().toInt())
                                : DXFApparelCompatibility::STANDARD;
    return options;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingFileOptions::Suffix() const -> QString
{
    return VLayoutExporter::ExportFormatSuffix(Format());
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingFileOptions::Format() const -> LayoutExportFormats
{
    return static_cast<LayoutExportFormats>(m_subTypeCombo->currentData().toInt());
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingFileOptions::PopulateFileFormats()
{
    auto AddItem = [this](const QString &label, LayoutExportFileFormat format)
    { m_formatCombo->addItem(label, QVariant(static_cast<int>(format))); };

    // Les trois formats d'un plan de coupe, et rien d'autre (CuttingPlan.md §24.1) : ni PDF, ni
    // image — l'image du plan est l'action « Export as image… », qui existe à côté.
    AddItem(QStringLiteral("DXF"), LayoutExportFileFormat::DXF);
    AddItem(QStringLiteral("PLT"), LayoutExportFileFormat::PLT);
    AddItem(QStringLiteral("HPGL"), LayoutExportFileFormat::HPGL);

    m_formatCombo->setCurrentIndex(0);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingFileOptions::PopulateSubTypes()
{
    const auto fileFormat = static_cast<LayoutExportFileFormat>(m_formatCombo->currentData().toInt());

    {
        // Le combo se remplit dans le dos de ses lecteurs : c'est le format qui décide ce qu'il
        // contient, et rien entre les deux ne doit se croire choisi.
        const QSignalBlocker blocker(m_subTypeCombo);
        m_subTypeCombo->clear();

        for (const LayoutExportFormats type : VLayoutExporter::MapLayoutExportFormats(fileFormat))
        {
            m_subTypeCombo->addItem(VLayoutExporter::ExportFormatDescription(type),
                                    QVariant(static_cast<int>(type)));
        }

        m_subTypeCombo->setCurrentIndex(0);
    }

    ShowOptionsForFormat();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingFileOptions::FileFormatChanged()
{
    PopulateSubTypes();
    emit FormatChanged();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingFileOptions::SubTypeChanged()
{
    ShowOptionsForFormat();
    emit FormatChanged();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingFileOptions::ShowOptionsForFormat()
{
    // Un combo vide n'a pas de format : le lire donnerait l'index 0, qui est un format qui n'existe
    // pas ici (`SVG`). Le cas n'arrive pas — les trois formats peuplent toujours leur liste — mais
    // c'est le genre de valeur par défaut qui écrit un `.svg` sans que personne ne l'ait demandé.
    if (m_subTypeCombo->count() == 0)
    {
        return;
    }

    const LayoutExportFormats format = Format();
    const LayoutExportFileFormat fileFormat = VLayoutExporter::LayoutExportFileFormat(format);

    const bool apparel = format == LayoutExportFormats::DXF_AAMA || format == LayoutExportFormats::DXF_ASTM;
    const bool dxf = fileFormat == LayoutExportFileFormat::DXF;
    const bool flat = dxf && !apparel;

    // ── Les options du mode Details, format par format ───────────────────────────────────────────
    //
    // Une option qui n'existe pas pour un format n'est pas offerte pour lui — c'est la règle de la
    // dialogue du mode Details, et la seule façon qu'un plan de coupe s'écrive comme les mêmes
    // pièces s'écriraient de là-bas (CuttingPlan.md §24.1).
    m_binaryCheck->setVisible(dxf);
    m_textAsPathsCheck->setVisible(flat);
    m_grainlineCheck->setVisible(!apparel);
    m_mainPathCheck->setVisible(apparel);
    m_compatibilityLabel->setVisible(apparel);
    m_compatibilityCombo->setVisible(apparel);

    // « Text as paths » est FORCÉ quand la police simple ligne est active : elle n'a pas de contour à
    // écrire. Même règle, même raison que dans la dialogue du mode Details.
    VValentinaSettings const *settings = VAbstractValApplication::VApp()->ValentinaSettings();
    const bool forcedPaths = settings->GetSingleLineFonts() || settings->GetSingleStrokeOutlineFont();

    m_textAsPathsCheck->setEnabled(!forcedPaths);
    if (forcedPaths)
    {
        m_textAsPathsCheck->setChecked(true);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingFileOptions::InitDxfCompatibility()
{
    // Les mêmes entrées que la dialogue du mode Details, dans le même ordre : c'est la même donnée
    // lue par le même moteur.
    m_compatibilityCombo->addItem(tr("By standard"), QVariant(static_cast<int>(DXFApparelCompatibility::STANDARD)));
    m_compatibilityCombo->addItem(QStringLiteral("Richpeace CAD V8"),
                                  QVariant(static_cast<int>(DXFApparelCompatibility::RPCADV08)));
    m_compatibilityCombo->addItem(QStringLiteral("Richpeace CAD V9"),
                                  QVariant(static_cast<int>(DXFApparelCompatibility::RPCADV09)));
    m_compatibilityCombo->addItem(QStringLiteral("Richpeace CAD V10"),
                                  QVariant(static_cast<int>(DXFApparelCompatibility::RPCADV10)));
    m_compatibilityCombo->addItem(QStringLiteral("Clo3D/Style3D"),
                                  QVariant(static_cast<int>(DXFApparelCompatibility::CLO3D)));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingFileOptions::Retranslate()
{
    m_formatLabel->setText(tr("File format:"));
    m_subTypeLabel->setText(tr("Type:"));
    m_binaryCheck->setText(tr("Binary form"));
    m_textAsPathsCheck->setText(tr("Text as paths"));
    m_grainlineCheck->setText(tr("Show grainline"));
    m_notchesCheck->setText(tr("Boundary together with notches"));
    m_mainPathCheck->setText(tr("Show the main path, even when the pieces hide it"));
    m_compatibilityLabel->setText(tr("Compatibility:"));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingFileOptions::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QWidget::changeEvent(event);
}
