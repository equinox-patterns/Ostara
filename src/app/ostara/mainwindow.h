/************************************************************************
 **
 **  @file   mainwindow.h
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../vformat/vdimensions.h"
#include "../vmisc/vabstractshortcutmanager.h"
#include "../vmisc/vlockguard.h"
#include "../vpatterndb/osizerange.h"
#include "core/oexportoutput.h"
#include "oexportartboards.h"
#include "oexportcorrection.h"
#include "oexportcoverpage.h"
#include "oexportsheets.h"
#include "core/ogradedsizeranges.h"
#include "core/vcmdexport.h"
#include "mainwindowsnogui.h"

#include <QDoubleSpinBox>
#include <QHash>
#include <QMap>
#include <QPointer>
#include <QPoint>
#include <QPointF>
#include <QStringList>
#include <QUndoGroup>

#include <optional>

namespace Ui
{
class MainWindow;
}

class VMeasurements;
class QFileSystemWatcher;
class QLabel;
class DialogIncrements;
class DialogTool;
class DialogHistory;
class DialogFinalMeasurements;
class OInfoBlockWindow;
class OExportTemplateWindow;
struct OExportTemplate;
class ODockManager;
class QToolButton;
class QProgressBar;
class WatermarkWindow;
class Quuid;
class VBackgroundImageItem;
class VBackgroundImageControls;
class DrawToolBar;
class TopBar;
class BreadcrumbBar;
class QGraphicsItem;
class WelcomeScreen;
class QStackedWidget;

/**
 * @brief What the Export canvas draws of one generation: the grid, the sheets worth printing,
 *        the cover page, and the rectangle of the plan itself.
 *
 * File-scope and not nested, because a nested one would sit inside MainWindow's `private slots:`
 * region — where moc reads everything as a slot declaration, and a struct is not one.
 *
 * The figures are gathered rather than recomputed by each caller, because the status label states
 * them and the image export measures itself against them: two readings of the same thing are two
 * answers waiting to differ.
 */
struct OExportDrawing
{
    OExportArtboards::Grid artboards{};
    OExportSheets::Set sheets{};
    OExportCoverPage::Drawing cover{};

    /// The plan's own rectangle — the sheets on a tiled run, the bin on any other. This is the
    /// figure the status label states, and what an image of the plan is measured on.
    QRectF drawnRect{};
};

/**
 * @brief The MainWindow class main windows.
 */
class MainWindow final : public MainWindowsNoGUI
{
    Q_OBJECT // NOLINT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    auto LoadPattern(QString fileName, const QString &customMeasureFile = QString()) -> bool;

public slots:
    void ProcessCMD();
    void ShowToolTip(const QString &toolTip) override;
    void UpdateVisibilityGroups() override;
    void UpdateDetailsList() override;
    void ZoomFitBestCurrent() override;
    void SetToolOptionsDialogVisible(bool visible) override;
    void PlaceBackgroundImage(const QPointF &pos, const QString &fileName);
    void RemoveBackgroundImage(const QUuid &id);

signals:
    void EnableItemMove(bool move);
    void ItemsSelection(SelectionType type);

    void EnablePointSelection(bool enable);
    void EnableLineSelection(bool enable);
    void EnableArcSelection(bool enable);
    void EnableElArcSelection(bool enable);
    void EnableSplineSelection(bool enable);
    void EnableSplinePathSelection(bool enable);
    void EnableNodePointSelection(bool enable);
    void EnableDetailSelection(bool enable);
    void EnableBackgroundImageSelection(bool enable);

    void EnablePointHover(bool enable);
    void EnableLineHover(bool enable);
    void EnableArcHover(bool enable);
    void EnableElArcHover(bool enable);
    void EnableSplineHover(bool enable);
    void EnableSplinePathHover(bool enable);
    void EnableNodePointHover(bool enable);
    void EnableDetailHover(bool enable);
    void EnableImageBackgroundHover(bool enable);

    void patternClosed();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void customEvent(QEvent *event) override;
    void CleanLayout() override;
    void PrepareSceneList(PreviewQuatilty quality) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
#if defined(Q_OS_MAC)
    void ToolBarStyle(QToolBar *bar) const override;
#endif
private slots:
    void ScaleChanged(qreal scale);
    void HandleItemClicked(QGraphicsItem *item);
    void HandleItemHovered(QGraphicsItem *item);
    void ItemDoubleClicked(QGraphicsItem *item);
    void MouseMove(const QPointF &scenePos);
    void Clear();
    void PatternChangesWereSaved(bool saved);
    void LastUsedTool();
    void FullParseFile();
    void SetEnabledGUI(bool enabled);
    void GlobalChangePP(const QString &patternPiece);
    void PreviousPatternPiece();
    void NextPatternPiece();
    void ToolBarStyles();
    void ToolboxIconSize();
    void ShowPaper(int index);
    void Preferences();
    void ExportDrawAs(bool checked);
    void ExportLayoutAs(bool checked);
    void ExportDetailsAs(bool checked);

