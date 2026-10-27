/************************************************************************
 **
 **  @file   vtablesearch.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   15 9, 2015
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 19/10/2026.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#ifndef VTABLESEARCH_H
#define VTABLESEARCH_H

#include <QObject>
#include <QList>
#include <QString>
#include <QTableWidget>
#include <QtGlobal>

#include "../vmisc/defglobal.h"

class VTableSearch: public QObject
{
    Q_OBJECT // NOLINT
public:
    explicit VTableSearch(QTableWidget *table, QObject *parent = nullptr);

    void Find(const QString &term);
    void FindPrevious();
    void FindNext();
    void RemoveRow(int row);
    void AddRow(vsizetype row);
    void RefreshList(const QString &term);

    void SetMatchCase(bool value);
    auto IsMatchCase() const -> bool;

    void SetMatchWord(bool value);
    auto IsMatchWord() const -> bool;

    void SetMatchRegexp(bool value);
    auto IsMatchRegexp() const -> bool;

    void SetUseUnicodePreperties(bool value);
    auto IsUseUnicodePreperties() const -> bool;

    auto MatchIndex() const -> vsizetype;
    auto MatchCount() const -> vsizetype;

    auto SearchPlaceholder() const -> QString;

    /// What the search knows about a cell, for a table whose own delegate paints the matches.
    ///
    /// The item background the search writes (`Qt::BackgroundRole`) is enough for a plain table, but a view whose
    /// stylesheet sets `QAbstractItemView::item { background-color: ... }` paints over it — the highlight then
    /// exists and shows nothing. Such a table reads this role in its delegate instead: `Row` is every cell of a
    /// line that holds at least one match, `Match` the cell that matched, `Current` the match the counter points
    /// at, and `None` any other cell.
    enum class Highlight : quint8
    {
        None = 0,
        Row = 1,
        Match = 2,
        Current = 3
    };
    static constexpr int HighlightRole = Qt::UserRole + 100;

    static const int MaxHistoryRecords;

signals:
    void HasResult(bool state);

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(VTableSearch) // NOLINT

    QTableWidget *table;
    vsizetype searchIndex{-1};
    QList<QTableWidgetItem *> searchList{};

    bool m_matchCase{false};
    bool m_matchWord{false};
    bool m_matchRegexp{false};
    bool m_useUnicodePreperties{false};

    void Clear();
    void ShowNext(vsizetype newIndex);
    /// Write HighlightRole for the current matches: every cell of a matching line first, then the matched cells,
    /// then the one the counter points at — so the current cell is never overwritten by its own line.
    void ApplyHighlights();
    void ClearHighlights();
    void MarkRow(int row, Highlight level);
    auto FindTableItems(QString term) -> QList<QTableWidgetItem *>;
    auto FindCurrentMatchIndex() const -> int;
};

#endif // VTABLESEARCH_H
