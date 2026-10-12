/************************************************************************
 **
 **  @file   vabstractpattern.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   15 6, 2015
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2015 Valentina project
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
 // Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 *************************************************************************/

#ifndef VABSTRACTPATTERN_H
#define VABSTRACTPATTERN_H

#include <QHash>
#include <QMap>
#include <QMetaObject>
#include <QMutex>
#include <QObject>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QUuid>
#include <QVector>
#include <QtGlobal>

#include <atomic>
#include <limits>

#include "../vmisc/def.h"
#include "vcoverpagedata.h"
#include "vdomdocument.h"
#include "vtoolrecord.h"

class QDomElement;
template<typename T>
class QFutureWatcher;
class VPiecePath;
class VPieceNode;
class VPatternImage;
class VBackgroundPatternImage;
class VPatternBlockMapper;

enum class Document : qint8
{
    FullLiteParse,
    LiteParse,
    LitePPParse,
    FullParse
};
enum class LabelType : qint8
{
    NewPatternPiece,
    NewLabel
};

// Don't touch values!!!. Same values stored in xml.
enum class CrossCirclesPoint : qint8
{
    FirstPoint = 1,
    SecondPoint = 2
};
enum class VCrossCurvesPoint : qint8
{
    HighestPoint = 1,
    LowestPoint = 2
};
enum class HCrossCurvesPoint : qint8
{
    LeftmostPoint = 1,
    RightmostPoint = 2
};
enum class AxisType : qint8
{
    VerticalAxis = 1,
    HorizontalAxis = 2
};

class VContainer;
class VDataTool;
class VPatternGraph;

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Weffc++")

struct VFormulaField
{
    QString expression{};  // NOLINT(misc-non-private-member-variables-in-classes)
    QDomElement element{}; // NOLINT(misc-non-private-member-variables-in-classes)
    QString attribute{};   // NOLINT(misc-non-private-member-variables-in-classes)
};

struct VFinalMeasurement
{
    QString name{};
    QString formula{};
    QString description{};
};

struct VGroupData
{
    QString name{};
    bool visible{true};
    vidtype tool{NULL_ID};
    QVector<QPair<vidtype, vidtype>> items{};
    /// The layer's own pen. It is what the objects created inside it are offered as a default — the layer never
    /// repaints what is already drawn, and a user's change in the tool dialog wins over it.
    QString color{ColorBlack};
    QString penStyle{TypeLineLine};
    /// The layer the canvas draws into. Exactly one per pattern piece (see NormalizeActiveLayer).
    bool active{false};
    /// The layer the application itself maintains for the objects no layer references (see AttrAuto). Its name is
    /// translated and may be changed by the user, which is precisely why the role is carried by this flag.
    bool automatic{false};
    /// The group of layers this layer is gathered under, NULL_ID when it belongs to none. One layer, one group:
    /// the attribute holds a single id, and the dock reads the whole hierarchy from it.
    quint32 layerGroup{NULL_ID};
};

/// A group of layers: a name, and the layers that point at it. The group owns nothing else — its visibility is
/// the state of its layers, and an empty group is still a group, which is what lets it be deleted.
struct VLayerGroupData
{
    QString name{};
    /// Whether the dock shows the group folded. The reader's own state, kept with the document.
    bool collapsed{false};
};

QT_WARNING_POP

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wsuggest-final-types")

class VAbstractPattern : public VDomDocument
{
    Q_OBJECT // NOLINT

public:
    explicit VAbstractPattern(QObject *parent = nullptr);
    ~VAbstractPattern() override;

    auto RequiresMeasurements() const -> bool;
    auto ListMeasurements() const -> QStringList;
    auto ListExpressions() const -> QVector<VFormulaField>;
    auto ListIncrementExpressions() const -> QVector<VFormulaField>;

    virtual void CreateEmptyFile() = 0;

    auto CountPatternBlockTags() const -> int;

    auto GetActivNodeElement(const QString &name, QDomElement &element) const -> bool;

    void setXMLContent(const QString &fileName) override;

    virtual auto GenerateLabel(const LabelType &type, const QString &reservedName = QString()) const -> QString = 0;

    virtual void UpdateToolData(const quint32 &id, VContainer *data) = 0;

    void Clear() override;

