/************************************************************************
 **
 **  @file   vpattern.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   24 2, 2014
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

#ifndef VPATTERN_H
#define VPATTERN_H

#include "../ifc/xml/vabstractpattern.h"
#include "../ifc/xml/vpatternconverter.h"
#include "../vpatterndb/vcontainer.h"

class VMainGraphicsScene;
class VNodeDetail;
struct VToolSinglePointInitData;
struct VToolLinePointInitData;
struct OToolTransfertInitData;
class VToolSeamAllowance;
class OInfoBlockData;
class OAnnotationData;
class QTimer;

/**
 * @brief The VPattern class working with pattern file.
 */
class VPattern : public VAbstractPattern
{
    Q_OBJECT // NOLINT

public:
    VPattern(VContainer *data, VMainGraphicsScene *sceneDraw, VMainGraphicsScene *sceneDetail,
             QObject *parent = nullptr);
    ~VPattern() override = default;

    void CreateEmptyFile() override;

    void Parse(const Document &parse);

    void Clear() override;

    void setCurrentData();
    void UpdateToolData(const quint32 &id, VContainer *data) override;

    auto GetCompleteData() const -> VContainer override;
    auto GetCompletePPData(const QString &name) const -> VContainer override;

    auto SPointActiveDraw() -> quint32;

    auto GetActivePPPieces() const -> QVector<quint32>;

    auto SaveDocument(const QString &fileName, QString &error) -> bool override;

    auto ActiveDrawBoundingRect() const -> QRectF;

    void AddEmptyIncrement(const QString &name, IncrementType type = IncrementType::Increment);
    void AddEmptyPreviewCalculation(const QString &name, IncrementType type = IncrementType::Increment);

    void AddEmptyIncrementAfter(const QString &after, const QString &name,
                                IncrementType type = IncrementType::Increment);
    void AddEmptyPreviewCalculationAfter(const QString &after, const QString &name,
                                         IncrementType type = IncrementType::Increment);

    void RemoveIncrement(const QString &name);
    void RemovePreviewCalculation(const QString &name);

    void MoveUpIncrement(const QString &name);
    void MoveUpPreviewCalculation(const QString &name);

    void MoveDownIncrement(const QString &name);
    void MoveDownPreviewCalculation(const QString &name);

    void SetIncrementName(const QString &name, const QString &text);
    void SetPreviewCalculationName(const QString &name, const QString &text);

    void SetIncrementFormula(const QString &name, const QString &text);
    void SetPreviewCalculationFormula(const QString &name, const QString &text);

    void SetIncrementDescription(const QString &name, const QString &text);
    void SetPreviewCalculationDescription(const QString &name, const QString &text);

    void SetIncrementSpecialUnits(const QString &name, bool special);
    void SetPreviewCalculationSpecialUnits(const QString &name, bool special);

    void ReplaceNameInFormula(QVector<VFormulaField> &expressions, const QString &name, const QString &newName);

    auto GenerateLabel(const LabelType &type, const QString &reservedName = QString()) const -> QString override;

    auto IsReadOnly() const -> bool;
    void SetReadOnly(bool rOnly);

    auto GetLabelPrefix() const -> QString;
    void SetLabelPrefix(const QString &prefix);

    void LiteParseIncrements();

    /// Rewrite the `<lineNames>` section from the container — that section has one author, and this is it: the
    /// save path asks for it too. A caller that has just registered line names in the container (an import does)
    /// must ask for it as well, because a parse reads the **section**, never the container.
    void SyncLineNamesToDom();

    static const QString AttrReadOnly;
    static const QString AttrLabelPrefix;
    static const QString AttrHighestId;

    auto ElementsToParse() const -> int;

    auto IsPieceGeometryDirty() const -> bool;
    void SetPieceGeometryDirty(bool newPieceGeometryDirty);

    void RefreshDirtyPieceGeometry(const QList<vidtype> &list);

