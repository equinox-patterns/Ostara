/************************************************************************
 **
 **  @file   dialogoperationtool.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   29 5, 2026
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2026 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
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

#ifndef DIALOGOPERATIONTOOL_H
#define DIALOGOPERATIONTOOL_H

#include "dialogtool.h"
#include "../../tools/drawTools/operation/ooperationlines.h"
#include "../../tools/toolsdef.h"

#include <QPair>
#include <QSet>
#include <QUuid>
#include <QVBoxLayout>

class QGroupBox;
class QLabel;
class QPlainTextEdit;
class VCompleterLineEdit;

struct OperationWidgets
{
    QLabel *labelStatus{nullptr};
    QLabel *labelGroupName{nullptr};
    QPlainTextEdit *notes{nullptr};
    VCompleterLineEdit *groupTags{nullptr};
    QLineEdit *visibilityGroupLine{nullptr};
    QGroupBox *visibilityGroup{nullptr};
};

class DialogOperationTool : public DialogTool
{
    Q_OBJECT // NOLINT

public:
    auto GetVisibilityGroupName() const -> QString;
    void SetVisibilityGroupName(const QString &name) const;

    auto HasLinkedVisibilityGroup() const -> bool;
    void SetHasLinkedVisibilityGroup(bool linked) const;

    void SetVisibilityGroupTags(const QStringList &tags) const;
    auto GetVisibilityGroupTags() const -> QStringList;

    void SetNotes(const QString &notes) const;
    auto GetNotes() const -> QString;

    void SetGroupCategories(const QStringList &categories) override;

    auto GetSourceObjects() const -> QVector<SourceItem>;
    void SetSourceObjects(const QVector<SourceItem> &value);

    /** @brief The lines the user selected, one descriptor each. A line is not a geometric object, so it travels
     *  beside the operated objects and never inside them; and it is not named by a tool id either — a rectangle
     *  draws four sides under one id — so what travels is the pair of its endpoints, with the tool that draws it. */
    auto GetLineSources() const -> const QVector<OOperationLine> &;
    void SetLineSources(const QVector<OOperationLine> &value);


    void CheckDependencyTreeComplete() override;

public slots:
    void ClearSourceObjects() override;
    void ChosenSegment(quint32 id, quint32 point1, quint32 point2) override;
    void GroupNameChanged();
    /** @brief Pick or unpick one of the lines a Line tool draws. */
    void ToggleLineSource(quint32 id);
    /** @brief Pick or unpick one of the segments a tool draws itself — a side of a rectangle, a tracé of a dart
     *  cap, the main line of a line point. What identifies it is the pair of its endpoints, never a tool id. */
    void ToggleDrawnSegment(const OOperationLine &segment);

protected:
    explicit DialogOperationTool(const VContainer *data, VAbstractPattern *doc, quint32 toolId,
                                 QWidget *parent = nullptr);

    virtual auto Widgets() const -> OperationWidgets = 0;

    virtual void OnSourceObjectsSet() {}

    void CheckState() override;

    // True when the set of operated objects differs from the set the dialog was opened with. Compared by
    // recordId (the same key SyncDestination() uses), so adding/removing - or removing and re-adding -
    // an object counts as a change. Such a change forces a full reparse that recreates the tool, so
    // Apply is disabled for it.
    auto SourceObjectsChanged() const -> bool;

    /** @brief True when the transformation gives this point an image: it is one of the operated objects.
     *
     *  Deliberately stricter than what the operation will do — a rotation leaves its pivot in place, a mirror
     *  leaves the points of its axis in place, a transfer refers the source merge point to the merge point of
     *  the host — because the rule the user has to hold in mind is one rule and not five: a line is taken when
     *  its two points are taken, and never before. */
    auto HasImage(quint32 pointId) const -> bool;

    /** @brief True when the line can be transformed: both of its endpoints have an image. A line is a segment
     *  drawn between the images of its two points, so one endpoint without an image leaves nothing to build. */
    auto LineCanBeBuilt(const OOperationLine &line) const -> bool;

    /** @brief Drop the taken lines that lost an endpoint, so the preview never promises a line that cannot be
     *  built any more. Called after an operated object is removed. */
    void DropUnbuildableLines();

    /** @brief The lines the operation carries: the ones the user picked, as pairs of endpoint ids, ready for the
     *  visualization. */
    auto PreviewLines() const -> QVector<QPair<quint32, quint32>>;

    void ToggleSourceObject(quint32 id);

    auto SourceObjects() -> QVector<SourceItem> & { return m_sourceObjects; }
    auto IsStage1() const -> bool { return stage1; }
    void SetStage1(bool value) { stage1 = value; }
    auto IsFlagGroupName() const -> bool { return flagGroupName; }
    auto IsFlagError() const -> bool { return flagError; }
    void SetFlagError(bool value) { flagError = value; }
    auto GroupTags() -> QStringList & { return m_groupTags; }

private:
    QVector<SourceItem> m_sourceObjects{};
    QVector<OOperationLine> m_lineSources{};

    /** @brief m_baselineSourceRecords operated-object recordIds captured when the dialog was first
     * populated. Used to detect object-set changes (see SourceObjectsChanged). */
    QSet<QUuid> m_baselineSourceRecords{};
    bool m_sourceBaselineCaptured{false};

    /** @brief Same baseline idea for the selected lines: any change of the line set needs a full
     *  reparse to create or drop the transformed lines, exactly like an object-set change. */
    QVector<OOperationLine> m_baselineLineSources{};
    bool m_lineBaselineCaptured{false};

    bool stage1{true};
    bool flagGroupName{true};
    bool flagError{false};
    QStringList m_groupTags{};
};

//---------------------------------------------------------------------------------------------------------------------
/** @brief The widgets every operation dialog offers, read from its generated .ui.
 *
 * The six operations name these widgets the same way in their .ui, so the list is written once here rather than
 * six times. A dialog that renames one of them gets a compile error, which is the point: the widget set is a
 * contract between the base class and every operation dialog.
 *
 * The source-object list and its combos are NOT here: what an operation operates is chosen on the canvas, one
 * click per object, and a dialog no longer edits that set.
 */
template <class Ui> auto OperationWidgetsFrom(const Ui *ui) -> OperationWidgets
{
    return {.labelStatus = ui->labelStatus,
            .labelGroupName = ui->labelGroupName,
            .notes = ui->plainTextEditToolNotes,
            .groupTags = ui->lineEditGroupTags,
            .visibilityGroupLine = ui->lineEditVisibilityGroup,
            .visibilityGroup = ui->groupBoxVisibilityGroup};
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief Put a formula field where a .ui left a placeholder.
 *
 * A formula field is a C++ component, so a .ui cannot declare one: it declares an empty widget instead, and the
 * field takes its place in the same vertical layout. Written once here, for every operation dialog that has one
 * — a rotation has two, the tool that closes a dart has one.
 */
inline void InsertFormulaField(QWidget *placeholder, QWidget *field)
{
    if (placeholder == nullptr)
    {
        return;
    }

    auto *parentLayout = qobject_cast<QVBoxLayout *>(placeholder->parentWidget()->layout());
    if (parentLayout == nullptr)
    {
        return;
    }

    parentLayout->insertWidget(parentLayout->indexOf(placeholder), field);
    placeholder->deleteLater();
}

#endif // DIALOGOPERATIONTOOL_H
