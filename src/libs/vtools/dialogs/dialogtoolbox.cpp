/************************************************************************
 **
 **  @file   dialogtoolbox.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   25 1, 2019
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2019 Valentina project
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
#include "dialogtoolbox.h"

#include "../ifc/exception/vexceptionbadid.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../qmuparser/qmudef.h"
#include "../tools/drawTools/operation/vabstractoperation.h"
#include "../vgeometry/varc.h"
#include "../vgeometry/vcubicbezier.h"
#include "../vgeometry/vcubicbezierpath.h"
#include "../vgeometry/vellipticalarc.h"
#include "../vgeometry/vpointf.h"
#include "../vgeometry/vspline.h"
#include "../vgeometry/vsplinepath.h"
#include "../vmisc/def.h"
#include "../vmisc/FactoryWidgets.h"
#include "themeDef.h"
#include "../vtheme/vscenestylesheet.h"
#include "../vtheme/vtheme.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/calculator.h"
#include "../vpatterndb/variables/vcurvelength.h"
#include "../vpatterndb/variables/vcurvevariable.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vformula.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vpatterndb/vtranslatevars.h"
#include "../undocommands/vchangelinerolename.h"
#include "../vgeometry/vabstractcurve.h"
#include "../vpropertyexplorer/qtcolorpicker.h"
#include "../tools/drawTools/toolcurve/vtoolabstractcurve.h"
#include "../vtools/tools/vabstracttool.h"
#include "support/dialognameobject.h"
#include "../tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcut.h"

#include <qnumeric.h>
#include <QBuffer>
#include <QCompleter>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QUndoStack>
#include <QEvent>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QPainter>
#include <QPixmap>
#include <QStyle>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScreen>
#include <QTextCursor>
#include <QTimer>

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

const QColor errorColor = Qt::red;

namespace
{
const int dialogMaxFormulaHeight = 80;

//---------------------------------------------------------------------------------------------------------------------
auto DoublePoint(const VPieceNode &firstNode, const VPieceNode &secondNode, const VContainer *data, QString &error)
    -> bool
{
    if (firstNode.GetTypeTool() == Tool::NodePoint && not(firstNode.GetId() == NULL_ID) &&
        secondNode.GetTypeTool() == Tool::NodePoint && not(secondNode.GetId() == NULL_ID))
    {
        QSharedPointer<VPointF> firstPoint;
        QSharedPointer<VPointF> secondPoint;

        try
        {
            firstPoint = data->GeometricObject<VPointF>(firstNode.GetId());
            secondPoint = data->GeometricObject<VPointF>(secondNode.GetId());
        }
        catch (const VExceptionBadId &)
        {
            return false;
        }

        // don't ignore the same point twice
        if (firstNode.GetId() == secondNode.GetId())
        {
            error = QObject::tr("Point '%1' repeats twice").arg(firstPoint->name());
            return true;
        }

        // The same point, but different modeling objects
        if (firstPoint->getIdObject() != NULL_ID && secondPoint->getIdObject() != NULL_ID &&
            firstPoint->getMode() == Draw::Modeling && secondPoint->getMode() == Draw::Modeling &&
            firstPoint->getIdObject() == secondPoint->getIdObject())
        {
            error = QObject::tr("Point '%1' repeats twice").arg(firstPoint->name());
            return true;
        }

        // But ignore the same coordinate if a user wants
        if (not firstNode.IsCheckUniqueness() || not secondNode.IsCheckUniqueness())
        {
            return false;
        }

        bool const sameCoordinates = VFuzzyComparePoints(firstPoint->toQPointF(), secondPoint->toQPointF());
        if (sameCoordinates)
        {
            error = QObject::tr("Points '%1' and '%2' have the same coordinates.")
                        .arg(firstPoint->name(), secondPoint->name());
        }

        return sameCoordinates;
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto DoubleCurve(const VPieceNode &firstNode, const VPieceNode &secondNode, const VContainer *data, QString &error)
    -> bool
{
    if (firstNode.GetTypeTool() != Tool::NodePoint && not(firstNode.GetId() == NULL_ID) &&
        secondNode.GetTypeTool() != Tool::NodePoint && not(secondNode.GetId() == NULL_ID))
    {
        QSharedPointer<VGObject> curve1;
        QSharedPointer<VGObject> curve2;

        try
        {
            curve1 = data->GetGObject(firstNode.GetId());
            curve2 = data->GetGObject(secondNode.GetId());
        }
        catch (const VExceptionBadId &)
        {
            return false;
        }

        // don't ignore the same curve twice
        if (firstNode.GetId() == secondNode.GetId())
        {
            error = QObject::tr("Leave only one copy of curve '%1'").arg(curve1->name());
            return true;
        }

        // The same curve, but different modeling objects
        if (curve1->getMode() == Draw::Modeling && curve2->getMode() == Draw::Modeling &&
            curve1->getIdObject() != NULL_ID && curve2->getIdObject() != NULL_ID &&
            curve1->getIdObject() == curve2->getIdObject())
        {
            error = QObject::tr("Leave only one copy of curve '%1'").arg(curve1->name());
            return true;
        }
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> auto CurveAliases(const QString &alias1, const QString &alias2) -> QPair<QString, QString>
{
    T curve1;
    curve1.SetAliasSuffix(alias1);

    T curve2;
    curve2.SetAliasSuffix(alias2);

    return qMakePair(curve1.GetAlias(), curve2.GetAlias());
}

//---------------------------------------------------------------------------------------------------------------------
template<class T>
auto CurveNames(const QString &name1, const QString &name2) -> QPair<QString, QString>
{
    T curve1;
    curve1.SetNameSuffix(name1);

    T curve2;
    curve2.SetNameSuffix(name2);

    return qMakePair(curve1.name(), curve2.name());
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto RowNode(QListWidget *listWidget, int i) -> VPieceNode
{
    SCASSERT(listWidget != nullptr);

    if (i < 0 || i >= listWidget->count())
    {
        return {};
    }

    const QListWidgetItem *rowItem = listWidget->item(i);
    SCASSERT(rowItem != nullptr)
    return qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));
}

//---------------------------------------------------------------------------------------------------------------------
void MoveCursorToEnd(QPlainTextEdit *plainTextEdit)
{
    SCASSERT(plainTextEdit != nullptr)
    QTextCursor cursor = plainTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
    plainTextEdit->setTextCursor(cursor);
}

//---------------------------------------------------------------------------------------------------------------------
void DeployFormula(QDialog *dialog, QPlainTextEdit *formula, QPushButton *buttonGrowLength, int formulaBaseHeight)
{
    // cppcheck-suppress unknownMacro
    SCASSERT(dialog != nullptr)
    SCASSERT(formula != nullptr)
    SCASSERT(buttonGrowLength != nullptr)

    const QTextCursor cursor = formula->textCursor();

    // Release the vertical constraints so the dialog can grow with the formula field,
    // while keeping the minimum width (575 px set in DialogTool) intact.
    dialog->setMaximumHeight(QWIDGETSIZE_MAX);
    dialog->setMinimumHeight(0);

    if (formula->height() < dialogMaxFormulaHeight)
    {
        formula->setFixedHeight(dialogMaxFormulaHeight);
        // Set icon from theme (internal for Windows system)
        buttonGrowLength->setIcon(FromTheme(VThemeIcon::GoNext));
    }
    else
    {
        formula->setFixedHeight(formulaBaseHeight);
        // Set icon from theme (internal for Windows system)
        buttonGrowLength->setIcon(FromTheme(VThemeIcon::GoDown));
    }

    // I found that after change size of formula field, it was filed for angle formula, field for formula became black.
    // This code prevent this.
    dialog->setUpdatesEnabled(false);
    dialog->repaint();
    dialog->setUpdatesEnabled(true);

    formula->setFocus();
    formula->setTextCursor(cursor);
}

//---------------------------------------------------------------------------------------------------------------------
namespace
{
// Gray200 — the standard colour of the Dark icon set (Icons.md §2, palette.json).
const QColor darkIconColor = QColor(0xE5, 0xE5, 0xE5);

//---------------------------------------------------------------------------------------------------------------------
// Repaint an icon in the Dark set colour, keeping its shape: the SVG icons are monochrome, so painting the
// pixmap with CompositionMode_SourceIn gives exactly the dark variant of the icon, at any size.
auto DarkIconVariant(const QIcon &icon, const QSize &size, qreal devicePixelRatio) -> QIcon
{
    QPixmap pixmap = icon.pixmap(size, devicePixelRatio);
    if (pixmap.isNull())
    {
        return icon;
    }

    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), darkIconColor);
    painter.end();

    return QIcon(pixmap);
}

//---------------------------------------------------------------------------------------------------------------------
// The caret of a formula field must stay readable on the accent hover background (see InitFormulaGrowButton).
// Resting icon and hover icon are both kept: the dialog may replace the resting one at any time (deploying or
// collapsing the field), and the shape must follow.
class FormulaGrowButtonFilter : public QObject
{
public:
    explicit FormulaGrowButtonFilter(QPushButton *button)
      : QObject(button),
        m_button(button)
    {
    }

protected:
    auto eventFilter(QObject *object, QEvent *event) -> bool override
    {
        switch (event->type())
        {
            case QEvent::Enter:
                Darken();
                break;
            case QEvent::Leave:
            case QEvent::Hide: // hiding the dialog may skip the Leave event
                Restore();
                break;
            case QEvent::MouseButtonRelease:
                // A click deploys or collapses the field, so the dialog sets a new icon while the button is
                // still hovered. Redo the swap once the button has finished handling the click.
                QTimer::singleShot(0, m_button, [this]() { Refresh(); });
                break;
            default:
                break;
        }

        return QObject::eventFilter(object, event);
    }

private:
    void Darken()
    {
        if (m_hovered)
        {
            return;
        }

        m_hovered = true;
        m_restIcon = m_button->icon();
        m_darkIcon = DarkIconVariant(m_restIcon, m_button->iconSize(), m_button->devicePixelRatioF());
        m_button->setIcon(m_darkIcon);
    }

    void Restore()
    {
        if (!m_hovered)
        {
            return;
        }

        m_hovered = false;
        m_button->setIcon(m_restIcon);
    }

    void Refresh()
    {
        if (!m_hovered)
        {
            return;
        }

        if (m_button->icon().cacheKey() != m_darkIcon.cacheKey())
        {
            // The dialog replaced the icon (field deployed or collapsed): it becomes the new resting icon.
            m_restIcon = m_button->icon();
            m_darkIcon = DarkIconVariant(m_restIcon, m_button->iconSize(), m_button->devicePixelRatioF());
        }

        if (m_button->underMouse())
        {
            m_button->setIcon(m_darkIcon);
        }
        else
        {
            Restore();
        }
    }

    QPushButton *m_button;
    bool m_hovered{false};
    QIcon m_restIcon;
    QIcon m_darkIcon;
};
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void InitFormulaGrowButton(QPushButton *button)
{
    if (button == nullptr || button->property("formulaGrowHover").toBool())
    {
        return;
    }

    button->setProperty("formulaGrowHover", true);
    button->installEventFilter(new FormulaGrowButtonFilter(button)); // owned by the button
}

//---------------------------------------------------------------------------------------------------------------------
auto FilterObject(QObject *object, QEvent *event) -> bool
{
    if (auto *plainTextEdit = qobject_cast<QPlainTextEdit *>(object))
    {
        if (event->type() == QEvent::KeyPress)
        {
            if (const auto *keyEvent = static_cast<QKeyEvent *>(event);
                (keyEvent->key() == Qt::Key_Period) && (keyEvent->modifiers() & Qt::KeypadModifier))
            {
                if (VAbstractApplication::VApp()->Settings()->GetOsSeparator())
                {
                    plainTextEdit->insertPlainText(LocaleDecimalPoint(QLocale()));
                }
                else
                {
                    plainTextEdit->insertPlainText(LocaleDecimalPoint(QLocale::c()));
                }
                return true;
            }
        }
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Bring a formula typed in a field back to the internal notation.
 *
 * The global ODecimalSeparatorFilter already turns a typed decimal comma into a dot, so a field holds internal
 * notation whatever the "decimal separator" setting says; a pasted comma is normalised here the same way. A
 * comma is never legitimate in a formula — arguments are separated by ';' — so the substitution is safe.
 *
 * @param formula Text of the formula field, in the notation the user reads.
 * @return The formula in internal notation, its variables and functions still in the user language.
 */
