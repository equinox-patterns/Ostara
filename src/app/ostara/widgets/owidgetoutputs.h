/************************************************************************
 **
 **  @file   owidgetoutputs.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   14/09/2026
 **
 **  @brief  Dock "Outputs" content (Export mode): a compact list of the
 **          generated outputs — one row each, an "Add an output" button and an
 **          "Add a group" button on top, four move buttons under them, and one
 **          icon action per row (favorite/edit/refresh/export/delete).
 **          The output shown in the canvas is marked by a filled check-circle.
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

#ifndef OWIDGETOUTPUTS_H
#define OWIDGETOUTPUTS_H

#include <QEvent>
#include <QIcon>
#include <QList>
#include <QString>
#include <QVector>
#include <QWidget>

class QPushButton;
class QTableWidget;

/**
 * @brief One row of the Outputs dock: what it takes to draw a generation.
 *
 * The dock never sees the geometry, and never sees the store either — it is handed the
 * rows that are to be listed, already filtered to the current (table, axis) context by
 * MainWindow, which is the only owner of that rule.
 */
struct OExportOutputRow
{
    quint32 id{0};   ///< stable identity — the row index is not one
    QString name{};  ///< "Unnamed" for now: no Name field exists yet
};

/**
 * @brief The OWidgetOutputs class is the content of the Export-mode "Outputs"
 *        dock. It lists the generated outputs (active dot + name on the left,
 *        action icons on the right) grouped under the groups the user files
 *        them into. Like the Size Ranges dock it never touches the pattern: it
 *        will act through signals once the behaviour lands.
 *
 *        Shell stage (14-09-2026): every control is present and inert — the
 *        buttons do nothing. See spec
 *        Modifications/Ostara/ExportMode/DockOutputs.md.
 */
class OWidgetOutputs final : public QWidget
{
    Q_OBJECT

public:
    explicit OWidgetOutputs(QWidget *parent = nullptr);

    /**
     * @brief Show @p outputs — already filtered to the current (table, axis) context by
     *        MainWindow. Does not re-pop: the caller follows with UpdateList().
     */
    void SetOutputs(const QVector<OExportOutputRow> &outputs);

    /** @brief Re-pop the list, marking @p activeOutputId (0 = none) as the one shown. */
    void UpdateList(quint32 activeOutputId);

    /** @brief Re-label the chrome (LanguageChange); re-pops the list (icons too). */
    void RetranslateUI();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void BuildUi();
    void Populate();
    void UpdateButtonIcons();
    QPushButton *MakeIconButton(const QString &objectName, const QString &iconBase);

    QTableWidget *m_table{nullptr};
    QPushButton *m_addOutputButton{nullptr};

    // Icon buttons: base name + the two variants, re-read on every theme change.
    // A button created inline in a lambda would not be reachable from the slot,
    // hence the list (§16 of SubtiliteCode/03-polices-icones.md).
    struct IconButton
    {
        QPushButton *button{nullptr};
        QString baseName;
        QIcon normal; // current-theme variant
        QIcon hover;  // always the dark variant — the hovered background is an accent
    };
    QList<IconButton> m_iconButtons;

    QVector<OExportOutputRow> m_outputs{};
    quint32 m_activeOutputId{0};
};

#endif // OWIDGETOUTPUTS_H
