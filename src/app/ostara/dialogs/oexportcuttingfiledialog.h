/************************************************************************
 **
 **  @file   oexportcuttingfiledialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/10/2026
 **
 **  @brief  Export mode — where a cutting plan is written, and in what.
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

#ifndef OEXPORTCUTTINGFILEDIALOG_H
#define OEXPORTCUTTINGFILEDIALOG_H

#include "../oexportcuttingfile.h"
#include "oexportfiledialog.h"

class OExportCuttingFileOptions;

/**
 * @brief Where a cutting plan is written, and in what: DXF, PLT or HPGL.
 *
 * The name and the folder come from @ref OExportFileDialog, and with them the overwrite question and
 * the folder that is remembered between two exports. What this one adds is the setting — the format,
 * its sub-type and the options — which lives in @ref OExportCuttingFileOptions because the batch's
 * cutting page needs the very same one, and two copies of seven fields would be two truths.
 *
 * **And it does not offer the scale.** A cutting plan leaves at 100 %, always: it is a file a machine
 * cuts, and a reduced scale would produce a plan that looks right and cuts wrong (`CuttingPlan.md`
 * §24.2).
 *
 * ⚠️ The suffix follows the format. The base class takes its own in the constructor and appends it to
 * the name; with three formats to choose from, that one has to change, which is what `SetSuffix` is
 * for.
 */
class OExportCuttingFileDialog : public OExportFileDialog
{
    Q_OBJECT // NOLINT

public:
    /// @param outputName the generation's name, offered as the file name.
    /// @param directory  where it opens; the folder last used, or Documents.
    explicit OExportCuttingFileDialog(const QString &outputName, const QString &directory,
                                      QWidget *parent = nullptr);

    /// @brief Everything the writer needs: the format, its sub-type, and the options.
    auto Options() const -> OExportCuttingFile::Options;

protected:
    void Retranslate() override;

private:
    OExportCuttingFileOptions *m_options{nullptr};
};

#endif // OEXPORTCUTTINGFILEDIALOG_H