auto ToInternalNotation(const QString &formula) -> QString
{
    QString text = formula;
    text.replace(QLatin1Char(','), QLatin1Char('.'));
    return text;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief The reason shown when a formula names an object the user never named.
 *
 * A line or a curve of Ostara answers to its points — `Line_C7_C8`, `Spl_A_D` — so a formula can be written
 * without anyone naming anything, and it computes. What it cannot do is mean something: the name travels with
 * the points, and it says nothing about the object, so it is refused and the user is asked to name it.
 *
 * The object is deliberately NOT named here. Ostara's point is that its user stops reading and writing point
 * names, so putting `C7–C8` in a refusal would hand back the very habit the rule exists to break — and it
 * would name nothing at all to someone who does not think in points. `@a token` and `@a var` stay in the
 * signature so a caller may still tell the families apart, and so the message can gain an object name the day
 * objects stopped being named after points.
 */
auto UnnamedObjectMessage(const QString &token, const VContainer *container,
                          const QHash<QString, QSharedPointer<VInternalVariable>> *variables) -> QString
{
    Q_UNUSED(token)
    Q_UNUSED(container)
    Q_UNUSED(variables)

    return QObject::tr("This formula uses an object that has no name. Give it one to use it in a formula.");
}

//---------------------------------------------------------------------------------------------------------------------
auto InternalFormula(const QString &formula, const VContainer *data) -> QString
{
    if (formula.isEmpty())
    {
        return formula;
    }

    const VTranslateVars *const trVars = VAbstractApplication::VApp()->TrVars();
    if (trVars == nullptr)
    {
        return formula;
    }

    // First pass: the whole formula, with its functions and its decimal separator. It is the one that gives up in
    // silence — `Try` — because a formula nobody can read is the calculator's business, not this function's.
    //
    // `false`, and not the separator of the system: what is read here is the calculator's own notation, whose
    // decimal separator is a dot — `ToInternalNotation` has just put one there. Asking for commas makes the reader
    // stumble on the very dots it was handed, and the whole conversion is then abandoned without a word.
    const QString converted = VTranslateVars::TryFormulaFromUser(ToInternalNotation(formula), false);

    // Second pass: name by name, for the names that pass left as they were. A name that is already the
    // calculator's own comes back unchanged, so such a formula is not touched a second time.
    const QString translated
        = VFormula::MapTokens(converted, [trVars](const QString &token) -> QString
                              { return trVars->InternalVarFromUser(token); });

    const QHash<QString, QSharedPointer<VInternalVariable>> *const vars
        = data == nullptr ? nullptr : data->DataVariables();
    if (vars == nullptr)
    {
        return translated;
    }

    // Third pass, for what the dictionary left alone: a name written the way it is *read* is still a name the
    // pattern answers to. The direction that displays every name is asked what the token is — the same direction
    // the formula field itself is filled with — so a name is understood wherever it can be shown, whether the
    // reverse of the dictionary answers or not.
    return VFormula::MapTokens(translated,
                               [trVars, vars](const QString &token) -> QString
                               {
                                   if (vars->contains(token))
                                   {
                                       return token; // Already the calculator's own.
                                   }

                                   for (auto i = vars->constBegin(); i != vars->constEnd(); ++i)
                                   {
                                       if (trVars->VarToUser(i.key()) == token)
                                       {
                                           return i.key();
                                       }
                                   }

                                   return token;
                               });
}

//---------------------------------------------------------------------------------------------------------------------
auto NameCurveObject(VAbstractPattern *doc, const VContainer *data, quint32 curveId, const QString &name) -> bool
{
    if (doc == nullptr || data == nullptr || name.isEmpty())
    {
        return false;
    }

    // L'outil de la courbe d'abord : c'est lui qui écrit l'alias, vérifie qu'il est libre et pousse la commande
    // qui répare les formules du fichier.
    if (auto *tool = qobject_cast<VToolAbstractCurve *>(VAbstractPattern::FindTool(curveId)); tool != nullptr)
    {
        tool->SetAliasSuffix(name);
        return true;
    }

    // Un segment de coupe n'a pas d'outil à lui : il appartient à l'outil du **point** qui le déclare, et l'index
    // du segment est celui de l'alias — `segment2Id` se nomme par `SetAliasSuffix2`. Même lecture que
    // `CollectDeclaredIds`, qui attribue une dépendance par ces quatre attributs.
    const QDomNodeList points = doc->documentElement().elementsByTagName(QStringLiteral("point"));
    for (int i = 0; i < points.count(); ++i)
    {
        const QDomElement point = points.at(i).toElement();
        if (point.isNull())
        {
            continue;
        }

        for (int segment = 1; segment <= 2; ++segment)
        {
            const QString attribute = QStringLiteral("segment%1Id").arg(segment);
            if (point.attribute(attribute).toUInt() != curveId)
            {
                continue;
            }

            auto *tool = qobject_cast<VToolCut *>(VAbstractPattern::FindTool(point.attribute(VAbstractPattern::AttrId).toUInt()));
            if (tool == nullptr)
            {
                continue;
            }

            if (segment == 1)
            {
                tool->SetAliasSuffix1(name);
            }
            else
            {
                tool->SetAliasSuffix2(name);
            }
            return true;
        }
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto UnnamedObjects(const VContainer *data, const QString &formula) -> QList<DialogNameObject::Object>
{
    QList<DialogNameObject::Object> objects;
    if (data == nullptr || formula.isEmpty())
    {
        return objects;
    }

    const QHash<QString, QSharedPointer<VInternalVariable>> *const vars = data->DataVariables();
    if (vars == nullptr)
    {
        return objects;
    }

    const VTranslateVars *const trVars = VAbstractApplication::VApp()->TrVars();
    QStringList collected; // the objects already asked about, by the pair of a line or the tool of a curve

    // Token by token, with the calculator's own reading of the formula: a name is only ever read as a whole token,
    // never as a piece of text, so a line named `A_B` can never reach into one named `A_B2`. And a name repeated
    // five times in a three-way formula is one object, hence one question.
    VFormula::MapTokens(formula,
                        [&](const QString &token) -> QString
                        {
                            const QSharedPointer<VInternalVariable> var = vars->value(token);
                            if (var.isNull() || not VContainer::IsUnnamedObject(data, var))
                            {
                                return token;
                            }

                            const VarType type = var->GetType();
                            if (type == VarType::LineLength || type == VarType::LineAngle)
                            {
                                // A line answers to the pair of points it joins: the length and the angle of one
                                // pair carry that one name, which is what the user gives here. The dialogue shows
                                // the token as written — the length or the angle — and the field holds the pair
                                // alone, never the prefix.
                                const QString &prefix = type == VarType::LineLength ? line_ : angleLine_;
                                const QString pairKey = var->GetName().mid(prefix.length());
                                if (not collected.contains(pairKey))
                                {
                                    collected << pairKey;
                                    objects << DialogNameObject::Object{
                                        .shown = trVars->VarToUser(SimplifiedName(token)),
                                        .suffix = pairKey,
                                        .pairKey = pairKey};
                                }
                            }
                            else
                            {
                                // A curve, and everything read on it, is named once: the name belongs to the curve,
                                // and its angles, handles, radii and the segments of a path follow it. So the route
                                // — the points it is built on while nobody has named it — is what the user replaces.
                                const QSharedPointer<VCurveVariable> curveVar
                                    = qSharedPointerDynamicCast<VCurveVariable>(var);
                                const QSharedPointer<VAbstractCurve> curve
                                    = curveVar.isNull()
                                          ? QSharedPointer<VAbstractCurve>()
                                          : qSharedPointerDynamicCast<VAbstractCurve>(
                                                data->GetGObject(curveVar->GetId()));
                                const QString handle = curveVar.isNull() ? QString() : QString::number(curveVar->GetId());
                                if (not curve.isNull() && not collected.contains(handle))
                                {
                                    collected << handle;
                                    objects << DialogNameObject::Object{
                                        .shown = trVars->VarToUser(SimplifiedName(token)),
                                        .suffix = curve->HeadlessName(),
                                        .curveId = curveVar->GetId(),
                                        .typeHead = curve->GetTypeHead()};
                                }
                            }

                            return token;
                        });

    return objects;
}

//---------------------------------------------------------------------------------------------------------------------
auto NameUnnamedObjects(QWidget *parent, const VContainer *data, QString &formula) -> bool
{
    
    if (data == nullptr || formula.isEmpty())
    {
        return true;
    }

    VAbstractPattern *const doc = VAbstractValApplication::VApp()->getCurrentDocument();
    if (doc == nullptr)
    {
        return true; // Nothing can be written, so there is nothing to ask for either.
    }

    const QList<DialogNameObject::Object> objects = UnnamedObjects(data, formula);

    if (objects.isEmpty())
    {
        return true; // Nothing is unnamed: every object the formula uses already answers to a name.
    }

    
    DialogNameObject dialogue(objects, data, parent);
    if (dialogue.exec() != QDialog::Accepted)
    {
        return false; // Cancelled: not one name was given, not one formula was touched.
    }

    const QStringList names = dialogue.Names();

    // One operation: naming the objects of one formula is one intent, and one undo takes it back as a whole.
    QUndoStack *const undoStack = VAbstractApplication::VApp()->getUndoStack();
    undoStack->beginMacro(QObject::tr("name the objects of a formula"));
    for (int i = 0; i < objects.size(); ++i)
    {
        const DialogNameObject::Object &object = objects.at(i);
        if (names.at(i).isEmpty())
        {
            continue;
        }

        if (object.pairKey.isEmpty())
        {
            // L'outil écrit l'alias, vérifie qu'il est libre et pousse sa commande, qui répare les formules du
            // fichier — et pour un segment de coupe, c'est l'outil du point qui le déclare qui s'en charge.
            NameCurveObject(doc, data, object.curveId, names.at(i));
        }
        else
        {
            auto *command
                = new VChangeLineRoleName(doc, object.pairKey, data->LineRoleName(object.pairKey), names.at(i));
            QObject::connect(command, &VChangeLineRoleName::NeedLiteParsing, doc, &VAbstractPattern::LiteParseTree);
            undoStack->push(command);
        }
    }
    undoStack->endMacro();

    // The names are in the pattern, and every formula of the file has been repaired with them. This one is not in
    // the file — it is being edited — so it is repaired here, with the same rule: the object's name, nothing else.
    formula = VContainer::FormulaWithoutStaleHandles(data, formula);
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief EvalToolFormula evaluate formula and show result
 * @param dialog dialog
 * @param data formula data
 * @param flag formula valid
 * @return result of calculation
 */
auto EvalToolFormula(QDialog *dialog, const FormulaData &data, bool &flag) -> qreal
{
    SCASSERT(data.labelResult != nullptr)

    qreal result = INT_MIN; // Value can be 0, so use max imposible value

    auto ShowErrorResult = [&data](const QString &reason)
    {
        QString message = QObject::tr("Error");
        if (!data.postfix.isEmpty())
        {
            message += " ("_L1 + data.postfix + ")"_L1;
        }
        data.labelResult->setText(message);
        data.labelResult->setToolTip(reason);
    };

    // The one reason that is an instruction rather than a diagnosis is written where it can be read. "Error"
    // plus a tooltip is enough for a formula that does not parse — the user knows what a syntax error is — but
    // not for a rule they are meeting for the first time: the whole point of refusing a name is to have them
    // name the object, and an instruction nobody reads changes nothing.
    auto ShowInstruction = [&data](const QString &reason)
    {
        data.labelResult->setText(reason);
        data.labelResult->setToolTip(reason);
    };

    // The label beside the field names what the field holds, and a name is not in error — so a caller whose
    // label is a plain caption passes none, and the colour then falls to the value alone, which carries both
    // the reason and the state. Every other caller keeps the behaviour it had.
    auto MarkState = [&data](const QColor &color)
    {
        if (data.labelEditFormula != nullptr)
        {
            ChangeColor(data.labelEditFormula, color);
        }
    };

    if (data.formula.isEmpty())
    {
        flag = false;
        MarkState(errorColor);
        ShowErrorResult(QObject::tr("Empty formula"));
    }
    else
    {
        try
        {
            // Translate to internal look. The field already holds internal numbers (see ToInternalNotation),
            // so the internal separator is the one to parse with: with the OS separator the parser would
            // refuse the very dot the global filter puts in.
            //
            // A formula written before the object was named — or one waiting in a field while the name was given
            // elsewhere — is not refused for it: what it calls the object still resolves, and the name the object
            // has now is known. It is put right, and what is judged is the formula the user meant.
            QString const formula = InternalFormula(data.formula, data.container);
            QScopedPointer<Calculator> const cal(new Calculator());
            result = cal->EvalFormula(data.variables, formula);

            // A name the user never gave is read as a value — the pattern defines it — and computing it here
            // would show a result for a formula that can never be kept. It is refused before the value is
            // believed, and the reason says which object to name.
            const QString unnamed = VContainer::UnnamedObjectInFormula(data.container, formula);
            if (not unnamed.isEmpty())
            {
                flag = false;
                MarkState(errorColor);
                ShowInstruction(UnnamedObjectMessage(unnamed, data.container, data.variables));
            }
            else if (qIsInf(result) || qIsNaN(result))
            {
                flag = false;
                MarkState(errorColor);
                ShowErrorResult(
                    QObject::tr("Invalid result. Value is infinite or NaN. Please, check your calculations."));
            }
            else
            {
                // if result equal 0
                if (data.checkZero && qFuzzyIsNull(result))
                {
                    flag = false;
                    MarkState(errorColor);
                    ShowErrorResult(QObject::tr("Value can't be 0"));
                }
                else if (data.checkLessThanZero && result < 0)
                {
                    flag = false;
                    MarkState(errorColor);
                    ShowErrorResult(QObject::tr("Value can't be less than 0"));
                }
                else
                {
                    QString message = VAbstractApplication::VApp()->LocaleToString(result);
                    if (!data.postfix.isEmpty())
                    {
                        message += QChar(QChar::Space) + data.postfix;
                    }
                    data.labelResult->setText(message);
                    flag = true;
                    MarkState(OkColor(dialog));
                    data.labelResult->setToolTip(QObject::tr("Value"));
                }
            }
        }
        catch (qmu::QmuParserError &e)
        {
            flag = false;
            MarkState(errorColor);
            ShowErrorResult(QObject::tr("Parser error: %1").arg(e.GetMsg()));
            qDebug() << "\nMath parser error:\n"
                     << "--------------------------------------\n"
                     << "Message:     " << e.GetMsg() << "\n"
                     << "Expression:  " << e.GetExpr() << "\n"
                     << "--------------------------------------";
        }
    }
    return result;
}

//---------------------------------------------------------------------------------------------------------------------
void ChangeColor(QWidget *widget, const QColor &color)
{
    SCASSERT(widget != nullptr)
    QPalette palette = widget->palette();
    palette.setColor(QPalette::Active, widget->foregroundRole(), color);
    palette.setColor(QPalette::Inactive, widget->foregroundRole(), color);
    widget->setPalette(palette);
}

//---------------------------------------------------------------------------------------------------------------------
auto OkColor(const QWidget *widget) -> QColor
{
    SCASSERT(widget != nullptr);
    return widget->palette().color(QPalette::Active, QPalette::WindowText);
}

//---------------------------------------------------------------------------------------------------------------------
auto GetColorDotPixmap(const QColor &color, int diameter) -> QPixmap
{
    QPixmap pix(diameter, diameter);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(1, 1, diameter - 2, diameter - 2);
    painter.end();
    return pix;
}

//---------------------------------------------------------------------------------------------------------------------
void CheckPointLabel(QDialog *dialog, QLineEdit *edit, QLabel *labelEditNamePoint, const QString &pointName,
                     const VContainer *data, bool &flag)
{
    SCASSERT(dialog != nullptr)
    SCASSERT(edit != nullptr)
    SCASSERT(labelEditNamePoint != nullptr)

    const QString name = edit->text();
    if (QRegularExpression const rx(NameRegExp());
        name.isEmpty() || (pointName != name && not data->IsUnique(name)) || not rx.match(name).hasMatch())
    {
        flag = false;
        edit->setStyleSheet(QStringLiteral("border: 1px solid red;"));
        ChangeColor(labelEditNamePoint, errorColor);
    }
    else
    {
        flag = true;
        edit->setStyleSheet({});
        ChangeColor(labelEditNamePoint, OkColor(dialog));
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto AddShowPointNameCheckbox(QFormLayout *layout) -> QCheckBox *
{
    auto *checkBox = new QCheckBox(QCoreApplication::translate("DialogTool", "Show Point name"));
    layout->addRow(checkBox);
    return checkBox;
}

//---------------------------------------------------------------------------------------------------------------------
auto FindNotExcludedNeighborNodeDown(QListWidget *listWidget, int candidate) -> int
{
    SCASSERT(listWidget != nullptr);

    int index = -1;
    if (candidate < 0 || candidate >= listWidget->count())
    {
        return index;
    }

    int i = candidate;
    VPieceNode rowNode;
    bool foundNeighbor = false;

    do
    {
        const QListWidgetItem *rowItem = listWidget->item(i);
        SCASSERT(rowItem != nullptr)
        rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));

        if (not rowNode.IsExcluded())
        {
            index = i;
            foundNeighbor = true;
            break;
        }

        ++i;
        if (i >= listWidget->count())
        {
            // Wrap around to the beginning of the list
            i = 0;
        }

    } while (i != candidate); // Continue until we reach back to the starting point or find a neighbor

    if (!foundNeighbor)
    {
        // No not excluded neighbor found
        index = -1;
    }

    return index;
}

//---------------------------------------------------------------------------------------------------------------------
auto FindNotExcludedNeighborNodeUp(QListWidget *listWidget, int candidate) -> int
{
    SCASSERT(listWidget != nullptr);

    int index = -1;
    if (candidate < 0 || candidate >= listWidget->count())
    {
        return index;
    }

    int i = candidate;
    VPieceNode rowNode;
    bool foundNeighbor = false;

    do
    {
        const QListWidgetItem *rowItem = listWidget->item(i);
        SCASSERT(rowItem != nullptr)
        rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));

        if (not rowNode.IsExcluded())
        {
            index = i;
            foundNeighbor = true;
            break;
        }

        --i;
        if (i < 0)
        {
            // Wrap around to the end of the list
            i = listWidget->count() - 1;
        }

    } while (i != candidate);

    if (!foundNeighbor)
    {
        // No not excluded neighbor found
        index = -1;
    }

    return index;
}

//---------------------------------------------------------------------------------------------------------------------
auto FindNotExcludedNodeDown(QListWidget *listWidget, int candidate) -> int
{
    SCASSERT(listWidget != nullptr);

    int index = -1;
    if (candidate < 0 || candidate >= listWidget->count())
    {
        return index;
    }

    int i = candidate;
    VPieceNode rowNode;
    do
    {
        const QListWidgetItem *rowItem = listWidget->item(i);
        SCASSERT(rowItem != nullptr)
        rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));

        if (not rowNode.IsExcluded())
        {
            index = i;
        }

        ++i;
    } while (rowNode.IsExcluded() && i < listWidget->count());

    return index;
}

