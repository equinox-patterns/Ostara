/************************************************************************
 **
 **  @file   oexportfiledialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Export mode — where an exported file goes, and what it is
 **          called. One skeleton for every format.
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

#ifndef OEXPORTFILEDIALOG_H
#define OEXPORTFILEDIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>
#include <QVector>

class QDialogButtonBox;
class QEvent;
class QGridLayout;
class QLabel;
class QLineEdit;
class QPushButton;

/**
 * @brief Where an exported file is written: a name, a folder, and nothing else.
 *
 * The one question the dock cannot answer by itself. Everything else a run needs is already settled
 * when the button is clicked — the generation is filed, its geometry frozen — and what is left is
 * where the file goes and what it is called.
 *
 * **Shared by every export that writes a file the user names**: the PDF of a print, the SVG of a
 * plotter plan. They ask the very same thing, and the thing has **one definition** — the folder goes
 * through the same check, an existing file gets the same question, and the suffix is appended the
 * same way. A copy per format would be four places to keep in step, and the day one of them forgets
 * the overwrite question is the day a file is lost without a word.
 *
 * A format that needs one more switch derives from this and adds it with AddOption — the PDF's decor
 * layer is exactly that, and nothing else about the dialog changes.
 *
 * The name is offered rather than typed: it opens on the generation's own name, which is what the
 * list calls it and what a folder of six of them wants to read. It stays editable because the file
 * is a delivery, and a delivery is named for whoever receives it.
 */
class OExportFileDialog : public QDialog
{
    Q_OBJECT

public:
    /// @param outputName the generation's name, offered as the file name.
    /// @param directory  where it opens; the folder last used, or Documents.
    /// @param suffix     the extension of this format, dot included — ".pdf", ".svg".
    explicit OExportFileDialog(const QString &outputName, const QString &directory,
                               const QString &suffix, QWidget *parent = nullptr);

    /// @brief The file name, with the format's suffix — appended here if the user left it off.
    auto FileName() const -> QString;

    /// @brief The folder, as typed or browsed to.
    auto Directory() const -> QString;

    /// @brief Folder and file name together.
    auto FullPath() const -> QString;

    /// @brief TOUS les chemins, dans l'ordre des lignes de nom — un, ou deux.
    ///
    /// Un dialogue qui écrit deux fichiers en demande deux, et l'appelant les écrit dans cet ordre-là :
    /// c'est la liste, et non un comptage refait de son côté, qui dit combien de fichiers il y a.
    auto FullPaths() const -> QStringList;

    /// @brief TOUS les noms de fichier, dans l'ordre des lignes — un, ou deux. Extension comprise.
    auto FileNames() const -> QStringList;

    /// @brief La question d'écrasement, posée une fois par fichier.
    ///
    /// **Une seule définition**, parce que l'export en lot écrit plusieurs fichiers sans passer par
    /// ce dialogue : deux copies de cette question seraient deux façons d'écraser un fichier, et le
    /// jour où l'une oublie de demander est le jour où un fichier est perdu sans un mot — c'est
    /// exactement ce pourquoi ce dialogue existe.
    ///
    /// @return true quand il faut écrire — donc aussi quand le fichier n'existe pas.
    ///
    /// @param title the window title, defaulting to the export's. It is a parameter because the question
    ///        is not the export's alone — saving a piece to the library asks the same one — and a window
    ///        titled "Export" over a piece would name the wrong gesture. The question itself stays here,
    ///        single.
    static bool ConfirmOverwrite(QWidget *parent, const QString &filePath, const QString &title = QString());

protected:
    void changeEvent(QEvent *event) override;

    /// @brief Les textes du dialogue.
    ///
    /// **Surchargeable, et appelée deux fois par un dialogue dérivé** : la première passe est celle de
    /// ce squelette, où la partie dérivée n'existe pas encore — son titre et ses options ne sont pas
    /// posés. Un dialogue qui ajoute une option la rappelle une fois la sienne construite.
    virtual void Retranslate();