    static auto getTool(quint32 id) -> VDataTool *;

    /** @brief Tool from the table, or nullptr when the id is unknown.
     *  Unlike getTool(), which throws VExceptionBadId on an unknown id, this one can be used as a plain
     *  predicate - for instance to know whether a tool has already been created. */
    static auto FindTool(quint32 id) -> VDataTool *;

    static void AddTool(quint32 id, VDataTool *tool);
    static void RemoveTool(quint32 id);

    static auto ParsePieceNodes(const QDomElement &domElement) -> VPiecePath;
    static auto ParsePieceCSARecords(const QDomElement &domElement) -> QVector<CustomSARecord>;
    static auto ParsePieceInternalPaths(const QDomElement &domElement) -> QVector<quint32>;
    static auto ParsePiecePointRecords(const QDomElement &domElement) -> QVector<quint32>;

    void AddToolOnRemove(VDataTool *tool);

    auto getHistory() -> QVector<VToolRecord> *;
    auto getHistory() const -> const QVector<VToolRecord> *;
    auto GetLocalHistory(int blockIndex = -1) const -> QVector<VToolRecord>;

    auto MPath() const -> QString;
    void SetMPath(const QString &path);

    auto SiblingNodeId(const quint32 &nodeId) const -> quint32;

    auto GetDescription() const -> QString;
    void SetDescription(const QString &text);

    auto GetNotes() const -> QString;
    void SetNotes(const QString &text);

    auto GetPatternName() const -> QString;
    void SetPatternName(const QString &name);

    auto GetCustomer() const -> QString;
    void SetCustomer(const QString &text);

    auto GetCustomerBirthDate() const -> QString;
    void SetCustomerBirthDate(const QString &text);

    auto GetCustomerEmail() const -> QString;
    void SetCustomerEmail(const QString &text);

    auto GetCollection() const -> QString;
    void SetCollection(const QString &text);

    auto GetFabricInfo() const -> QString;
    void SetFabricInfo(const QString &text);

    /** @brief Pattern-level seed "activate seam allowances" applied to pieces created
     *         afterwards. Defaults to true when the pattern attribute is absent. */
    auto GetDefaultActivateSA() const -> bool;
    void SetDefaultActivateSA(bool on);

    /** @brief Pattern-level seed "hide main path" applied to pieces created afterwards.
     *         Defaults to the application setting (IsHideMainPath) when the pattern
     *         attribute is absent. */
    auto GetDefaultHideMainPath() const -> bool;
    void SetDefaultHideMainPath(bool on);

    /** @brief Pattern-level seed "export the sew line with the drawings (DXF-AAMA)" applied
     *         to pieces created afterwards. Defaults to the application setting
     *         (GetSewLineOnDrawing) when the pattern attribute is absent. */
    auto GetDefaultSewLineOnDrawing() const -> bool;
    void SetDefaultSewLineOnDrawing(bool on);

    /** @brief Pattern-level seed "show the second passmark on the seam line" applied to pieces
     *         created afterwards. Defaults to the application setting (IsDoublePassmark) when the
     *         pattern attribute is absent. */
    auto GetDefaultSecondPassmarkOnSeamLine() const -> bool;
    void SetDefaultSecondPassmarkOnSeamLine(bool on);

    /** @brief Pattern-level reference to the information block (cartouche) template
     *         shown on every piece created afterwards. Absent or empty = no information
     *         block. The template is referenced by name and resolved at render. */
    auto GetCartoucheTemplate() const -> QString;
    void SetCartoucheTemplate(const QString &name);

    auto GetCompanyName() const -> QString;

    auto GetPatternNumber() const -> QString;

    auto SetWatermarkPath(const QString &path) -> bool;
    auto GetWatermarkPath() const -> QString;

    auto GetFinalMeasurements() const -> QVector<VFinalMeasurement>;
    void SetFinalMeasurements(const QVector<VFinalMeasurement> &measurements);

    /// The pattern's own contribution to its cover page. Setting it to nothing removes the block,
    /// so a pattern that has nothing to say does not carry an empty one for having been looked at.
    auto GetCoverPage() const -> VCoverPageData;
    void SetCoverPage(const VCoverPageData &data);

    // ── Where a root element goes ────────────────────────────────────────

