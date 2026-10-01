/************************************************************************
 **
 **  @file   vtooloptionspropertybrowser.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   22 8, 2014
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
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

#ifndef VTOOLOPTIONSPROPERTYBROWSER_H
#define VTOOLOPTIONSPROPERTYBROWSER_H

#include <QMap>
#include <QObject>
#include <QPointer>

#include "../vpropertyexplorer/vproperty.h"
#include "../vpropertyexplorer/vpropertyformview.h"
#include "../vpropertyexplorer/vpropertymodel.h"
#include "../vmisc/typedef.h"

class QDockWidget;
class QGraphicsItem;
class QScrollArea;
class QStackedWidget;
class VFormula;
class OPatternPropertiesWidget;

struct PropertyDesc;

class VToolOptionsPropertyBrowser : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit VToolOptionsPropertyBrowser(QWidget *parent);
    ~VToolOptionsPropertyBrowser() override = default;

    /** @brief The stacked widget the browser built its content into: page 0 is
     *         the tool options form, page 1 the pattern work fields. Owned by
     *         the dock's QStackedWidget — callers must not delete or reparent it. */
    QWidget *widget() const;

    void ClearPropertyBrowser();
    QGraphicsItem *currentItem() const { return m_currentItem; }

    /** @brief The object the last click landed on, as the container knows it.
     *
     * Kept beside the resolved item because the browser shows the options of the TOOL — the operation — while the
     * click was on ONE of its copies. The copy's own pen and colour are edited in the operation's element, and
     * this is the id that tells which copy: `VAbstractSimple::GetId()` on the item, its tool's id for a line.
     * NULL_ID when the click carried none (a piece, the background). */
    auto ClickedObjectId() const -> quint32 { return m_clickedObjectId; }

    auto ComboBoxPalette() const -> QPalette;

    void RetranslateUI();

public slots:
    void itemClicked(QGraphicsItem *item);
    void UpdateOptions();
    void RefreshOptions();

signals:
    /** @brief PatternPieceRenameRequested the user typed a new name for the pattern piece in the options dock.
     *
     * The browser only knows the tools; renaming the piece is the window's business (undo command + top bar
     * combo), so the request is handed over and the window answers by actually renaming — or by refusing an
     * empty or already used name. */
    void PatternPieceRenameRequested(const QString &name);

private slots:
    void userChangedData(VPE::VProperty *property);

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(VToolOptionsPropertyBrowser) // NOLINT

    VPE::VPropertyModel *m_PropertyModel{nullptr};
    VPE::VPropertyFormView *m_formView{nullptr};
    QScrollArea *m_scroll{nullptr};
    QStackedWidget *m_stack{nullptr};
    OPatternPropertiesWidget *m_patternWidget{nullptr};

    QGraphicsItem *m_currentItem{nullptr};
    QPointer<QObject> m_currentObject{};
    quint32 m_clickedObjectId{NULL_ID};
    QMap<VPE::VProperty *, QString> m_propertyToId{};
    QMap<QString, VPE::VProperty *> m_idToProperty{};

    /** @brief Add a whole set of rows to the dock in one insertion: the form view rebuilds every editor on
     *  each insertion, so one by one they would cost n(n+1)/2 editor creations instead of n. */
    void AddProperties(const QList<VPE::VProperty *> &properties, const QStringList &ids);
    void ShowItemOptions(QGraphicsItem *item);

    // --- Declarative property system ---
    template <class Tool>
    void ShowToolOptions(QGraphicsItem *item, const QString &title,
                         const std::vector<PropertyDesc> &descriptors);

    template <class Tool>
    void ChangeToolData(VPE::VProperty *property,
                        const std::vector<PropertyDesc> &descriptors);

    template <class Tool>
    void UpdateToolOptions(const std::vector<PropertyDesc> &descriptors);
};

#endif // VTOOLOPTIONSPROPERTYBROWSER_H
