/************************************************************************
 **
 **  @file   dialogeditwrongformula.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   29 5, 2014
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 14/10/2026.
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

#include <QFile>
#include <QTextStream>
#include "dialogeditwrongformula.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QCheckBox>
#include <QCursor>
#include <QDialog>
#include <QFont>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMapIterator>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QSharedPointer>
#include <QShowEvent>
#include <QSize>
#include <QStyle>
#include <QStyleOptionButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextCursor>
#include <QTimer>
#include <QToolButton>
#include <QWidget>
#include <new>

#include "../tools/dialogtool.h"
#include "../tools/drawTools/toolcurve/vtoolabstractcurve.h"
#include "../vformat/knownmeasurements/vknownmeasurement.h"
#include "../vformat/knownmeasurements/vknownmeasurements.h"
#include "../vformat/knownmeasurements/vknownmeasurementsdatabase.h"
#include "../vmisc/def.h"
#include "../qmuparser/qmudef.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/variables/varcradius.h"
#include "../vpatterndb/variables/vcurveangle.h"
#include "../vpatterndb/variables/vcurveclength.h"
#include "../vpatterndb/variables/vcurvelength.h"
#include "../vpatterndb/variables/vincrement.h"
#include "../vpatterndb/variables/vlineangle.h"
#include "../vpatterndb/variables/vlinelength.h"
#include "../vpatterndb/variables/vmeasurement.h"
#include "../vpatterndb/variables/vpiecearea.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vtranslatevars.h"
#include "../undocommands/vchangelinerolename.h"
#include "ui_dialogeditwrongformula.h"

namespace
{
/// @brief La formule telle qu'elle doit s'**afficher** : jamais `Spl_` ni `SplPath_`.
///
/// La conversion vers la forme utilisateur rend l'alias de l'objet, donc parfois son ancienne tête — et valider
/// la remettait dans le patron (relevé le 13-10-2026 : le point portait `Courbe_RepereDos+1`, l'éditeur
/// affichait `SplPath_RepereDos+1`). La tête `Curve_` s'écrit donc ici, dans le **texte montré**, et seulement
/// quand elle existe vraiment dans le conteneur : un nom qui ne résout pas ne vaut pas mieux qu'un ancien.
auto CurveHeadsUser(const VTranslateVars *trVars, const VContainer *data, const QString &formula) -> QString
{
    static const QStringList prefixes{QString(),           QStringLiteral("C1Length"), QStringLiteral("C2Length"),
                                      QStringLiteral("Angle1"), QStringLiteral("Angle2"), QStringLiteral("Handle1"),
                                      QStringLiteral("Handle2")};
    if (trVars == nullptr || data == nullptr || data->DataVariables() == nullptr)
    {
        return formula;
    }

    QString out = formula;
    for (const QString &prefix : prefixes)
    {
        for (const QString &head : {QStringLiteral("SplPath_"), QStringLiteral("Spl_")})
        {
            const QString old = prefix + head;
            qsizetype from = 0;
            while ((from = out.indexOf(old, from)) >= 0)
            {
                qsizetype end = from + old.size();
                while (end < out.size() && (out.at(end).isLetterOrNumber() || out.at(end) == QLatin1Char('_')
                                            || out.at(end) == QLatin1Char('\'')))
                {
                    ++end;
                }

                const QString canonical = prefix + QStringLiteral("Curve_") + out.mid(from + old.size(),
                                                                                      end - from - old.size());
                if (data->DataVariables()->contains(canonical))
                {
                    out.replace(from, end - from, trVars->VarToUser(canonical));
                }
                from += 1;
            }
        }
    }

    return out;
}
} // namespace



template <class T> class QSharedPointer;

enum
{
    ColumnName = 0,
    ColumnFullName,
    ColumnNameAction
};

//---------------------------------------------------------------------------------------------------------------------
DialogEditWrongFormula::DialogEditWrongFormula(const VContainer *data, quint32 toolId, QWidget *parent)
  : QDialog(parent),
    ui(new Ui::DialogEditWrongFormula),
    m_data(data),
    m_toolId(toolId),
    timerFormula(new QTimer(this))
{
    SCASSERT(data != nullptr)

    ui->setupUi(this);

    ui->radioButtonPieceArea->setVisible(false);

    timerFormula->setSingleShot(true);
    connect(timerFormula, &QTimer::timeout, this, &DialogEditWrongFormula::EvalFormula);

#if defined(Q_OS_MAC)
    setWindowFlags(Qt::Window);
#endif

    InitVariables();
    this->formulaBaseHeight = ui->plainTextEditFormula->height();
    ui->plainTextEditFormula->installEventFilter(this);

    connect(ui->filterFormulaInputs, &QLineEdit::textChanged, this, &DialogEditWrongFormula::FilterVariablesEdited);

    QPushButton  const*bOk = ui->buttonBox->button(QDialogButtonBox::Ok);
    SCASSERT(bOk != nullptr)
    connect(bOk, &QPushButton::clicked, this, &DialogEditWrongFormula::DialogAccepted);

    QPushButton  const*bCancel = ui->buttonBox->button(QDialogButtonBox::Cancel);
    SCASSERT(bCancel != nullptr)
    connect(bCancel, &QPushButton::clicked, this, &DialogEditWrongFormula::DialogRejected);

    VAbstractApplication::VApp()->Settings()->GetOsSeparator() ? setLocale(QLocale()) : setLocale(QLocale::c());

    connect(ui->tableWidget, &QTableWidget::itemDoubleClicked, this, &DialogEditWrongFormula::PutVal);

    connect(ui->plainTextEditFormula, &QPlainTextEdit::textChanged, this,
            [this]() { timerFormula->start(formulaTimerTimeout); });

    // Disable Qt::WaitCursor
#ifndef QT_NO_CURSOR
    if (QGuiApplication::overrideCursor() != nullptr && QGuiApplication::overrideCursor()->shape() == Qt::WaitCursor)
    {
        restoreCursor = true;
        QGuiApplication::restoreOverrideCursor();
    }
#endif
    // Two columns to start with, and the headers are written by SetTableColumns with the count, in one place.
    // The third column exists only in the category that has something to name — see SetTableColumns.
    ui->tableWidget->setColumnCount(ColumnFullName + 1);
    ui->tableWidget->horizontalHeader()->setMinimumSectionSize(90);
    ui->tableWidget->setEditTriggers(QTableWidget::NoEditTriggers);
    ui->tableWidget->verticalHeader()->hide();
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    // The row above the field reads "Calculated value:" then the value, both against the left edge. A layout
    // item's alignment places the box, never the text inside it: that one is the label's own, and every QLabel
    // here leaves it to the style. Said here rather than in the .ui because `retranslateUi` cannot take it
    // away — the caption and its value then cannot drift apart, and no gap opens between them.
    ui->labelEditFormula->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->labelResultCalculation->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
}

//---------------------------------------------------------------------------------------------------------------------
DialogEditWrongFormula::~DialogEditWrongFormula()
{
#ifndef QT_NO_CURSOR
    if (restoreCursor)
    {
        QGuiApplication::setOverrideCursor(Qt::WaitCursor);
    }
#endif
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::DialogAccepted()
{
    formula = ui->plainTextEditFormula->toPlainText();
    emit DialogClosed(QDialog::Accepted);
    emit accepted();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::DialogRejected()
{
    emit DialogClosed(QDialog::Rejected);
    emit rejected();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::EvalFormula()
{
    const VTranslateVars *const trVars = VAbstractApplication::VApp()->TrVars();

    const QString typed = ui->plainTextEditFormula->toPlainText();
    const QString internal = InternalFormula(typed, m_data);

    // A name given since this formula was written — the object named from the canvas, from its own field, or from
    // this dialogue a moment ago — is nothing to ask about: the formula is repaired with the name the object has
    // now, and the field is told, so what is read is what the formula will write. Done once, and only when a token
    // really changed: from then on the two agree and typing is left alone.
    const QString repaired = VContainer::FormulaWithoutStaleHandles(m_data, internal);
    QString displayed = typed;
    if (repaired != internal)
    {
        displayed = CurveHeadsUser(trVars, m_data,
                                   trVars->FormulaToUser(repaired,
                                                         VAbstractApplication::VApp()->Settings()->GetOsSeparator()));
        QSignalBlocker blocker(ui->plainTextEditFormula);
        ui->plainTextEditFormula->setPlainText(displayed);
        MoveCursorToEnd(ui->plainTextEditFormula);
    }

    // Which object the list has to point at, in the very words the list writes it with — the two have to agree
    // for the row to be found. Asked here, before the evaluation, because the refusal only says that something
    // has no name: the identity is what makes the row answer.
    m_unnamedObject.clear();
    const QString unnamed = VContainer::UnnamedObjectInFormula(m_data, repaired);
    if (not unnamed.isEmpty())
    {
        const QSharedPointer<VInternalVariable> var = m_data->DataVariables()->value(unnamed);
        if (not var.isNull())
        {
            // In the very words the list uses, so the row can be found: the name is the alias when there is one,
            // the object's own name otherwise — exactly what ShowVariable writes on the row.
            const QString display = SimplifiedName(var->GetAlias().isEmpty() ? var->GetName() : var->GetAlias());

            m_unnamedObject = trVars->VarToUser(display);
        }
    }

    // No label is handed over: the caption beside the value is a name, not a state, and must stay neutral
    // whatever the formula does. The colour is then carried by the value alone, which is also what says why.
    Eval({.formula = displayed,
          .variables = m_data->DataVariables(),
          .container = m_data,
          .labelResult = ui->labelResultCalculation,
          .postfix = postfix,
          .checkZero = checkZero,
          .checkLessThanZero = checkLessThanZero},
         flagFormula);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ValChanged show description when current variable changed
 * @param row number of row
 */
