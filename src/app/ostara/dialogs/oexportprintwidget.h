/************************************************************************
 **
 **  @file   oexportprintwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  The Print tab of the Export dialog — what to nest, and on which
 **          sheet it comes out.
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

#ifndef OEXPORTPRINTWIDGET_H
#define OEXPORTPRINTWIDGET_H

#include <QWidget>

#include "../vmisc/def.h"
#include "../core/ogradedsizeranges.h"

class OExportRollFormatWidget;
class OExportTiledFormatWidget;
class QComboBox;
class QLabel;
class QLineEdit;
class QStackedWidget;
class VContainer;
class VMeasurements;

/**
 * @brief The Print tab: a cascade, and the sheet it ends on.
 *
 * The tab asks two questions in order, and nothing else:
 *
 *   1. <b>what to nest</b> — a whole size range, or one size picked along the axes. The
 *      question disappears when there is only one answer to it: on a table with no axis
 *      to walk, a range has nothing to walk along, and offering the choice would promise
 *      a run that can only end on a warning.
 *   2. <b>how it comes out</b> — one of the print models. Picking one reveals its own
 *      fields and hides the others': the models differ by what they let the user set, and
 *      showing every field at once would ask about a column count on a model that has no
 *      columns.
 *
 * What the tab does NOT ask is the width the nesting runs on. That was the whole point of
 * the models: the user says which sheet she prints on, and the width follows from it —
 * from the model alone on a roll, from the model times the column count on a tiled sheet.
 * Asking her to type it meant asking her to know, before the nesting had run, whether the
 * plan would fit on the paper she had.
 *
 * Every length is collected in millimetres and shown in the unit the combo picks. The
 * unit is the tab's, not the pattern's: the sheet is a standard in millimetres before it
 * is anything on screen, and a pattern measured in inches does not make A4 an inch size.
 */
class OExportPrintWidget : public QWidget
{
    Q_OBJECT

public:
    /// @param context the (measurements table, declared grading axis) the pattern is on —
    ///                the same filter the Graded dock lists its ranges by.
    explicit OExportPrintWidget(const VContainer *data, const QSharedPointer<VMeasurements> &measurements,
                                const OGradedSizeRanges::Context &context, QWidget *parent = nullptr);

    /// @brief The width the nesting runs on, in millimetres — the tab's one output that
    ///        is not simply a field: on a tiled model it is the sheet times the columns.
    auto DocumentWidthMm() const -> qreal;

    auto MarginMm() const -> qreal;
    auto GapMm() const -> qreal;

    /// @brief Key of the selected size range. Empty on the single-size path.
    auto SizeRangeName() const -> QString;

    /// @brief True when one size is nested rather than a whole range.
    ///
    /// Spelled out rather than read off the combo: when the combo is not offered there is
    /// no combo to read, and a single size is exactly what the run then means.
    auto IsSingleSize() const -> bool;

    /// @brief Base of the axis in slot @p index (0 = X, 1 = Y, 2 = W); -1 when the table
    ///        carries no such axis, or the user picked nothing for it.
    auto DimensionBase(int index) const -> qreal;

    /// @brief What the axis in slot @p index reads as, empty when the table carries none.
    ///
    /// The label, not the base: a size is named by what the user sees in the combo, and
    /// the base underneath is a number the measurement table uses — it happens to read
    /// "160" on a stature, and reads nothing at all on an axis labelled "S".
    auto DimensionLabel(int index) const -> QString;

    /// @brief The token of the selected print model — what the generated name carries and
    ///        what the replay looks the model back up by.
    auto PrintModelToken() const -> QString;

    /**
     * @brief The template of assembly marks the run is drawn with, by its file name.
     *
     * Empty when the library holds none: the plan is then drawn alone, without marks, which is
     * what it was before there were templates at all.
     */
    auto MarksTemplateName() const -> QString;

