/************************************************************************
 **
 **  @file   vcontainer.h
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

#ifndef VCONTAINER_H
#define VCONTAINER_H

#include <QCoreApplication>
#include <QHash>
#include <QMap>
#include <QMessageLogger>
#include <QSet>
#include <QSharedData>
#include <QSharedDataPointer>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QTypeInfo>
#include <QtGlobal>
#include <new>

#include "../ifc/exception/vexceptionbadid.h"
#include "../ifc/ifcdef.h"
#include "../vgeometry/vabstractcubicbezierpath.h"
#include "../vgeometry/vabstractcurve.h"
#include "../vgeometry/vgobject.h"
#include "../vmisc/def.h"
#include "variables/vinternalvariable.h"
#include "opiecegroup.h"
#include "osizerange.h"
#include "vpiece.h"
#include "vpiecepath.h"
#include "vtranslatevars.h"

class VEllipticalArc;
class VMeasurement;
class VIncrement;
class VLengthLine;
class VCurveLength;
class VCurveCLength;
class VLineAngle;
class VArcRadius;
class VCurveAngle;
class VPieceArea;

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Weffc++")
QT_WARNING_DISABLE_INTEL(2021)
QT_WARNING_DISABLE_GCC("-Wnon-virtual-dtor")

class VContainerData final : public QSharedData //-V690
{
public:
    VContainerData(const VTranslateVars *trVars, const Unit *patternUnit, QString nspace);
    VContainerData(const VContainerData &data) = default;
    ~VContainerData();

    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    QHash<quint32, QSharedPointer<VGObject>> calculationObjects{};
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    QSharedPointer<QHash<quint32, QSharedPointer<VGObject>>> modelingObjects{
        QSharedPointer<QHash<quint32, QSharedPointer<VGObject>>>::create()};

    /**
     * @brief variables container for measurements, increments, lines lengths, lines angles, arcs lengths, curve lengths
     */
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    QSharedPointer<QHash<QString, QSharedPointer<VInternalVariable>>> variables{
        QSharedPointer<QHash<QString, QSharedPointer<VInternalVariable>>>::create()};

    /**
     * @brief lineRoleNames role-name suffix for lines, keyed by "P1_P2" (ordered pair of point names).
     *        Lines have no VGObject to carry an alias; the suffix lives here.
     *        The persistent source is the <lineNames> DOM section under <pattern>.
     */
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    QHash<QString, QString> lineRoleNames{};

    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    QSharedPointer<QHash<quint32, VPiece>> pieces{QSharedPointer<QHash<quint32, VPiece>>::create()};
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    QSharedPointer<QHash<quint32, OPieceGroup>> pieceGroups{QSharedPointer<QHash<quint32, OPieceGroup>>::create()};
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    QSharedPointer<QHash<QString, OSizerange>> sizeRanges{
        QSharedPointer<QHash<QString, OSizerange>>::create()};
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    QSharedPointer<QHash<quint32, VPiecePath>> piecePaths{QSharedPointer<QHash<quint32, VPiecePath>>::create()};

    const VTranslateVars *trVars; // NOLINT(misc-non-private-member-variables-in-classes)
    const Unit *patternUnit;      // NOLINT(misc-non-private-member-variables-in-classes)

    /** @brief nspace namespace for static variables */
    QString nspace; // NOLINT(misc-non-private-member-variables-in-classes)

private:
    Q_DISABLE_ASSIGN_MOVE(VContainerData) // NOLINT
};

QT_WARNING_POP

/**
 * @brief The VContainer class container of all variables.
 */
class VContainer
{
    Q_DECLARE_TR_FUNCTIONS(VContainer) // NOLINT

public:
    VContainer(const VTranslateVars *trVars, const Unit *patternUnit, const QString &nspace);
    VContainer(const VContainer &data);
    ~VContainer();

    friend class VContainerData;

    auto operator=(const VContainer &data) -> VContainer &;

    VContainer(VContainer &&data) noexcept;
    auto operator=(VContainer &&data) noexcept -> VContainer &;

    static auto UniqueNamespace() -> QString;