void DialogEditWrongFormula::ValChanged(int row)
{
    if (ui->tableWidget->rowCount() == 0)
    {
        ui->labelDescription->setText(QString());
        return;
    }
    QTableWidgetItem *item = ui->tableWidget->item(row, ColumnName);
    // The variable's own name, carried by the row, rather than the text on screen: that text is a display name,
    // and looking it up as a key is what raised `ExceptionBadId` and left the description empty.
    QString name = item->data(Qt::UserRole).toString();
    if (name.isEmpty())
    {
        name = item->text();
    }
    else
    {
        name = VAbstractApplication::VApp()->TrVars()->VarFromUser(name);
    }

    // **Ce n'est pas la page qui dit ce qu'un objet est, c'est l'objet.** La catégorie « Arc » réunit des
    // longueurs — enregistrées comme des longueurs de courbe, type 5 — et des rayons : demander le rayon d'une
    // longueur levait `ExceptionBadId` (« Name = 'Arc_Y_21', type = 5. ») au lieu d'afficher la description. Le
    // type de la variable tranche donc, et une ligne dont le type n'est pas celui de la page ne lève plus rien.
    VarType type = VarType::Unknown;
    if (const QSharedPointer<VInternalVariable> anyVar = m_data->DataVariables()->value(name); not anyVar.isNull())
    {
        type = anyVar->GetType();
    }

    try
    {
        if (ui->radioButtonStandardTable->isChecked())
        {
            SetMeasurementDescription(item, name);
        }
        else if (ui->radioButtonIncrements->isChecked() || ui->radioButtonPC->isChecked())
        {
            const QSharedPointer<VIncrement> incr = m_data->GetVariable<VIncrement>(name);
            SetDescription(item->text(), *incr->GetValue(), incr->IsSpecialUnits(), incr->GetDescription());
        }
        else if (ui->radioButtonLengthLine->isChecked() && type == VarType::LineLength)
        {
            const bool specialUnits = false;
            SetDescription(item->text(), *m_data->GetVariable<VLengthLine>(name)->GetValue(), specialUnits,
                           tr("Line length"));
        }
        else if (ui->radioButtonLengthSpline->isChecked() && type == VarType::CurveLength)
        {
            const bool specialUnits = false;
            SetDescription(item->text(), *m_data->GetVariable<VCurveLength>(name)->GetValue(), specialUnits,
                           tr("Curve length"));
        }
        else if (ui->radioButtonAngleLine->isChecked() && type == VarType::LineAngle)
        {
            const bool specialUnits = true;
            SetDescription(item->text(), *m_data->GetVariable<VLineAngle>(name)->GetValue(), specialUnits,
                           tr("Line Angle"));
        }
        else if (ui->radioButtonRadiusesArcs->isChecked() && type == VarType::ArcRadius)
        {
            const bool specialUnits = false;
            SetDescription(item->text(), *m_data->GetVariable<VArcRadius>(name)->GetValue(), specialUnits,
                           tr("Arc radius"));
        }
        else if (ui->radioButtonAnglesCurves->isChecked() && type == VarType::CurveAngle)
        {
            const bool specialUnits = true;
            SetDescription(item->text(), *m_data->GetVariable<VCurveAngle>(name)->GetValue(), specialUnits,
                           tr("Curve angle"));
        }
        else if (ui->radioButtonCLength->isChecked() && type == VarType::CurveCLength)
        {
            const bool specialUnits = false;
            SetDescription(item->text(), *m_data->GetVariable<VCurveCLength>(name)->GetValue(), specialUnits,
                           tr("Length to control point"));
        }
        else if (ui->radioButtonPieceArea->isChecked())
        {
            SetPieceAreaDescription(item, name);
        }
        else if (ui->radioButtonFunctions->isChecked() || ui->radioButtonConstants->isChecked())
        {
            ui->labelDescription->setText(item->toolTip());
        }
    }
    catch (const VExceptionBadId &e)
    {
        qCritical() << qUtf8Printable(e.ErrorMessage());
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PutVal put variable into edit
 * @param item chosen item of table widget
 */
void DialogEditWrongFormula::PutVal(QTableWidgetItem *item)
{
    SCASSERT(item != nullptr)
    QTableWidgetItem  const*valItem = ui->tableWidget->item(item->row(), ColumnName);
    QTextCursor cursor = ui->plainTextEditFormula->textCursor();
    if (ui->radioButtonFunctions->isChecked())
    {
        QString const function = valItem->data(Qt::UserRole).toString();
        const VTranslateVars *trVars = VAbstractApplication::VApp()->TrVars();
        const QMap<QString, QString> functionsArguments = trVars->GetFunctionsArguments();
        cursor.insertText(valItem->text() + functionsArguments.value(function));
    }
    else
    {
        cursor.insertText(valItem->text());
    }

    ui->plainTextEditFormula->setTextCursor(cursor);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Measurements show in list measurements
 */
void DialogEditWrongFormula::Measurements()
{
    SetCategoryCheckBoxes(true, false);

    QList<QSharedPointer<VMeasurement>> vars = m_data->DataMeasurements().values();
    std::stable_sort(vars.begin(), vars.end(),
                     [](const QSharedPointer<VMeasurement> &obj1, const QSharedPointer<VMeasurement> &obj2)
                     { return obj1->Index() < obj2->Index(); });

    ShowMeasurements(vars);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief LengthLines show in list lengths of lines variables
 */
void DialogEditWrongFormula::LengthLines()
{
    SetCategoryCheckBoxes(false, true);
    // The one list that offers to name what it shows, because a pair of points is all a line has until it is
    // named. Every other list is read-only.
    ShowVariable(m_data->DataLengthLines().values(), true, true);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::RadiusArcs()
{
    SetCategoryCheckBoxes(false, true);
    Arc();
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The curve lengths that belong to an arc, which the category "Arc" takes and "Length of curves" leaves.
 *
 *  An arc's length is registered as a curve length — same class, same variable — so nothing but the name tells
 *  the two apart: an arc is `Arc_…` or `ElArc_…`, a spline `Spl_…` or `SplPath…`. The name is read rather than
 *  the tool, because the variable is all this list has.
 */
auto DialogEditWrongFormula::IsArcName(const QString &name) -> bool
{
    static const QStringList prefixes{QString(),           QStringLiteral("C1Length"), QStringLiteral("C2Length"),
                                      QStringLiteral("Angle1"), QStringLiteral("Angle2"), QStringLiteral("Handle1"),
                                      QStringLiteral("Handle2")};
    for (const QString &prefix : prefixes)
    {
        if (not name.startsWith(prefix))
        {
            continue;
        }

        const QString rest = name.mid(prefix.size());
        if (rest.startsWith(arc_) || rest.startsWith(elarc_))
        {
            return true;
        }
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEditWrongFormula::IsArcCurve(const QSharedPointer<VCurveLength> &var) -> bool
{
    return IsArcName(var->GetName());
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief Arc: everything the pattern reads on an arc — its length, and its radius.
 *
 *  A pattern holds many curves and few arcs, so the two live together here instead of leaving the radius alone
 *  in a category of its own. The lengths come from the curve lengths, the radii from the radii: one list, so the
 *  user reads an arc in one place.
 */
void DialogEditWrongFormula::Arc()
{
    SetCategoryCheckBoxes(false, true);

    // Both maps hold the same kind of variable under different typed views, and an arc has one of each. Appending
    // them keeps a single list — and a single ordering — for the whole category.
    QList<QSharedPointer<VCurveVariable>> vars;
    for (const auto &var : m_data->DataLengthCurves())
    {
        if (IsArcCurve(var))
        {
            vars.append(var);
        }
    }
    for (const auto &var : m_data->DataRadiusesArcs())
    {
        vars.append(var);
    }

    // Ses longueurs de contrôle aussi : « tout ce qui est relatif à un arc va dans Arc ».
    for (const auto &var : m_data->DataCurvesCLength())
    {
        if (IsArcName(var->GetName()))
        {
            vars.append(var);
        }
    }

    // Ses angles aussi : un arc est un arc, et la catégorie « Arc » le lit **en entier** — longueur, rayon,
    // angles. Les angles d'arc sont enregistrés comme des angles de courbe (même classe, même variable), donc
    // seul le nom les distingue, exactement comme pour les longueurs (`IsArcCurve`).
    for (const auto &var : m_data->DataAnglesCurves())
    {
        if (IsArcName(var->GetName()))
        {
            vars.append(var);
        }
    }

    ShowVariable(vars, true, true);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AnglesCurves show in list angles of curves variables
 *
 * Curves only: an arc is read in the "Arc" category, which holds its length, its radius and its angles together.
 * Listing an arc's angle here too would offer the same object twice, in two categories that would then disagree
 * on what it is — the same rule `LengthCurves` applies to the lengths.
 */
void DialogEditWrongFormula::AnglesCurves()
{
    SetCategoryCheckBoxes(false, true);

    auto angles = m_data->DataAnglesCurves().values();
    for (auto it = angles.begin(); it != angles.end();)
    {
        if (IsArcName((*it)->GetName()))
        {
            it = angles.erase(it);
        }
        else
        {
            ++it;
        }
    }

    ShowVariable(angles, true, true);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief LengthCurves show in list lengths of curves variables
 *
 * Curves only: an arc is read in the "Arc" category, which holds its length and its radius together. Listing it
 * here too would offer the same object twice, in two categories that would then disagree on what it is.
 */
void DialogEditWrongFormula::LengthCurves()
{
    SetCategoryCheckBoxes(false, true);

    QList<QSharedPointer<VCurveLength>> curves;
    for (const auto &var : m_data->DataLengthCurves())
    {
        if (not IsArcCurve(var))
        {
            curves.append(var);
        }
    }

    ShowVariable(curves, true, true);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief CurvesCLength shows the lengths to the control points
 *
 * Curves only: an arc's own control-point lengths are read in the "Arc" category, like its length, its radius and
 * its angles — one rule for the whole distribution, so an arc is never offered half here and half there.
 */
void DialogEditWrongFormula::CurvesCLength()
{
    SetCategoryCheckBoxes(false, true);

    auto lengths = m_data->DataCurvesCLength().values();
    for (auto it = lengths.begin(); it != lengths.end();)
    {
        if (IsArcName((*it)->GetName()))
        {
            it = lengths.erase(it);
        }
        else
        {
            ++it;
        }
    }

    ShowVariable(lengths, true, true);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::AngleLines()
{
    SetCategoryCheckBoxes(false, true);
    ShowVariable(m_data->DataAngleLines().values(), false, true);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Increments show in list increment variables
 */
void DialogEditWrongFormula::Increments()
{
    ShowIncrementsInPreviewCalculation(false);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::PreviewCalculations()
{
    ShowIncrementsInPreviewCalculation(true);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::PieceArea()
{
    SetCategoryCheckBoxes(false, false);
    ShowVariable(m_data->DataPieceArea().values());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Functions show in list functions
 */
void DialogEditWrongFormula::Functions()
{
    SetCategoryCheckBoxes(false, false);
    ShowFunctions();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Constants show in list formula constants
 */
void DialogEditWrongFormula::Constants()
{
    SetCategoryCheckBoxes(false, false);
    ShowConstants();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::closeEvent(QCloseEvent *event)
{
    ui->plainTextEditFormula->blockSignals(true);
    QDialog::closeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    if (event->spontaneous())
    {
        return;
    }

    // The formula this dialogue was given could not be edited: nobody named the objects it uses, and the question
    // was put to the user in SetFormula, who cancelled it. Cancelling means nothing happened at all, so the
    // dialogue has nothing to show and closes itself as soon as the event loop lets it.
    if (m_nameRequestCancelled)
    {
        QTimer::singleShot(0, this, &QDialog::reject);
        return;
    }

    if (m_isInitialized)
    {
        return;
    }
    // do your init stuff here

    const QSize sz = VAbstractApplication::VApp()->Settings()->GetFormulaWizardDialogSize();
    if (not sz.isEmpty())
    {
        resize(sz);
    }

    CheckState();

    m_isInitialized = true; // first show windows are held

    // The "Name it" column is the only one with a width of its own; the two others share the rest, which is what
    // `Stretch` is for. Measured here, once the table has its real size, because a cell widget reports nothing
    // to `ResizeToContents` — measured before the dialogue is on screen, the button's column came out too narrow.
    ApplyColumnWidths();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::resizeEvent(QResizeEvent *event)
{
    // The button's column keeps the width it was measured for; the two others are stretched, and Qt does that
    // again on its own when the table grows.
    ApplyColumnWidths();

    // remember the size for the next time this dialog is opened, but only
    // if widget was already initialized, which rules out the resize at
    // dialog creating, which would
    if (m_isInitialized)
    {
        VAbstractApplication::VApp()->Settings()->SetFormulaWizardDialogSize(size());
    }
    QDialog::resizeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }

    // remember to call base class implementation
    QDialog::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::CheckState()
{
    QPushButton *bOk = ui->buttonBox->button(QDialogButtonBox::Ok);
    SCASSERT(bOk != nullptr)
    bOk->setEnabled(IsValid());
    // In case dialog hasn't apply button
    QPushButton *bApply = ui->buttonBox->button(QDialogButtonBox::Apply);
    if (bApply != nullptr)
    {
        bApply->setEnabled(bOk->isEnabled());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::SetFormula(const QString &value)
{
    const VTranslateVars *const trVars = VAbstractApplication::VApp()->TrVars();

    // Callers do not agree on the notation they hand over — a tool's field writes in the user's own, the repair of
    // a written formula hands over the internal one — so the formula is brought to the internal notation here, the
    // one the container answers to. Everything below judges names, and a name is internal.
    QString internal = InternalFormula(value, m_data);

    // An object that already has a name is not a question: a formula still written with the handle it had before
    // is repaired with the name it has now, here and without a word.
    internal = VContainer::FormulaWithoutStaleHandles(m_data, internal);

    // An object nobody ever named is another matter: a formula can only be written with names, so the names are
    // asked for before the assistant shows itself — one object at a time, the token as the formula writes it and
    // the part of it the user replaces. Cancelling means nothing at all happened: no name, no repair, and no
    // assistant either, which is why the refusal is remembered for showEvent.
    if (not NameUnnamedObjects(parentWidget() != nullptr ? parentWidget() : this, m_data, internal))
    {
        m_nameRequestCancelled = true;
        return;
    }

    formula = CurveHeadsUser(trVars, m_data,
                             trVars->FormulaToUser(internal,
                                                   VAbstractApplication::VApp()->Settings()->GetOsSeparator()));
    ui->plainTextEditFormula->setPlainText(formula);
    MoveCursorToEnd(ui->plainTextEditFormula);
    ui->plainTextEditFormula->selectAll();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::setCheckZero(bool value)
{
    checkZero = value;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::setCheckLessThanZero(bool value)
{
    checkLessThanZero = value;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::setPostfix(const QString &value)
{
    postfix = value;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Narrow the list of input sources to what the caller's context can offer.
 *
 * The three setters chain into one another: the measurements mode drops the increments, the increments mode drops the
 * preview calculations, and both drop the six sources the table of variables does not read — the lengths of lines and
 * of curves, the angles, the arc radii, the angles of curves and the lengths to the control points. Those are removed
 * rather than greyed out, the idiom ShowPieceArea already follows: a greyed entry one cannot pick is only noise, and
 * a short list reads faster than a long one with dead rows in it.
 */
void DialogEditWrongFormula::SetMeasurementsMode()
{
    ui->radioButtonIncrements->setVisible(false);
    SetIncrementsMode();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::SetIncrementsMode()
{
    ui->radioButtonPC->setVisible(false);
    SetPreviewCalculationsMode();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::SetPreviewCalculationsMode()
{
    ui->radioButtonLengthLine->setVisible(false);
    ui->radioButtonLengthSpline->setVisible(false);
    ui->radioButtonAngleLine->setVisible(false);
    ui->radioButtonRadiusesArcs->setVisible(false);
    ui->radioButtonAnglesCurves->setVisible(false);
    ui->radioButtonCLength->setVisible(false);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::ShowPieceArea(bool show) const
{
    ui->radioButtonPieceArea->setVisible(show);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEditWrongFormula::GetFormula() const -> QString
{
    return VTranslateVars::TryFormulaFromUser(formula, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::InitVariables()
{
    connect(ui->tableWidget, &QTableWidget::currentCellChanged, this, &DialogEditWrongFormula::ValChanged);

    ui->radioButtonStandardTable->setChecked(true);
    Measurements();

    // clear text filter every time when new radio button selected
    auto ClearFilterFormulaInputs = [this]() { ui->filterFormulaInputs->clear(); };

    connect(ui->radioButtonStandardTable, &QRadioButton::clicked, this, &DialogEditWrongFormula::Measurements);
    connect(ui->radioButtonStandardTable, &QRadioButton::clicked, this, ClearFilterFormulaInputs);

    connect(ui->radioButtonIncrements, &QRadioButton::clicked, this, &DialogEditWrongFormula::Increments);
    connect(ui->radioButtonIncrements, &QRadioButton::clicked, this, ClearFilterFormulaInputs);

    connect(ui->radioButtonPC, &QRadioButton::clicked, this, &DialogEditWrongFormula::PreviewCalculations);
    connect(ui->radioButtonPC, &QRadioButton::clicked, this, ClearFilterFormulaInputs);

    connect(ui->radioButtonLengthLine, &QRadioButton::clicked, this, &DialogEditWrongFormula::LengthLines);
    connect(ui->radioButtonLengthLine, &QRadioButton::clicked, this, ClearFilterFormulaInputs);

    connect(ui->radioButtonLengthSpline, &QRadioButton::clicked, this, &DialogEditWrongFormula::LengthCurves);
    connect(ui->radioButtonLengthSpline, &QRadioButton::clicked, this, ClearFilterFormulaInputs);

    connect(ui->radioButtonAngleLine, &QRadioButton::clicked, this, &DialogEditWrongFormula::AngleLines);
    connect(ui->radioButtonAngleLine, &QRadioButton::clicked, this, ClearFilterFormulaInputs);

    connect(ui->radioButtonRadiusesArcs, &QRadioButton::clicked, this, &DialogEditWrongFormula::RadiusArcs);
    connect(ui->radioButtonRadiusesArcs, &QRadioButton::clicked, this, ClearFilterFormulaInputs);

    connect(ui->radioButtonAnglesCurves, &QRadioButton::clicked, this, &DialogEditWrongFormula::AnglesCurves);
    connect(ui->radioButtonAnglesCurves, &QRadioButton::clicked, this, ClearFilterFormulaInputs);

    connect(ui->radioButtonCLength, &QRadioButton::clicked, this, &DialogEditWrongFormula::CurvesCLength);
    connect(ui->radioButtonCLength, &QRadioButton::clicked, this, ClearFilterFormulaInputs);

    connect(ui->radioButtonPieceArea, &QRadioButton::clicked, this, &DialogEditWrongFormula::PieceArea);
    connect(ui->radioButtonPieceArea, &QRadioButton::clicked, this, ClearFilterFormulaInputs);

    connect(ui->radioButtonFunctions, &QRadioButton::clicked, this, &DialogEditWrongFormula::Functions);
    connect(ui->radioButtonFunctions, &QRadioButton::clicked, this, ClearFilterFormulaInputs);

    connect(ui->radioButtonConstants, &QRadioButton::clicked, this, &DialogEditWrongFormula::Constants);
    connect(ui->radioButtonConstants, &QRadioButton::clicked, this, ClearFilterFormulaInputs);

    connect(ui->checkBoxHideEmpty, CHECKBOX_STATE_CHANGED, this, &DialogEditWrongFormula::Measurements);

    // The unnamed switch does not filter what is already listed, it decides what is listed at all: the category
    // is built again, which is what a filter over a table built on the fly costs.
    connect(ui->checkBoxHideUnnamed, CHECKBOX_STATE_CHANGED, this, &DialogEditWrongFormula::ShowCurrentCategory);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::SetDescription(const QString &name, qreal value, bool specialUnits,
                                            const QString &description, bool square)
{
    QString unitName = postfix;
    if (not specialUnits && square)
    {
        unitName += QStringLiteral("²");
    }

    const QString unit = specialUnits ? degreeSymbol : ' ' + unitName;
    const auto desc = QStringLiteral("%1(%2%3) - %4").arg(name).arg(value).arg(unit, description);
    ui->labelDescription->setText(desc);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEditWrongFormula::Eval(const FormulaData &formulaData, bool &flag) -> qreal
{
    // The dialogue's own container, so the names the formula uses are judged against the objects it lists.
    FormulaData data = formulaData;
    data.container = m_data;

    const qreal result = EvalToolFormula(this, data, flag);
    CheckState(); // Disable Ok and Apply buttons if something wrong.
    return result;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ShowVariable show variables in list
 * @param vars container with variables
 */
template <class T> void DialogEditWrongFormula::ShowVariable(const QList<T> &vars, bool nameable, bool unnamed)
{
    QSignalBlocker blocker(ui->tableWidget);
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);
    ui->tableWidget->setColumnHidden(ColumnFullName, true);
    ui->labelDescription->setText(QString());

    // "Length of lines" is the category whose objects Ostara names in a table of pairs, and so the one that has
    // a line to offer to name. It says so itself when it calls this; every other category leaves the table two
    // columns wide.
    SetTableColumns(nameable);

    const VTranslateVars *trVars = VAbstractApplication::VApp()->TrVars();

    QSet<QString> processedNames;

    int refusedRow = -1; // the object the formula was refused for, when this list is the one that carries it

    for (const auto &var : vars)
    {
        if (ui->checkBoxHideEmpty->isVisible() && ui->checkBoxHideEmpty->isChecked() && var->IsNotUsed())
        {
            continue; // skip this measurement
        }

        if (!var->Filter(m_toolId))
        { // If we create this variable don't show
            // The name shown is the object's own, exactly as the formula writes it: `Line_TestRectangle` for a
            // named line, `Line_Y_Y1` for one nobody named, `#Cou` for an increment. It is never blank — every
            // object that reaches this list has a name the calculator resolves — so the list can always be read
            // and a double click always inserts something that works.
            const QString display = SimplifiedName(var->GetAlias().isEmpty() ? var->GetName() : var->GetAlias());

            // The switch hides what a formula may not name: the objects nobody has named. The question is asked
            // of the object, not of a name: a line is judged by the pair of points it joins, a curve by its own
            // alias. Asked through a name — the one the row shows — it would answer a different question, and
            // only for the objects whose name happens to resolve. One verdict per row, read twice below.
            const bool nameless = VContainer::IsUnnamedObject(m_data, var);
            if (unnamed && ui->checkBoxHideUnnamed->isChecked() && nameless)
            {
                continue;
            }

            // Two keys can hold the same object — a role name and the pair it replaced — and the name is what
            // tells them apart. Asking by the displayed name keeps one row per object.
            const QString key = display;
            if (processedNames.contains(key))
            {
                continue;
            }

            processedNames.insert(key);

            ui->tableWidget->setRowCount(ui->tableWidget->rowCount() + 1);
            const int row = ui->tableWidget->rowCount() - 1;

            auto *item = new QTableWidgetItem(trVars->VarToUser(display));
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);

            // What the button of this row has to act on. The list is rebuilt on every category change, so this
            // cannot be a member: the row is the answer, and it carries what it is about.
            item->setData(Qt::UserRole, var->GetName());
            item->setData(Qt::UserRole + 1, QString());
            ui->tableWidget->setItem(row, ColumnName, item);

            // The objects the switch above hides are the ones with a name to give: a line is named through the
            // pair of points it joins, a curve through the alias of the tool that draws it. The row remembers
            // which of the two it is, so the button knows where to write.
            if (nameless)
            {
                if (var->GetType() == VarType::LineLength)
                {
                    item->setData(Qt::UserRole + 1, QStringLiteral("line"));
                    AddNameButton(row, var->GetName().mid(line_.length()));
                }
                else if (var->GetType() == VarType::CurveLength)
                {
                    item->setData(Qt::UserRole + 1, QStringLiteral("curve"));
                    AddNameButton(row, var->GetName());
                }
            }

            // The object the formula was refused for is the one to look at: the row is pointed at rather than
            // named, so the refusal says which object without putting its points back in front of the user.
            if (not m_unnamedObject.isEmpty() && item->text() == m_unnamedObject)
            {
                refusedRow = row;
            }
        }
    }

    blocker.unblock();
    ui->tableWidget->selectRow(refusedRow >= 0 ? refusedRow : 0);
    if (refusedRow >= 0)
    {
        ui->tableWidget->scrollToItem(ui->tableWidget->item(refusedRow, ColumnName));
    }
    // Two columns of equal width and a third one at the width of its button — see ApplyColumnWidths.
    ApplyColumnWidths();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::SetCategoryCheckBoxes(bool measurements, bool unnamed)
{
    // A category shows the two switches that mean something for it, and none of the others: a checkbox that
    // does nothing is a question the user answers for nothing. "Hide empty measurements" is a question about
    // measurements; "Hide unnamed objects" is a question about the objects a formula can name.
    ui->checkBoxHideEmpty->setVisible(measurements);
    ui->checkBoxHideUnnamed->setVisible(unnamed);

    // The switch keeps the answer the user gave it. Setting it back to checked here — to give each category its
    // own default — swallowed the first click on it: unchecking the box rebuilds the list, and the rebuild put
    // the box back to checked before anything could see the change.
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::ApplyColumnWidths()
{
    // Two columns of equal width, like every other list of this dialogue, and a third one that takes only what
    // its button needs. `Stretch` divides the free width evenly between the two first; the third is served first,
    // because a button has a width below which it cannot be read, and `ResizeToContents` cannot find it: the
    // button is a cell *widget*, and a view measures a column from its items.
    QHeaderView *header = ui->tableWidget->horizontalHeader();
    header->setSectionResizeMode(ColumnName, QHeaderView::Stretch);
    header->setSectionResizeMode(ColumnFullName, QHeaderView::Stretch);

    if (ui->tableWidget->isColumnHidden(ColumnNameAction))
    {
        return;
    }

    header->setSectionResizeMode(ColumnNameAction, QHeaderView::Interactive);
    header->resizeSection(ColumnNameAction, NameActionWidth());

    // A row is as tall as its text, which is not enough to hold a button: the theme's item padding leaves 30
    // pixels and a button asks for 32, so "Name it" came out cut off at the top and the bottom. The height is
    // taken from the button itself, so a style that wants more room gets it, and every row of the list matches.
    QPushButton probe(tr("Name it"));
    probe.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    ui->tableWidget->verticalHeader()->setDefaultSectionSize(probe.sizeHint().height() + 4);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEditWrongFormula::NameActionWidth() const -> int
{
    // What the column has to hold, measured rather than supposed. The button asks for only 76 pixels for
    // "Name it", and a column that narrow reads as a mistake: a column holding a button and a header needs room
    // around them, so a floor is set well above the bare request.
    constexpr int minWidth = 130;

    QPushButton probe(tr("Name it"));
    probe.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    const int buttonWidth = probe.sizeHint().width();

    // The font of the table, not the font of a button built here: the header draws with its own.
    const QFontMetrics metrics(ui->tableWidget->fontMetrics());
    const QTableWidgetItem *header = ui->tableWidget->horizontalHeaderItem(ColumnNameAction);
    const int headerWidth = header == nullptr ? 0 : metrics.horizontalAdvance(header->text()) + 16;

    return qMax(minWidth, qMax(headerWidth, buttonWidth + 16));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::SetTableColumns(bool nameable)
{
    // The header is written with the column count, in one place: `setColumnCount` alone would leave the third
    // label behind and show it again the next time the column comes back.
    if (nameable)
    {
        ui->tableWidget->setColumnCount(ColumnNameAction + 1);
        ui->tableWidget->setHorizontalHeaderLabels({QString(), QString(), tr("Name it")});
    }
    else
    {
        ui->tableWidget->setColumnCount(ColumnFullName + 1);
        ui->tableWidget->setHorizontalHeaderLabels({QString(), QString()});
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Offer to name the object of @a reference, in the row that shows it.
 *
 * The reference is carried by the button rather than by a member: the table is rebuilt on every category change,
 * so a member would have to be kept in step with rows that no longer exist. The row itself is the answer — it is
 * the object the user is reading when the name is missing.
 *
 * @a reference is the pair of points for a line, the variable's own name for a curve: the two families are not
 * named in the same place, and RenameObject reads the kind from the row to know which one it is holding.
 */
void DialogEditWrongFormula::AddNameButton(int row, const QString &reference)
{
    auto *button = new QPushButton(tr("Name it"));
    button->setProperty("reference", reference);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(button, &QPushButton::clicked, this, &DialogEditWrongFormula::RenameObject);
    ui->tableWidget->setCellWidget(row, ColumnNameAction, button);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::RenameObject()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    if (button == nullptr)
    {
        return;
    }

    const QString reference = button->property("reference").toString();
    if (reference.isEmpty())
    {
        return;
    }

    VAbstractPattern *const doc = VAbstractValApplication::VApp()->getCurrentDocument();
    if (doc == nullptr)
    {
        return;
    }

    // The row says which family this button belongs to, and the family decides where the name is written and
    // how it is checked: a line answers to a pair of points and lives in the role-name table, a curve answers
    // to its own tool and carries its alias in the pattern.
    const int row = RowOfWidget(button);
    const QString kind = row >= 0 ? ui->tableWidget->item(row, ColumnName)->data(Qt::UserRole + 1).toString()
                                  : QString();
    if (kind.isEmpty())
    {
        return;
    }

    const bool curve = kind == QStringLiteral("curve");
    const QString oldSuffix = curve ? CurveAlias(reference) : m_data->LineRoleName(reference);

    const QString title = curve ? tr("Name the curve") : tr("Name the line");
    bool accepted = false;
    const QString proposed =
        QInputDialog::getText(this, title, tr("Name:"), QLineEdit::Normal, oldSuffix, &accepted);
    if (not accepted)
    {
        return;
    }

    const QString suffix = proposed.trimmed();
    if (suffix.isEmpty())
    {
        return;
    }

    // The name has to be one a formula can carry, and the pattern's own rule says which: a name no formula
    // could write would be worse than no name at all. The tool checks it again before writing — both families
    // refuse a name that does not match, and a curve also refuses one already taken — but a refusal nobody
    // explains would leave the row looking as if nothing had happened.
    if (not QRegularExpression(NameRegExp()).match(suffix).hasMatch())
    {
        QMessageBox::warning(this, title, tr("The name %1 cannot be used in a formula.").arg(suffix));
        return;
    }

    if (curve)
    {
        // The curve's tool writes its own name, checks that it is unique, and pushes the rename — which is
        // undoable, and re-parses the pattern. Whether it took is not readable here: the container this dialogue
        // holds is refreshed by that parse, so asking it now would answer with the name from before.
        auto *tool = qobject_cast<VToolAbstractCurve *>(VAbstractPattern::FindTool(CurveToolId(reference)));
        if (tool == nullptr)
        {
            return;
        }

        tool->SetAliasSuffix(suffix);

        // The formula this dialogue is holding was not in the pattern when the rename ran, so the rename could not
        // repair it: it is repaired here, with the name just given, and the field is told. Without this the user
        // would be left reading the handle they have just replaced, with Ok still refusing it.
        EvalFormula();
        ShowCurrentCategory();
        return;
    }

    // The name has to be unique among the lines of the piece: two lines answering to one name would leave the
    // pattern unable to say which one a formula means. The container refuses a duplicate silently, so the
    // question is asked here, where an answer can still be given.
    for (auto i = m_data->LineRoleNames().constBegin(); i != m_data->LineRoleNames().constEnd(); ++i)
    {
        if (i.key() != reference && i.value() == suffix)
        {
            QMessageBox::warning(this, title, tr("The name %1 is already used by another line.").arg(suffix));
            return;
        }
    }

    auto *command = new VChangeLineRoleName(doc, reference, oldSuffix, suffix);
    connect(command, &VChangeLineRoleName::NeedLiteParsing, doc, &VAbstractPattern::LiteParseTree);
    VAbstractApplication::VApp()->getUndoStack()->push(command);

    // The formula held here is repaired the same way as the curve above, and for the same reason.
    EvalFormula();

    // The pattern has just re-read its names, and the list is read from the container this dialogue holds: showing
    // the category again is what turns the row that was named into the name that was given, before the user's
    // eyes. A curve is named the same way, one branch above.
    ShowCurrentCategory();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEditWrongFormula::RowOfWidget(const QWidget *widget) const -> int
{
    for (int row = 0; row < ui->tableWidget->rowCount(); ++row)
    {
        if (ui->tableWidget->cellWidget(row, ColumnNameAction) == widget)
        {
            return row;
        }
    }

    return -1;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEditWrongFormula::CurveToolId(const QString &variableName) const -> quint32
{
    const QSharedPointer<VInternalVariable> var = m_data->DataVariables()->value(variableName);
    if (var.isNull())
    {
        return NULL_ID;
    }

    const QSharedPointer<VCurveVariable> curve = qSharedPointerDynamicCast<VCurveVariable>(var);
    return curve.isNull() ? NULL_ID : curve->GetId();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEditWrongFormula::CurveAlias(const QString &variableName) const -> QString
{
    auto *tool = qobject_cast<VToolAbstractCurve *>(VAbstractPattern::FindTool(CurveToolId(variableName)));
    return tool == nullptr ? QString() : tool->GetAliasSuffix();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::ShowCurrentCategory()
{
    // The list is rebuilt from the container this dialogue holds, and the pattern has just re-parsed it: showing
    // the category again is what turns the row that was named into the name that was given, before the user's
    // eyes. Every category the unnamed switch governs is here, because that switch is also what calls this.
    if (ui->radioButtonLengthLine->isChecked())
    {
        LengthLines();
    }
    else if (ui->radioButtonLengthSpline->isChecked())
    {
        LengthCurves();
    }
    else if (ui->radioButtonAngleLine->isChecked())
    {
        AngleLines();
    }
    else if (ui->radioButtonRadiusesArcs->isChecked())
    {
        Arc();
    }
    else if (ui->radioButtonAnglesCurves->isChecked())
    {
        AnglesCurves();
    }
    else if (ui->radioButtonCLength->isChecked())
    {
        CurvesCLength();
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ShowMeasurements show measurements in table
 * @param vars container with measurements
 */
void DialogEditWrongFormula::ShowMeasurements(const QList<QSharedPointer<VMeasurement>> &vars)
{
    QSignalBlocker blocker(ui->tableWidget);
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);
    // A measurement is named by the measurement file it comes from: there is nothing here to give it, so the
    // column that offers to name an object is not part of this table at all.
    SetTableColumns(false);
    ui->tableWidget->setColumnHidden(ColumnFullName, false);
    ui->labelDescription->setText(QString());

    for (const auto &var : vars)
    {
        if (ui->checkBoxHideEmpty->isVisible() && ui->checkBoxHideEmpty->isChecked() && var->IsNotUsed())
        {
            continue; // skip this measurement
        }

        if (not var->Filter(m_toolId))
        { // If we create this variable don't show
            ui->tableWidget->setRowCount(ui->tableWidget->rowCount() + 1);
            auto *itemName = new QTableWidgetItem(var->GetName());
            QFont fontName = itemName->font();
            fontName.setBold(true);
            itemName->setFont(fontName);
            itemName->setToolTip(itemName->text());

            auto *itemFullName = new QTableWidgetItem();
            QFont fontFullName = itemName->font();
            fontFullName.setBold(true);
            itemFullName->setFont(fontFullName);
            if (var->IsCustom())
            {
                itemFullName->setText(var->GetGuiText());
            }
            else
            {
                itemFullName->setText(QString());
                if (VKnownMeasurementsDatabase  const*db = VAbstractApplication::VApp()->KnownMeasurementsDatabase())
                {
                    VKnownMeasurements const known = db->KnownMeasurements(var->GetKnownMeasurementsId());
                    if (known.IsValid())
                    {
                        itemFullName->setText(known.Measurement(var->GetName()).fullName);
                    }
                }
            }

            itemFullName->setToolTip(itemFullName->text());
            ui->tableWidget->setItem(ui->tableWidget->rowCount() - 1, ColumnName, itemName);
            ui->tableWidget->setItem(ui->tableWidget->rowCount() - 1, ColumnFullName, itemFullName);
        }
    }

    blocker.unblock();
    ui->tableWidget->selectRow(0);
    // Placed by hand like every other list: the columns were left to `Stretch` and the name column took an
    // even share of the width, which is too little for a name beside a button.
    ApplyColumnWidths();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ShowFunctions show functions in list
 */
void DialogEditWrongFormula::ShowFunctions()
{
    QSignalBlocker blocker(ui->tableWidget);
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);
    // A function is a name the calculator already knows: there is nothing here to name.
    SetTableColumns(false);
    ui->tableWidget->setColumnHidden(ColumnFullName, true);
    ui->labelDescription->setText(QString());

    VCommonSettings  const*settings = VAbstractApplication::VApp()->Settings();
    const VTranslateVars *trVars = VAbstractApplication::VApp()->TrVars();
    const QMap<QString, qmu::QmuTranslation> functionsDescriptions = trVars->GetFunctionsDescriptions();
    const QMap<QString, qmu::QmuTranslation> functions = trVars->GetFunctions();
    QMap<QString, qmu::QmuTranslation>::const_iterator i = functions.constBegin();
    while (i != functions.constEnd())
    {
        ui->tableWidget->setRowCount(ui->tableWidget->rowCount() + 1);

        QString const name = (!settings->IsTranslateFormula()
                                  ? i.value().getMsourceText()
                                  : i.value().translate(VAbstractApplication::VApp()->Settings()->GetLocale()));

        auto *item = new QTableWidgetItem(name);
        item->setData(Qt::UserRole, i.key());
        QFont font = item->font();
        font.setBold(true);
        item->setFont(font);
        ui->tableWidget->setItem(ui->tableWidget->rowCount() - 1, ColumnName, item);

        if (functionsDescriptions.contains(i.key()))
        {
            QString const description =
                (!settings->IsTranslateFormula() ? functionsDescriptions.value(i.key()).getMsourceText()
                                                 : functionsDescriptions.value(i.key()).translate(
                                                       VAbstractApplication::VApp()->Settings()->GetLocale()));
            item->setToolTip(description);
        }
        else
        {
            item->setToolTip(i.value().getMdisambiguation());
        }
        ++i;
    }

    blocker.unblock();
    ui->tableWidget->selectRow(0);
    // Placed by hand like every other list: the columns were left to `Stretch` and the name column took an
    // even share of the width, which is too little for a name beside a button.
    ApplyColumnWidths();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ShowConstants show the formula constants in list
 *
 * Unlike a function name, a constant token is never translated and never carries parentheses: it is shown exactly as
 * it has to be typed in the formula, and a double click puts it there as it is (see PutVal()).
 */
void DialogEditWrongFormula::ShowConstants()
{
    QSignalBlocker blocker(ui->tableWidget);
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);
    // A constant is a name the calculator already knows: there is nothing here to name. The second column
    // stays, because a constant carries a description worth reading beside its token.
    SetTableColumns(false);
    ui->tableWidget->setColumnHidden(ColumnFullName, false);
    ui->labelDescription->setText(QString());

    VCommonSettings const *settings = VAbstractApplication::VApp()->Settings();
    const QList<VFormulaConstant> constants = VAbstractApplication::VApp()->TrVars()->GetConstants();
    for (const auto &constant : constants)
    {
        ui->tableWidget->setRowCount(ui->tableWidget->rowCount() + 1);

        auto *item = new QTableWidgetItem(constant.token);
        QFont font = item->font();
        font.setBold(true);
        item->setFont(font);

        // The description is documentation, so it is always localized — the container names setting that switches
        // formulas between internal and translated notation has nothing to say about it.
        const QString description = constant.description.translate(settings->GetLocale());
        item->setToolTip(description);

        auto *itemDescription = new QTableWidgetItem(description);
        itemDescription->setToolTip(description);

        ui->tableWidget->setItem(ui->tableWidget->rowCount() - 1, ColumnName, item);
        ui->tableWidget->setItem(ui->tableWidget->rowCount() - 1, ColumnFullName, itemDescription);
    }

    blocker.unblock();
    ui->tableWidget->selectRow(0);
    // Placed by hand like every other list: the columns were left to `Stretch` and the name column took an
    // even share of the width, which is too little for a name beside a button.
    ApplyColumnWidths();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::ShowIncrementsInPreviewCalculation(bool show)
{
    SetCategoryCheckBoxes(false, false);

    QMap<QString, QSharedPointer<VIncrement>> increments;

    const QMap<QString, QSharedPointer<VIncrement>> list = m_data->DataIncrements();
    QMap<QString, QSharedPointer<VIncrement>>::const_iterator i = list.constBegin();
    while (i != list.constEnd())
    {
        if (i.value()->IsPreviewCalculation() == show)
        {
            increments.insert(i.key(), i.value());
        }
        ++i;
    }

    QList<QSharedPointer<VIncrement>> vars = increments.values();
    std::stable_sort(vars.begin(), vars.end(),
                     [](const QSharedPointer<VIncrement> &obj1, const QSharedPointer<VIncrement> &obj2)
                     { return obj1->GetIndex() < obj2->GetIndex(); });

    ShowVariable(vars);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::SetMeasurementDescription(QTableWidgetItem *item, const QString &name)
{
    const QSharedPointer<VMeasurement> stable = m_data->GetVariable<VMeasurement>(name);

    QString description;

    if (!stable->IsCustom())
    {
        if (VKnownMeasurementsDatabase  const*db = VAbstractApplication::VApp()->KnownMeasurementsDatabase())
        {
            VKnownMeasurements const known = db->KnownMeasurements(stable->GetKnownMeasurementsId());
            if (known.IsValid())
            {
                description = known.Measurement(stable->GetName()).description;
            }
        }
    }
    else
    {
        description = stable->GetDescription();
    }

    SetDescription(item->text(), *stable->GetValue(), stable->IsSpecialUnits(), description);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::SetPieceAreaDescription(QTableWidgetItem *item, const QString &name)
{
    const bool specialUnits = false;
    const QSharedPointer<VPieceArea> var = m_data->GetVariable<VPieceArea>(name);
    QString description = tr("Area of piece");

    try
    {
        VPiece const piece = m_data->GetPiece(var->GetPieceId());
        QString const name = piece.GetName();
        if (not name.isEmpty())
        {
            description += QStringLiteral(" '%1'").arg(piece.GetName());
        }
    }
    catch (const VExceptionBadId &)
    {
        // do nothing
    }

    SetDescription(item->text(), *var->GetValue(), specialUnits, description, true);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEditWrongFormula::FilterVariablesEdited(const QString &filter)
{
    const QSignalBlocker blocker(ui->tableWidget);

    // If filter is empty findItems() for unknown reason returns nullptr items.
    // See issue #586. https://bitbucket.org/dismine/valentina/issues/586/valentina-crashes-if-clear-input-filter
    if (filter.isEmpty())
    {
        // show all rows
        for (auto i = 0; i < ui->tableWidget->rowCount(); ++i)
        {
            ui->tableWidget->showRow(i);
        }
    }
    else
    {
        // hide all rows
        for (auto i = 0; i < ui->tableWidget->rowCount(); i++)
        {
            ui->tableWidget->hideRow(i);
        }

        // show rows with matched filter
        const QList<QTableWidgetItem *> items = ui->tableWidget->findItems(filter, Qt::MatchContains);
        for (auto *item : items)
        {
            // If filter is empty findItems() for unknown reason returns nullptr
            // items.
            if (item)
            {
                ui->tableWidget->showRow(item->row());
            }
        }
    }
}
