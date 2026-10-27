/************************************************************************
 **
 **  @file   vcontainer.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   November 15, 2013
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 19/10/2026.
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

#include "vcontainer.h"
#include "vformula.h"

#include <climits>
#include <utility>
#include <QLineF>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>
#include <QVector>
#include <QtDebug>

#include "../ifc/ifcdef.h"
#include "../vgeometry/vabstractcubicbezierpath.h"
#include "../vgeometry/vabstractcurve.h"
#include "../vgeometry/varc.h"
#include "../vgeometry/vellipticalarc.h"
#include "../vgeometry/vgeometrydef.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "../vgeometry/vspline.h"
#include "vexception.h"
#include "../vmisc/literals.h"
#include "variables/varcradius.h"
#include "variables/vcurveangle.h"
#include "variables/vcurveclength.h"
#include "variables/vcurvelength.h"
#include "variables/vincrement.h"
#include "variables/vlineangle.h"
#include "variables/vlinelength.h"
#include "variables/vmeasurement.h"
#include "variables/vpiecearea.h"
#include "vtranslatevars.h"

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wmissing-prototypes")
QT_WARNING_DISABLE_INTEL(1418)

Q_LOGGING_CATEGORY(vCon, "v.container") // NOLINT

QT_WARNING_POP

QMap<QString, quint32> VContainer::_id = QMap<QString, quint32>();
QMap<QString, QSet<QString>> VContainer::uniqueNames = QMap<QString, QSet<QString>>();
QMap<QString, quint32> VContainer::copyCounter = QMap<QString, quint32>();
bool VContainer::deferredFormulaMode = false;
int VContainer::deferredFormulas = 0;
QSet<QString> VContainer::deferredFormulaNames = QSet<QString>();
bool VContainer::parseInProgress = false;

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VContainer create empty container
 */