    /**
     * @brief The children of `<pattern>`, in the order the schema's sequence gives them.
     *
     * **The one authority on placement.** The serialized document is validated against the schema
     * before every save, so an element out of this order is not a document that reads oddly — it is
     * a document that **cannot be written at all**. Writers used to keep their own idea of the
     * order: a partial list here, an `insertBefore(sizeRanges)` there, and an `appendChild` that
     * knew nothing. The one that knew nothing turned *adding a piece* into *this pattern can no
     * longer be saved*, and nobody could see why.
     */
    static auto RootElementOrder() -> const QStringList &;

    /**
     * @brief Inserts @p element among the children of the root, where the schema puts it.
     *
     * **After the nearest neighbour the document already holds** — the last one of its own tag when
     * it carries any, since a tag may repeat, and otherwise the closest predecessor in
     * @ref RootElementOrder. Everything between the two is absent, so that slot is this one; and as
     * `version` and `unit` are required and come first, the walk always finds somewhere to land.
     *
     * An element the schema does not name is appended: nothing can be said about where it belongs,
     * and refusing it would throw away something a reader may still understand.
     */
    void InsertRootElement(const QDomElement &element);

    void SetPatternWasChanged(bool changed);
    auto GetPatternWasChanged() const -> bool;

    auto GetPassmarkLengthVariable() const -> QString;
    void SetPassmarkLengthVariable(const QString &name);

    auto GetPassmarkWidthVariable() const -> QString;
    void SetPassmarkWidthVariable(const QString &name);

    auto GetImage() const -> VPatternImage;
    auto SetImage(const VPatternImage &image) -> bool;
    void DeleteImage();

    auto GetBackgroundImages() const -> QVector<VBackgroundPatternImage>;
    void SaveBackgroundImages(const QVector<VBackgroundPatternImage> &images);
    auto GetBackgroundImage(const QUuid &id) const -> VBackgroundPatternImage;
    void SaveBackgroundImage(const VBackgroundPatternImage &image);
    void DeleteBackgroundImage(const QUuid &id);

    auto GetVersion() const -> QString;
    void SetVersion();

    auto IsModified() const -> bool;
    void SetModified(bool modified);

    void ParseGroups(const QDomElement &domElement);
    auto CreateGroups(const QString &patternPieceName = QString()) -> QDomElement;
    auto CreateGroup(quint32 id,
                     const QString &name,
                     const QMap<quint32, quint32> &groupData,
                     vidtype tool = null_id,
                     const QString &color = ColorBlack,
                     const QString &penStyle = TypeLineLine,
                     quint32 layerGroup = NULL_ID) -> QDomElement;
    /// The layer the application maintains itself: the one holding the objects no layer references. Its NAME is
    /// translated ("Unnamed" / "Sans nom") and the user may change it, so what identifies it is the marker this
    /// writes (AttrAuto) — recognising it by its name made a second one appear in another language.
    auto CreateAutomaticGroup(quint32 id, const QString &name, const QMap<quint32, quint32> &groupData)
        -> QDomElement;
    auto GroupLinkedToTool(vidtype toolId) const -> vidtype;

    auto GetGroupName(quint32 id) -> QString;
    void SetGroupName(quint32 id, const QString &name);

    auto GetGroupColor(quint32 id) -> QString;
    auto GetGroupPenStyle(quint32 id) -> QString;
    void SetGroupPen(quint32 id, const QString &color, const QString &penStyle);

    auto GetDimensionAValue() -> double;
    void SetDimensionAValue(double value);

    auto GetDimensionBValue() -> double;
    void SetDimensionBValue(double value);

    auto GetDimensionCValue() -> double;
    void SetDimensionCValue(double value);

    auto GetGroups(const QString &patternPieceName = QString()) -> QMap<quint32, VGroupData>;
    auto GetGroupsContainingItem(quint32 toolId, quint32 objectId, bool containItem) -> QMap<quint32, QString>;
    auto AddItemToGroup(quint32 toolId, quint32 objectId, quint32 groupId) -> QDomElement;
    /// Add several (objectId, toolId) pairs in one go, then refresh the drawing once. Used when a tool is created:
    /// an operation can produce many objects and a per-object refresh would re-parse the layers once per object.
    auto AddItemsToGroup(const QVector<QPair<quint32, quint32>> &items, quint32 groupId) -> bool;
    auto RemoveItemFromGroup(quint32 toolId, quint32 objectId, quint32 groupId) -> QDomElement;
    auto GetGroupVisibility(quint32 id) -> bool;
    void SetGroupVisibility(quint32 id, bool visible);

