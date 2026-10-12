/************************************************************************
 **
 **  @file   vpattern_increments.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Increment parsing and CRUD methods extracted from vpattern.cpp
 **
 **  @note   Extracted from vpattern.cpp by Sophie Denys <contact@sophiedenys.com>
 **          as part of Ostara refactoring.
 **
 **  @copyright
 **  Copyright (C) 2013-2015 Valentina project
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

#include "vpattern.h"

#include "../core/vapplication.h"
#include "../ifc/exception/vexceptionconversionerror.h"
#include "../ifc/exception/vexceptionemptyparameter.h"
#include "../ifc/exception/vexceptionobjecterror.h"
#include "../ifc/exception/vexceptionundo.h"
#include "../ifc/exception/vexceptionwrongid.h"
#include "../qmuparser/qmuparsererror.h"
#include "../qmuparser/qmutokenparser.h"
#include "../vmisc/customevents.h"
#include "../vmisc/def.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vpatterndb/calculator.h"
#include "../vpatterndb/variables/vincrement.h"

#include <QApplication>
#include <QScopedPointer>

//---------------------------------------------------------------------------------------------------------------------
void VPattern::LiteParseIncrements()
{
    try
    {
        emit SetEnabledGUI(true);

        data->ClearUniqueIncrementNames();

        Q_STATIC_ASSERT_X(static_cast<int>(VarType::Unknown) == 12, "Check that you used all types");
        data->ClearVariables(VarType::Increment);
        data->ClearVariables(VarType::IncrementSeparator);

        QDomNodeList tags = elementsByTagName(TagIncrements);
        if (not tags.isEmpty())
        {
            if (const QDomNode domElement = tags.at(0); not domElement.isNull())
            {
                ParseIncrementsElement(domElement, Document::LiteParse);
            }
        }

        tags = elementsByTagName(TagPreviewCalculations);
        if (not tags.isEmpty())
        {
            if (const QDomNode domElement = tags.at(0); not domElement.isNull())
            {
                ParseIncrementsElement(domElement, Document::LiteParse);
            }
        }
    }
    catch (const VExceptionUndo &e)
    {
        Q_UNUSED(e)
        /* If user want undo last operation before undo we need finish broken redo operation. For those we post event
         * myself. Later in method customEvent call undo.*/
        QApplication::postEvent(this, new UndoEvent());
        return;
    }
    catch (const VExceptionObjectError &e)
    {
        qCCritical(vXML, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error parsing file.")), //-V807
                   qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        emit SetEnabledGUI(false);
        return;
    }
    catch (const VExceptionConversionError &e)
    {
        qCCritical(vXML, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error can't convert value.")),
                   qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        emit SetEnabledGUI(false);
        return;
    }
    catch (const VExceptionEmptyParameter &e)
    {
        qCCritical(vXML, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error empty parameter.")),
                   qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        emit SetEnabledGUI(false);
        return;
    }
    catch (const VExceptionWrongId &e)
    {
        qCCritical(vXML, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error wrong id.")), qUtf8Printable(e.ErrorMessage()),
                   qUtf8Printable(e.DetailedInformation()));
        emit SetEnabledGUI(false);
        return;
    }
    catch (VException &e)
    {
        qCCritical(vXML, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error parsing file.")), qUtf8Printable(e.ErrorMessage()),
                   qUtf8Printable(e.DetailedInformation()));
        emit SetEnabledGUI(false);
        return;
    }
    catch (const std::bad_alloc &)
    {
        qCCritical(vXML, "%s", qUtf8Printable(tr("Error parsing file (std::bad_alloc).")));
        emit SetEnabledGUI(false);
        return;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VPattern::EvalFormula(VContainer *data, const QString &formula, bool *ok) const -> qreal
{
    if (formula.isEmpty())
    {
        *ok = true;
        return 0;
    }

    try
    {
        QScopedPointer<Calculator> const cal(new Calculator());
        const qreal result = cal->EvalFormula(data->DataVariables(), formula);

        (qIsInf(result) || qIsNaN(result)) ? *ok = false : *ok = true;
        return result;
    }
    catch (qmu::QmuParserError &e)
    {
        Q_UNUSED(e)
        *ok = false;
        return 0;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VPattern::MakeEmptyIncrement(const QString &name, IncrementType type) -> QDomElement
{
    QDomElement element = createElement(TagIncrement);
    SetAttribute(element, AttrName, name);
    if (type != IncrementType::Increment)
    {
        SetAttribute(element, AttrType, IncrementTypeToString(type));
    }
    return element;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPattern::FindIncrement(const QString &name) const -> QDomElement
{
    QDomNodeList const list = elementsByTagName(TagIncrement);

    for (int i = 0; i < list.size(); ++i)
    {
        if (const QDomElement domElement = list.at(i).toElement(); not domElement.isNull())
        {
            if (const QString parameter = domElement.attribute(AttrName); parameter == name)
            {
                return domElement;
            }
        }
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::NewEmptyIncrement(const QString &type, const QString &name, IncrementType varType)
{
    const QDomElement element = MakeEmptyIncrement(name, varType);

    const QDomNodeList list = elementsByTagName(type);
    list.at(0).appendChild(element);
    emit patternChanged(false);
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::NewEmptyIncrementAfter(const QString &type, const QString &after, const QString &name,
                                      IncrementType varType)
{
    const QDomElement element = MakeEmptyIncrement(name, varType);
    const QDomElement sibling = FindIncrement(after);

    const QDomNodeList list = elementsByTagName(type);

    if (sibling.isNull())
    {
        list.at(0).appendChild(element);
    }
    else
    {
        list.at(0).insertAfter(element, sibling);
    }
    emit patternChanged(false);
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::RemoveIncrement(const QString &type, const QString &name)
{
    const QDomNodeList list = elementsByTagName(type);
    list.at(0).removeChild(FindIncrement(name));
    emit patternChanged(false);

    // The rows of one table are filed in its own list: what is dropped is the list of the table the row just left.
    // Removing the last increment of an imported group takes the group with it — nothing stands under a heading no
    // row refers to, and the reader has no gesture to take it away.
    DropEmptyIncrementGroups(type == TagPreviewCalculations);
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::MoveUpIncrement(const QString &type, const QString &name)
{
    if (const QDomElement node = FindIncrement(name); not node.isNull())
    {
        const QDomElement prSibling = node.previousSiblingElement(TagIncrement);
        if (not prSibling.isNull())
        {
            elementsByTagName(type).at(0).insertBefore(node, prSibling);
        }
    }
    emit patternChanged(false);
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::MoveDownIncrement(const QString &type, const QString &name)
{
    if (const QDomElement node = FindIncrement(name); not node.isNull())
    {
        const QDomElement nextSibling = node.nextSiblingElement(TagIncrement);
        if (not nextSibling.isNull())
        {
            elementsByTagName(type).at(0).insertAfter(node, nextSibling);
        }
    }
    emit patternChanged(false);
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::SetIncrementAttribute(const QString &name, const QString &attr, const QString &text)
{
    if (QDomElement node = FindIncrement(name); not node.isNull())
    {
        SetAttribute(node, attr, text);
        emit patternChanged(false);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ParseIncrementsElement parse increments tag.
 * @param node tag in xml tree.
 * @param parse parser file mode.
 */
void VPattern::ParseIncrementsElement(const QDomNode &node, const Document &parse)
{
    int index = 0;
    QDomNode domNode = node.firstChild();
    while (not domNode.isNull())
    {
        if (domNode.isElement())
        {
            if (const QDomElement domElement = domNode.toElement();
                not domElement.isNull() && domElement.tagName() == TagIncrement)
            {
                const QString name = GetParametrString(domElement, AttrName, QString()).simplified();
                const QString desc = GetParametrEmptyString(domElement, AttrDescription);
                const IncrementType type =
                    StringToIncrementType(GetParametrString(domElement, AttrType, strTypeIncrement));
                const QString formula = (type == IncrementType::Separator)
                                            ? QChar('0')
                                            : GetParametrString(domElement, AttrFormula, QChar('0'));
                const bool specialUnits = GetParametrBool(domElement, AttrSpecialUnits, falseStr);

                bool ok = false;
                const qreal value = EvalFormula(data, formula, &ok);

                auto *increment = new VIncrement(data, name, type);
                increment->SetIndex(static_cast<quint32>(index++));
                increment->SetFormula(value, formula, ok);
                increment->SetDescription(desc);
                increment->SetSpecialUnits(specialUnits);
                increment->SetPreviewCalculation(node.toElement().tagName() == TagPreviewCalculations);
                data->AddUniqueVariable(increment);
            }
        }
        domNode = domNode.nextSibling();
    }

    if (parse == Document::FullParse)
    {
        emit MadeProgress();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::AddEmptyIncrement(const QString &name, IncrementType type)
{
    NewEmptyIncrement(TagIncrements, name, type);
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::AddEmptyPreviewCalculation(const QString &name, IncrementType type)
{
    NewEmptyIncrement(TagPreviewCalculations, name, type);
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::AddEmptyIncrementAfter(const QString &after, const QString &name, IncrementType type)
{
    NewEmptyIncrementAfter(TagIncrements, after, name, type);
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::AddEmptyPreviewCalculationAfter(const QString &after, const QString &name, IncrementType type)
{
    NewEmptyIncrementAfter(TagPreviewCalculations, after, name, type);
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::RemoveIncrement(const QString &name)
{
    RemoveIncrement(TagIncrements, name);
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::RemovePreviewCalculation(const QString &name)
{
    RemoveIncrement(TagPreviewCalculations, name);
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::MoveUpIncrement(const QString &name)
{
    MoveUpIncrement(TagIncrements, name);
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::MoveUpPreviewCalculation(const QString &name)
{
    MoveUpIncrement(TagPreviewCalculations, name);
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::MoveDownIncrement(const QString &name)
{
    MoveDownIncrement(TagIncrements, name);
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::MoveDownPreviewCalculation(const QString &name)
{
    MoveDownIncrement(TagPreviewCalculations, name);
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::SetIncrementName(const QString &name, const QString &text)
{
    SetIncrementAttribute(name, AttrName, text);
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::SetIncrementFormula(const QString &name, const QString &text)
{
    SetIncrementAttribute(name, AttrFormula, text);
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::SetIncrementDescription(const QString &name, const QString &text)
{
    if (not text.isEmpty())
    {
        SetIncrementAttribute(name, AttrDescription, text);
    }
    else
    {
        if (QDomElement node = FindIncrement(name); not node.isNull())
        {
            node.removeAttribute(AttrDescription);
            emit patternChanged(false);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::SetIncrementSpecialUnits(const QString &name, bool special)
{
    if (QDomElement node = FindIncrement(name); not node.isNull())
    {
        SetAttributeOrRemoveIf<bool>(node, AttrSpecialUnits, special,
                                     [](bool special) noexcept { return not special; });
        emit patternChanged(false);
    }
    else
    {
        qWarning() << tr("Can't find increment '%1'").arg(name);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ReplaceNameInFormula(QVector<VFormulaField> &expressions, const QString &name, const QString &newName)
{
    const auto bias = name.length() - newName.length();

    for (int i = 0; i < expressions.size(); ++i)
    {
        if (expressions.at(i).expression.indexOf(name) != -1)
        {
            QMap<vsizetype, QString> tokens;

            // Eval formula
            try
            {
                QScopedPointer<qmu::QmuTokenParser> const cal(
                    new qmu::QmuTokenParser(expressions.at(i).expression, false, false));
                tokens = cal->GetTokens(); // Tokens (variables, measurements)
            }
            catch (const qmu::QmuParserError &)
            {
                continue; // Because we not sure if used. A formula is broken.
            }

            QList<QString> tValues = tokens.values();
            if (not tValues.contains(name))
            {
                continue;
            }

            QList<vsizetype> tKeys = tokens.keys(); // Take all tokens positions
            QString newFormula = expressions.at(i).expression;

            for (int i = 0; i < tKeys.size(); ++i)
            {
                if (tValues.at(i) != name)
                {
                    continue;
                }

                newFormula.replace(tKeys.at(i), name.length(), newName);

                if (bias != 0)
                { // Translated token has different length than original. Position next tokens need to be corrected.
                    VTranslateVars::BiasTokens(tKeys.at(i), bias, tokens);
                    tKeys = tokens.keys();
                    tValues = tokens.values();
                }
            }

            expressions[i].expression = newFormula;
            expressions[i].element.setAttribute(expressions.at(i).attribute, newFormula);
            emit patternChanged(false);
        }
    }
}
