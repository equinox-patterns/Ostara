/************************************************************************
 **
 **  @file   ooperationdestination.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  Images of the source objects of an operation. The operation tools — the two flippings, move,
 **          rotation, close dart and the transfer — used to carry the same factory: the same switch over
 **          GOType, the same five helpers per shape, the same loops. Only the transformation differed:
 **          one line per tool. The factory is written once here and takes that transformation as a
 **          parameter, so adding an operation is a new transform class, not one more copy of this file.
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

#ifndef OOPERATIONDESTINATION_H
#define OOPERATIONDESTINATION_H

#include <QPointF>
#include <QSharedPointer>
#include <QString>
#include <QVector>
#include <QtDebug>
#include <QtGlobal>

#include "../../toolsdef.h"
#include "../ifc/ifcdef.h"
#include "../vgeometry/varc.h"
#include "../vgeometry/vcubicbezier.h"
#include "../vgeometry/vcubicbezierpath.h"
#include "../vgeometry/vellipticalarc.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "../vgeometry/vspline.h"
#include "../vgeometry/vsplinepath.h"
#include "../vpatterndb/vcontainer.h"

/**
 * @brief The OOperationDestination class builds the image of every source object of an operation.
 *
 * The transformation is the only thing that tells a rotation from a translation or a mirror: it is
 * passed in, and it needs nothing more than
 *
 *     template <class Item> auto Image(const Item &item, const QString &name) const -> Item;
 *
 * which points, arcs, splines and the three curve-with-segments types all answer to, since they all
 * declare Rotate, Move and Flip with the same signature. One method therefore serves every shape.
 *
 * Almost no state: the operation still keeps its own destination list, and this class only remembers which
 * sources are referenced rather than copied — the ids a transfer anchors its portion on.
 */
