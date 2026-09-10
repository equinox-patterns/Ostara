/************************************************************************
 **
 **  @file
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   6 11, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2016 Valentina project
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

// Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>

#ifndef VTOOLSEAMALLOWANCE_H
#define VTOOLSEAMALLOWANCE_H

#include <QGraphicsPathItem>
#include <QObject>
#include <QtGlobal>

#include "../vlayout/vfoldline.h"
#include "../vwidgets/osamarginhighlights.h"
#include "../vwidgets/vgrainlineitem.h"
#include "oannotationdata.h"
#include "oinfoblockcontent.h"
#include "vinteractivetool.h"

class DialogTool;
class VContainer;
class VGrainlineData;
class OInfoBlockData;
class VNoBrushScalePathItem;
class VPatternGraph;
class OAnnotationItem;
class OInfoBlockItem;
class ONodePlaceLabel;

struct VToolSeamAllowanceInitData : VAbstractToolInitData
{
    VPiece detail{};    // NOLINT(misc-non-private-member-variables-in-classes)
    QString width{'0'}; // NOLINT(misc-non-private-member-variables-in-classes)
    QString drawName{}; // NOLINT(misc-non-private-member-variables-in-classes)
};

// Snapshot of a piece's computed scene geometry. Produced off the GUI thread by
// VToolSeamAllowance::ComputePieceGeometry() (no QGraphicsItem access) and consumed on the GUI
// thread by VToolSeamAllowance::ApplyPieceGeometry(). This split lets many pieces be recomputed in
// parallel before their results are applied to the scene.
struct VToolSeamAllowanceGeometry
{
    bool valid{false}; // false if the geometry could not be computed; such a piece is skipped on apply
    QPointF pos{};
    bool showMainPath{false};
    QPainterPath seamPath{};
    QPainterPath passmarks{};
    bool combineTogether{false};
    VFoldLine foldLine{};
    QPainterPath mirrorLine{};
    bool hasSeamAllowance{false};
    bool seamAllowanceValid{true};
    QPainterPath seamAllowance{};
    QPainterPath placeLabels{};
    QString pieceName{};
};

class VToolSeamAllowance final : public VInteractiveTool, public QGraphicsPathItem
{
    Q_OBJECT // NOLINT

public:
    ~VToolSeamAllowance() override;

    static auto Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                       VContainer *data) -> VToolSeamAllowance *;
    static auto Create(VToolSeamAllowanceInitData &initData) -> VToolSeamAllowance *;
    static auto Duplicate(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc)
        -> VToolSeamAllowance *;
    static auto Duplicate(VToolSeamAllowanceInitData &initData) -> VToolSeamAllowance *;

    static void AddPieceDependencies(quint32 id,
                                     const VPiece &piece,
                                     VAbstractPattern *doc,
                                     const QHash<QString, QList<quint32>> &variables);

    static const quint8 pieceVersion;

    static const QString TagCSA;
    static const QString TagRecord;
    static const QString TagIPaths;
    static const QString TagPlaceLabels;
    static const QString TagSaMarginProperties;
    static const QString TagCartoucheBlocks;
    static const QString TagDispatch;
    static const QString TagBlock;
    static const QString TagAnnotations;
    static const QString TagAnnotation;
    static const QString TagText;
    static const QString TagGeom;

    static const QString AttrSeamAllowance;
    static const QString AttrHideMainPath;
    static const QString AttrShowFullPiece;
    static const QString AttrSeamAllowanceBuiltIn;
    static const QString AttrUnited;
    static const QString AttrFont;
    static const QString AttrPiecePriority;
    static const QString AttrActive;
    static const QString AttrBlock;
    static const QString AttrIndex;
    static const QString AttrFontScale;

    void RemoveWithConfirm(bool ask);

    static auto InsertNodes(const QVector<VPieceNode> &nodes, quint32 pieceId, VMainGraphicsScene *scene,
                            VContainer *data, VAbstractPattern *doc) -> bool;

    static void AddAttributes(VAbstractPattern *doc, QDomElement &domElement, quint32 id, const VPiece &piece);
    static void AddCSARecord(VAbstractPattern *doc, QDomElement &domElement, CustomSARecord record);
    static void AddCSARecords(VAbstractPattern *doc, QDomElement &domElement, const QVector<CustomSARecord> &records);
    static void AddInternalPaths(VAbstractPattern *doc, QDomElement &domElement, const QVector<quint32> &paths);
    static void AddPlaceLabels(VAbstractPattern *doc, QDomElement &domElement, const QVector<quint32> &placeLabels);
    static void AddGrainline(VAbstractPattern *doc, QDomElement &domElement, const VGrainlineData &glGeom);
    static void AddSAMarginProperties(VAbstractPattern *doc, QDomElement &domElement, const VPiece &piece);
    static void AddMirrorLine(VAbstractPattern *doc, QDomElement &domElement, const VPiece &piece);
    static void AddCartoucheBlocks(VAbstractPattern *doc, QDomElement &domElement, const OInfoBlockData &info);
    static void AddAnnotations(VAbstractPattern *doc, QDomElement &domElement, const OAnnotationData &data);
    static void AddPieceGroups(VAbstractPattern *doc, QDomElement &domElement, const VContainer *data);

    /** @brief Registers the app-side content builder (called once at app startup). */
    static void SetInfoBlockContentBuilder(OInfoBlockContentBuilder builder);
    static auto AnnotationStyleResolver() -> const OAnnotationStyleResolver &;
    /** @brief Registers the app-side annotation style resolver (called once at app startup). */
    static void SetAnnotationStyleResolver(OAnnotationStyleResolver resolver);

    void SetName(const QString &name);
    void Move(qreal x, qreal y);
    void Update(const VPiece &piece);
    void DisconnectOutsideSignals();
    void ConnectOutsideSignals();
    void ReinitInternals(const VPiece &detail, VMainGraphicsScene *scene);
    void RefreshGeometry(bool updateChildren = true);
    void RefreshScale();
    /** @brief Deselects every place-label handle of this piece (clears their Qt selection). */
    void ClearPlaceLabelSelection();
    /** @brief Sets a place label's Type through an undo command (canvas context menu).
     *  @a type is the PlaceLabelType enum value passed as int to keep the enum out of this header. */
    void ChangePlaceLabelType(quint32 labelId, int type);

    // Two-phase refresh for batch updates. PrepareRefreshGeometry() computes the geometry off the
    // GUI thread (safe to call concurrently for different pieces); ApplyBatchGeometry() applies the
    // prepared result and must run on the GUI thread.
    void PrepareRefreshGeometry(bool combineTogether, bool pieceShowMainPath);
    void ApplyBatchGeometry(bool updateChildren = true);

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Tool::Piece)
    };

    auto getTagName() const -> QString override;
    void ShowVisualization(bool show) override;
    void GroupVisibility(quint32 object, bool visible) override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    auto boundingRect() const -> QRectF override;
    auto shape() const -> QPainterPath override;