    void ArrowTool(bool checked);
    void ToolEndLine(bool checked);
    void ToolLine(bool checked);
    void ToolAlongLine(bool checked);
    void ToolMidpoint(bool checked);
    void ToolShoulderPoint(bool checked);
    void ToolNormal(bool checked);
    void ToolBisector(bool checked);
    void ToolLineIntersect(bool checked);
    void ToolSpline(bool checked);
    void ToolCubicBezier(bool checked);
    void ToolCutSpline(bool checked);
    void ToolArc(bool checked);
    void ToolEllipticalArc(bool checked);
    void ToolSplinePath(bool checked);
    void ToolCurve(bool checked);
    void ToolCubicBezierPath(bool checked);
    void ToolCutSplinePath(bool checked);
    void ToolCutCurve(bool checked);
    void ToolPointOfContact(bool checked);
    void ToolDetail(bool checked);
    void ToolPiecePath(bool checked);
    void ToolHeight(bool checked);
    void ToolTriangle(bool checked);
    void ToolPointOfIntersection(bool checked);
    void ToolUnionDetails(bool checked);
    void ToolDuplicateDetail(bool checked);
    void ToolGroup(bool checked);
    void ToolAttach(bool checked);
    void ToolRotation(bool checked);
    void ToolFlippingByLine(bool checked);
    void ToolFlippingByAxis(bool checked);
    void ToolMove(bool checked);
    void ToolCutArc(bool checked);
    void ToolLineIntersectAxis(bool checked);
    void ToolCurveIntersectAxis(bool checked);
    void ToolArcIntersectAxis(bool checked);
    void ToolPointOfIntersectionArcs(bool checked);
    void ToolPointOfIntersectionCircles(bool checked);
    void ToolPointOfIntersectionCurves(bool checked);
    void ToolPointFromCircleAndTangent(bool checked);
    void ToolPointFromArcAndTangent(bool checked);
    void ToolArcWithLength(bool checked);
    void ToolTrueDarts(bool checked);
    void ToolInsertNode(bool checked);
    void ToolPlaceLabel(bool checked);
    void ToolArcStart(bool checked);
    void ToolArcEnd(bool checked);
    void ToolEllipticalArcWithLength(bool checked);
    void ToolParallelCurve(bool checked);
    void ToolGraduatedCurve(bool checked);
    void ToolArmholeCurve(bool checked);

    void ActionDraw(bool checked);
    void ActionDetails(bool checked);
    void ActionLayout(bool checked);
    void ActionGraded(bool checked);
    void ActionExport(bool checked);

    /** @brief Resolve a size range and nest it on the imposed width. Draws NOTHING: the
     *         result goes back to the caller, which files it and shows it. Empty when
     *         the run failed — the reason has already been shown to the user.
     *         The context is passed in rather than re-read, so the generation records
     *         the table it was really made under, not the one in force seconds later. */
    auto RunExportNesting(const OExportRunRequest &request, const OGradedSizeRanges::Context &context)
        -> std::optional<OExportOutput>;

    /** @brief Nest @p request, file the result under @p name, and show it. This is the one
     *         way a generation enters the dock — the entry dialog's own Generate button and
     *         every line of its list to generate come through here.
     *
     *         The name is settled by the caller, never derived here: the dialog is the only
     *         side that knows the labels a single size is named by, and the only one that can
     *         see the list a name must not collide with. It is used as given, which is what
     *         makes the name on the line and the name in the dock the same name.
     *  @return false when the run failed — the reason has already been shown. */
    auto GenerateNamedExport(const OExportRunRequest &request, const QString &name,
                             const OGradedSizeRanges::Context &context) -> bool;

    /**
     * @brief What a generation is drawn with — on the canvas as in the PDF.
     *
     * The assembly marks template it was made with, resolved from the live file, and whether
     * its format is the one that carries a frame. Both are read here and nowhere else: a class
     * of drawing that looked up its own template would be a SECOND place deciding what gets
     * drawn, and the two would drift — the canvas showing a template the file no longer holds,
     * or the PDF drawing one the screen never showed.
     *
     * @param marks  the resolved template; untouched when none resolved.
     * @param framed whether the format is one of the framed ones. Read off the print model
     *               rather than guessed from the geometry: the frame is a property of the
     *               model, and not of its size.
     * @return whether a template resolved at all. False is a state, not an error: a folder the
     *         user has emptied leaves every generation without marks, and the plan is drawn
     *         alone — as it was before there were templates.
     */
    auto ExportMarksOf(const OExportOutput &output, OExportTemplate &marks, bool &framed) const
        -> bool;

    /** @brief The figures of @p output as the Export canvas reads them. Computes, draws nothing.
     *
     *  Separate from the drawing itself so that the caller can slip between the two: the
     *  correction has to be told the scene is ABOUT to be rebuilt while the item holding its
     *  mouse grab still exists, and that call belongs between this and DrawExportDrawing. */
    auto ComputeExportDrawing(const OExportOutput &output) const -> OExportDrawing;

    /** @brief Draw @p output into @p scene, exactly as the Export canvas draws it.
     *
     *  **ONE place decides what is drawn.** The screen calls this pair to fill the scene it
     *  displays, and an image export calls it to fill a scene of its own — a batch cannot borrow
     *  the canvas's, which holds the generation on show and no other. Neither can come apart
     *  from the other, because there is only one of them.
     *
     *  It draws the plan and nothing of the screen: the correction's holder is mounted by
     *  OExportCorrection on the displayed scene alone, which is why a scene filled here carries
     *  no editing tool — a frame a client would have received. */
    void DrawExportDrawing(QGraphicsScene *scene, const OExportOutput &output,
                           const OExportDrawing &drawing) const;

    /** @brief Draw @p output in the Export scene. Recomputes nothing — no nesting, no
     *         geometry: that is the whole point of keeping a generation. */
    void ShowExportOutput(const OExportOutput &output, bool zoomFit);

    /** @brief The ids of the generations of the current (table, axis) context, in store order. */
    auto VisibleExportOutputIds() const -> QVector<quint32>;

    /** @brief The active generation when the filter lists it, else the last one it keeps.
     *         The filter can hide the active one — the measurements table changed under
     *         the session — and showing a row the list does not carry would be an
     *         invisible selection. */
    auto ActiveVisibleOutput() const -> const OExportOutput *;