    /** @brief Whether the layers show the object @a objectId.
     *
     *  The object belongs to no pattern block of its own: the layers that hold it are read from the document, in
     *  every block, and joined the way the layer pass joins them — shown as soon as one layer that holds it is
     *  visible, and shown when no layer holds it at all, which is what the pass decides too. An object id is the
     *  document's own counter, so a walk over every block answers for the one block the object lives in.
     *
     *  Read rather than remembered: a pass only ever describes the block it is given, and a tool refreshed after
     *  it by another block's pass would be answered with the wrong piece's layers — measured, a layer saved hidden
     *  came back drawn until it was toggled by hand. A tool whose tracés join objects that are not its own needs
     *  this answer (see VToolLinePoint::ApplySegmentsVisibility). */
    auto IsObjectVisible(quint32 objectId) const -> bool;

    /// The sentinel a "which group?" answer carries when the group named does not exist yet: the dialogs ask for a
    /// new group with it, and the code that writes it creates one. A real id never reaches it — the ids come from
    /// the document's own counter.
    static constexpr quint32 NewLayerGroupId = std::numeric_limits<quint32>::max();

    /// The groups of layers that can be joined, id -> name — what a dialog offers in a combo. Like the layers,
    /// they belong to a piece.
    auto LayerGroupNames(const QString &patternPieceName = QString()) -> QMap<quint32, QString>;
    auto GetLayerGroups(const QString &patternPieceName = QString()) -> QMap<quint32, VLayerGroupData>;
    /// The `<layerGroups>` element of one piece, created on the spot when the piece has none. Placed right after
    /// the layers, where the schema declares it.
    auto CreateLayerGroups(const QString &patternPieceName = QString()) -> QDomElement;
    auto CreateLayerGroup(quint32 id, const QString &name) -> QDomElement;
    /// Create a group AND put it in the piece in one gesture — what the operations need, where the layer is written
    /// into the DOM before any undo command exists. @p id comes from the document's own counter. NULL_ID when there
    /// is nothing to create.
    auto InsertLayerGroup(quint32 id, const QString &name) -> quint32;
    /// Take the group out of the document. The group's layers are expected to have left it already.
    void RemoveLayerGroup(quint32 id);

    auto GetLayerGroupName(quint32 id) -> QString;
    void SetLayerGroupName(quint32 id, const QString &name);

    auto GetLayerGroupCollapsed(quint32 id) -> bool;
    void SetLayerGroupCollapsed(quint32 id, bool collapsed);

    /// The group one layer belongs to, NULL_ID when it belongs to none — or when the group it names is not in the
    /// document any more, which a file edited by hand can hold. A layer is never in two groups: nothing to choose.
    auto GetLayerGroup(quint32 layerId) -> quint32;
    /// Put one layer in a group, or take it out of every group (NULL_ID). Written on the layer, never on the group.
    void SetLayerGroup(quint32 layerId, quint32 groupId);

    /// The layers of one group, in the order the document holds them.
    auto LayerGroupLayers(quint32 groupId) -> QVector<quint32>;

    /// The (objectId, toolId) pairs a layer can really hold for one tool element, read from the DOM.
    ///
    /// The pair is not decorative: GroupVisibility receives the object id and each tool decides from its own C++
    /// class what it hides. A single point, a line and a curve answer to their own id; a double point
    /// (trueDarts, rectangle) only answers to the two ids it keeps as `point1`/`point2` attributes — it has no
    /// element of its own for them; an operation hides the objects its `<destination>` declares, never its
    /// `<source>`. Pieces, unions and nodes answer to nothing at all and are deliberately left out, rather than
    /// parked in a layer that could never hide them.
    static auto LayerableObjectsOfTool(const QDomElement &toolElement, quint32 toolId)
        -> QVector<QPair<quint32, quint32>>;

