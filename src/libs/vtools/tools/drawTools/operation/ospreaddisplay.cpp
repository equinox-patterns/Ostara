/************************************************************************
 **
 **  @file   ospreaddisplay.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The visual representation of a slash-and-spread: the mark of every rotation, and the pen it is
 **          drawn with.
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

#include "ospreaddisplay.h"

#include <QSharedPointer>

#include "../ifc/exception/vexceptionbadid.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "../vmisc/literals.h"
#include "../vpatterndb/vcontainer.h"
#include "vabstractoperation.h"

using namespace Qt::Literals::StringLiterals;

const QString OSpreadDisplay::TagDisplay = QStringLiteral("rotationDisplay");
const QString OSpreadDisplay::AttrShow = QStringLiteral("show");
const QString OSpreadDisplay::AttrPoint = QStringLiteral("point");
const QString OSpreadDisplay::AttrLine1 = QStringLiteral("line1");
const QString OSpreadDisplay::AttrLine2 = QStringLiteral("line2");

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/** @brief The position a mark's third vertex stands at: the start point of the cut, turned by the steps it has
 *  already been through when the rotation begins. Empty when the start point is not a point of the container
 *  any more: a closed region has no third vertex to place. */
auto ThirdVertex(const OSpreadCut &cut, const QString &name, const VContainer *data) -> QSharedPointer<VPointF>
{
    QSharedPointer<VPointF> vertex;
    try
    {
        vertex = QSharedPointer<VPointF>::create(*data->GeometricObject<VPointF>(cut.startId));
    }
    catch (const VExceptionBadId &)
    {
        return {};
    }

    for (int i = 0; i < cut.carried.size(); ++i)
    {
        const OSpreadStep &step = cut.carried.at(i);
        // Only the last step carries the name, exactly as the copies do it: handing a name to every step would
        // stack suffixes the user never asked for.
        *vertex = vertex->Rotate(step.pivot, step.angle, (i == cut.carried.size() - 1) ? name : QString());
    }

    return vertex;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The name of a mark's third vertex: the name of the point it comes from, and the generation it shows.
 *
 * A point a slash-and-spread materializes is named <source>__R; the third vertex of a rotation is that same
 * point one generation earlier, so it takes the number of the generation, <source>__R1. A name already taken,
 * by the user or by another mark, falls back to the numbered form the rest of the application uses. */
auto ThirdVertexName(const OSpreadCut &cut, const VContainer *data) -> QString
{
    QString base;
    try
    {
        base = data->GeometricObject<VPointF>(cut.startId)->name();
    }
    catch (const VExceptionBadId &)
    {
        return {};
    }

    return CopyName(data, base, CopySuffix::Spread + QString::number(cut.carried.size()));
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void OSpreadDisplay::EnsureIds(OSpreadDisplay &display, const QVector<OSpreadRotation> &rotations,
                               const OSpreadPlan &plan, VContainer *data, bool withLines)
{
    // One mark per rotation, at the rotation's own place: the two lists are read together, so a rotation that
    // draws nothing still holds its rank.
    display.marks.resize(rotations.size());

    for (int k = 0; k < rotations.size() && k < plan.cuts.size(); ++k)
    {
        const OSpreadCut &cut = plan.cuts.at(k);
        OSpreadMark &mark = display.marks[k];

        if (cut.startId == NULL_ID)
        {
            mark = OSpreadMark{}; // The pair describes no region: this rotation draws nothing.
            continue;
        }

        // The third vertex is a source object when no earlier rotation carried the start point: there the mark
        // creates no point, the start point itself is the vertex.
        if (not cut.carried.isEmpty() && mark.point == NULL_ID)
        {
            mark.point = data->getNextId();
        }

        if (mark.point != NULL_ID && mark.name.isEmpty())
        {
            mark.name = ThirdVertexName(cut, data);
        }

        if (not withLines)
        {
            // The arms are off: their ids go, or the file would keep declaring lines nobody builds.
            mark.line1 = NULL_ID;
            mark.line2 = NULL_ID;
            continue;
        }

        if (mark.line1 == NULL_ID)
        {
            mark.line1 = data->getNextId();
        }

        if (mark.line2 == NULL_ID)
        {
            mark.line2 = data->getNextId();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OSpreadDisplay::Create(quint32 idTool, const OSpreadDisplay &display, const OSpreadPlan &plan,
                            VContainer *data)
{
    for (int k = 0; k < plan.cuts.size() && k < display.marks.size(); ++k)
    {
        const OSpreadCut &cut = plan.cuts.at(k);
        const OSpreadMark &mark = display.marks.at(k);

        if (cut.startId == NULL_ID || mark.point == NULL_ID)
        {
            continue;
        }

        const QSharedPointer<VPointF> vertex = ThirdVertex(cut, mark.name, data);
        if (vertex.isNull())
        {
            continue;
        }

        vertex->setIdObject(idTool);
        // The label place the entry keeps, when it keeps one: zero means the figure never said, and the place of the
        // point the vertex comes from — which the clone inherited — then stands. See OToolSpread: the vertex is no
        // destination, so the entry of the mark is the only place its label can be kept.
        if (not qFuzzyIsNull(mark.mx) || not qFuzzyIsNull(mark.my))
        {
            vertex->setMx(mark.mx);
            vertex->setMy(mark.my);
        }
        // The vertex is a construction mark: its name is read in the tree, where it names the object, and not on
        // the canvas, where it would sit under the lines of the display.
        vertex->SetShowLabel(false);

        data->UpdateGObject(mark.point, vertex);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OSpreadDisplay::PointImages(const OSpreadDisplay &display, const VContainer *data) -> QVector<quint32>
{
    QVector<quint32> ids;
    ids.reserve(display.marks.size());
    for (const OSpreadMark &mark : std::as_const(display.marks))
    {
        // A mark whose point the container does not know is a mark that was never placed: it has no image to
        // offer, and handing its id over would have the caller ask the container for an object that is not there.
        if (mark.point != NULL_ID && data->HasObject(mark.point))
        {
            ids.append(mark.point);
        }
    }
    return ids;
}

//---------------------------------------------------------------------------------------------------------------------
void OSpreadDisplay::Save(VAbstractPattern *doc, QDomElement &tag, const OSpreadDisplay &display)
{
    SCASSERT(doc != nullptr)

    // The element is written whole: a rotation that stops describing a region loses its mark, and the ids of a
    // mark that is gone must not stay behind in the file.
    for (QDomElement old = tag.firstChildElement(TagDisplay); not old.isNull();)
    {
        const QDomElement next = old.nextSiblingElement(TagDisplay);
        tag.removeChild(old);
        old = next;
    }

    QDomElement element = doc->createElement(TagDisplay);
    doc->SetAttribute(element, AttrShow, display.show);
    doc->SetAttribute(element, AttrTypeLine, display.typeLine);
    doc->SetAttribute(element, AttrLineColor, display.lineColor);

    for (const OSpreadMark &mark : std::as_const(display.marks))
    {
        // An entry per rotation, even an empty one: the two lists are read side by side, and dropping the
        // rotations that draw nothing would pair every following mark with the wrong rotation.
        QDomElement item = doc->createElement(VAbstractOperation::TagItem);
        if (mark.point != NULL_ID)
        {
            doc->SetAttribute(item, AttrPoint, mark.point);
            // The label place, written like everything else in the pattern: pixels out, the pattern's unit in the
            // file, and only when the entry carries one.
            if (not qFuzzyIsNull(mark.mx))
            {
                doc->SetAttribute(item, AttrMx, VAbstractValApplication::VApp()->fromPixel(mark.mx));
            }
            if (not qFuzzyIsNull(mark.my))
            {
                doc->SetAttribute(item, AttrMy, VAbstractValApplication::VApp()->fromPixel(mark.my));
            }
            if (not mark.name.isEmpty())
            {
                doc->SetAttribute(item, AttrName, mark.name);
            }
        }
        if (mark.line1 != NULL_ID)
        {
            doc->SetAttribute(item, AttrLine1, mark.line1);
        }
        if (mark.line2 != NULL_ID)
        {
            doc->SetAttribute(item, AttrLine2, mark.line2);
        }
        element.appendChild(item);
    }

    tag.appendChild(element);
}

//---------------------------------------------------------------------------------------------------------------------
auto OSpreadDisplay::Extract(const QDomElement &domElement) -> OSpreadDisplay
{
    OSpreadDisplay display;

    for (QDomElement element = domElement.firstChildElement(TagDisplay); not element.isNull();
         element = element.nextSiblingElement(TagDisplay))
    {
        display.show = VAbstractPattern::GetParametrBool(element, AttrShow, trueStr);
        display.typeLine = VAbstractPattern::GetParametrString(element, AttrTypeLine, TypeLineLine);
        display.lineColor = VAbstractPattern::GetParametrString(element, AttrLineColor, ColorBlack);

        for (QDomElement item = element.firstChildElement(VAbstractOperation::TagItem); not item.isNull();
             item = item.nextSiblingElement(VAbstractOperation::TagItem))
        {
            OSpreadMark mark;
            mark.point = VAbstractPattern::GetParametrUInt(item, AttrPoint, NULL_ID_STR);
            mark.mx = VAbstractValApplication::VApp()->toPixel(VAbstractPattern::GetParametrDouble(item, AttrMx,
                                                                                                    "0.0"));
            mark.my = VAbstractValApplication::VApp()->toPixel(VAbstractPattern::GetParametrDouble(item, AttrMy,
                                                                                                    "0.0"));
            mark.line1 = VAbstractPattern::GetParametrUInt(item, AttrLine1, NULL_ID_STR);
            mark.line2 = VAbstractPattern::GetParametrUInt(item, AttrLine2, NULL_ID_STR);
            mark.name = VDomDocument::GetParametrEmptyString(item, AttrName);
            display.marks.append(mark);
        }

        break; // Only the first element is read, exactly as the operation lines are: an operation has one.
    }

    return display;
}