    /** @brief Push the filtered rows to the Outputs dock (no-op when it is not built). */
    void RefreshOutputsDock();

    /** @brief The entry flow of the Export mode: an empty filtered list asks what to nest,
     *         a non-empty one warns when the pattern moved on, then shows the last
     *         generation open. Never relaunches anything by itself.
     * @return false only when the generation dialog was cancelled on a fresh entry — the
     *         caller then replays the previous mode. */
    auto EnterExportFlow() -> bool;

    /** @brief Open the "Add an output" flow from the dock button. A cancellation is a
     *         no-op (the mode stays, the dock can be clicked again).
     * @return always true — a failed nesting keeps the state as it was and the reason has
     *         been shown to the user. */
    auto AddExportOutput() -> bool;

    /** @brief Open the "Add an output" flow on a fresh Export-mode entry: a cancellation
     *        signals the caller to replay the previous mode (returns false).
     * @return false only when the dialog was cancelled; a failed nesting keeps the state
     *         as it was and returns true. */
    auto AddExportOutputFromEntry() -> bool;

    /** @brief Shared body — see AddExportOutput() / AddExportOutputFromEntry() above for
     *        the @p allowCancel contract. */
    auto AddExportOutputImpl(bool allowCancel) -> bool;

    /** @brief Apply an inline rename from the dock. The dock has already rejected empty
     *         and conflicting names; this is the write-through to the store + a refresh. */
    void RenameExportOutput(quint32 id, const QString &newName);

    /** @brief Show the generation @p id in the canvas — a click on the dock's status icon.
     *         Nothing is recomputed: the generation carries its own result. */
    void SetActiveExportOutput(quint32 id);

    /** @brief Add the recipe of the generation @p id to the favorites of the current
     *         (table, axis) context, or take it out when it is already there — the click on
     *         the dock's star. Favorites are software-level (never in the .ostara): every
     *         pattern loading the same measurements table offers the same ones. */
    void ToggleExportFavorite(quint32 id);

    /** @brief Re-run the nesting of the generation @p id with what it recorded when it was
     *         made, and keep its row. Never runs on its own — the user asks for it. */
    void RefreshExportOutput(quint32 id);

    /** @brief Redraw the shown generation after a template was saved in the editor.
     *
     *         The nesting is NOT re-run: the plan has not moved, only the ground it is drawn
     *         on. The generation goes back to the canvas exactly as it was, zoom included. */
    void RefreshExportDrawing();

    /** @brief Drop the generation @p id, after confirming with the user. Undo will not
     *         bring it back: a generation is session-scoped, never part of the document. */
    void DeleteExportOutput(quint32 id);

    /// @brief Write the plan of @p id as a PDF. Asks where, once, and does the rest.
    void ExportOutputToPdf(quint32 id);

    /// @brief Write the plan of @p id for a domestic plotter. Asks where, once, and does the rest.
    ///
    /// Le **même bouton** que le PDF, et le dialogue de son type de run : c'est ce qui garde au dock
    /// un seul endroit qui décide ce qu'un run produit, sans une colonne de plus ni un menu.
    void ExportOutputToPlotterSvg(quint32 id);

    /** @brief Write the plan the canvas is showing as an image — SVG or PNG. Asks where, what
     *         format and what size, once, and does the rest.
     *
     *  No id, where ExportOutputToPdf takes one: the dock names a generation, and a button of
     *  the toolbar can only speak for what is on screen. What it reads is the mode's own rule,
     *  ActiveVisibleOutput(), so the button and the canvas cannot mean two different plans. */
    void ExportActiveOutputToImage();

    /**
     * @brief Write several generations of the current context in one go — the dock's
     *        "Export several outputs" button.
     *
     *  The dialog asks the category: Print writes a PDF per generation (with the decor layer as
     *  an option for the whole run), Plotter writes a SVG per generation, and the Cutting plan
     *  tab is there and disabled — a cutting plan has no export yet. Each page carries its own
     *  folder, so the batch is one format at a time.
     *
     *  No id, like the image export: a batch is not about a row. The generations offered are the
     *  ones the dock lists — the current (table, axis) context, resolved here because it is the
     *  only owner of that rule — and a cutting plan is left out of both pages.
     */
    void ExportSeveralOutputs();

    void on_actionNew_triggered();
    bool hasOpenPattern() const;
    bool on_actionSaveAs_triggered(); // NOLINT(modernize-use-trailing-return-type)
    bool on_actionSave_triggered();   // NOLINT(modernize-use-trailing-return-type)
    void on_actionOpen_triggered();

    void on_actionOpenPuzzle_triggered();
    void on_actionCreateManualLayout_triggered();
    void on_actionUpdateManualLayout_triggered();

    void ActionAddBackgroundImage();


    /**
     * @brief Opens a file dialog to allow the user to select one or more
     * single-line font files for installation.
     * It supports TrueType (.ttf), OpenType (.otf), and SVG (.svg) font formats.
     */
    void SetupDrawToolsIcons();

    void ClosedDialogUnionDetails(int result);
    void ClosedDialogDuplicateDetail(int result);
    void ClosedDialogGroup(int result);
    void ClosedDialogAttach(int result);
    void ClosedDialogPiecePath(int result);
    void ClosedDialogInsertNode(int result);
    void ClosedDialogPlaceLabel(int result);
    void ClosedCutCurveDialog(int result);
    void ClosedCurveDialog(int result);
    void ClosedArmholeCurveDialog(int result);
    void ClosedPieceDialog(int result);
    void ApplyPieceDialog(VMainGraphicsScene *scene);

