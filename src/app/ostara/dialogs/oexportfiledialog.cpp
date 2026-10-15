/************************************************************************
 **
 **  @file   oexportfiledialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Implementation of the shared export destination dialog.
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

#include "oexportfiledialog.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>

#include <utility>

//---------------------------------------------------------------------------------------------------------------------
OExportFileDialog::OExportFileDialog(const QString &outputName, const QString &directory,
                                     const QString &suffix, QWidget *parent)
  : QDialog(parent),
    m_suffix(suffix)
{
    setModal(true);
    setMinimumWidth(520);

    m_grid = new QGridLayout(this);

    m_nameLabel = new QLabel(this);
    m_nameEdits.append(new QLineEdit(outputName, this));

    m_pathLabel = new QLabel(this);
    m_pathEdit = new QLineEdit(directory, this);
    m_browseButton = new QPushButton(this);
    connect(m_browseButton, &QPushButton::clicked, this, &OExportFileDialog::OnBrowse);

    m_grid->addWidget(m_nameLabel, 0, 0);
    m_grid->addWidget(FirstNameEdit(), 0, 1, 1, 2);
    m_grid->addWidget(m_pathLabel, 1, 0);
    m_grid->addWidget(m_pathEdit, 1, 1);
    m_grid->addWidget(m_browseButton, 1, 2);

    m_buttonBox = new QDialogButtonBox(this);
    auto *exportButton = m_buttonBox->addButton(tr("Export"), QDialogButtonBox::AcceptRole);
    exportButton->setAutoDefault(false);
    m_buttonBox->addButton(QDialogButtonBox::Cancel);
    // Asserted last, once every button is in: the box lays its buttons out as they arrive
    // and can hand the default to the one added after Export — Return would then cancel
    // the export the user just described.
    exportButton->setDefault(true);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &OExportFileDialog::OnAccept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    m_grid->addWidget(m_buttonBox, m_optionRow, 0, 1, 3);

    Retranslate();
    FirstNameEdit()->setFocus();
    FirstNameEdit()->selectAll();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFileDialog::FirstNameEdit() const -> QLineEdit *
{
    return m_nameEdits.first();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFileDialog::FileName() const -> QString
{
    return FileNames().value(0);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFileDialog::FileNames() const -> QStringList
{
    QStringList names;
    names.reserve(m_nameEdits.size());

    // **Chaque champ passe par la même règle**, extension comprise : deux noms dont un seul porterait
    // l'extension seraient deux fichiers dont un seul s'ouvrirait.
    for (const QLineEdit *edit : m_nameEdits)
    {
        QString name = edit->text().trimmed();
        if (!name.isEmpty() && !m_suffix.isEmpty() && !name.endsWith(m_suffix, Qt::CaseInsensitive))
        {
            name += m_suffix;
        }

        names.append(name);
    }

    return names;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFileDialog::Directory() const -> QString
{
    return m_pathEdit->text().trimmed();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFileDialog::FullPath() const -> QString
{
    return FullPaths().value(0);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFileDialog::FullPaths() const -> QStringList
{
    QStringList paths;
    const QDir folder(Directory());

    // Le dossier est UN pour tous : deux fichiers d'un même export se livrent ensemble, et deux champs
    // de dossier seraient deux occasions de les séparer sans le vouloir.
    const QStringList names = FileNames();
    paths.reserve(names.size());
    for (const QString &name : names)
    {
        paths.append(folder.filePath(name));
    }

    return paths;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFileDialog::NameLabel() const -> QLabel *
{
    return m_nameLabel;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFileDialog::ShiftRowsDown(int first)
{
    struct Spot
    {
        QWidget *widget{nullptr};
        int row{0};
        int col{0};
        int rowSpan{1};
        int colSpan{1};
    };

    // **Un QGridLayout ne se décale pas tout seul.** Il n'a pas d'`insertRow` — contrairement à un
    // QFormLayout — donc la place se fait à la main : on relève où chaque widget se trouve, puis on le
    // repose une ligne plus bas. Rien n'est détruit : les widgets restent ceux de la fenêtre, et
    // `removeWidget` ne fait que les lâcher le temps du déplacement (il n'y a pas d'item à posséder,
    // donc pas de fuite à craindre).
    QVector<Spot> below;
    const int count = m_grid->count();
    for (int i = 0; i < count; ++i)
    {
        int row = 0;
        int col = 0;
        int rowSpan = 1;
        int colSpan = 1;
        m_grid->getItemPosition(i, &row, &col, &rowSpan, &colSpan);

        if (row >= first && m_grid->itemAt(i) != nullptr && m_grid->itemAt(i)->widget() != nullptr)
        {
            below.append({m_grid->itemAt(i)->widget(), row, col, rowSpan, colSpan});
        }
    }

    for (const Spot &spot : std::as_const(below))
    {
        m_grid->removeWidget(spot.widget);
        m_grid->addWidget(spot.widget, spot.row + 1, spot.col, spot.rowSpan, spot.colSpan);
    }

    // Les options et les boutons qui les suivent ont descendu avec le reste : le compte de lignes du
    // squelette doit le savoir, sans quoi la prochaine option s'écraserait sur la ligne des boutons.
    ++m_optionRow;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFileDialog::AddNameRow(QLabel *label, const QString &name) -> QLineEdit *
{
    // **Sous celles qui existent déjà, et pas toujours en deuxième ligne.** Le premier nom occupe la
    // ligne 0 ; le second prend la 1 ; un troisième trouverait la 2 — l'ordre des champs et l'ordre des
    // lignes ne peuvent donc pas diverger, quel que soit le nombre de noms demandés.
    const int row = static_cast<int>(m_nameEdits.size());

    ShiftRowsDown(row);

    auto *edit = new QLineEdit(name, this);
    label->setParent(this);
    m_grid->addWidget(label, row, 0);
    m_grid->addWidget(edit, row, 1, 1, 2);

    m_nameEdits.append(edit);
    return edit;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFileDialog::AddOption(QWidget *option)
{
    m_grid->addWidget(option, m_optionRow, 0, 1, 3);
    ++m_optionRow;

    // Les boutons suivent l'option, et c'est ce qui fait qu'un dialogue dérivé n'a pas à savoir où
    // ils vivent : il ajoute, le squelette replace.
    m_grid->removeWidget(m_buttonBox);
    m_grid->addWidget(m_buttonBox, m_optionRow, 0, 1, 3);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFileDialog::SetSuffix(const QString &suffix)
{
    if (suffix == m_suffix)
    {
        return;
    }

    // L'ancienne extension est RETIRÉE du champ, pas empilée : un dialogue qui change de format ne
    // doit pas transformer « plan.dxf » en « plan.dxf.plt ». Le reste du nom est ce que
    // l'utilisatrice a écrit, et il ne bouge pas.
    //
    // ⚠️ On retire l'ANCIENNE extension : sa longueur est prise avant que `m_suffix` ne change, sans
    // quoi `.dxf` et `.plt` ne se retireraient l'un l'autre que par hasard — ils n'ont pas la même.
    const QString previous = m_suffix;
    m_suffix = suffix;

    // ⚠️ **Et on le fait sur CHAQUE champ de nom** : un dialogue qui écrit deux fichiers sous un format
    // qui vient de changer laisserait sinon l'un des deux annoncer l'ancien — un nom qui ment sur son
    // contenu, et précisément sur le fichier qu'on ne regarde pas.
    if (previous.isEmpty())
    {
        return;
    }

    for (QLineEdit *edit : std::as_const(m_nameEdits))
    {
        const QString typed = edit->text().trimmed();
        if (typed.endsWith(previous, Qt::CaseInsensitive))
        {
            edit->setText(typed.chopped(previous.size()));
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFileDialog::ConfirmOverwrite(QWidget *parent, const QString &filePath, const QString &title) -> bool
{
    if (!QFileInfo::exists(filePath))
    {
        return true;
    }

    const auto answer = QMessageBox::question(parent, title.isEmpty() ? tr("Export") : title,
                                              tr("%1 already exists. Replace it?")
                                                  .arg(QFileInfo(filePath).fileName()),
                                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    return answer == QMessageBox::Yes;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFileDialog::OnBrowse()
{
    const QString chosen = QFileDialog::getExistingDirectory(this, tr("Folder"), Directory());
    if (!chosen.isEmpty())
    {
        m_pathEdit->setText(chosen);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFileDialog::OnAccept()
{
    // **Chaque nom, puis chaque fichier.** Les contrôles se font en deux temps : d'abord tous les noms
    // — un dialogue qui en demande deux et n'en reçoit qu'un ne décrit rien —, ensuite la question
    // d'écrasement, posée UNE FOIS PAR FICHIER, comme le lot la pose. Un « non » garde le dialogue
    // ouvert : c'est la même réponse que pour un seul fichier, et rien n'est écrit à moitié.
    for (QLineEdit *edit : std::as_const(m_nameEdits))
    {
        if (edit->text().trimmed().isEmpty())
        {
            QMessageBox::warning(this, tr("Export"), tr("The file needs a name."));
            edit->setFocus();
            return;
        }
    }

    const QFileInfo folder(Directory());
    if (!folder.isDir())
    {
        QMessageBox::warning(this, tr("Export"), tr("That folder does not exist."));
        m_pathEdit->setFocus();
        return;
    }

    const QStringList paths = FullPaths();
    for (const QString &path : paths)
    {
        if (!ConfirmOverwrite(this, path))
        {
            return;
        }
    }

    accept();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFileDialog::Retranslate()
{
    // « Le plan », sans dire lequel : c'est le titre du squelette, et un dialogue dérivé nomme son
    // propre format dans le sien.
    setWindowTitle(tr("Export the plan"));
    m_nameLabel->setText(tr("File name:"));
    m_pathLabel->setText(tr("Folder:"));
    m_browseButton->setText(tr("Browse..."));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFileDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QDialog::changeEvent(event);
}