    template <typename T> auto GeometricObject(const quint32 &id) const -> QSharedPointer<T>;
    auto GetGObject(quint32 id) const -> QSharedPointer<VGObject>;

    /** @brief Whether an object is held under @a id, in either draw mode. The question a caller has to ask
     *  before resolving an id it read from a file: @a GeometricObject throws, and there are cases where not
     *  knowing yet is not an error. */
    auto HasObject(quint32 id) const -> bool;

    /** @brief The first of @a ids that holds a point, NULL_ID when there is none.
     *
     * A rotation can be set by a length only for a point: that length is the chord of the image of this very
     * point (see Modifications/Ostara/Tools/Draw/Operations/Rotation.md). Ids whose object is gone are
     * skipped rather than reported. */
    auto FirstPointId(const QVector<quint32> &ids) const -> quint32;

    /** @brief Distance in pixels between two points of the pattern, 0 when one of them is not a point.
     *
     * A rotation set by a length turns around the pivot at that distance (see Rotation.md). */
    auto PointsDistance(quint32 first, quint32 second) const -> qreal;

    static auto GetFakeGObject(quint32 id) -> QSharedPointer<VGObject>;
    auto GetPiece(quint32 id) const -> VPiece;
    auto GetPiecePath(quint32 id) const -> VPiecePath;
    auto ContainsPiecePath(quint32 id) const -> bool;
    auto GetPieceForPiecePath(quint32 id) const -> quint32;
    template <typename T> auto GetVariable(const QString &name) const -> QSharedPointer<T>;
    auto getId() const -> quint32;
    auto getNextId() const -> quint32;
    void UpdateId(quint32 newId) const;
    static void UpdateId(quint32 newId, const QString &nspace);

    void RegisterUniqueName(const QSharedPointer<VGObject> &obj) const;
    void RegisterUniqueName(const QString &name, const QString &alias) const;

    auto AddGObject(VGObject *obj) -> quint32;
    auto AddGObject(const QSharedPointer<VGObject> &obj) -> quint32;
    auto AddPiece(const VPiece &detail) -> quint32;
    auto AddPieceGroup(const OPieceGroup &group) -> quint32;
    auto AddPiecePath(const VPiecePath &path) -> quint32;
    /** @brief Register a line of the pattern: the pair of points, as a length and an angle variable.
     *
     *  @a takeRoleName false for a line the pattern does not name — the transformed line an operation builds.
     *  Its endpoints are copies whose names can be exactly the ones of the pair it comes from, and the role
     *  name of that pair belongs to the line the user named: taking it would make `Line_<name>` designate the
     *  copy. */
    void AddLine(quint32 firstPointId, quint32 secondPointId, quint32 mainReference = NULL_ID,
                 bool takeRoleName = true);
    void SetLineRoleName(const QString &pairKey, const QString &suffix);
    void RemoveLineRoleName(const QString &pairKey);
    void ClearLineRoleNames();
    auto LineRoleName(const QString &pairKey) const -> QString;
    auto LineRoleNames() const -> const QHash<QString, QString> &;
    void AddArc(const QSharedPointer<VAbstractCurve> &arc, const quint32 &id, const quint32 &parentId = NULL_ID);
    void AddSpline(const QSharedPointer<VAbstractBezier> &curve, quint32 id, quint32 parentId = NULL_ID);
    void AddCurveWithSegments(const QSharedPointer<VAbstractCubicBezierPath> &curve, const quint32 &id,
                              quint32 parentId = NULL_ID);

    template <typename T> void AddUniqueVariable(T *var);
    template <typename T> void AddUniqueVariable(const QSharedPointer<T> &var);
    template <typename T> void AddVariable(T *var);
    template <typename T> void AddVariable(const QSharedPointer<T> &var);
    void RemoveVariable(const QString &name);
    void RemovePiece(quint32 id);
    void RemovePieceGroup(quint32 id);

    /** @brief Before a group is erased, spread its stored graded offset (groupContent) onto
     *  each surviving member's pieceContent so a Graded move is not lost when the group
     *  dissolves. */
    void TransferGroupOffsetToMembers(quint32 groupId);