public:
    void EnterPieceIsolation();
    void ExitPieceIsolation();
    auto IsInPieceIsolation() const -> bool { return m_isPieceIsolated; }
    void SetGroupSelectionHighlight(bool highlight);
    void RefreshGroupedChildren();
    /** @brief Highlight or clear a whole internal path of this piece: its trace
     *  (VToolPiecePath) and its point nodes (VNodePoint). */
    void SetInternalPathHighlight(quint32 pathId, bool highlight);

public slots:
    void FullUpdateFromGuiApply() override;
    void FullUpdateFromFile() override;
    void EnableToolMove(bool move);
    void AllowHover(bool enabled) override;
    void AllowSelecting(bool enabled) override;
    void ResetChildren(QGraphicsItem *pItem);
    void UpdateAll();
    void retranslateUi();
    void Highlight(quint32 id);
    void UpdatePassmarks();
    void ShowOptions();
    void DeleteFromMenu();
    /** @brief Re-render this piece's information blocks (e.g. when the effective piece
     *  count changes because a group is created/removed). Public so the main window can
     *  re-render every piece's blocks without touching the geometry. */
    void UpdateInfoBlocks();

private slots:
    void UpdateGrainline();
    void RefreshFoldLine();
    void SaveMoveGrainline(const QPointF &ptPos);
    void SaveResizeGrainline(qreal dLength);
    void SaveRotateGrainline(qreal dRot, const QPointF &ptPos);
    void SaveMoveInfoBlock(int blockIndex, const QPointF &pos);
    void SaveResizeInfoBlock(int blockIndex, qreal newW, qreal newH, qreal origW, qreal origH);
    void UpdateAnnotations();
    void SaveMoveAnnotation(int index, const QPointF &pos);
    void SaveResizeAnnotation(int index, qreal newW, qreal newH, qreal origW, qreal origH);
    void ToggleInLayout(bool checked);
    void ToggleHideMainPath(bool checked);
    void ToggleForbidFlipping(bool checked);
    void ToggleForceFlipping(bool checked);
    void ToggleShowFullPiece(bool checked);
    void ToggleExcludeState(quint32 id);
    void ToggleTurnPointState(quint32 id);
    void ToggleNodePointAngleType(quint32 id, PieceNodeAngle type);
    void ToggleNodePointPassmark(quint32 id, bool toggle);
    void TogglePassmarkAngleType(quint32 id, PassmarkAngleType type);
    void TogglePassmarkLineType(quint32 id, PassmarkLineType type);

