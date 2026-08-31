/************************************************************************
 **
 **  @file   oblockconfigwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   31 August, 2026
 **
 **  @brief  Blocks sub-tab of the Information dock tab: per-piece cartouche
 **          configuration — activation, template override and the dispatch
 **          of each template section to a block (1..11).
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

#ifndef OBLOCKCONFIGWIDGET_H
#define OBLOCKCONFIGWIDGET_H

#include <QWidget>

#include <QHash>
#include <QStringList>
#include <QVector>
#include <functional>
#include <utility>

#include "../vpatterndb/oinfoblockdata.h"
#include "../vpatterndb/vpiece.h"

class QCheckBox;
class QComboBox;
class QFormLayout;
class QGroupBox;
class QLabel;
class VContainer;

/**
 * @brief Blocks sub-tab of the Information dock tab.
 *
 * Mutates VPiece through the same snapshot + undo pipeline as the rest of
 * the dock (SetModifyPending / ModifyPiece). The first edit materializes
 * the effective (seed-derived) state into explicit per-piece data.
 */
class OBlockConfigWidget : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit OBlockConfigWidget(QWidget *parent = nullptr);
    ~OBlockConfigWidget() override = default;

    /** Callback that applies a VPiece mutation and commits it to the undo stack. */
    void SetModifyPending(std::function<void(std::function<void(VPiece &)>)> callback);

    void Show(const VPiece &detail, const VContainer *data);
    void Clear();
    void RetranslateUI();

private:
    void ClearRows();
    void RebuildInfoList(const QString &templateName, const QHash<QString, int> &dispatch);
    auto ReadDispatch() const -> QHash<QString, int>;
    auto BuildData() const -> OInfoBlockData;
    void Commit();

    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(OBlockConfigWidget) // NOLINT

    std::function<void(std::function<void(VPiece &)>)> m_modifyPending;

    QCheckBox *m_activeCheck{nullptr};
    QLabel *m_templateLabel{nullptr};
    QComboBox *m_templateCombo{nullptr};
    QGroupBox *m_dispatchGroup{nullptr};
    QFormLayout *m_listLayout{nullptr};
    QStringList m_templatePaths;

    // Per displayed row: section type id + its "Block N" combo.
    QVector<QPair<QString, QComboBox *>> m_rows;

    QString m_lastTemplateName;
    QHash<QString, int> m_lastDispatch;
    bool m_hasState{false};
    bool m_loading{false};
};

#endif // OBLOCKCONFIGWIDGET_H
