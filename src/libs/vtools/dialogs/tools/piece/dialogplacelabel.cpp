/************************************************************************
 **
 **  @file   dialogplacelabel.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   14 10, 2017
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2017 Valentina project
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
#include "dialogplacelabel.h"
#include "../dialogtoolbox.h"

#include "../../../visualization/line/ovistoolplacelabel.h"
#include "../../support/dialogeditwrongformula.h"
#include "../vpatterndb/calculator.h"
#include "../vmisc/FactoryWidgets.h"
#include "../vmisc/vabstractapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "vtheme.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScopedPointer>
#include <QSize>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QDebug>
#include <qnumeric.h>

//---------------------------------------------------------------------------------------------------------------------
DialogPlaceLabel::DialogPlaceLabel(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    m_showMode(false),
    timerAngle(new QTimer(this)),
    timerWidth(new QTimer(this)),
    timerHeight(new QTimer(this)),
    m_timerVisible(new QTimer(this)),
    m_flagPoint(false),
    m_flagWidth(false),
    m_flagHeight(false),
    m_flagAngle(false),
    m_flagFormulaVisible(false),
    m_flagError(false)
{
    BuildUi();

    // Validation live (debounce) : chaque frappe relance l'évaluation de la formule du champ.
    connect(m_widthEdit, &QLineEdit::textChanged, this, [this]() { timerWidth->start(formulaTimerTimeout); });
    connect(m_heightEdit, &QLineEdit::textChanged, this, [this]() { timerHeight->start(formulaTimerTimeout); });
    connect(m_angleEdit, &QLineEdit::textChanged, this, [this]() { timerAngle->start(formulaTimerTimeout); });
    connect(m_visibleEdit, &QLineEdit::textChanged, this, [this]() { m_timerVisible->start(formulaTimerTimeout); });

    timerWidth->setSingleShot(true);
    timerHeight->setSingleShot(true);
    timerAngle->setSingleShot(true);
    m_timerVisible->setSingleShot(true);

    connect(timerWidth, &QTimer::timeout, this, &DialogPlaceLabel::EvalWidth);
    connect(timerHeight, &QTimer::timeout, this, &DialogPlaceLabel::EvalHeight);
    connect(timerAngle, &QTimer::timeout, this, &DialogPlaceLabel::EvalAngle);
    connect(m_timerVisible, &QTimer::timeout, this, &DialogPlaceLabel::EvalVisible);

    // Boutons "fx" → éditeur de formule.
    connect(m_widthFx, &QToolButton::clicked, this, &DialogPlaceLabel::FXWidth);
    connect(m_heightFx, &QToolButton::clicked, this, &DialogPlaceLabel::FXHeight);
    connect(m_angleFx, &QToolButton::clicked, this, &DialogPlaceLabel::FXAngle);
    connect(m_visibleFx, &QToolButton::clicked, this, &DialogPlaceLabel::FXVisible);

    connect(m_pieceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this]() { CheckPieces(); });

    FillPlaceLabelTypes();

    // Valeurs par défaut d'une création (comportement d'origine).
    m_widthEdit->setText(QString::number(UnitConvertor(1, Unit::Cm, VAbstractValApplication::VApp()->patternUnits())));
    m_heightEdit->setText(QString::number(UnitConvertor(1, Unit::Cm, VAbstractValApplication::VApp()->patternUnits())));
    m_angleEdit->setText(QStringLiteral("0"));
    m_visibleEdit->setText(QStringLiteral("1"));

    // Évaluation initiale → colore les labels et (dés)active OK selon l'état.
    EvalWidth();
    EvalHeight();
    EvalAngle();
    EvalVisible();

    vis = new OVisToolPlaceLabel(&this->data);
}

//---------------------------------------------------------------------------------------------------------------------
DialogPlaceLabel::~DialogPlaceLabel() = default;

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::EnbleShowMode(bool disable)
{
    m_showMode = disable;
    m_pieceCombo->setDisabled(m_showMode);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPlaceLabel::GetCenterPoint() const -> quint32
{
    return m_centerPoint;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::SetCenterPoint(quint32 id)
{
    m_centerPoint = id;

    if (not m_showMode)
    {
        auto *point = qobject_cast<OVisToolPlaceLabel *>(vis);
        SCASSERT(point != nullptr)
        point->SetPointIds(QVector<quint32>{id});
    }

    CheckPoint();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPlaceLabel::GetLabelType() const -> PlaceLabelType
{
    return static_cast<PlaceLabelType>(m_typeCombo->currentData().toInt());
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::SetLabelType(PlaceLabelType type)
{
    const qint32 index = m_typeCombo->findData(static_cast<int>(type));
    if (index != -1)
    {
        m_typeCombo->setCurrentIndex(index);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPlaceLabel::GetWidth() const -> QString
{
    return VTranslateVars::TryFormulaFromUser(m_widthEdit->text(),
                                              VAbstractApplication::VApp()->Settings()->GetOsSeparator());
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::SetWidth(const QString &value)
{
    const QString formula = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
        value, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
    m_widthEdit->setText(formula);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPlaceLabel::GetHeight() const -> QString
{
    return VTranslateVars::TryFormulaFromUser(m_heightEdit->text(),
                                              VAbstractApplication::VApp()->Settings()->GetOsSeparator());
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::SetHeight(const QString &value)
{
    const QString formula = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
        value, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
    m_heightEdit->setText(formula);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPlaceLabel::GetAngle() const -> QString
{
    return VTranslateVars::TryFormulaFromUser(m_angleEdit->text(),
                                              VAbstractApplication::VApp()->Settings()->GetOsSeparator());
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::SetAngle(const QString &value)
{
    const QString formula = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
        value, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
    m_angleEdit->setText(formula);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPlaceLabel::GetPieceId() const -> quint32
{
    return getCurrentObjectId(m_pieceCombo);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::SetPieceId(quint32 id)
{
    if (m_pieceCombo->count() <= 0)
    {
        m_pieceCombo->addItem(data.GetPiece(id).GetName(), id);
    }
    else
    {
        const qint32 index = m_pieceCombo->findData(id);
        if (index != -1)
        {
            m_pieceCombo->setCurrentIndex(index);
        }
        else
        {
            m_pieceCombo->setCurrentIndex(0);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::SetPiecesList(const QVector<quint32> &list)
{
    FillComboBoxPiecesList(m_pieceCombo, list);

    if (list.isEmpty())
    {
        qWarning() << tr("The list of pieces is empty. Please, first create at least one piece for current pattern "
                         "piece.");
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::CheckDependencyTreeComplete()
{
    // does nothing
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::ChosenObject(quint32 id, const SceneObject &type)
{
    if (m_showMode || prepare || type != SceneObject::Point)
    {
        return; // Dialog déjà ouverte ou mode édition : la scène n'ajoute plus de points.
    }

    // Collecte : clic = (dé)sélection d'une ancre. Re-clic sur un point déjà retenu → on le retire.
    const qsizetype index = m_collectedPoints.indexOf(id);
    if (index != -1)
    {
        m_collectedPoints.removeAt(index);
    }
    else
    {
        m_collectedPoints.append(id);
    }

    auto *point = qobject_cast<OVisToolPlaceLabel *>(vis);
    SCASSERT(point != nullptr)
    point->SetPointIds(m_collectedPoints);

    CheckState(); // OK s'active dès qu'au moins un point est retenu et que les formules sont valides.
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPlaceLabel::GetCenterPoints() const -> QVector<quint32>
{
    return m_collectedPoints;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::ShowDialog(bool click)
{
    if (click || m_showMode || prepare)
    {
        return; // Un relâchement souris pendant la collecte ne doit pas ouvrir la dialog.
    }

    if (m_collectedPoints.isEmpty())
    {
        return; // Entrée sans point retenu : on reste en collecte.
    }

    prepare = true;

    CheckState();

    this->setModal(true);
    this->show();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::ShowVisualization()
{
    if (not m_showMode)
    {
        AddVisualization<OVisToolPlaceLabel>();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        RetranslateUi();
    }

    if (event->type() == QEvent::PaletteChange)
    {
        SetFxIcons();
    }

    // remember to call base class implementation
    DialogTool::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::EvalWidth()
{
    EvalFormulaField(m_widthEdit, m_widthLabel, m_flagWidth,
                     UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true), /*checkLessThanZero*/ true);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::EvalHeight()
{
    EvalFormulaField(m_heightEdit, m_heightLabel, m_flagHeight,
                     UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true), /*checkLessThanZero*/ true);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::EvalAngle()
{
    EvalFormulaField(m_angleEdit, m_angleLabel, m_flagAngle, degreeSymbol, /*checkLessThanZero*/ false);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::EvalVisible()
{
    EvalFormulaField(m_visibleEdit, m_visibleLabel, m_flagFormulaVisible, QString(), /*checkLessThanZero*/ true);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::EvalFormulaField(QLineEdit *edit, QLabel *label, bool &flag, const QString &postfix,
                                        const bool checkLessThanZero)
{
    flag = false;
    const QString text = edit->text().trimmed();
    QString reason;

    if (text.isEmpty())
    {
        reason = tr("Empty formula");
    }
    else
    {
        try
        {
            // Traduit vers la notation interne puis évalue la formule brute.
            const QString formula = VAbstractApplication::VApp()->TrVars()->FormulaFromUser(
                text, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
            QScopedPointer<Calculator> const cal(new Calculator());
            const qreal result = cal->EvalFormula(data.DataVariables(), formula);

            if (qIsInf(result) || qIsNaN(result))
            {
                reason = tr("Invalid result. Value is infinite or NaN. Please, check your calculations.");
            }
            else if (checkLessThanZero && result < 0)
            {
                reason = tr("Value can't be less than 0");
            }
            else
            {
                flag = true;
            }
        }
        catch (qmu::QmuParserError &e)
        {
            reason = tr("Parser error: %1").arg(e.GetMsg());
            qDebug() << "\nMath parser error:\n"
                     << "--------------------------------------\n"
                     << "Message:     " << e.GetMsg() << "\n"
                     << "Expression:  " << e.GetExpr() << "\n"
                     << "--------------------------------------";
        }
    }

    if (not flag)
    {
        ChangeColor(label, errorColor);
        edit->setToolTip(postfix.isEmpty() ? reason : reason + QStringLiteral(" (") + postfix + QLatin1Char(')'));
    }
    else
    {
        ChangeColor(label, OkColor(this));
        edit->setToolTip(QString());
    }

    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::FXWidth()
{
    QScopedPointer<DialogEditWrongFormula> const dialog(new DialogEditWrongFormula(&data, toolId, this));
    dialog->setWindowTitle(tr("Edit rectangle width"));
    dialog->SetFormula(GetWidth());
    dialog->setCheckLessThanZero(true);
    dialog->setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
    if (dialog->exec() == QDialog::Accepted)
    {
        SetWidth(dialog->GetFormula());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::FXHeight()
{
    QScopedPointer<DialogEditWrongFormula> const dialog(new DialogEditWrongFormula(&data, toolId, this));
    dialog->setWindowTitle(tr("Edit rectangle width"));
    dialog->SetFormula(GetHeight());
    dialog->setCheckLessThanZero(true);
    dialog->setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
    if (dialog->exec() == QDialog::Accepted)
    {
        SetHeight(dialog->GetFormula());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::FXAngle()
{
    QScopedPointer<DialogEditWrongFormula> const dialog(new DialogEditWrongFormula(&data, toolId, this));
    dialog->setWindowTitle(tr("Edit angle"));
    dialog->SetFormula(GetAngle());
    dialog->setPostfix(degreeSymbol);
    if (dialog->exec() == QDialog::Accepted)
    {
        SetAngle(dialog->GetFormula());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::FXVisible()
{
    QScopedPointer<DialogEditWrongFormula> const dialog(new DialogEditWrongFormula(&data, toolId, this));
    dialog->setWindowTitle(tr("Control visibility"));
    dialog->SetFormula(GetFormulaVisible());
    if (dialog->exec() == QDialog::Accepted)
    {
        SetFormulaVisible(dialog->GetFormula());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::BuildUi()
{
    auto *grid = new QGridLayout();
    grid->setHorizontalSpacing(6);
    grid->setVerticalSpacing(6);
    grid->setColumnStretch(1, 1);

    int row = 0;

    // Piece
    m_pieceLabel = new QLabel(this);
    m_pieceCombo = MakeComboBox(this);
    grid->addWidget(m_pieceLabel, row, 0);
    grid->addWidget(m_pieceCombo, row, 1, 1, 2);
    ++row;

    // Width · Height · Angle : 1 ligne = label + QLineEdit + fx
    m_widthLabel = new QLabel(this);
    m_widthEdit = new QLineEdit(this);
    m_widthFx = new QToolButton(this);
    grid->addWidget(m_widthLabel, row, 0);
    grid->addWidget(m_widthEdit, row, 1);
    grid->addWidget(m_widthFx, row, 2);
    ++row;

    m_heightLabel = new QLabel(this);
    m_heightEdit = new QLineEdit(this);
    m_heightFx = new QToolButton(this);
    grid->addWidget(m_heightLabel, row, 0);
    grid->addWidget(m_heightEdit, row, 1);
    grid->addWidget(m_heightFx, row, 2);
    ++row;

    m_angleLabel = new QLabel(this);
    m_angleEdit = new QLineEdit(this);
    m_angleFx = new QToolButton(this);
    grid->addWidget(m_angleLabel, row, 0);
    grid->addWidget(m_angleEdit, row, 1);
    grid->addWidget(m_angleFx, row, 2);
    ++row;

    // Type
    m_typeLabel = new QLabel(this);
    m_typeCombo = MakeComboBox(this);
    grid->addWidget(m_typeLabel, row, 0);
    grid->addWidget(m_typeCombo, row, 1, 1, 2);
    ++row;

    // Separator
    auto *separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    grid->addWidget(separator, row, 0, 1, 3);
    ++row;

    // Visibility
    m_visibleLabel = new QLabel(this);
    m_visibleEdit = new QLineEdit(this);
    m_visibleFx = new QToolButton(this);
    grid->addWidget(m_visibleLabel, row, 0);
    grid->addWidget(m_visibleEdit, row, 1);
    grid->addWidget(m_visibleFx, row, 2);
    ++row;

    // Légende en retrait (sous le champ, colonne des valeurs — pas alignée avec les labels).
    m_visibleHint = new QLabel(this);
    m_visibleHint->setWordWrap(true);
    grid->addWidget(m_visibleHint, row, 1, 1, 2);
    ++row;

    // Not mirrored — alignée col. gauche (avec les labels des champs), PAS sous la légende en retrait.
    m_notMirroredCheck = new QCheckBox(this);
    grid->addWidget(m_notMirroredCheck, row, 0, 1, 3);
    ++row;

    grid->setRowStretch(row, 1);

    // Boutons "fx" : petits carrés, NoFocus pour ne pas voler le focus au QLineEdit.
    const auto configureFx = [](QToolButton *button) {
        button->setFocusPolicy(Qt::NoFocus);
        button->setFixedSize(24, 24);
        button->setIconSize(QSize(16, 16));
    };
    configureFx(m_widthFx);
    configureFx(m_heightFx);
    configureFx(m_angleFx);
    configureFx(m_visibleFx);

    // Boutons OK / Cancel (équivalent de DialogTool::InitOkCancel, sans .ui).
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    bOk = buttonBox->button(QDialogButtonBox::Ok);
    SCASSERT(bOk != nullptr)
    connect(bOk, &QPushButton::clicked, this, &DialogTool::DialogAccepted);

    QPushButton *bCancel = buttonBox->button(QDialogButtonBox::Cancel);
    SCASSERT(bCancel != nullptr)
    connect(bCancel, &QPushButton::clicked, this, &DialogTool::DialogRejected);

    VAbstractApplication::VApp()->Settings()->GetOsSeparator() ? setLocale(QLocale()) : setLocale(QLocale::c());

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(grid);
    mainLayout->addWidget(buttonBox);

    RetranslateUi();
    SetFxIcons();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::RetranslateUi()
{
    setWindowTitle(tr("Internal mark"));
    m_pieceLabel->setText(tr("Piece"));
    m_widthLabel->setText(tr("Width"));
    m_heightLabel->setText(tr("Height"));
    m_angleLabel->setText(tr("Angle"));
    m_typeLabel->setText(tr("Type"));
    m_visibleLabel->setText(tr("Visibility"));
    m_visibleHint->setText(tr("1 = visible, 0 = invisible, formulas allowed"));
    m_notMirroredCheck->setText(tr("Not mirrored"));
    const QString fxToolTip = tr("Edit expression");
    m_widthFx->setToolTip(fxToolTip);
    m_heightFx->setToolTip(fxToolTip);
    m_angleFx->setToolTip(fxToolTip);
    m_visibleFx->setToolTip(fxToolTip);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::SetFxIcons()
{
    const QString resource = QStringLiteral("icon");
    const QString fxIcon = QStringLiteral("24x24/fx.png");
    m_widthFx->setIcon(VTheme::GetIconResource(resource, fxIcon));
    m_heightFx->setIcon(VTheme::GetIconResource(resource, fxIcon));
    m_angleFx->setIcon(VTheme::GetIconResource(resource, fxIcon));
    m_visibleFx->setIcon(VTheme::GetIconResource(resource, fxIcon));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::FillPlaceLabelTypes()
{
    m_typeCombo->addItem(tr("Cross"), static_cast<int>(PlaceLabelType::Cross));
    m_typeCombo->addItem(tr("Segment"), static_cast<int>(PlaceLabelType::Segment));
    m_typeCombo->addItem(tr("Rectangle"), static_cast<int>(PlaceLabelType::Rectangle));
    m_typeCombo->addItem(tr("T-shaped"), static_cast<int>(PlaceLabelType::Tshaped));
    m_typeCombo->addItem(tr("Doubletree"), static_cast<int>(PlaceLabelType::Doubletree));
    m_typeCombo->addItem(tr("Corner"), static_cast<int>(PlaceLabelType::Corner));
    m_typeCombo->addItem(tr("Triangle"), static_cast<int>(PlaceLabelType::Triangle));
    m_typeCombo->addItem(tr("H-shaped"), static_cast<int>(PlaceLabelType::Hshaped));
    m_typeCombo->addItem(tr("Button"), static_cast<int>(PlaceLabelType::Button));
    m_typeCombo->addItem(tr("Circle"), static_cast<int>(PlaceLabelType::Circle));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::CheckPieces()
{
    QColor color;
    if (m_pieceCombo->count() <= 0 || m_pieceCombo->currentIndex() == -1)
    {
        m_flagError = false;
        color = errorColor;
    }
    else
    {
        m_flagError = true;
        color = OkColor(this);
    }
    ChangeColor(m_pieceLabel, color);
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::CheckPoint()
{
    const bool ok = m_centerPoint != NULL_ID;
    m_flagPoint = ok;
    ChangeColor(m_pieceLabel, ok ? OkColor(this) : errorColor);
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPlaceLabel::GetFormulaVisible() const -> QString
{
    QString const formula = m_visibleEdit->text();
    return VTranslateVars::TryFormulaFromUser(formula, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::SetFormulaVisible(const QString &formula)
{
    const QString f = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
        formula, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
    m_visibleEdit->setText(f);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPlaceLabel::IsNotMirrored() const -> bool
{
    return m_notMirroredCheck->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPlaceLabel::SetNotMirrored(bool value)
{
    m_notMirroredCheck->setChecked(value);
}
