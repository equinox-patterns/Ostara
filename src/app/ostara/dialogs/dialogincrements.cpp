/************************************************************************
 **
 **  @file   dialogincrements.cpp
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

#include "dialogincrements.h"
#include "../qmuparser/qmudef.h"
#include "../qmuparser/qmutokenparser.h"
#include "../vmisc/dialogs/dialogexporttocsv.h"
#include "../vmisc/qxtcsvmodel.h"
#include "../vmisc/vmodifierkey.h"
#include "vtheme.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vpatterndb/calculator.h"
#include "../vpatterndb/variables/varcradius.h"
#include "../vpatterndb/variables/vcurveangle.h"
#include "../vpatterndb/variables/vcurveclength.h"
#include "../vpatterndb/variables/vcurvelength.h"
#include "../vpatterndb/variables/vincrement.h"
#include "../vpatterndb/variables/vlineangle.h"
#include "../vpatterndb/variables/vlinelength.h"
#include "../vpatterndb/vtranslatevars.h"
#include "../vtools/dialogs/support/dialogeditwrongformula.h"
#include "dialogincrementscsvcolumns.h"
#include "ui_dialogincrements.h"

#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QtNumeric>

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include "../vmisc/vtextcodec.h"
using QTextCodec = VTextCodec;
#else
#include <QTextCodec>
#endif // QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

using namespace Qt::Literals::StringLiterals;

constexpr int DIALOG_MAX_FORMULA_HEIGHT = 64;

namespace
{
enum class IncrUnits : qint8
{
    Pattern,
    Degrees
};

struct IncrementData
{
    QString name{};           // NOLINT(misc-non-private-member-variables-in-classes)
    QString value{'0'};       // NOLINT(misc-non-private-member-variables-in-classes)
    QString description{};    // NOLINT(misc-non-private-member-variables-in-classes)
    bool specialUnits{false}; // NOLINT(misc-non-private-member-variables-in-classes)
};

/// What a row of either table stands for. A group line is the level above the increments: it wears the group's name,
/// and the rows that follow it down to the next group line are its content — exactly the shape of a group of
/// measurements. An increment row carries no marker at all: its first column holds a name, not an id, and nothing
/// but this marker could tell the two lines apart.
enum class RowKind : quint8
{
    Increment,
    Group
};

/// The group a group line stands for rides in Qt::UserRole, on the very column an increment row uses for its name;
/// what KIND of row it is rides in the role after it. One group id can be told from one increment name by nothing
/// else.
constexpr int kIdRole = Qt::UserRole;
constexpr int kKindRole = Qt::UserRole + 1;

//---------------------------------------------------------------------------------------------------------------------
void SetIncrementDescription(int i, const QxtCsvModel &csv, const QVector<int> &map, IncrementData &measurement)
{
    if (csv.columnCount() > 2)
    {
        if (const int descriptionColumn = map.at(static_cast<int>(IncrementsColumns::Description));
            descriptionColumn >= 0)
        {
            measurement.description = csv.text(i, descriptionColumn).simplified();
        }
    }
}
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DialogIncrements create dialog
 * @param data container with data
 * @param doc dom document container
 * @param parent parent widget
 */