    // Ostara Measurement
    void OnLoadMeasurements();
    void OnEditMeasurements();
    void OnNewMeasurements();
    void LoadAndConnectMeasurements(const QString &path);

    void CreateWatermark();
    void EditCurrentWatermark();
    void LoadWatermark();
    void RemoveWatermark();
#if defined(Q_OS_MAC)
    void OpenAt(QAction *where);
#endif // defined(Q_OS_MAC)

    void DimensionABaseChanged();
    void DimensionBBaseChanged();
    void DimensionCBaseChanged();

    void GradationChanged();

    /** @brief Arm the deferred piece-miniature refresh (coalesces a burst of edits). */
    void ScheduleThumbnailSnapshots();
    /** @brief Capture every piece miniature at the reference size, then restore the active size. */
    void RefreshThumbnailSnapshots();
    /** @brief Stand the document on a size and rebuild every piece there. */
    void ApplyReferenceSize(qreal baseA, qreal baseB, qreal baseC);
    /** @brief Tell the miniature blocks whether the geometry to show is the live one. */
    void UpdateThumbnailReferenceFlag();

    void ShowProgress();
    void ClearPatternMessages();

    void AskDefaultSettings();

    void AddBackgroundImageItem(const QUuid &id);
    void DeleteBackgroundImageItem(const QUuid &id);
    void ShowBackgroundImageInExplorer(const QUuid &id);
    void SaveBackgroundImage(const QUuid &id);

    void ParseBackgroundImages();

    void ActionExportRecipe_triggered();
    void ActionNewDraw_triggered();
    void ActionTable_triggered();
    void ActionFinalMeasurements_triggered();
    void ActionInfoBlock_triggered(int page);
    void ActionExportTemplates_triggered(int tab);
    void ActionManageMeasurements();
    void ActionShowMainPath_triggered(bool checked);
    void ActionBoundaryTogetherWithNotches_triggered(bool checked);

    void UpdateShortcuts();

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(MainWindow) // NOLINT
    /** @brief ui keeps information about user interface */
    Ui::MainWindow *ui;

    /** @brief tool current tool */
    Tool m_currentTool{Tool::Arrow};

    /** @brief tool last used tool */
    Tool m_lastUsedTool{Tool::Arrow};

    /** @brief sceneDraw draw scene. */
    VMainGraphicsScene *m_sceneDraw{nullptr};

    /** @brief sceneDetails details scene. */
    VMainGraphicsScene *m_sceneDetails{nullptr};
    VMainGraphicsScene *m_sceneGraded{nullptr};

    /** @brief sceneExport export mode scene. */
    VMainGraphicsScene *m_sceneExport{nullptr};

    /// The generations of this session. Never written to the .ostara: a result filed in
    /// the document would read as pattern data, and therefore as up to date.
    OExportOutputStore m_exportOutputs{};

    /// @brief Runs of our own that must be over before this window may be deleted.
    ///
    /// The window is deleted on close, and that deletion is performed by whichever event loop is
    /// running when control returns to one. A run waits on a loop of its own — the nesting engine
    /// has no abort hook, so the loop outlives the dialog announcing it — and a close asked for
    /// during a run was carried out by the run's own loop, deleting this window while the run was
    /// still working through it. While the count is above zero the close is remembered instead;
    /// @ref ApplyDeferredClose grants it once the count is back to zero and no dialog is up.
    int m_workInFlight{0};

    /// @brief Whether a close was asked for while a run was in flight.
    bool m_closeRequested{false};

    /** @brief Declares a run in flight, and calls @ref EndWorkInFlight when it ends. */
    void BeginWorkInFlight();

    /** @brief Ends a run, and grants a close remembered while it lasted. */
    void EndWorkInFlight();

    /** @brief Closes this window if a close was remembered, and if nothing is running or asking. */
    void ApplyDeferredClose();

    /// The hand on the plan, session-scoped like the generations it corrects. Owned here
    /// rather than by the Export mode because a correction has to survive a mode switch — the
    /// user leaves the canvas and comes back to the plan exactly as they left it.
    OExportCorrection *m_exportCorrection{nullptr};

    /// The histories of the application and the switch between them. Created once, in
    /// InitScenes, right after the correction whose stack it takes — both stacks exist by then,
    /// and CreateMenus, which builds the Undo and Redo actions from this group, runs later.
    ///
    /// Before this, nothing stopped a Ctrl+Z pressed in Export from undoing a Draw-mode edit:
    /// the commands only ever consulted the application's single stack, which knows nothing of
    /// modes. The group is what makes the boundary real, and switchScene is what moves it.
    QUndoGroup *m_undoGroup{nullptr};

    /// Name of the active size range in Graded mode (empty when none yet).
    QString m_activeGradedRangeName{};

    /// Graded reference drag: base (un-moved) position of each reference piece.
    QHash<quint32, QPointF> m_gradedBasePositions{};
    bool m_gradedDragHooked{false};
    /// Re-entrancy guard around the (expensive) graded scene capture.
    bool m_gradedBuilding{false};
    /// Structural signature of the last built graded scene (range + piece/group set). The
    /// canvas is only rebuilt when it changes — plain moves in Details must not relocate it.
    QString m_gradedSceneSignature{};
    /// Frozen Details base (mx/my) per piece, captured when the piece first enters a graded
    /// build. Details moves afterwards never relocate the graded canvas.
    QHash<quint32, QPointF> m_gradedFrozenBases{};
    /// When true, the next BuildGradedScene always re-captures (entry from another mode), so
    /// ANY piece-shape change in Details is picked up — not only the structural ones.
    bool m_gradedForceCapture{false};
    /// Content of the active size range as it was when the graded scene was last applied. The
    /// deferred rebuild diffs it against the current range to decide the targeted refresh scope.
    OSizerange m_gradedAppliedRange{};

