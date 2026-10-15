/************************************************************************
 **
 **  @file   oexportcuttingwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   23/09/2026
 **
 **  @brief  The Cutting plan tab of the Export dialog — the width, and one
 **          piece per group.
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

#ifndef OEXPORTCUTTINGWIDGET_H
#define OEXPORTCUTTINGWIDGET_H

#include <QMap>
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
class QRadioButton;
class QVBoxLayout;
class OExportGroupChoiceWidget;
class VContainer;
class VMeasurements;

/**
 * @brief The Cutting plan tab: the width the pieces are cut from, and which member of each group is cut.
 *
 * Three questions, in the order the user answers them:
 *
 *   1. <b>which size</b> — one, never a range. A cutting machine cuts one size at a time, and a plan
 *      holds the pieces of that size laid on the fabric. The axes are the same rows the Print tab
 *      offers for a single size, from the same measurement table.
 *   2. <b>how wide the fabric is</b> — the laize, a free field: fabrics do not all come the same
 *      width, so no model could answer it. It is the width the nesting runs on, and the only one.
 *      Right below it, <b>whether that fabric is folded in two</b> — home sewing folds it along the
 *      selvedge and traces the halves on the folded cloth. The nesting then runs on half the width,
 *      which is the one thing about the folded answer the user has to be told rather than find.
 *      Et, sous la même laize, <b>la doublure</b> : un vêtement doublé se taille deux fois, et le
 *      patron le dit déjà — chaque pièce porte une quantité de coupe par matière. La case demande le
 *      second plan, et la ligne qu'elle ouvre sa largeur à lui ; tout le reste est commun aux deux.
 *   3. <b>which member of each group</b> — a group serves printing, where two pieces fill one shape
 *      on a sheet. On fabric a piece is cut once, so a group is decomposed and the user says which
 *      of its members she cuts. The box only appears when the pattern has a group to ask about.
 *
 * The laize is collected in millimetres and shown in centimetres: a laize is always read in
 * centimetres, and a pattern measured in inches does not make one.
 */
class OExportCuttingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OExportCuttingWidget(const VContainer *data, const QSharedPointer<VMeasurements> &measurements,
                                  QWidget *parent = nullptr);

    /// @brief The laize, in millimetres — the width the nesting runs on.
    auto FabricWidthMm() const -> qreal;

    /// @brief Vrai quand le run demande AUSSI un plan de DOUBLURE — la case cochée.
    ///
    /// La case ne peut être cochée que si le patron porte au moins une pièce à tailler dans la
    /// doublure (voir @ref UpdateLiningState) : cochée veut donc dire « disponible », et il n'y a pas
    /// deux lectures à faire.
    auto HasLiningPlan() const -> bool;

    /// @brief La laize de la doublure, en millimètres.
    ///
    /// **Tant qu'elle n'a pas été saisie, c'est celle du tissu** : c'est ce que veut dire « identique
    /// par défaut ». Dès que l'utilisatrice écrit dans le champ, la doublure a la sienne et ne suit
    /// plus le tissu.
    auto LiningWidthMm() const -> qreal;

    /// @brief L'unité dans laquelle la laize est SAISIE à l'écran — `cm` ou `inch`, jamais le mm.
    ///
    /// Ce n'est pas une conversion : la laize reste stockée en millimètres, et changer d'unité ne
    /// change pas un millimètre de tissu. C'est ce que l'utilisatrice tape et relit — et c'est le
    /// monde dans lequel elle achète, donc c'est aussi ce que son run enregistre.
    auto DisplayUnit() const -> Unit;

    /// @brief The member kept for each group: group id → piece id. Empty when it has no group.
    auto MemberChoices() const -> QMap<quint32, quint32>;

    /// @brief Vrai quand une seule taille est décrite — par les combos.
    ///
    /// **Toujours vrai sur une table sans axes** : il n'y a pas de taille à choisir, le document
    /// est sur la sienne, et le formulaire ne pose même pas la question.
    auto IsSingleSize() const -> bool;

    /// @brief Les bases cochées pour l'axe en slot @p index (0 = X, 1 = Y, 2 = W), dans l'ordre de
    ///        la table. Vide en taille unique.
    auto CheckedBases(int index) const -> QVector<qreal>;

    /// @brief Les libellés cochés pour l'axe en slot @p index, dans le même ordre que les bases.
    auto CheckedLabels(int index) const -> QStringList;

    /// @brief Vrai quand la table porte des axes — donc quand il y a un choix de tailles à faire.
    auto HasAxes() const -> bool;

    /// @brief « Sens unique » — no piece is turned a half-turn. Removes a sense, not an axis.
    auto IsOneWayLayout() const -> bool;

    /// @brief « Tissu plié » — the fabric is folded in two along the selvedge.
    ///
    /// The run then happens on **half the laize**: what is traced on the folded cloth is a half,
    /// and the fold gives the other one. A piece cut on the fold keeps its fold line on that fold
    /// edge, so it stays folded — the plan unfolded it, this one lays the half.
    auto IsFoldedFabric() const -> bool;

    /// @brief L'espace gardé entre deux pièces, en millimètres. **Zéro par défaut** : sur du tissu,
    ///        deux pièces peuvent se toucher bord à bord, et rien ne justifie de les écarter.
    auto GapMm() const -> qreal;

    /// @brief L'espace gardé depuis les BORDS du tissu, en millimètres. **Zéro par défaut.**
    ///
    /// **C'est une marge d'erreur** — un tissu n'est pas toujours coupé droit — et elle s'applique
    /// donc à **trois côtés de chaque rectangle : tous sauf le pli**. Une pièce au pli garde en effet
    /// sa foldline SUR le pli, sans quoi elle serait trop étroite du double une fois le tissu
    /// déplié. Sur du tissu plié, le nesting la répartit par rectangle — voir `SeatMargins`, dans
    /// oexportnesting.cpp, seul endroit qui écrive ces marges.
    auto EdgeMarginMm() const -> qreal;

    /// @brief Base of the axis in slot @p index (0 = X, 1 = Y, 2 = W); -1 when the table carries no such axis.
    auto DimensionBase(int index) const -> qreal;

    /// @brief What the axis in slot @p index reads as, empty when the table carries none.
    auto DimensionLabel(int index) const -> QString;

    /// @brief Whether the tab currently describes a run that can be asked for.
    auto IsValid() const -> bool;

signals:
    /// Anything the dialog's Generate button depends on changed.
    void Changed();

protected:
    void changeEvent(QEvent *event) override;

