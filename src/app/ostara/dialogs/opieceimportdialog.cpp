/************************************************************************
 **
 **  @file   opieceimportdialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/10/2026
 **
 **  @brief  How a library piece enters a pattern: the name it takes, and the aliases it carries.
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
#include "opieceimportdialog.h"
#include "ui_opieceimportdialog.h"

#include "../qmuparser/qmudef.h"
#include "vmisc/FactoryWidgets.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <utility>

namespace
{
/// Les deux colonnes : le genre d'objet, et le nom qu'il porte.
constexpr int kObject = 0;
constexpr int kAlias = 1;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OPieceImportDialog::OPieceImportDialog(const OPieceInsert::Proposal &proposal, const QStringList &blockNames,
                                       const QSet<QString> &taken, const QVector<OPieceImport::Reference> &references,
                                       QWidget *parent)
    : QDialog(parent),
      ui(new Ui::OPieceImportDialog),
      m_proposal(proposal),
      m_blockNames(blockNames),
      m_taken(taken),
      m_references(references)
{
    ui->setupUi(this);

    // Le bouton dit ce qu'il fera : la pièce n'est pas créée, elle entre dans un patron qui existe déjà.
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Insert the piece"));
    ui->lineEditName->setText(proposal.instanceName);

    connect(ui->lineEditName, &QLineEdit::textChanged, this, &OPieceImportDialog::CheckState);

    // L'ordre des trois moments de la dialogue : le nom, ce qui est lu ailleurs, puis les alias (§5).
    FillReferences();
    FillAliases();
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
OPieceImportDialog::~OPieceImportDialog()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceImportDialog::InstanceName() const -> QString
{
    return ui->lineEditName->text().trimmed();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceImportDialog::Table() const -> QHash<QString, QString>
{
    // The proposal comes first: it carries the names no alias shows — the points, the items, the details, the
    // offsets — which the mark rewrites all the same. The lines then correct it, one alias at a time.
    QHash<QString, QString> table = m_proposal.table;

    for (int row = 0; row < ui->tableAliases->rowCount(); ++row)
    {
        const QTableWidgetItem *object = ui->tableAliases->item(row, kObject);
        const auto *edit = qobject_cast<QLineEdit *>(ui->tableAliases->cellWidget(row, kAlias));
        if (object == nullptr || edit == nullptr)
        {
            continue;
        }

        const QString before = object->data(Qt::UserRole).toString();
        const QString after = edit->text().trimmed();
        if (after == before)
        {
            table.remove(before); // l'alias garde son nom : la marque ne l'a pas touché, et n'a rien à réécrire
        }
        else
        {
            table.insert(before, after);
        }
    }

    return table;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceImportDialog::ChosenReferences() const -> QHash<QString, QString>
{
    QHash<QString, QString> chosen;
    for (auto box = m_referenceBoxes.constBegin(); box != m_referenceBoxes.constEnd(); ++box)
    {
        if (box.value() != nullptr && box.value()->currentIndex() >= 0)
        {
            chosen.insert(box.key(), box.value()->currentText());
        }
    }
    return chosen;
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceImportDialog::FillReferences()
{
    if (m_references.isEmpty())
    {
        return;
    }

    auto *group = new QGroupBox(tr("Reference pieces"), this);
    auto *layout = new QFormLayout(group);

    for (const OPieceImport::Reference &reference : std::as_const(m_references))
    {
        auto *combo = MakeComboBox(group);
        combo->addItems(reference.candidates);

        // Le bloc qui est entré le premier est celui dont les noms sont ceux que la pièce écrit déjà : c'est le
        // choix par défaut, et le seul qui n'ait rien à réécrire.
        combo->setCurrentIndex(0);

        layout->addRow(tr("Read %1 from:", "the piece this one reads values in").arg(reference.origin), combo);
        m_referenceBoxes.insert(reference.origin, combo);
    }

    // Le groupe se glisse avant les boutons, après le nom et ses alias : le troisième moment de la dialogue (§5).
    if (auto *vertical = findChild<QVBoxLayout *>(QStringLiteral("verticalLayout")); vertical != nullptr)
    {
        vertical->insertWidget(vertical->count() - 1, group);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceImportDialog::CheckState()
{
    const QString name = InstanceName();
    QString problem;
    if (name.isEmpty())
    {
        problem = tr("The piece needs a name.");
    }
    else if (m_blockNames.contains(name))
    {
        problem = tr("A piece of this pattern already wears this name.");
    }
    SetProblem(ui->labelNameProblem, problem);

    QString aliasProblem;
    QSet<QString> seen;
    for (int row = 0; row < ui->tableAliases->rowCount() && aliasProblem.isEmpty(); ++row)
    {
        const auto *edit = qobject_cast<QLineEdit *>(ui->tableAliases->cellWidget(row, kAlias));
        if (edit == nullptr)
        {
            continue;
        }

        const QString typed = edit->text().trimmed();
        aliasProblem = AliasProblem(typed);
        if (aliasProblem.isEmpty() && seen.contains(typed))
        {
            aliasProblem = tr("Two names cannot be the same.");
        }
        seen.insert(typed);
    }
    SetProblem(ui->labelAliasProblem, aliasProblem);

    // Rien n'est écrit tant que tout n'est pas décidable : un nom déjà porté, une forme que le schéma refuse, ou
    // deux alias qui ne feraient qu'un laissent le bouton hors de portée.
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(problem.isEmpty() && aliasProblem.isEmpty());
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceImportDialog::FillAliases()
{
    ui->tableAliases->setColumnCount(2);
    ui->tableAliases->setHorizontalHeaderLabels({tr("Object"), tr("Name")});
    ui->tableAliases->verticalHeader()->setVisible(false);

    // Les cellules ne s'éditent pas : le champ de droite est le widget, et c'est lui qui porte la décision.
    ui->tableAliases->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableAliases->setRowCount(static_cast<int>(m_proposal.aliases.size()));

    for (int row = 0; row < m_proposal.aliases.size(); ++row)
    {
        const OPieceMark::Alias &alias = m_proposal.aliases.at(row);

        auto *object = new QTableWidgetItem(ObjectText(alias));
        object->setData(Qt::UserRole, alias.name); // le nom d'avant : c'est lui que la table des renommages indexe
        object->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        ui->tableAliases->setItem(row, kObject, object);

        // Le pré-rempli est la proposition : le nom de la pièce quand le patron ne répond à rien de ce qu'elle
        // porte, le même marqué quand il répond à quelque chose.
        auto *edit = new QLineEdit(m_proposal.table.value(alias.name, alias.name), ui->tableAliases);
        edit->setValidator(new QRegularExpressionValidator(QRegularExpression(NameRegExp()), edit));
        connect(edit, &QLineEdit::textChanged, this, &OPieceImportDialog::CheckState);
        ui->tableAliases->setCellWidget(row, kAlias, edit);
    }

    ui->tableAliases->resizeColumnsToContents();
    ui->tableAliases->horizontalHeader()->setSectionResizeMode(kObject, QHeaderView::ResizeToContents);
    ui->tableAliases->horizontalHeader()->setSectionResizeMode(kAlias, QHeaderView::Stretch);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceImportDialog::ObjectText(const OPieceMark::Alias &alias) -> QString
{
    // Le genre d'objet, et rien de plus : le nom qu'on lit à droite est celui que quelqu'un a donné, donc il sait
    // de quoi il parle.
    switch (alias.of)
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
void OPieceImportDialog::SetProblem(QLabel *label, const QString &text)
{
    label->setText(text);
    label->setVisible(not text.isEmpty());
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceImportDialog::AliasProblem(const QString &typed) const -> QString
{
    if (typed.isEmpty())
    {
        return tr("A name cannot be empty.");
    }

    if (QRegularExpression const rx(NameRegExp()); not rx.match(typed).hasMatch())
    {
        return tr("A name cannot hold a space nor a reserved character.");
    }

    if (OPieceMark::AnswersTo(m_taken, typed))
    {
        return tr("The pattern already answers to %1.").arg(typed);
    }

    return {};
}
