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
class OLayerGroupChooser;

struct OperationWidgets
{
    QLabel *labelStatus{nullptr};
    QLabel *labelGroupName{nullptr};
    QPlainTextEdit *notes{nullptr};
    OLayerGroupChooser *layerGroup{nullptr};
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

    /// The group of layers the operation's own layer joins. NULL_ID when it joins none, the id of an existing
    /// group, or VAbstractPattern::NewLayerGroupId when the user named one that does not exist yet — the tool then
    /// creates it (see GetNewLayerGroupName()).
    void SetLayerGroup(quint32 groupId) const;
    auto GetLayerGroup() const -> quint32;
    auto GetNewLayerGroupName() const -> QString;

    void SetNotes(const QString &notes) const;
    auto GetNotes() const -> QString;

    /// The groups of layers that can be joined, id -> name: what the chooser offers.
    void SetLayerGroups(const QMap<quint32, QString> &groups) override;

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

    /** @brief An operation is applied by the parse its figure asks for, and that parse can be a full one: the lines
     *  a figure draws are tools only a full parse builds (see OPleatDisplay::LineIds, and the display of the cuts a
     *  spread shows). The dialog is what the file is written from, and the tool the parse rebuilds carries that same
     *  state: it therefore stays open across that parse, and the window links it to the tool it rebuilt (see
     *  MainWindow::FullParseFile) — so that Apply is the OK of every operation dialog without the closing: the change
     *  is written, the canvas shows it, and the window stays there to be looked at, changed again, or confirmed. */
    auto SurvivesFullParse() const -> bool override { return true; }

    virtual void OnSourceObjectsSet() {}

    void CheckState() override;

    // True when the set of operated objects differs from the set the dialog was opened with. Compared by
    // **object id**, the key the pattern itself carries in <source> — and never by the recordId of the session,
    // which every parse makes anew — so adding/removing - or removing and re-adding - an object counts as a
    // change, while the state the parser has just rebuilt stays the one the dialog was opened with. Such a
    // change forces a full reparse that recreates the tool, so Apply is disabled for it.
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

private:
    QVector<SourceItem> m_sourceObjects{};
    QVector<OOperationLine> m_lineSources{};

    /** @brief The operated objects the dialog was populated with, by **object id**: what says whether the user has
     *  added, taken away or moved one of them since (see SourceObjectsChanged).
     *
     *  The ids are the ones the pattern carries in `<source>`, and a parse reads them back as they are. The
     *  `recordId` of a source item is a key of the session, made anew by every parse and never written to the file:
     *  comparing those would take the very state the parser has just rebuilt for a change of the user, and grey
     *  Apply as soon as one Apply has run — the tool being rebuilt under the dialog it drives. */
    QVector<quint32> m_baselineSourceIds{};
    bool m_sourceBaselineCaptured{false};

    /** @brief Same baseline idea for the selected lines: any change of the line set needs a full
     *  reparse to create or drop the transformed lines, exactly like an object-set change. */
    QVector<OOperationLine> m_baselineLineSources{};
    bool m_lineBaselineCaptured{false};

    bool stage1{true};
    bool flagGroupName{true};
    bool flagError{false};
};

//---------------------------------------------------------------------------------------------------------------------
/** @brief The widgets every operation dialog offers, read from its generated .ui.
 *
 * The operations name these widgets the same way in their .ui, so the list is written once here rather than six
 * times. The status line and the notes are what every operation dialog has; the layer group — its name, its tags and
 * its box — is the part a dialog may leave out when the operation has no layer of its own, and the entries are then
 * simply left null (the base class answers an empty value for them, and never touches a widget that is not there).
 *
 * The source-object list and its combos are NOT here: what an operation operates is chosen on the canvas, one
 * click per object, and a dialog no longer edits that set.
 */
template <class Ui> auto OperationWidgetsFrom(const Ui *ui) -> OperationWidgets
{
    OperationWidgets widgets{.labelStatus = ui->labelStatus, .notes = ui->plainTextEditToolNotes};

    if constexpr (requires { ui->labelGroupName; })
    {
        widgets.labelGroupName = ui->labelGroupName;
    }
    if constexpr (requires { ui->layerGroupChooser; })
    {
        widgets.layerGroup = ui->layerGroupChooser;
    }
    if constexpr (requires { ui->lineEditVisibilityGroup; })
    {
        widgets.visibilityGroupLine = ui->lineEditVisibilityGroup;
    }
    if constexpr (requires { ui->groupBoxVisibilityGroup; })
    {
        widgets.visibilityGroup = ui->groupBoxVisibilityGroup;
    }

    return widgets;
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
