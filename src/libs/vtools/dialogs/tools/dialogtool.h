/************************************************************************
 **
 **  @file   dialogtool.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   November 15, 2013
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

#ifndef DIALOGTOOL_H
#define DIALOGTOOL_H

#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QList>
#include <QListWidget>
#include <QLocale>
#include <QMap>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QPushButton>
#include <QRadioButton>
#include <QString>
#include <QVariant>
#include <QtGlobal>

#include "../dialogtoolbox.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vgeometry/vgeometrydef.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vtools/visualization/visualization.h" // Issue on Windows
#include "../vwidgets/vmaingraphicsscene.h"

template <class T> class QSharedPointer;

Q_DECLARE_LOGGING_CATEGORY(vDialog)

class QDoubleSpinBox;
class QLabel;
class QPlainTextEdit;
class QVBoxLayout;
class VAbstractTool;

enum class FillComboBox : qint8
{
    Whole,
    NoChildren
};

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wsuggest-final-types")
QT_WARNING_DISABLE_GCC("-Wsuggest-final-methods")

/**
 * @brief The DialogTool class parent for all dialog of tools.
 */
class DialogTool : public QDialog
{
    Q_OBJECT // NOLINT

public:
    DialogTool(const VContainer *patternData, VAbstractPattern *doc, quint32 toolId, QWidget *parent = nullptr);
    ~DialogTool() override;

    auto GetAssociatedTool() -> VAbstractTool *;
    void SetAssociatedTool(VAbstractTool *tool);

    virtual void ShowDialog(bool click);
    auto VisualizationBroken() const -> bool;
    virtual void Build(const Tool &type);
    virtual void SetPiecesList(const QVector<quint32> &list);
    virtual void SetPatternDoc(VAbstractPattern *doc);

    auto GetToolId() const -> quint32;
    void SetToolId(const quint32 &value);

    /** @brief "Show Point name" checkbox state at creation ("Label" = point name shown as exception). */
    auto GetShowLabel() const -> bool { return m_showLabel; }
    void SetShowLabel(bool value) { m_showLabel = value; }

    /** @brief "Show the two segments instead of the curve" checkbox state (cut tools only). */
    auto GetShowTwoSegments() const -> bool { return m_showTwoSegments; }
    void SetShowTwoSegments(bool value);

    /** @brief Look of the piece prolonging the curve, when the cut goes past an end (cut tools only). */
    auto GetProlongationType() const -> QString { return m_prolongationType; }
    void SetProlongationType(const QString &value);
    auto GetProlongationColor() const -> QString { return m_prolongationColor; }
    void SetProlongationColor(const QString &value);

    /** @brief True when the entered length takes the cut point past an end of the curve. */
    auto IsProlonged() const -> bool { return m_prolonged; }

    /** @brief Recomputes the prolongation from the entered formula and the base curve, and says so. */
    void UpdateProlongation(quint32 curveId, const QString &formula);

    virtual void CheckDependencyTreeComplete() = 0;

    virtual void SetGroupCategories(const QStringList &categories);

    static void MoveListRowTop(QListWidget *list);
    static void MoveListRowUp(QListWidget *list);
    static void MoveListRowDown(QListWidget *list);
    static void MoveListRowBottom(QListWidget *list);
signals:
    /**
     * @brief DialogClosed signal dialog closed
     * @param result keep result
     */
    void DialogClosed(int result);
    /**
     * @brief DialogApplied emit signal dialog apply changes
     */
    void DialogApplied();
    /**
     * @brief ToolTip emit tooltipe for tool
     * @param toolTip text tooltipe
     */
    void ToolTip(const QString &toolTip);
public slots:
    virtual void ChosenObject(quint32 id, const SceneObject &type);
    virtual void SelectedObject(bool selected, quint32 object, quint32 tool);
    virtual void ClearSourceObjects() {}
    virtual void DialogAccepted();
    /**
     * @brief DialogApply save data and emit signal DialogApplied.
     */
    virtual void DialogApply();
    virtual void DialogRejected();
    virtual void PointNameChanged() {}

protected:
    /** @brief data container with data */
    VContainer data;

    VAbstractPattern *m_doc;

    /** @brief isInitialized true if window is initialized */
    bool isInitialized;

    /** @brief bOk button ok */
    QPushButton *bOk;

    /** @brief bApply button apply */
    QPushButton *bApply;

    /**
     * @brief associatedTool vdrawtool associated with opened dialog.
     */
    VAbstractTool *associatedTool;
    quint32 toolId;

