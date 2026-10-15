/************************************************************************
 **
 **  @file   oexportcuttingfileoptions.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/10/2026
 **
 **  @brief  Export mode — what a cutting file is written with: format, sub-type, options.
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

#ifndef OEXPORTCUTTINGFILEOPTIONS_H
#define OEXPORTCUTTINGFILEOPTIONS_H

#include "../oexportcuttingfile.h"
#include "../vlayout/vlayoutdef.h"

#include <QWidget>

class QCheckBox;
class QComboBox;
class QEvent;
class QLabel;

/**
 * @brief Le réglage d'un fichier de découpe : le format, son sous-type, et les options.
 *
 * **Une seule définition, deux endroits** : la dialogue d'un plan de coupe exporté seul, et la page
 * du lot. Le lot écrit **un format pour tout son lot** — comme la case « mise en page » du PDF vaut
 * pour tous les PDF du lot : une utilisatrice qui veut deux jeux d'options fait deux lots. Recopier
 * ces sept champs dans la page aurait été deux vérités à tenir, et la seconde aurait dérivé.
 *
 * Les options sont celles de l'export du mode Details, **format par format** : une option qui
 * n'existe pas pour un format n'est pas offerte pour lui. L'échelle n'y est pas — un plan de coupe
 * sort à 100 %, toujours (`CuttingPlan.md` §24).
 */
class OExportCuttingFileOptions : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit OExportCuttingFileOptions(QWidget *parent = nullptr);

    /// @brief Le réglage, tel que l'écrivain l'attend.
    auto Options() const -> OExportCuttingFile::Options;

    /// @brief L'extension du format courant — le nom du fichier la suit, et lui seul la connaît.
    auto Suffix() const -> QString;

    void Retranslate();

signals:
    /// @brief Le format a changé : ce que l'appelant nomme doit le suivre — l'extension, d'abord.
    void FormatChanged();

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void FileFormatChanged();
    void SubTypeChanged();

private:
    void PopulateFileFormats();
    void PopulateSubTypes();
    void ShowOptionsForFormat();
    void InitDxfCompatibility();

    auto Format() const -> LayoutExportFormats;

    QLabel *m_formatLabel{nullptr};
    QComboBox *m_formatCombo{nullptr};
    QLabel *m_subTypeLabel{nullptr};
    QComboBox *m_subTypeCombo{nullptr};
    QCheckBox *m_binaryCheck{nullptr};
    QCheckBox *m_textAsPathsCheck{nullptr};
    QCheckBox *m_grainlineCheck{nullptr};
    QCheckBox *m_notchesCheck{nullptr};
    QCheckBox *m_mainPathCheck{nullptr};
    QLabel *m_compatibilityLabel{nullptr};
    QComboBox *m_compatibilityCombo{nullptr};
};

#endif // OEXPORTCUTTINGFILEOPTIONS_H