private:
    Q_DISABLE_COPY_MOVE(VToolSeamAllowance) // NOLINT

    QPainterPath m_mainPath{}; // Must be first to prevent crash
    QRectF m_pieceBoundingRect{};

    /** @brief sceneDetails pointer to the scene. */
    VMainGraphicsScene *m_sceneDetails;
    QString m_drawName;

    VNoBrushScalePathItem *m_seamAllowance;
    VGrainlineItem *m_grainLine;
    QGraphicsPathItem *m_passmarks;
    QGraphicsPathItem *m_placeLabels;
    QGraphicsPathItem *m_mirrorLine;
    QGraphicsPathItem *m_foldLineMark;
    QGraphicsPathItem *m_foldLineLabel;
    QGraphicsSimpleTextItem *m_foldLineLabelText;
    OSAMarginHighlights m_saHighlights;

    /** @brief On-piece information block items (one per block 1..11), hidden by default. */
    QList<OInfoBlockItem *> m_infoBlockItems;

    /** @brief On-piece annotation items (one per annotation, grown on demand, never recreated). */
    QList<OAnnotationItem *> m_annotationItems;

    /** @brief Per-place-label clickable handles (one per label, grown on demand, never
     *  recreated). The labels themselves stay drawn by the aggregated m_placeLabels path;
     *  these handles give each label a "real point" to hover/click in Details. */
    QList<ONodePlaceLabel *> m_placeLabelItems;

    /** @brief App-registered content builder (layering bridge). */
    static OInfoBlockContentBuilder s_infoBlockContentBuilder;

    /** @brief App-registered annotation style resolver (layering bridge). */
    static OAnnotationStyleResolver s_annotationStyleResolver;

    bool m_acceptHoverEvents{true};

    /** @brief Piece isolation: only this piece's children are editable (foldline definition). */
    bool m_isPieceIsolated{false};

    /** @brief Guards the "Move piece group" undo macro (ported from Val Fork events.cpp). */
    static bool s_isInGroupMoveMacro;

    /** @brief Group Link selection: draw the orange contour. */
    bool m_groupSelectionHighlight{false};

    /** @brief m_geometryIsReady is true when a piece's geometry is ready and checks for validity can be enabled. */
    bool m_geometryIsReady{false};

    /** @brief Geometry computed by PrepareRefreshGeometry() (off the GUI thread) and consumed by
     * ApplyBatchGeometry() (on the GUI thread). */
    VToolSeamAllowanceGeometry m_batchGeometry{};

    explicit VToolSeamAllowance(const VToolSeamAllowanceInitData &initData, QGraphicsItem *parent = nullptr);

    auto ResolveAnnotationStyle(const QString &name) const -> OAnnotationRenderStyle;
    auto EvaluateAnnotationRotation(const QString &formula) const -> qreal;

    void AddToFile() override;
    void RefreshDataInFile() override;
    auto itemChange(GraphicsItemChange change, const QVariant &value) -> QVariant override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void SetVisualization() override {}
    void DeleteToolWithConfirm(bool ask = true) override;
    void ToolCreation(const Source &typeCreation) override;
    void SetDialog() override;
    void SaveDialogChange(const QString &undoText = QString()) override;

    void UpdateExcludeState();
    void UpdateInternalPaths();
    void HideOrphanInternalPaths(const QVector<quint32> &currentPaths);
    void UpdatePlaceLabels();

    auto FindGrainlineGeometry(const VGrainlineData &geom, qreal &length, qreal &rotationAngle, QPointF &pos)
        -> VPieceItem::MoveTypes;

    void InitNodes(const VPiece &detail, VMainGraphicsScene *scene);
    static void InitNode(const VPieceNode &node, VMainGraphicsScene *scene, VToolSeamAllowance *parent);
    static void InitInternalNode(const VPieceNode &node, VToolSeamAllowance *parent);
    void InitInternalPaths(const VPiece &detail);

    auto SelectedTools() const -> QList<VToolSeamAllowance *>;

    auto IsGrainlinePositionValid() const -> bool;
    auto IsFoldLinePositionValid(const QVector<QPainterPath> &shape, FoldLineType type) const -> bool;

    static void AddPointRecords(VAbstractPattern *doc, QDomElement &domElement, const QVector<quint32> &records,
                                const QString &tag);

    static auto DuplicateNodes(const VPiecePath &path, const VToolSeamAllowanceInitData &initData,
                               QMap<quint32, quint32> &replacements) -> QVector<VPieceNode>;
    static auto DuplicateNode(const VPieceNode &node, const VToolSeamAllowanceInitData &initData) -> quint32;

    static auto DuplicatePiecePath(quint32 id, const VToolSeamAllowanceInitData &initData) -> quint32;

    static auto DuplicateCustomSARecords(const QVector<CustomSARecord> &records,
                                         const VToolSeamAllowanceInitData &initData,
                                         const QMap<quint32, quint32> &replacements) -> QVector<CustomSARecord>;

    static auto DuplicateInternalPaths(const QVector<quint32> &iPaths, const VToolSeamAllowanceInitData &initData)
        -> QVector<quint32>;
    static auto DuplicatePlaceLabels(const QVector<quint32> &placeLabels, const VToolSeamAllowanceInitData &initData)
        -> QVector<quint32>;

    void UpdateFoldLine(const VFoldLine &foldLine);

    auto ComputePieceGeometry(bool combineTogether, bool pieceShowMainPath) const -> VToolSeamAllowanceGeometry;
    void ApplyPieceGeometry(const VToolSeamAllowanceGeometry &geom, bool updateChildren);
};

#endif // VTOOLSEAMALLOWANCE_H
