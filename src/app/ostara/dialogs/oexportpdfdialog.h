/************************************************************************
 **
 **  @file   oexportpdfdialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  Export mode — where the PDF of a generation goes.
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

#ifndef OEXPORTPDFDIALOG_H
#define OEXPORTPDFDIALOG_H

#include <QDialog>
#include <QString>

class QDialogButtonBox;
class QEvent;
class QLabel;
class QCheckBox;
class QLineEdit;
class QPushButton;

/**
 * @brief Where the PDF of a generation is written: a name, a folder, and one choice.
 *
 * Two fields for the delivery — a name and a folder — and one switch for the paper. Everything
 * else the run needs is already settled: the generation is filed, its geometry frozen, its page
 * given by the bin.
 *
 * The name is offered rather than typed: it opens on the generation's own name, which is
 * what the list calls it and what a folder of six of them wants to read. It stays editable
 * because the file is a delivery, and a delivery is named for whoever receives it.
 *
 * ⚠️ The switch is the one thing here that is not about the file, and the header used to say
 * « Two fields and nothing else » — which was true until the decor layer could be offered or
 * withheld. It is OFF by default, and that default is the point rather than a taste: a plan
 * whose decor is a layer can be printed without its cartouche, its folds and its annotations
 * and look perfectly normal. Printers untick boxes. So the safe answer is the one the dialog
 * opens on, and the layer is something the user has to ask for.
 */
class OExportPdfDialog : public QDialog
{
    Q_OBJECT

public:
    /// @param outputName the generation's name, offered as the file name.
    /// @param directory  where it opens; the folder last used, or Documents.
    explicit OExportPdfDialog(const QString &outputName, const QString &directory,
                              QWidget *parent = nullptr);

    /// @brief The file name, with a .pdf suffix — added here if the user left it off.
    auto FileName() const -> QString;

    /// @brief The folder, as typed or browsed to.
    auto Directory() const -> QString;

    /// @brief Folder and file name together.
    auto FullPath() const -> QString;

    /// @brief Whether the decor is written as one of the reader's layers.
    ///
    /// False — the default — draws the decor exactly as it always was and leaves it out of the
    /// optional content altogether: no entry in the reader's panel, nothing to switch off, and
    /// the underlay with it. True offers it beside the sizes, where it can be turned off like
    /// any other layer.
    auto AddLayoutLayer() const -> bool;

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void OnBrowse();
    void OnAccept();

private:
    void Retranslate();

    QLabel *m_nameLabel{nullptr};
    QLabel *m_pathLabel{nullptr};
    QLineEdit *m_nameEdit{nullptr};
    QLineEdit *m_pathEdit{nullptr};
    QPushButton *m_browseButton{nullptr};
    QCheckBox *m_layoutLayerCheck{nullptr};
    QDialogButtonBox *m_buttonBox{nullptr};
};

#endif  // OEXPORTPDFDIALOG_H