    /// The same answer, found from the id alone. Needed for the one tool the document holds no element for: the
    /// line an operation produces. The operation keeps its id and the parse rebuilds it from the `<lines>` entry,
    /// so nothing in the file carries it — and it is still a line, named in a layer by (its id, its id) exactly
    /// like the line a Line tool draws. Empty when the document knows no tool of that id at all.
    auto LayerableObjectsOfTool(quint32 toolId) -> QVector<QPair<quint32, quint32>>;

    /// The pen of the active layer — what the objects drawn into it are offered as a default.
    auto GetActiveGroupColor() -> QString;
    auto GetActiveGroupPenStyle() -> QString;

    /// The layer the canvas draws into, for one pattern piece. NULL_ID when the piece has no layer at all — it is
    /// never NULL_ID once the piece owns at least one, because NormalizeActiveLayer picks one at load.
    auto GetActiveGroup(const QString &patternPieceName = QString()) -> quint32;
    /// Make one layer the active one: the attribute is set on it and removed from every other layer of the piece.
    void SetActiveGroup(quint32 id, const QString &patternPieceName = QString());

    auto PieceDrawName(quint32 id) -> QString;

    virtual auto GetCompleteData() const -> VContainer;
    virtual auto GetCompletePPData(const QString &name) const -> VContainer;

    auto Units() const -> Unit;

    static const QString TagPattern;
    static const QString TagCalculation;
    static const QString TagModeling;
    static const QString TagDetails;
    static const QString TagDetail;
    static const QString TagDescription;
    static const QString TagImage;
    static const QString TagNotes;
    static const QString TagMeasurements;
    static const QString TagIncrements;
    /// The groups of the table of increments: one list per table, and each row points at its group.
    static const QString TagIncrementGroups;
    static const QString TagLineNames;
    static const QString TagPreviewCalculations;
    static const QString TagPreviewCalculationGroups;
    static const QString TagIncrement;
    static const QString TagDraw;
    static const QString TagGroups;
    static const QString TagGroup;
    static const QString TagGroupItem;
    static const QString TagLayerGroups;
    static const QString TagLayerGroup;
    static const QString TagPieceGroups;
    static const QString TagPieceGroup;
    static const QString TagMember;
    static const QString TagPoint;
    static const QString TagSpline;
    static const QString TagArc;
    static const QString TagElArc;
    static const QString TagTools;
    static const QString TagOperation;
    static const QString TagPatternName;
    static const QString TagPatternNum;
    static const QString TagCompanyName;
    static const QString TagCustomerName;
    static const QString TagCustomerBirthDate;
    static const QString TagCustomerEmail;
    static const QString TagCollection;
    static const QString TagFabricInfo;
    static const QString TagCartouche;
    static const QString TagWatermark;
    static const QString TagFinalMeasurements;
    static const QString TagMaterial;
    static const QString TagFMeasurement;
    static const QString TagGrainline;
    static const QString TagPath;
    static const QString TagNodes;
    static const QString TagNode;
    static const QString TagBackgroundImages;
    static const QString TagBackgroundImage;
    static const QString TagOffset;
    static const QString TagMirrorLine;
    /// What the cover page reads from the pattern rather than from its own template. Ostara's own
    /// root element, and the only one the Export mode writes.
    static const QString TagCoverPage;

    /// The size ranges of the Graded mode — Ostara's own, like the two above.
    static const QString TagSizeRanges;

    /// The materials a pattern is made of. A root element of the schema that had no constant.
    static const QString TagPatternMaterials;

