/************************************************************************
 **
 **  @file   ochooselayerdialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   28/09/2026
 **
 **  @brief  Choose one layer out of a list — the question object picking
 **          cannot ask by itself.
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

#ifndef OCHOOSELAYERDIALOG_H
#define OCHOOSELAYERDIALOG_H

#include "../vmisc/typedef.h"

#include <QDialog>
#include <QIcon>
#include <QString>
#include <QVector>

class QComboBox;
class QPushButton;

/**
 * @brief The OChooseLayerDialog class — one layer, picked out of a list.
 *
 * It knows nothing about how a layer is stored: the caller hands it an id, the name to read and the
 * dot it draws for that layer, and gets the chosen id back. NULL_ID means the user changed their
 * mind — the workflow that opened it writes nothing in that case.
 *
 * The answer is one value out of a handful, so it is a combo: the line already shows the layer that
 * will be written, and the button that accepts says what it will do rather than "OK".
 */
class OChooseLayerDialog final : public QDialog
{
    Q_OBJECT // NOLINT

public:
    /// One entry: the layer, the name read out of it, and the dot the caller draws for its pen.
    struct Choice
    {
        quint32 id{NULL_ID};
        QString name;
        QIcon icon;
    };

    struct Config
    {
        QString intro;                // the sentence above the field, translated by the caller; may be empty
        QString acceptText;           // the action's own word — "OK" when the caller has none
        QVector<Choice> choices;      // in the order the caller wants them read
        quint32 preselected{NULL_ID}; // the layer shown when the dialog opens, when it is in the list
    };

    explicit OChooseLayerDialog(const Config &config, QWidget *parent = nullptr);
    ~OChooseLayerDialog() override = default;

    /// The layer the user picked. NULL_ID when the dialog was rejected, and while nothing is picked.
    auto ChosenLayerId() const -> quint32;

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(OChooseLayerDialog) // NOLINT

    QComboBox *m_combo{nullptr};
    QPushButton *m_acceptButton{nullptr};
};

#endif // OCHOOSELAYERDIALOG_H
