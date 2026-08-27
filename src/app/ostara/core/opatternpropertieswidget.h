/************************************************************************
 **
 **  @file   opatternpropertieswidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/08/2026
 **
 **  @brief  Pattern work fields (pattern name, description, notes) shown in
 **          the Draw property block when no tool is selected.
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

#ifndef OPATTERNPROPERTIESWIDGET_H
#define OPATTERNPROPERTIESWIDGET_H

#include <QPointer>
#include <QWidget>

class QLineEdit;
class QPlainTextEdit;
class QTimer;
class VAbstractPattern;

/**
 * @brief Pattern-level work fields for the Draw property block.
 *
 * Owns the editing surface of three document fields: pattern name,
 * description and notes. One field, one source (VAbstractPattern);
 * the widget reads/writes it and refreshes on document changes.
 */
class OPatternPropertiesWidget : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit OPatternPropertiesWidget(QWidget *parent = nullptr);

    /** Reload the fields from the current pattern document. */
    void Refresh();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void CommitPatternName();
    void ScheduleCommit();
    void CommitDelayed();

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(OPatternPropertiesWidget) // NOLINT

    auto CurrentDocument() const -> VAbstractPattern *;
    void Populate();

    QPointer<VAbstractPattern> m_doc{};
    QLineEdit *m_patternNameEdit{nullptr};
    QPlainTextEdit *m_descriptionEdit{nullptr};
    QPlainTextEdit *m_notesEdit{nullptr};
    QTimer *m_commitTimer{nullptr};
    bool m_loading{false};
};

#endif // OPATTERNPROPERTIESWIDGET_H