    /** @brief Update an object of the container, holding it by a QSharedPointer.
     *
     * There is deliberately no overload taking a raw pointer. The container keeps the object it already holds
     * when the id is known — it copies the new value into it — so a raw pointer wrapped in a temporary
     * QSharedPointer would be freed at the end of the call, leaving the caller's object dangling: the
     * use-after-free that crashed every tool edit until ten call sites were fixed. Making the caller declare
     * the QSharedPointer is what lets the compiler refuse that shape. */
    template <class T> void UpdateGObject(quint32 id, const QSharedPointer<T> &obj);

    /** @brief Update an object of the container from the value itself.
     *
     * Same as the overload above for the callers that hold a value and not a handle: nothing has to be allocated
     * just to hand an object over. The container copies what it is given and writes only its own copy. */
    template <class T> void UpdateGObject(quint32 id, const T &value);

    void UpdatePiece(quint32 id, const VPiece &detail);
    void UpdatePieceGroup(quint32 id, const OPieceGroup &group);
    void UpdatePiecePath(quint32 id, const VPiecePath &path);

    void Clear();
    void ClearForFullParse();
    void ClearGObjects();
    void ClearVariables(const VarType &type = VarType::Unknown);
    void ClearVariables(const QVector<VarType> &types);

    /** @brief Give this container its own variable table. Every copy of a container shares one table, so a
     *  copy that means to trim it — an increment keeps a snapshot of the pattern with no internal variable —
     *  must detach it first, or the trim reaches the live pattern. */
    void DetachVariables();

    /** @brief Tolerant formula reading, used by the first pass of a parse: a formula may name a variable that
     *  a block further down the file defines, so an unknown symbol is recorded instead of stopping the parse.
     *  The pattern then reads itself a second time, strictly, once every variable exists.
     *
     *  Static on purpose: these belong to the parse in progress, not to a container. Tool snapshots copy the
     *  container, and a copied "tolerant" flag would follow a tool into a dialog long after the parse. */
    static void SetDeferredFormulaMode(bool value);
    static auto IsDeferredFormulaMode() -> bool;
    static void DeferFormula(const QString &missingName);
    static auto DeferredFormulas() -> int;
    static auto DeferredFormulaNames() -> QStringList;
    static void ResetDeferredFormulas();

    /** @brief True while a pattern is being read. A formula that fails there must not open the "edit the
     *  formula" dialog: that dialog runs its own event loop, the queued parse runs again inside it, fails
     *  again and opens another dialog - the nesting only ends with the process. The parse reports instead. */
    static void SetParseInProgress(bool value);
    static auto IsParseInProgress() -> bool;

    /** @brief Prepare the second pass of a parse. The value variables a reparse recomputes are cleared - so
     *  they are registered once and not suffixed into duplicates - except the ones the deferred formulas were
     *  waiting for: those come from the first pass, where every block had already been read. */
    static void PrepareSecondParsePass(VContainer *data);
    void ClearUniqueNames() const;
    void ClearUniqueIncrementNames() const;
    void ClearExceptUniqueIncrementNames() const;

    void RemoveUniqueName(const QString &name) const;

    void RemoveIncrement(const QString &name);

    void FillPiecesAreas(Unit unit);

    auto CalculationGObjects() const -> const QHash<quint32, QSharedPointer<VGObject>> *;
    auto DataPieces() const -> QHash<quint32, VPiece> *;
    auto DataPieceGroups() const -> QHash<quint32, OPieceGroup> *;
    auto DataPieceGroups() -> QHash<quint32, OPieceGroup> *;
    auto SizeRanges() const -> const QHash<QString, OSizerange> *;
    void SetSizeRanges(const QHash<QString, OSizerange> &ranges);
    auto GetPieceGroupId(quint32 pieceId) const -> quint32;
    auto IsPieceInLayout(quint32 pieceId) const -> bool;
    auto GetEffectivePieceCount() const -> int;
    auto DataVariables() const -> const QHash<QString, QSharedPointer<VInternalVariable>> *;

