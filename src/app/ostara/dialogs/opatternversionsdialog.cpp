/************************************************************************
 **
 **  @file   opatternversionsdialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/11/2026
 **
 **  @brief  The versions of the pattern: the switch, the names, and the
 **          pieces that would be left without one.
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

#include "opatternversionsdialog.h"

#include "../core/opatternversions.h"
#include "../ifc/exception/vexceptionbadid.h"
#include "../vmisc/def.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/FactoryWidgets.h"
#include "../vmisc/vabstractapplication.h"
#include "../vtheme/vtheme.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vtools/undocommands/savepieceoptions.h"

#include <QCheckBox>
#include <QFrame>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QScreen>
#include <QScrollArea>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QIcon>
#include <QSize>
#include <QToolButton>
#include <QUndoStack>
#include <QVBoxLayout>

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/// @brief Where the pieces of a version about to disappear go.
///
/// A `MakeComboBox`, like every other choice in the application: the native macOS popup ignores the theme's
/// sheets, so a plain completion box would come out unthemed.
auto ChooseVersion(QWidget *parent, const QString &label, const QList<VPatternVersionData> &versions,
                   quint32 *chosen) -> bool
{
    QDialog dialog(parent);
    dialog.setWindowTitle(QCoreApplication::translate("OPatternVersionsDialog", "Pattern versions"));

    auto *layout = new QVBoxLayout(&dialog);
    auto *text = new QLabel(label, &dialog);
    text->setWordWrap(true);
    layout->addWidget(text);

    auto *combo = MakeComboBox(&dialog);
    for (const VPatternVersionData &version : versions)
    {
        combo->addItem(version.name, version.id);
    }
    layout->addWidget(combo);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
    {
        return false;
    }
    if (chosen != nullptr)
    {
        *chosen = combo->currentData().toUInt();
    }
    return true;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OPatternVersionsDialog::OPatternVersionsDialog(VAbstractPattern *doc, QWidget *parent)
  : QDialog(parent),
    m_doc(doc)
{
    SCASSERT(m_doc != nullptr)

    setWindowTitle(tr("Pattern versions"));
    setModal(true);

    auto *layout = new QVBoxLayout(this);
    // **The dialog never gets smaller than what it holds.** Without this, a list that grows keeps the height
    // it had: the rows overflow their container and the button below covers them (measured: dialog 263 px
    // for content asking 282, the list losing one row's height). The constraint is a minimum, not a fixed
    // size — the reader can still enlarge the dialog.
    layout->setSizeConstraint(QLayout::SetMinimumSize);

    m_multiCheck = new QCheckBox(tr("Pattern with several versions"));
    m_multiCheck->setChecked(m_doc->IsMultiVersion());
    layout->addWidget(m_multiCheck);

    m_hint = new QLabel(tr("A piece belongs to one version. A group belongs to every version of its members."));
    m_hint->setWordWrap(true);
    layout->addWidget(m_hint);

    // ── The list: grows with the dialog, scrolls when it no longer fits ──────────────────────────
    //
    // The dialog takes the height the list asks for, up to half the screen; past that the list scrolls
    // instead of being cropped (a version hidden under the button was the defect). The scroll area restates
    // the no-colour rule: a QScrollArea viewport has a native background on macOS (SubtiliteCode §13), and
    // transparent is what keeps the dialog's own colour.
    m_rowsScroll = new QScrollArea(this);
    m_rowsScroll->setWidgetResizable(true);
    m_rowsScroll->setFrameShape(QFrame::NoFrame);
    m_rowsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_rowsScroll->viewport()->setStyleSheet(QStringLiteral(".QWidget { background: transparent; }"));

    // A QFrame, not a bare QWidget: a dock's `.QWidget { background }` rule cascades into a child
    // dialog and colours every bare QWidget in it (SubtiliteCode §14f). The dot selector does not match
    // a QFrame, so the rows sit on the dialog's own background.
    m_rowsHost = new QFrame();
    m_rowsHost->setFrameStyle(QFrame::NoFrame);
    m_rowsLayout = new QVBoxLayout(m_rowsHost);
    m_rowsLayout->setContentsMargins(0, 0, 0, 0);
    m_rowsLayout->setSpacing(4);
    m_rowsScroll->setWidget(m_rowsHost);
    layout->addWidget(m_rowsScroll, 1);

    m_addButton = new QPushButton(tr("Add a version"));
    auto *addRow = new QHBoxLayout();
    addRow->addWidget(m_addButton);
    addRow->addStretch();
    layout->addLayout(addRow);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_addButton, &QPushButton::clicked, this, &OPatternVersionsDialog::AddVersion);
    connect(m_multiCheck, &QCheckBox::toggled, this, &OPatternVersionsDialog::OnMultiVersionToggled);
    // A trash icon is a themed drawing: a theme switch while the dialog is open must not leave the old one.
    connect(VTheme::Instance(), &VTheme::ThemeSettingsChanged, this, &OPatternVersionsDialog::Rebuild);

    Rebuild();
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternVersionsDialog::Rebuild()
{
    const QSignalBlocker blocker(m_multiCheck);

    while (QLayoutItem *item = m_rowsLayout->takeAt(0))
    {
        if (QWidget *widget = item->widget())
        {
            // Deleted later, never here: the row that asked for the rebuild is one of them when a
            // button was clicked, and deleting the sender of a signal under its own slot is a crash.
            widget->hide();
            widget->deleteLater();
        }
        delete item;
    }

    m_multiCheck->setChecked(m_doc->IsMultiVersion());

    const quint32 allVersions = m_doc->AllVersionsId();
    const QVector<VPatternVersionData> versions = m_doc->GetPatternVersions();

    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
    const QIcon trashIcon(QStringLiteral(":/icons/%1/trash_%1.svg").arg(dark ? QStringLiteral("dark")
                                                                          : QStringLiteral("light")));

    for (const VPatternVersionData &version : versions)
    {
        // Same reason as the host: a bare QWidget would be painted by the dock's dot selector.
        auto *row = new QFrame(m_rowsHost);
        row->setFrameStyle(QFrame::NoFrame);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 2, 0, 2);

        auto *edit = new QLineEdit(version.name, row);
        edit->setEnabled(m_doc->IsMultiVersion());
        // A field is never narrower than its own text asks: a squeezed row cropped the names, and a name
        // the reader cannot read is a name she cannot check.
        edit->setMinimumHeight(edit->fontMetrics().height() + 10);
        if (version.id == allVersions)
        {
            // Not a version among others: it is where the pieces that belong to none live.
            edit->setToolTip(tr("Every piece that belongs to no particular version belongs to this one. "
                                "It cannot be deleted."));
        }
        rowLayout->addWidget(edit, 1);

        if (version.id != allVersions)
        {
            // The icon every delete wears in this application — see Interface/references/Icons.md.
            auto *remove = new QToolButton(row);
            remove->setIcon(trashIcon);
            remove->setIconSize(QSize(16, 16));
            remove->setToolTip(tr("Delete this version"));
            remove->setAutoRaise(true);
            rowLayout->addWidget(remove);

            const quint32 id = version.id;
            connect(remove, &QToolButton::clicked, this, [this, id]() { RemoveVersion(id); });
        }

        const quint32 id = version.id;
        connect(edit, &QLineEdit::editingFinished, this,
                [this, id, edit]() { CommitName(id, edit->text(), edit); });

        m_rowsLayout->addWidget(row);
        // **A child built after its parent is visible stays hidden until it is shown.** Without this the
        // rows existed but the layout counted none of them: the list measured 0, the dialog shrank, and
        // the whole list went blank until the dialog was reopened (measured: content 0 for two rows).
        row->show();
    }

    UpdateControls();

    // **A row built while the dialog is open has not been polished yet.** Its size hint is the unstyled
    // one, so the row comes out shorter than its own text — and only when it was added, because a dialog
    // that reopens builds its rows before showing them. Polish them, then let the dialog take the height
    // the list now asks for: adding a version must show a version, not a cropped one.
    const QList<QWidget *> widgets = m_rowsHost->findChildren<QWidget *>();
    for (QWidget *widget : widgets)
    {
        widget->ensurePolished(); // the row, and its field and its button: each one's hint is the styled one
    }

    // **A row never comes out shorter than its own field.** A new row makes the dialog ask for more height; if
    // the dialog kept its former height, the rows were compressed — the field overflowed its row and its text
    // was cut. Fixed height on the row, and the dialog grows to what the new list needs.
    for (QFrame *row : m_rowsHost->findChildren<QFrame *>(QString(), Qt::FindDirectChildrenOnly))
    {
        row->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        row->setMinimumHeight(row->sizeHint().height());
    }

    // The list takes the height its rows ask for, never more than half the screen: past that it scrolls.
    const int content = m_rowsHost->sizeHint().height();
    const QScreen *screen = this->screen() != nullptr ? this->screen() : QGuiApplication::primaryScreen();
    const int available = screen != nullptr ? screen->availableGeometry().height() : 900;
    const int listHeight = qMin(content, qMax(available / 2, 160));
    m_rowsScroll->setMinimumHeight(listHeight);
    m_rowsScroll->setMaximumHeight(listHeight);

    if (QLayout *dialogLayout = layout(); dialogLayout != nullptr)
    {
        dialogLayout->activate();
        adjustSize(); // the size the list now asks for, never less (see the minimum-size constraint)
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternVersionsDialog::UpdateControls()
{
    m_addButton->setEnabled(m_doc->IsMultiVersion());
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternVersionsDialog::AddVersion()
{
    m_doc->AddPatternVersion(UniqueName());
    Rebuild();
    emit VersionsChanged();

    // The new row is the last one: put the caret in it, so naming it is typing rather than aiming.
    if (QLayoutItem *item = m_rowsLayout->itemAt(m_rowsLayout->count() - 1))
    {
        if (QWidget *row = item->widget())
        {
            if (auto *edit = row->findChild<QLineEdit *>())
            {
                edit->setFocus();
                edit->selectAll();
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternVersionsDialog::RemoveVersion(quint32 id)
{
    const QList<quint32> pieces = PiecesWearing(*m_doc, id);
    const QString name = m_doc->GetPatternVersion(id).name;
    const QString allName = m_doc->GetPatternVersion(m_doc->AllVersionsId()).name;

    if (!pieces.isEmpty())
    {
        // **Where do the pieces go?** The reader decides whenever there is something to decide: the entry
        // that covers every version, or any of the versions that remain. Only when nothing but "all
        // versions" would be left is the destination not a question — then the box simply says so.
        QList<VPatternVersionData> destinations;
        for (const VPatternVersionData &version : m_doc->GetPatternVersions())
        {
            if (version.id != id)
            {
                destinations.append(version);
            }
        }

        quint32 destination = m_doc->AllVersionsId();
        if (destinations.size() > 1)
        {
            const QString label =
                pieces.size() == 1
                    ? tr("One piece wears the version \"%1\". Move it to:").arg(name)
                    : tr("%1 pieces wear the version \"%2\". Move them to:")
                          .arg(QString::number(pieces.size()), name);

            if (!ChooseVersion(this, label, destinations, &destination))
            {
                return; // refused: the version stays, and so do the pieces' versions
            }
        }
        else
        {
            const QString question =
                pieces.size() == 1
                    ? tr("One piece wears the version \"%1\". Move it to \"%2\" and delete the version?")
                          .arg(name, allName)
                    : tr("%1 pieces wear the version \"%2\". Move them all to \"%3\" and delete the version?")
                          .arg(QString::number(pieces.size()), name, allName);
            if (!ConfirmReassign(this, question, allName))
            {
                return;
            }
        }

        ReassignPieces(m_doc, pieces, destination);
    }

    m_doc->RemovePatternVersion(id);
    Rebuild();
    emit VersionsChanged();
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternVersionsDialog::CommitName(quint32 id, const QString &name, QLineEdit *editor)
{
    const QString trimmed = name.trimmed();
    const QString current = m_doc->GetPatternVersion(id).name;
    if (trimmed == current)
    {
        return;
    }

    bool used = false;
    for (const VPatternVersionData &version : m_doc->GetPatternVersions())
    {
        if (version.id != id && version.name.compare(trimmed, Qt::CaseInsensitive) == 0)
        {
            used = true;
        }
    }

    if (trimmed.isEmpty() || used)
    {
        // The name is data every piece is read against: "two versions called A" would make the
        // assignments nonsense, and "" would name nothing at all.
        QMessageBox::warning(this, tr("Pattern versions"),
                             tr("A version needs a name, and two versions cannot share one."));
        if (editor != nullptr)
        {
            editor->setText(current);
        }
        return;
    }

    if (m_doc->RenamePatternVersion(id, trimmed))
    {
        emit VersionsChanged();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternVersionsDialog::OnMultiVersionToggled(bool on)
{
    if (on)
    {
        m_doc->SetMultiVersion(true);
        Rebuild();
        emit VersionsChanged();
        return;
    }

    if (!SwitchOffWithReassignment(m_doc, this))
    {
        // Refused: the switch goes back where it was, and nothing else changes.
        const QSignalBlocker blocker(m_multiCheck);
        m_multiCheck->setChecked(true);
        return;
    }

    Rebuild();
    emit VersionsChanged();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternVersionsDialog::SwitchOffWithReassignment(VAbstractPattern *doc, QWidget *parent) -> bool
{
    Q_ASSERT(doc != nullptr);

    // Switching off is a decision about the pieces, not only about the list: a piece wearing "Version
    // A" would otherwise keep an id pointing at a list nobody shows.
    QList<quint32> pieces;
    const quint32 allVersions = doc->AllVersionsId();
    for (const VPatternVersionData &version : doc->GetPatternVersions())
    {
        if (version.id != allVersions)
        {
            pieces.append(PiecesWearing(*doc, version.id));
        }
    }

    const QString allName = doc->GetPatternVersion(allVersions).name;
    if (!pieces.isEmpty())
    {
        const QString question =
            pieces.size() == 1
                ? QCoreApplication::translate("OPatternVersionsDialog",
                                              "One piece wears a version. Move it to \"%1\" and switch the versions off?")
                      .arg(allName)
                : QCoreApplication::translate("OPatternVersionsDialog",
                                              "%1 pieces wear a version. Move them all to \"%2\" and switch the versions off?")
                      .arg(QString::number(pieces.size()), allName);
        if (!ConfirmReassign(parent, question, allName))
        {
            return false;
        }
        ReassignPieces(doc, pieces, allVersions);
    }

    doc->SetMultiVersion(false);
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternVersionsDialog::PiecesWearing(const VAbstractPattern &doc, quint32 id) -> QList<quint32>
{
    return OPatternVersions::PieceIdsUsing(doc, id);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternVersionsDialog::ConfirmReassign(QWidget *parent, const QString &question,
                                            const QString &allName) -> bool
{
    QMessageBox box(parent);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(QCoreApplication::translate("OPatternVersionsDialog", "Pattern versions"));
    box.setText(question);
    box.setInformativeText(
        QCoreApplication::translate("OPatternVersionsDialog",
                                    "The pieces stay in \"%1\" — the one every piece of the pattern belongs to.")
            .arg(allName));
    QPushButton *yes = box.addButton(QCoreApplication::translate("OPatternVersionsDialog", "Yes, move them"),
                                     QMessageBox::AcceptRole);
    box.addButton(QCoreApplication::translate("OPatternVersionsDialog", "No, keep the version"),
                  QMessageBox::RejectRole);
    box.setDefaultButton(yes);
    box.exec();
    return box.clickedButton() == yes;
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternVersionsDialog::ReassignPieces(VAbstractPattern *doc, const QList<quint32> &pieceIds,
                                            quint32 destinationId)
{
    if (pieceIds.isEmpty())
    {
        return;
    }

    QUndoStack *undoStack = VAbstractApplication::VApp()->getUndoStack();
    undoStack->beginMacro(QCoreApplication::translate("OPatternVersionsDialog", "Move the pieces to \"%1\"")
                              .arg(doc->GetPatternVersion(destinationId).name));

    for (const quint32 pieceId : pieceIds)
    {
        auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::FindTool(pieceId));
        if (tool == nullptr)
        {
            continue;
        }

        const VContainer *data = tool->getData();
        if (data == nullptr)
        {
            continue;
        }

        try
        {
            const VPiece oldDet = data->GetPiece(pieceId);
            VPiece newDet = oldDet;
            newDet.SetPatternVersion(destinationId);
            undoStack->push(new SavePieceOptions(oldDet, newDet, doc, pieceId));
        }
        catch (const VExceptionBadId &)
        {
            continue; // a piece the scene does not hold: nothing to move
        }
    }

    undoStack->endMacro();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternVersionsDialog::UniqueName() const -> QString
{
    const QVector<VPatternVersionData> versions = m_doc->GetPatternVersions();
    for (int number = 1;; ++number)
    {
        const QString candidate = tr("Version %1").arg(number);
        bool used = false;
        for (const VPatternVersionData &version : versions)
        {
            if (version.name.compare(candidate, Qt::CaseInsensitive) == 0)
            {
                used = true;
                break;
            }
        }
        if (!used)
        {
            return candidate;
        }
    }
}
