/************************************************************************
 **
 **  @file   opiecelibrarydialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/10/2026
 **
 **  @brief  The pieces saved in the library, and what each of them will ask for.
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
#ifndef OPIECELIBRARYDIALOG_H
#define OPIECELIBRARYDIALOG_H

#include "core/opiecelibrary.h"

#include <QDialog>
#include <QList>
#include <QString>

class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

/**
 * @brief The library of pieces: what it holds, and what each piece needs.
 *
 * A list, and nothing else — no piece is created here, and none is renamed or deleted: this is where a
 * piece is *found*. What the list adds to a folder is the one thing a file name cannot say: a piece that
 * reads a value it does not declare needs another piece, and that other piece is named here before the
 * piece is ever imported.
 */
class OPieceLibraryDialog : public QDialog
{
    Q_OBJECT // NOLINT

public:
    /// @param directory the folder the library is read from — given, never guessed, so the window can be
    ///        pointed at any folder the pieces of a test live in, and so the settings are read once, by
    ///        the caller, where the library's location is decided.
    explicit OPieceLibraryDialog(const QString &directory, QWidget *parent = nullptr);
    ~OPieceLibraryDialog() override = default;

    /// @brief The piece the reader chose, as an absolute path — empty until she chooses one.
    auto SelectedPath() const -> QString;

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void OnSearchChanged(const QString &text);
    void ShowDetails();

private:
    Q_DISABLE_COPY_MOVE(OPieceLibraryDialog) // NOLINT

    void SetupUI();
    void Retranslate();
    void Refresh();
    void Populate();
    void Filter(const QString &filter);

    /// @brief Follow the selection: remember the piece chosen and say whether it can be inserted at all.
    void FollowSelection();

    QLabel *m_searchLabel{nullptr};
    QLineEdit *m_searchEdit{nullptr};
    QTableWidget *m_table{nullptr};
    QLabel *m_detailsLabel{nullptr};
    QDialogButtonBox *m_buttonBox{nullptr};
    QPushButton *m_insertButton{nullptr};

    QList<OPieceLibrary::Summary> m_summaries{};
    QString m_directory{};
    QString m_selectedPath{};
};

#endif // OPIECELIBRARYDIALOG_H