    /// Right-click alignment edit pending: when set, the deferred undo-stack hook runs
    /// RealignGradedUnit(unit) instead of the whole-canvas BuildGradedScene — the alignment
    /// change refreshes geometry of that single unit only, never the other units nor their decor.
    quint32 m_gradedPendingAlignUnit{0};
    bool m_gradedPendingAlignIsGroup{false};

    /** @brief Canvas holders rendering each piece group's common children. */
    QHash<quint32, class OPieceGroupHolderItem *> m_groupHolders;

    /** @brief Last effective piece count seen, to re-render info blocks only when it changes. */
    int m_lastEffectivePieceCount{-1};

    /** @brief isInitialized true after first show window. */
    bool m_isInitialized{false};

    bool m_patternReadOnly{false};

    QPointer<DialogIncrements> m_dialogTable;
    QPointer<DialogTool> m_dialogTool{};
    QPointer<DialogFinalMeasurements> m_dialogFMeasurements;
    QPointer<OInfoBlockWindow> m_infoBlockWindow;
    QPointer<OExportTemplateWindow> m_exportTemplateWindow;

    /** @brief comboBoxDraws comboc who show name of pattern peaces. */
    QComboBox *m_comboBoxDraws{nullptr};
    QLabel *m_patternPieceLabel{nullptr};

    DrawToolBar *m_drawToolBar{nullptr};
    TopBar      *m_topBar{nullptr};
    BreadcrumbBar *m_breadcrumb{nullptr};

    QStackedWidget *m_stackedWidget{nullptr};
    WelcomeScreen  *m_welcomeScreen{nullptr};

    QComboBox *m_comboBoxPenStyle{nullptr};
    QLabel *m_globalPenStyleLabel{nullptr};

    QComboBox *m_colorCombo{nullptr};
    QLabel *m_globalColorLabel{nullptr};

    /** @brief currentDrawIndex save current selected pattern peace. */
    qint32 m_currentDrawIndex{0};

    /** @brief drawMode true if we current draw scene. */
    bool m_drawMode{true};

    QLabel *m_leftGoToStage{nullptr};
    QLabel *m_rightGoToStage{nullptr};
    QTimer *m_autoSaveTimer{nullptr};
    bool m_guiEnabled{true};
    QPointer<QComboBox> m_dimensionA{nullptr};
    QPointer<QComboBox> m_dimensionB{nullptr};
    QPointer<QComboBox> m_dimensionC{nullptr};
    QPointer<QLabel> m_dimensionALabel{nullptr};
    QPointer<QLabel> m_dimensionBLabel{nullptr};
    QPointer<QLabel> m_dimensionCLabel{nullptr};
    QPointer<QDoubleSpinBox> m_doubleSpinBoxScale{nullptr};
    QPointer<QToolButton> m_btnZoomIn{nullptr};
    QPointer<QToolButton> m_btnZoomOut{nullptr};
    QPointer<QLabel> m_measurementFileNameLabel{nullptr};
    QPointer<QLabel> m_measurementSeparator{nullptr};
    ODockManager *m_dockManager{nullptr};
    QSharedPointer<VLockGuard<char>> m_lock{nullptr};

    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;

    QList<QPointer<WatermarkWindow>> m_watermarkEditors{};

    qreal m_currentDimensionA{0};
    qreal m_currentDimensionB{0};
    qreal m_currentDimensionC{0};

    QSharedPointer<VMeasurements> m_m{};
    QTimer *m_gradation;

    /** @brief Coalesces the piece-miniature refresh: freezing the reference size costs two parses. */
    QTimer *m_thumbnailSnapshots{nullptr};
    /** @brief Guards the miniature capture against re-entrance: it parses the document twice. */
    bool m_thumbnailSnapshotBusy{false};

    QMap<QUuid, VBackgroundImageItem *> m_backgroundImages{};
    VBackgroundImageControls *m_backgroudcontrols{nullptr};

    // Dock visibility booleans and per-mode dock layouts live in ODockManager.

    // True while a tool options (edit) dialog is open. Undo/redo are blocked during this time because
    // running them triggers a full reparse that destroys the tool and pattern container the open dialog
    // still references, leading to a use-after-free when the dialog is accepted.
    bool m_toolOptionsDialogVisible{false};

    // True when the click being routed belongs to a node-picking workflow (passmarks, foldline, group
    // foldline, custom seam allowance, group link). Those workflows are read twice per click: once while
    // routing it (HandleItemClicked) and once by the scene (ItemByMousePress). A workflow that ends on
    // the click itself — the second point of a custom seam allowance, for instance — has already reset
    // its state when the scene reads it, which used to open the clicked node's PB and drop the dock tab.
    // Freezing the answer at routing time keeps the dock where the user is working.
    bool m_pickingClick{false};

    QMultiHash<VShortcutAction, QAction *> m_shortcutActions{};

    bool IsNodePickingActive() const;

    void InitDimensionControls();
    void InitDimensionGradation(int index, const MeasurementDimension_p &dimension, const QPointer<QComboBox> &control);
    static void InitDimensionXGradation(const QVector<qreal> &bases, const DimesionLabels &labels,
                                        const QPointer<QComboBox> &control);
    void InitDimensionYWZGradation(const QVector<qreal> &bases, const DimesionLabels &labels,
                                   const QPointer<QComboBox> &control, bool bodyMeasurement);