DialogIncrements::DialogIncrements(VContainer *data, VPattern *doc, QWidget *parent)
  : DialogTool(data, doc, NULL_ID, parent),
    ui(new Ui::DialogIncrements),
    m_data(data),
    m_patternDoc(doc),
    m_completeData(doc->GetCompleteData()),
    m_searchHistory(new QMenu(this)),
    m_searchHistoryPC(new QMenu(this))
{
    ui->setupUi(this);

#if defined(Q_OS_MAC)
    setWindowFlags(Qt::Window);
#endif

    InitIcons();

    ui->toolButtonImportIncrementsCSV->setIcon(FromTheme(VThemeIcon::DocumentImport));
    ui->toolButtonExportIncrementsCSV->setIcon(FromTheme(VThemeIcon::DocumentExport));

    ui->toolButtonImportIncrementsCSVPC->setIcon(FromTheme(VThemeIcon::DocumentImport));
    ui->toolButtonExportIncrementsCSVPC->setIcon(FromTheme(VThemeIcon::DocumentExport));


    ui->lineEditFind->installEventFilter(this);
    ui->lineEditFindPC->installEventFilter(this);

    m_search = QSharedPointer<VTableSearch>(new VTableSearch(ui->tableWidgetIncrement));
    m_searchPC = QSharedPointer<VTableSearch>(new VTableSearch(ui->tableWidgetPC));

    m_formulaBaseHeight = ui->plainTextEditFormula->height();
    ui->plainTextEditFormula->installEventFilter(this);
    m_formulaBaseHeightPC = ui->plainTextEditFormulaPC->height();
    ui->plainTextEditFormulaPC->installEventFilter(this);

    VAbstractApplication::VApp()->Settings()->GetOsSeparator() ? setLocale(QLocale()) : setLocale(QLocale::c());

    qCDebug(vDialog, "Showing variables.");
    ShowUnits();

    InitIncrementUnits(ui->comboBoxIncrementUnits);
    InitIncrementUnits(ui->comboBoxPreviewCalculationUnits);

    {
        const QSignalBlocker blocker(ui->comboBoxIncrementUnits);
        ui->comboBoxIncrementUnits->setCurrentIndex(-1);
    }

    {
        const QSignalBlocker blocker(ui->comboBoxPreviewCalculationUnits);
        ui->comboBoxPreviewCalculationUnits->setCurrentIndex(-1);
    }

    connect(ui->comboBoxIncrementUnits, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DialogIncrements::SaveIncrUnits);

    FillIncrements();
    FillPreviewCalculations();
    FillLengthsLines();
    FillLengthLinesAngles();
    FillLengthsCurves();
    FillCurvesCLengths();
    FillRadiusesArcs();
    FillAnglesCurves();

    connect(this->m_patternDoc, &VPattern::FullUpdateFromFile, this, &DialogIncrements::FullUpdateFromFile);

    ui->tabWidget->setCurrentIndex(0);
    auto *validator = new QRegularExpressionValidator(QRegularExpression(QStringLiteral("^$|") + NameRegExp()), this);
    ui->lineEditName->setValidator(validator);
    ui->lineEditNamePC->setValidator(validator);

    connect(ui->tableWidgetIncrement, &QTableWidget::itemSelectionChanged, this,
            &DialogIncrements::ShowIncrementDetails);
    connect(ui->tableWidgetPC, &QTableWidget::itemSelectionChanged, this, &DialogIncrements::ShowIncrementDetails);

    // A click on the line of a group folds it. No other gesture comes from the tables themselves — every other
    // change is an edit made in the panel below — so this is the only click either table listens to.
    connect(ui->tableWidgetIncrement, &QTableWidget::cellClicked, this, &DialogIncrements::OnIncrementCellClicked);
    connect(ui->tableWidgetPC, &QTableWidget::cellClicked, this, &DialogIncrements::OnIncrementCellClicked);

    ui->toolButtonAdd->setMenu(InitVarTypeMenu(ui->toolButtonAdd->menu(), true /*increments tab*/));
    ui->toolButtonAdd->setProperty("hasMenu", true);
    ui->toolButtonAdd->style()->unpolish(ui->toolButtonAdd);
    ui->toolButtonAdd->style()->polish(ui->toolButtonAdd);

    ui->toolButtonAddPC->setMenu(InitVarTypeMenu(ui->toolButtonAddPC->menu(), false /*preview calculations tab*/));
    ui->toolButtonAddPC->setProperty("hasMenu", true);
    ui->toolButtonAddPC->style()->unpolish(ui->toolButtonAddPC);
    ui->toolButtonAddPC->style()->polish(ui->toolButtonAddPC);

    connect(ui->toolButtonAdd, &QToolButton::clicked, this, &DialogIncrements::AddIncrement);
    connect(ui->toolButtonAddPC, &QToolButton::clicked, this, &DialogIncrements::AddIncrement);
    connect(ui->toolButtonRemove, &QToolButton::clicked, this, &DialogIncrements::RemoveIncrement);
    connect(ui->toolButtonRemovePC, &QToolButton::clicked, this, &DialogIncrements::RemoveIncrement);
    connect(ui->toolButtonUp, &QToolButton::clicked, this, &DialogIncrements::MoveUp);
    connect(ui->toolButtonUpPC, &QToolButton::clicked, this, &DialogIncrements::MoveUp);
    connect(ui->toolButtonDown, &QToolButton::clicked, this, &DialogIncrements::MoveDown);
    connect(ui->toolButtonDownPC, &QToolButton::clicked, this, &DialogIncrements::MoveDown);
    connect(ui->pushButtonGrow, &QPushButton::clicked, this, &DialogIncrements::DeployFormula);
    connect(ui->pushButtonGrowPC, &QPushButton::clicked, this, &DialogIncrements::DeployFormula);
    connect(ui->toolButtonExpr, &QToolButton::clicked, this, &DialogIncrements::Fx);
    connect(ui->toolButtonExprPC, &QToolButton::clicked, this, &DialogIncrements::Fx);
    connect(ui->lineEditName, &QLineEdit::textEdited, this, &DialogIncrements::SaveIncrName);
    connect(ui->lineEditNamePC, &QLineEdit::textEdited, this, &DialogIncrements::SaveIncrName);
    connect(ui->plainTextEditDescription, &QPlainTextEdit::textChanged, this, &DialogIncrements::SaveIncrDescription);
    connect(ui->plainTextEditDescriptionPC, &QPlainTextEdit::textChanged, this, &DialogIncrements::SaveIncrDescription);
    connect(ui->plainTextEditFormula, &QPlainTextEdit::textChanged, this, &DialogIncrements::SaveIncrFormula);
    connect(ui->plainTextEditFormulaPC, &QPlainTextEdit::textChanged, this, &DialogIncrements::SaveIncrFormula);
    connect(ui->pushButtonRefresh, &QPushButton::clicked, this, &DialogIncrements::RefreshPattern);
    connect(ui->pushButtonRefreshPC, &QPushButton::clicked, this, &DialogIncrements::RefreshPattern);
    connect(ui->toolButtonImportIncrementsCSV, &QToolButton::clicked, this, &DialogIncrements::ImportIncrements);
    connect(ui->toolButtonImportIncrementsCSVPC, &QToolButton::clicked, this, &DialogIncrements::ImportIncrements);
    connect(ui->toolButtonExportIncrementsCSV, &QToolButton::clicked, this, &DialogIncrements::ExportIncrements);
    connect(ui->toolButtonExportIncrementsCSVPC, &QToolButton::clicked, this, &DialogIncrements::ExportIncrements);

    InitSearch();

    if (ui->tableWidgetIncrement->rowCount() > 0)
    {
        ui->tableWidgetIncrement->selectRow(0);
    }

    if (ui->tableWidgetPC->rowCount() > 0)
    {
        ui->tableWidgetPC->selectRow(0);
    }

    m_shortcuts.insert(VShortcutAction::CaseSensitiveMatch, ui->toolButtonCaseSensitive);
    m_shortcuts.insert(VShortcutAction::WholeWordMatch, ui->toolButtonWholeWord);
    m_shortcuts.insert(VShortcutAction::RegexMatch, ui->toolButtonRegexp);
    m_shortcuts.insert(VShortcutAction::SearchHistory, ui->pushButtonSearch);
    m_shortcuts.insert(VShortcutAction::RegexMatchUnicodeProperties, ui->toolButtonUseUnicodeProperties);
    m_shortcuts.insert(VShortcutAction::FindNext, ui->toolButtonFindNext);
    m_shortcuts.insert(VShortcutAction::FindPrevious, ui->toolButtonFindNext);

    m_shortcuts.insert(VShortcutAction::CaseSensitiveMatch, ui->toolButtonCaseSensitivePC);
    m_shortcuts.insert(VShortcutAction::WholeWordMatch, ui->toolButtonWholeWordPC);
    m_shortcuts.insert(VShortcutAction::RegexMatch, ui->toolButtonRegexpPC);
    m_shortcuts.insert(VShortcutAction::SearchHistory, ui->pushButtonSearchPC);
    m_shortcuts.insert(VShortcutAction::RegexMatchUnicodeProperties, ui->toolButtonUseUnicodePropertiesPC);
    m_shortcuts.insert(VShortcutAction::FindNext, ui->toolButtonFindNextPC);
    m_shortcuts.insert(VShortcutAction::FindPrevious, ui->toolButtonFindNextPC);

    if (VAbstractShortcutManager  const*manager = VAbstractApplication::VApp()->GetShortcutManager())
    {
        connect(manager, &VAbstractShortcutManager::ShortcutsUpdated, this, &DialogIncrements::UpdateShortcuts);
        UpdateShortcuts();
    }

    const QPushButton *bClose = ui->buttonBox->button(QDialogButtonBox::Close);
    SCASSERT(bClose != nullptr)
    connect(bClose, &QPushButton::clicked, this, &DialogIncrements::close);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief FillIncrementTable fill data for increment table
 */
void DialogIncrements::FillIncrements()
{
    FillIncrementsTable(ui->tableWidgetIncrement, m_data->DataIncrementsWithSeparators(), false);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::FillPreviewCalculations()
{
    FillIncrementsTable(ui->tableWidgetPC, m_data->DataIncrementsWithSeparators(), true);
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T> void DialogIncrements::FillTable(const QMap<QString, T> &varTable, QTableWidget *table)
{
    SCASSERT(table != nullptr)

    qint32 currentRow = -1;
    QMapIterator<QString, T> i(varTable);
    while (i.hasNext())
    {
        i.next();
        qreal const length = *i.value()->GetValue();
        currentRow++;
        table->setRowCount(static_cast<int>(varTable.size()));

        auto *item = new QTableWidgetItem(i.key());
        item->setTextAlignment(Qt::AlignLeft);
        QFont font = item->font();
        font.setBold(true);
        item->setFont(font);
        table->setItem(currentRow, 0, item);

        item = new QTableWidgetItem(VAbstractApplication::VApp()->LocaleToString(length));
        item->setTextAlignment(Qt::AlignHCenter);
        table->setItem(currentRow, 1, item);
    }
    table->resizeColumnsToContents();
    table->resizeRowsToContents();
    table->verticalHeader()->setDefaultSectionSize(20);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief FillLengthLines fill data for table of lines lengths
 */
void DialogIncrements::FillLengthsLines()
{
    FillTable(m_completeData.DataLengthLines(), ui->tableWidgetLines);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::FillLengthLinesAngles()
{
    FillTable(m_completeData.DataAngleLines(), ui->tableWidgetLinesAngles);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief FillLengthSplines fill data for table of splines lengths
 */
void DialogIncrements::FillLengthsCurves()
{
    FillTable(m_completeData.DataLengthCurves(), ui->tableWidgetSplines);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::FillCurvesCLengths()
{
    FillTable(m_completeData.DataCurvesCLength(), ui->tableWidgetCLength);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::FillRadiusesArcs()
{
    FillTable(m_completeData.DataRadiusesArcs(), ui->tableWidgetRadiusesArcs);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::FillAnglesCurves()
{
    FillTable(m_completeData.DataAnglesCurves(), ui->tableWidgetAnglesCurves);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::ShowUnits()
{
    const QString unit = UnitsToStr(VAbstractValApplication::VApp()->patternUnits());

    ShowHeaderUnits(ui->tableWidgetIncrement, 1, unit); // calculated value
    ShowHeaderUnits(ui->tableWidgetIncrement, 2, unit); // formula
    ShowHeaderUnits(ui->tableWidgetPC, 1, unit);        // calculated value
    ShowHeaderUnits(ui->tableWidgetPC, 2, unit);        // formula

    ShowHeaderUnits(ui->tableWidgetLines, 1, unit);                // lengths
    ShowHeaderUnits(ui->tableWidgetSplines, 1, unit);              // lengths
    ShowHeaderUnits(ui->tableWidgetCLength, 1, unit);              // lengths
    ShowHeaderUnits(ui->tableWidgetLinesAngles, 1, degreeSymbol);  // angle
    ShowHeaderUnits(ui->tableWidgetRadiusesArcs, 1, unit);         // radius
    ShowHeaderUnits(ui->tableWidgetAnglesCurves, 1, degreeSymbol); // angle
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::ShowHeaderUnits(QTableWidget *table, int column, const QString &unit)
{
    SCASSERT(table != nullptr)

    const QString header = table->horizontalHeaderItem(column)->text();
    const auto unitHeader = QStringLiteral("%1 (%2)").arg(header, unit);
    table->horizontalHeaderItem(column)->setText(unitHeader);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogIncrements::AddCell(QTableWidget *table, const QString &text, int row, int column, int aligment, bool ok)
    -> QTableWidgetItem *
{
    SCASSERT(table != nullptr)

    auto *item = new QTableWidgetItem(text);
    SetTextAlignment(item, static_cast<Qt::Alignment>(aligment));
    item->setToolTip(text);

    // set the item non-editable (view only), and non-selectable
    Qt::ItemFlags flags = item->flags();
    flags &= ~(Qt::ItemIsEditable); // reset/clear the flag
    item->setFlags(flags);

    if (not ok)
    {
        QBrush brush = item->foreground();
        brush.setColor(Qt::red);
        item->setForeground(brush);
    }

    table->setItem(row, column, item);

    return item;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogIncrements::AddSeparatorCell(QTableWidget *table, const QString &text, int row, int column, int aligment,
                                        bool ok) -> QTableWidgetItem *
{
    QTableWidgetItem *item = AddCell(table, text, row, column, aligment, ok);

    QFont itemFont = item->font();
    itemFont.setBold(true);
    itemFont.setItalic(true);
    item->setFont(itemFont);
    return item;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogIncrements::AddGroupCell(QTableWidget *table, quint32 groupId, const QString &name, bool collapsed, int row)
    -> QTableWidgetItem *
{
    SCASSERT(table != nullptr)

    auto *item = new QTableWidgetItem(GroupCaret(collapsed), name);
    SetTextAlignment(item, Qt::AlignVCenter);
    item->setData(kIdRole, groupId);
    item->setData(kKindRole, static_cast<quint32>(RowKind::Group));
    item->setToolTip(tr("Group: %1").arg(name));

    QFont itemFont = item->font();
    itemFont.setBold(true);
    item->setFont(itemFont);

    // Not editable: a double click would otherwise open the increment's rename editor on the line of a group.
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);

    table->setItem(row, 0, item);

    return item;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogIncrements::RowIncrementName(const QTableWidget *table, int row) -> QString
{
    if (table == nullptr || row < 0 || row >= table->rowCount())
    {
        return {};
    }

    const QTableWidgetItem *item = table->item(row, 0);
    if (item == nullptr)
    {
        return {};
    }

    // A row built before the groups existed carries no marker, and every such row is an increment.
    if (item->data(kKindRole).toUInt() == static_cast<quint32>(RowKind::Group))
    {
        return {};
    }

    return item->text();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogIncrements::RowGroupId(const QTableWidget *table, int row) -> quint32
{
    if (table == nullptr || row < 0 || row >= table->rowCount())
    {
        return NULL_ID;
    }

    const QTableWidgetItem *item = table->item(row, 0);
    if (item == nullptr || item->data(kKindRole).toUInt() != static_cast<quint32>(RowKind::Group))
    {
        return NULL_ID;
    }

    return item->data(kIdRole).toUInt();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogIncrements::RowOfIncrement(const QTableWidget *table, const QString &name) -> int
{
    if (table == nullptr || name.isEmpty())
    {
        return -1;
    }

    for (int row = 0; row < table->rowCount(); ++row)
    {
        if (RowIncrementName(table, row) == name)
        {
            return row;
        }
    }

    return -1;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogIncrements::GroupCaret(bool collapsed) -> QIcon
{
    // The same pair the formula field's own grow button wears: pointing right while there is something folded away,
    // down while everything is out. Read from the theme, so the two themes each get their own drawing.
    return FromTheme(collapsed ? VThemeIcon::GoNext : VThemeIcon::GoDown);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogIncrements::GetCustomName() const -> QString
{
    qint32 num = 1;
    QString name;
    QString const subName = CustomIncrSign + VAbstractApplication::VApp()->TrVars()->InternalVarToUser(increment_);
    do
    {
        name = subName + QString::number(num);
        num++;
    } while (not m_data->IsUnique(name));
    return name;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogIncrements::ClearIncrementName(const QString &name) -> QString
{
    QString clear = name;
    if (const auto index = clear.indexOf(CustomIncrSign); index == 0)
    {
        clear.remove(0, 1);
    }
    return clear;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogIncrements::EvalIncrementFormula(const QString &formula, bool fromUser, VContainer *data, QLabel *label,
                                            bool special) -> bool
{
    QString postfix;
    if (special)
    {
        postfix = degreeSymbol;
    }
    else
    {
        postfix =
            UnitsToStr(VAbstractValApplication::VApp()->patternUnits()); // Show unit in dialog lable (cm, mm or inch)
    }
    if (formula.isEmpty())
    {
        label->setText(tr("Error") + " (" + postfix + "). " + tr("Empty field."));
        label->setToolTip(tr("Empty field"));
        return false;
    }

    try
    {
        QString f;
        // Replace line return character with spaces for calc if exist
        if (fromUser)
        {
            f = VAbstractApplication::VApp()->TrVars()->FormulaFromUser(
                formula, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
        }
        else
        {
            f = formula;
        }
        QScopedPointer<Calculator> const cal(new Calculator());
        const qreal result = cal->EvalFormula(data->DataVariables(), f);

        if (qIsInf(result) || qIsNaN(result))
        {
            label->setText(tr("Error") + " (" + postfix + ").");
            label->setToolTip(tr("Invalid result. Value is infinite or NaN. Please, check your calculations."));
            return false;
        }

        label->setText(VAbstractApplication::VApp()->LocaleToString(result) + QChar(QChar::Space) + postfix);
        label->setToolTip(tr("Value"));
        return true;
    }
    catch (qmu::QmuParserError &e)
    {
        label->setText(tr("Error") + " (" + postfix + "). " + tr("Parser error: %1").arg(e.GetMsg()));
        label->setToolTip(tr("Parser error: %1").arg(e.GetMsg()));
        return false;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::Controls(QTableWidget *table)
{
    SCASSERT(table != nullptr)

    const QString name = RowIncrementName(table, table->currentRow());

    if (name.isEmpty())
    {
        // A group line is not a row of the table of variables: it can neither be removed nor moved, and there is
        // nothing to ask the formulas about. A table with no current row lands here too — the old shape of this
        // function read the current row whenever the table held one, which is not the same thing.
        if (table == ui->tableWidgetIncrement)
        {
            ui->toolButtonRemove->setEnabled(false);
            ui->toolButtonUp->setEnabled(false);
            ui->toolButtonDown->setEnabled(false);
        }
        else if (table == ui->tableWidgetPC)
        {
            ui->toolButtonRemovePC->setEnabled(false);
            ui->toolButtonUpPC->setEnabled(false);
            ui->toolButtonDownPC->setEnabled(false);
        }

        return;
    }

    if (table == ui->tableWidgetIncrement)
    {
        ui->toolButtonRemove->setEnabled(not IncrementUsed(name));
    }
    else if (table == ui->tableWidgetPC)
    {
        ui->toolButtonRemovePC->setEnabled(not IncrementUsed(name));
    }

    const int row = table->currentRow();
    const int lastRow = table->rowCount() - 1;

    // A member cannot leave its group by the arrows: above the first member sits the group's line, and below the
    // last one the arrows would reorder the document across the group. Everywhere else the row moves between two
    // increments, which is what the arrows promise.
    const bool canMoveUp = row > 0 && RowGroupId(table, row - 1) == NULL_ID;
    const bool canMoveDown = row < lastRow && RowGroupId(table, row + 1) == NULL_ID;

    if (table == ui->tableWidgetIncrement)
    {
        ui->toolButtonUp->setEnabled(canMoveUp);
        ui->toolButtonDown->setEnabled(canMoveDown);
    }
    else if (table == ui->tableWidgetPC)
    {
        ui->toolButtonUpPC->setEnabled(canMoveUp);
        ui->toolButtonDownPC->setEnabled(canMoveDown);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::EnableDetails(QTableWidget *table, bool enabled)
{
    SCASSERT(table != nullptr)

    if (enabled)
    {
        Controls(table);
    }
    else
    {
        if (table == ui->tableWidgetIncrement)
        {
            ui->toolButtonRemove->setEnabled(enabled);
            ui->toolButtonUp->setEnabled(enabled);
            ui->toolButtonDown->setEnabled(enabled);
        }
        else if (table == ui->tableWidgetPC)
        {
            ui->toolButtonRemovePC->setEnabled(enabled);
            ui->toolButtonUpPC->setEnabled(enabled);
            ui->toolButtonDownPC->setEnabled(enabled);
        }
    }

    if (not enabled)
    { // Clear
        if (table == ui->tableWidgetIncrement)
        {
            {
                const QSignalBlocker blocker(ui->lineEditName);
                ui->lineEditName->clear();
            }

            {
                const QSignalBlocker blocker(ui->comboBoxIncrementUnits);
                ui->comboBoxIncrementUnits->setCurrentIndex(-1);
            }

            {
                const QSignalBlocker blocker(ui->plainTextEditDescription);
                ui->plainTextEditDescription->clear();
            }

            {
                const QSignalBlocker blocker(ui->labelCalculatedValue);
                ui->labelCalculatedValue->clear();
            }

            const QSignalBlocker blocker(ui->plainTextEditFormula);
            ui->plainTextEditFormula->clear();
        }
        else if (table == ui->tableWidgetPC)
        {
            {
                const QSignalBlocker blocker(ui->lineEditNamePC);
                ui->lineEditNamePC->clear();
            }

            {
                const QSignalBlocker blocker(ui->comboBoxPreviewCalculationUnits);
                ui->comboBoxPreviewCalculationUnits->setCurrentIndex(-1);
            }

            {
                const QSignalBlocker blocker(ui->plainTextEditDescriptionPC);
                ui->plainTextEditDescriptionPC->clear();
            }

            {
                const QSignalBlocker blocker(ui->labelCalculatedValuePC);
                ui->labelCalculatedValuePC->clear();
            }

            const QSignalBlocker blocker(ui->plainTextEditFormulaPC);
            ui->plainTextEditFormulaPC->clear();
        }
    }

    if (table == ui->tableWidgetIncrement)
    {
        ui->pushButtonGrow->setEnabled(enabled);
        ui->toolButtonExpr->setEnabled(enabled);
        ui->lineEditName->setEnabled(enabled);
        ui->comboBoxIncrementUnits->setEnabled(enabled);
        ui->plainTextEditDescription->setEnabled(enabled);
        ui->plainTextEditFormula->setEnabled(enabled);
    }
    else if (table == ui->tableWidgetPC)
    {
        ui->pushButtonGrowPC->setEnabled(enabled);
        ui->toolButtonExprPC->setEnabled(enabled);
        ui->lineEditNamePC->setEnabled(enabled);
        ui->comboBoxPreviewCalculationUnits->setEnabled(enabled);
        ui->plainTextEditDescriptionPC->setEnabled(enabled);
        ui->plainTextEditFormulaPC->setEnabled(enabled);
    }

    if (table->rowCount() > 0 && table->currentRow() != -1)
    {
        // A group line has neither a value nor a formula: its line is a heading, and the panel would otherwise offer
        // to edit something that does not exist. The container is asked only about a real increment — it throws on a
        // name it does not hold, and a group's name is not one of them.
        const QString name = RowIncrementName(table, table->currentRow());

        bool showsValue = false;
        if (not name.isEmpty())
        {
            QSharedPointer<VIncrement> const incr = m_data->GetVariable<VIncrement>(name);
            showsValue = incr->GetIncrementType() != IncrementType::Separator;
        }

        if (table == ui->tableWidgetIncrement)
        {
            ui->labelCalculated->setVisible(showsValue);
            ui->labelCalculatedValue->setVisible(showsValue);
            ui->labelFormula->setVisible(showsValue);
            ui->plainTextEditFormula->setVisible(showsValue);
            ui->pushButtonGrow->setVisible(showsValue);
            ui->toolButtonExpr->setVisible(showsValue);
            ui->labelIncrementUnits->setVisible(showsValue);
            ui->comboBoxIncrementUnits->setVisible(showsValue);
        }
        else if (table == ui->tableWidgetPC)
        {
            ui->labelCalculatedPC->setVisible(showsValue);
            ui->labelCalculatedValuePC->setVisible(showsValue);
            ui->labelFormulaPC->setVisible(showsValue);
            ui->plainTextEditFormulaPC->setVisible(showsValue);
            ui->pushButtonGrowPC->setVisible(showsValue);
            ui->toolButtonExprPC->setVisible(showsValue);
            ui->labelPreviewCalculationUnits->setVisible(showsValue);
            ui->comboBoxPreviewCalculationUnits->setVisible(showsValue);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::LocalUpdateTree()
{
    m_patternDoc->LiteParseIncrements();

    int row = ui->tableWidgetIncrement->currentRow();

    FillIncrements();

    {
        const QSignalBlocker blocker(ui->tableWidgetIncrement);
        ui->tableWidgetIncrement->selectRow(row);
    }

    row = ui->tableWidgetPC->currentRow();

    FillPreviewCalculations();

    {
        const QSignalBlocker blocker(ui->tableWidgetPC);
        ui->tableWidgetPC->selectRow(row);
    }

    m_search->RefreshList(ui->lineEditFind->text());
    m_searchPC->RefreshList(ui->lineEditFindPC->text());
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogIncrements::IncrementUsed(const QString &name) const -> bool
{
    const QVector<VFormulaField> expressions = m_patternDoc->ListExpressions();

    return std::any_of(expressions.constBegin(), expressions.constEnd(),
                       [name](const auto &field)
                       {
                           if (field.expression.indexOf(name) == -1)
                           {
                               return false;
                           }

                           // Eval formula
                           try
                           {
                               QScopedPointer<qmu::QmuTokenParser> const cal(
                                   new qmu::QmuTokenParser(field.expression, false, false));

                               // Tokens (variables, measurements)
                               if (cal->GetTokens().values().contains(name))
                               {
                                   return true;
                               }
                           }
                           catch (const qmu::QmuParserError &)
                           {
                               // Do nothing. Because we not sure if used. A formula is broken.
                           }
                           return false;
                       });
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::CacheRename(const QString &name, const QString &newName)
{
    for (auto &[first, second] : m_renameList)
    {
        if (second == name)
        {
            second = newName;
            return;
        }
    }

    m_renameList.append(qMakePair(name, newName));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::ShowTableIncrementDetails(QTableWidget *table)
{
    QLineEdit *lineEditName = nullptr;
    QPlainTextEdit *plainTextEditDescription = nullptr;
    QPlainTextEdit *plainTextEditFormula = nullptr;
    QLabel *labelCalculatedValue = nullptr;
    QComboBox *comboboxUnits = nullptr;

    if (table == ui->tableWidgetIncrement)
    {
        lineEditName = ui->lineEditName;
        plainTextEditDescription = ui->plainTextEditDescription;
        plainTextEditFormula = ui->plainTextEditFormula;
        labelCalculatedValue = ui->labelCalculatedValue;
        comboboxUnits = ui->comboBoxIncrementUnits;
    }
    else if (table == ui->tableWidgetPC)
    {
        lineEditName = ui->lineEditNamePC;
        plainTextEditDescription = ui->plainTextEditDescriptionPC;
        plainTextEditFormula = ui->plainTextEditFormulaPC;
        labelCalculatedValue = ui->labelCalculatedValuePC;
        comboboxUnits = ui->comboBoxPreviewCalculationUnits;
    }
    else
    {
        return;
    }

    const QString name = RowIncrementName(table, table->currentRow());

    // A group line has nothing to show, and its name is not an increment: the panel is cleared and switched off
    // without ever asking the container, which throws on a name it does not hold. This is where that call used to
    // happen before the try below could catch anything.
    if (name.isEmpty())
    {
        EnableDetails(table, false);
        return;
    }

    if (table->rowCount() > 0 && table->currentRow() != -1)
    {
        EnableDetails(table, true);

        QSharedPointer<VIncrement> incr;

        try
        {
            incr = m_data->GetVariable<VIncrement>(name);
        }
        catch (const VExceptionBadId &e)
        {
            Q_UNUSED(e)
            EnableDetails(table, false);
            return;
        }

        {
            const QSignalBlocker blocker(lineEditName);
            lineEditName->setText(ClearIncrementName(incr->GetName()));
        }

        {
            const QSignalBlocker blocker(comboboxUnits);
            comboboxUnits->setCurrentIndex(comboboxUnits->findData(
                static_cast<int>(incr->IsSpecialUnits() ? IncrUnits::Degrees : IncrUnits::Pattern)));
        }

        {
            const QSignalBlocker blocker(plainTextEditDescription);
            plainTextEditDescription->setPlainText(incr->GetDescription());
        }

        EvalIncrementFormula(incr->GetFormula(), false, incr->GetData(), labelCalculatedValue, incr->IsSpecialUnits());

        const QSignalBlocker blocker(plainTextEditFormula);

        QString const formula
            = VTranslateVars::TryFormulaToUser(incr->GetFormula(),
                                               VAbstractApplication::VApp()->Settings()->GetOsSeparator());

        plainTextEditFormula->setPlainText(formula);
    }
    else
    {
        EnableDetails(table, false);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::OnIncrementCellClicked(int row, int column)
{
    Q_UNUSED(column)

    auto *table = qobject_cast<QTableWidget *>(sender());
    if (table == nullptr)
    {
        return;
    }

    // Which column was hit changes nothing: a group line is a heading, it has nothing to edit, and the whole line
    // answers the click the way the line of a group of measurements does.
    if (RowGroupId(table, row) != NULL_ID)
    {
        ToggleIncrementGroup(table, row);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::ToggleIncrementGroup(QTableWidget *table, int groupRow)
{
    const quint32 groupId = RowGroupId(table, groupRow);
    if (groupId == NULL_ID)
    {
        return;
    }

    const bool collapsed = not m_patternDoc->GetIncrementGroupCollapsed(groupId);

    // Written with the document, so the fold is where the reader left it the next time the pattern is opened — it is
    // the reader's own state, exactly like the folds of the layers, and it is what marks the pattern modified when
    // all that was done is folding a group.
    m_patternDoc->SetIncrementGroupCollapsed(groupId, collapsed);

    if (QTableWidgetItem *item = table->item(groupRow, 0); item != nullptr)
    {
        item->setIcon(GroupCaret(collapsed));
    }

    // Then the rows this line owns: everything down to the next group line, which is exactly what the fill built
    // under it. **Hidden, not removed** — the table keeps every row, so the indices the dialog navigates by, and
    // the selection, do not move.
    for (int row = groupRow + 1; row < table->rowCount(); ++row)
    {
        if (RowGroupId(table, row) != NULL_ID)
        {
            break;
        }
        table->setRowHidden(row, collapsed);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogIncrements::InitVarTypeMenu(QMenu *menu, bool incrementTab) -> QMenu *
{
    if (menu == nullptr)
    {
        menu = new QMenu(this);
    }
    else
    {
        menu->clear();
    }

    QAction *action = menu->addAction(tr("Increment"));
    action->setData(incrementTab); // Increments tab
    connect(action, &QAction::triggered, this, &DialogIncrements::AddIncrement);

    action = menu->addAction(tr("Separator"));
    action->setData(incrementTab); // Increments tab
    connect(action, &QAction::triggered, this, &DialogIncrements::AddSeparator);

    return menu;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::AddNewIncrement(IncrementType type)
{
    qCDebug(vDialog, "Add new increment");

    bool incrementMode = true;
    if (auto *action = qobject_cast<QAction *>(sender()))
    {
        incrementMode = action->data().toBool();
    }
    else if (auto *button = qobject_cast<QToolButton *>(sender()))
    {
        incrementMode = button == ui->toolButtonAdd;
    }
    else
    {
        return;
    }

    QTableWidget *table = incrementMode ? ui->tableWidgetIncrement : ui->tableWidgetPC;

    const QString name = GetCustomName();
    qint32 currentRow = -1;

    // A group line is no anchor: the new increment goes to the end of its table, exactly as when nothing is
    // selected. Anchoring on a heading would file the row outside every group by accident.
    const QString anchor = RowIncrementName(table, table->currentRow());

    if (anchor.isEmpty())
    {
        currentRow = table->rowCount();
        incrementMode ? m_patternDoc->AddEmptyIncrement(name, type)
                      : m_patternDoc->AddEmptyPreviewCalculation(name, type);
    }
    else
    {
        currentRow = table->currentRow() + 1;

        incrementMode ? m_patternDoc->AddEmptyIncrementAfter(anchor, name, type)
                      : m_patternDoc->AddEmptyPreviewCalculationAfter(anchor, name, type);
    }

    m_hasChanges = true;
    LocalUpdateTree();

    table->selectRow(currentRow);
    table->repaint(); // Force repain to fix paint artifacts on Mac OS
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::InitSearch()
{
    VValentinaSettings  const*settings = VAbstractValApplication::VApp()->ValentinaSettings();

    m_search->SetUseUnicodePreperties(settings->GetIncrementsSearchOptionUseUnicodeProperties());
    m_search->SetMatchWord(settings->GetIncrementsSearchOptionWholeWord());
    m_search->SetMatchRegexp(settings->GetIncrementsSearchOptionRegexp());
    m_search->SetMatchCase(settings->GetIncrementsSearchOptionMatchCase());

    m_searchPC->SetUseUnicodePreperties(settings->GetPreviewCalculationsSearchOptionUseUnicodeProperties());
    m_searchPC->SetMatchWord(settings->GetPreviewCalculationsSearchOptionWholeWord());
    m_searchPC->SetMatchRegexp(settings->GetPreviewCalculationsSearchOptionRegexp());
    m_searchPC->SetMatchCase(settings->GetPreviewCalculationsSearchOptionMatchCase());

    ui->lineEditFind->setPlaceholderText(m_search->SearchPlaceholder());
    ui->lineEditFindPC->setPlaceholderText(m_searchPC->SearchPlaceholder());

    UpdateSearchControlsTooltips();

    connect(ui->lineEditFind, &QLineEdit::textEdited, this, [this](const QString &term) { m_search->Find(term); });
    connect(ui->lineEditFind, &QLineEdit::editingFinished, this,
            [this]()
            {
                SaveIncrementsSearchRequest();
                InitIncrementsSearchHistory();
                m_search->Find(ui->lineEditFind->text());
            });
    connect(ui->lineEditFindPC, &QLineEdit::textEdited, this, [this](const QString &term) { m_searchPC->Find(term); });
    connect(ui->lineEditFindPC, &QLineEdit::editingFinished, this,
            [this]()
            {
                SavePreviewCalculationsSearchRequest();
                InitPreviewCalculationsSearchHistory();
                m_searchPC->Find(ui->lineEditFindPC->text());
            });

    connect(ui->toolButtonFindPrevious, &QToolButton::clicked, this,
            [this]()
            {
                SaveIncrementsSearchRequest();
                InitPreviewCalculationsSearchHistory();
                m_search->FindPrevious();
                ui->labelResults->setText(
                    QStringLiteral("%1/%2").arg(m_search->MatchIndex() + 1).arg(m_search->MatchCount()));
            });
    connect(ui->toolButtonFindPreviousPC, &QToolButton::clicked, this,
            [this]()
            {
                SavePreviewCalculationsSearchRequest();
                InitPreviewCalculationsSearchHistory();
                m_searchPC->FindPrevious();
                ui->labelResultsPC->setText(
                    QStringLiteral("%1/%2").arg(m_searchPC->MatchIndex() + 1).arg(m_searchPC->MatchCount()));
            });
    connect(ui->toolButtonFindNext, &QToolButton::clicked, this,
            [this]()
            {
                SaveIncrementsSearchRequest();
                InitIncrementsSearchHistory();
                m_search->FindNext();
                ui->labelResults->setText(
                    QStringLiteral("%1/%2").arg(m_search->MatchIndex() + 1).arg(m_search->MatchCount()));
            });
    connect(ui->toolButtonFindNextPC, &QToolButton::clicked, this,
            [this]()
            {
                SavePreviewCalculationsSearchRequest();
                InitPreviewCalculationsSearchHistory();
                m_searchPC->FindNext();
                ui->labelResultsPC->setText(
                    QStringLiteral("%1/%2").arg(m_searchPC->MatchIndex() + 1).arg(m_searchPC->MatchCount()));
            });

    connect(m_search.data(), &VTableSearch::HasResult, this,
            [this](bool state)
            {
                ui->toolButtonFindPrevious->setEnabled(state);
                ui->toolButtonFindNext->setEnabled(state);

                if (state)
                {
                    ui->labelResults->setText(
                        QStringLiteral("%1/%2").arg(m_search->MatchIndex() + 1).arg(m_search->MatchCount()));
                }
                else
                {
                    ui->labelResults->setText(tr("0 results"));
                }

                QPalette palette;

                if (not state && not ui->lineEditFind->text().isEmpty())
                {
                    palette.setColor(QPalette::Text, Qt::red);
                    ui->lineEditFind->setPalette(palette);

                    palette.setColor(QPalette::Active, ui->labelResults->foregroundRole(), Qt::red);
                    palette.setColor(QPalette::Inactive, ui->labelResults->foregroundRole(), Qt::red);
                    ui->labelResults->setPalette(palette);
                }
                else
                {
                    ui->lineEditFind->setPalette(palette);
                    ui->labelResults->setPalette(palette);
                }
            });

    connect(m_searchPC.data(), &VTableSearch::HasResult, this,
            [this](bool state)
            {
                ui->toolButtonFindPreviousPC->setEnabled(state);
                ui->toolButtonFindNextPC->setEnabled(state);

                if (state)
                {
                    ui->labelResultsPC->setText(
                        QStringLiteral("%1/%2").arg(m_searchPC->MatchIndex() + 1).arg(m_searchPC->MatchCount()));
                }
                else
                {
                    ui->labelResultsPC->setText(tr("0 results"));
                }

                QPalette palette;

                if (not state && not ui->lineEditFindPC->text().isEmpty())
                {
                    palette.setColor(QPalette::Text, Qt::red);
                    ui->lineEditFindPC->setPalette(palette);

                    palette.setColor(QPalette::Active, ui->labelResultsPC->foregroundRole(), Qt::red);
                    palette.setColor(QPalette::Inactive, ui->labelResultsPC->foregroundRole(), Qt::red);
                    ui->labelResultsPC->setPalette(palette);
                }
                else
                {
                    ui->lineEditFindPC->setPalette(palette);
                    ui->labelResultsPC->setPalette(palette);
                }
            });

    connect(ui->toolButtonCaseSensitive, &QToolButton::toggled, this,
            [this](bool checked)
            {
                m_search->SetMatchCase(checked);
                m_search->Find(ui->lineEditFind->text());
                ui->lineEditFind->setPlaceholderText(m_search->SearchPlaceholder());
            });

    connect(ui->toolButtonCaseSensitivePC, &QToolButton::toggled, this,
            [this](bool checked)
            {
                m_searchPC->SetMatchCase(checked);
                m_searchPC->Find(ui->lineEditFindPC->text());
                ui->lineEditFindPC->setPlaceholderText(m_searchPC->SearchPlaceholder());
            });

    connect(ui->toolButtonWholeWord, &QToolButton::toggled, this,
            [this](bool checked)
            {
                m_search->SetMatchWord(checked);
                m_search->Find(ui->lineEditFind->text());
                ui->lineEditFind->setPlaceholderText(m_search->SearchPlaceholder());
            });

    connect(ui->toolButtonWholeWordPC, &QToolButton::toggled, this,
            [this](bool checked)
            {
                m_searchPC->SetMatchWord(checked);
                m_searchPC->Find(ui->lineEditFindPC->text());
                ui->lineEditFindPC->setPlaceholderText(m_searchPC->SearchPlaceholder());
            });

    connect(ui->toolButtonRegexp, &QToolButton::toggled, this,
            [this](bool checked)
            {
                m_search->SetMatchRegexp(checked);

                if (checked)
                {
                    {
                        const QSignalBlocker blocker(ui->toolButtonWholeWord);
                        ui->toolButtonWholeWord->setChecked(false);
                    }
                    ui->toolButtonWholeWord->setEnabled(false);

                    ui->toolButtonUseUnicodeProperties->setEnabled(true);
                }
                else
                {
                    ui->toolButtonWholeWord->setEnabled(true);
                    {
                        const QSignalBlocker blocker(ui->toolButtonUseUnicodeProperties);
                        ui->toolButtonUseUnicodeProperties->setChecked(false);
                    }
                    ui->toolButtonUseUnicodeProperties->setEnabled(false);
                }
                m_search->Find(ui->lineEditFind->text());
                ui->lineEditFind->setPlaceholderText(m_search->SearchPlaceholder());
            });

    connect(ui->toolButtonRegexpPC, &QToolButton::toggled, this,
            [this](bool checked)
            {
                m_searchPC->SetMatchRegexp(checked);

                if (checked)
                {
                    {
                        const QSignalBlocker blocker(ui->toolButtonWholeWordPC);
                        ui->toolButtonWholeWordPC->setChecked(false);
                    }
                    ui->toolButtonWholeWordPC->setEnabled(false);

                    ui->toolButtonUseUnicodePropertiesPC->setEnabled(true);
                }
                else
                {
                    ui->toolButtonWholeWordPC->setEnabled(true);
                    {
                        const QSignalBlocker blocker(ui->toolButtonUseUnicodePropertiesPC);
                        ui->toolButtonUseUnicodePropertiesPC->setChecked(false);
                    }
                    ui->toolButtonUseUnicodePropertiesPC->setEnabled(false);
                }
                m_searchPC->Find(ui->lineEditFindPC->text());
                ui->lineEditFindPC->setPlaceholderText(m_searchPC->SearchPlaceholder());
            });

    connect(ui->toolButtonUseUnicodeProperties, &QToolButton::toggled, this,
            [this](bool checked)
            {
                m_search->SetUseUnicodePreperties(checked);
                m_search->Find(ui->lineEditFind->text());
            });

    connect(ui->toolButtonUseUnicodePropertiesPC, &QToolButton::toggled, this,
            [this](bool checked)
            {
                m_searchPC->SetUseUnicodePreperties(checked);
                m_searchPC->Find(ui->lineEditFindPC->text());
            });

    m_searchHistory->setStyleSheet(QStringLiteral("QMenu { menu-scrollable: 1; }"));
    m_searchHistoryPC->setStyleSheet(QStringLiteral("QMenu { menu-scrollable: 1; }"));

    InitIncrementsSearchHistory();
    InitPreviewCalculationsSearchHistory();

    ui->pushButtonSearch->setMenu(m_searchHistory);
    ui->pushButtonSearch->setProperty("hasMenu", true);
    ui->pushButtonSearch->style()->unpolish(ui->pushButtonSearch);
    ui->pushButtonSearch->style()->polish(ui->pushButtonSearch);

    ui->pushButtonSearchPC->setMenu(m_searchHistoryPC);
    ui->pushButtonSearchPC->setProperty("hasMenu", true);
    ui->pushButtonSearchPC->style()->unpolish(ui->pushButtonSearchPC);
    ui->pushButtonSearchPC->style()->polish(ui->pushButtonSearchPC);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::InitIncrementsSearchHistory()
{
    QStringList const searchHistory =
        VAbstractValApplication::VApp()->ValentinaSettings()->GetIncrementsSearchHistory();
    m_searchHistory->clear();

    if (searchHistory.isEmpty())
    {
        QAction *action = m_searchHistory->addAction('<'_L1 + tr("Empty", "list") + '>'_L1);
        action->setDisabled(true);
        return;
    }

    for (const auto &term : searchHistory)
    {
        QAction *action = m_searchHistory->addAction(term);
        action->setData(term);
        connect(action, &QAction::triggered, this,
                [this]()
                {
                    auto *action = qobject_cast<QAction *>(sender());
                    if (action != nullptr)
                    {
                        QString const termText = action->data().toString();
                        ui->lineEditFind->setText(termText);
                        m_search->Find(termText);
                        ui->lineEditFind->setFocus();
                    }
                });
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::InitPreviewCalculationsSearchHistory()
{
    QStringList const searchHistory =
        VAbstractValApplication::VApp()->ValentinaSettings()->GetPreviewCalculationsSearchHistory();
    m_searchHistoryPC->clear();

    if (searchHistory.isEmpty())
    {
        QAction *action = m_searchHistoryPC->addAction('<'_L1 + tr("Empty", "list") + '>'_L1);
        action->setDisabled(true);
        return;
    }

    for (const auto &term : searchHistory)
    {
        QAction *action = m_searchHistoryPC->addAction(term);
        action->setData(term);
        connect(action, &QAction::triggered, this,
                [this]()
                {
                    auto *action = qobject_cast<QAction *>(sender());
                    if (action != nullptr)
                    {
                        QString const termText = action->data().toString();
                        ui->lineEditFindPC->setText(termText);
                        m_searchPC->Find(termText);
                        ui->lineEditFindPC->setFocus();
                    }
                });
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::SaveIncrementsSearchRequest()
{
    QStringList searchHistory = VAbstractValApplication::VApp()->ValentinaSettings()->GetIncrementsSearchHistory();
    QString const term = ui->lineEditFind->text();
    if (term.isEmpty())
    {
        return;
    }

    searchHistory.removeAll(term);
    searchHistory.prepend(term);
    while (searchHistory.size() > VTableSearch::MaxHistoryRecords)
    {
        searchHistory.removeLast();
    }
    VAbstractValApplication::VApp()->ValentinaSettings()->SetIncrementsSearchHistory(searchHistory);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::SavePreviewCalculationsSearchRequest()
{
    QStringList searchHistory =
        VAbstractValApplication::VApp()->ValentinaSettings()->GetPreviewCalculationsSearchHistory();
    QString const term = ui->lineEditFindPC->text();
    if (term.isEmpty())
    {
        return;
    }

    searchHistory.removeAll(term);
    searchHistory.prepend(term);
    while (searchHistory.size() > VTableSearch::MaxHistoryRecords)
    {
        searchHistory.removeLast();
    }
    VAbstractValApplication::VApp()->ValentinaSettings()->SetPreviewCalculationsSearchHistory(searchHistory);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::UpdateSearchControlsTooltips()
{
    auto UpdateToolTip = [this](QAbstractButton *button)
    {
        if (button->toolTip().contains("%1"_L1))
        {
            m_serachButtonTooltips.insert(button, button->toolTip());
            button->setToolTip(button->toolTip().arg(VModifierKey::Sequence(button->shortcut())));
        }
        else if (m_serachButtonTooltips.contains(button))
        {
            QString const tooltip = m_serachButtonTooltips.value(button);
            button->setToolTip(tooltip.arg(VModifierKey::Sequence(button->shortcut())));
        }
    };

    UpdateToolTip(ui->toolButtonCaseSensitive);
    UpdateToolTip(ui->toolButtonWholeWord);
    UpdateToolTip(ui->toolButtonRegexp);
    UpdateToolTip(ui->toolButtonUseUnicodeProperties);
    UpdateToolTip(ui->pushButtonSearch);
    UpdateToolTip(ui->toolButtonFindPrevious);
    UpdateToolTip(ui->toolButtonFindNext);

    UpdateToolTip(ui->toolButtonCaseSensitivePC);
    UpdateToolTip(ui->toolButtonWholeWordPC);
    UpdateToolTip(ui->toolButtonRegexpPC);
    UpdateToolTip(ui->toolButtonUseUnicodePropertiesPC);
    UpdateToolTip(ui->pushButtonSearchPC);
    UpdateToolTip(ui->toolButtonFindPreviousPC);
    UpdateToolTip(ui->toolButtonFindNextPC);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::InitIncrementUnits(QComboBox *combo)
{
    SCASSERT(combo != nullptr)

    const QSignalBlocker blocker(combo);

    int current = -1;
    if (combo->currentIndex() != -1)
    {
        current = combo->currentData().toInt();
    }

    QString units;
    switch (VAbstractValApplication::VApp()->patternUnits())
    {
        case Unit::Mm:
            units = tr("Millimeters");
            break;
        case Unit::Inch:
            units = tr("Inches");
            break;
        case Unit::Cm:
            units = tr("Centimeters");
            break;
        default:
            units = QStringLiteral("<Invalid>");
            break;
    }

    combo->clear();
    combo->addItem(units, QVariant(static_cast<int>(IncrUnits::Pattern)));
    combo->addItem(tr("Degrees"), QVariant(static_cast<int>(IncrUnits::Degrees)));

    if (int const i = combo->findData(current); i != -1)
    {
        combo->setCurrentIndex(i);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::InitIcons()
{
    const auto resource = QStringLiteral("icon");
    const auto fxIcon = QStringLiteral("24x24/fx.png");

    ui->toolButtonExpr->setIcon(VTheme::GetIconResource(resource, fxIcon));
    ui->toolButtonExprPC->setIcon(VTheme::GetIconResource(resource, fxIcon));
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogIncrements::CheckIncrementName(const QString &name, const QSet<QString> &importedNames) const -> QString
{
    if (name.isEmpty())
    {
        throw VException(tr("Increment name is empty."));
    }

    if (importedNames.contains(name))
    {
        throw VException(tr("Imported file must not contain the same name twice."));
    }

    if (QRegularExpression const rx(NameRegExp()); not rx.match(name).hasMatch())
    {
        throw VException(tr("Increment '%1' doesn't match regex pattern.").arg(name));
    }

    if (not m_data->IsUnique(name))
    {
        throw VException(tr("Increment '%1' already present in the file.").arg(name));
    }

    return name;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::ShowError(const QString &text)
{
    QMessageBox messageBox(this);
    messageBox.setIcon(QMessageBox::Critical);
    messageBox.setText(text);
    messageBox.setStandardButtons(QMessageBox::Ok);
    messageBox.setDefaultButton(QMessageBox::Ok);
    messageBox.exec();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogIncrements::IncrementsCSVColumnHeader(int column) const -> QString
{
    switch (column)
    {
        case 0: // name
            return tr("Name", "increment column");
        case 1: // calculated value
            return tr("Calculated value", "increment column");
        case 2: // formula
            return tr("Formula", "increment column");
        case 3: // description
            return tr("Description", "increment column");
        default:
            return {};
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::ImportCSVIncrements(bool incrementMode,
                                           const QxtCsvModel &csv,
                                           const QVector<int> &map,
                                           bool withHeader)
{
    const int columns = csv.columnCount();
    const int rows = csv.rowCount();

    if (columns < 2)
    {
        ShowError(tr("Increments require at least 2 columns."));
        return;
    }

    QVector<IncrementData> increments;
    QSet<QString> importedNames;

    increments.reserve(rows);

    for (int i = 0; i < rows; ++i)
    {
        try
        {
            const int nameColumn = map.at(static_cast<int>(IncrementsColumns::Name));
            QString name = csv.text(i, nameColumn).simplified();
            if (name.isEmpty())
            {
                ShowError(tr("Error in row %1. The increment name is empty.").arg(i));
                continue;
            }

            if (!name.startsWith(CustomIncrSign))
            {
                name.prepend(CustomIncrSign);
            }

            IncrementData increment;
            const QString iName = CheckIncrementName(name, importedNames);
            importedNames.insert(iName);
            increment.name = iName;

            const int valueColumn = map.at(static_cast<int>(IncrementsColumns::Value));
            QString rawValue = csv.text(i, valueColumn);

            if (rawValue.endsWith(degreeSymbol))
            {
                increment.specialUnits = true;
                RemoveLast(rawValue);
            }

            increment.value
                = VTranslateVars::TryFormulaFromUser(rawValue,
                                                     VAbstractApplication::VApp()->Settings()->GetOsSeparator());

            SetIncrementDescription(i, csv, map, increment);

            increments.append(increment);
        }
        catch (VException &e)
        {
            int rowIndex = i + 1;
            if (withHeader)
            {
                ++rowIndex;
            }
            ShowError(tr("Error in row %1. %2").arg(rowIndex).arg(e.ErrorMessage()));
            return;
        }
    }

    QTableWidget *table = incrementMode ? ui->tableWidgetIncrement : ui->tableWidgetPC;
    qint32 const currentRow = table->rowCount();

    for (const auto &icr : std::as_const(increments))
    {
        incrementMode ? m_patternDoc->AddEmptyIncrement(icr.name, IncrementType::Increment)
                      : m_patternDoc->AddEmptyPreviewCalculation(icr.name, IncrementType::Increment);

        m_patternDoc->SetIncrementFormula(icr.name, icr.value);

        if (icr.specialUnits)
        {
            m_patternDoc->SetIncrementSpecialUnits(icr.name, icr.specialUnits);
        }

        if (not icr.description.isEmpty())
        {
            m_patternDoc->SetIncrementDescription(icr.name, icr.description);
        }
    }

    m_hasChanges = true;
    LocalUpdateTree();

    table->selectRow(currentRow);
    table->repaint(); // Force repain to fix paint artifacts on Mac OS
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::ExportCSVIncrements(
    bool incrementMode, const QString &fileName, bool withHeader, int mib, const QChar &separator)
{
    QxtCsvModel csv;
    int const columns = 4;

    for (int column = 0; column < columns; ++column)
    {
        csv.insertSingleColumn(column);
    }

    if (withHeader)
    {
        for (int column = 0; column < columns; ++column)
        {
            csv.setHeaderText(column, IncrementsCSVColumnHeader(column));
        }
    }

    QMap<QString, QSharedPointer<VIncrement>> const increments = m_data->DataIncrements();
    QMap<quint32, QString> map;
    // Sorting QHash by id
    for (auto i = increments.constBegin(); i != increments.constEnd(); ++i)
    {
        if (const QSharedPointer<VIncrement> &incr = i.value(); incrementMode != incr->IsPreviewCalculation())
        {
            map.insert(incr->GetIndex(), i.key());
        }
    }

    int row = 0;
    for (auto iMap = map.constBegin(); iMap != map.constEnd(); ++iMap)
    {
        const QSharedPointer<VIncrement> &incr = increments.value(iMap.value());

        csv.insertSingleRow(row);

        csv.setText(row, 0, incr->GetName());

        QString calculatedValue = locale().toString(*incr->GetValue());
        if (incr->IsSpecialUnits())
        {
            calculatedValue = calculatedValue + degreeSymbol;
        }
        csv.setText(row, 1, calculatedValue);

        QString const formula
            = VTranslateVars::TryFormulaToUser(incr->GetFormula(),
                                               VAbstractApplication::VApp()->Settings()->GetOsSeparator());
        csv.setText(row, 2, formula);

        csv.setText(row, 3, incr->GetDescription());

        ++row;
    }

    QString error;
    csv.toCSV(fileName, error, withHeader, separator, QTextCodec::codecForMib(mib));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief FullUpdateFromFile update information in tables form file
 */
void DialogIncrements::FullUpdateFromFile()
{
    m_hasChanges = false;

    const int incrementRow = ui->tableWidgetIncrement->currentRow();
    const int pcRow = ui->tableWidgetPC->currentRow();

    ui->tableWidgetLines->clearContents();
    ui->tableWidgetSplines->clearContents();
    ui->tableWidgetAnglesCurves->clearContents();
    ui->tableWidgetLinesAngles->clearContents();
    ui->tableWidgetRadiusesArcs->clearContents();

    m_completeData = m_patternDoc->GetCompleteData();

    FillIncrements();
    FillPreviewCalculations();
    FillLengthsLines();
    FillLengthLinesAngles();
    FillLengthsCurves();
    FillCurvesCLengths();
    FillRadiusesArcs();
    FillAnglesCurves();

    m_search->RefreshList(ui->lineEditFind->text());
    m_searchPC->RefreshList(ui->lineEditFindPC->text());

    ui->tableWidgetIncrement->selectRow(incrementRow);
    ui->tableWidgetPC->selectRow(pcRow);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::RefreshPattern()
{
    if (m_hasChanges)
    {
        QVector<VFormulaField> expressions = m_patternDoc->ListExpressions();
        for (const auto &[first, second] : std::as_const(m_renameList))
        {
            m_patternDoc->ReplaceNameInFormula(expressions, first, second);
        }
        m_renameList.clear();

        const int row = ui->tableWidgetIncrement->currentRow();
        const int rowPC = ui->tableWidgetPC->currentRow();

        m_patternDoc->LiteParseTree(Document::FullLiteParse);

        {
            const QSignalBlocker blocker(ui->tableWidgetIncrement);
            ui->tableWidgetIncrement->selectRow(row);
        }

        {
            const QSignalBlocker blocker(ui->tableWidgetPC);
            ui->tableWidgetPC->selectRow(rowPC);
        }

        m_hasChanges = false;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::UpdateShortcuts()
{
    if (VAbstractShortcutManager *manager = VAbstractApplication::VApp()->GetShortcutManager())
    {
        manager->UpdateButtonShortcut(m_shortcuts);
        UpdateSearchControlsTooltips();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::ImportIncrements()
{
    const QString filters = tr("Comma-Separated Values") + QStringLiteral(" (*.csv)");
    const auto suffix = QStringLiteral("csv");

    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Import from CSV"),
                                                    QDir::homePath(),
                                                    filters,
                                                    nullptr,
                                                    VAbstractApplication::VApp()->NativeFileDialog());

    if (fileName.isEmpty())
    {
        return;
    }

    if (QFileInfo const f(fileName); f.suffix().isEmpty() && f.suffix() != suffix)
    {
        fileName += '.'_L1 + suffix;
    }

    DialogExportToCSV dialog(this);
    dialog.SetWithHeader(VAbstractApplication::VApp()->Settings()->GetCSVWithHeader());
    dialog.SetSelectedMib(VAbstractApplication::VApp()->Settings()->GetCSVCodec());
    dialog.SetSeparator(VAbstractApplication::VApp()->Settings()->GetCSVSeparator());
    dialog.ShowFilePreview(fileName);

    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    VAbstractApplication::VApp()->Settings()->SetCSVSeparator(dialog.GetSeparator());
    VAbstractApplication::VApp()->Settings()->SetCSVCodec(dialog.GetSelectedMib());
    VAbstractApplication::VApp()->Settings()->SetCSVWithHeader(dialog.IsWithHeader());

    DialogIncrementsCSVColumns columns(fileName, this);
    columns.SetWithHeader(dialog.IsWithHeader());
    columns.SetSeparator(dialog.GetSeparator());
    columns.SetCodec(QTextCodec::codecForMib(dialog.GetSelectedMib()));

    if (columns.exec() != QDialog::Accepted)
    {
        return;
    }

    QxtCsvModel const csv(fileName,
                          nullptr,
                          dialog.IsWithHeader(),
                          dialog.GetSeparator(),
                          QTextCodec::codecForMib(dialog.GetSelectedMib()));
    const QVector<int> map = columns.ColumnsMap();

    if (auto *button = qobject_cast<QToolButton *>(sender()); button == ui->toolButtonImportIncrementsCSV)
    {
        ImportCSVIncrements(true, csv, map, dialog.IsWithHeader());
    }
    else if (button == ui->toolButtonImportIncrementsCSVPC)
    {
        ImportCSVIncrements(false, csv, map, dialog.IsWithHeader());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::ExportIncrements()
{
    const QString filters = tr("Comma-Separated Values") + " (*.csv)"_L1;
    const QString suffix("csv"_L1);
    const QString path = QDir::homePath() + '/'_L1 + tr("values") + '.'_L1 + suffix;

    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Export to CSV"),
                                                    path,
                                                    filters,
                                                    nullptr,
                                                    VAbstractApplication::VApp()->NativeFileDialog());

    if (fileName.isEmpty())
    {
        return;
    }

    if (QFileInfo const f(fileName); f.suffix().isEmpty() && f.suffix() != suffix)
    {
        fileName += '.'_L1 + suffix;
    }

    VCommonSettings  const*settings = VAbstractApplication::VApp()->Settings();

    DialogExportToCSV dialog(this);
    dialog.SetWithHeader(settings->GetCSVWithHeader());
    dialog.SetSelectedMib(settings->GetCSVCodec());
    dialog.SetSeparator(settings->GetCSVSeparator());

    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    if (auto *button = qobject_cast<QToolButton *>(sender()); button == ui->toolButtonExportIncrementsCSV)
    {
        ExportCSVIncrements(true, fileName, dialog.IsWithHeader(), dialog.GetSelectedMib(), dialog.GetSeparator());
    }
    else if (button == ui->toolButtonExportIncrementsCSVPC)
    {
        ExportCSVIncrements(false, fileName, dialog.IsWithHeader(), dialog.GetSelectedMib(), dialog.GetSeparator());
    }

    settings->SetCSVSeparator(dialog.GetSeparator());
    settings->SetCSVCodec(dialog.GetSelectedMib());
    settings->SetCSVWithHeader(dialog.IsWithHeader());
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::FillIncrementsTable(QTableWidget *table,
                                           const QMap<QString, QSharedPointer<VIncrement>> &increments,
                                           bool takePreviewCalculations)
{
    SCASSERT(table != nullptr)

    const QSignalBlocker blocker(table);
    table->clearContents();

    QMap<quint32, QString> map;
    // Sorting QHash by id
    for (auto i = increments.constBegin(); i != increments.constEnd(); ++i)
    {
        const QSharedPointer<VIncrement> &incr = i.value();
        if (takePreviewCalculations == incr->IsPreviewCalculation())
        {
            map.insert(incr->GetIndex(), i.key());
        }
    }

    // What this table shows of the groups **of its own table**, read once. The rows keep the order of the document,
    // and a group is a heading in front of the rows it holds — the shape a group of measurements has. Asking the
    // document row by row would walk it once per row, and this table is rebuilt on every keystroke of the panel
    // below, so the walk would be paid again on each character typed.
    const QMap<quint32, VIncrementGroupData> groups = m_patternDoc->GetIncrementGroups(takePreviewCalculations);
    const QMap<QString, quint32> rowGroups = m_patternDoc->IncrementRowGroups(takePreviewCalculations);

    // A row naming a group the document does not hold any more is a loose row, exactly as the model answers it.
    auto GroupOf = [&groups, &rowGroups](const QString &name) -> quint32
    {
        const quint32 id = rowGroups.value(name, NULL_ID);
        return groups.contains(id) ? id : NULL_ID;
    };

    struct Row
    {
        RowKind kind;
        QString name;                          // the group's name on a heading, the increment's own otherwise
        QSharedPointer<VIncrement> increment;  // none on a heading
        quint32 groupId{NULL_ID};
    };

    // Which rows belong to which group, gathered once — NULL_ID holding the rows no group holds.
    QMap<quint32, QVector<QSharedPointer<VIncrement>>> byGroup;
    for (auto i = map.constBegin(); i != map.constEnd(); ++i)
    {
        const QSharedPointer<VIncrement> &incr = increments.value(i.value());
        byGroup[GroupOf(incr->GetName())].append(incr);
    }

    QVector<Row> rows;

    // **The rows no group holds come first**, in the order of the document: a group line owns everything that
    // follows it, so a row outside every group has nowhere else to be.
    for (const QSharedPointer<VIncrement> &incr : byGroup.value(NULL_ID))
    {
        rows.append({RowKind::Increment, incr->GetName(), incr, NULL_ID});
    }

    // Then each group followed by **its own** rows, in the order of their ids — the order the document created
    // them, one import after another. A group no row refers to is shown all the same: the file holds it and the
    // rows point at it, so leaving it out would hide a state the file really has.
    for (auto it = groups.constBegin(); it != groups.constEnd(); ++it)
    {
        rows.append({RowKind::Group, it.value().name, {}, it.key()});

        for (const QSharedPointer<VIncrement> &incr : byGroup.value(it.key()))
        {
            rows.append({RowKind::Increment, incr->GetName(), incr, it.key()});
        }
    }

    table->setRowCount(static_cast<int>(rows.size()));

    bool folded = false;

    for (int row = 0; row < rows.size(); ++row)
    {
        const Row &desc = rows.at(row);

        // A span set by an earlier fill outlives the items it covered — the dialog found that out with the
        // separators. Every row takes its own back to one column before any of them sets one.
        if (table->columnSpan(row, 1) > 1)
        {
            table->setSpan(row, 1, 1, 1);
        }

        if (desc.kind == RowKind::Group)
        {
            AddGroupCell(table, desc.groupId, desc.name, groups.value(desc.groupId).collapsed, row);

            // Everything this line owns is hidden while it is folded. The heading stays — what it hides is one
            // gesture away — and the rows are hidden rather than left out, so no index moves under the reader.
            folded = groups.value(desc.groupId).collapsed;
            continue;
        }

        table->setRowHidden(row, folded);

        const QSharedPointer<VIncrement> &incr = desc.increment;

        if (incr->GetType() == VarType::Increment)
        {
            AddCell(table, incr->GetName(), row, 0, Qt::AlignVCenter); // name

            QString calculatedValue = VAbstractApplication::VApp()->LocaleToString(*incr->GetValue());
            if (incr->IsSpecialUnits())
            {
                calculatedValue = calculatedValue + degreeSymbol;
            }
            AddCell(table, calculatedValue, row, 1, Qt::AlignCenter, incr->IsFormulaOk()); // calculated value

            QString const formula = VTranslateVars::TryFormulaToUser(
                incr->GetFormula(), VAbstractApplication::VApp()->Settings()->GetOsSeparator());

            AddCell(table, formula, row, 2, Qt::AlignVCenter); // formula
        }
        else if (incr->GetType() == VarType::IncrementSeparator)
        {
            AddSeparatorCell(table, incr->GetName(), row, 0, Qt::AlignVCenter);    // name
            AddCell(table, incr->GetDescription(), row, 1, Qt::AlignCenter);       // description
            table->setSpan(row, 1, 1, 2);
        }
    }

    table->horizontalHeader()->setStretchLastSection(true);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief clickedToolButtonAdd create new row in table
 */
void DialogIncrements::AddIncrement()
{
    AddNewIncrement(IncrementType::Increment);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::AddSeparator()
{
    AddNewIncrement(IncrementType::Separator);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief clickedToolButtonRemove remove one row from table
 */
void DialogIncrements::RemoveIncrement()
{
    auto *button = qobject_cast<QToolButton *>(sender());

    QTableWidget *table = nullptr;

    if (button == ui->toolButtonRemove)
    {
        table = ui->tableWidgetIncrement;
    }
    else if (button == ui->toolButtonRemovePC)
    {
        table = ui->tableWidgetPC;
    }
    else
    {
        return;
    }

    const int row = table->currentRow();

    if (row == -1)
    {
        return;
    }

    // A group line is not a row of the table of variables: there is nothing to take out.
    const QString name = RowIncrementName(table, row);
    if (name.isEmpty())
    {
        return;
    }

    if (button == ui->toolButtonRemove)
    {
        m_patternDoc->RemoveIncrement(name);
    }
    else if (button == ui->toolButtonRemovePC)
    {
        m_patternDoc->RemovePreviewCalculation(name);
    }

    m_hasChanges = true;
    LocalUpdateTree();

    table->rowCount() > 0 ? table->selectRow(0) : EnableDetails(table, false);
    table->repaint(); // Force repain to fix paint artifacts on Mac OS X
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::MoveUp()
{
    auto *button = qobject_cast<QToolButton *>(sender());

    QTableWidget *table = nullptr;

    if (button == ui->toolButtonUp)
    {
        table = ui->tableWidgetIncrement;
    }
    else if (button == ui->toolButtonUpPC)
    {
        table = ui->tableWidgetPC;
    }
    else
    {
        return;
    }

    const int row = table->currentRow();

    if (row == -1)
    {
        return;
    }

    // A group line is not a row of the table of variables: there is nothing to move.
    const QString name = RowIncrementName(table, row);
    if (name.isEmpty())
    {
        return;
    }

    if (button == ui->toolButtonUp)
    {
        m_patternDoc->MoveUpIncrement(name);
    }
    else if (button == ui->toolButtonUpPC)
    {
        m_patternDoc->MoveUpPreviewCalculation(name);
    }

    m_hasChanges = true;
    LocalUpdateTree();

    // Found by its own name, not by arithmetic on the index it had: a group's heading may sit just above the row,
    // and what the reader must be left looking at is the row, not whatever landed one place higher.
    if (const int moved = RowOfIncrement(table, name); moved != -1)
    {
        table->selectRow(moved);
    }
    table->repaint(); // Force repain to fix paint artifacts on Mac OS X
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::MoveDown()
{
    auto *button = qobject_cast<QToolButton *>(sender());

    QTableWidget *table = nullptr;

    if (button == ui->toolButtonDown)
    {
        table = ui->tableWidgetIncrement;
    }
    else if (button == ui->toolButtonDownPC)
    {
        table = ui->tableWidgetPC;
    }
    else
    {
        return;
    }

    const int row = table->currentRow();

    if (row == -1)
    {
        return;
    }

    // A group line is not a row of the table of variables: there is nothing to move.
    const QString name = RowIncrementName(table, row);
    if (name.isEmpty())
    {
        return;
    }

    if (button == ui->toolButtonDown)
    {
        m_patternDoc->MoveDownIncrement(name);
    }
    else if (button == ui->toolButtonDownPC)
    {
        m_patternDoc->MoveDownPreviewCalculation(name);
    }

    m_hasChanges = true;
    LocalUpdateTree();

    // Found by its own name, for the same reason as above.
    if (const int moved = RowOfIncrement(table, name); moved != -1)
    {
        table->selectRow(moved);
    }
    table->repaint(); // Force repain to fix paint artifacts on Mac OS X
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::SaveIncrName(const QString &text)
{
    auto *lineEdit = qobject_cast<QLineEdit *>(sender());

    QTableWidget *table = nullptr;

    if (lineEdit == ui->lineEditName)
    {
        table = ui->tableWidgetIncrement;
    }
    else if (lineEdit == ui->lineEditNamePC)
    {
        table = ui->tableWidgetPC;
    }
    else
    {
        return;
    }

    const int row = table->currentRow();

    if (row == -1)
    {
        return;
    }

    // A group line is not an increment: there is nothing to rename, and no name of its own to rewrite in the
    // formulas — a group's name is the instance's, which no formula reads.
    const QString name = RowIncrementName(table, row);
    if (name.isEmpty())
    {
        return;
    }

    QTableWidgetItem *nameField = table->item(row, 0);

    QString newName = text.isEmpty() ? GetCustomName() : CustomIncrSign + text;
    bool updateFlag = not text.isEmpty();

    if (not m_data->IsUnique(newName))
    {
        updateFlag = false;
        qint32 num = 2;
        QString name = newName;
        do
        {
            name = name + '_'_L1 + QString::number(num);
            num++;
        } while (not m_data->IsUnique(name));
        newName = name;
    }

    m_patternDoc->SetIncrementName(name, newName);
    QVector<VFormulaField> expression = m_patternDoc->ListIncrementExpressions();
    m_patternDoc->ReplaceNameInFormula(expression, name, newName);

    CacheRename(name, newName);

    m_hasChanges = true;
    if (updateFlag)
    {
        LocalUpdateTree();
    }
    else
    {
        nameField->setText(newName);
        m_patternDoc->LiteParseIncrements();
    }

    const QSignalBlocker blocker(table);
    table->selectRow(row);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::SaveIncrUnits()
{
    auto *combo = qobject_cast<QComboBox *>(sender());

    QTableWidget *table = nullptr;

    if (combo == ui->comboBoxIncrementUnits)
    {
        table = ui->tableWidgetIncrement;
    }
    else if (combo == ui->comboBoxPreviewCalculationUnits)
    {
        table = ui->tableWidgetPC;
    }
    else
    {
        return;
    }

    const int row = table->currentRow();

    if (row == -1)
    {
        return;
    }

    // A group line is not an increment: there is nothing to switch to degrees.
    const QString name = RowIncrementName(table, row);
    if (name.isEmpty())
    {
        return;
    }

    const auto units = static_cast<IncrUnits>(combo->currentData().toInt());
    m_patternDoc->SetIncrementSpecialUnits(name, units == IncrUnits::Degrees);
    LocalUpdateTree();

    {
        const QSignalBlocker blocker(table);
        table->selectRow(row);
    }

    ShowTableIncrementDetails(table);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::SaveIncrDescription()
{
    auto *textEdit = qobject_cast<QPlainTextEdit *>(sender());

    QTableWidget *table = nullptr;

    if (textEdit == ui->plainTextEditDescription)
    {
        table = ui->tableWidgetIncrement;
    }
    else if (textEdit == ui->plainTextEditDescriptionPC)
    {
        table = ui->tableWidgetPC;
    }
    else
    {
        return;
    }

    const int row = table->currentRow();

    if (row == -1)
    {
        return;
    }

    const QTextCursor cursor = textEdit->textCursor();

    // A group line is not an increment: there is nothing to describe.
    const QString name = RowIncrementName(table, row);
    if (name.isEmpty())
    {
        return;
    }

    m_patternDoc->SetIncrementDescription(name, textEdit->toPlainText());
    LocalUpdateTree();

    {
        const QSignalBlocker blocker(table);
        table->selectRow(row);
    }
    textEdit->setTextCursor(cursor);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::SaveIncrFormula()
{
    auto *textEdit = qobject_cast<QPlainTextEdit *>(sender());

    QTableWidget *table = nullptr;
    QLabel *labelCalculatedValue = nullptr;

    if (textEdit == ui->plainTextEditFormula)
    {
        table = ui->tableWidgetIncrement;
        labelCalculatedValue = ui->labelCalculatedValue;
    }
    else if (textEdit == ui->plainTextEditFormulaPC)
    {
        table = ui->tableWidgetPC;
        labelCalculatedValue = ui->labelCalculatedValuePC;
    }
    else
    {
        return;
    }

    const int row = table->currentRow();

    if (row == -1)
    {
        return;
    }

    // A group line is not an increment: the container would throw on its name. The panel below is switched off for
    // such a row, so nothing can be typed into it — this guard is the last word, not a convenience.
    const QString name = RowIncrementName(table, row);
    if (name.isEmpty())
    {
        return;
    }

    QString const text = textEdit->toPlainText();
    QSharedPointer<VIncrement> const incr = m_data->GetVariable<VIncrement>(name);

    if (const QTableWidgetItem *formulaField = table->item(row, 2); formulaField->text() == text)
    {
        QTableWidgetItem  const*result = table->item(row, 1);
        if (incr->IsSpecialUnits())
        {
            labelCalculatedValue->setText(result->text() + QChar(QChar::Space) + degreeSymbol);
        }
        else
        {
            // Show unit in dialog lable (cm, mm or inch)
            const QString postfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits());
            labelCalculatedValue->setText(result->text() + QChar(QChar::Space) + postfix);
        }
        return;
    }

    if (text.isEmpty())
    {
        if (incr->IsSpecialUnits())
        {
            labelCalculatedValue->setText(tr("Error") + " (" + degreeSymbol + "). " + tr("Empty field."));
        }
        else
        {
            // Show unit in dialog lable (cm, mm or inch)
            const QString postfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits());
            labelCalculatedValue->setText(tr("Error") + " (" + postfix + "). " + tr("Empty field."));
        }
        return;
    }

    if (incr->GetIncrementType() == IncrementType::Separator)
    {
        return;
    }

    if (not EvalIncrementFormula(text, true, incr->GetData(), labelCalculatedValue, incr->IsSpecialUnits()))
    {
        return;
    }

    try
    {
        const QString formula = VAbstractApplication::VApp()->TrVars()->FormulaFromUser(
            text, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
        m_patternDoc->SetIncrementFormula(name, formula);
    }
    catch (qmu::QmuParserError &e) // Just in case something bad will happen
    {
        Q_UNUSED(e)
        return;
    }

    const QTextCursor cursor = textEdit->textCursor();

    m_hasChanges = true;
    LocalUpdateTree();

    {
        const QSignalBlocker blocker(table);
        table->selectRow(row);
    }
    textEdit->setTextCursor(cursor);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::DeployFormula()
{
    SCASSERT(ui->plainTextEditFormula != nullptr)
    SCASSERT(ui->pushButtonGrow != nullptr)
    SCASSERT(ui->plainTextEditFormulaPC != nullptr)
    SCASSERT(ui->pushButtonGrowPC != nullptr)

    auto *button = qobject_cast<QPushButton *>(sender());

    QPlainTextEdit *plainTextEditFormula = nullptr;
    QPushButton *pushButtonGrow = nullptr;
    int baseHeight = 0;

    if (button == ui->pushButtonGrow)
    {
        plainTextEditFormula = ui->plainTextEditFormula;
        pushButtonGrow = ui->pushButtonGrow;
        baseHeight = m_formulaBaseHeight;
    }
    else if (button == ui->pushButtonGrowPC)
    {
        plainTextEditFormula = ui->plainTextEditFormulaPC;
        pushButtonGrow = ui->pushButtonGrowPC;
        baseHeight = m_formulaBaseHeightPC;
    }
    else
    {
        return;
    }

    const QTextCursor cursor = plainTextEditFormula->textCursor();

    if (plainTextEditFormula->height() < DIALOG_MAX_FORMULA_HEIGHT)
    {
        plainTextEditFormula->setFixedHeight(DIALOG_MAX_FORMULA_HEIGHT);
        // Set icon from theme (internal for Windows system)
        pushButtonGrow->setIcon(FromTheme(VThemeIcon::GoNext));
    }
    else
    {
        plainTextEditFormula->setFixedHeight(baseHeight);
        // Set icon from theme (internal for Windows system)
        pushButtonGrow->setIcon(FromTheme(VThemeIcon::GoDown));
    }

    // I found that after change size of formula field, it was filed for angle formula, field for formula became black.
    // This code prevent this.
    setUpdatesEnabled(false);
    repaint();
    setUpdatesEnabled(true);

    plainTextEditFormula->setFocus();
    plainTextEditFormula->setTextCursor(cursor);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::Fx()
{
    auto *button = qobject_cast<QToolButton *>(sender());

    QTableWidget *table = nullptr;
    QPlainTextEdit  const*plainTextEditFormula = nullptr;

    bool incrementMode = true;
    if (button == ui->toolButtonExpr)
    {
        incrementMode = true;
        table = ui->tableWidgetIncrement;
        plainTextEditFormula = ui->plainTextEditFormula;
    }
    else if (button == ui->toolButtonExprPC)
    {
        incrementMode = false;
        table = ui->tableWidgetPC;
        plainTextEditFormula = ui->plainTextEditFormulaPC;
    }
    else
    {
        return;
    }

    const int row = table->currentRow();

    if (row == -1)
    {
        return;
    }

    // A group line is not an increment: the container would throw on its name.
    const QString name = RowIncrementName(table, row);
    if (name.isEmpty())
    {
        return;
    }

    QSharedPointer<VIncrement> const incr = m_data->GetVariable<VIncrement>(name);

    QScopedPointer<DialogEditWrongFormula> const dialog(new DialogEditWrongFormula(incr->GetData(), NULL_ID, this));
    dialog->setWindowTitle(tr("Edit increment"));
    incrementMode ? dialog->SetIncrementsMode() : dialog->SetPreviewCalculationsMode();

    dialog->SetFormula(VTranslateVars::TryFormulaFromUser(plainTextEditFormula->toPlainText(),
                                                          VAbstractApplication::VApp()->Settings()->GetOsSeparator()));
    const QString postfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true);
    dialog->setPostfix(postfix); // Show unit in dialog lable (cm, mm or inch)

    if (dialog->exec() == QDialog::Accepted)
    {
        // Fix the bug #492. https://bitbucket.org/dismine/valentina/issues/492/valentina-crashes-when-add-an-increment
        // Because of the bug need to take QTableWidgetItem twice time. Previous update "killed" the pointer.
        // The name is the one read at the top of this function, so there is nothing to read a second time.
        m_patternDoc->SetIncrementFormula(name, dialog->GetFormula());

        m_hasChanges = true;
        LocalUpdateTree();

        table->selectRow(row);
        ShowTableIncrementDetails(table);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::closeEvent(QCloseEvent *event)
{
    RefreshPattern();

    const QSignalBlocker blockerFormula(ui->plainTextEditFormula);
    const QSignalBlocker blockerName(ui->lineEditName);
    const QSignalBlocker blockerDescription(ui->plainTextEditDescription);

    disconnect(this->m_patternDoc, &VPattern::FullUpdateFromFile, this, &DialogIncrements::FullUpdateFromFile);

    VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();

    settings->SetIncrementsSearchOptionMatchCase(m_search->IsMatchCase());
    settings->SetIncrementsSearchOptionWholeWord(m_search->IsMatchWord());
    settings->SetIncrementsSearchOptionRegexp(m_search->IsMatchRegexp());
    settings->SetIncrementsSearchOptionUseUnicodeProperties(m_search->IsUseUnicodePreperties());

    settings->SetPreviewCalculationsSearchOptionMatchCase(m_searchPC->IsMatchCase());
    settings->SetPreviewCalculationsSearchOptionWholeWord(m_searchPC->IsMatchWord());
    settings->SetPreviewCalculationsSearchOptionRegexp(m_searchPC->IsMatchRegexp());
    settings->SetPreviewCalculationsSearchOptionUseUnicodeProperties(m_searchPC->IsUseUnicodePreperties());

    emit UpdateProperties();
    emit DialogClosed(QDialog::Accepted);
    event->accept();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        // retranslate designer form (single inheritance approach)
        ui->retranslateUi(this);

        ui->toolButtonAdd->setMenu(InitVarTypeMenu(ui->toolButtonAdd->menu(), true /*increments tab*/));
        ui->toolButtonAdd->setProperty("hasMenu", true);
        ui->toolButtonAdd->style()->unpolish(ui->toolButtonAdd);
        ui->toolButtonAdd->style()->polish(ui->toolButtonAdd);

        ui->toolButtonAddPC->setMenu(InitVarTypeMenu(ui->toolButtonAddPC->menu(), false /*preview calculations tab*/));
        ui->toolButtonAddPC->setProperty("hasMenu", true);
        ui->toolButtonAddPC->style()->unpolish(ui->toolButtonAddPC);
        ui->toolButtonAddPC->style()->polish(ui->toolButtonAddPC);

        ui->lineEditFind->setPlaceholderText(m_search->SearchPlaceholder());
        ui->lineEditFindPC->setPlaceholderText(m_searchPC->SearchPlaceholder());
        UpdateSearchControlsTooltips();

        FullUpdateFromFile();
    }

    if (event->type() == QEvent::PaletteChange)
    {
        InitIcons();
    }

    // remember to call base class implementation
    QWidget::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogIncrements::eventFilter(QObject *object, QEvent *event) -> bool
{
    if (auto *textEdit = qobject_cast<QLineEdit *>(object))
    {
        if (event->type() == QEvent::KeyPress)
        {
            if (const auto *keyEvent = static_cast<QKeyEvent *>(event);
                (keyEvent->key() == Qt::Key_Period) && (keyEvent->modifiers() & Qt::KeypadModifier))
            {
                if (VAbstractApplication::VApp()->Settings()->GetOsSeparator())
                {
                    textEdit->insert(LocaleDecimalPoint(QLocale()));
                }
                else
                {
                    textEdit->insert(LocaleDecimalPoint(QLocale::c()));
                }
                return true;
            }
        }
    }

    // pass the event on to the parent class
    return DialogTool::eventFilter(object, event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::showEvent(QShowEvent *event)
{
    // Skip DialogTool implementation
    QDialog::showEvent(event); // NOLINT(bugprone-parent-virtual-call)
    if (event->spontaneous())
    {
        return;
    }

    if (isInitialized)
    {
        return;
    }
    // do your init stuff here

    if (const QSize sz = VAbstractApplication::VApp()->Settings()->GetIncrementsDialogSize(); not sz.isEmpty())
    {
        resize(sz);
    }

    isInitialized = true; // first show windows are held
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::resizeEvent(QResizeEvent *event)
{
    // remember the size for the next time this dialog is opened, but only
    // if widget was already initialized, which rules out the resize at
    // dialog creating, which would
    if (isInitialized)
    {
        VAbstractApplication::VApp()->Settings()->SetIncrementsDialogSize(size());
    }
    DialogTool::resizeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::ShowIncrementDetails()
{
    ShowTableIncrementDetails(qobject_cast<QTableWidget *>(sender()));
}

//---------------------------------------------------------------------------------------------------------------------
DialogIncrements::~DialogIncrements()
{
    ui->lineEditFind->blockSignals(true);   // prevents crash
    ui->lineEditFindPC->blockSignals(true); // prevents crash
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::RestoreAfterClose()
{
    connect(this->m_patternDoc, &VPattern::FullUpdateFromFile, this, &DialogIncrements::FullUpdateFromFile);

    ui->tabWidget->setCurrentIndex(0);

    if (ui->tableWidgetIncrement->rowCount() > 0)
    {
        ui->tableWidgetIncrement->selectRow(0);
    }

    if (ui->tableWidgetPC->rowCount() > 0)
    {
        ui->tableWidgetPC->selectRow(0);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogIncrements::CheckDependencyTreeComplete()
{
    // does nothing
}
