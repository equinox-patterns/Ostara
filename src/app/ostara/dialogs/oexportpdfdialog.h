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

#include "oexportfiledialog.h"

class QCheckBox;
class QEvent;

/**
 * @brief Where the PDF of a generation is written: the file, and one choice about the paper.
 *
 * Le nom, le dossier, le contrôle du dossier et la question de l'écrasement sont ceux de
 * `OExportFileDialog` : un seul endroit les définit, et le `.pdf` y entre par le suffixe.
 *
 * Ce que ce dialogue ajoute est la **seule** chose qui soit propre au PDF : une case pour écrire le
 * décor du plan comme un calque du document, que le lecteur peut éteindre.
 *
 * ⚠️ The switch is the one thing here that is not about the file, and this header used to say
 * « Two fields and nothing else » — which was true until the decor layer could be offered or
 * withheld. It is OFF by default, and that default is the point rather than a taste: a plan
 * whose decor is a layer can be printed without its cartouche, its folds and its annotations
 * and look perfectly normal. Printers untick boxes. So the safe answer is the one the dialog
 * opens on, and the layer is something the user has to ask for.
 */
class OExportPdfDialog : public OExportFileDialog
{
    Q_OBJECT

public:
    /// @param outputName the generation's name, offered as the file name.
    /// @param directory  where it opens; the folder last used, or Documents.
    explicit OExportPdfDialog(const QString &outputName, const QString &directory,
                              QWidget *parent = nullptr);

    /// @brief Whether the decor is written as one of the reader's layers.
    ///
    /// False — the default — draws the decor exactly as it always was and leaves it out of the
    /// optional content altogether: no entry in the reader's panel, nothing to switch off, and
    /// the underlay with it. True offers it beside the sizes, where it can be turned off like
    /// any other layer.
    auto AddLayoutLayer() const -> bool;

protected:
    void Retranslate() override;

private:
    QCheckBox *m_layoutLayerCheck{nullptr};
};

#endif  // OEXPORTPDFDIALOG_H
