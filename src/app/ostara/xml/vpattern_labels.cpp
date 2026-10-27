/************************************************************************
 **
 **  @file   vpattern_labels.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Label generation, ReadOnly, LabelPrefix methods
 **          extracted from vpattern.cpp
 **
 **  @note   Extracted from vpattern.cpp by Sophie Denys <contact@sophiedenys.com>
 **          as part of Ostara refactoring.
 **
 **  @copyright
 **  Copyright (C) 2013-2015 Valentina project
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 17/10/2026.
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
#include "../ifc/xml/vpatternblockmapper.h"
#include "../vmisc/compatibility.h"
#include "../vmisc/def.h"
#include "vpattern_helpers.h"

using namespace Qt::Literals::StringLiterals;
using namespace VPatternHelpers;

//---------------------------------------------------------------------------------------------------------------------
auto VPattern::GetLabelBase(quint32 index) const -> QString
{
    QStringList alphabet;
    switch (VApplication::LabelLanguages().indexOf(GetLabelPrefix()))
    {
        case 0: // de
        {
            const auto al = QStringLiteral("A,Ä,B,C,D,E,F,G,H,I,J,K,L,M,N,O,Ö,P,Q,R,S,ß,T,U,Ü,V,W,X,Y,Z");
            alphabet = al.split(','_L1);
            break;
        }
        case 2: // fr
        {
            const auto al = QStringLiteral("A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z");
            alphabet = al.split(','_L1);
            break;
        }
        case 3: // ru
        {
            const auto al = QStringLiteral("А,Б,В,Г,Д,Е,Ж,З,И,К,Л,М,Н,О,П,Р,С,Т,У,Ф,Х,Ц,Ч,Ш,Щ,Э,Ю,Я");
            alphabet = al.split(','_L1);
            break;
        }
        case 4: // uk
        {
            const auto al = QStringLiteral("А,Б,В,Г,Д,Е,Ж,З,І,Ї,Й,К,Л,М,Н,О,П,Р,С,Т,У,Ф,Х,Ц,Ч,Ш,Щ,Є,Ю,Я");
            alphabet = al.split(','_L1);
            break;
        }
        case 5: // hr
        case 7: // bs
        {
            const auto al = QStringLiteral("A,B,C,Č,Ć,D,Dž,Ð,E,F,G,H,I,J,K,L,Lj,M,N,Nj,O,P,R,S,Š,T,U,V,Z,Ž");
            alphabet = al.split(','_L1);
            break;
        }
        case 6: // sr
        {
            const auto al = QStringLiteral("А,Б,В,Г,Д,Ђ,Е,Ж,З,И,Ј,К,Л,Љ,М,Н,Њ,О,П,Р,С,Т,Ћ,У,Ф,Х,Ц,Ч,Џ,Ш");
            alphabet = al.split(','_L1);
            break;
        }
        case 8: // cs
        {
            const auto al = QStringLiteral("a,á,b,c,č,d,ď,e,é,ě,f,g,h,ch,i,í,j,k,l,m,n,ň,o,ó,p,q,r,ř,s,š,t,ť,u,ú,ů,"
                                           "v,w,x,y,ý,z,ž");
            alphabet = al.split(','_L1);
            break;
        }
        case 1:  // en
        default: // en
        {
            const auto al = QStringLiteral("A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z");
            alphabet = al.split(','_L1);
            break;
        }
    }

    QString base;
    const int count = qFloor(index / static_cast<quint32>(alphabet.size()));
    const auto number = static_cast<int>(index) - static_cast<int>(alphabet.size()) * count;
    int i = 0;
    do
    {
        base.append(alphabet.at(number));
        ++i;
    } while (i < count);
    return base;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GenerateLabel create label for pattern piece of point.
 * @param type type of the label.
 * @param reservedName reversed point name. Use when need reserve name, but point is not in data base yet.
 * @return unique name for current pattern piece.
 */
auto VPattern::GenerateLabel(const LabelType &type, const QString &reservedName) const -> QString
{
    if (type == LabelType::NewPatternPiece)
    {
        const QDomNodeList drawList = elementsByTagName(TagDraw);
        QString name;
        int i = 0;
        for (;;)
        {
            name = GetLabelBase(static_cast<quint32>(drawList.size() + i));
            if (data->IsUnique(name))
            {
                return name;
            }
            if (i == INT_MAX)
            {
                break;
            }
            ++i;
        }
        qCDebug(vXML, "Point label: %s", qUtf8Printable(name));
        return name;
    }

    if (type == LabelType::NewLabel)
    {
        const int index = qMax(PatternBlockMapper()->GetActiveId(), 0);
        const QString labelBase = GetLabelBase(static_cast<quint32>(index));

        qint32 num = 1;
        QString name;
        do
        {
            name = QStringLiteral("%1%2").arg(labelBase).arg(num);
            num++;
            if (num == INT_MAX)
            {
                break;
            }
        } while (data->IsUnique(name) == false || name == reservedName);
        qCDebug(vXML, "Point label: %s", qUtf8Printable(name));
        return name;
    }
    qCDebug(vXML, "Got unknown type %d", static_cast<int>(type));
    return QString();
}

//---------------------------------------------------------------------------------------------------------------------
auto VPattern::IsReadOnly() const -> bool
{
    const QDomElement pattern = documentElement();

    if (pattern.isNull())
    {
        return false;
    }

    return GetParametrBool(pattern, AttrReadOnly, falseStr);
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::SetReadOnly(bool rOnly)
{
    QDomElement pattern = documentElement();

    if (not pattern.isNull())
    {
        SetAttributeOrRemoveIf<bool>(pattern, AttrReadOnly, rOnly, [](bool rOnly) noexcept { return not rOnly; });
        modified = true;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VPattern::GetLabelPrefix() const -> QString
{
    const QDomElement pattern = documentElement();

    if (pattern.isNull())
    {
        return DefLabelLanguage();
    }

    return GetParametrString(pattern, AttrLabelPrefix, DefLabelLanguage());
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::SetLabelPrefix(const QString &prefix)
{
    QDomElement pattern = documentElement();

    if (not pattern.isNull() && ConvertToSet<QString>(VApplication::LabelLanguages()).contains(prefix))
    {
        SetAttribute(pattern, AttrLabelPrefix, prefix);
        modified = true;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VPattern::LastDrawName() const -> QString
{
    const QDomNodeList elements = this->documentElement().elementsByTagName(TagDraw);
    if (elements.isEmpty())
    {
        return {};
    }

    if (const QDomElement &elem = elements.at(elements.size() - 1).toElement(); not elem.isNull())
    {
        return GetParametrString(elem, AttrName);
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
auto VPattern::LastToolId() const -> quint32
{
    const QString name = LastDrawName();
    if (name.isEmpty())
    {
        return NULL_ID;
    }

    return PPLastToolId(PatternBlockMapper()->FindId(name));
}

//---------------------------------------------------------------------------------------------------------------------
auto VPattern::PPLastToolId(int blockIndex) const -> quint32
{
    const QVector<VToolRecord> localHistory = GetLocalHistory(blockIndex);

    return (not localHistory.isEmpty() ? localHistory.constLast().GetId() : NULL_ID);
}