    /**
     * @brief The cover page the run carries, by its file name.
     *
     * **Empty means no cover page** — nothing reserved in the bin, nothing drawn, no page added —
     * and it is the answer of an untouched combo as much as of one the user set to "No cover
     * page". One representation for the two, because they mean the same thing.
     */
    auto CoverPageTemplateName() const -> QString;

    /// @brief How many sheets side by side. Always 1 on a roll, which has no columns.
    auto Columns() const -> int;

    /// @brief The long side of one sheet, in millimetres — the height of the frame the plan
    ///        is cut into. **0 when there is no frame at all**, and that single figure is
    ///        what says so: it is the token the canvas reads to decide whether there are
    ///        sheets to draw.
    ///
    /// A roll has no sheet, and the A0 model is never placed side by side — neither is cut,
    /// so neither has a frame, so both answer zero here.
    auto FrameHeightMm() const -> qreal;

    /// @brief The unit the tab is read in. Stored with the generation so that the figures
    ///        come back in it, rather than in whatever the pattern happens to use.
    auto DisplayUnit() const -> Unit;

    /// @brief Whether the tab currently describes a run that can be asked for.
    auto IsValid() const -> bool;

signals:
    /// Anything the dialog's OK button depends on changed.
    void Changed();

protected:
    void changeEvent(QEvent *event) override;

private:
    void BuildUi();
    void PopulateSizeRanges();
    void PopulateDimensions();
    void PopulateModels();
    void PopulateMarks();
    void PopulateCoverPages();
    void UpdateSourceVisibility();
    void UpdateModel();
    void UpdateUnit();
    void UpdateDocumentWidth();
    void CommitEditedLengths();
    /// @brief Lit les deux champs de longueur **dans @p unit** et garde les millimètres.
    ///
    /// Séparé de l'écriture pour que le changement d'unité puisse lire dans l'ancienne avant de
    /// passer à la nouvelle : c'est tout le défaut que la séparation répare (voir UpdateUnit).
    void ReadEditedLengths(const Unit &unit);
    /// @brief Réécrit les deux champs de longueur dans @ref m_unit, depuis les millimètres gardés.
    void WriteEditedLengths();
    void Retranslate();

    auto HasAxes() const -> bool;

    const VContainer *m_data{nullptr};
    QSharedPointer<VMeasurements> m_measurements{};
    OGradedSizeRanges::Context m_rangeContext{};

    QLabel *m_unitLabel{nullptr};
    QComboBox *m_unitCombo{nullptr};

    QLabel *m_sourceLabel{nullptr};
    QComboBox *m_sourceCombo{nullptr};

    QLabel *m_rangeLabel{nullptr};
    QComboBox *m_rangeCombo{nullptr};

    QVector<QLabel *> m_dimensionLabels{};
    QVector<QComboBox *> m_dimensionCombos{};
    QVector<int> m_dimensionSlots{};

    QLabel *m_modelLabel{nullptr};
    QComboBox *m_modelCombo{nullptr};

    QLabel *m_marksLabel{nullptr};
    QComboBox *m_marksCombo{nullptr};

    QLabel *m_coverLabel{nullptr};
    QComboBox *m_coverCombo{nullptr};

    QStackedWidget *m_shapeStack{nullptr};
    OExportRollFormatWidget *m_rollWidget{nullptr};
    OExportTiledFormatWidget *m_tiledWidget{nullptr};

    QLabel *m_marginLabel{nullptr};
    QLineEdit *m_marginEdit{nullptr};
    QLabel *m_gapLabel{nullptr};
    QLineEdit *m_gapEdit{nullptr};

    QLabel *m_documentWidthLabel{nullptr};
    QLabel *m_documentWidthValue{nullptr};

    /// Kept in millimetres rather than re-read from the fields: an unreadable field would
    /// otherwise come back as zero, and zero is a legitimate margin — a mistyped figure
    /// would silently ask for no margin at all.
    qreal m_marginMm{5.0};
    qreal m_gapMm{5.0};

    Unit m_unit{Unit::Cm};
};

#endif // OEXPORTPRINTWIDGET_H
