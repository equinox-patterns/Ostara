/************************************************************************
 **
 **  @file   dialognameobject.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   11/10/2026
 **
 **  @brief  Ask for a name for the objects of a formula that have none.
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
#ifndef DIALOGNAMEOBJECT_H
#define DIALOGNAMEOBJECT_H

#include <QDialog>
#include <QList>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include "../vmisc/typedef.h"

class QLabel;
class QLineEdit;
class VContainer;

/** @brief Ask, one object at a time, for a name for what a formula uses and nobody has named.
 *
 *  The dialogue answers and holds the names; it writes nothing. Applying them belongs to the caller, which is what
 *  makes a cancel mean nothing at all: no name was given to anybody, and no formula was repaired.
 *
 *  The user reads the name the formula writes — `AngleLine_A1_A3`, `Curve_Em_Efs_Seg_1` — and the field holds the
 *  part of it to replace, `A1_A3` or `Em_Efs`, so what is typed is a name and not a prefixed name.
 */
class DialogNameObject : public QDialog
{
    Q_OBJECT // NOLINT

public:
    /** One object of a formula that has no name, and what naming it means. */
    struct Object
    {
        /** The token as the formula writes it, in the notation of whoever reads it. */
        QString shown{};
        /** What the field holds: the pair of points of a line, the route of a curve. */
        QString suffix{};
        /** A line: the pair of points it joins. Empty for a curve. */
        QString pairKey{};
        /** A curve: the tool that draws it. NULL_ID for a line. */
        quint32 curveId{NULL_ID};
        /** A curve: what its own name begins with, so uniqueness is asked the way the tool asks it. */
        QString typeHead{};
    };

    DialogNameObject(QList<Object> objects, const VContainer *data, QWidget *parent = nullptr);

    /** @brief The names given, in the order the objects were asked for. Meaningful once accepted. */
    auto Names() const -> QStringList;

private:
    Q_DISABLE_COPY_MOVE(DialogNameObject) // NOLINT

    QList<Object> m_objects{};
    QStringList m_names{};
    const VContainer *m_data{nullptr};

    int m_current{0};

    QLabel *m_object{nullptr};
    QLabel *m_progress{nullptr};
    QLineEdit *m_name{nullptr};

    void ShowCurrent();
    void AcceptCurrent();
    auto IsNameFree(const Object &object, const QString &name) const -> bool;
    static auto NameKey(const Object &object, const QString &name) -> QString;
};

#endif // DIALOGNAMEOBJECT_H