    /** @brief prepare show if we prepare. Show dialog after finish working with visual part of tool*/
    bool prepare;

    QPointer<Visualization> vis;

    QComboBox *m_colorCombo{nullptr};

    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    auto eventFilter(QObject *object, QEvent *event) -> bool override;

    virtual auto IsValid() const -> bool = 0;
    virtual void CheckState();

    void FillComboBoxPiecesList(QComboBox *box, const QVector<quint32> &list);
    void FillComboBoxPoints(QComboBox *box, FillComboBox rule = FillComboBox::Whole, quint32 ch1 = NULL_ID,
                            quint32 ch2 = NULL_ID) const;
    void FillComboBoxArcs(QComboBox *box, FillComboBox rule = FillComboBox::Whole, quint32 ch1 = NULL_ID,
                          quint32 ch2 = NULL_ID) const;
    void FillComboBoxEllipticalArcs(QComboBox *box,
                                    FillComboBox rule = FillComboBox::Whole,
                                    quint32 ch1 = NULL_ID,
                                    quint32 ch2 = NULL_ID) const;
    void FillComboBoxArcCurves(QComboBox *box,
                               FillComboBox rule = FillComboBox::Whole,
                               quint32 ch1 = NULL_ID,
                               quint32 ch2 = NULL_ID) const;
    void FillComboBoxSplines(QComboBox *box) const;
    void FillComboBoxSplinesPath(QComboBox *box) const;
    void FillComboBoxCurves(QComboBox *box) const;
    void FillComboBoxCrossCirclesPoints(QComboBox *box) const;
    void FillComboBoxVCrossCurvesPoint(QComboBox *box) const;
    void FillComboBoxHCrossCurvesPoint(QComboBox *box) const;

    auto Eval(const FormulaData &formulaData, bool &flag) -> qreal;

    void setCurrentPointId(QComboBox *box, const quint32 &value, FillComboBox rule = FillComboBox::NoChildren,
                           const quint32 &ch1 = NULL_ID, const quint32 &ch2 = NULL_ID) const;
    void setCurrentSplineId(QComboBox *box, const quint32 &value) const;
    void setCurrentArcId(QComboBox *box, const quint32 &value, FillComboBox rule = FillComboBox::NoChildren,
                         const quint32 &ch1 = NULL_ID, const quint32 &ch2 = NULL_ID) const;
    void setCurrentArcCurveId(QComboBox *box,
                              const quint32 &value,
                              FillComboBox rule = FillComboBox::NoChildren,
                              const quint32 &ch1 = NULL_ID,
                              const quint32 &ch2 = NULL_ID) const;
    void setCurrentSplinePathId(QComboBox *box, const quint32 &value) const;
    void setCurrentCurveId(QComboBox *box, const quint32 &value) const;

    static auto getCurrentObjectId(QComboBox *box) -> quint32;

    template <typename T> auto getCurrentCrossPoint(QComboBox *box) const -> T;

    auto SetObject(const quint32 &id, QComboBox *box, const QString &toolTip) -> bool;

    template <typename T> void InitOkCancelApply(T *ui);

    template <typename T> void InitOkCancel(T *ui);

    template <typename T> void AddVisualization();

    template <typename T> auto GetListInternals(const QListWidget *list) const -> QVector<T>;

    virtual void ShowVisualization() {}

    /**
     * @brief RestoreTransientVariables re-inject dialog-local formula variables (e.g. "CurrentLength")
     * that live only in this dialog's own container copy. SetAssociatedTool() replaces that copy
     * wholesale from the associated tool's data, which never permanently holds such variables, so
     * dialogs relying on them must restore them here to keep the "Edit length" variable list valid.
     */
    virtual void RestoreTransientVariables() {}

    /**
     * @brief UsesVisualization whether this dialog drives a scene visualization (vis) during creation.
     * Almost all tool dialogs do; the few that never create a vis (e.g. DialogGroup) override this to
     * return false so VisualizationBroken() never reports them as broken just because vis is null.
     */
    virtual auto UsesVisualization() const -> bool { return true; }
    /**
     * @brief SaveData Put dialog data in local variables
     */
    virtual void SaveData() {}
    auto DNumber(const QString &baseName) const -> quint32;
    void NewNodeItem(QListWidget *listWidget, const VPieceNode &node, bool showPassmark = true,
                     bool showExclusion = true);

    void InitNodeAngles(QComboBox *box);

    /**
     * @brief InitShowTwoSegmentsCheckBox adds the cut tools switching checkbox just above @a before.
     * Shared by the three cut dialogs so the wording and the wiring stay in one place.
     */
    void InitShowTwoSegmentsCheckBox(QVBoxLayout *layout, QWidget *before);