    static const QString AttrName;
    static const QString AttrEnabled;
    static const QString AttrVisible;
    static const QString AttrObject;
    static const QString AttrTool;
    static const QString AttrType;
    static const QString AttrArrows;
    static const QString AttrNodeReverse;
    static const QString AttrNodeExcluded;
    static const QString AttrNodePassmark;
    static const QString AttrNodePassmarkLine;
    static const QString AttrNodePassmarkAngle;
    static const QString AttrNodeShowSecondPassmark;
    static const QString AttrNodePassmarkOpening;
    static const QString AttrNodePassmarkNotMirrored;
    static const QString AttrNodeTurnPoint;
    static const QString AttrSABefore;
    static const QString AttrSAAfter;
    static const QString AttrStart;
    static const QString AttrPath;
    static const QString AttrEnd;
    static const QString AttrIncludeAs;
    static const QString AttrRotation;
    static const QString AttrNumber;
    static const QString AttrCheckUniqueness;
    static const QString AttrManualPassmarkLength;
    static const QString AttrPassmarkLength;
    static const QString AttrDefaultActivateSA;
    static const QString AttrDefaultHideMainPath;
    static const QString AttrDefaultSewLineOnDrawing;
    static const QString AttrDefaultSecondPassmarkOnSeamLine;
    static const QString AttrTemplate;
    static const QString AttrManualPassmarkWidth;
    static const QString AttrPassmarkWidth;
    static const QString AttrManualPassmarkAngle;
    static const QString AttrPassmarkAngle;
    static const QString AttrPassmarkVisibility;
    static const QString AttrOpacity;
    /// The group of layers a `<group>` belongs to, written on the layer: one id, at most one group.
    static const QString AttrLayerGroup;
    /// Whether the dock folds a `<layerGroup>`: the reader's own state, kept with the document.
    static const QString AttrCollapsed;
    /// The group an `<increment>` is filed under, written on the row: one id, at most one group — the
    /// same shape as the layer's `layerGroup`. Only importing a piece writes one.
    static const QString AttrGroupId;
    /// Marks the layer the canvas draws into. Absent means "not active", so only the active one carries it —
    /// which also makes it self-healing: deleting the layer deletes its flag with it.
    static const QString AttrActive;
    /// Marks the layer the application keeps for the objects no layer references — the one the dock shows as
    /// "Unnamed" ("Sans nom"). An IDENTITY, never a name: the name is translated and the user may rename the
    /// layer, so recognising it by that would make a second one appear in another language or after a rename.
    static const QString AttrAuto;
    static const QString AttrTransform;
    static const QString AttrHold;
    static const QString AttrZValue;
    static const QString AttrImageId;
    static const QString AttrDimensionA;
    static const QString AttrDimensionB;
    static const QString AttrDimensionC;
    static const QString AttrMirrorLineP1;
    static const QString AttrMirrorLineP2;
    static const QString AttrMirrorLineVisible;
    static const QString AttrFoldLineHeightFormula;
    static const QString AttrFoldLineWidthFormula;
    static const QString AttrFoldLineCenterFormula;
    static const QString AttrFoldLineManualHeight;
    static const QString AttrFoldLineManualWidth;
    static const QString AttrFoldLineManualCenter;
    static const QString AttrFoldLineType;
    static const QString AttrFoldLineFontSize;
    static const QString AttrFoldLineLabel;

    static const QString AttrContentType;

    static const QString AttrFormula;
    static const QString AttrSpecialUnits;

    static const QString NodeArc;
    static const QString NodeElArc;
    static const QString NodePoint;
    static const QString NodeSpline;
    static const QString NodeSplinePath;

    auto PatternGraph() const -> VPatternGraph *;

    void FindFormulaDependencies(const QString &formula, quint32 id, const QHash<QString, QList<quint32>> &variables);

    auto IsPatternGraphComplete() const -> bool;

    auto PatternBlockMapper() const -> VPatternBlockMapper *;

signals:
    /**
     * @brief FullUpdateFromFile update tool data form file.
     */
    void FullUpdateFromFile();
    /**
     * @brief patternChanged emit if we have unsaved change.
     */
    void patternChanged(bool saved);
    void UpdatePatternLabel();
    void ClearMainWindow();
    void UndoCommand();
    void SetEnabledGUI(bool enabled);
    void CheckLayout();
    void UpdateInLayoutList();
    void ShowDetail(quint32 id);
    void ShowPatternBlock(const QString &patterBlockName);
    void MadeProgress();
    /**
     * @brief UpdateGroups emit if the groups have been updated
     */
    void UpdateGroups();
    void UpdateToolTip();

    void BackgroundImageTransformationChanged(QUuid id);
    void BackgroundImagesHoldChanged();
    void BackgroundImageHoldChanged(const QUuid &id);
    void BackgroundImageVisibilityChanged(const QUuid &id);
    void BackgroundImagesVisibilityChanged();
    void BackgroundImageNameChanged(const QUuid &id);
    void BackgroundImagesZValueChanged();
    void BackgroundImagePositionChanged(const QUuid &id);
    void BackgroundImageOpacityChanged(const QUuid &id);

