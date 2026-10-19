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
#include <QPainter>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <utility>

namespace
{
/// Les deux colonnes : le genre d'objet, et le nom qu'il porte.
constexpr int kObject = 0;
constexpr int kAlias = 1;

/// A row tall enough for a field whose text is read, not clipped: the field keeps the theme's own height, and the
/// table no longer cuts 4 px above and below it (see FillAliases).
constexpr int kAliasRowHeight = 36;

/// The list's own height: never smaller than a few rows, never taller than a screen can hold around the rest of
/// the dialog. Between the two, it is the number of aliases that decides, so the whole list is read at once.
constexpr int kAliasListMinHeight = 200;
constexpr int kAliasListMaxHeight = 560;

/**
 * @brief A row that does not answer the pointer.
 *
 * The theme paints every hovered item of an item view in the accent colour — right for a list where the pointer
 * picks a line, wrong here: these rows are text fields, and the field under the mouse is already the one being
 * typed in. Clearing the state before the base delegate paints leaves the row exactly as it is; the selected
 * row keeps its colour, because the selection state is left alone.
 */
class NoHoverDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem plain = option;
        plain.state &= ~QStyle::State_MouseOver;
        QStyledItemDelegate::paint(painter, plain, index);
    }
};
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

    // The field is a cell widget: it is laid on the item's text rect, which the theme's `::item` rule shrinks by
    // 4 px above and below. At the theme's own row height the text box was left 12 px high for a 15 px line, so
    // the text was cut at both ends. The table gets a taller row, and its own item rule without vertical padding
    // gives the field the whole height; the name it carries is what decides the import, and it has to be read.
    ui->tableAliases->setObjectName(QStringLiteral("pieceAliasTable"));
    ui->tableAliases->verticalHeader()->setDefaultSectionSize(kAliasRowHeight);

    // No hover row here: the delegate below drops the state before painting, and the row stays as it is.
    ui->tableAliases->setAlternatingRowColors(false);
    ui->tableAliases->setStyleSheet(QStringLiteral("QTableWidget#pieceAliasTable::item { padding: 0px 8px; }"));
    ui->tableAliases->setItemDelegate(new NoHoverDelegate(ui->tableAliases));

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

    // The dialog is as tall as its aliases, up to what a screen can hold: a list of names that has to be scrolled
    // to be read is a list where a wrong name is easy to leave in place. Past the cap the scroll stays — nothing
    // is gained by a window taller than the screen.
    const int header = ui->tableAliases->horizontalHeader()->sizeHint().height();
    const int content = header + static_cast<int>(m_proposal.aliases.size()) * kAliasRowHeight + 4;
    ui->tableAliases->setMinimumHeight(qBound(kAliasListMinHeight, content, kAliasListMaxHeight));
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
