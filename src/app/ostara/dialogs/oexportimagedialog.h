/************************************************************************
 **
 **  @file   oexportimagedialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   23/09/2026
 **
 **  @brief  Export mode — where an image of the plan goes, in what format and at what size.
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

#ifndef OEXPORTIMAGEDIALOG_H
#define OEXPORTIMAGEDIALOG_H

#include "../oexportplanimage.h"
#include "../oexportplanimagewriter.h"
#include "../vmisc/def.h"

#include <QDialog>
#include <QString>

class QComboBox;
class QDialogButtonBox;
class QEvent;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

/**
 * @brief Where an image of the plan is written, in what format, and at what size.
 *
 * A separate dialog from the PDF's, and not an extra field in it: that one says « Two fields
 * and nothing else » in its own header, and meant it.
 *
 * **One length is typed, the other is computed.** The combo says which side of the plan the
 * figure measures; the field carries it; the line below shows the side that follows. There is
 * therefore no pair of fields to hold in agreement and no loop to break — which is the whole
 * reason the axis is asked for rather than guessed from whichever field was touched last.
 *
 * The plan is handed over already measured (OExportPlanImage::Plan), so this dialog holds no
 * scene, no store and no arithmetic of its own: it converts at its edges through
 * OExportUnitFormat and asks OExportPlanImage everything else.
 *
 * The resolution is shown for a PNG alone. An SVG's page IS a physical size — the millimetres
 * are the size — where a raster has only pixels, and the figure is what turns one into the
 * other.
 */
class OExportImageDialog : public QDialog
{
    Q_OBJECT

public:
    /// @param outputName the generation's name, offered as the file name.
    /// @param directory  where it opens; the folder last used, or Documents.
    /// @param plan       the plan the canvas is showing — this dialog's whole knowledge of it.
    explicit OExportImageDialog(const QString &outputName, const QString &directory,
                                const OExportPlanImage::Plan &plan, QWidget *parent = nullptr);

    /// @brief The file name, with the suffix the chosen format needs — added here if left off.
    auto FileName() const -> QString;

    /// @brief The folder, as typed or browsed to.
    auto Directory() const -> QString;

    /// @brief Folder and file name together.
    auto FullPath() const -> QString;

    auto Format() const -> OExportPlanImageWriter::Format;

    /// @brief Which side of the plan the typed length measures.
    auto Axis() const -> OExportPlanImage::Axis;

    /// @brief The physical length the user typed, in MILLIMETRES; 0 when it is not a number.
    ///
    /// OExportUnitFormat::Parse's own return, so the conversion happens at the edge and
    /// nothing inside this class does arithmetic on units.
    auto TargetMm() const -> qreal;

    /// @brief The resolution, in dots per inch — read for a PNG alone.
    auto Dpi() const -> qreal;

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void OnBrowse();
    void OnAccept();
    void OnFormatChanged();
    void OnAxisChanged();
    void OnUnitChanged();
    void OnSizeEdited();

private:
    void Retranslate();

    /// Re-read the field and the combos and say what follows from them: the plan's own figures,
    /// the two labels of the size rows, the computed side, and whether the resolution is asked
    /// for at all. One reader for every change, so that no two of them can be out of step.
    void RefreshDerived();

    OExportPlanImage::Plan m_plan{};

    /// The one length that is stored. The field, the unit and the computed side all derive
    /// from it, and it is in millimetres like every other length of the export mode.
    qreal m_sizeMm{0.0};

    /// The unit the field is read and written in — never what is stored.
    Unit m_unit{Unit::Cm};

    QLabel *m_nameLabel{nullptr};
    QLabel *m_pathLabel{nullptr};
    QLineEdit *m_nameEdit{nullptr};
    QLineEdit *m_pathEdit{nullptr};
    QPushButton *m_browseButton{nullptr};

    QLabel *m_formatLabel{nullptr};
    QComboBox *m_formatCombo{nullptr};

    QLabel *m_planLabel{nullptr};
    QLabel *m_planValue{nullptr};

    QLabel *m_axisLabel{nullptr};
    QComboBox *m_axisCombo{nullptr};

    QLabel *m_sizeLabel{nullptr};
    QLineEdit *m_sizeEdit{nullptr};
    QComboBox *m_unitCombo{nullptr};

    QLabel *m_otherLabel{nullptr};
    QLabel *m_otherValue{nullptr};

    QLabel *m_dpiLabel{nullptr};
    QSpinBox *m_dpiSpin{nullptr};

    QDialogButtonBox *m_buttonBox{nullptr};
};

#endif // OEXPORTIMAGEDIALOG_H
