/************************************************************************
 **
 **  @file   owidgetsizestyles.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   06/09/2026
 **
 **  @brief  Dock "Size Styles" content: a read-only list of the graded size axis
 **          values (one row each) with the trace style each one carries — layer
 **          name, color swatch and outline preview — an "Edit size styles…"
 **          button on top and a global "Show in canvas" gate below. The dock
 **          never touches the pattern: it asks the MainWindow to act through its
 **          signals.
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

#ifndef OWIDGETSIZESTYLES_H
#define OWIDGETSIZESTYLES_H

#include <QEvent>
#include <QIcon>
#include <QStringList>
#include <QWidget>

class QCheckBox;
class QPushButton;
class QTableWidget;

/**
 * @brief The OWidgetSizeStyles class is the content of the Graded-mode
 *        "Size Styles" dock. MainWindow feeds it the (canonical path, size
 *        axis, axis values, in-library) context it is showing; it reads the
 *        software style store itself and renders one compact read-only row per
 *        axis value. It never touches the pattern nor the canvas.
 */
class OWidgetSizeStyles final : public QWidget
{
    Q_OBJECT

public:
    explicit OWidgetSizeStyles(QWidget *parent = nullptr);

    /** @brief Tell the dock which size axis it displays and refresh the rows.
     *         An empty @p sizeValues clears the list. */
    void SetSizeStylesContext(const QString &canonicalPath, const QString &axisName,
                              const QStringList &sizeValues, bool inLibrary);

    /** @brief Re-label the chrome (LanguageChange); re-pops the list (icons too). */
    void RetranslateUI();

signals:
    void editRequested();
    void showInCanvasChanged(bool on);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void BuildUi();
    void Populate();
    void UpdateEditButtonIcon();

    QString m_canonicalPath;
    QString m_axisName;
    QStringList m_values;
    bool m_inLibrary{false};

    QTableWidget *m_table{nullptr};
    QPushButton *m_editButton{nullptr};
    QCheckBox *m_showInCanvas{nullptr};

    QIcon m_editNormalIcon; // current-theme variant
    QIcon m_editHoverIcon;  // dark variant — used on hover (accent button background)
};

#endif // OWIDGETSIZESTYLES_H
