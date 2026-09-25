/************************************************************************
 **
 **  @file   odetailsnodewidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   09/09/2026
 **
 **  @brief  Property browser page for a piece contour (main path) node in the
 **          Details dock — passmark and seam allowance angle options of the point.
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

#include "odetailsnodewidget.h"

#include "osymbolshapes.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QScopedPointer>
#include <QSize>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtMath>

#include "../vmisc/FactoryWidgets.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/calculator.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpassmark.h"
#include "../vpatterndb/vpiece.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vpatterndb/vpiecepath.h"
#include "../vtools/dialogs/support/dialogeditwrongformula.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vtools/undocommands/savepieceoptions.h"
#include "vtheme.h"

namespace
{
// "None" in the passmark Type combo means the node is not a passmark at all
// (there is no PassmarkLineType::None).
constexpr int kNoPassmark = -1;

// The notch types and the words they go by are the shared catalogue's (OSymbolShapes) — a notch is
// named in one place, and the symbol library offers the same words this page does. A "Check mark" is
// one type and two shapes there, told apart by its opening, so the combo is read by index: findData()
// would return the first match for both.
constexpr int kFirstLineTypeIndex = 1; // "None" sits at index 0.

struct PassmarkAngleEntry
{
    PassmarkAngleType type;
    const char *name;
};

constexpr PassmarkAngleEntry kPassmarkAngleTypes[] = {
    {PassmarkAngleType::Straightforward, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Straightforward")},
    {PassmarkAngleType::Bisector, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Bisector")},
    {PassmarkAngleType::Intersection, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Intersection")},
    {PassmarkAngleType::IntersectionOnlyLeft,
     QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Intersection (only left)")},
    {PassmarkAngleType::IntersectionOnlyRight,
     QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Intersection (only right)")},
    {PassmarkAngleType::Intersection2, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Intersection 2")},
    {PassmarkAngleType::Intersection2OnlyLeft,
     QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Intersection 2 (only left)")},
    {PassmarkAngleType::Intersection2OnlyRight,
     QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Intersection 2 (only right)")},
};

struct SeamAllowanceAngleEntry
{
    PieceNodeAngle type;
    const char *name;
};

constexpr SeamAllowanceAngleEntry kSeamAllowanceAngles[] = {
    {PieceNodeAngle::ByLength, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "By length")},
    {PieceNodeAngle::ByPointsIntersection, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "By points intersection")},
    {PieceNodeAngle::ByFirstEdgeSymmetry, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "By first edge symmetry")},
    {PieceNodeAngle::BySecondEdgeSymmetry, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "By second edge symmetry")},
    {PieceNodeAngle::ByFirstEdgeRightAngle, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "By first edge right angle")},
    {PieceNodeAngle::BySecondEdgeRightAngle, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "By second edge right angle")},
};
} // namespace

//---------------------------------------------------------------------------------------------------------------------
ODetailsNodeWidget::ODetailsNodeWidget(QWidget *parent)
  : QWidget(parent)
{
    // "fx" buttons open the standard formula editor. NoFocus so a click does not steal the focus
    // from the QLineEdit (which would commit an in-progress, possibly invalid, formula first).
    const auto makeFxButton = [this]() {
        auto *button = new QToolButton(m_passmarkGroup);
        button->setFocusPolicy(Qt::NoFocus);
        button->setFixedSize(24, 24);
        button->setIconSize(QSize(16, 16));
        button->setToolTip(tr("Edit expression"));
        m_fxButtons.append(button); // The icon itself is set by RefreshFxIcons(), which follows the theme.
        return button;
    };

    // ── Section "Passmark" (framed group, like the place-label page) ──────────
    m_passmarkGroup = new QGroupBox(tr("Passmark"), this);
    auto *passmarkGrid = new QGridLayout(m_passmarkGroup);
    passmarkGrid->setSpacing(6);
    passmarkGrid->setColumnStretch(1, 1);

    int row = 0;

    // Type (line type of the notch; "None" removes the passmark).
    m_typeLabel = new QLabel(tr("Type"), m_passmarkGroup);
    m_typeCombo = MakeComboBox(m_passmarkGroup);
    passmarkGrid->addWidget(m_typeLabel, row, 0);
    passmarkGrid->addWidget(m_typeCombo, row, 1, 1, 2);
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ODetailsNodeWidget::CommitPassmarkType);
    ++row;

    // Angle (direction of the notch).
    m_angleLabel = new QLabel(tr("Angle"), m_passmarkGroup);
    m_angleCombo = MakeComboBox(m_passmarkGroup);
    passmarkGrid->addWidget(m_angleLabel, row, 0);
    passmarkGrid->addWidget(m_angleCombo, row, 1, 1, 2);
    connect(m_angleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ODetailsNodeWidget::CommitPassmarkAngle);
    ++row;

    // Show the second passmark on seam line.
    m_secondPassmarkCheck = new QCheckBox(tr("Show the second passmark on seam line"), m_passmarkGroup);
    passmarkGrid->addWidget(m_secondPassmarkCheck, row, 0, 1, 3);
    connect(m_secondPassmarkCheck, &QCheckBox::toggled, this, &ODetailsNodeWidget::CommitShowSecondPassmark);
    ++row;

    // Not mirrored — do not reproduce the notch as a mirrored copy (seam allowance dialog wording).
    m_notMirroredCheck = new QCheckBox(tr("Not Mirrored"), m_passmarkGroup);
    passmarkGrid->addWidget(m_notMirroredCheck, row, 0, 1, 3);
    connect(m_notMirroredCheck, &QCheckBox::toggled, this, &ODetailsNodeWidget::CommitNotMirrored);
    ++row;

    // The three dimensions of the notch. Each row is always visible and pre-filled with the value the
    // engine would draw, so there is nothing to untick to get back to the automatic value — emptying
    // the field is enough.
    const auto addValueRow = [&](const QString &title, FormulaField field, QLabel *&label, QLineEdit *&edit) {
        label = new QLabel(title, m_passmarkGroup);
        edit = new QLineEdit(m_passmarkGroup);
        passmarkGrid->addWidget(label, row, 0);
        passmarkGrid->addWidget(edit, row, 1);
        auto *fx = makeFxButton();
        passmarkGrid->addWidget(fx, row, 2);
        connect(edit, &QLineEdit::editingFinished, this, [this, field]() { CommitFormula(field); });
        connect(fx, &QToolButton::clicked, this, [this, field, edit]() { OpenFormulaEditor(field, edit); });
        ++row;
    };

    addValueRow(tr("Length"), FormulaField::Length, m_lengthLabel, m_lengthEdit);
    addValueRow(tr("Width"), FormulaField::Width, m_widthLabel, m_widthEdit);
    addValueRow(tr("Angle"), FormulaField::Angle, m_valueAngleLabel, m_valueAngleEdit);

    m_valueHint = new QLabel(tr("Leave a field empty for the automatic value, formulas allowed"),
                             m_passmarkGroup);
    m_valueHint->setWordWrap(true);
    passmarkGrid->addWidget(m_valueHint, row, 1, 1, 2);
    ++row;

    // Visibility — two-line block, same style as the place-label page.
    m_visibleLabel = new QLabel(tr("Visibility"), m_passmarkGroup);
    m_visibleEdit = new QLineEdit(m_passmarkGroup);
    passmarkGrid->addWidget(m_visibleLabel, row, 0);
    passmarkGrid->addWidget(m_visibleEdit, row, 1);
    auto *visibleFx = makeFxButton();
    passmarkGrid->addWidget(visibleFx, row, 2);
    connect(m_visibleEdit, &QLineEdit::editingFinished, this,
            [this]() { CommitFormula(FormulaField::Visibility); });
    connect(visibleFx, &QToolButton::clicked, this,
            [this]() { OpenFormulaEditor(FormulaField::Visibility, m_visibleEdit); });
    ++row;

    m_visibleHint = new QLabel(tr("1 = visible, 0 = invisible, formulas allowed"), m_passmarkGroup);
    m_visibleHint->setWordWrap(true);
    passmarkGrid->addWidget(m_visibleHint, row, 1, 1, 2);
    ++row;

    // ── Section "Seam allowance angle" (framed group) ─────────────────────────
    m_seamGroup = new QGroupBox(tr("Seam allowance angle"), this);
    auto *seamGrid = new QGridLayout(m_seamGroup);
    seamGrid->setSpacing(6);
    seamGrid->setColumnStretch(1, 1);

    int seamRow = 0;

    // Type (squaring of the seam allowance at this point).
    m_seamTypeLabel = new QLabel(tr("Type"), m_seamGroup);
    m_seamTypeCombo = MakeComboBox(m_seamGroup);
    seamGrid->addWidget(m_seamTypeLabel, seamRow, 0);
    seamGrid->addWidget(m_seamTypeCombo, seamRow, 1, 1, 2);
    connect(m_seamTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ODetailsNodeWidget::CommitSeamAllowanceAngle);
    ++seamRow;

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(6);
    layout->addWidget(m_passmarkGroup);
    layout->addWidget(m_seamGroup);
    layout->addStretch();

    PopulateCombos();

    RefreshFxIcons();
    // The dock stays open across a theme toggle: nothing else re-reads these icons.
    connect(VTheme::Instance(), &VTheme::ThemeSettingsChanged, this, &ODetailsNodeWidget::RefreshFxIcons);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::PopulateCombos()
{
    m_typeCombo->clear();
    m_typeCombo->addItem(tr("None"), kNoPassmark);
    for (const OSymbolShapes::PassmarkEntry &entry : OSymbolShapes::Passmarks())
    {
        m_typeCombo->addItem(tr(entry.name), static_cast<int>(entry.type));
    }

    m_angleCombo->clear();
    for (const auto &entry : kPassmarkAngleTypes)
    {
        m_angleCombo->addItem(tr(entry.name), static_cast<int>(entry.type));
    }

    m_seamTypeCombo->clear();
    for (const auto &entry : kSeamAllowanceAngles)
    {
        m_seamTypeCombo->addItem(tr(entry.name), static_cast<int>(entry.type));
    }

    m_typeCombo->setCurrentIndex(0);
    m_angleCombo->setCurrentIndex(0);
    m_seamTypeCombo->setCurrentIndex(0);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsNodeWidget::EditFor(FormulaField field) const -> QLineEdit *
{
    switch (field)
    {
        case FormulaField::Length:
            return m_lengthEdit;
        case FormulaField::Width:
            return m_widthEdit;
        case FormulaField::Angle:
            return m_valueAngleEdit;
        case FormulaField::Visibility:
        default:
            return m_visibleEdit;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsNodeWidget::Show(VToolSeamAllowance *piece, quint32 nodeId) -> bool
{
    m_piece = piece;
    m_nodeId = nodeId;
    return LoadFromNode();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsNodeWidget::FetchCurrentNode(VPieceNode &out) const -> bool
{
    if (m_piece == nullptr)
    {
        return false;
    }
    try
    {
        const VPiece det = m_piece->getData()->GetPiece(m_piece->getId());
        const int nodeIndex = det.GetPath().indexOfNode(m_nodeId);
        if (nodeIndex < 0)
        {
            return false;
        }
        out = det.GetPath().at(nodeIndex);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsNodeWidget::FetchPassmark(VPassmark &out) const -> bool
{
    if (m_piece == nullptr)
    {
        return false;
    }

    try
    {
        const VContainer *data = m_piece->getData();
        const VPiece det = data->GetPiece(m_piece->getId());

        // Matched on the node id, never on the index: Passmarks() walks the *united* path, which is not
        // the piece path the dock works from.
        for (const VPassmark &passmark : det.Passmarks(data))
        {
            if (passmark.Data().id == m_nodeId)
            {
                out = passmark;
                return true;
            }
        }
    }
    catch (...)
    {
        return false;
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsNodeWidget::LoadFromNode() -> bool
{
    VPieceNode node;
    if (!FetchCurrentNode(node))
    {
        return false;
    }

    VPassmark passmark;
    const bool hasPassmark = node.IsPassmark() && FetchPassmark(passmark);
    const bool oneLine = hasPassmark && node.GetPassmarkLineType() == PassmarkLineType::OneLine;
    const bool straightforward = node.GetPassmarkAngleType() == PassmarkAngleType::Straightforward;

    m_loading = true;

    // The two "Check mark" entries share their line type, so the combo is matched by index.
    int entry = -1;
    int index = 0;
    for (const OSymbolShapes::PassmarkEntry &candidate : OSymbolShapes::Passmarks())
    {
        if (candidate.type == node.GetPassmarkLineType() &&
            candidate.clockwise == node.IsPassmarkClockwiseOpening())
        {
            entry = index;
            break;
        }
        ++index;
    }
    m_typeCombo->setCurrentIndex(entry >= 0 ? entry + kFirstLineTypeIndex : 0);

    const int angleIndex = m_angleCombo->findData(static_cast<int>(node.GetPassmarkAngleType()));
    m_angleCombo->setCurrentIndex(angleIndex >= 0 ? angleIndex : 0);

    m_secondPassmarkCheck->setChecked(node.IsShowSecondPassmark());
    m_notMirroredCheck->setChecked(node.IsPassmarkNotMirrored());

    FillValueField(FormulaField::Length, node.GetFormulaPassmarkLength(),
                   AutomaticValue(FormulaField::Length, passmark, hasPassmark), true, false);
    if (oneLine)
    {
        // A single line carries no width at all — the row is disabled, so it holds nothing.
        if (!m_widthEdit->hasFocus())
        {
            m_widthEdit->setText(QString());
        }
    }
    else
    {
        FillValueField(FormulaField::Width, node.GetFormulaPassmarkWidth(),
                       AutomaticValue(FormulaField::Width, passmark, hasPassmark), true, false);
    }
    // The automatic angle is only meaningful on the "Straightforward" rule, and asking for it means
    // running the whole seam-allowance geometry — so it is only asked for where the row can be edited.
    // Elsewhere the field keeps whatever the piece holds, which the XML writer empties anyway.
    if (straightforward)
    {
        FillValueField(FormulaField::Angle, node.GetFormulaPassmarkAngle(),
                       AutomaticValue(FormulaField::Angle, passmark, hasPassmark), false, false);
    }
    else if (!m_valueAngleEdit->hasFocus())
    {
        m_valueAngleEdit->setText(node.GetFormulaPassmarkAngle());
    }

    m_visibleEdit->setText(node.GetFormulaPassmarkVisibility());

    const int seamIndex = m_seamTypeCombo->findData(static_cast<int>(node.GetAngleType()));
    m_seamTypeCombo->setCurrentIndex(seamIndex >= 0 ? seamIndex : 0);

    UpdatePassmarkEnabled(node);
    m_loading = false;

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::RefreshFxIcons()
{
    const QString theme = VTheme::ColorSheme() == VColorSheme::Dark ? QStringLiteral("dark")
                                                                    : QStringLiteral("light");
    const QIcon icon(QStringLiteral(":/icons/%1/function_%1.svg").arg(theme));

    for (QToolButton *button : m_fxButtons)
    {
        button->setIcon(icon);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::ReloadAfterCommit()
{
    // Unlike the piece page — repopulated 100 ms after every command by ODetailsPieceWidget's timer —
    // this page has no such refresh: nothing else knows it is open. Without this, the page would keep
    // the state it had when the point was clicked. A type change says which rows apply and what the
    // automatic values are; a length change moves the automatic width. Reading back from the model
    // keeps the two in step, and the field being typed in is left alone.
    if (m_piece != nullptr)
    {
        LoadFromNode();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsNodeWidget::AutomaticValue(FormulaField field, const VPassmark &passmark, bool hasPassmark) const -> qreal
{
    if (!hasPassmark)
    {
        return 0;
    }

    switch (field)
    {
        case FormulaField::Length:
            return passmark.AutomaticLength();
        case FormulaField::Width:
            return passmark.AutomaticWidth();
        case FormulaField::Angle:
        {
            if (m_piece == nullptr)
            {
                return 0;
            }
            // Reaches the seam-allowance geometry, which can refuse under pedantic mode — a property
            // browser must never let that escape, the field simply falls back to zero.
            try
            {
                const VContainer *data = m_piece->getData();
                return passmark.AutomaticAngle(data->GetPiece(m_piece->getId()), data);
            }
            catch (...)
            {
                return 0;
            }
        }
        case FormulaField::Visibility:
        default:
            return 0;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::FillValueField(FormulaField field, const QString &formula, qreal automatic, bool isLength,
                                        bool force)
{
    QLineEdit *edit = EditFor(field);

    // The dock repopulates 100 ms after every commit, so never overwrite the field being typed in —
    // unless this is the restore that follows a refused entry.
    if (!force && edit->hasFocus())
    {
        return;
    }

    if (!formula.isEmpty())
    {
        edit->setText(formula);
        return;
    }

    // No formula of its own: the field shows the value the engine would draw. Length and width are
    // lengths, shown in the pattern unit; the angle is a plain number of degrees and must not go
    // through the unit conversion.
    edit->setText(QString::number(isLength ? VAbstractValApplication::VApp()->fromPixel(automatic) : automatic));
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::UpdatePassmarkEnabled(const VPieceNode &node)
{
    const bool isPassmark = node.IsPassmark();
    const bool oneLine = isPassmark && node.GetPassmarkLineType() == PassmarkLineType::OneLine;
    const bool straightforward = node.GetPassmarkAngleType() == PassmarkAngleType::Straightforward;

    // "None" means the node carries no notch at all: nothing below the type selector has an object.
    m_angleLabel->setEnabled(isPassmark);
    m_angleCombo->setEnabled(isPassmark);
    m_secondPassmarkCheck->setEnabled(isPassmark);
    m_notMirroredCheck->setEnabled(isPassmark);
    m_lengthLabel->setEnabled(isPassmark);
    m_lengthEdit->setEnabled(isPassmark);
    m_visibleLabel->setEnabled(isPassmark);
    m_visibleEdit->setEnabled(isPassmark);
    m_valueHint->setEnabled(isPassmark);

    // A single line has no width, and the XML writer drops the attribute for it as well.
    m_widthLabel->setEnabled(isPassmark && !oneLine);
    m_widthEdit->setEnabled(isPassmark && !oneLine);

    // A hand-typed angle only has something to override on the "Straightforward" rule — the seven other
    // rules derive the notch direction from the corner geometry on their own.
    m_valueAngleLabel->setEnabled(isPassmark && straightforward);
    m_valueAngleEdit->setEnabled(isPassmark && straightforward);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::RestoreField(FormulaField field)
{
    VPieceNode node;
    if (!FetchCurrentNode(node))
    {
        EditFor(field)->clear();
        return;
    }

    VPassmark passmark;
    const bool hasPassmark = node.IsPassmark() && FetchPassmark(passmark);

    FillValueField(field, StoredFormula(node, field), AutomaticValue(field, passmark, hasPassmark),
                   field != FormulaField::Angle, true);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::RetranslateUI()
{
    m_passmarkGroup->setTitle(tr("Passmark"));
    m_seamGroup->setTitle(tr("Seam allowance angle"));
    m_typeLabel->setText(tr("Type"));
    m_angleLabel->setText(tr("Angle"));
    m_secondPassmarkCheck->setText(tr("Show the second passmark on seam line"));
    m_notMirroredCheck->setText(tr("Not Mirrored"));
    m_lengthLabel->setText(tr("Length"));
    m_widthLabel->setText(tr("Width"));
    m_valueAngleLabel->setText(tr("Angle"));
    m_valueHint->setText(tr("Leave a field empty for the automatic value, formulas allowed"));
    m_visibleLabel->setText(tr("Visibility"));
    m_visibleHint->setText(tr("1 = visible, 0 = invisible, formulas allowed"));
    m_seamTypeLabel->setText(tr("Type"));

    // ⚠️ Les boutons « fx » portent une infobulle posée à leur construction : une infobulle n'est pas
    // un widget, la classe de base ne la retouche jamais. Ils sont gardés dans `m_fxButtons`.
    for (QToolButton *button : m_fxButtons)
    {
        button->setToolTip(tr("Edit expression"));
    }

    PopulateCombos();
    if (m_piece != nullptr)
    {
        LoadFromNode();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::PushNodeChange(const std::function<void(VPieceNode &node)> &mutate)
{
    if (m_piece == nullptr)
    {
        return;
    }

    VPiece oldDet;
    try
    {
        oldDet = m_piece->getData()->GetPiece(m_piece->getId());
    }
    catch (...)
    {
        return;
    }

    VPiece newDet = oldDet;
    const int nodeIndex = newDet.GetPath().indexOfNode(m_nodeId);
    if (nodeIndex < 0)
    {
        return;
    }

    VPieceNode node = newDet.GetPath().at(nodeIndex);
    mutate(node);
    newDet.GetPath()[nodeIndex] = node;

    auto *doc = VAbstractValApplication::VApp()->getCurrentDocument();
    if (doc == nullptr)
    {
        return;
    }

    VAbstractApplication::VApp()->getUndoStack()->push(new SavePieceOptions(oldDet, newDet, doc, m_piece->getId()));
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::CommitPassmarkType(int index)
{
    if (m_loading)
    {
        return;
    }

    VPieceNode node;
    if (!FetchCurrentNode(node))
    {
        return;
    }

    if (m_typeCombo->itemData(index).toInt() == kNoPassmark)
    {
        if (!node.IsPassmark())
        {
            return;
        }
        PushNodeChange([](VPieceNode &n) { n.SetPassmark(false); });
        ReloadAfterCommit();
        return;
    }

    const OSymbolShapes::PassmarkEntry &entry = OSymbolShapes::Passmarks().at(index - kFirstLineTypeIndex);
    const bool sameEntry = node.GetPassmarkLineType() == entry.type &&
                           node.IsPassmarkClockwiseOpening() == entry.clockwise;
    if (node.IsPassmark() && sameEntry)
    {
        return;
    }

    // The clockwise opening is not a setting of its own — picking an entry is what sets it. It only
    // means anything for a "Check mark", so every other type stores a plain false.
    //
    // Width and angle go back to the mark's own values: the width is not the same quantity from one
    // mark to the next (a fixed gap for the line marks, a bar length for the T, a V opening, an arc
    // radius), so an override typed for the previous mark would silently keep applying to the new
    // one. Length means the same thing whatever the mark, and is left alone. Coming back from
    // "None" with the same entry is not a change and drops nothing.
    const PassmarkLineType type = entry.type;
    const bool clockwise = entry.clockwise;
    PushNodeChange([type, clockwise, sameEntry](VPieceNode &n) {
        n.SetPassmark(true);
        n.SetPassmarkLineType(type);
        n.SetPassmarkClockwiseOpening(clockwise);
        if (!sameEntry)
        {
            SetOverride(n, FormulaField::Width, QString());
            SetOverride(n, FormulaField::Angle, QString());
        }
    });
    ReloadAfterCommit();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::CommitPassmarkAngle(int index)
{
    if (m_loading)
    {
        return;
    }

    const auto type = static_cast<PassmarkAngleType>(m_angleCombo->itemData(index).toInt());
    VPieceNode node;
    if (!FetchCurrentNode(node))
    {
        return;
    }
    if (node.GetPassmarkAngleType() == type)
    {
        return;
    }
    PushNodeChange([type](VPieceNode &n) { n.SetPassmarkAngleType(type); });
    ReloadAfterCommit();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::CommitShowSecondPassmark(bool checked)
{
    if (m_loading)
    {
        return;
    }
    VPieceNode node;
    if (!FetchCurrentNode(node))
    {
        return;
    }
    if (node.IsShowSecondPassmark() == checked)
    {
        return;
    }
    PushNodeChange([checked](VPieceNode &n) { n.SetShowSecondPassmark(checked); });
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::CommitNotMirrored(bool checked)
{
    if (m_loading)
    {
        return;
    }
    VPieceNode node;
    if (!FetchCurrentNode(node))
    {
        return;
    }
    if (node.IsPassmarkNotMirrored() == checked)
    {
        return;
    }
    PushNodeChange([checked](VPieceNode &n) { n.SetPassmarkNotMirrored(checked); });
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::CommitFormula(FormulaField field)
{
    if (m_loading || m_piece == nullptr)
    {
        return;
    }

    QLineEdit *edit = EditFor(field);

    VPieceNode node;
    if (!FetchCurrentNode(node))
    {
        return;
    }

    // Accept both decimal separators (Ostara rule §3.1) — normalise ',' to '.' before evaluating,
    // and keep the canonical form in the model/field.
    QString text = edit->text().trimmed();
    text.replace(QLatin1Char(','), QLatin1Char('.'));

    if (text.isEmpty())
    {
        // An empty field is not a formula that evaluates to zero: it drops the override and hands the
        // dimension back to the engine. This is the only way back to the automatic value.
        PushNodeChange([field](VPieceNode &n) { SetOverride(n, field, QString()); });
        // Reload the two other rows first — clearing a length moves the automatic width — and only
        // then force this one, which the reload deliberately steps over while it holds the focus.
        ReloadAfterCommit();
        RestoreField(field);
        return;
    }

    if (text == StoredFormula(node, field))
    {
        return; // nothing changed
    }

    // Evaluate the raw formula. Invalid input (or an unparsable expression, e.g. a stray character)
    // reverts the field; the parser throws and must never propagate out of the dock.
    Calculator cal;
    qreal raw = 0;
    try
    {
        raw = cal.EvalFormula(m_piece->getData()->DataVariables(), text);
    }
    catch (...)
    {
        RestoreField(field);
        return;
    }
    if (qIsNaN(raw) || qIsInf(raw))
    {
        RestoreField(field);
        return;
    }

    // Refuse here what the engine would refuse later, in silence: a length of zero or below is an
    // error state ("Cannot calculate passmark length"), and so is a width of zero. A negative width
    // is refused too, except on a "Check mark" where the sign is what tells the two sides apart.
    // The angle carries no guard at all — zero and negatives are legitimate there.
    if (field == FormulaField::Width)
    {
        const bool checkMark = node.IsPassmark() && node.GetPassmarkLineType() == PassmarkLineType::CheckMark;
        if (qFuzzyIsNull(raw) || (raw < 0 && !checkMark))
        {
            RestoreField(field);
            return;
        }
    }
    else if (field == FormulaField::Length && raw <= 0)
    {
        RestoreField(field);
        return;
    }

    edit->setText(text);
    PushNodeChange([field, text](VPieceNode &n) { SetOverride(n, field, text); });
    ReloadAfterCommit();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsNodeWidget::StoredFormula(const VPieceNode &node, FormulaField field) -> QString
{
    switch (field)
    {
        case FormulaField::Length:
            return node.GetFormulaPassmarkLength();
        case FormulaField::Width:
            return node.GetFormulaPassmarkWidth();
        case FormulaField::Angle:
            return node.GetFormulaPassmarkAngle();
        case FormulaField::Visibility:
        default:
            return node.GetFormulaPassmarkVisibility();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::SetOverride(VPieceNode &node, FormulaField field, const QString &formula)
{
    // The manual flag is derived from the formula — there is a value or there is not, nothing else.
    // A dimension with a formula but no flag would be an error state for the engine, and a flag with
    // no formula is one already.
    const bool manual = !formula.isEmpty();

    switch (field)
    {
        case FormulaField::Length:
            node.SetManualPassmarkLength(manual);
            node.SetFormulaPassmarkLength(formula);
            break;
        case FormulaField::Width:
            node.SetManualPassmarkWidth(manual);
            node.SetFormulaPassmarkWidth(formula);
            break;
        case FormulaField::Angle:
            node.SetManualPassmarkAngle(manual);
            node.SetFormulaPassmarkAngle(formula);
            break;
        case FormulaField::Visibility:
        default:
            node.SetFormulaPassmarkVisibility(formula);
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::CommitSeamAllowanceAngle(int index)
{
    if (m_loading)
    {
        return;
    }

    const auto type = static_cast<PieceNodeAngle>(m_seamTypeCombo->itemData(index).toInt());
    VPieceNode node;
    if (!FetchCurrentNode(node))
    {
        return;
    }
    if (node.GetAngleType() == type)
    {
        return;
    }
    PushNodeChange([type](VPieceNode &n) { n.SetAngleType(type); });
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::OpenFormulaEditor(FormulaField field, QLineEdit *edit)
{
    if (m_piece == nullptr)
    {
        return;
    }

    QScopedPointer<DialogEditWrongFormula> const dialog(
        new DialogEditWrongFormula(m_piece->getData(), m_nodeId, this));
    dialog->SetFormula(edit->text());

    if (dialog->exec() == QDialog::Accepted)
    {
        edit->setText(dialog->GetFormula());
        CommitFormula(field);
    }
}