template <class Transform> class OOperationDestination
{
public:
    explicit OOperationDestination(Transform transform, QVector<quint32> referencedSources = {})
      : m_transform(std::move(transform)),
        m_referencedSources(std::move(referencedSources))
    {
    }

    /** @brief True for a source the operation references instead of copying: its entry keeps its place in the
     *  positional pairing with the images, but holds no object of its own — a transfer anchors its portion on a
     *  point of the host part, it does not duplicate it. One question, one answer: Create, Update and the tools
     *  that rebuild their destination by hand all ask it here. */
    auto IsReferenced(quint32 sourceId) const -> bool { return m_referencedSources.contains(sourceId); }

    /** @brief Create the image of every source object, in source order: destination[i] is the image of
     *  source[i]. Used when the operation is created from the GUI. The sources the operation references —
     *  IsReferenced — keep their place and get no image of their own. */
    void Create(quint32 idTool, const QVector<SourceItem> &source, VContainer *data,
                QVector<DestinationItem> &destination) const
    {
        destination.reserve(source.size());

        for (const auto &object : std::as_const(source))
        {
            if (IsReferenced(object.id))
            {
                destination.append({.recordId = object.recordId});
                continue;
            }

            destination.append(CreateObject(idTool, object, data));
        }
    }

    /** @brief Rebuild the images in place, under the ids the file gave them. Used when an operation is
     *  read back from a pattern. The two lists are written together, but a file can hold more sources
     *  than destinations: indexing the shorter one would read out of bounds on a mere open. */
    void Update(quint32 idTool, const QVector<SourceItem> &source, VContainer *data,
                const QVector<DestinationItem> &destination) const
    {
        const qsizetype count = qMin(source.size(), destination.size());

        for (int i = 0; i < count; ++i)
        {
            if (IsReferenced(source.at(i).id))
            {
                continue; // Referenced, not copied: there is no image of its own to rewrite.
            }

            UpdateObject(idTool, source.at(i), data, destination.at(i));
        }
    }

    /** @brief The image of one source object: a new pattern object carrying the transformed geometry,
     *  the name, the pen style and the colour of its source. */
    auto CreateObject(quint32 idTool, const SourceItem &sItem, VContainer *data) const -> DestinationItem
    {
        const QSharedPointer<VGObject> object = data->GetGObject(sItem.id);

        // This check helps to find missed objects in the switch
        Q_STATIC_ASSERT_X(static_cast<int>(GOType::Unknown) == 8, "Not all objects were handled.");

        QT_WARNING_PUSH
        QT_WARNING_DISABLE_GCC("-Wswitch-default")
        QT_WARNING_DISABLE_CLANG("-Wswitch-default")

        switch (object->getType())
        {
            case GOType::Point:
                return CreatePoint(sItem, idTool, data);
            case GOType::Arc:
                return CreateArc<VArc>(sItem, idTool, data);
            case GOType::EllipticalArc:
                return CreateArc<VEllipticalArc>(sItem, idTool, data);
            case GOType::Spline:
                return CreateCurve<VSpline>(sItem, idTool, data);
            case GOType::SplinePath:
                return CreateCurveWithSegments<VSplinePath>(sItem, idTool, data);
            case GOType::CubicBezier:
                return CreateCurve<VCubicBezier>(sItem, idTool, data);
            case GOType::CubicBezierPath:
                return CreateCurveWithSegments<VCubicBezierPath>(sItem, idTool, data);
            case GOType::PlaceLabel:
            case GOType::Unknown:
                // A place label is not an operated object, and an unknown type has no geometry to
                // transform: the source list is read from the pattern, so a hand-edited file can hold
                // either. It is reported and skipped rather than left undefined.
                qCDebug(vTool, "Object %u cannot be operated on, skipped", sItem.id);
                break;
        }

        QT_WARNING_POP

        return {};
    }

    /** @brief Rewrite an existing image in place, keeping the id and the label position the file gave it. */
    void UpdateObject(quint32 idTool, const SourceItem &sItem, VContainer *data, const DestinationItem &item) const
    {
        const QSharedPointer<VGObject> object = data->GetGObject(sItem.id);

        Q_STATIC_ASSERT_X(static_cast<int>(GOType::Unknown) == 8, "Not all objects were handled.");

        QT_WARNING_PUSH
        QT_WARNING_DISABLE_GCC("-Wswitch-default")
        QT_WARNING_DISABLE_CLANG("-Wswitch-default")

        switch (object->getType())
        {
            case GOType::Point:
                UpdatePoint(sItem, idTool, data, item);
                break;
            case GOType::Arc:
                UpdateArc<VArc>(sItem, idTool, data, item.id);
                break;
            case GOType::EllipticalArc:
                UpdateArc<VEllipticalArc>(sItem, idTool, data, item.id);
                break;
            case GOType::Spline:
                UpdateCurve<VSpline>(sItem, idTool, data, item.id);
                break;
            case GOType::SplinePath:
                UpdateCurveWithSegments<VSplinePath>(sItem, idTool, data, item.id);
                break;
            case GOType::CubicBezier:
                UpdateCurve<VCubicBezier>(sItem, idTool, data, item.id);
                break;
            case GOType::CubicBezierPath:
                UpdateCurveWithSegments<VCubicBezierPath>(sItem, idTool, data, item.id);
                break;
            case GOType::PlaceLabel:
            case GOType::Unknown:
                qCDebug(vTool, "Object %u cannot be operated on, skipped", sItem.id);
                break;
        }

        QT_WARNING_POP
    }

private:
    Q_DISABLE_COPY_MOVE(OOperationDestination) // NOLINT

    Transform m_transform;

    /** @brief The sources referenced rather than copied, decided once by the tool that owns the operation. */
    QVector<quint32> m_referencedSources;

    auto CreatePoint(const SourceItem &sItem, quint32 idTool, VContainer *data) const -> DestinationItem
    {
        const QSharedPointer<VPointF> point = data->GeometricObject<VPointF>(sItem.id);
        VPointF image = m_transform.Image(*point, sItem.name);
        image.setIdObject(idTool);

        return {.id = data->AddGObject(new VPointF(image)),
                .mx = image.mx(),
                .my = image.my(),
                .showLabel = image.IsShowLabel(),
                .recordId = sItem.recordId};
    }

    template <class Item>
    auto CreateItem(const SourceItem &sItem, quint32 idTool, VContainer *data) const -> DestinationItem
    {
        const QSharedPointer<Item> item = data->GeometricObject<Item>(sItem.id);
        Item image = m_transform.Image(*item, sItem.name);
        image.setIdObject(idTool);

        if (sItem.penStyle != TypeLineDefault)
        {
            image.SetPenStyle(sItem.penStyle);
        }

        if (sItem.color != ColorDefault)
        {
            image.SetColor(sItem.color);
        }

        return {.id = data->AddGObject(new Item(image)), .recordId = sItem.recordId};
    }

    template <class Item>
    auto CreateArc(const SourceItem &sItem, quint32 idTool, VContainer *data) const -> DestinationItem
    {
        const DestinationItem item = CreateItem<Item>(sItem, idTool, data);
        data->AddArc(data->GeometricObject<Item>(item.id), item.id);
        return item;
    }

    template <class Item>
    auto CreateCurve(const SourceItem &sItem, quint32 idTool, VContainer *data) const -> DestinationItem
    {
        const DestinationItem item = CreateItem<Item>(sItem, idTool, data);
        data->AddSpline(data->GeometricObject<Item>(item.id), item.id);
        return item;
    }

    template <class Item>
    auto CreateCurveWithSegments(const SourceItem &sItem, quint32 idTool, VContainer *data) const -> DestinationItem
    {
        const DestinationItem item = CreateItem<Item>(sItem, idTool, data);
        data->AddCurveWithSegments(data->GeometricObject<Item>(item.id), item.id);
        return item;
    }

    void UpdatePoint(const SourceItem &sItem, quint32 idTool, VContainer *data, const DestinationItem &item) const
    {
        const QSharedPointer<VPointF> point = data->GeometricObject<VPointF>(sItem.id);
        VPointF image = m_transform.Image(*point, sItem.name);
        image.setIdObject(idTool);
        image.setMx(item.mx);
        image.setMy(item.my);
        image.SetShowLabel(item.showLabel);

        data->UpdateGObject(item.id, image);
    }

    template <class Item>
    void UpdateItem(const SourceItem &sItem, quint32 idTool, VContainer *data, quint32 id) const
    {
        const QSharedPointer<Item> item = data->GeometricObject<Item>(sItem.id);
        Item image = m_transform.Image(*item, sItem.name);
        image.setIdObject(idTool);

        if (sItem.penStyle != TypeLineDefault)
        {
            image.SetPenStyle(sItem.penStyle);
        }

        if (sItem.color != ColorDefault)
        {
            image.SetColor(sItem.color);
        }

        data->UpdateGObject(id, image);
    }

    template <class Item>
    void UpdateArc(const SourceItem &sItem, quint32 idTool, VContainer *data, quint32 id) const
    {
        UpdateItem<Item>(sItem, idTool, data, id);
        data->AddArc(data->GeometricObject<Item>(id), id);
    }

    template <class Item>
    void UpdateCurve(const SourceItem &sItem, quint32 idTool, VContainer *data, quint32 id) const
    {
        UpdateItem<Item>(sItem, idTool, data, id);
        data->AddSpline(data->GeometricObject<Item>(id), id);
    }

    template <class Item>
    void UpdateCurveWithSegments(const SourceItem &sItem, quint32 idTool, VContainer *data, quint32 id) const
    {
        UpdateItem<Item>(sItem, idTool, data, id);
        data->AddCurveWithSegments(data->GeometricObject<Item>(id), id);
    }
};

#endif // OOPERATIONDESTINATION_H
