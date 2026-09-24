/************************************************************************
 **
 **  @file   oexportrowlistwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  A scrolling stack of row widgets, with a line of text to show while it holds
 **          none — the frame the Export dialog's lists are built on.
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

#ifndef OEXPORTROWLISTWIDGET_H
#define OEXPORTROWLISTWIDGET_H

#include <QVector>
#include <QWidget>

class QLabel;
class QLayout;
class QScrollArea;
class QVBoxLayout;

/// Largeur de la colonne d'état d'une ligne : assez pour le plus long des quatre états, pour que
/// les noms à côté restent dans une seule colonne au lieu d'avancer et de reculer pendant que le
/// lot défile. Partagée par les deux sortes de lignes — deux valeurs qui divergeraient
/// désaligneraient les deux listes l'une sous l'autre.
inline constexpr int kExportRowStateWidth = 70;

/// Largeur d'un bouton-icône de fin de ligne (corbeille). Partagée pour la même raison.
inline constexpr int kExportRowButtonWidth = 30;

/**
 * @brief What every list of the Export dialog's entry screen has in common.
 *
 * A scrollable stack of rows, one after another at the top, with a line of text shown instead
 * while there is none: an empty page with a lone greyed button says "broken", where one
 * sentence says what to do about it.
 *
 * Extracted (24-09-2026) when the favorites lists arrived. It is not the row API that justifies
 * it — the rows have nothing in common — but the frame: the scroll area's viewport paints a
 * flat NATIVE background (white on macOS) that overrides both the palette and the global QSS,
 * so it takes an inline dot-selector with an OPAQUE colour to follow the theme
 * (SubtiliteCode §13). That is a platform workaround, and one copy of it is enough.
 *
 * It carries no action button: what acts on a whole list belongs to the dialog whose button bar
 * follows it, not here where a second button would answer to almost the same name as the bar's
 * own.
 */
class OExportRowListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OExportRowListWidget(QWidget *parent = nullptr);
    ~OExportRowListWidget() override = default;

    auto IsEmpty() const -> bool;

    /// @brief Lock what acts on the list, without greying what it shows.
    ///
    /// A batch locks a list while it walks it — a row taken down under the walk would shift
    /// every row after it — but the rows themselves must stay legible: the states are exactly
    /// what the user is watching.
    void SetBusy(bool busy);

    auto IsBusy() const -> bool;

protected:
    /// @brief Stack @p row at the end of the list, before the stretch that absorbs the spare
    ///        height. The caller keeps ownership of what it stacks.
    void AppendRowWidget(QWidget *row);

    /// @brief Take @p row down. No-op when it is not in the list.
    ///
    /// The row is deleted later rather than at once: a row is removed from inside the clicked()
    /// handler of one of its own buttons, and deleting it there would destroy the object whose
    /// signal is still on the stack.
    void RemoveRowWidget(QWidget *row);

    /// @brief The rows stacked so far, in order, converted to the type the subclass stacked.
    ///
    /// A template because the rows have nothing in common beyond being widgets, and a subclass
    /// keeping its own typed copy of the list would be a second list to keep in step with this
    /// one.
    template <typename T> [[nodiscard]] auto RowWidgets() const -> QVector<T *>
    {
        QVector<T *> typed;
        typed.reserve(m_rows.size());
        for (QWidget *row : m_rows)
        {
            typed.append(static_cast<T *>(row)); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
        }
        return typed;
    }

    /// @brief What to show instead of the rows while the list holds none.
    void SetEmptyHint(const QString &text);

    /// @brief Lay @p bar out above the rows — buttons that act on the list as a whole.
    void SetTopBar(QLayout *bar);

    /// @brief Put the empty hint and the rows back in step with the list.
    ///
    /// Called by the base on every change. A subclass overrides it to lock what acts on its own
    /// rows, and must call the base's implementation first.
    ///
    /// ⚠️ The base constructor does NOT call it: a virtual called from a constructor runs before
    /// the subclass exists. The initial state is set directly instead, and every change after
    /// construction goes through here.
    virtual void Refresh();

    auto RootLayout() const -> QVBoxLayout *;

private:
    QVector<QWidget *> m_rows{};
    QVBoxLayout *m_rootLayout{nullptr};
    QScrollArea *m_scrollArea{nullptr};
    QWidget *m_listContainer{nullptr};
    QVBoxLayout *m_listLayout{nullptr};
    QLabel *m_emptyHint{nullptr};
    bool m_busy{false};
};

#endif  // OEXPORTROWLISTWIDGET_H
