/************************************************************************
 **
 **  @file   renameobject.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   16 1, 2026
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2026 Valentina project
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
#ifndef RENAMEOBJECT_H
#define RENAMEOBJECT_H

#include "../ifc/xml/vpatterngraphnode.h"
#include "../vgeometry/vgeometrydef.h"
#include "renametoken.h"
#include "vundocommand.h"

enum class OperationType : quint8
{
    Redo,
    Undo,
    Unknown
};

class AbstractObjectRename : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    AbstractObjectRename(VAbstractPattern *doc, quint32 id, QUndoCommand *parent = nullptr);
    ~AbstractObjectRename() override = default;

    void undo() override;
    void redo() override;

    void SetFullParse(bool newFullParse);

protected:
    auto ProcessType() const -> OperationType;

    virtual auto ProcessToken(const QString &token) const -> QString = 0;
    virtual void ProcessLineNames();

    /** @brief Whether this rename repairs every formula of the file, or only the tools that depend on the object.
     *
     *  Renaming a *point* moves a geometry, and the dependency graph knows exactly which tools read it. Renaming
     *  what an object is *called* is another matter: the name is written in formulas all over the file — a piece's
     *  seam allowance, a label, a tool of another block — and no graph knows all of them. */
    virtual auto RenamesEverywhere() const -> bool;

    void ProcessElementByType(QDomElement &element) const;

private:
    Q_DISABLE_COPY_MOVE(AbstractObjectRename) // NOLINT

    QVector<VNode> m_dependencies{};
    OperationType m_operationType{OperationType::Unknown};

    bool m_fullParse{false};

    void RenameFormulas();

    void ProcessPointElement(QDomElement &element) const;
    void ProcessOperationElement(QDomElement &element) const;
    void ProcessArcElement(QDomElement &element) const;
    void ProcessElArcElement(QDomElement &element) const;
    void ProcessSplineElement(QDomElement &element) const;
    void ProcessNodeElement(QDomElement &node) const;
    void ProcessPathElement(QDomElement &element) const;
    void ProcessToolsElement(QDomElement &element) const;
    void ProcessDetailElement(QDomElement &element) const;

    void ProcessAttribute(QDomElement &element, const QString &attr) const;
    auto ProcessFormula(const QString &formula) const -> QString;
};

//---------------------------------------------------------------------------------------------------------------------
inline auto AbstractObjectRename::ProcessType() const -> OperationType
{
    return m_operationType;
}

//---------------------------------------------------------------------------------------------------------------------
inline void AbstractObjectRename::SetFullParse(bool newFullParse)
{
    m_fullParse = newFullParse;
}

//---------------------------------------------------------------------------------------------------------------------
inline auto AbstractObjectRename::RenamesEverywhere() const -> bool
{
    return false;
}

class RenameLabel : public AbstractObjectRename
{
    Q_OBJECT // NOLINT

public:
    RenameLabel(QString oldLabel, QString newLabel, VAbstractPattern *doc, quint32 id, QUndoCommand *parent = nullptr);
    ~RenameLabel() override = default;

protected:
    auto ProcessToken(const QString &token) const -> QString override;
    void ProcessLineNames() override;

private:
    Q_DISABLE_COPY_MOVE(RenameLabel) // NOLINT
    QString m_oldLabel;
    QString m_newLabel;
};

class RenamePair : public AbstractObjectRename
{
    Q_OBJECT // NOLINT

public:
    Q_REQUIRED_RESULT static auto CreateForLine(const ObjectPair_t &oldPair,
                                                const ObjectPair_t &newPair,
                                                VAbstractPattern *doc,
                                                quint32 id,
                                                QUndoCommand *parent = nullptr) -> RenamePair *;

    Q_REQUIRED_RESULT static auto CreateForSpline(const ObjectPair_t &oldPair,
                                                  const ObjectPair_t &newPair,
                                                  quint32 oldDuplicate,
                                                  quint32 newDuplicate,
                                                  VAbstractPattern *doc,
                                                  quint32 id,
                                                  QUndoCommand *parent = nullptr) -> RenamePair *;
    Q_REQUIRED_RESULT static auto CreateForSplinePath(const ObjectPair_t &oldPair,
                                                      const ObjectPair_t &newPair,
                                                      quint32 oldDuplicate,
                                                      quint32 newDuplicate,
                                                      VAbstractPattern *doc,
                                                      quint32 id,
                                                      QUndoCommand *parent = nullptr) -> RenamePair *;

