/************************************************************************
 **
 **  @file   dialogarmholecurve.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/08/2026
 **
 **  @brief  Dialog for the ArmholeCurve tool — 4-point, 2-segment cubic
 **          Bezier path with auto-computed handles for armhole drafting.
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

#ifndef DIALOGARMHOLECURVE_H
#define DIALOGARMHOLECURVE_H

#include <QMetaObject>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QtGlobal>

#include "../vgeometry/vcubicbezierpath.h"
#include "../vmisc/def.h"
#include "dialogtool.h"

namespace Ui
{
class DialogArmholeCurve;
}

class DialogArmholeCurve final : public DialogTool
{
    Q_OBJECT // NOLINT

public:
    explicit DialogArmholeCurve(const VContainer *data, VAbstractPattern *doc, quint32 toolId,
                                QWidget *parent = nullptr);
    ~DialogArmholeCurve() override;

    auto GetPath() const -> VCubicBezierPath;
    void SetPath(const VCubicBezierPath &path);

    auto GetT() const -> double;
    void SetT(double value);

    void SetNotes(const QString &notes);
    auto GetNotes() const -> QString;

    void SetDefPenStyle(const QString &value);
    void SetDefColor(const QString &value);

    void CheckDependencyTreeComplete() override;

public slots:
    void ChosenObject(quint32 id, const SceneObject &type) override;
    void PointNameChanged() override;
    void ShowDialog(bool click) override;

protected:
    void ShowVisualization() override;
    void SaveData() override;
    auto IsValid() const -> bool override;

private slots:
    void ValidateAlias();

private:
    Q_DISABLE_COPY_MOVE(DialogArmholeCurve) // NOLINT
    Ui::DialogArmholeCurve *ui;

    VCubicBezierPath m_path{};

    bool flagError{false};
    bool flagAlias{true};

    QString originAliasSuffix{};

    qint32 number{0}; // number of points selected (0..4)

    QString m_oldName{};

    auto GetP1() const -> QSharedPointer<VPointF>; // armhole
    auto GetP2() const -> QSharedPointer<VPointF>; // chest/back width
    auto GetP3() const -> QSharedPointer<VPointF>; // perpendicular
    auto GetP4() const -> QSharedPointer<VPointF>; // shoulder

    void SavePath();
};

//---------------------------------------------------------------------------------------------------------------------
inline auto DialogArmholeCurve::IsValid() const -> bool
{
    return flagError && flagAlias;
}

#endif // DIALOGARMHOLECURVE_H