    // Apply the pending piece geometry now, whatever the active canvas. The mode-guarded slot only
    // defers the flush when the app is off the modeling canvas, which is not good enough for a caller
    // that reads the scene items right away — the piece thumbnail snapshot does exactly that.
    void FlushPieceGeometry();

    void SetGBBackupFilePath(const QString &fileName);

signals:
    void PreParseState();

public slots:
    void LiteParseTree(const Document &parse) override;

    void RefreshPieceGeometry();

    /** @brief Re-read the whole file when a tool of another piece is built on what the active block holds.
     *  Armed by the cheap per-move parse of a base-point drag, and only does anything when the dependency
     *  graph finds such a tool. */
    void RefreshCrossBlockDependents();

protected:
    void customEvent(QEvent *event) override;

private slots:
    void CollectGarbage();

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(VPattern) // NOLINT

    /** @brief data container with data. */
    VContainer *data;

    VMainGraphicsScene *sceneDraw;
    VMainGraphicsScene *sceneDetail;
    QList<vidtype> updatePieces{};

    /** @brief A transfer whose inputs were not in the container yet when its block was read: the piece it
     *  copies from is built LATER in the file. Built at the end of the first pass instead, when every block
     *  has been read, and the block it belongs to is put back as the active one — the record and the graph
     *  node both take their block from the mapper. */
    struct DeferredTransfert
    {
        QDomElement element;
        quint32 id{NULL_ID};
        Document parse{Document::FullParse};
        int blockIndex{-1};
    };

    /** @brief The transfers kept aside by ParseToolTransfert. Emptied at the start of every parse and
     *  replayed at the end of its first pass. */
    QVector<DeferredTransfert> m_deferredTransfers{};

    /** @brief Debounce timer that coalesces piece geometry refreshes after a burst of parses
     * (file open, interactive tool edits, measurement sync). A single restartable timer replaces
     * the previous per-parse QTimer::singleShot so rapid parses collapse into one refresh. */
    QTimer *m_refreshGeometryTimer;

    /** @brief Debounce timer for the whole-file re-read a base-point drag owes the other pieces. The drag
     * itself re-reads the active block only, on every mouse move; this one waits for the gesture to settle
     * before catching the rest up, because that re-read costs ~180 ms on a five-piece pattern. */
    QTimer *m_crossBlockParseTimer;

    bool m_pieceGeometryDirty{true};

    bool m_garbageCollected{false};
    QString m_garbageCollectBackupFilePath{};

    static auto ParseDetailNode(const QDomElement &domElement) -> VNodeDetail;

