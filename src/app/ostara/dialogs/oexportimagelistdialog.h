/************************************************************************
 **
 **  @file   oexportimagelistdialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   23/09/2026
 **
 **  @brief  Export mode — which generations to write, and under which names.
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

#ifndef OEXPORTIMAGELISTDIALOG_H
#define OEXPORTIMAGELISTDIALOG_H

#include "../oexportplanimagewriter.h"

#include <QDialog>
#include <QString>
#include <QVector>

class QCheckBox;
class QDialogButtonBox;
class QEvent;
class QLineEdit;

/**
 * @brief The second step of an image export: one line per generation, ticked and named.
 *
 * The first step said where the files go, what they are and how big — everything that is the
 * same for all of them. What is left is per generation, and it is exactly this: which ones, and
 * what each file is called. One line per generation, ticked by default, because a batch is
 * asked for when most of them are wanted and unticking two is less work than ticking four.
 *
 * **A column of lines, and not a table.** A table brings a header that teaches nothing — a box
 * and a text field say what they are — and it brings cell hover, cell selection and a row height
 * that squeezes whatever is put inside. What this dialog holds is a checkbox and a field, so that
 * is what it is made of.
 *
 * **The name is offered rather than typed** — it opens on the generation's own name, the one the
 * dock lists it under — and it stays editable, because the file is a delivery and a delivery is
 * named for whoever receives it. The suffix follows the format chosen in the first step: a file
 * called "plan.png" holding an SVG is a name that lies about its own content.
 *
 * A dialog with ONE line is the same dialog. Asking for the name of a single generation and
 * asking for the names of six are one question with one answer each, and a second dialog that
 * would hold a single text field would be a place for the two to differ.
 */
class OExportImageListDialog : public QDialog
{
    Q_OBJECT

public:
    /// One generation to write, and what it will be called.
    struct Row
    {
        quint32 id{0};
        QString name{}; ///< the FILE name, suffix included — what the user reads and edits
    };

    /// @param rows   the generations to offer, in the order they will be written.
    /// @param format the file's format, whose suffix the names carry.
    explicit OExportImageListDialog(const QVector<Row> &rows, OExportPlanImageWriter::Format format,
                                    QWidget *parent = nullptr);

    /// @brief The lines still ticked, with the names as they stand, ready to be written.
    ///
    /// A name left empty is not a name: the line is dropped, and Export says so before closing.
    auto Chosen() const -> QVector<Row>;

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void OnAccept();

private:
    void Retranslate();

    /// The name of @p row as it is typed, suffix added where it is missing. One reader, so that
    /// what the line shows and what the run writes cannot come from two spellings.
    auto NameOf(int row) const -> QString;

    auto Suffix() const -> QString;

    QVector<Row> m_rows{};
    OExportPlanImageWriter::Format m_format{OExportPlanImageWriter::Format::Svg};

    /// One box and one field per generation, in the order they are written. Held rather than
    /// looked up: there is no table to ask, which is the point of the thing.
    QVector<QCheckBox *> m_ticks{};
    QVector<QLineEdit *> m_names{};

    QDialogButtonBox *m_buttonBox{nullptr};
};

#endif // OEXPORTIMAGELISTDIALOG_H