    void CancelLabelRendering();

    void PatternDependencyGraphCompleted();

public slots:
    virtual void LiteParseTree(const Document &parse) = 0;
    void haveLiteChange();
    void NeedFullParsing();
    void ClearScene();
    void CheckInLayoutList();
    void SelectedDetail(quint32 id);
    void UpdateVisiblityGroups();

protected slots:
    void CancelFormulaDependencyChecks();

protected:
    QVector<VDataTool *> toolsOnRemove;

    /** @brief history history records. */
    QVector<VToolRecord> history;

    /** @brief modified keep state of the document for cases that do not cover QUndoStack*/
    mutable bool modified;

    Unit m_units{Unit::LAST_UNIT_DO_NOT_USE};
    QString m_patternNumber{};
    QString m_patternName{};
    QString m_MPath{};
    QString m_watermarkPath{};
    QString m_companyName{};

    bool m_fileParsingCompleted{true};

    /** @brief tools list with pointer on tools. */
    static QHash<quint32, VDataTool *> tools;
    static bool patternLabelWasChanged;

    static void ToolExists(const quint32 &id);
    static auto ParsePathNodes(const QDomElement &domElement) -> VPiecePath;
    static auto ParseSANode(const QDomElement &domElement) -> VPieceNode;

    auto CheckTagExists(const QString &tag) -> QDomElement;

    void SetChildTag(const QString &qsParent, const QString &qsChild, const QString &qsValue);

    auto GroupHasItem(const QDomElement &groupDomElement, quint32 toolId, quint32 objectId) -> bool;

    auto SaveSchema() const -> QString override;

    auto ReadUnits() const -> Unit;
    auto ReadPatternNumber() const -> QString;
    auto ReadPatternName() const -> QString;
    auto ReadMPath() const -> QString;
    auto ReadWatermarkPath() const -> QString;
    auto ReadCompanyName() const -> QString;

private slots:
    void CleanDependenciesWatcher();

private:
    Q_DISABLE_COPY_MOVE(VAbstractPattern) // NOLINT

    VPatternGraph *m_patternGraph;

    VPatternBlockMapper *m_patternBlockMapper;

    QList<QFutureWatcher<void> *> m_formulaDependenciesWatchers{};
    mutable QMutex m_watchersMutex{};
    std::atomic<quint64> m_dependencyCheckGeneration{0};

    void ProcessFormulaDependencies(const QString &formula, quint32 id,
                                     const QHash<QString, QList<quint32>> &variables, quint64 generation);

    auto ListIncrements() const -> QStringList;
    auto ListPointExpressions() const -> QVector<VFormulaField>;
    auto ListArcExpressions() const -> QVector<VFormulaField>;
    auto ListElArcExpressions() const -> QVector<VFormulaField>;
    auto ListSplineExpressions() const -> QVector<VFormulaField>;
    auto ListOperationExpressions() const -> QVector<VFormulaField>;
    auto ListNodesExpressions(const QDomElement &nodes) const -> QVector<VFormulaField>;
    auto ListPathExpressions() const -> QVector<VFormulaField>;
    auto ListGrainlineExpressions(const QDomElement &element) const -> QVector<VFormulaField>;
    auto ListPieceExpressions() const -> QVector<VFormulaField>;
    auto ListFinalMeasurementsExpressions() const -> QVector<VFormulaField>;

    static auto IsVariable(const QString &token) -> bool;
    static auto IsFunction(const QString &token) -> bool;

    auto ParseItemElement(const QDomElement &domElement) -> QPair<bool, QMap<quint32, quint32>>;

    auto GetFMeasurements(const QDomElement &element) const -> QVector<VFinalMeasurement>;
    void SetFMeasurements(QDomElement &element, const QVector<VFinalMeasurement> &measurements);

    auto GetBackgroundPatternImage(const QDomElement &element) const -> VBackgroundPatternImage;
    auto GetBackgroundImageElement(const QUuid &id) const -> QDomElement;
    void WriteBackgroundImage(QDomElement &element, const VBackgroundPatternImage &image);

    auto GetPatternVariable(const QString &attribute) const -> QString;
    void SePatternVariable(const QString &attribute, const QString &name);
};

QT_WARNING_POP

#endif // VABSTRACTPATTERN_H