    void ParseRootElement(const Document &parse, const QDomNode &node);
    /** @brief Read every top-level node of the document once, in file order. Called twice when the first,
     *  tolerant pass met a formula naming a variable a later block defines. */
    void ParseRoot(const Document &parse);
    void ParseDrawElement(const QDomNode &node, const Document &parse);
    void ParseDrawMode(const QDomNode &node, const Document &parse, const Draw &mode);
    void ParseDrawModeElement(QDomElement &domElement, const Document &parse, const Draw &mode);
    void ParseDetailElement(QDomElement &domElement, const Document &parse);
    void ParseDetailInternals(const QDomElement &domElement, VPiece &detail) const;
    auto ParseDetailNodes(const QDomElement &domElement, qreal width, bool closed) const -> QVector<VPieceNode>;
    auto ParsePieceGrainline(const QDomElement &domElement, VGrainlineData gGeometry) const -> VGrainlineData;
    auto ParsePieceCartoucheBlocks(const QDomElement &domElement) const -> OInfoBlockData;
    auto ParsePieceAnnotations(const QDomElement &domElement) const -> OAnnotationData;
    auto ParsePieceThumbnail(const QDomElement &domElement) const -> OThumbnailData;
    void ParseDetails(const QDomElement &domElement, const Document &parse);
    void ParsePieceGroups(const QDomElement &domElement);
    void ParseSizeRanges(const QDomElement &domElement);
    void ParsePointElement(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse,
                           const QString &type);
    void ParseLineElement(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse);
    void ParseSplineElement(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse,
                            const QString &type);
    void ParseArcElement(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse,
                         const QString &type);
    void ParseEllipticalArcElement(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse,
                                   const QString &type);
    void ParseToolsElement(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse,
                           const QString &type);
    void ParseOperationElement(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse,
                               const QString &type);
    void ParsePathElement(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseIncrementsElement(const QDomNode &node, const Document &parse);
    void ParseLineNamesElement(const QDomElement &domElement);

    void PrepareForParse(const Document &parse);
    void ToolsCommonAttributes(const QDomElement &domElement, quint32 &id);
    void DrawToolsCommonAttributes(const QDomElement &domElement, quint32 &id, QString &notes);
    void PointsWithLineCommonAttributes(const QDomElement &domElement, VToolLinePointInitData &initData);
    void PointsCommonAttributes(const QDomElement &domElement, VToolSinglePointInitData &initData);
    void PointsCommonAttributes(const QDomElement &domElement, quint32 &id, qreal &mx, qreal &my);
    void DrawPointsCommonAttributes(const QDomElement &domElement, quint32 &id, qreal &mx, qreal &my, QString &notes);
    void SplinesCommonAttributes(const QDomElement &domElement, quint32 &id, quint32 &idObject, quint32 &idTool);
    template <typename T> auto ToolBoundingRect(const QRectF &rec, quint32 id) const -> QRectF;
    void ParseCurrentPP();
    auto GetLabelBase(quint32 index) const -> QString;

    void ParseToolBasePoint(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse);
    void ParseToolEndLine(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolAlongLine(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolShoulderPoint(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolNormal(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolBisector(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolLineIntersect(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse);
    void ParseToolPointOfContact(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseNodePoint(const QDomElement &domElement, const Document &parse);
    void ParsePlaceLabel(QDomElement &domElement, const Document &parse);
    void ParseToolHeight(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse);
    void ParseToolTriangle(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse);
    void ParseToolPointOfIntersection(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse);
    void ParseToolCutSpline(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolCutSplinePath(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolCutArc(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolLineIntersectAxis(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolDartCap(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolRectangle(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolCurveIntersectAxis(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolPointOfIntersectionArcs(VMainGraphicsScene *scene, const QDomElement &domElement,
                                          const Document &parse);
    void ParseToolPointOfIntersectionCircles(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolPointOfIntersectionCurves(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolPointFromCircleAndTangent(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolPointFromArcAndTangent(VMainGraphicsScene *scene, const QDomElement &domElement,
                                         const Document &parse);
    void ParseToolArcPoint(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse, bool isStart);
    void ParseToolCurvePoint(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse, bool isStart);
    void ParseToolTrueDarts(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse);

    // TODO. Delete if minimal supported version is 0.2.7
    Q_STATIC_ASSERT_X(VPatternConverter::PatternMinVer < FormatVersion(0, 2, 7), "Time to refactor the code.");
    void ParseOldToolSpline(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);

    void ParseToolSpline(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolCubicBezier(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse);

    // TODO. Delete if minimal supported version is 0.2.7
    Q_STATIC_ASSERT_X(VPatternConverter::PatternMinVer < FormatVersion(0, 2, 7), "Time to refactor the code.");
    void ParseOldToolSplinePath(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);

    void ParseToolSplinePath(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse);
    void ParseToolParallelCurve(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolGraduatedCurve(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolArmholeCurve(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse);
    void ParseToolSpiral(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);

    auto ArrivalPointName(quint32 spiralId) const -> QString;
    void ParseToolCubicBezierPath(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse);
    void ParseNodeSpline(const QDomElement &domElement, const Document &parse);
    void ParseNodeSplinePath(const QDomElement &domElement, const Document &parse);

    void ParseToolArc(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseNodeArc(const QDomElement &domElement, const Document &parse);
    void ParseToolArcWithLength(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);

    void ParseToolEllipticalArc(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseNodeEllipticalArc(const QDomElement &domElement, const Document &parse);
    void ParseToolEllipticalArcWithLength(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);

    void ParseToolRotation(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolFlippingByLine(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolFlippingByAxis(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolMove(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolCloseDart(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolTransfert(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolSpread(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolMoveBatch(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolPleat(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolFlounceSpiral(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);
    void ParseToolDartCapOperation(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse);

    /** @brief Builds the transfers kept aside by ParseToolTransfert, and reports the ones that still cannot
     *  be built once every block has been read. */
    void ReplayDeferredTransfers();

    /** @brief Whether the graph already holds a vertex for every object a transfer reads. The graph is rebuilt
     *  in file order on every parse but a values-only one, so a transfer whose sources are in a LATER block
     *  would have its edges dropped in silence — AddEdge wants both ends. */
    auto TransfertSourcesAreGraphVertices(const OToolTransfertInitData &initData) -> bool;

    auto EvalFormula(VContainer *data, const QString &formula, bool *ok) const -> qreal;

    /** @brief Adopts an anchor for every piece child placed before the anchor existed.
     *
     * Pieces only — see the definition for why a group cannot be done here. */
    void MaterializeChildAnchors();

    /** @brief Mirrors the children's resolved positions and anchors into the DOM before saving.
     *
     * The save path does not go through RefreshDataInFile, so a position that moved only in the
     * container — which is every position that followed a change of size — would otherwise be
     * written to the file as it stood at the previous save. */
    void SyncChildGeometryToDom();

    /** @brief Rewrites <sizeRanges> from the container before saving.
     *
     * **The container is the live answer, the DOM only the last written one.** Dissolving a group
     * or deleting a piece moves a range's content (a group's offset is handed to its members, a
     * removed piece leaves the ranges), and those moves touch the container only — nothing calls
     * the writer. Saved without this, the file would keep the stale element and the graded layout
     * would drift back on reopening. Same reason, same place as @ref SyncChildGeometryToDom. */
    void SyncSizeRangesToDom();

    auto MakeEmptyIncrement(const QString &name, IncrementType type) -> QDomElement;

    void NewEmptyIncrement(const QString &type, const QString &name, IncrementType varType);
    void NewEmptyIncrementAfter(const QString &type, const QString &after, const QString &name, IncrementType varType);
    void RemoveIncrement(const QString &type, const QString &name);
    void MoveUpIncrement(const QString &type, const QString &name);
    void MoveDownIncrement(const QString &type, const QString &name);

    /** @brief Writes @p attr on the row @p name **of the table @p type**.
     *
     * `type` is `TagIncrements` or `TagPreviewCalculations`: the same name can live in both tables, and a row
     * belongs to the one the caller is editing. A search of the whole document answers the first in document
     * order — `<increments>` always comes first — and would write in the wrong table. */
    void SetIncrementAttribute(const QString &type, const QString &name, const QString &attr, const QString &text);

    void SetIncrementDescription(const QString &type, const QString &name, const QString &text);
    void SetIncrementSpecialUnits(const QString &type, const QString &name, bool special);

    auto LastDrawName() const -> QString;
    auto LastToolId() const -> quint32;
    auto PPLastToolId(int blockIndex) const -> quint32;

    auto MaxMentionedId() const -> quint32;

    auto HighestId() const -> quint32;
    void SyncHighestIdToDom();

    void GarbageCollector();
    void BackupBeforeGarbageCollector() const;

    auto GCStaleModelingObjects(const VPatternGraph *graph) -> QSet<vidtype>;
    auto GCStaleUnionTools(const VPatternGraph *graph) -> QSet<vidtype>;
    auto GCRemoveFromModeling(const QSet<vidtype> &candidates) -> QSet<vidtype>;
    void GCUpdateHistory(const QSet<vidtype> &removed);
};

#endif // VPATTERN_H
