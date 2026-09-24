/************************************************************************
 **
 **  @file   oexportplotterwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  The Plotter tab of the Export dialog — the size to cut, and
 **          the mat a domestic plotter cuts it on.
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

#ifndef OEXPORTPLOTTERWIDGET_H
#define OEXPORTPLOTTERWIDGET_H

#include <QSet>
#include <QSharedPointer>
#include <QVector>
#include <QWidget>

#include "../vmisc/def.h"

class QCheckBox;
class QComboBox;
class QFormLayout;
class QGroupBox;
class QLabel;
class QLineEdit;
class QVBoxLayout;
class OExportGroupMembersWidget;
class VContainer;
class VMeasurements;

/**
 * @brief The Plotter tab: which size is cut, the mat it is cut on, and which members of a group.
 *
 * Three questions, and the second is the whole feature:
 *
 *   1. <b>which size</b> — one, never a range. A plotter cuts one pattern at a time, and the axes
 *      are the same rows the Print and Cutting tabs offer for a single size, from the same
 *      measurement table. A table without axes asks nothing: the document is on its one size.
 *   2. <b>which mat</b> — the **usable area** the user reads in her plotter's own software. Nothing
 *      could model it: mats come in as many sizes as there are machines, and the area a machine
 *      really cuts is a number only its software knows. It is entered once and remembered by the
 *      run, and it decides everything downstream — how many pieces a piece is cut into, and where.
 *   3. <b>which members of a group</b> — one tick each, **all of them ticked**. A plotter decomposes
 *      a group: every member is a piece to cut, one by one (see `OExportGeometry::SetCuttingMat`).
 *      So the ticks are not a choice between members — the Cutting tab asks that, because on fabric a
 *      group is cut once — they are the way to leave one out. All ticked is exactly what a plotter
 *      run has always done; untick one and the run cuts the others.
 *
 * **No "how many sizes", and no folded fabric** — the two questions the Cutting tab asks and this
 * one must not: a plotter run is a single size, and there is no fabric to fold.
 *
 * The mat is collected in millimetres and shown in the unit the combo picks, exactly as the laize
 * is: it is a width the user buys mats in, not a width the pattern is measured in.
 */
class OExportPlotterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OExportPlotterWidget(const VContainer *data,
                                  const QSharedPointer<VMeasurements> &measurements,
                                  QWidget *parent = nullptr);

    /// @brief La zone utile du tapis, en millimètres — ce que le run enregistre.
    auto MatWidthMm() const -> qreal;
    auto MatHeightMm() const -> qreal;

    /// @brief L'unité dans laquelle le tapis est SAISI à l'écran — `cm` ou `inch`, jamais le mm.
    ///
    /// Ce n'est pas une conversion : le tapis reste stocké en millimètres. C'est ce que l'utilisatrice
    /// tape et relit, et les zones utiles des tapis du commerce sont des valeurs en pouces.
    auto MatUnit() const -> Unit;

    /// @brief Base of the axis in slot @p index (0 = X, 1 = Y, 2 = W); -1 when the table carries no such axis.
    auto DimensionBase(int index) const -> qreal;

    /// @brief What the axis in slot @p index reads as, empty when the table carries none.
    auto DimensionLabel(int index) const -> QString;

    /// @brief Les pièces que l'utilisatrice a DÉCOCHÉES dans les groupes. Vide : aucune.
    ///
    /// Un plotter décompose les groupes, donc chaque membre est une pièce à couper : les cases ne
    /// font pas un choix, elles font une **exception**. Ce que le run en reçoit, ce sont les pièces à
    /// laisser de côté — et vide veut dire « toutes », c'est-à-dire le comportement de toujours.
    auto ExcludedPieces() const -> QSet<quint32>;

    /// @brief Whether the tab currently describes a run that can be asked for.
    auto IsValid() const -> bool;

signals:
    /// Anything the dialog's Generate button depends on.
    void Changed();

protected:
    void changeEvent(QEvent *event) override;

private:
    void BuildUi();
    void PopulateDimensions(QFormLayout *form);
    void CommitMat();
    void UpdateMatUnit();
    void Retranslate();

    /// Le patron, pour ses groupes — le même conteneur que l'onglet Cutting reçoit, et pour la même
    /// raison : les groupes vivent dans le document, pas dans ce formulaire.
    const VContainer *m_data{nullptr};
    const QSharedPointer<VMeasurements> m_measurements{};

    QLabel *m_matLabel{nullptr};
    QLineEdit *m_matWidthEdit{nullptr};
    QLabel *m_matCrossLabel{nullptr};
    QLineEdit *m_matHeightEdit{nullptr};
    QComboBox *m_matUnitCombo{nullptr};
    Unit m_matUnit{Unit::Cm};

    QVector<QLabel *> m_dimensionLabels{};
    QVector<QComboBox *> m_dimensionCombos{};
    QVector<int> m_dimensionSlots{};

    /// La question des groupes, portée par le widget qui la pose : le même que celui du pré-vol du
    /// lot, pour qu'il n'y ait pas deux façons de peupler cette liste (voir
    /// OExportGroupMembersWidget).
    OExportGroupMembersWidget *m_groupMembers{nullptr};

    /// The mat kept in millimetres rather than re-read from the fields: an unreadable field would
    /// come back as zero, and a mat of zero is a run that cuts nothing.
    ///
    /// The default is 29 × 59 cm — two round numbers, read back exactly as they were typed. It is a
    /// DEFAULT and not a rule: every plotter has its own mat, and the user's own software is the only
    /// place that says which.
    qreal m_matWidthMm{290.0};
    qreal m_matHeightMm{590.0};
};

#endif // OEXPORTPLOTTERWIDGET_H