    /**
     * @brief InitProlongationWidgets adds the prolongation message and its line style controls just above
     * @a before. Both stay hidden until the entered length takes the cut point outside the curve.
     */
    void InitProlongationWidgets(QVBoxLayout *layout, QWidget *before);

    /** @brief Shows or hides the prolongation message and controls, and states the length. */
    void SetProlonged(bool prolonged, qreal amount);

private:
    Q_DISABLE_COPY_MOVE(DialogTool) // NOLINT

    bool m_showLabel{false};

    bool m_showTwoSegments{false};
    QCheckBox *m_showTwoSegmentsCheck{nullptr};

    /** @brief Prolongation of the curve: the look of the piece, and whether there is one at all. */
    QString m_prolongationType{};
    QString m_prolongationColor{};
    bool m_prolonged{false};
    QLabel *m_prolongationLabel{nullptr};
    QWidget *m_prolongationRow{nullptr};
    QComboBox *m_prolongationTypeCombo{nullptr};
    QComboBox *m_prolongationColorCombo{nullptr};

    template <typename T> void PrepareList(QMap<QString, quint32> &list, quint32 id) const;

    auto IsSpline(const QSharedPointer<VGObject> &obj) const -> bool;
    auto IsSplinePath(const QSharedPointer<VGObject> &obj) const -> bool;

    template <typename GObject>
    void FillCombo(QComboBox *box, GOType gType, FillComboBox rule = FillComboBox::Whole, const quint32 &ch1 = NULL_ID,
                   const quint32 &ch2 = NULL_ID) const;
};

// cppcheck-suppress unknownMacro
QT_WARNING_POP

//---------------------------------------------------------------------------------------------------------------------
template <typename T> auto DialogTool::GetListInternals(const QListWidget *list) const -> QVector<T>
{
    SCASSERT(list != nullptr)
    QVector<T> internals;
    for (qint32 i = 0; i < list->count(); ++i)
    {
        QListWidgetItem  const*item = list->item(i);
        internals.append(qvariant_cast<T>(item->data(Qt::UserRole)));
    }
    return internals;
}

//---------------------------------------------------------------------------------------------------------------------
inline auto DialogTool::GetAssociatedTool() -> VAbstractTool *
{
    return this->associatedTool;
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
/**
 * @brief InitOkCancelApply initialise OK / Cancel and Apply buttons
 * @param ui Dialog container
 */
inline void DialogTool::InitOkCancelApply(T *ui)
{
    InitOkCancel(ui);
    bApply = ui->buttonBox->button(QDialogButtonBox::Apply);
    SCASSERT(bApply != nullptr)
    connect(bApply, &QPushButton::clicked, this, &DialogTool::DialogApply);
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
/**
 * @brief InitOkCancel initialise OK and Cancel buttons
 * @param ui Dialog container
 */
inline void DialogTool::InitOkCancel(T *ui)
{
    bOk = ui->buttonBox->button(QDialogButtonBox::Ok);
    SCASSERT(bOk != nullptr)
    connect(bOk, &QPushButton::clicked, this, &DialogTool::DialogAccepted);

    QPushButton *bCancel = ui->buttonBox->button(QDialogButtonBox::Cancel);
    SCASSERT(bCancel != nullptr)
    connect(bCancel, &QPushButton::clicked, this, &DialogTool::DialogRejected);

    VAbstractApplication::VApp()->Settings()->GetOsSeparator() ? setLocale(QLocale()) : setLocale(QLocale::c());
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T> inline void DialogTool::AddVisualization()
{
    if (prepare == false)
    {
        auto *scene = qobject_cast<VMainGraphicsScene *>(VAbstractValApplication::VApp()->getCurrentScene());
        SCASSERT(scene != nullptr)

        T *toolVis = qobject_cast<T *>(vis);
        SCASSERT(toolVis != nullptr)

        if (not scene->items().contains(toolVis))
        {
            scene->addItem(toolVis);
        }

        toolVis->SetMode(Mode::Show);
        toolVis->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T> inline auto DialogTool::getCurrentCrossPoint(QComboBox *box) const -> T
{
    int value;
    bool ok = false;
    value = box->currentData().toInt(&ok);

    if (not ok)
    {
        return static_cast<T>(1);
    }

    switch (value)
    {
        case 1:
        case 2:
            return static_cast<T>(value);
        default:
            return static_cast<T>(1);
    }
}

#endif // DIALOGTOOL_H