    ~RenamePair() override = default;

protected:
    auto ProcessToken(const QString &token) const -> QString override;
    void ProcessLineNames() override;

private:
    Q_DISABLE_COPY_MOVE(RenamePair) // NOLINT
    RenameObjectType m_type;
    ObjectPair_t m_oldPair;
    ObjectPair_t m_newPair;
    quint32 m_oldDuplicate;
    quint32 m_newDuplicate;

    RenamePair(RenameObjectType type,
               ObjectPair_t oldPair,
               ObjectPair_t newPair,
               quint32 oldDuplicate,
               quint32 newDuplicate,
               VAbstractPattern *doc,
               quint32 id,
               QUndoCommand *parent = nullptr);
};

class RenameAlias : public AbstractObjectRename
{
    Q_OBJECT // NOLINT

public:
    /**
     * @brief Renommer l'alias d'une courbe — et réparer les formules qui l'appelaient encore par son nom composé.
     *
     * Un objet qui prend un alias peut être cité de **deux** façons : par son ancien alias (`Courbe_RepereDos`) et
     * par le nom que le patron compose sur ses points (`Courbe_R1_Eb`), qu'une formule écrite avant l'alias porte
     * toujours. Les deux formes désignent le même objet, et les deux doivent suivre — sinon l'alias posé reste sans
     * effet et la relecture qui suit remet le nom composé sous les yeux de l'utilisateur (relevé le
     * 13-10-2026 : « au lieu de forcer l'alias, ça force toujours le nom composé avec les points »).
     * `VChangeLineRoleName` fait déjà les deux pour une ligne : le rôle *et* la paire de points qu'elle joint.
     *
     * @param composedName le nom composé de l'objet quand l'appelant le connaît : les deux noms d'un segment
     *                     (`curveName1`, `curveName2`), ou celui que la courbe compose sur ses points
     *                     (`HeadlessName()`). Vide quand il n'y en a pas d'autre.
     */
    RenameAlias(CurveAliasType type,
                QString oldAlias,
                QString newAlias,
                VAbstractPattern *doc,
                quint32 id,
                QUndoCommand *parent = nullptr,
                QString composedName = {});
    ~RenameAlias() override = default;

    static auto CurveType(GOType objType) -> CurveAliasType;

protected:
    auto ProcessToken(const QString &token) const -> QString override;
    auto RenamesEverywhere() const -> bool override;

private:
    Q_DISABLE_COPY_MOVE(RenameAlias) // NOLINT
    CurveAliasType m_type;
    QString m_oldAlias;
    QString m_newAlias;
    QString m_composedName;
};

class RenameArc : public AbstractObjectRename
{
    Q_OBJECT // NOLINT

public:
    RenameArc(RenameArcType type,
              QString oldCenterLabel,
              QString newCenterLabel,
              VAbstractPattern *doc,
              quint32 id,
              QUndoCommand *parent = nullptr);
    RenameArc(RenameArcType type,
              QString oldCenterLabel,
              QString newCenterLabel,
              quint32 oldDuplicate,
              quint32 newDuplicate,
              VAbstractPattern *doc,
              quint32 id,
              QUndoCommand *parent = nullptr);
    ~RenameArc() override = default;

protected:
    auto ProcessToken(const QString &token) const -> QString override;

private:
    Q_DISABLE_COPY_MOVE(RenameArc) // NOLINT
    RenameArcType m_type;
    QString m_oldCenterLabel;
    QString m_newCenterLabel;
    quint32 m_oldDuplicate{0};
    quint32 m_newDuplicate{0};
};

class RenameSegmentCurves : public AbstractObjectRename
{
    Q_OBJECT // NOLINT

public:
    RenameSegmentCurves(CurveAliasType type,
                        QString pointName,
                        QString leftSub,
                        QString rightSub,
                        VAbstractPattern *doc,
                        QUndoCommand *parent = nullptr);
    ~RenameSegmentCurves() override = default;

    void undo() override;
    void redo() override;

protected:
    auto ProcessToken(const QString &token) const -> QString override;

private:
    Q_DISABLE_COPY_MOVE(RenameSegmentCurves) // NOLINT
    CurveAliasType m_type;
    QString m_pointName;
    QString m_leftSub;
    QString m_rightSub;
};

#endif // RENAMEOBJECT_H
