/************************************************************************
 **
 **  @file   oexportbatchpage.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Export mode — une catégorie de l'export en lot : son dossier, ses
 **          générations, et son option.
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

#ifndef OEXPORTBATCHPAGE_H
#define OEXPORTBATCHPAGE_H

#include <QString>
#include <QVector>
#include <QWidget>

class QCheckBox;
class QEvent;
class QLabel;
class QLineEdit;
class QPushButton;

/**
 * @brief Une catégorie de l'export en lot : le dossier où elle écrit, les générations qu'elle
 *        offre, et l'option qui n'appartient qu'à elle.
 *
 * **Une page par catégorie, et une seule catégorie à la fois.** C'est ce qui fait qu'un lot de
 * print n'écrit que des PDF et qu'un lot de plotter n'écrit que des SVG : le dossier est celui de
 * la page, et les lignes offertes sont celles de la page.
 *
 * **Les générations sont une colonne de lignes, et non une table** — même raison que le second
 * temps de l'export d'image, dont ceci est le frère : une table apporte un en-tête qui n'apprend
 * rien, le survol des cellules et une sélection dont on ne fait rien. Une case et un champ disent
 * ce qu'ils sont.
 *
 * **Le nom est offert plutôt que tapé** : la ligne s'ouvre sur le nom de la génération, celui que
 * le dock affiche, et l'extension du format est **forcée** à la lecture — un fichier nommé
 * « plan.pdf » qui contiendrait un SVG serait un nom qui ment sur son contenu.
 */
class OExportBatchPage : public QWidget
{
    Q_OBJECT

public:
    /// Une génération à écrire, et le nom du fichier qu'elle produira.
    struct Row
    {
        quint32 id{0};
        QString name{}; ///< le nom de FICHIER, extension comprise — ce que l'utilisatrice lit et édite
    };

    /// @param rows            les générations de cette catégorie, dans l'ordre où elles seront écrites.
    /// @param suffix          l'extension du format, point compris — « .pdf », « .svg ».
    /// @param directory       le dossier sur lequel la page s'ouvre.
    /// @param withLayoutLayer si la catégorie offre la case du calque de décor — **Print seule**.
    explicit OExportBatchPage(const QVector<Row> &rows, const QString &suffix,
                              const QString &directory, bool withLayoutLayer,
                              QWidget *parent = nullptr);

    /// @brief Le dossier de la page, tel que tapé ou parcouru.
    auto Directory() const -> QString;

    /// @brief Change l'extension des noms — le format de la catégorie a changé.
    ///
    /// Le plan de coupe offre **trois** formats : l'extension des lignes suit celui qui est choisi,
    /// et elle est remplacée plutôt qu'empilée — sans quoi « plan.dxf » deviendrait « plan.dxf.plt ».
    void SetSuffix(const QString &suffix);

    /// @brief Les lignes encore cochées, avec leur nom tel qu'il est, prêtes à être écrites.
    auto Chosen() const -> QVector<Row>;

    /// @brief Si le décor du plan est écrit comme un calque du lecteur. Faux quand la catégorie
    ///        n'offre pas l'option — le plan de coupe n'a pas de PDF, et le plotter un fichier
    ///        sans calque du tout.
    auto AddLayoutLayer() const -> bool;

    /// @brief Si la page a quelque chose à offrir. Une page vide garde son bouton éteint.
    auto HasRows() const -> bool;

    /// @brief Met le focus sur le champ du dossier — le seul endroit qu'une erreur de dossier
    ///        demande de corriger.
    void FocusDirectory();

    /// @brief Met le focus sur le premier nom et le sélectionne : c'est le seul champ de la page
    ///        qui soit là pour être touché, le dossier venant d'être proposé et les cases cochées.
    void FocusFirstName();

    void Retranslate();

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void OnBrowse();

private:
    /// Le nom de la ligne @p row tel qu'il est tapé, extension ajoutée là où elle manque. Un seul
    /// lecteur, pour que ce que la ligne montre et ce que le lot écrit ne viennent pas de deux
    /// orthographes.
    auto NameOf(int row) const -> QString;

    QVector<Row> m_rows{};
    QString m_suffix{};

    /// Une case et un champ par génération, dans l'ordre d'écriture. Retenus plutôt que retrouvés :
    /// il n'y a aucune table à interroger, et c'est le sujet de la chose.
    QVector<QCheckBox *> m_ticks{};
    QVector<QLineEdit *> m_names{};

    QLabel *m_pathLabel{nullptr};
    QLineEdit *m_pathEdit{nullptr};
    QPushButton *m_browseButton{nullptr};

    /// Nulle quand la catégorie n'offre pas l'option — et c'est ce que `AddLayoutLayer` lit.
    QCheckBox *m_layoutLayerCheck{nullptr};

    /// Montrée à la place des lignes quand la catégorie n'a aucune génération à offrir.
    QLabel *m_emptyLabel{nullptr};
};

#endif // OEXPORTBATCHPAGE_H
