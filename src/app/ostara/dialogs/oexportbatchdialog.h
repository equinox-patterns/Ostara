/************************************************************************
 **
 **  @file   oexportbatchdialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Export mode — l'export en lot : trois catégories, une à la fois.
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

#ifndef OEXPORTBATCHDIALOG_H
#define OEXPORTBATCHDIALOG_H

#include "oexportbatchpage.h"

#include <QDialog>
#include <QString>
#include <QVector>

class QDialogButtonBox;
class QEvent;
class QLabel;
class QPushButton;
class QTabWidget;

/**
 * @brief Écrire plusieurs générations d'un coup — le bouton « Export several outputs » du dock.
 *
 * **Une catégorie à la fois, et c'est la catégorie qui décide du format** : Print écrit un PDF par
 * génération, Plotter un SVG par génération. Les deux pages sont donc indépendantes — chacune son
 * dossier, ses lignes, ses noms — et le bouton d'export n'écrit que la page courante. Rien ne
 * traverse l'onglet : exporter les PDF ne touche pas aux SVG, et réciproquement.
 *
 * **Le plan de coupe a son onglet, et il est désactivé.** Il n'a aucun format aujourd'hui — sa
 * sortie sera HPGL/PLT, et elle est reportée (décidé le 24-09-2026). L'onglet reste donc
 * là, avec ce qu'il attend écrit dedans, plutôt que d'être absent : un onglet qui manque se lirait
 * comme un oubli, et un onglet ouvert sur le vide comme une panne.
 *
 * **La question d'écrasement est posée fichier par fichier**, comme l'export d'une seule
 * génération : le lot ne demande rien avant de commencer, il demande en écrivant — et une réponse
 * « non » laisse ce fichier-là tel qu'il est, sans arrêter les autres.
 */
class OExportBatchDialog : public QDialog
{
    Q_OBJECT

public:
    /// La catégorie de la page courante — et donc le format que l'export écrira.
    ///
    /// ⚠️ Le type ne s'appelle **pas** `Category` : un membre et un type du même nom dans une même
    /// classe se masquent, et `OExportBatchDialog::Category` ne nommerait plus le type mais
    /// l'accesseur.
    enum class OutputCategory
    {
        Cutting,
        Print,
        Plotter
    };

    /// @param printRows   les générations de print à offrir, dans l'ordre du dock.
    /// @param plotterRows les générations de plotter à offrir, dans l'ordre du dock.
    /// @param directory   le dossier sur lequel les deux pages s'ouvrent.
    explicit OExportBatchDialog(const QVector<OExportBatchPage::Row> &printRows,
                                const QVector<OExportBatchPage::Row> &plotterRows,
                                const QString &directory, QWidget *parent = nullptr);

    /// @brief La catégorie de la page courante.
    auto Category() const -> OutputCategory;

    /// @brief Le dossier de la page courante.
    auto Directory() const -> QString;

    /// @brief Les lignes cochées de la page courante, avec leur nom tel qu'il est.
    auto Chosen() const -> QVector<OExportBatchPage::Row>;

    /// @brief L'option de calque de décor de la page courante — fausse partout sauf en Print.
    auto AddLayoutLayer() const -> bool;

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void OnAccept();
    void OnTabChanged(int index);

private:
    void Retranslate();

    /// La page de la catégorie courante — **nulle** pour l'onglet du plan de coupe, qui n'a ni
    /// dossier ni lignes. Tout ce qui se lit d'une page passe par ici, et chaque lecteur répond à
    /// la nullité plutôt que de supposer qu'un onglet désactivé ne peut pas être courant.
    auto CurrentPage() const -> OExportBatchPage *;

    /// Le bouton d'export suit la page : une catégorie sans génération s'exporte à vide, donc pas
    /// du tout, et le bouton le dit sur son infobulle.
    void UpdateExportButton();

    QTabWidget *m_tabs{nullptr};
    QLabel *m_cuttingLabel{nullptr};
    OExportBatchPage *m_printPage{nullptr};
    OExportBatchPage *m_plotterPage{nullptr};
    QDialogButtonBox *m_buttonBox{nullptr};
    QPushButton *m_exportButton{nullptr};
};

#endif // OEXPORTBATCHDIALOG_H