private:
    /// Un axe et ses cases à cocher : le contenu du mode « plusieurs tailles ».
    struct SizeGrid
    {
        int slot{-1};
        QLabel *label{nullptr};
        QWidget *box{nullptr};
        QVector<QCheckBox *> boxes{};
        QVector<qreal> bases{};
    };

    void BuildUi();
    void PopulateDimensions(QFormLayout *form);
    void PopulateSizeGrids(QVBoxLayout *layout);
    void UpdateSizeMode();
    void CommitFabricWidth();
    void UpdateFabricWidthUnit();
    void CommitGap();
    void CommitEdgeMargin();
    void Retranslate();

    /// Règle l'état de la case « Doublure » : disponible — et cochée — dès que le patron porte une
    /// pièce à tailler dans la doublure, indisponible et décochée sinon.
    ///
    /// ⚠️ **La disponibilité se lit avec la règle du run**, `PiecesToCut` sur la matière doublure, et
    /// non avec un balayage à part : le membre de groupe retenu, la pièce hors layout et la quantité
    /// vide sont donc jugés exactement comme ils le seront au nesting. Une case allumée sur une pièce
    /// que le run écarterait promettrait un plan vide.
    ///
    /// Rappelée à chaque changement du choix des groupes : une case peut s'éteindre quand la pièce
    /// doublure du groupe n'est plus celle qu'on taille.
    void UpdateLiningState();

    /// Montre la ligne de laize de doublure quand — et seulement quand — la case est cochée.
    void UpdateLiningWidthRow();

    /// Réaffiche le champ de laize doublure depuis ses millimètres, dans son unité.
    void ShowLiningWidth();

    void CommitLiningWidth();
    void UpdateLiningWidthUnit();

    const VContainer *m_data{nullptr};
    QSharedPointer<VMeasurements> m_measurements{};

    /// Le formulaire des réglages du plan. Gardé pour pouvoir MASQUER la ligne de la doublure :
    /// `QFormLayout::setRowVisible` a besoin de lui, et un `hide()` sur les deux widgets laisserait
    /// l'espace de la ligne.
    QFormLayout *m_form{nullptr};

    QLabel *m_fabricWidthLabel{nullptr};
    QLineEdit *m_fabricWidthEdit{nullptr};

    /// L'unité de la laize, choisie par l'utilisatrice : cm ou inch. Le libellé du combo la dit, donc
    /// il n'y a plus de QLabel d'unité à côté du champ — il n'aurait plus rien à dire.
    QComboBox *m_fabricWidthUnitCombo{nullptr};
    Unit m_fabricWidthUnit{Unit::Cm};
    QCheckBox *m_foldedCheck{nullptr};
    QCheckBox *m_oneWayCheck{nullptr};

    /// La case « Doublure », et le champ de sa laize. Ils vont ensemble : la case dit qu'il y a un
    /// second plan, le champ dit sur quelle largeur il se coupe.
    QCheckBox *m_liningCheck{nullptr};
    QLabel *m_liningWidthLabel{nullptr};
    QLineEdit *m_liningWidthEdit{nullptr};
    QComboBox *m_liningWidthUnitCombo{nullptr};
    Unit m_liningWidthUnit{Unit::Cm};

    /// La laize PROPRE de la doublure, en millimètres — lue seulement quand `m_liningWidthEdited`.
    /// Gardée en millimètres plutôt que relue du champ, comme celle du tissu et pour la même raison.
    qreal m_liningWidthMm{1400.0};

    /// Vrai dès que l'utilisatrice a écrit dans le champ de la doublure (ou commité sa valeur) :
    /// jusque-là la doublure suit la laize du tissu, et c'est ce que « identique par défaut » veut dire.
    bool m_liningWidthEdited{false};

    /// L'état de disponibilité au tour précédent, pour ne cocher la case qu'au moment où elle le
    /// devient — sans quoi un décochage volontaire serait défait au changement suivant.
    bool m_liningWasAvailable{false};

    QLabel *m_gapLabel{nullptr};
    QLineEdit *m_gapEdit{nullptr};
    QLabel *m_gapUnit{nullptr};
    qreal m_gapMm{0.0};

    QLabel *m_edgeMarginLabel{nullptr};
    QLineEdit *m_edgeMarginEdit{nullptr};
    QLabel *m_edgeMarginUnit{nullptr};
    qreal m_edgeMarginMm{0.0};

    QVector<QLabel *> m_dimensionLabels{};
    QVector<QComboBox *> m_dimensionCombos{};
    QVector<int> m_dimensionSlots{};

    /// Le cadre des tailles, et le contenu de CHAQUE choix — la radio et ce qu'elle commande
    /// doivent se lire ensemble : deux cadres voisins du même titre ne se comprenaient pas.
    QGroupBox *m_sizeModeBox{nullptr};
    QRadioButton *m_singleRadio{nullptr};
    QRadioButton *m_severalRadio{nullptr};
    QWidget *m_singleContent{nullptr};
    QWidget *m_severalContent{nullptr};
    QVector<SizeGrid> m_sizeGrids{};

    /// La question des groupes, posée par le widget qui la porte : le même que celui du pré-vol,
    /// pour qu'il n'y ait pas deux façons de peupler cette liste (voir OExportGroupChoiceWidget).
    OExportGroupChoiceWidget *m_groupChoice{nullptr};

    /// Kept in millimetres rather than re-read from the field: an unreadable field would come back
    /// as zero, and zero is a laize that would run the nesting on nothing.
    qreal m_fabricWidthMm{1400.0};
};

#endif // OEXPORTCUTTINGWIDGET_H