    void UpdateMeasurementState();
    void ToolBarStages();
    void ToolBarDraws();
    void ToolBarPenOptions();
    void ToolBarTools();
    void ToolBarMeasurements();
    void UpdateMeasurementFileNameLabel();
    void UpdateMeasurementSeparatorIcon();
    void UpdateTopBarIcons();
    void UpdateMenuIcons();
    void UpdateMessagesIcons();
    void InitToolButtons();
    void InitActionShortcuts();
    void CancelTool();


    /// @brief The one place that decides whether Undo and Redo are available.
    ///
    /// Every path that could change the answer calls this, and nothing else calls setEnabled on
    /// those two actions. It has to be a single function rather than a setEnabled here and there:
    /// QUndoGroup re-evaluates the actions it created on every activeStackChanged, so an isolated
    /// setEnabled(false) anywhere would be overwritten the next time the user changed mode.
    ///
    /// @param enabled the GUI's own enabled state. Passed in rather than read from m_guiEnabled
    ///        because SetEnableWidgets runs before that member is updated — reading it there
    ///        would answer with the state the window is leaving, not the one it is entering.
    void UpdateUndoRedoActions(bool enabled);

    void SetEnableWidgets(bool enable);
    void SetEnableTool(bool enable);
    void SetLayoutModeActions();

    void SaveCurrentScene();
    void RestoreCurrentScene();
    void MinimumScrollBar();

    template <typename Dialog, typename Func>
    void SetToolButton(bool checked, Tool t, const QString &cursor, const QString &toolTip, Func closeDialogSlot);
    template <typename Dialog, typename Func, typename Func2>
    void SetToolButtonWithApply(bool checked, Tool t, const QString &cursor, const QString &toolTip,
                                Func closeDialogSlot, Func2 applyDialogSlot);
    template <typename DrawTool> void ClosedDialog(int result);

    template <typename DrawTool> void ClosedDialogWithApply(int result, VMainGraphicsScene *scene);
    template <typename DrawTool> void ApplyDialog(VMainGraphicsScene *scene);
    template <typename DrawTool> void ClosedDrawDialogWithApply(int result);
    template <typename DrawTool> void ApplyDrawDialog();
    template <typename DrawTool> void ClosedDetailsDialogWithApply(int result);
    template <typename DrawTool> void ApplyDetailsDialog();

    auto SavePattern(const QString &fileName, QString &error) -> bool;
    void AutoSavePattern();
    void setCurrentFile(const QString &fileName);

    void ReadSettings();
    void WriteSettings();

    auto MaybeSave() -> bool;
    void CreateMenus();
    //---------------------------------------------------------------------------------------------------------------------
    void CreateActions();
    void InitAutoSave();
    auto PatternPieceName(QString &name) -> bool;
    auto CheckPathToMeasurements(const QString &patternPath, const QString &path) -> QString;
    void ChangePP(int index, bool zoomBestFit = true);
    /**
     * @brief EndVisualization try show dialog after and working with tool visualization.
     */
    void EndVisualization(bool click = false);
    void ZoomFirstShow();

    void InitDocksContain();
    void RefreshGroupHolders();

    /// Leaves the "inside the miniature" sub-mode of a group's miniatures, if one is open. False when
    /// none was: the piece page's own miniature is handled by the options dock.
    auto CancelGroupThumbnailEdit() -> bool;

    void SetAllGroupHoldersInteractive(bool interactive);
    void UpdateGroupHolderSelection(quint32 groupId);
    auto OpenNewValentina(const QString &fileName = QString()) const -> bool;
    void FileClosedCorrect();
    static auto GetUnlokedRestoreFileList() -> QStringList;

    void AddPP(const QString &PPName);
    auto StartPositionNewPP() const -> QPointF;

    void InitScenes();
    void InitWelcomePage();
    void ShowWelcomeScreen();
    void ShowEditor();
    auto switchScene(Draw mode, const QVector<DetailForLayout> &details = {}) -> bool;

    /// Graded mode: create/reuse the active size range for the loaded measurements
    /// table. Returns true when a range is available and the graded canvas may show.
    bool EnterGradedMode();

    /// Edit the definition of an existing size range (Size Ranges dock, dialog
    /// pre-filled with the current values). Range content is preserved. Returns
    /// false when cancelled/invalid.
    bool EditGradedRange(const QString &rangeName);

    /// Delete an existing size range (Size Ranges dock). When the active range is
    /// deleted, another remaining range (lowest name) becomes active; an empty list
    /// clears the graded canvas. Returns false when the range does not exist.
    bool DeleteGradedRange(const QString &rangeName);

    /// Rename a size range (inline edit in the dock list). No-op when the range
    /// does not exist or the new name is already taken (the dock already rejects
    /// those before emitting).
    void RenameGradedRange(const QString &oldName, const QString &newName);

    /// Whether renaming @p oldRange to @p newName would collide with a favorite, and
    /// says so when it would. Shared by the two rename paths (inline edit, edit
    /// dialog) — they each wrote the guard and its message before this.
    auto RangeRenameHitsFavorite(const OSizerange &oldRange, const QString &newName) -> bool;

    /// Everything that named @p oldRange follows it to @p newRange: the graded
    /// favorite, the favorites of the Export mode (whose recipes name the range they
    /// were built on), and the generations of this session (replayed by name — left
    /// alone, they replay into an empty size loop). Called by the two rename paths.
    void PropagateRangeRename(const OSizerange &oldRange, const OSizerange &newRange);

    /// Make @p rangeName the active size range (single click in the dock list).
    void ActivateGradedRange(const QString &rangeName);

    /// Create a brand-new size range (Add button of the Size Ranges dock). Same
    /// creation flow as the entry dialog; the new range becomes the active one.
    bool AddGradedRange();

