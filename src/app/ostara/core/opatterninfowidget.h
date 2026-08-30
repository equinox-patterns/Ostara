/************************************************************************
 **
 **  @file   opatterninfowidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   30/08/2026
 **
 **  @brief  Pattern information page (pattern name, client, collection,
 **          fabric info) shown in the Details options dock when no piece
 **          is selected.
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

#ifndef OPATTERNINFOWIDGET_H
#define OPATTERNINFOWIDGET_H

#include "../widgets/InfoBlock/OPieceCartoucheStorage.h"

#include <QPointer>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QTimer;
class VAbstractPattern;

/**
 * @brief Pattern information fields for the Details dock (no piece selected).
 *
 * Owns the editing surface of four document fields — pattern name, client,
 * collection and fabric info — plus the pattern-level seeds applied to pieces
 * created afterwards: seam allowances (activate + hide main path) and the
 * information-block cartouche template. One field, one source
 * (VAbstractPattern); the widget reads/writes it and refreshes on document
 * changes. Pattern name is the shared field also surfaced by the Draw pattern
 * block and the dialog step 1.
 */
class OPatternInfoWidget : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit OPatternInfoWidget(QWidget *parent = nullptr);

    /** Reload the fields from the current pattern document. */
    void Refresh();

    void RetranslateUI();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void CommitPatternName();
    void CommitClient(int index);
    void CommitCollection(int index);
    void CommitActivateSA(bool on);
    void CommitHideMainPath(bool on);
    void CommitCartoucheActivation(bool on);
    void CommitCartoucheTemplate(int index);
    void ScheduleCommit();
    void CommitDelayed();
    void OnFullUpdate();

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(OPatternInfoWidget) // NOLINT

    auto CurrentDocument() const -> VAbstractPattern *;
    void Populate();

    QPointer<VAbstractPattern> m_doc{};
    QLineEdit *m_patternNameEdit{nullptr};
    QComboBox *m_clientCombo{nullptr};
    QComboBox *m_collectionCombo{nullptr};
    QCheckBox *m_activateSACheck{nullptr};
    QCheckBox *m_hideMainPathCheck{nullptr};
    QCheckBox *m_activateInfoBlockCheck{nullptr};
    QComboBox *m_cartoucheCombo{nullptr};
    QPlainTextEdit *m_fabricInfoEdit{nullptr};
    QLabel *m_patternNameLabel{nullptr};
    QLabel *m_clientLabel{nullptr};
    QLabel *m_collectionLabel{nullptr};
    QLabel *m_fabricLabel{nullptr};
    QTimer *m_commitTimer{nullptr};
    bool m_loading = false;

    // Parsed client/collection/template libraries — reloaded on document change
    // or full update only, so a plain show does not hit the disk again.
    QList<OPieceCartoucheStorage::ClientEntry> m_clients;
    QList<OPieceCartoucheStorage::CollectionEntry> m_collections;
    QStringList m_templates;
};

#endif // OPATTERNINFOWIDGET_H
