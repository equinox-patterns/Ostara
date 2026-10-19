/************************************************************************
 **
 **  @file   osavepiecedialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/10/2026
 **
 **  @brief  The name a piece will wear in the library, asked before anything is written.
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

#include "osavepiecedialog.h"

#include "ui_osavepiecedialog.h"

#include "olibraryfolderdialog.h"

#include "core/opiecelibrary.h"
#include "def.h"
#include "../qmuparser/qmudef.h"
#include "widgets/olibraryicondelegate.h"
#include "widgets/olibrarytree.h"

#include <QDir>
#include <QEvent>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSet>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTreeWidget>

//---------------------------------------------------------------------------------------------------------------------
OSavePieceDialog::OSavePieceDialog(const QString &pieceName, const QString &libraryDirectory,
                                   const QVector<OPieceName::Blocker> &blockers, const QSet<QString> &taken,
                                   QWidget *parent)
    : QDialog(parent),
      ui(new Ui::OSavePieceDialog),
      m_library(libraryDirectory),
      m_blockers(blockers),
      m_taken(taken)
{
    ui->setupUi(this);

    // The same tree the library's window shows, down to its object name: the theme draws its branch carets from
    // that name, and a tree that looked different here would look like another tree.
    ui->treeFolder->setObjectName(QStringLiteral("libraryTree"));
    ui->treeFolder->setIconSize(QSize(20, 20));
    ui->treeFolder->setItemDelegate(new OLibraryRowDelegate(ui->treeFolder));
    connect(ui->treeFolder, &QTreeWidget::itemSelectionChanged, this, &OSavePieceDialog::ShowDestination);
    connect(ui->createFolderButton, &QPushButton::clicked, this, &OSavePieceDialog::OnCreateFolder);

    // The piece's own name, as a starting point and not as a decision: the library is shared, and two
    // patterns can hold a piece of the same name without it being the same piece.
    ui->lineEditName->setText(pieceName);

    FillBlockers();
    PopulateTree();
    CheckState();
    connect(ui->lineEditName, &QLineEdit::textChanged, this, &OSavePieceDialog::CheckState);
}

//---------------------------------------------------------------------------------------------------------------------
void OSavePieceDialog::PopulateTree()
{
    const QString current = OLibraryTree::SelectedPath(ui->treeFolder);

    // Rebuilding the tree clears the selection, and that is not the reader choosing the library's own folder:
    // the signals are cut, and the folder is put back before the destination is said.
    const QSignalBlocker blocker(ui->treeFolder);

    // Here the library's own folder **is** a line: a destination has to be visible at all times.
    OLibraryTree::Fill(ui->treeFolder, m_library, true, tr("The library's own folder"),
                       OLibraryIcon(QStringLiteral("folder-open")));
    OLibraryTree::Select(ui->treeFolder, current.isEmpty() ? m_library : current);

    ShowDestination();
}

//---------------------------------------------------------------------------------------------------------------------
auto OSavePieceDialog::TargetFolder() const -> QString
{
    const QString selected = OLibraryTree::SelectedPath(ui->treeFolder);
    return selected.isEmpty() ? m_library : selected;
}

//---------------------------------------------------------------------------------------------------------------------
auto OSavePieceDialog::LibraryPath() const -> QString
{
    return QDir(TargetFolder()).absoluteFilePath(name() + QLatin1Char('.') + OPieceLibrary::Extension());
}

//---------------------------------------------------------------------------------------------------------------------
void OSavePieceDialog::ShowDestination()
{
    const QString name = ui->lineEditName->text();

    // The consequence in plain words: a tree and a name do not make a path by themselves, and the reader is
    // about to write a file she will have to find again.
    ui->labelDestination->setText(name.isEmpty() ? tr("Give the file a name.")
                                                 : tr("Will be saved in: %1").arg(TargetFolder()));
}

//---------------------------------------------------------------------------------------------------------------------
void OSavePieceDialog::OnCreateFolder()
{
    OLibraryFolderDialog dialog(TargetFolder(), this);
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    // The folder created becomes the destination, since that is why it was made.
    const QString created = dialog.CreatedPath();
    PopulateTree();

    const QSignalBlocker blocker(ui->treeFolder);
    OLibraryTree::Select(ui->treeFolder, created);
    ShowDestination();
}

//---------------------------------------------------------------------------------------------------------------------
void OSavePieceDialog::FillBlockers()
{
    // Rien à nommer : la fenêtre est celle d'avant, et le groupe n'existe pas pour l'utilisateur.
    if (m_blockers.isEmpty())
    {
        ui->groupNames->setVisible(false);

        // La géométrie du .ui réserve la hauteur du groupe (430) : caché, il ne prend plus de place mais la
        // fenêtre la garde, et le vide se voit sous les notes. Elle se réduit donc à ce qu'elle montre — en
        // gardant la largeur que le .ui lui donne, sinon elle rétrécirait aussi en largeur.
        const int width = this->width();
        adjustSize();
        resize(width, this->height());
        return;
    }

    ui->tableNames->setColumnCount(3);
    ui->tableNames->setHorizontalHeaderLabels({tr("Object"), tr("Name"), tr("Called in")});
    ui->tableNames->verticalHeader()->setVisible(false);
    ui->tableNames->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableNames->setRowCount(static_cast<int>(m_blockers.size()));

    for (int row = 0; row < m_blockers.size(); ++row)
    {
        const OPieceName::Blocker &blocker = m_blockers.at(row);

        auto *object = new QTableWidgetItem(ObjectText(blocker.of));
        object->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        ui->tableNames->setItem(row, 0, object);

        // Le nom proposé est celui que la pièce emploie aujourd'hui : le garder est une réponse, le changer en
        // est une autre, et c'est la formule de droite qui dit dans quel contexte il faut le décider.
        auto *edit = new QLineEdit(blocker.name, ui->tableNames);
        edit->setValidator(new QRegularExpressionValidator(QRegularExpression(NameRegExp()), edit));
        connect(edit, &QLineEdit::textChanged, this, &OSavePieceDialog::CheckState);
        ui->tableNames->setCellWidget(row, 1, edit);

        auto *formula = new QTableWidgetItem(blocker.formula);
        formula->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        formula->setToolTip(blocker.formula);
        ui->tableNames->setItem(row, 2, formula);
    }

    ui->tableNames->resizeColumnsToContents();
    ui->tableNames->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
}

//---------------------------------------------------------------------------------------------------------------------
auto OSavePieceDialog::ObjectText(const OPieceMark::AliasOf &of) const -> QString
{
    switch (of)
    {
        case OPieceMark::AliasOf::Line:
            return tr("Line");
        case OPieceMark::AliasOf::Arc:
            return tr("Arc");
        case OPieceMark::AliasOf::EllipticalArc:
            return tr("Elliptical arc");
        case OPieceMark::AliasOf::Curve:
            break;
    }

    return tr("Curve");
}

//---------------------------------------------------------------------------------------------------------------------
auto OSavePieceDialog::NameProblem(const QString &typed, const QStringList &others) const -> QString
{
    if (typed.isEmpty())
    {
        return tr("A name cannot be empty.");
    }

    if (QRegularExpression const rx(NameRegExp()); not rx.match(typed).hasMatch())
    {
        return tr("A name cannot hold a space nor a reserved character.");
    }

    if (others.contains(typed))
    {
        return tr("Two names cannot be the same.");
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
auto OSavePieceDialog::Names() const -> QStringList
{
    QStringList names;
    names.reserve(m_blockers.size());
    for (int row = 0; row < m_blockers.size(); ++row)
    {
        const auto *edit = qobject_cast<QLineEdit *>(ui->tableNames->cellWidget(row, 1));
        names.append(edit == nullptr ? QString() : edit->text().trimmed());
    }

    return names;
}

//---------------------------------------------------------------------------------------------------------------------
OSavePieceDialog::~OSavePieceDialog()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
void OSavePieceDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);

        // The tree's only word is the library's own folder, and the destination sentence is built rather than
        // written in the .ui: both are said again, in the language the dialogue now speaks.
        PopulateTree();
    }

    QDialog::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
auto OSavePieceDialog::name() const -> QString
{
    return ui->lineEditName->text();
}

//---------------------------------------------------------------------------------------------------------------------
auto OSavePieceDialog::notes() const -> QString
{
    return ui->plainTextEditNotes->toPlainText();
}

//---------------------------------------------------------------------------------------------------------------------
void OSavePieceDialog::SetNotes(const QString &notes)
{
    ui->plainTextEditNotes->setPlainText(notes);
}

//---------------------------------------------------------------------------------------------------------------------
void OSavePieceDialog::CheckState()
{
    QPushButton *bOk = ui->buttonBox->button(QDialogButtonBox::Ok);
    SCASSERT(bOk != nullptr)

    // The name becomes a file name in the library, so it has to be able to be one: a name holding a path
    // separator would write the piece into a folder the library does not scan, where nobody would find it
    // again. An empty field needs no explanation; a separator does, so the hint says which rule is not met
    // instead of leaving a dead button.
    const QString name = ui->lineEditName->text();
    const bool isFileName = not name.contains(QLatin1Char('/')) && not name.contains(QLatin1Char('\\'));

    ui->labelHint->setVisible(not name.isEmpty() && not isFileName);

    // Chaque objet sans nom doit recevoir un nom que le schéma accepte, et deux objets ne peuvent pas porter le
    // même : tant qu'un champ ne dit rien de valable, l'enregistrement n'est pas à portée.
    QString problem;
    const QStringList names = Names();
    for (int row = 0; row < names.size() && problem.isEmpty(); ++row)
    {
        QStringList others = names;
        others.removeAt(row);
        problem = NameProblem(names.at(row), others);

        // Un alias déjà porté par un autre objet du patron n'est pas libre — et l'objet qu'on nomme ici ne se
        // refuse pas lui-même : c'est **son** nom d'aujourd'hui, celui que la fenêtre propose, qu'on écarte.
        if (problem.isEmpty() && row < m_blockers.size())
        {
            QSet<QString> taken = m_taken;
            taken.remove(m_blockers.at(row).name);
            if (OPieceMark::AnswersTo(taken, names.at(row)))
            {
                problem = tr("The pattern already answers to %1.").arg(names.at(row));
            }
        }
    }
    ui->labelNamesProblem->setText(problem);
    ui->labelNamesProblem->setVisible(not problem.isEmpty());

    bOk->setEnabled(not name.isEmpty() && isFileName && problem.isEmpty());

    ShowDestination();
}