VContainer::VContainer(const VTranslateVars *trVars, const Unit *patternUnit, const QString &nspace)
  : d(new VContainerData(trVars, patternUnit, nspace))
{
    if (nspace.isEmpty())
    {
        qFatal("Namesapce is empty.");
    }

    if (VContainer::_id.contains(nspace))
    {
        qFatal("Namespace is not unique.");
    }

    if (not _id.contains(d->nspace))
    {
        _id[d->nspace] = NULL_ID;
    }

    if (not uniqueNames.contains(d->nspace))
    {
        uniqueNames[d->nspace] = QSet<QString>();
    }

    if (not copyCounter.contains(d->nspace))
    {
        copyCounter[d->nspace] = 1;
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief operator = copy constructor
 * @param data container
 * @return copy container
 */
auto VContainer::operator=(const VContainer &data) -> VContainer &
{
    if (&data == this)
    {
        return *this;
    }
    d = data.d;
    ++copyCounter[d->nspace];
    return *this;
}

//---------------------------------------------------------------------------------------------------------------------
VContainer::VContainer(VContainer &&data) noexcept
  : d(std::move(data.d))
{
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::operator=(VContainer &&data) noexcept -> VContainer &
{
    std::swap(d, data.d);
    return *this;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VContainer create container from another container
 * @param data container
 */
VContainer::VContainer(const VContainer &data)
  : d(data.d)
{
    ++copyCounter[d->nspace];
}

//---------------------------------------------------------------------------------------------------------------------
VContainer::~VContainer() = default;

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::UniqueNamespace() -> QString
{
    QString candidate;
    do
    {
        candidate = QUuid::createUuid().toString();
    } while (_id.contains(candidate));

    return candidate;
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::ClearNamespace(const QString &nspace)
{
    _id.remove(nspace);
    uniqueNames.remove(nspace);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetGObject returns a point by id
 * @param id id of point
 * @return point
 */
// cppcheck-suppress unusedFunction
auto VContainer::GetGObject(quint32 id) const -> QSharedPointer<VGObject>
{
    if (d->calculationObjects.contains(id))
    {
        return d->calculationObjects.value(id);
    }

    if (d->modelingObjects->contains(id))
    {
        return d->modelingObjects->value(id);
    }

    throw VExceptionBadId(QCoreApplication::translate("VContainer", "Can't find object"), id);
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::HasObject(quint32 id) const -> bool
{
    return d->calculationObjects.contains(id) || d->modelingObjects->contains(id);
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::FirstPointId(const QVector<quint32> &ids) const -> quint32
{
    for (const quint32 &id : ids)
    {
        try
        {
            if (GetGObject(id)->getType() == GOType::Point)
            {
                return id;
            }
        }
        catch (const VExceptionBadId &)
        {
            continue; // the object is gone: it cannot be the reference of a length
        }
    }

    return NULL_ID;
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::PointsDistance(quint32 first, quint32 second) const -> qreal
{
    try
    {
        const QSharedPointer<VPointF> one = GeometricObject<VPointF>(first);
        const QSharedPointer<VPointF> other = GeometricObject<VPointF>(second);
        return QLineF(static_cast<QPointF>(*one), static_cast<QPointF>(*other)).length();
    }
    catch (const VExceptionBadId &)
    {
        return 0.; // one of them is gone, or is not a point at all
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::GetFakeGObject(quint32 id) -> QSharedPointer<VGObject>
{
    auto *obj = new VGObject();
    obj->setId(id);
    QSharedPointer<VGObject> pointer(obj);
    return pointer;
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::GetPiece(quint32 id) const -> VPiece
{
    if (d->pieces->contains(id))
    {
        return d->pieces->value(id);
    }

    throw VExceptionBadId(tr("Can't find object"), id);
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::GetPiecePath(quint32 id) const -> VPiecePath
{
    if (d->piecePaths->contains(id))
    {
        return d->piecePaths->value(id);
    }
    else
    {
        throw VExceptionBadId(tr("Can't find object"), id);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::ContainsPiecePath(quint32 id) const -> bool
{
    return d->piecePaths->contains(id);
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::GetPieceForPiecePath(quint32 id) const -> quint32
{
    auto i = d->pieces->constBegin();
    while (i != d->pieces->constEnd())
    {
        if (i.value().GetInternalPaths().contains(id))
        {
            return i.key();
        }
        ++i;
    }

    return NULL_ID;
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::RegisterUniqueName(const QSharedPointer<VGObject> &obj) const
{
    SCASSERT(not obj.isNull())
    RegisterUniqueName(obj->name(), obj->GetAlias());
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::RegisterUniqueName(const QString &name, const QString &alias) const
{
    uniqueNames[d->nspace].insert(name);

    if (not alias.isEmpty())
    {
        uniqueNames[d->nspace].insert(alias);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::ObjectsFor(Draw mode) -> QHash<quint32, QSharedPointer<VGObject>> *
{
    switch (mode)
    {
        case Draw::Calculation:
            return &d->calculationObjects;
        case Draw::Modeling:
            return d->modelingObjects.data();
        default:
            return nullptr;
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AddGObject add new GObject to container
 * @param obj new object
 * @return return id of new object in container
 */
auto VContainer::AddGObject(VGObject *obj) -> quint32
{
    SCASSERT(obj != nullptr)
    QSharedPointer<VGObject> const pointer(obj);
    return AddGObject(pointer);
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::AddGObject(const QSharedPointer<VGObject> &obj) -> quint32
{
    SCASSERT(not obj.isNull())

    RegisterUniqueName(obj);

    const quint32 id = getNextId();
    obj->setId(id);

    if (obj->getMode() == Draw::Calculation)
    {
        d->calculationObjects.insert(id, obj);
    }
    else if (obj->getMode() == Draw::Modeling)
    {
        d->modelingObjects->insert(id, obj);
    }

    return id;
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::AddPiece(const VPiece &detail) -> quint32
{
    const quint32 id = getNextId();
    d->pieces->insert(id, detail);
    return id;
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::AddPieceGroup(const OPieceGroup &group) -> quint32
{
    // Insert under the group's own id when it is already assigned (undo/redo stability),
    // otherwise auto-assign a fresh id.
    quint32 id = group.id;
    if (id == NULL_ID)
    {
        id = getNextId();
    }
    OPieceGroup stored = group;
    stored.id = id;  // keep the stored group's id in sync with its hash key (round-trip integrity)
    d->pieceGroups->insert(id, stored);
    return id;
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::AddPiecePath(const VPiecePath &path) -> quint32
{
    const quint32 id = getNextId();
    d->piecePaths->insert(id, path);
    return id;
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::getId() const -> quint32
{
    return _id.value(d->nspace);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getNextId generate next unique id
 * @return next unique id
 */
auto VContainer::getNextId() const -> quint32
{
    // TODO. Current count of ids are very big and allow us save time before someone will reach its max value.
    // Better way, of cource, is to seek free ids inside the set of values and reuse them.
    // But for now better to keep it as it is now.
    if (_id.value(d->nspace) == UINT_MAX)
    {
        qCritical() << (tr("Number of free id exhausted."));
    }
    _id[d->nspace]++;
    return _id.value(d->nspace);
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::UpdateId(quint32 newId, const QString &nspace)
{
    if (_id.contains(nspace))
    {
        if (newId > _id.value(nspace))
        {
            _id[nspace] = newId;
        }
    }
    else
    {
        throw VException(QStringLiteral("Unknown namespace"));
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief UpdateId update id. If new id bigger when current save new like current.
 * @param newId id
 */
void VContainer::UpdateId(quint32 newId) const
{
    VContainer::UpdateId(newId, d->nspace);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Clear clear data in container. Id will be 0.
 */
void VContainer::Clear()
{
    qCDebug(vCon, "Clearing container data.");
    _id[d->nspace] = NULL_ID;

    d->pieces->clear();
    d->piecePaths->clear();
    d->sizeRanges->clear(); // graded ranges belong to one pattern — never leak across files
    d->pieceGroups->clear(); // nor does a piece group: keeping one whose members are gone leaves a
                             // ghost group behind — a holder nothing can click without aborting
    ClearVariables();
    ClearGObjects();
    ClearUniqueNames();
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::ClearForFullParse()
{
    _id[d->nspace] = NULL_ID;

    d->pieces->clear();
    d->piecePaths->clear();
    d->sizeRanges->clear(); // graded ranges belong to one pattern — never leak across files
    d->pieceGroups->clear(); // nor does a piece group: keeping one whose members are gone leaves a
                             // ghost group behind — a holder nothing can click without aborting
    Q_STATIC_ASSERT_X(static_cast<int>(VarType::Unknown) == 12, "Check that you used all types");
    ClearVariables(QVector<VarType>{VarType::Increment, VarType::IncrementSeparator, VarType::LineAngle,
                                    VarType::LineLength, VarType::CurveLength, VarType::CurveCLength,
                                    VarType::ArcRadius, VarType::CurveAngle, VarType::PieceExternalArea,
                                    VarType::PieceSeamLineArea});
    ClearGObjects();
    ClearUniqueNames();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ClearObject points, splines, arcs, spline paths will be cleared.
 */
void VContainer::ClearGObjects()
{
    d->calculationObjects.clear();
    d->modelingObjects->clear();
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::ClearVariables(const VarType &type)
{
    ClearVariables(QVector<VarType>({type}));
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::ClearVariables(const QVector<VarType> &types)
{
    if (not d->variables->isEmpty()) //-V807
    {
        if (types.isEmpty() || types.contains(VarType::Unknown))
        {
            d->variables->clear();
        }
        else
        {
            for (auto i = d->variables->begin(); i != d->variables->end();)
            {
                if (types.contains(i.value()->GetType()))
                {
                    i = d->variables->erase(i);
                }
                else
                {
                    ++i;
                }
            }
        }
    }

}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::SetDeferredFormulaMode(bool value)
{
    // Only the flag: the count is what the caller reads to decide whether a second pass is needed, so it must
    // survive turning the mode off (see VPattern::Parse).
    deferredFormulaMode = value;
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::IsDeferredFormulaMode() -> bool
{
    return deferredFormulaMode;
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::DeferFormula(const QString &formula)
{
    ++deferredFormulas;

    // Which names the formula was waiting for is asked of the formula itself: the parser error is not reliable
    // here - it carries the offending token for some errors and nothing for others.
    static const QRegularExpression namePattern(QStringLiteral(R"([\p{L}_#][\p{L}\p{N}_'#]*)"));
    auto matches = namePattern.globalMatch(formula);
    while (matches.hasNext())
    {
        deferredFormulaNames.insert(matches.next().captured());
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::DeferredFormulas() -> int
{
    return deferredFormulas;
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::DeferredFormulaNames() -> QStringList
{
    QStringList names = deferredFormulaNames.values();
    names.sort();
    return names;
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::SetParseInProgress(bool value)
{
    parseInProgress = value;
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::IsParseInProgress() -> bool
{
    return parseInProgress;
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::ResetDeferredFormulas()
{
    deferredFormulas = 0;
    deferredFormulaNames.clear();
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::PrepareSecondParsePass(VContainer *data)
{
    if (data == nullptr)
    {
        return;
    }

    // Same list as a LiteParse: the variables recomputed from geometry. They are cleared so the second pass
    // registers them once, under their own names, instead of colliding with the first pass and being suffixed.
    Q_STATIC_ASSERT_X(static_cast<int>(VarType::Unknown) == 12, "Check that you used all types");
    const QVector<VarType> types{VarType::LineAngle,         VarType::LineLength,       VarType::CurveLength,
                                 VarType::CurveCLength,      VarType::ArcRadius,        VarType::CurveAngle,
                                 VarType::PieceExternalArea, VarType::PieceSeamLineArea};

    // The variables a deferred formula was waiting for are the exception: they belong to a block read after it,
    // so clearing them would put the second pass back where the first one was. Keep them from the first pass,
    // where every block had been read and their value is final.
    //
    // Kept for the second pass only: when that pass reaches the block that defines them, it inserts its own
    // instance over this one. What would not be corrected is a name kept here that the second pass never defines
    // again — the names come from a regular expression over the formula, so they include function names and
    // constants — and that name happened to be a variable of a type the pass recomputes. Re-reading the value
    // from the container rather than trusting this table is what keeps the rest honest.
    QHash<QString, QSharedPointer<VInternalVariable>> kept;
    const auto *vars = data->DataVariables();
    if (vars != nullptr)
    {
        for (const QString &name : std::as_const(deferredFormulaNames))
        {
            if (auto it = vars->constFind(name); it != vars->constEnd())
            {
                kept.insert(name, it.value());
            }
        }
    }

    data->ClearVariables(types);

    for (auto it = kept.constBegin(); it != kept.constEnd(); ++it)
    {
        data->d->variables->insert(it.key(), it.value());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::DetachVariables()
{
    if (d->variables.isNull())
    {
        return;
    }

    // The values are shared pointers to variables, as in any copy of the hash; only the table itself is
    // duplicated, so the caller can erase from it without touching anyone else's.
    d->variables = QSharedPointer<QHash<QString, QSharedPointer<VInternalVariable>>>::create(*d->variables);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AddLine add line to container
 * @param firstPointId id of first point of line
 * @param secondPointId id of second point of line
 * @param mainReference tool the line belongs to
 * @param takeRoleName false for a transformed line built by an operation — see the declaration
 */
void VContainer::AddLine(quint32 firstPointId, quint32 secondPointId, quint32 mainReference, bool takeRoleName)
{
    const QSharedPointer<VPointF> first = GeometricObject<VPointF>(firstPointId);
    const QSharedPointer<VPointF> second = GeometricObject<VPointF>(secondPointId);

    auto *lengthVar =
        new VLengthLine(first.data(), firstPointId, second.data(), secondPointId, *GetPatternUnit(), mainReference);
    auto *angleVar = new VLineAngle(first.data(), firstPointId, second.data(), secondPointId, mainReference);

    const QString pairKey = first->name() + '_' + second->name();
    if (takeRoleName && d->lineRoleNames.contains(pairKey))
    {
        // The pair has a bearer: the role is not a name left by a line this pattern no longer has. The parse
        // reads this set at its end and drops the names no bearer took (PruneOrphanLineRoleNames(true)).
        d->borneLineRolePairs.insert(pairKey);

        const QString &suffix = d->lineRoleNames[pairKey];
        // Register the derived aliases in uniqueNames so that IsUnique() can detect
        // duplicate RoleNames across different lines (same mechanism as curves/increments).
        uniqueNames[d->nspace].insert(line_ + suffix);
        uniqueNames[d->nspace].insert(angleLine_ + suffix);

        lengthVar->SetAlias(line_ + suffix);       // Line_CarrureDevant
        angleVar->SetAlias(angleLine_ + suffix);   // AngleLine_CarrureDevant
    }

    AddVariable(lengthVar);
    AddVariable(angleVar);
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::IsUnnamedObject(const VContainer *container, const QSharedPointer<VInternalVariable> &var) -> bool
{
    if (container == nullptr || var.isNull())
    {
        return false;
    }

    // The two values a tool hands over under a name of its own, and never a name the user was asked to give:
    // `CurrentLength` is read in a piece's formulas as it stands, and refusing it would take away a value the
    // user can neither rename nor replace. They are therefore named, whatever family they are computed in.
    const QString &technical = var->GetName();
    if (technical == currentLength || technical == currentSeamAllowance)
    {
        return false;
    }

    switch (var->GetType())
    {
        case VarType::LineLength:
        case VarType::LineAngle:
        {
            // A line answers to the two points it joins, and the name the user gave it lives in the role-name
            // table, under that very pair — never on the variable. A line nobody named has nothing to say:
            // `Line_C7_C8` is the points, a name that travels with them, whatever the line is. So only a name
            // makes a line named.
            const QString &prefix = var->GetType() == VarType::LineLength ? line_ : angleLine_;
            return container->LineRoleName(technical.mid(prefix.length())).isEmpty();
        }

        case VarType::CurveLength:
        case VarType::CurveCLength:
        case VarType::CurveAngle:
        case VarType::ArcRadius:
            // A curve, and everything read on it, carry the alias of the object itself — the one typed in the
            // Options dock — and it is empty for a curve nobody named. A segment of a path is named with the
            // path that carries it: it is registered under a route built from points, and a route is not an
            // alias, so it lands here as unnamed, which is what it is.
            return var->GetAlias().isEmpty();

        default:
            return false; // measurements, increments, areas of a piece: they have never had a name to give
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::UnnamedObjectInFormula(const VContainer *container, const QString &formula) -> QString
{
    if (container == nullptr || formula.isEmpty())
    {
        return QString();
    }

    const QHash<QString, QSharedPointer<VInternalVariable>> *const vars = container->DataVariables();
    if (vars == nullptr)
    {
        return QString();
    }

    // Same token shape as DeferFormula: what an identifier can hold in a formula, `#Tm` included. A name that
    // resolves to nothing is none of this function's business — the calculator reports it on its own.
    static const QRegularExpression namePattern(QStringLiteral(R"([\p{L}_#][\p{L}\p{N}_'#]*)"));
    auto matches = namePattern.globalMatch(formula);
    while (matches.hasNext())
    {
        const QString token = matches.next().captured();
        if (not vars->contains(token))
        {
            continue; // not a variable of this pattern: a function, a number, a name the parser will report
        }

        const QSharedPointer<VInternalVariable> var = vars->value(token);
        if (var.isNull())
        {
            continue; // nothing to judge
        }

        // The two values a tool hands over under a name of its own are not names anybody gave, and are never
        // refused, whatever family they are computed in. `CurrentLength` *is* a VLengthLine while a cut is being
        // edited, and a line is refused for being written by its pair instead of its name: without this, the value
        // a tool lends would be refused like a point-based name, and `CurrentLength/2` could not even be created.
        const QString &technical = var->GetName();
        if (technical == currentLength || technical == currentSeamAllowance)
        {
            continue;
        }

        // The list's question, asked of the object: is there a name here at all? One definition, read by the
        // assistant's list and by this guard alike, so the two never disagree about what is named.
        if (IsUnnamedObject(container, var))
        {
            return token;
        }

        // A line is the one family where being named is not enough. The pair of points it joins resolves like
        // any other name, and it stays refused: the name the user gave is the only handle a formula may use.
        // A curve has nothing of the sort — its technical name is its own construction, not the points of the
        // pattern — so a named curve is accepted under either name.
        if ((var->GetType() == VarType::LineLength || var->GetType() == VarType::LineAngle)
            && token != var->GetAlias())
        {
            return token;
        }
    }

    return QString();
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::FormulaWithoutStaleHandles(const VContainer *container, const QString &formula) -> QString
{
    if (container == nullptr || formula.isEmpty())
    {
        return formula;
    }

    const QHash<QString, QSharedPointer<VInternalVariable>> *const vars = container->DataVariables();
    if (vars == nullptr)
    {
        return formula;
    }

    return VFormula::MapTokens(formula,
                               [vars](const QString &token) -> QString
                               {
                                   const QSharedPointer<VInternalVariable> var = vars->value(token);
                                   if (var.isNull())
                                   {
                                       // Not a variable of this pattern: a function, a number, a name the parser
                                       // reports on its own.
                                       return token;
                                   }

                                   // The two values a tool hands over under a name of its own are not names the
                                   // user gave, and are never rewritten. They are lent the alias of whatever they are
                                   // computed on — a line, a curve, an arc — so rewriting `CurrentLength` would put the
                                   // name of that object in the middle of a formula that asks for a length.
                                   const QString &technical = var->GetName();
                                   if (technical == currentLength || technical == currentSeamAllowance)
                                   {
                                       return token;
                                   }

                                   // The name of the object this token resolves to, and nothing else, is what a
                                   // formula may write. An object nobody named has no name to offer here: that one
                                   // is the guard's business, which reports it and asks for one.
                                   const QString &name = var->GetAlias();
                                   return name.isEmpty() ? token : name;
                               });
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief The two names a key joins, recovered against the objects the pattern holds.
 *
 * The key is `P1_P2`, and a name may contain `_` — one the reader typed, and every name the import of a piece
 * marks with `__P<n>`. Splitting is therefore not a decoding: the cut that puts a **known object name** on each
 * side is. Exactly one cut has to be right; when none or several are, nothing is invented and the pair stays
 * unknown — the pattern then writes the key the way it always did.
 */
namespace
{
auto RecoveredPair(const QHash<quint32, QSharedPointer<VGObject>> &objects, const QString &pairKey)
    -> QPair<QString, QString>
{
    QSet<QString> known;
    known.reserve(objects.size());
    for (const QSharedPointer<VGObject> &object : objects)
    {
        if (not object.isNull())
        {
            known.insert(object->name());
        }
    }

    QPair<QString, QString> found{};
    int cuts = 0;
    for (qsizetype cut = pairKey.indexOf(QLatin1Char('_')); cut > 0;
         cut = pairKey.indexOf(QLatin1Char('_'), cut + 1))
    {
        const QString first = pairKey.left(cut);
        const QString second = pairKey.mid(cut + 1);
        if (known.contains(first) && known.contains(second))
        {
            found = {first, second};
            ++cuts;
        }
    }

    return cuts == 1 ? found : QPair<QString, QString>{};
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void VContainer::SetLineRoleName(const QString &pairKey, const QString &suffix)
{
    if (suffix.isEmpty())
    {
        d->lineRoleNames.remove(pairKey);
        d->lineRolePairs.remove(pairKey);
        return;
    }

    // Reject a suffix already in use by a different pairKey.
    for (auto i = d->lineRoleNames.constBegin(); i != d->lineRoleNames.constEnd(); ++i)
    {
        if (i.key() != pairKey && i.value() == suffix)
        {
            return; // duplicate role name — silently ignored
        }
    }

    d->lineRoleNames.insert(pairKey, suffix);

    // The pair, when the objects of the pattern can tell it: this form of the call carries the key alone.
    if (const QPair<QString, QString> pair = RecoveredPair(d->calculationObjects, pairKey); not pair.first.isEmpty())
    {
        d->lineRolePairs.insert(pairKey, pair);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::SetLineRoleName(const QString &first, const QString &second, const QString &suffix)
{
    if (first.isEmpty() || second.isEmpty())
    {
        return;
    }

    const QString pairKey = first + '_' + second;
    SetLineRoleName(pairKey, suffix);

    if (d->lineRoleNames.contains(pairKey))
    {
        // The caller just said which two names these are — there is nothing to recover, and nothing to be wrong
        // about: a name may hold an underscore, and the key alone would then be undecidable.
        d->lineRolePairs.insert(pairKey, {first, second});
    }
    else
    {
        d->lineRolePairs.remove(pairKey); // refused for a duplicate role name: nothing was filed
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::RemoveLineRoleName(const QString &pairKey)
{
    d->lineRoleNames.remove(pairKey);
    d->lineRolePairs.remove(pairKey);
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::ClearLineRoleNames()
{
    d->lineRoleNames.clear();
    d->lineRolePairs.clear();
    d->borneLineRolePairs.clear();
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::LineRoleName(const QString &pairKey) const -> QString
{
    return d->lineRoleNames.value(pairKey);
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::LineRoleNames() const -> const QHash<QString, QString> &
{
    return d->lineRoleNames;
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::LineRolePair(const QString &pairKey) const -> QPair<QString, QString>
{
    return d->lineRolePairs.value(pairKey);
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::BorneLineRolePairs() const -> const QSet<QString> &
{
    return d->borneLineRolePairs;
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::IsLineRoleNameTaken(const QString &suffix, const QString &pairKeyToIgnore) const -> bool
{
    if (suffix.isEmpty())
    {
        return false;
    }

    for (auto i = d->lineRoleNames.constBegin(); i != d->lineRoleNames.constEnd(); ++i)
    {
        if (i.key() != pairKeyToIgnore && i.value() == suffix)
        {
            return true;
        }
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::PruneOrphanLineRoleNames(bool requireBearer) -> int
{
    QSet<QString> known;
    known.reserve(d->calculationObjects.size()
                  + (d->modelingObjects.isNull() ? 0 : static_cast<qsizetype>(d->modelingObjects->size())));

    const auto Collect = [&known](const QHash<quint32, QSharedPointer<VGObject>> &objects)
    {
        for (const QSharedPointer<VGObject> &object : objects)
        {
            if (not object.isNull())
            {
                known.insert(object->name());
            }
        }
    };

    Collect(d->calculationObjects);
    if (not d->modelingObjects.isNull())
    {
        Collect(*d->modelingObjects);
    }

    int dropped = 0;
    const QStringList pairKeys = d->lineRoleNames.keys();
    for (const QString &pairKey : pairKeys)
    {
        // A name no tool has taken is a name nobody can give to a line, even when both its points still exist:
        // the line was deleted, the points stayed, and the name would be offered forever. Judged first, because
        // it is the one question that does not need the pair to be readable.
        if (requireBearer && not d->borneLineRolePairs.contains(pairKey))
        {
            d->lineRoleNames.remove(pairKey);
            d->lineRolePairs.remove(pairKey);
            ++dropped;
            continue;
        }

        // Only a pair the container remembers as two names is judged: the key alone does not say where the
        // second name begins when a name holds `_`, and a pair nobody can read is kept, not guessed away.
        const QPair<QString, QString> pair = d->lineRolePairs.value(pairKey);
        if (pair.first.isEmpty())
        {
            continue;
        }

        if (not known.contains(pair.first) || not known.contains(pair.second))
        {
            d->lineRoleNames.remove(pairKey);
            d->lineRolePairs.remove(pairKey);
            ++dropped;
        }
    }

    return dropped;
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::AddArc(const QSharedPointer<VAbstractCurve> &arc, const quint32 &id, const quint32 &parentId)
{
    AddCurve(arc, id, parentId);

    if (arc->getType() == GOType::Arc)
    {
        const QSharedPointer<VArc> casted = arc.staticCast<VArc>();

        AddVariable(new VArcRadius(id, parentId, casted.data(), *GetPatternUnit()));
    }
    else if (arc->getType() == GOType::EllipticalArc)
    {
        const QSharedPointer<VEllipticalArc> casted = arc.staticCast<VEllipticalArc>();

        const QSharedPointer<VArcRadius> first(new VArcRadius(id, parentId, casted.data(), 1, *GetPatternUnit()));
        const QSharedPointer<VArcRadius> second(new VArcRadius(id, parentId, casted.data(), 2, *GetPatternUnit()));

        AddVariable(first);
        AddVariable(second);
        AddVariable(new VEllipticalArcRotation(id, parentId, casted.data()));

        // The two radii used to answer to one unnumbered alias — `Radius_ElArc_<suffix>` — and a formula
        // written then still writes it. It keeps resolving, as a route that lands on the second radius: that
        // is what the unnumbered key held, the two having overwritten each other in registration order. The
        // change left the file's own meaning alone and only made the two names tell each other apart.
        if (not casted->GetAlias().isEmpty())
        {
            AddVariable(second, second->GetName(), radius_V + casted->GetAlias());
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::AddCurve(const QSharedPointer<VAbstractCurve> &curve, const quint32 &id, quint32 parentId)
{
    const GOType curveType = curve->getType();
    if (curveType != GOType::Spline && curveType != GOType::SplinePath && curveType != GOType::CubicBezier &&
        curveType != GOType::CubicBezierPath && curveType != GOType::Arc && curveType != GOType::EllipticalArc)
    {
        throw VException(tr("Can't create a curve with type '%1'").arg(static_cast<int>(curveType)));
    }

    AddVariable(new VCurveLength(id, parentId, curve.data(), *GetPatternUnit()));
    AddVariable(new VCurveAngle(id, parentId, curve.data(), CurveAngle::StartAngle));
    AddVariable(new VCurveAngle(id, parentId, curve.data(), CurveAngle::EndAngle));
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::AddSpline(const QSharedPointer<VAbstractBezier> &curve, quint32 id, quint32 parentId)
{
    AddCurve(curve, id, parentId);
    AddVariable(new VCurveCLength(id, parentId, curve.data(), CurveCLength::C1, *GetPatternUnit()));
    AddVariable(new VCurveCLength(id, parentId, curve.data(), CurveCLength::C2, *GetPatternUnit()));
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::AddCurveWithSegments(const QSharedPointer<VAbstractCubicBezierPath> &curve, const quint32 &id,
                                      quint32 parentId)
{
    AddSpline(curve, id, parentId);

    for (qint32 i = 1; i <= curve->CountSubSpl(); ++i)
    {
        const VSpline spl = curve->GetSpline(i);
        const QString ptSuffix = spl.GetP1().name() + '_' + spl.GetP4().name();

        // Helper: register variable + point-based simplified KEY (RoleName step 3)
        //
        // The key is a ROUTE, not a name: `Curve_A_B` resolves the segment, exactly as it always has, so a
        // formula or a .val that writes it keeps working. It is deliberately NOT set as the variable's alias
        // any more — an alias is a name the user gave, and this one is built from two point names. It used to
        // be set so that the formula assistant showed `Curve_A_B` instead of `Curve_A_C_Seg_1`; that put a
        // point-name under a name-shaped label, which is precisely what a user may not reference. The
        // assistant now shows the variable's technical name, and nothing that looks like a user name comes
        // out of the geometry.
        auto addWithPointKey = [&](const QSharedPointer<VInternalVariable> &var, const QString &simplifiedPrefix) {
            QString baseKey = simplifiedPrefix + ptSuffix;
            QString key = baseKey;
            int n = 1;
            while (d->variables->contains(key))
            {
                key = baseKey + '_' + QString::number(n++);
            }
            AddVariable(var, var->GetName(), key);
            uniqueNames[d->nspace].insert(key);
        };

        addWithPointKey(
            QSharedPointer<VCurveLength>::create(id, parentId, curve.data(), spl, *GetPatternUnit(), i), curve_);
        addWithPointKey(
            QSharedPointer<VCurveAngle>::create(id, parentId, curve.data(), spl, CurveAngle::StartAngle, i),
            angle1Curve_);
        addWithPointKey(
            QSharedPointer<VCurveAngle>::create(id, parentId, curve.data(), spl, CurveAngle::EndAngle, i),
            angle2Curve_);
        addWithPointKey(QSharedPointer<VCurveCLength>::create(id, parentId, curve.data(), spl, CurveCLength::C1,
                                                               *GetPatternUnit(), i),
                        handle1Curve_);
        addWithPointKey(QSharedPointer<VCurveCLength>::create(id, parentId, curve.data(), spl, CurveCLength::C2,
                                                               *GetPatternUnit(), i),
                        handle2Curve_);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::RemoveVariable(const QString &name)
{
    d->variables->remove(name);
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::TransferGroupOffsetToMembers(quint32 groupId)
{
    if (!d->pieceGroups->contains(groupId))
    {
        return;
    }
    const OPieceGroup &group = d->pieceGroups->value(groupId);

    bool changed = false;
    QHash<QString, OSizerange> ranges = *d->sizeRanges;
    for (auto it = ranges.begin(); it != ranges.end(); ++it)
    {
        const auto gc = it->groupContent.constFind(groupId);
        if (gc == it->groupContent.cend())
        {
            continue;
        }
        const qreal dx = gc->dx;
        const qreal dy = gc->dy;
        for (quint32 memberId : group.pieceIds)
        {
            if (d->pieces->contains(memberId))
            {
                OSizerange::PieceContent &pc = it->pieceContent[memberId];
                pc.dx += dx;
                pc.dy += dy;
            }
        }
        it->groupContent.remove(groupId);
        changed = true;
    }
    if (changed)
    {
        SetSizeRanges(ranges);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::RemovePiece(quint32 id)
{
    d->pieces->remove(id);

    // A deleted member must leave its group(s) — a dangling id in a group's
    // pieceIds would make later group rendering throw VExceptionBadId.
    for (auto it = d->pieceGroups->begin(); it != d->pieceGroups->end();)
    {
        it.value().pieceIds.removeAll(id);
        it.value().memberOffsets.remove(id);
        if (it.value().pieceIds.size() < 2)
        {
            // Keep the graded move of the dissolving group: spread its offset onto the
            // surviving members before the group is erased.
            TransferGroupOffsetToMembers(it.key());
            it = d->pieceGroups->erase(it);  // a group needs at least 2 members
        }
        else
        {
            ++it;
        }
    }

    // Graded ranges reference pieces/groups by id — drop the deleted piece from
    // every stored range, plus any group that the cascade just erased.
    for (auto it = d->sizeRanges->begin(); it != d->sizeRanges->end(); ++it)
    {
        it->pieceContent.remove(id);
        for (auto g = it->groupContent.begin(); g != it->groupContent.end();)
        {
            if (!d->pieceGroups->contains(g.key()))
            {
                g = it->groupContent.erase(g);
            }
            else
            {
                ++g;
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::RemovePieceGroup(quint32 id)
{
    // Dissolving a group keeps its graded move: spread the group offset onto the members
    // before the group (and its stored groupContent) is erased.
    TransferGroupOffsetToMembers(id);

    d->pieceGroups->remove(id);

    for (auto it = d->sizeRanges->begin(); it != d->sizeRanges->end(); ++it)
    {
        it->groupContent.remove(id);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::SizeRanges() const -> const QHash<QString, OSizerange> *
{
    return d->sizeRanges.data();
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::SetSizeRanges(const QHash<QString, OSizerange> &ranges)
{
    // Mutate in place, like piece groups: sizeRanges is a QSharedPointer<QHash> shared across
    // every VContainer copy (tool data snapshots). Replacing the pointer here would make the new
    // hash visible only to this container — the next VPattern::setCurrentData() (`*data =
    // vTool->GetDataCopy()`) would restore an older snapshot and the ranges would vanish.
    *d->sizeRanges = ranges;
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::UpdatePiece(quint32 id, const VPiece &detail)
{
    Q_ASSERT_X(id != NULL_ID, Q_FUNC_INFO, "id == 0"); //-V654 //-V712
    d->pieces->insert(id, detail);
    UpdateId(id);
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::UpdatePieceGroup(quint32 id, const OPieceGroup &group)
{
    Q_ASSERT_X(id != NULL_ID, Q_FUNC_INFO, "id == 0"); //-V654 //-V712
    d->pieceGroups->insert(id, group);
    UpdateId(id);
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::UpdatePiecePath(quint32 id, const VPiecePath &path)
{
    Q_ASSERT_X(id != NULL_ID, Q_FUNC_INFO, "id == 0"); //-V654 //-V712
    d->piecePaths->insert(id, path);
    UpdateId(id);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief RemoveIncrement remove increment by name from increment table
 * @param name name of existing increment
 */
void VContainer::RemoveIncrement(const QString &name)
{
    (*d->variables)[name].clear();
    d->variables->remove(name);
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::FillPiecesAreas(Unit unit)
{
    QHash<quint32, VPiece>  const*pieces = d->pieces.data();

    auto i = pieces->constBegin();
    while (i != pieces->constEnd())
    {
        AddVariable(QSharedPointer<VPieceArea>::create(PieceAreaType::External, i.key(), i.value(), this, unit));
        AddVariable(QSharedPointer<VPieceArea>::create(PieceAreaType::SeamLine, i.key(), i.value(), this, unit));
        ++i;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::DataMeasurements() const -> QMap<QString, QSharedPointer<VMeasurement>>
{
    return DataVar<VMeasurement>(VarType::Measurement);
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::DataMeasurementsWithSeparators() const -> QMap<QString, QSharedPointer<VMeasurement>>
{
    QMap<QString, QSharedPointer<VMeasurement>> measurements = DataVar<VMeasurement>(VarType::Measurement);
    QMap<QString, QSharedPointer<VMeasurement>> const separators = DataVar<VMeasurement>(VarType::MeasurementSeparator);

    measurements.insert(separators);

    return measurements;
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::DataIncrements() const -> QMap<QString, QSharedPointer<VIncrement>>
{
    return DataVar<VIncrement>(VarType::Increment);
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::DataIncrementsWithSeparators() const -> QMap<QString, QSharedPointer<VIncrement>>
{
    QMap<QString, QSharedPointer<VIncrement>> increments = DataVar<VIncrement>(VarType::Increment);
    QMap<QString, QSharedPointer<VIncrement>> const separators = DataVar<VIncrement>(VarType::IncrementSeparator);

    increments.insert(separators);

    return increments;
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::DataLengthLines() const -> QMap<QString, QSharedPointer<VLengthLine>>
{
    return DataVar<VLengthLine>(VarType::LineLength);
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::DataLengthCurves() const -> QMap<QString, QSharedPointer<VCurveLength>>
{
    return DataVar<VCurveLength>(VarType::CurveLength);
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::DataCurvesCLength() const -> QMap<QString, QSharedPointer<VCurveCLength>>
{
    return DataVar<VCurveCLength>(VarType::CurveCLength);
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::DataAngleLines() const -> QMap<QString, QSharedPointer<VLineAngle>>
{
    return DataVar<VLineAngle>(VarType::LineAngle);
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::DataRadiusesArcs() const -> QMap<QString, QSharedPointer<VArcRadius>>
{
    return DataVar<VArcRadius>(VarType::ArcRadius);
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::DataAnglesCurves() const -> QMap<QString, QSharedPointer<VCurveAngle>>
{
    return DataVar<VCurveAngle>(VarType::CurveAngle);
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::DataPieceArea() const -> QMap<QString, QSharedPointer<VPieceArea>>
{
    QMap<QString, QSharedPointer<VPieceArea>> externalAreas = DataVar<VPieceArea>(VarType::PieceExternalArea);
    QMap<QString, QSharedPointer<VPieceArea>> const seamLineAreas = DataVar<VPieceArea>(VarType::PieceSeamLineArea);

    externalAreas.insert(seamLineAreas);

    return externalAreas;
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::DataDependencyVariables() const -> QHash<QString, QList<quint32>>
{
    Q_STATIC_ASSERT_X(static_cast<int>(VarType::Unknown) == 12, "Check that you used all types");
    QVector<VarType> const types{VarType::LineAngle,
                           VarType::LineLength,
                           VarType::CurveLength,
                           VarType::CurveCLength,
                           VarType::ArcRadius,
                           VarType::CurveAngle};

    QHash<QString, QList<quint32>> varData;

    for (const auto &var : *d->variables)
    {
        // `CurrentLength` et `CurrentSeamAllowance` ne nomment **aucun** objet : ce sont les variables du **contexte**
        // d'un outil — « la courbe sur laquelle je me construis » — et le conteneur n'en garde qu'une, la dernière
        // enregistrée. En faire une arête du graphe relie donc deux pièces **indépendantes** : mesuré le 13-10-2026
        // sur deux imports de la même pièce de bibliothèque, la spline `dos` du premier recevait une arête vers
        // quatorze points du second, qui devenait « dépendant » du premier — sa suppression refusée, alors que leurs
        // fichiers ne partagent pas un seul id.
        const QString name = var->GetName();
        const QString alias = var->GetAlias();
        const auto isContext = [](const QString &key) { return key == currentLength || key == currentSeamAllowance; };
        if (isContext(name) || isContext(alias))
        {
            continue;
        }

        if (types.contains(var->GetType()))
        {
            auto const references = var->GetReferences();
            varData.insert(name, references);

            if (!alias.isEmpty())
            {
                varData.insert(alias, references);
            }
        }
    }

    return varData;
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::IsUnique(const QString &name) const -> bool
{
    return VContainer::IsUnique(name, d->nspace);
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::IsUnique(const QString &name, const QString &nspace) -> bool
{
    if (uniqueNames.contains(nspace))
    {
        return (!uniqueNames.value(nspace).contains(name) && !BuilInFunctions().contains(name));
    }

    throw VException(QStringLiteral("Unknown namespace"));
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::UniqueDuplicate(const QString &baseName) const -> quint32
{
    quint32 number = 0;
    QString name;
    do
    {
        ++number;
        name = baseName + QLatin1Char('_') + QString::number(number);
    } while (not IsUnique(name));

    return number;
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::AllUniqueNames() const -> QStringList
{
    return AllUniqueNames(d->nspace);
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::AllUniqueNames(const QString &nspace) -> QStringList
{
    if (uniqueNames.contains(nspace))
    {
        QStringList names = BuilInFunctions();
        names.append(uniqueNames.value(nspace).values());
        return names;
    }

    throw VException(QStringLiteral("Unknown namespace"));
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::GetPatternUnit() const -> const Unit *
{
    return d->patternUnit;
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::GetTrVars() const -> const VTranslateVars *
{
    return d->trVars;
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T> auto VContainer::DataVar(const VarType &type) const -> QMap<QString, QSharedPointer<T>>
{
    QMap<QString, QSharedPointer<T>> map;
    // Sorting QHash by id
    for (auto i = d->variables->constBegin(); i != d->variables->constEnd(); ++i)
    {
        if (i.value()->GetType() == type)
        {
            QSharedPointer<T> const var = GetVariable<T>(i.key());
            map.insert(d->trVars->VarToUser(i.key()), var);
        }
    }
    return map;
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::ClearUniqueNames() const
{
    uniqueNames[d->nspace].clear();
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::ClearUniqueIncrementNames() const
{
    const QList<QString> list = uniqueNames.value(d->nspace).values();
    ClearUniqueNames();

    for (const auto &name : list)
    {
        if (not name.startsWith('#'))
        {
            uniqueNames[d->nspace].insert(name);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::ClearExceptUniqueIncrementNames() const
{
    const QList<QString> list = uniqueNames.value(d->nspace).values();
    ClearUniqueNames();

    for (const auto &name : list)
    {
        if (name.startsWith('#'))
        {
            uniqueNames[d->nspace].insert(name);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VContainer::RemoveUniqueName(const QString &name) const
{
    uniqueNames[d->nspace].remove(name);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief data container with datagObjects return container of gObjects
 * @return pointer on container of gObjects
 */
auto VContainer::CalculationGObjects() const -> const QHash<quint32, QSharedPointer<VGObject>> *
{
    return &d->calculationObjects;
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::DataPieces() const -> QHash<quint32, VPiece> *
{
    return d->pieces.data();
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::DataPieceGroups() const -> QHash<quint32, OPieceGroup> *
{
    return d->pieceGroups.data();
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::DataPieceGroups() -> QHash<quint32, OPieceGroup> *
{
    return d->pieceGroups.data();
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::GetPieceGroupId(quint32 pieceId) const -> quint32
{
    for (auto it = d->pieceGroups->cbegin(); it != d->pieceGroups->cend(); ++it)
    {
        if (it.value().pieceIds.contains(pieceId))
        {
            return it.key();
        }
    }
    return NULL_ID;
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::IsPieceInLayout(quint32 pieceId) const -> bool
{
    // A member piece's in-layout state follows its GROUP (a group behaves like one piece). This is
    // the single definition of "exportable": the export filter and the piece total both use it, so
    // the total shown in a cartouche always matches what an export contains.
    const quint32 groupId = GetPieceGroupId(pieceId);
    if (groupId != NULL_ID)
    {
        if (const auto *groups = DataPieceGroups(); groups != nullptr && groups->contains(groupId))
        {
            return groups->value(groupId).IsInLayout();
        }
    }

    const auto *pieces = DataPieces();
    return pieces != nullptr && pieces->contains(pieceId) && pieces->value(pieceId).IsInLayout();
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::GetEffectivePieceCount() const -> int
{
    // Only the units that go to the layout count: an excluded piece or group is not part of the
    // total. « Un groupe = 1 pièce » : a group of two or more members counts for 1, its members do
    // not count on their own.
    const auto *groups = DataPieceGroups();
    QSet<quint32> memberIds;
    int count = 0;

    if (groups != nullptr)
    {
        for (auto it = groups->cbegin(); it != groups->cend(); ++it)
        {
            if (it.value().pieceIds.size() < 2)
            {
                continue;
            }
            for (quint32 pid : it.value().pieceIds)
            {
                memberIds.insert(pid);
            }
            if (it.value().IsInLayout())
            {
                count++;
            }
        }
    }

    if (const auto *pieces = DataPieces(); pieces != nullptr)
    {
        for (auto it = pieces->cbegin(); it != pieces->cend(); ++it)
        {
            if (!memberIds.contains(it.key()) && IsPieceInLayout(it.key()))
            {
                count++;
            }
        }
    }

    return count;
}

//---------------------------------------------------------------------------------------------------------------------
auto VContainer::DataVariables() const -> const QHash<QString, QSharedPointer<VInternalVariable>> *
{
    return d->variables.data();
}

//---------------------------------------------------------------------------------------------------------------------
VContainerData::VContainerData(const VTranslateVars *trVars, const Unit *patternUnit, QString nspace)
  : trVars(trVars),
    patternUnit(patternUnit),
    nspace(std::move(nspace))
{
}

//---------------------------------------------------------------------------------------------------------------------
VContainerData::~VContainerData()
{
    if (ref.loadRelaxed() == 0)
    {
        --VContainer::copyCounter[nspace];
    }

    if (VContainer::copyCounter.value(nspace) == 0)
    {
        VContainer::ClearNamespace(nspace);
    }
}