    /// @brief Ajoute une option sous les deux champs, au-dessus des boutons.
    ///
    /// Le squelette reste ici : une option de plus ne doit pas obliger son auteur à savoir à quelle
    /// ligne du tableau vivent les boutons, ni à les déplacer quand elle en ajoute une seconde.
    void AddOption(QWidget *option);

    /// @brief Change l'extension du format — et le nom avec elle.
    ///
    /// Un dialogue qui offre **plusieurs** formats ne peut pas garder l'extension de son
    /// constructeur : le `.dxf` d'un choix deviendrait un `.dxf.plt` au suivant. Ce que
    /// l'utilisatrice a tapé est conservé ; seule l'ancienne extension est retirée.
    void SetSuffix(const QString &suffix);

    /// @brief Le libellé de la PREMIÈRE ligne de nom.
    ///
    /// Le squelette y met « File name: » au moment de traduire. **Un dialogue qui écrit deux fichiers
    /// ne demande plus « un nom »** : il demande celui de chaque plan, et la première ligne prend alors
    /// le nom de sa matière. Le dérivé le repose donc après chaque traduction, comme son titre.
    auto NameLabel() const -> QLabel *;

    /// @brief Insère une ligne de NOM **sous celles qui existent**, et rend son champ.
    ///
    /// C'est le squelette qui la pose, et non le dérivé dans sa propre grille : l'extension du format
    /// est reportée sur chaque champ (voir SetSuffix), le contrôle de nom vide les visite tous, et la
    /// question d'écrasement est posée pour chacun. Une seconde mécanique écrite ailleurs serait une
    /// seconde façon de nommer et d'écraser un fichier — exactement ce que ce dialogue existe pour
    /// empêcher.
    ///
    /// @param label le libellé de la ligne. **Il appartient à l'appelant**, qui le traduit : c'est son
    ///              mot, pas celui du squelette.
    /// @param name  le nom offert, extension comprise ou non — FileName l'ajoute s'il manque.
    auto AddNameRow(QLabel *label, const QString &name) -> QLineEdit *;

private slots:
    void OnBrowse();
    void OnAccept();

private:
    /// L'extension du format, ajoutée au nom si l'utilisatrice l'a omise. C'est la seule chose qui
    /// distingue deux dialogues de formats différents, avec le titre.
    QString m_suffix{};

    QGridLayout *m_grid{nullptr};
    /// La ligne où la prochaine option se pose — et celle des boutons, qui la suit toujours.
    int m_optionRow{2};

    QLabel *m_nameLabel{nullptr};
    QLabel *m_pathLabel{nullptr};

    /// Les champs de nom, dans l'ordre où ils s'affichent : celui de la sortie, et — pour un dialogue
    /// qui écrit deux fichiers — celui de la seconde. **Le premier est le seul que `FileName()` lit** :
    /// les autres n'existent que par `FullPaths()`.
    QVector<QLineEdit *> m_nameEdits{};

    QLineEdit *m_pathEdit{nullptr};
    QPushButton *m_browseButton{nullptr};
    QDialogButtonBox *m_buttonBox{nullptr};

    /// Le champ de la première ligne. Jamais nul : le squelette le pose dans son constructeur.
    auto FirstNameEdit() const -> QLineEdit *;

    /// @brief Fait descendre d'une ligne tout ce qui occupe la grille à partir de @p first.
    ///
    /// **Un QGridLayout n'a pas d'`insertRow`** — un QFormLayout en a un, pas lui — donc la place pour
    /// une ligne de nom de plus se fait à la main : on relève l'emplacement de chaque widget, on le
    /// repose un cran plus bas, et `m_optionRow` suit. Les widgets, eux, ne bougent pas d'un parent.
    void ShiftRowsDown(int first);
};

#endif  // OEXPORTFILEDIALOG_H
