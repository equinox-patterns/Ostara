/************************************************************************
 **
 **  @file   dialogincrements.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   November 15, 2013
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
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

#ifndef DIALOGINCREMENTS_H
#define DIALOGINCREMENTS_H

#include "../vmisc/vabstractshortcutmanager.h"
#include "../vmisc/vtablesearch.h"
#include "../vtools/dialogs/tools/dialogtool.h"
#include "../xml/vpattern.h"

#include <QPair>

class VIndividualMeasurements;
class QxtCsvModel;

namespace Ui
{
class DialogIncrements;
}

/**
 * @brief The DialogIncrements class show dialog increments. Tables of all variables in program will be here.
 */
class DialogIncrements : public DialogTool
{
    Q_OBJECT // NOLINT

public:
    DialogIncrements(VContainer *data, VPattern *doc, QWidget *parent = nullptr);
    ~DialogIncrements() override;

    void RestoreAfterClose();

    void CheckDependencyTreeComplete() override;

signals:
    void UpdateProperties();

public slots:
    void FullUpdateFromFile();

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    auto eventFilter(QObject *object, QEvent *event) -> bool override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void PlaceDialog() override;
    auto IsValid() const -> bool final { return true; }
private slots:
    void ShowIncrementDetails();
    void OnIncrementCellClicked(int row, int column);
    void AddIncrement();
    void RemoveIncrement();
    void MoveUp();
    void MoveDown();
    void SaveIncrName(const QString &text);
    void SaveIncrUnits();
    void SaveIncrDescription();
    void SaveIncrFormula();
    void Fx();
    void RefreshPattern();
    void ImportIncrements();
    void ExportIncrements();

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(DialogIncrements) // NOLINT

    /** @brief ui keeps information about user interface */
    Ui::DialogIncrements *ui;

    /** @brief m_data container with data */
    VContainer *m_data; // need because we must change data //-V703

    /** @brief m_patternDoc dom document container */
    VPattern *m_patternDoc;

    /** @brief m_completeData need to show all internal variables */
    VContainer m_completeData;

    QSharedPointer<VTableSearch> m_search{};
    QSharedPointer<VTableSearch> m_searchPC{};

    bool m_hasChanges{false};

    QVector<QPair<QString, QString>> m_renameList{};

    template <typename T> void FillTable(const QMap<QString, T> &varTable, QTableWidget *table);

    void FillIncrementsTable(QTableWidget *table, const QMap<QString, QSharedPointer<VIncrement>> &increments,
                             bool takePreviewCalculations);

    /// The increment name a row stands for, an empty string when the row is not an increment — a group line, or no
    /// row at all. Both tables identify a row by the text of its first column, which for a group line is the group's
    /// name: every action asks this before believing it holds an increment.
    static auto RowIncrementName(const QTableWidget *table, int row) -> QString;
    /// The group a group line stands for, NULL_ID for any other row — what rides in the row's own marker.
    static auto RowGroupId(const QTableWidget *table, int row) -> quint32;
    /// The row that lists one increment, -1 when it is not listed. Up and down find their row by name: a group line
    /// may sit just above a group's first member, where arithmetic on the index would land on the heading.
    static auto RowOfIncrement(const QTableWidget *table, const QString &name) -> int;
    /// The caret a group line wears: pointing right while the group is folded, down while it is open.
    static auto GroupCaret(bool collapsed) -> QIcon;
    /// Fold or unfold one group in one table, and only its own rows: the line answers, nothing is rebuilt, so the
    /// reader keeps their place. The state is written with the document, so it survives the dialog.
    void ToggleIncrementGroup(QTableWidget *table, int groupRow);

    void FillIncrements();
    void FillPreviewCalculations();
    void FillLengthsLines();
    void FillLengthLinesAngles();
    void FillLengthsCurves();
    void FillCurvesCLengths();
    void FillRadiusesArcs();
    void FillAnglesCurves();

    void ShowUnits();
    static void ShowHeaderUnits(QTableWidget *table, int column, const QString &unit);

    static auto AddCell(QTableWidget *table, const QString &text, int row, int column, int aligment, bool ok = true)
        -> QTableWidgetItem *;
    static auto AddSeparatorCell(QTableWidget *table, const QString &text, int row, int column, int aligment,
                                 bool ok = true) -> QTableWidgetItem *;
    /// The line of a group: its caret and its name in the very column an increment uses for its own name, in bold,
    /// and nothing in the two columns that hold a value and a formula — a group computes nothing. The id and the kind
    /// ride on the item, which is the only way a click can later tell this line from an increment's.
    static auto AddGroupCell(QTableWidget *table, quint32 groupId, const QString &name, bool collapsed, int row)
        -> QTableWidgetItem *;

    auto GetCustomName() const -> QString;
    static auto ClearIncrementName(const QString &name) -> QString;

    static auto EvalIncrementFormula(const QString &formula, bool fromUser, VContainer *data, QLabel *label,
                                     bool special) -> bool;
    void Controls(QTableWidget *table);
    void EnableDetails(QTableWidget *table, bool enabled);

    void LocalUpdateTree();

    auto IncrementUsed(const QString &name) const -> bool;

    void CacheRename(const QString &name, const QString &newName);

    void ShowTableIncrementDetails(QTableWidget *table);


    void AddNewIncrement(IncrementType type);

    void InitSearch();
    static void InitIncrementUnits(QComboBox *combo);

    void InitIcons();

    /// The list and the panel of details share the width of each tab, half and half, and keep that half when the
    /// dialog is resized. Which pane a tab puts on each side is the .ui's business — this only settles the share.
    void InitSplitterHalves();

    auto CheckIncrementName(const QString &name, const QSet<QString> &importedNames) const -> QString;
    void ShowError(const QString &text);

    auto IncrementsCSVColumnHeader(int column) const -> QString;

    void ImportCSVIncrements(bool previewCalculation, const QxtCsvModel &csv, const QVector<int> &map, bool withHeader);
    void ExportCSVIncrements(
        bool incrementMode, const QString &fileName, bool withHeader, int mib, const QChar &separator);
};

#endif // DIALOGINCREMENTS_H
