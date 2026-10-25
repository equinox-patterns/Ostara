/************************************************************************
 **
 **  @file   oexportprojectorwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/10/2026
 **
 **  @brief  Export mode — l'onglet Projecteur.
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

#ifndef OEXPORTPROJECTORWIDGET_H
#define OEXPORTPROJECTORWIDGET_H

#include "../core/ogradedsizeranges.h"
#include "../vmisc/def.h"

#include <QSharedPointer>
#include <QString>
#include <QVector>
#include <QWidget>

class VContainer;
class VMeasurements;
class QComboBox;
class QLabel;
class QLineEdit;

/**
 * @brief L'onglet Projecteur : quelles tailles, et les deux écarts.
 *
 * **Ce que l'onglet ne demande pas, et c'est le format qui le dit** (`Projector.md` §12) : pas de
 * laize — la largeur est CHERCHÉE, palier par palier, jusqu'à ce que la page tienne —, pas de
 * matière (une projection est toujours du tissu : la doublure n'a pas de sens projetée), pas de page
 * de garde (une projection n'en a pas), et pas de fenêtre de choix de membre (les groupes n'existent
 * pas pour ce format : chaque membre est une pièce classique).
 *
 * **Les tailles, elles, sont celles de Print** : une taille unique, ou une plage enregistrée. Une
 * plage de N tailles donne **N calques dans un seul PDF, une seule page** — jamais N runs. ⚠️ Ce n'est
 * donc pas le mécanisme de l'onglet Cutting plan, dont le « plusieurs tailles » produit N générations
 * séparées.
 *
 * **Les deux écarts** : l'espace entre les pièces (10 cm par défaut, **jamais moins de 5** : en
 * dessous ce n'est plus un plan projecteur, c'est un plan de coupe) et l'écart au bord (0 par défaut,
 * et c'est lui qui décide si le plan tient — 20 à 30 cm font la différence).
 */
class OExportProjectorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OExportProjectorWidget(const VContainer *data,
                                    const QSharedPointer<VMeasurements> &measurements,
                                    const OGradedSizeRanges::Context &context,
                                    QWidget *parent = nullptr);

    /// @brief Le nom de la plage choisie, vide quand la taille unique est demandée.
    auto SizeRangeName() const -> QString;

    /// @brief Vrai quand l'onglet décrit une seule taille, choisie par ses axes.
    auto IsSingleSize() const -> bool;

    /// @brief Base de l'axe du slot @p index (0 = X, 1 = Y, 2 = W) ; -1 quand le tableau n'en a pas.
    auto DimensionBase(int index) const -> qreal;

    /// @brief Ce que lit l'axe du slot @p index, vide quand le tableau n'en porte pas.
    auto DimensionLabel(int index) const -> QString;

    /// @brief L'espace entre deux pièces, en millimètres — **jamais moins de 50** (5 cm).
    auto GapMm() const -> qreal;

    /// @brief L'écart au bord, en millimètres.
    auto EdgeMarginMm() const -> qreal;

    /// @brief L'unité dans laquelle l'onglet est lu et saisi — `cm` ou `inch`, jamais le mm.
    auto DisplayUnit() const -> Unit;

    /// @brief Vrai quand l'onglet décrit un run qui peut être demandé.
    auto IsValid() const -> bool;

signals:
    /// Ce dont le bouton Generate dépend a changé.
    void Changed();

protected:
    void changeEvent(QEvent *event) override;

private:
    void BuildUi();
    void PopulateSizeRanges();
    void PopulateDimensions();
    void UpdateSourceVisibility();
    void CommitLengths();
    void ReadLengths(const Unit &unit);
    void WriteLengths();
    void Retranslate();

    auto HasAxes() const -> bool;

    const VContainer *m_data{nullptr};
    QSharedPointer<VMeasurements> m_measurements{};
    OGradedSizeRanges::Context m_rangeContext{};

    QLabel *m_unitLabel{nullptr};
    QComboBox *m_unitCombo{nullptr};

    QLabel *m_sourceLabel{nullptr};
    QComboBox *m_sourceCombo{nullptr};

    QLabel *m_rangeLabel{nullptr};
    QComboBox *m_rangeCombo{nullptr};

    QVector<QLabel *> m_dimensionLabels{};
    QVector<QComboBox *> m_dimensionCombos{};
    QVector<int> m_dimensionSlots{};

    QLabel *m_gapLabel{nullptr};
    QLineEdit *m_gapEdit{nullptr};

    QLabel *m_marginLabel{nullptr};
    QLineEdit *m_marginEdit{nullptr};

    /// Ce que le PDF porte et que le canvas ne montre pas — **une phrase, pas une documentation**.
    ///
    /// Un utilisateur de projecteur sait à quoi servent une grille de calibration et un fond magenta.
    /// Ce qu'il ne peut pas deviner, c'est qu'ils existent dans le fichier alors que l'aperçu ne les
    /// dessine pas : c'est la seule chose que cette ligne dit.
    QLabel *m_calibrationLabel{nullptr};

    /// Gardés en millimètres plutôt que relus dans les champs : un champ illisible reviendrait à
    /// zéro, et zéro est un écart au bord parfaitement légitime — une faute de frappe demanderait
    /// alors silencieusement aucun écart.
    qreal m_gapMm{100.0};    ///< 10 cm — le défaut du format
    qreal m_marginMm{0.0};   ///< 0 — la marge est une décision, pas un défaut

    Unit m_unit{Unit::Cm};
};

#endif // OEXPORTPROJECTORWIDGET_H