//---------------------------------------------------------------------------------------------------------------------
auto FindNotExcludedNodeUp(QListWidget *listWidget, int candidate) -> int
{
    SCASSERT(listWidget != nullptr);

    int index = -1;
    if (candidate < 0 || candidate >= listWidget->count())
    {
        return index;
    }

    int i = candidate;
    VPieceNode rowNode;
    do
    {
        const QListWidgetItem *rowItem = listWidget->item(i);
        SCASSERT(rowItem != nullptr)
        rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));

        if (not rowNode.IsExcluded())
        {
            index = i;
        }

        --i;
    } while (rowNode.IsExcluded() && i > -1);

    return index;
}

//---------------------------------------------------------------------------------------------------------------------
auto FirstPointEqualLast(QListWidget *listWidget, const VContainer *data, QString &error) -> bool
{
    SCASSERT(listWidget != nullptr);
    if (listWidget->count() > 1)
    {
        const VPieceNode topNode = RowNode(listWidget, FindNotExcludedNodeDown(listWidget, 0));
        const VPieceNode bottomNode = RowNode(listWidget, FindNotExcludedNodeUp(listWidget, listWidget->count() - 1));

        return DoublePoint(topNode, bottomNode, data, error);
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto DoublePoints(QListWidget *listWidget, const VContainer *data, QString &error) -> bool
{
    SCASSERT(listWidget != nullptr);
    for (int i = 0, sz = listWidget->count() - 1; i < sz; ++i)
    {
        const int firstIndex = FindNotExcludedNodeDown(listWidget, i);
        const VPieceNode firstNode = RowNode(listWidget, firstIndex);
        const VPieceNode secondNode = RowNode(listWidget, FindNotExcludedNodeDown(listWidget, firstIndex + 1));

        if (DoublePoint(firstNode, secondNode, data, error))
        {
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto DoubleCurves(QListWidget *listWidget, const VContainer *data, QString &error) -> bool
{
    SCASSERT(listWidget != nullptr);
    for (int i = 0, sz = listWidget->count() - 1; i < sz; ++i)
    {
        const int firstIndex = FindNotExcludedNodeDown(listWidget, i);
        const VPieceNode firstNode = RowNode(listWidget, firstIndex);
        const VPieceNode secondNode = RowNode(listWidget, FindNotExcludedNodeDown(listWidget, firstIndex + 1));

        if (DoubleCurve(firstNode, secondNode, data, error))
        {
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto EachPointLabelIsUnique(QListWidget *listWidget) -> bool
{
    SCASSERT(listWidget != nullptr);
    QSet<quint32> pointLabels;
    int countPoints = 0;
    for (int i = 0; i < listWidget->count(); ++i)
    {
        const QListWidgetItem *rowItem = listWidget->item(i);
        SCASSERT(rowItem != nullptr)
        const auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));
        if (rowNode.GetTypeTool() == Tool::NodePoint && not rowNode.IsExcluded())
        {
            ++countPoints;
            pointLabels.insert(rowNode.GetId());
        }
    }

    return countPoints == pointLabels.size();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogWarningIcon(const QWidget *w) -> QString
{
    const QIcon icon = FromTheme(VThemeIcon::DialogWarning);

    qreal scaleFactor = 1;
    if (w != nullptr)
    {
        if (const QScreen *wScreen = w->screen())
        {
            scaleFactor = wScreen->devicePixelRatio();
        }
    }

    const QPixmap pixmap = icon.pixmap(QSize(qRound(16 / scaleFactor), qRound(16 / scaleFactor)));
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    pixmap.save(&buffer, "PNG");
    return QStringLiteral("<img src=\"data:image/png;base64,") + byteArray.toBase64() + "\"/> "_L1;
}

//---------------------------------------------------------------------------------------------------------------------
auto NodeFont(QFont font, bool nodeExcluded) -> QFont
{
    font.setPointSize(12);
    font.setWeight(QFont::Bold);
    font.setStrikeOut(nodeExcluded);
    return font;
}

//---------------------------------------------------------------------------------------------------------------------
void CurrentCurveLength(vidtype curveId, VContainer *data)
{
    SCASSERT(data != nullptr)
    VCurveLength *length = nullptr;
    try
    {
        const QSharedPointer<VAbstractCurve> curve = data->GeometricObject<VAbstractCurve>(curveId);
        length = new VCurveLength(curveId, curveId, curve.data(), *data->GetPatternUnit());
    }
    catch (const VExceptionBadId &)
    {
        length = new VCurveLength();
    }

    SCASSERT(length != nullptr)
    length->SetName(currentLength);

    data->SetTransientVariable(length);
}

//---------------------------------------------------------------------------------------------------------------------
void SetTabStopDistance(QPlainTextEdit *edit, int tabWidthChar)
{
    SCASSERT(edit != nullptr)
    const auto fontMetrics = edit->fontMetrics();

    const QString testString(" ");

    // compute the size of a char in double-precision
    static constexpr int bigNumber = 1000; // arbitrary big number.
    const int many_char_width = fontMetrics.horizontalAdvance(testString.repeated(bigNumber));
    const double singleCharWidthDouble = many_char_width / double(bigNumber);
    // set the tab stop with double precision
    edit->setTabStopDistance(tabWidthChar * singleCharWidthDouble);
}

//---------------------------------------------------------------------------------------------------------------------
auto SegmentAliases(GOType curveType, const QString &alias1, const QString &alias2) -> QPair<QString, QString>
{
    // This check helps to find missed objects in the switch
    Q_STATIC_ASSERT_X(static_cast<int>(GOType::Unknown) == 8, "Not all objects were handled.");

    switch (curveType)
    {
        case GOType::EllipticalArc:
            return CurveAliases<VEllipticalArc>(alias1, alias2);
        case GOType::Arc:
            return CurveAliases<VArc>(alias1, alias2);
        case GOType::CubicBezier:
            return CurveAliases<VCubicBezier>(alias1, alias2);
        case GOType::Spline:
            return CurveAliases<VSpline>(alias1, alias2);
        case GOType::CubicBezierPath:
            return CurveAliases<VCubicBezierPath>(alias1, alias2);
        case GOType::SplinePath:
            return CurveAliases<VSplinePath>(alias1, alias2);
        case GOType::Point:
        case GOType::PlaceLabel:
        case GOType::Unknown:
            Q_UNREACHABLE();
            break;
        default:
            return {};
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto SegmentNames(GOType curveType, const QString &name1, const QString &name2) -> QPair<QString, QString>
{
    // This check helps to find missed objects in the switch
    Q_STATIC_ASSERT_X(static_cast<int>(GOType::Unknown) == 8, "Not all objects were handled.");

    switch (curveType)
    {
        case GOType::EllipticalArc:
            return CurveNames<VEllipticalArc>(name1, name2);
        case GOType::Arc:
            return CurveNames<VArc>(name1, name2);
        case GOType::CubicBezier:
            return CurveNames<VCubicBezier>(name1, name2);
        case GOType::Spline:
            return CurveNames<VSpline>(name1, name2);
        case GOType::CubicBezierPath:
            return CurveNames<VCubicBezierPath>(name1, name2);
        case GOType::SplinePath:
            return CurveNames<VSplinePath>(name1, name2);
        case GOType::Point:
        case GOType::PlaceLabel:
        case GOType::Unknown:
            Q_UNREACHABLE();
            break;
        default:
            return {};
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto GetNodeName(const VContainer *data, const VPieceNode &node, bool showPassmarkDetails) -> QString
{
    const QSharedPointer<VGObject> obj = data->GetGObject(node.GetId());
    QString name = obj->ObjectName();

    if (node.GetTypeTool() != Tool::NodePoint)
    {
        if (node.GetReverse())
        {
            name = "- "_L1 + name;
        }
    }
    else
    {
        if (showPassmarkDetails && node.IsPassmark())
        {
            switch (node.GetPassmarkLineType())
            {
                case PassmarkLineType::OneLine:
                    name += '|'_L1;
                    break;
                case PassmarkLineType::TwoLines:
                    name += "||"_L1;
                    break;
                case PassmarkLineType::ThreeLines:
                    name += "|||"_L1;
                    break;
                case PassmarkLineType::TMark:
                    name += QStringLiteral("┴");
                    break;
                case PassmarkLineType::ExternalVMark:
                    name += QStringLiteral("⊼");
                    break;
                case PassmarkLineType::InternalVMark:
                    name += QStringLiteral("⊽");
                    break;
                case PassmarkLineType::UMark:
                    name += QStringLiteral("⋃");
                    break;
                case PassmarkLineType::BoxMark:
                    name += QStringLiteral("⎕");
                    break;
                case PassmarkLineType::CheckMark:
                    name += QStringLiteral("✓");
                    break;
                default:
                    break;
            }
        }

        if (not node.IsCheckUniqueness())
        {
            name = '['_L1 + name + ']'_L1;
        }

        if (not node.IsTurnPoint())
        {
            name += QStringLiteral(" ⊙");
        }
    }

    return name;
}

//---------------------------------------------------------------------------------------------------------------------
auto FindNotExcludedPointDown(QListWidget *listWidget, int start) -> int
{
    SCASSERT(listWidget != nullptr);

    int index = -1;
    if (start < 0 || start >= listWidget->count())
    {
        return index;
    }

    int i = start;
    int count = 0;
    do
    {
        const QListWidgetItem *rowItem = listWidget->item(i);
        SCASSERT(rowItem != nullptr)
        auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));

        if (not rowNode.IsExcluded() && rowNode.GetTypeTool() == Tool::NodePoint && rowNode.GetId() != NULL_ID)
        {
            index = i;
            break;
        }

        ++i;
        if (i >= listWidget->count())
        {
            i = 0;
        }

        ++count;
    } while (count < listWidget->count());

    return index;
}

//---------------------------------------------------------------------------------------------------------------------
auto FindNotExcludedCurveDown(QListWidget *listWidget, int start) -> int
{
    SCASSERT(listWidget != nullptr);

    int index = -1;
    if (start < 0 || start >= listWidget->count())
    {
        return index;
    }

    int i = start;
    int count = 0;
    do
    {
        const QListWidgetItem *rowItem = listWidget->item(i);
        SCASSERT(rowItem != nullptr)
        auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));

        if (not rowNode.IsExcluded() && rowNode.GetTypeTool() != Tool::NodePoint && rowNode.GetId() != NULL_ID)
        {
            index = i;
            break;
        }

        ++i;
        if (i >= listWidget->count())
        {
            i = 0;
        }

        ++count;
    } while (count < listWidget->count());

    return index;
}

//---------------------------------------------------------------------------------------------------------------------
auto InvalidSegment(QListWidget *listWidget, const VContainer *data, QString &error) -> bool
{
    SCASSERT(listWidget != nullptr);

    for (int index = 0; index < listWidget->count(); ++index)
    {
        int firstCurveIndex = -1;
        int pointIndex = -1;
        int secondCurveIndex = -1;

        auto FindPair = [listWidget, &firstCurveIndex, &pointIndex, &secondCurveIndex](int start)
        {
            for (int i = start; i < listWidget->count(); ++i)
            {
                firstCurveIndex = FindNotExcludedCurveDown(listWidget, i);
                if (firstCurveIndex == -1)
                {
                    continue;
                }

                pointIndex = FindNotExcludedPointDown(listWidget, firstCurveIndex + 1);
                if (pointIndex == -1)
                {
                    continue;
                }

                secondCurveIndex = FindNotExcludedCurveDown(listWidget, pointIndex + 1);
                if (secondCurveIndex == -1 || firstCurveIndex == secondCurveIndex)
                {
                    continue;
                }

                return true;
            }

            return false;
        };

        if (not FindPair(index))
        {
            continue;
        }

        const VPieceNode firstCurveNode = RowNode(listWidget, firstCurveIndex);
        const VPieceNode secondCurveNode = RowNode(listWidget, secondCurveIndex);

        QString errorDoubleCurve;
        if (not DoubleCurve(firstCurveNode, secondCurveNode, data, errorDoubleCurve))
        {
            continue;
        }

        const VPieceNode pointNode = RowNode(listWidget, pointIndex);
        if (pointNode.GetId() == NULL_ID)
        {
            continue;
        }

        try
        {
            const QSharedPointer<VAbstractCurve> curve1 = data->GeometricObject<VAbstractCurve>(firstCurveNode.GetId());
            const QSharedPointer<VPointF> point = data->GeometricObject<VPointF>(pointNode.GetId());

            error = QObject::tr("Point '%1' does not lie on a curve '%2'").arg(point->name(), curve1->name());

            bool const validSegment = curve1->IsPointOnCurve(point->toQPointF());
            if (not validSegment)
            {
                return true;
            }
        }
        catch (const VExceptionBadId &)
        {
            continue;
        }
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto MirrorLinePointsNeighbors(QListWidget *listWidget, quint32 startPoint, quint32 endPoint) -> bool
{
    SCASSERT(listWidget != nullptr)

    if (startPoint == endPoint || startPoint == NULL_ID || endPoint == NULL_ID)
    {
        return false;
    }

    const int startIndex = NodeRowIndex(listWidget, startPoint);
    if (startIndex == -1)
    {
        return false;
    }

    int nextIndex = startIndex + 1;
    if (nextIndex >= listWidget->count())
    {
        nextIndex = 0;
    }

    int prevIndex = startIndex - 1;
    if (prevIndex < 0)
    {
        prevIndex = listWidget->count() - 1;
    }

    const int next = FindNotExcludedNeighborNodeDown(listWidget, nextIndex);
    const int prev = FindNotExcludedNeighborNodeUp(listWidget, prevIndex);

    return (next >= 0 && endPoint == RowNode(listWidget, next).GetId()) ||
           (prev >= 0 && endPoint == RowNode(listWidget, prev).GetId());
}

//---------------------------------------------------------------------------------------------------------------------
auto NodeRowIndex(QListWidget *listWidget, quint32 id) -> int
{
    SCASSERT(listWidget != nullptr)

    for (int i = 0, sz = listWidget->count(); i < sz; ++i)
    {
        const QListWidgetItem *rowItem = listWidget->item(i);
        SCASSERT(rowItem != nullptr)
        auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));

        if (id == rowNode.GetId())
        {
            return i;
        }
    }

    return -1;
}

//---------------------------------------------------------------------------------------------------------------------
void InitColorPicker(VPE::QtColorPicker *button)
{
    SCASSERT(button != nullptr)

    button->clear();
    const QMap<QString, QString> defaultColors = VAbstractTool::ColorsList();
    const bool dark = VTheme::ColorSheme() == VColorSheme::Dark;

    auto i = defaultColors.constBegin();
    while (i != defaultColors.constEnd())
    {
        button->insertColor(ColorNameToQColorForTheme(i.key(), dark), i.value());
        ++i;
    }

    button->setCurrentColor(ColorNameToQColorForTheme(ColorBlack, dark));
}

//---------------------------------------------------------------------------------------------------------------------
void InitOperationColorPicker(VPE::QtColorPicker *button)
{
    SCASSERT(button != nullptr)

    button->clear();
    QMap<QString, QString> defaultColors = VAbstractOperation::OperationColorsList();
    const bool dark = VTheme::ColorSheme() == VColorSheme::Dark;

    if (defaultColors.contains(ColorDefault))
    {
        button->insertColor(QColor(), defaultColors.value(ColorDefault));
        defaultColors.remove(ColorDefault);
    }

    auto i = defaultColors.constBegin();
    while (i != defaultColors.constEnd())
    {
        button->insertColor(ColorNameToQColorForTheme(i.key(), dark), i.value());
        ++i;
    }

    button->setCurrentColor(QColor());
}

//---------------------------------------------------------------------------------------------------------------------
void InitColorCombo(QComboBox *combo)
{
    SCASSERT(combo != nullptr)

    const bool dark = VTheme::ColorSheme() == VColorSheme::Dark;
    const QMap<QString, QString> colors = VAbstractTool::ColorsList();
    const int iconSize = combo->style()->pixelMetric(QStyle::PM_SmallIconSize);

    combo->blockSignals(true);
    combo->clear();

    auto i = colors.constBegin();
    while (i != colors.constEnd())
    {
        const QColor swatch = ColorNameToQColorForTheme(i.key(), dark);
        QString displayName = i.value();
        if (dark && i.key() == ColorBlack)
        {
            displayName = QCoreApplication::translate("VAbstractTool", "White");
        }

        QPixmap pix(iconSize, iconSize);
        pix.fill(Qt::transparent);
        QPainter painter(&pix);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(swatch);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(1, 1, iconSize - 2, iconSize - 2, 3, 3);
        painter.end();

        combo->addItem(QIcon(pix), displayName, i.key());
        ++i;
    }

    combo->blockSignals(false);
    combo->setCurrentIndex(0);
}

//---------------------------------------------------------------------------------------------------------------------
void SetColorComboCurrentName(QComboBox *combo, const QString &colorName)
{
    if (colorName.isEmpty() || colorName == ColorDefault)
    {
        combo->setCurrentIndex(0); // default = black
        return;
    }

    // Try exact match on color constant name.
    int idx = combo->findData(colorName, Qt::UserRole);
    if (idx >= 0)
    {
        combo->setCurrentIndex(idx);
        return;
    }

    // Try legacy SVG name → Ostara hex → Ostara name.
    QColor c = ColorNameToQColor(colorName);
    if (!c.isValid())
    {
        c = QColor(colorName); // Try as direct hex.
    }
    if (c.isValid())
    {
        const QString baseName = ColorNameFromQColor(c);
        idx = combo->findData(baseName, Qt::UserRole);
        if (idx >= 0)
        {
            combo->setCurrentIndex(idx);
            return;
        }
    }

    // Fallback: black.
    combo->setCurrentIndex(0);
}

//---------------------------------------------------------------------------------------------------------------------
auto GetColorComboCurrentName(const QComboBox *combo) -> QString
{
    return combo->currentData(Qt::UserRole).toString();
}

//---------------------------------------------------------------------------------------------------------------------
auto ReplaceColorPickerWithCombo(QWidget *parent, VPE::QtColorPicker *oldPicker) -> QComboBox *
{
    auto *combo = MakeComboBox(parent);
    combo->setSizePolicy(oldPicker->sizePolicy());

    QWidget *pw = oldPicker->parentWidget();
    if (pw != nullptr && pw->layout() != nullptr)
    {
        pw->layout()->replaceWidget(oldPicker, combo);
    }
    oldPicker->deleteLater();

    InitColorCombo(combo);
    return combo;
}

//---------------------------------------------------------------------------------------------------------------------
void FillComboBoxTypeLine(QComboBox *box, const QMap<QString, QIcon> &stylesPics, const QString &def)
{
    SCASSERT(box != nullptr)
    ApplyComboBoxView(box);
    QMap<QString, QIcon>::const_iterator i = stylesPics.constBegin();
    while (i != stylesPics.constEnd())
    {
        box->addItem(i.value(), QString(), QVariant(i.key()));
        ++i;
    }

    // A style that cannot be found falls back to the solid line, never to the first item of the combo. The
    // items are added in the alphabetical order of a QMap, so their first is 'dashDotLine' — a default that
    // fell through to it silently opened a line tool on a dash-dot pen instead of a plain one.
    const QString wanted = def.isEmpty() ? TypeLineLine : def;
    const int index = box->findData(QVariant(wanted));
    if (index != -1)
    {
        box->setCurrentIndex(index);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ChangeCurrentData select item in combobox by id
 * @param box combobox
 * @param value id of item
 */
void ChangeCurrentData(QComboBox *box, const QVariant &value)
{
    SCASSERT(box != nullptr)
    const qint32 index = box->findData(value);
    if (index != -1)
    {
        box->setCurrentIndex(index);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto GetComboBoxCurrentData(const QComboBox *box, const QString &def) -> QString
{
    SCASSERT(box != nullptr)
    QString value;
    value = box->currentData().toString();

    if (value.isEmpty())
    {
        value = def;
    }
    return value;
}

//---------------------------------------------------------------------------------------------------------------------
auto GenerateDefSubCurveName(const VContainer *data,
                             quint32 curveId,
                             const QString &derBase,
                             const QString &base,
                             const QString &pointName) -> QString
{
    QSharedPointer<VAbstractCurve> const curve = data->GeometricObject<VAbstractCurve>(curveId);

    if (!curve->IsDerivative())
    {
        return GenerateUniqueCurveName(data, curve->GetTypeHead(), curve->HeadlessName() + derBase);
    }

    return GenerateUniqueCurveName(data, curve->GetTypeHead(), base, pointName);
}

//---------------------------------------------------------------------------------------------------------------------
auto GenerateDefOffsetCurveName(const VContainer *data, quint32 curveId, const QString &derBase, const QString &base)
    -> QString
{
    auto GenerateName = [data](const QString &base) -> QString
    {
        VSplinePath path;
        qint32 num = 2;
        QString name;
        do
        {
            name = base + QString::number(num++);
            path.SetNameSuffix(name);

        } while (!data->IsUnique(path.name()));

        return name;
    };
    QSharedPointer<VAbstractCurve> const curve = data->GeometricObject<VAbstractCurve>(curveId);
    if (!curve->IsDerivative())
    {
        return GenerateName(curve->HeadlessName() + derBase);
    }

    return GenerateName(base);
}

//---------------------------------------------------------------------------------------------------------------------
auto GenerateUniqueCurveName(const VContainer *data,
                             const QString &typeHead,
                             const QString &base,
                             const QString &pointName) -> QString
{
    QString name = pointName + base;
    if (data->IsUnique(typeHead + name))
    {
        return name;
    }

    qint32 num = 2;
    do
    {
        name = pointName + base + QString::number(num++);
    } while (!data->IsUnique(typeHead + name));

    return name;
}



//---------------------------------------------------------------------------------------------------------------------
void MakeFilterableComboBox(QComboBox *combo, Qt::CaseSensitivity cs)
{
    SCASSERT(combo != nullptr)

    combo->setEditable(true);
    auto *completer = new QCompleter(combo->model(), combo);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCaseSensitivity(cs);
    combo->setCompleter(completer);
}