    auto DataMeasurements() const -> QMap<QString, QSharedPointer<VMeasurement>>;
    auto DataMeasurementsWithSeparators() const -> QMap<QString, QSharedPointer<VMeasurement>>;
    auto DataIncrements() const -> QMap<QString, QSharedPointer<VIncrement>>;
    auto DataIncrementsWithSeparators() const -> QMap<QString, QSharedPointer<VIncrement>>;
    auto DataLengthLines() const -> QMap<QString, QSharedPointer<VLengthLine>>;
    auto DataLengthCurves() const -> QMap<QString, QSharedPointer<VCurveLength>>;
    auto DataCurvesCLength() const -> QMap<QString, QSharedPointer<VCurveCLength>>;
    auto DataAngleLines() const -> QMap<QString, QSharedPointer<VLineAngle>>;
    auto DataRadiusesArcs() const -> QMap<QString, QSharedPointer<VArcRadius>>;
    auto DataAnglesCurves() const -> QMap<QString, QSharedPointer<VCurveAngle>>;
    auto DataPieceArea() const -> QMap<QString, QSharedPointer<VPieceArea>>;

    auto DataDependencyVariables() const -> QHash<QString, QList<quint32>>;

    auto IsUnique(const QString &name) const -> bool;
    static auto IsUnique(const QString &name, const QString &nspace) -> bool;

    /**
     * @brief First free number for a name already taken: 1 for @c base_1, 2 when that one is taken too.
     *
     * Every tool that names an object after something already in the pattern — a path after its two ends, a
     * rectangle after its two corners — asks the same question, and the number is what keeps its name usable
     * in a formula. It lives here because uniqueness is what this container knows about.
     */
    auto UniqueDuplicate(const QString &baseName) const -> quint32;

    auto AllUniqueNames() const -> QStringList;
    static auto AllUniqueNames(const QString &nspace) -> QStringList;

    auto GetPatternUnit() const -> const Unit *;
    auto GetTrVars() const -> const VTranslateVars *;

private:
    /**
     * @brief _id current id. New object will have value +1. For empty class equal 0.
     */
    static QMap<QString, quint32> _id;
    static QMap<QString, QSet<QString>> uniqueNames;
    static QMap<QString, quint32> copyCounter;

    /** @brief See SetDeferredFormulaMode: state of the parse in progress, shared by every container.
     *
     *  Static on purpose: the live container and every copy made while a pattern is read have to see the same
     *  state — a tool defers while it is built, and VPattern::Parse reads the count to decide whether a second
     *  pass is needed. It is therefore shared by all containers and all snapshots, and it is written and read
     *  only during a parse: VPattern::Parse brackets every pass with SetParseInProgress, and the only caller of
     *  DeferFormula (VAbstractTool::CheckFormula) is guarded by it. A parse that ever ran on two threads at once
     *  would need this carried explicitly through the parse instead of living here; nothing does today. */
    static bool deferredFormulaMode;
    static int deferredFormulas;
    static QSet<QString> deferredFormulaNames;
    static bool parseInProgress;

    QSharedDataPointer<VContainerData> d;

    void AddCurve(const QSharedPointer<VAbstractCurve> &curve, const quint32 &id, quint32 parentId = NULL_ID);

    template <typename T> void AddVariable(const QSharedPointer<T> &var, const QString &name);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    template <class T> auto qHash(const QSharedPointer<T> &p, uint seed = 0) -> uint;
#else
    template <class T> auto qHash(const QSharedPointer<T> &p, size_t seed = 0) -> size_t;
#endif

    template <typename T> void UpdateObject(const quint32 &id, const QSharedPointer<T> &point);
    template <typename T> void UpdateObject(const quint32 &id, const T &value);

    /** @brief The table of this container that holds the objects of @a mode, or nullptr for Layout. */
    auto ObjectsFor(Draw mode) -> QHash<quint32, QSharedPointer<VGObject>> *;

    template <typename T> auto DataVar(const VarType &type) const -> QMap<QString, QSharedPointer<T>>;

    static void ClearNamespace(const QString &nspace);
};

Q_DECLARE_TYPEINFO(VContainer, Q_MOVABLE_TYPE); // NOLINT

/*
 *  Defintion of templated member functions of VContainer
 */

