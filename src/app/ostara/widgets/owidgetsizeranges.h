/************************************************************************
 **
 **  @file   owidgetsizeranges.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/09/2026
 **
 **  @brief  Dock "Size Ranges" content: a compact list of the pattern size
 **          ranges (one row each), an "Add a size range" button on top and one
 **          icon action per row (favorite/edit/duplicate/export/delete).
 **          The active range is shown by a filled check-circle dot.
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

#ifndef OWIDGETSIZERANGES_H
#define OWIDGETSIZERANGES_H

#include <QEvent>
#include <QIcon>
#include <QWidget>

class QPushButton;
class QTableWidget;
class VContainer;

/**
 * @brief The OWidgetSizeRanges class is the content of the Graded-mode
 *        "Size Ranges" dock. It shows one compact row per stored range
 *        (active dot + name on the left, action icons on the right) and asks
 *        the MainWindow to act through its signals — it never touches the
 *        pattern itself.
 */
class OWidgetSizeRanges final : public QWidget
{
    Q_OBJECT

public:
    explicit OWidgetSizeRanges(VContainer *pattern, QWidget *parent = nullptr);

    /** @brief Re-read the pattern ranges and show @p activeRangeName as active. */
    void UpdateList(const QString &activeRangeName);

    /** @brief Re-label the chrome (LanguageChange); re-pops the list (icons too). */
    void RetranslateUI();

signals:
    void addRequested();
    void editRequested(const QString &rangeName);
    void renameRequested(const QString &oldName, const QString &newName);
    void duplicateRequested(const QString &rangeName);
    void deleteRequested(const QString &rangeName);
    void rangeActivated(const QString &rangeName);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void BuildUi();
    void Populate();
    void UpdateAddButtonIcon();
    void OnNameEdited(QTableWidgetItem *item);
    void ToggleFavorite(int row, const QString &rangeName);

    VContainer *m_pattern{nullptr};
    QString m_activeRangeName;

    QTableWidget *m_table{nullptr};
    QPushButton *m_addButton{nullptr};

    bool m_filling{false}; // true while Populate() rebuilds the rows

    QIcon m_addNormalIcon; // current-theme variant
    QIcon m_addHoverIcon;  // dark variant — used on hover (accent button background)
};

#endif // OWIDGETSIZERANGES_H