    /// Duplicate @p rangeName (Duplicate button): definition AND range content are
    /// copied under a fresh unique name; the copy becomes the active range. The two
    /// ranges stay independent — later edits touch only one of them.
    bool DuplicateGradedRange(const QString &rangeName);

    /// Export the range of a Size Ranges dock row to a layered SVG (one layer per
    /// size, grand → petit, plus a "Layout" layer with the reference decor).
    /// Direct export (no dialog), headless per-size recapture — the live canvas is
    /// never touched. Uses the range's stored export settings when present.
    void ExportGradedRange(const QString &rangeName);

    /// Export one size range to a layered SVG. @p forcedDir, when non-empty,
    /// overrides the range's stored export path (export-all batch); otherwise the
    /// range's path (or the software default) is used. With @p showDialogs the
    /// per-file overwrite-ask and the success/failure boxes appear (per-row
    /// export); without it the result is returned through @p writtenFilePath /
    /// @p errorOut and the caller reports once. Returns true when the file was written.
    bool ExportGradedRangeTo(const QString &rangeName, const QString &forcedDir,
                             bool showDialogs, QString *writtenFilePath,
                             QString *errorOut);

    /// Export every stored size range as its own layered SVG (one file per range,
    /// dock order = sorted names) into @p dirPath, with a single recap at the end.
    void ExportAllGradedRanges(const QString &dirPath);

    /// The name prefix of autogenerated export file names: the pattern name field
    /// (Options dock / Details), or the pattern FILE base name when that field is
    /// empty. Empty only when there is no pattern at all.
    QString GradedPatternNameToken() const;

    /// Shared "create a size range" dialog + store, used by EnterGradedMode and
    /// AddGradedRange. @p staleNameToDrop (empty = none) is only removed on the
    /// accept path. Returns the new range name, or an empty string when cancelled.
    QString CreateGradedRange(const QString &canonical, const QStringList &axes,
                              const QMap<QString, QStringList> &axisValues,
                              const QString &staleNameToDrop);

    /// Load the current measurements table's axes once (multi-size check). Returns
    /// false when the table is missing/not multi-size.
    auto LoadGradedTableAxes(QStringList *axes, QMap<QString, QStringList> *axisValues) -> bool;

    /// Refresh the Size Ranges dock list (no-op when the dock is not built).
    void RefreshSizeRangesDock();

    /// Re-capture the graded scene when Graded is the active mode (optional zoom-fit).
    void RebuildGradedCanvas(bool zoomFitBest = false);

    /// Build the stacked graded scene (one layer per size of the active range).
    void BuildGradedScene();

    /// Declared size axis of the currently loaded measurements table — empty when
    /// the table is outside the library or no axis is declared yet.
    QString CurrentLoadedTableAxis() const;

    /// The measurements-library dialog edits the declared size axis live, so when
    /// it changed for the loaded table while Graded mode is active nothing would
    /// refresh. Re-run the entry selection (filter to the new axis, re-pick or
    /// create the active range) and redraw — the same result as leaving and
    /// re-entering Graded mode.
    void RefreshGradedAfterTableAxisChange();

    // ── Graded Size Styles dock ──

    /// (Re)apply the per-size trace styles (color + px dashes) onto the existing
    /// graded scene — no rebuild. Gate "Show in canvas" + the store decide per
    /// size; sizes without a style keep the theme piece color. Used on capture,
    /// theme change, dialog OK and the gate toggle.
    void ApplyGradedSizeStylesToScene();

    /// Refresh the Size Styles dock list from the active size axis (no-op when
    /// the dock is not built).
    void RefreshSizeStylesDock();

    /// Open the "Edit size styles…" dialog for the current table's size axis;
    /// on OK: store the styles (software-level) and re-apply them on the canvas.
    void EditSizeStyles();

    /// Persist the global "Show in canvas" gate and re-apply on the scene.
    void ShowGradedSizeStylesInCanvas(bool on);

    /// Resolve the (canonical path, size axis, axis values, in-library) context the
    /// Size Styles dock, its dialog and the style pass run against. False when there
    /// is no usable table / size axis. The axis is the table's declared size axis,
    /// falling back to the active range's axis (external table, or not yet declared).
    auto GradedSizeStyleContext(QString *canonicalPath, QString *axisName, QStringList *axisValues,
                                bool *inLibrary) -> bool;

    /// Re-apply alignment + per-range content offsets onto the existing graded
    /// items (no re-capture). Used after undo/redo and at the end of a capture.
    /// When forceFull is false (a deferred rebuild after an undo/redo command), the
    /// children refresh is limited to what actually changed in the active size range.
    void ApplyGradedLayoutOffsets(bool forceFull = false);

    /// End of a graded reference drag: fan the offset out to all layers / groups
    /// and push an undo command saving the range content.
    void GradedMouseReleased();

    /// Right-click alignment menu on a graded piece/group (Horizontal/Vertical submenus, the
    /// current alignment marked with the check icon). Writes the model through the same undo
    /// commands as the Details dock, then flags a single-unit realign for the deferred hook.
    void GradedAlignContextMenu(const QPoint &globalPos);

    /// Re-align geometry of ONE graded unit (piece or group) after an alignment edit — reset to
    /// frozen base → alignment delta → stored range offset, restricted to that unit; group host
    /// is re-anchored. Children are parented to their reference item/host and follow: nothing else
    /// is refreshed (no decor rebuild, no whole-canvas re-apply).
    void RealignGradedUnit(quint32 unitId, bool isGroup);

