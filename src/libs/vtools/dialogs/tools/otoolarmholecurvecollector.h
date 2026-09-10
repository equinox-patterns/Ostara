/************************************************************************
 **
 **  @file   otoolarmholecurvecollector.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   10/09/2026
 **
 **  @brief  Headless creation collector for the ArmholeCurve tool — gathers
 **          the clicked points and builds the 7-point path, without any UI.
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

#ifndef OTOOLARMHOLECURVECOLLECTOR_H
#define OTOOLARMHOLECURVECOLLECTOR_H

#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "../vgeometry/vcubicbezierpath.h"
#include "../vmisc/def.h"
#include "dialogtool.h"

class VPointF;
template <class T> class QSharedPointer;

/**
 * @brief The OToolArmholeCurveCollector class is a headless creation collector for the ArmholeCurve
 * tool.
 *
 * It replaces the former DialogArmholeCurve, which was never shown to the user: the click sequence
 * (armhole → chest/back width → perpendicular → shoulder) is handled here, and the tool is created
 * once the fourth point is picked — see MainWindow::ClosedArmholeCurveDialog.
 *
 * The perpendicular point may be skipped by picking the shoulder point twice: that duplicate marks
 * the third slot as "not a real point" (see OToolArmholeCurve::IsPerpendicularSkipped).
 */
class OToolArmholeCurveCollector final : public DialogTool
{
    Q_OBJECT // NOLINT

public:
    OToolArmholeCurveCollector(const VContainer *data, VAbstractPattern *doc, quint32 toolId,
                               QWidget *parent = nullptr);
    ~OToolArmholeCurveCollector() override;

    auto GetPath() const -> VCubicBezierPath;

    void SetDefPenStyle(const QString &value);
    void SetDefColor(const QString &value);

    void CheckDependencyTreeComplete() override;

public slots:
    void ChosenObject(quint32 id, const SceneObject &type) override;
    void ShowDialog(bool click) override;

protected:
    auto IsValid() const -> bool override;

private:
    Q_DISABLE_COPY_MOVE(OToolArmholeCurveCollector) // NOLINT

    /// Slot of each clicked point in m_pointIds, and the total number of clicks.
    enum PointSlot : int
    {
        Armhole = 0,
        Carrure = 1,
        Perpendicular = 2,
        Shoulder = 3,
        PointCount = 4
    };

    auto PointId(PointSlot slot) const -> quint32;
    auto PointAt(PointSlot slot) const -> QSharedPointer<VPointF>;
    auto NextToolTip() const -> QString;
    void Finalize();

    QVector<quint32> m_pointIds{};
    VCubicBezierPath m_path{};

    // Kept apart from m_path: Finalize() rebuilds the path from scratch, so the global pen style
    // and color have to survive that rebuild.
    QString m_defPenStyle{};
    QString m_defColor{};
};

#endif // OTOOLARMHOLECURVECOLLECTOR_H