//---------------------------------------------------------------------------------------------------------------------
template <typename T> auto VContainer::GeometricObject(const quint32 &id) const -> QSharedPointer<T>
{
    if (id == NULL_ID)
    {
        throw VExceptionBadId(tr("Can't find object"), id);
    }

    QSharedPointer<VGObject> gObj;
    if (d->calculationObjects.contains(id))
    {
        gObj = d->calculationObjects.value(id);
    }
    else if (d->modelingObjects->contains(id))
    {
        gObj = d->modelingObjects->value(id);
    }
    else
    {
        throw VExceptionBadId(tr("Can't find object"), id);
    }

    QSharedPointer<T> obj = qSharedPointerDynamicCast<T>(gObj);
    if (obj.isNull())
    {
        const QString msg = tr("Can't cast object to the requested type. Object id = %1, actual type = %2.")
                                .arg(id)
                                .arg(static_cast<int>(gObj->getType()));
        throw VExceptionBadId(msg, id);
    }
    return obj;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetVariable return varible by name
 * @param name variable's name
 * @return variable
 */
template <typename T> auto VContainer::GetVariable(const QString &name) const -> QSharedPointer<T>
{
    SCASSERT(name.isEmpty() == false)
    if (d->variables->contains(name))
    {
        try
        {
            QSharedPointer<VInternalVariable> const gVar = d->variables->value(name);
            QSharedPointer<T> value = qSharedPointerDynamicCast<T>(gVar);
            if (value.isNull())
            {
                // SCASSERT is compiled away in release builds (V_NO_ASSERT), so a failed cast would
                // silently return a null pointer that callers dereference and crash on. Throw instead
                // so the existing VExceptionBadId handling deals with the wrong-type case.
                const QString msg = QCoreApplication::translate(
                                        "VContainer", "Can't cast object to the requested type. Name = '%1', type = %2.")
                                        .arg(name)
                                        .arg(static_cast<int>(gVar->GetType()));
                throw VExceptionBadId(msg, name);
            }
            return value;
        }
        catch (const std::bad_alloc &)
        {
            throw VExceptionBadId(QCoreApplication::translate("VContainer", "Can't cast object"), name);
        }
    }
    else
    {
        throw VExceptionBadId(QCoreApplication::translate("VContainer", "Can't find object"), name);
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T> void VContainer::AddUniqueVariable(T *var)
{
    AddUniqueVariable(QSharedPointer<T>(var));
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T> void VContainer::AddUniqueVariable(const QSharedPointer<T> &var)
{
    AddVariable(var);

    if (d->variables->contains(var->GetName()))
    {
        uniqueNames[d->nspace].insert(var->GetName());
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T> void VContainer::AddVariable(T *var)
{
    AddVariable(QSharedPointer<T>(var));
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T> void VContainer::AddVariable(const QSharedPointer<T> &var)
{
    SCASSERT(not var->GetName().isEmpty())
    AddVariable(var, var->GetName());

    // Simplified prefix (RoleName step 1) — additional key
    const QString simplified = SimplifiedName(var->GetName());
    if (simplified != var->GetName())
    {
        AddVariable(var, simplified);
    }

    if (not var->GetAlias().isEmpty())
    {
        AddVariable(var, var->GetAlias());

        const QString simplifiedAlias = SimplifiedName(var->GetAlias());
        if (simplifiedAlias != var->GetAlias())
        {
            AddVariable(var, simplifiedAlias);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
template<typename T>
void VContainer::AddVariable(const QSharedPointer<T> &var, const QString &name)
{
    if (name.isEmpty())
    {
        return;
    }


    if (d->variables->contains(name))
    {
        if (d->variables->value(name)->GetType() == var->GetType())
        {
            QSharedPointer<T> v = qSharedPointerDynamicCast<T>(d->variables->value(name));
            if (v.isNull())
            {
                throw VExceptionBadId(tr("Can't cast object. Name = '%1', type = %2.")
                                          .arg(name)
                                          .arg(static_cast<int>(d->variables->value(name)->GetType())),
                                      name);
            }

            *v = *var;
        }
        else
        {
            throw VExceptionBadId(tr("Can't find object. Type mismatch. Name = '%1', existing type = %2, "
                                     "incoming type = %3.")
                                      .arg(name)
                                      .arg(static_cast<int>(d->variables->value(name)->GetType()))
                                      .arg(static_cast<int>(var->GetType())),
                                  name);
        }
    }
    else
    {
        d->variables->insert(name, var);
    }
}

//---------------------------------------------------------------------------------------------------------------------
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
template <class T> auto VContainer::qHash(const QSharedPointer<T> &p, uint seed) -> uint
#else
template <class T> auto VContainer::qHash(const QSharedPointer<T> &p, size_t seed) -> size_t
#endif
{
    return qHash(p.data(), seed);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief UpdateGObject update GObject by id
 * @param id id of existing GObject
 * @param obj object, held by the caller for as long as it is used
 */
template <class T> void VContainer::UpdateGObject(quint32 id, const QSharedPointer<T> &obj)
{
    SCASSERT(not obj.isNull())
    UpdateObject(id, obj);
    RegisterUniqueName(obj);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief UpdateGObject update GObject by id, from the value itself
 * @param id id of existing GObject
 * @param value object; the container copies it, and the caller keeps it
 */
template <class T> void VContainer::UpdateGObject(quint32 id, const T &value)
{
    UpdateObject(id, value);
    RegisterUniqueName(value.name(), value.GetAlias());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief UpdateObject update an object the container holds, from a handle the caller keeps
 *
 * When the id is already known the value is copied into the object the container holds, and its key is stamped
 * back on that copy: the object handed in is only read, and never comes back modified. Only when the container
 * has nothing under that id does it adopt what it is given — and then it is the one stored, so it is the one
 * that has to carry its id.
 *
 * @param id id of existing object
 * @param point object
 */
template <typename T> void VContainer::UpdateObject(const quint32 &id, const QSharedPointer<T> &point)
{
    if (id == NULL_ID)
    {
        // An object keyed 0 would be a ghost the container hands back through GeometricObject(0). Refused where
        // the id is known to be wrong, instead of inserted and found later.
        qWarning("Can't update an object whose id is 0.");
        return;
    }

    QHash<quint32, QSharedPointer<VGObject>> *objects = ObjectsFor(point->getMode());
    if (objects == nullptr)
    {
        qWarning("Can't update an object with mode 'Layout'");
        return;
    }

    if (objects->contains(id))
    {
        QSharedPointer<T> obj = qSharedPointerDynamicCast<T>(objects->value(id));
        if (obj.isNull())
        {
            throw VExceptionBadId(tr("Can't cast object"), id);
        }
        *obj = *point;
        obj->setId(id);
    }
    else
    {
        point->setId(id);
        objects->insert(id, point);
    }

    UpdateId(id);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief UpdateObject update an object the container holds, from the value itself
 *
 * Same as the overload above for the callers that hold a value and not a handle. Nothing of the caller's is
 * modified, and nothing of the container's is shared with it: an object that came back changed, or that both
 * sides owned, is the trap that already cost a use-after-free here.
 *
 * @param id id of existing object
 * @param value object
 */
template <typename T> void VContainer::UpdateObject(const quint32 &id, const T &value)
{
    if (id == NULL_ID)
    {
        // Same refusal as the overload above, and the same reason: a Debug build must not abort where the
        // pointer version warns, or the two halves of one hardening would answer differently.
        qWarning("Can't update an object whose id is 0.");
        return;
    }

    QHash<quint32, QSharedPointer<VGObject>> *objects = ObjectsFor(value.getMode());
    if (objects == nullptr)
    {
        qWarning("Can't update an object with mode 'Layout'");
        return;
    }

    if (objects->contains(id))
    {
        QSharedPointer<T> obj = qSharedPointerDynamicCast<T>(objects->value(id));
        if (obj.isNull())
        {
            throw VExceptionBadId(tr("Can't cast object"), id);
        }
        *obj = value;
        obj->setId(id);
    }
    else
    {
        // The value may well be a local or a member of the caller's: the container takes its own copy rather
        // than a pointer to it.
        const QSharedPointer<T> stored = QSharedPointer<T>::create(value);
        stored->setId(id);
        objects->insert(id, stored);
    }

    UpdateId(id);
}
#endif // VCONTAINER_H