    /// (Re)build the interactive grainlines (lone pieces + groups) on the graded reference
    /// layer, from the Details base geometry or the per-range override, and wire move/resize
    /// to save into the active size range. Positions must be final when called.
    /// onlyPieceId/onlyGroupId (0 = all owners) restrict the rebuild to a single owner — the
    /// targeted path used after a single-owner content change.
    void RefreshGradedReferenceGrainlines(quint32 onlyPieceId = 0, quint32 onlyGroupId = 0);

    /// Persist a grainline move/resize of a lone piece (isGroup=false) or of a group into the
    /// active size range and push the undo command.
    void StoreGradedGrainline(quint32 ownerId, bool isGroup, const OSizerange::GrainlineOverride &grainline);

    /// (Re)build the interactive info blocks (lone pieces + groups) on the graded reference
    /// layer — Details-identical hosting, with per-range geometry overrides. Positions must be
    /// final when called. onlyPieceId/onlyGroupId (0 = all owners) restrict the rebuild.
    void RefreshGradedReferenceInfoBlocks(quint32 onlyPieceId = 0, quint32 onlyGroupId = 0);

    /// Persist an info-block move/resize of a lone piece (isGroup=false) or of a group into
    /// the active size range and push the undo command.
    void StoreGradedInfoBlock(quint32 ownerId, bool isGroup, int block, const OInfoBlockGeom &geom);

    /// (Re)build the interactive annotations (lone pieces + groups) on the graded reference
    /// layer — Details-identical hosting, per-range geometry overrides, fixed base rotation.
    /// onlyPieceId/onlyGroupId (0 = all owners) restrict the rebuild.
    void RefreshGradedReferenceAnnotations(quint32 onlyPieceId = 0, quint32 onlyGroupId = 0);

    /// Persist an annotation move/resize of a lone piece (isGroup=false) or of a group into
    /// the active size range and push the undo command. Rotation is never stored.
    void StoreGradedAnnotation(quint32 ownerId, bool isGroup, int index, const OAnnotationGeom &geom);

    /// (Re)build the interactive miniature blocks on the graded reference layer. A lone piece carries
    /// its own on its reference item; a member of a group has its miniature carried by the group —
    /// one per member, from the group's data — on the group's host. The drawing is always the one of
    /// the reference size of the pattern: a range only carries frames, never another size's geometry.
    /// Positions must be final when called. onlyPieceId/onlyGroupId (0 = all owners) restrict the
    /// rebuild.
    void RefreshGradedReferenceThumbnails(quint32 onlyPieceId = 0, quint32 onlyGroupId = 0);

    /// Persist a miniature move/resize into the active size range and push the undo command. A lone
    /// piece stores it on itself (@p groupId NULL_ID); a group member stores it in the group, per
    /// member — the piece's own miniature is never touched by a group gesture.
    void StoreGradedThumbnail(quint32 groupId, quint32 pieceId, const QPointF &pos, qreal width, qreal height);

    // Measurement slots (from embedded WelcomeScreen)
    void WelcomeNewMeasurement();
    void WelcomeOpenMeasurement();
    void WelcomeOpenRecentMeasurement(const QString &path);
    void AddRecentMeasurementAndRefresh(const QString &path);

    auto LoadMeasurements(const QString &patternPath, QString &path) -> bool;
    auto UpdateMeasurements(const QString &patternPath, QString &path, qreal baseA, qreal baseB, qreal baseC) -> bool;
    auto UpdateMeasurements(const QSharedPointer<VMeasurements> &mFile, qreal baseA, qreal baseB, qreal baseC,
                            bool rebuildGradation = true) -> bool;

    void ReadMeasurements(qreal baseA, qreal baseB, qreal baseC, bool rebuildGradation = true);

    void ReopenFilesAfterCrash(QStringList &args);
    auto DoExport(const VCommandLinePtr &expParams) -> bool;
    auto DoFMExport(const VCommandLinePtr &expParams) -> bool;

    auto SetDimensionA(int value) -> bool;
    auto SetDimensionB(int value) -> bool;
    auto SetDimensionC(int value) -> bool;

    static auto GetPatternFileName() -> QString;
    auto GetMeasurementFileName() -> QString;

    void UpdateWindowTitle();

    void ToolSelectPoint();
    void ToolSelectPointByPress();
    void ToolSelectPointByRelease();
    void ToolSelectSpline();
    void ToolSelectSplinePath();
    void ToolSelectCutCurve();
    void ToolSelectArc();
    void ToolSelectArcElArc();
    void ToolSelectArcCurve();
    void ToolSelectPointArc();
    void ToolSelectCurve();
    void ToolSelectAllDrawObjects();
    void ToolSelectOperationObjects();
    void ToolSelectGroupObjects();
    void ToolSelectDetail();

    void PrintPatternMessage(QEvent *event);

    void OpenWatermark(const QString &path = QString());
    void CleanWaterkmarkEditors();

    void StoreMultisizeMDimensions();
    void StoreIndividualMDimensions();

    void StoreMultisizeMDimension(const QList<MeasurementDimension_p> &dimensions, int index, qreal currentBase);
    void StoreIndividualMDimension(const QMap<QString, QSharedPointer<VMeasurement>> &measurements, IMD type);

    auto DimensionRestrictedValues(int index, const MeasurementDimension_p &dimension) -> QVector<qreal>;
    void SetDimensionBases();

    void StoreDimensions();

    void ExportDraw(const QString &fileName);

    void NewBackgroundImageItem(const VBackgroundPatternImage &image);
    auto InitBackgroundImageItem(const VBackgroundPatternImage &image) -> VBackgroundImageItem *;

    auto SavePatternAs(const QString &fileName) -> bool;

    auto FullParsePattern() -> bool;

    void RefreshPieceGeometry();
};

#endif // MAINWINDOW_H
