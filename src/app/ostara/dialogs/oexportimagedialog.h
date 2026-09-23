/************************************************************************
 **
 **  @file   oexportimagedialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   23/09/2026
 **
 **  @brief  Export mode — where an image of the plan goes, and how big.
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
 * @brief The first step of an image export: the folder, the format, and the size.
 *
 * Everything here is about the FILE — where it lands, what it is, how big — and nothing about
 * which generations it is made of. That is the whole reason the name is not here: one name
 * belongs to one file, and this dialog does not yet know how many files there will be.
 *
 * **One length is typed, the other is computed.** The combo says which side of a plan the figure
 * measures; the field carries it; the line below shows the side that follows. There is therefore
 * no pair of fields to hold in agreement and no loop to break. It applies to every generation
 * exported: a plan's proportions are its own, so two generations asked for the same width come
 * out at two different heights — which is what a plan per generation wants.
 *
 * The plan is handed over already measured (OExportPlanImage::Plan) — the one shown in the canvas
 * — and it is only there for the figure the size opens on. A generation that is not on screen
 * brings its own when it is exported.
 */
class OExportImageDialog : public QDialog
{
    Q_OBJECT

public:
    /// What the export will be made of. Read from the button that was pressed, so that the
    /// two answers cannot be told apart by anything else.
    enum class Scope : qint8
    {
        Active, ///< the generation the canvas is showing, and no other
        All     ///< every generation the Outputs dock lists
    };

    /// @param directory where it opens; the folder last used, or Documents.
    /// @param plan      the plan of the generation on show — what the size field opens on.
    /// @param hasActive whether a generation is on show at all. Without one the first button is
    ///                  there but dead: a list with nothing to list is not a choice.
    explicit OExportImageDialog(const QString &directory, const OExportPlanImage::Plan &plan,
                                bool hasActive, QWidget *parent = nullptr);

    /// @brief The folder, as typed or browsed to. Shared by every file of the run.
    auto Directory() const -> QString;

    auto Format() const -> OExportPlanImageWriter::Format;

    /// @brief Which side of a plan the typed length measures.
    auto Axis() const -> OExportPlanImage::Axis;

    /// @brief The physical length the user typed, in MILLIMETRES; 0 when it is not a number.
    ///
    /// OExportUnitFormat::Parse's own return, so the conversion happens at the edge and nothing
    /// inside this class does arithmetic on units.
    auto TargetMm() const -> qreal;

    /// @brief The resolution, in dots per inch — read for a PNG alone.
    auto Dpi() const -> qreal;

    /// @brief Which button closed the dialog.
    auto Chosen() const -> Scope;

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void OnBrowse();
    void OnFormatChanged();
    void OnAxisChanged();
    void OnUnitChanged();
    void OnSizeEdited();

private:
    void Retranslate();

    /// Validate what the dialog holds, then close on @p scope. One place, so that both buttons
    /// refuse a size that could not be written for the same reasons and in the same words.
    void Choose(Scope scope);

    /// Re-read the field and the combos and say what follows from them: the plan's own figures,
    /// the two labels of the size rows, the computed side, and whether the resolution is asked
    /// for at all. One reader for every change, so that no two of them can be out of step.
    void RefreshDerived();

    OExportPlanImage::Plan m_plan{};
    Scope m_scope{Scope::Active};

    /// The one length that is stored. The field, the unit and the computed side all derive from
    /// it, and it is in millimetres like every other length of the export mode.
    qreal m_sizeMm{0.0};

    /// The unit the field is read and written in — never what is stored.
    Unit m_unit{Unit::Cm};

    QLabel *m_pathLabel{nullptr};
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
