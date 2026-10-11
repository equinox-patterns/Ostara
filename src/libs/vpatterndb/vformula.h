/************************************************************************
 **
 **  @file   vformula.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   28 8, 2014
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

#ifndef VFORMULA_H
#define VFORMULA_H

#include <QCoreApplication>
#include <QMetaType>
#include <QSharedDataPointer>
#include <QString>
#include <QTypeInfo>
#include <QtGlobal>
#include <functional>

enum class FormulaType : qint8
{
    ToUser = 0,
    FromUser = 1,
    ToSystem = FromUser,
    FromSystem = ToUser
};

class VContainer;
class VFormulaData;

class VFormula
{
    Q_DECLARE_TR_FUNCTIONS(VFormula) // NOLINT

public:
    VFormula();
    explicit VFormula(const VContainer *container);
    VFormula(const QString &formula, const VContainer *container);
    auto operator=(const VFormula &formula) -> VFormula &;
    VFormula(const VFormula &formula);
    VFormula(VFormula &&formula) noexcept;
    auto operator=(VFormula &&formula) noexcept -> VFormula &;
    ~VFormula();

    auto operator==(const VFormula &formula) const -> bool;

    auto GetFormula(FormulaType type = FormulaType::ToUser) const -> QString;
    void SetFormula(const QString &value, FormulaType type = FormulaType::FromSystem);

    auto getStringValue() const -> QString;
    auto getDoubleValue() const -> qreal;

    auto getCheckZero() const -> bool;
    void setCheckZero(bool value);

    auto getCheckLessThanZero() const -> bool;
    void setCheckLessThanZero(bool value);

    auto getData() const -> const VContainer *;
    void setData(const VContainer *value);

    auto getToolId() const -> quint32;
    void setToolId(quint32 value);

    auto getPostfix() const -> QString;
    void setPostfix(const QString &value);

    auto error() const -> bool;
    auto Reason() const -> QString;

    /** @brief Whether every object the formula names was given a name by the user.
     *
     * Ostara reads an object by its points — `Line_C7_C8`, `Spl_A_D` — so a formula can be written without
     * anyone naming anything, and it computes. What it cannot do is mean something: the name travels with the
     * points. The formula editors of the Options dock ask this before handing a formula to a tool, which is
     * where a formula is *written*; the path that *reads* a file never asks, so a pattern written before the
     * rule keeps opening.
     *
     * On failure the reason is the sentence to show, and it is put where `Reason()` already answers — the dock
     * shows a refused formula the same way it shows one the parser cannot read.
     *
     * @return true when the formula may be kept. */
    auto CheckObjectNames() -> bool;

    /** @brief Apply @a map to every token of @a formula, and return the formula it gives.
     *
     *  A token is what the calculator reads as one name — `#Cou`, `E'` and `Line_A10_E'` whole — never a piece of
     *  one: a name that happens to contain another one is left alone. Positions are read once and replaced from
     *  the right, so a mapping that changes a length cannot move what follows it.
     *
     *  A formula the parser cannot read comes back untouched: reporting it belongs to the calculator, which is
     *  the only place that knows what is wrong with it.
     *
     *  @return @a formula itself when nothing changed. */
    static auto MapTokens(const QString &formula, const std::function<QString(const QString &)> &map) -> QString;

    static auto FormulaTypeId() -> int;

    void Eval();

private:
    QSharedDataPointer<VFormulaData> d;

    void ResetState();
};
Q_DECLARE_METATYPE(VFormula)                  // NOLINT
Q_DECLARE_TYPEINFO(VFormula, Q_MOVABLE_TYPE); // NOLINT

#endif // VFORMULA_H
