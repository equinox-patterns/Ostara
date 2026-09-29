/************************************************************************
 **
 **  @file   vabstractpattern.cpp
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

#include "vabstractpattern.h"

#include <QDomNode>
#include <QDomNodeList>
#include <QFuture>
#include <QFutureWatcher>
#include <QLatin1String>
#include <QList>
#include <QMessageLogger>
#include <QMimeDatabase>
#include <QScopeGuard>
#include <QSet>
#include <QtConcurrentMap>
#include <QtConcurrentRun>
#include <QtDebug>

#include "../exception/vexceptionbadid.h"
#include "../exception/vexceptionconversionerror.h"
#include "../exception/vexceptionemptyparameter.h"
#include "../exception/vexceptionobjecterror.h"
#include "../ifcdef.h"
#include "../qmuparser/qmutokenparser.h"
#include "../vmisc/compatibility.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vtools/tools/vdatatool.h"
#include "def.h"
#include "typedef.h"
#include "vbackgroundpatternimage.h"
#include "vdomdocument.h"
#include "vpatternblockmapper.h"
#include "vpatternconverter.h"
#include "vpatterngraph.h"
#include "vpatternimage.h"
#include "vtoolrecord.h"
#include "vvalentinasettings.h"

using namespace Qt::Literals::StringLiterals;

class QDomElement;

const QString VAbstractPattern::TagPattern = QStringLiteral("pattern");
const QString VAbstractPattern::TagCalculation = QStringLiteral("calculation");
const QString VAbstractPattern::TagModeling = QStringLiteral("modeling");
const QString VAbstractPattern::TagDetails = QStringLiteral("details");
const QString VAbstractPattern::TagDetail = QStringLiteral("detail");
const QString VAbstractPattern::TagDescription = QStringLiteral("description");
const QString VAbstractPattern::TagNotes = QStringLiteral("notes");
const QString VAbstractPattern::TagImage = QStringLiteral("image");
const QString VAbstractPattern::TagMeasurements = QStringLiteral("measurements");
const QString VAbstractPattern::TagIncrements = QStringLiteral("increments");
const QString VAbstractPattern::TagLineNames = QStringLiteral("lineNames");
const QString VAbstractPattern::TagPreviewCalculations = QStringLiteral("previewCalculations");
const QString VAbstractPattern::TagIncrement = QStringLiteral("increment");
const QString VAbstractPattern::TagDraw = QStringLiteral("draw");
const QString VAbstractPattern::TagGroups = QStringLiteral("groups");
const QString VAbstractPattern::TagGroup = QStringLiteral("group");
const QString VAbstractPattern::TagGroupItem = QStringLiteral("item");
const QString VAbstractPattern::TagPieceGroups = QStringLiteral("pieceGroups");
const QString VAbstractPattern::TagPieceGroup = QStringLiteral("pieceGroup");
const QString VAbstractPattern::TagMember = QStringLiteral("member");
const QString VAbstractPattern::TagAttachments = QStringLiteral("attachments");
const QString VAbstractPattern::TagAttachment = QStringLiteral("attachment");
const QString VAbstractPattern::TagAttachmentItem = QStringLiteral("item");
const QString VAbstractPattern::TagPoint = QStringLiteral("point");
const QString VAbstractPattern::TagSpline = QStringLiteral("spline");
const QString VAbstractPattern::TagArc = QStringLiteral("arc");
const QString VAbstractPattern::TagElArc = QStringLiteral("elArc");
const QString VAbstractPattern::TagTools = QStringLiteral("tools");
const QString VAbstractPattern::TagOperation = QStringLiteral("operation");
const QString VAbstractPattern::TagPatternName = QStringLiteral("patternName");
const QString VAbstractPattern::TagPatternNum = QStringLiteral("patternNumber");
const QString VAbstractPattern::TagCustomerName = QStringLiteral("customer");
const QString VAbstractPattern::TagCustomerBirthDate = QStringLiteral("birthDate");
const QString VAbstractPattern::TagCustomerEmail = QStringLiteral("email");
const QString VAbstractPattern::TagCollection = QStringLiteral("collection");
const QString VAbstractPattern::TagFabricInfo = QStringLiteral("fabricInfo");
const QString VAbstractPattern::TagCartouche = QStringLiteral("cartouche");
const QString VAbstractPattern::TagCompanyName = QStringLiteral("company");
const QString VAbstractPattern::TagWatermark = QStringLiteral("watermark");
const QString VAbstractPattern::TagFinalMeasurements = QStringLiteral("finalMeasurements");
const QString VAbstractPattern::TagMaterial = QStringLiteral("material");
const QString VAbstractPattern::TagFMeasurement = QStringLiteral("finalMeasurment");
const QString VAbstractPattern::TagGrainline = QStringLiteral("grainline");
const QString VAbstractPattern::TagPath = QStringLiteral("path");
const QString VAbstractPattern::TagNodes = QStringLiteral("nodes");
const QString VAbstractPattern::TagNode = QStringLiteral("node");
const QString VAbstractPattern::TagBackgroundImages = QStringLiteral("backgroudImages");
const QString VAbstractPattern::TagBackgroundImage = QStringLiteral("backgroudImage");
const QString VAbstractPattern::TagOffset = QStringLiteral("offset");
const QString VAbstractPattern::TagMirrorLine = QStringLiteral("mirrorLine"); // NOLINT(cert-err58-cpp)
const QString VAbstractPattern::TagCoverPage = QStringLiteral("coverPage");  // NOLINT(cert-err58-cpp)
const QString VAbstractPattern::TagSizeRanges = QStringLiteral("sizeRanges"); // NOLINT(cert-err58-cpp)
const QString VAbstractPattern::TagPatternMaterials = QStringLiteral("patternMaterials"); // NOLINT(cert-err58-cpp)

const QString VAbstractPattern::AttrName = QStringLiteral("name");
const QString VAbstractPattern::AttrEnabled = QStringLiteral("enabled");
const QString VAbstractPattern::AttrVisible = QStringLiteral("visible");
const QString VAbstractPattern::AttrObject = QStringLiteral("object");
const QString VAbstractPattern::AttrTool = QStringLiteral("tool");
const QString VAbstractPattern::AttrType = QStringLiteral("type");
const QString VAbstractPattern::AttrArrows = QStringLiteral("arrows");
const QString VAbstractPattern::AttrNodeReverse = QStringLiteral("reverse");
const QString VAbstractPattern::AttrNodeExcluded = QStringLiteral("excluded");
const QString VAbstractPattern::AttrNodePassmark = QStringLiteral("passmark");
const QString VAbstractPattern::AttrNodePassmarkLine = QStringLiteral("passmarkLine");
const QString VAbstractPattern::AttrNodePassmarkAngle = QStringLiteral("passmarkAngle");
const QString VAbstractPattern::AttrNodeShowSecondPassmark = QStringLiteral("showSecondPassmark");
const QString VAbstractPattern::AttrNodePassmarkOpening = QStringLiteral("passmarkClockwiseOpening");
const QString VAbstractPattern::AttrNodePassmarkNotMirrored = QStringLiteral("passmarkNotMirrored");
const QString VAbstractPattern::AttrNodeTurnPoint = QStringLiteral("turnPoint");
const QString VAbstractPattern::AttrSABefore = QStringLiteral("before");
const QString VAbstractPattern::AttrSAAfter = QStringLiteral("after");
const QString VAbstractPattern::AttrStart = QStringLiteral("start");
const QString VAbstractPattern::AttrPath = QStringLiteral("path");
const QString VAbstractPattern::AttrEnd = QStringLiteral("end");
const QString VAbstractPattern::AttrIncludeAs = QStringLiteral("includeAs");
const QString VAbstractPattern::AttrRotation = QStringLiteral("rotation");
const QString VAbstractPattern::AttrNumber = QStringLiteral("number");
const QString VAbstractPattern::AttrCheckUniqueness = QStringLiteral("checkUniqueness");
const QString VAbstractPattern::AttrManualPassmarkLength = QStringLiteral("manualPassmarkLength");
const QString VAbstractPattern::AttrPassmarkLength = QStringLiteral("passmarkLength");
const QString VAbstractPattern::AttrManualPassmarkWidth = QStringLiteral("manualPassmarkWidth");
const QString VAbstractPattern::AttrPassmarkWidth = QStringLiteral("passmarkWidth");
const QString VAbstractPattern::AttrDefaultActivateSA = QStringLiteral("defaultActivateSA");
const QString VAbstractPattern::AttrDefaultHideMainPath = QStringLiteral("defaultHideMainPath");
const QString VAbstractPattern::AttrDefaultSewLineOnDrawing = QStringLiteral("defaultSewLineOnDrawing");
const QString VAbstractPattern::AttrDefaultSecondPassmarkOnSeamLine =
    QStringLiteral("defaultSecondPassmarkOnSeamLine");
const QString VAbstractPattern::AttrTemplate = QStringLiteral("template");
const QString VAbstractPattern::AttrManualPassmarkAngle = QStringLiteral("manualPassmarkAngle");
const QString VAbstractPattern::AttrPassmarkAngle = QStringLiteral("passmarkAngleFormula");
const QString VAbstractPattern::AttrPassmarkVisibility = QStringLiteral("passmarkVisibilityFormula");
const QString VAbstractPattern::AttrOpacity = QStringLiteral("opacity");
const QString VAbstractPattern::AttrTags = QStringLiteral("tags");
const QString VAbstractPattern::AttrActive = QStringLiteral("active");
const QString VAbstractPattern::AttrTransform = QStringLiteral("transform");
const QString VAbstractPattern::AttrHold = QStringLiteral("hold");
const QString VAbstractPattern::AttrZValue = QStringLiteral("zValue");
const QString VAbstractPattern::AttrImageId = QStringLiteral("imageId");
const QString VAbstractPattern::AttrDimensionA = QStringLiteral("dimensionA");
const QString VAbstractPattern::AttrDimensionB = QStringLiteral("dimensionB");
const QString VAbstractPattern::AttrDimensionC = QStringLiteral("dimensionC");
const QString VAbstractPattern::AttrMirrorLineP1 = QStringLiteral("p1");
const QString VAbstractPattern::AttrMirrorLineP2 = QStringLiteral("p2");
const QString VAbstractPattern::AttrMirrorLineVisible = QStringLiteral("visible");
const QString VAbstractPattern::AttrFoldLineHeightFormula = QStringLiteral("height");
const QString VAbstractPattern::AttrFoldLineWidthFormula = QStringLiteral("width");
const QString VAbstractPattern::AttrFoldLineCenterFormula = QStringLiteral("center");
const QString VAbstractPattern::AttrFoldLineManualHeight = QStringLiteral("manualHeight");
const QString VAbstractPattern::AttrFoldLineManualWidth = QStringLiteral("manualWidth");
const QString VAbstractPattern::AttrFoldLineManualCenter = QStringLiteral("manualCenter");
const QString VAbstractPattern::AttrFoldLineType = QStringLiteral("type");
const QString VAbstractPattern::AttrFoldLineFontSize = QStringLiteral("fontSize");
const QString VAbstractPattern::AttrFoldLineLabel = QStringLiteral("label");

const QString VAbstractPattern::AttrContentType = QStringLiteral("contentType");

const QString VAbstractPattern::AttrFormula = QStringLiteral("formula");
const QString VAbstractPattern::AttrSpecialUnits = QStringLiteral("specialUnits");

const QString VAbstractPattern::NodeArc = QStringLiteral("NodeArc");
const QString VAbstractPattern::NodeElArc = QStringLiteral("NodeElArc");
const QString VAbstractPattern::NodePoint = QStringLiteral("NodePoint");
const QString VAbstractPattern::NodeSpline = QStringLiteral("NodeSpline");
const QString VAbstractPattern::NodeSplinePath = QStringLiteral("NodeSplinePath");

QHash<quint32, VDataTool *> VAbstractPattern::tools = QHash<quint32, VDataTool *>();
bool VAbstractPattern::patternLabelWasChanged = false;

namespace
{
QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wunused-member-function")

Q_GLOBAL_STATIC_WITH_ARGS(const QString, dimensionDefValue, ("-1"_L1)) // NOLINT

QT_WARNING_POP

//---------------------------------------------------------------------------------------------------------------------
void ReadExpressionAttribute(QVector<VFormulaField> &expressions, const QDomElement &element, const QString &attribute)
{
    const QString expression = VDomDocument::GetParametrEmptyString(element, attribute);
    if (expression.isEmpty())
    {
        return;
    }

    expressions.append({.expression = expression, .element = element, .attribute = attribute});
}

//---------------------------------------------------------------------------------------------------------------------
auto GetTokens(const VFormulaField &formula) -> QList<QString>
{
    try
    {
        QScopedPointer<qmu::QmuTokenParser> const cal(new qmu::QmuTokenParser(formula.expression, false, false));
        return cal->GetTokens().values();
    }
    catch (const qmu::QmuParserError &e)
    {
        qWarning() << QObject::tr("Cannot get tokens from formula '%1'. Parser error: %2.")
                          .arg(formula.expression, e.GetMsg());
        return {};
    }
    catch (const qmu::QmuParserWarning &e)
    {
        qWarning() << QObject::tr("Cannot get tokens from formula '%1'. Formula error: %2.")
                          .arg(formula.expression, e.GetMsg());
        return {};
    }
}

//---------------------------------------------------------------------------------------------------------------------
void GatherTokens(QSet<QString> &tokens, const QList<QString> &tokenList)
{
    tokens = tokens.unite(ConvertToSet(tokenList));
}

//---------------------------------------------------------------------------------------------------------------------
auto PrepareGroupTags(QStringList tags) -> QString
{
    for (auto &tag : tags)
    {
        tag = tag.simplified();
    }

    return ConvertToStringList(ConvertToSet<QString>(tags)).join(','_L1);
}

//---------------------------------------------------------------------------------------------------------------------
auto StringToTransfrom(const QString &matrix) -> QTransform
{
    if (QStringList const elements = matrix.split(';'_L1); elements.count() == 9)
    {
        qreal const m11 = elements.at(0).toDouble();
        qreal const m12 = elements.at(1).toDouble();
        qreal const m13 = elements.at(2).toDouble();
        qreal const m21 = elements.at(3).toDouble();
        qreal const m22 = elements.at(4).toDouble();
        qreal const m23 = elements.at(5).toDouble();
        qreal const m31 = elements.at(6).toDouble();
        qreal const m32 = elements.at(7).toDouble();
        qreal const m33 = elements.at(8).toDouble();
        return {m11, m12, m13, m21, m22, m23, m31, m32, m33};
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
template<class T>
auto NumberToString(T number) -> QString
{
    const QLocale locale = QLocale::c();
    return locale.toString(number, 'g', 12).remove(LocaleGroupSeparator(locale));
}

//---------------------------------------------------------------------------------------------------------------------
auto TransformToString(const QTransform &m) -> QString
{
    QStringList const matrix{NumberToString(m.m11()),
                             NumberToString(m.m12()),
                             NumberToString(m.m13()),
                             NumberToString(m.m21()),
                             NumberToString(m.m22()),
                             NumberToString(m.m23()),
                             NumberToString(m.m31()),
                             NumberToString(m.m32()),
                             NumberToString(m.m33())};
    return matrix.join(';'_L1);
}

//---------------------------------------------------------------------------------------------------------------------
// The cover page's own vocabulary. Named in one place so that the writer and the reader cannot
// drift apart, the way the tag constants above are named for the rest of the document.
const QString kTagCoverFreeText = QStringLiteral("freeText");
const QString kTagCoverSymbol = QStringLiteral("symbol");
const QString kTagCoverMarginName = QStringLiteral("marginName");
const QString kTagCoverText = QStringLiteral("text");
const QString kAttrCoverLocale = QStringLiteral("locale");
// **Not "id".** Every attribute named "id" in a pattern file is read as a tool's number, by a walk
// that goes over the whole document before every save — so a symbol's uuid under that name is not
// a symbol the file cannot hold, it is a document that cannot be written at all. The entries that
// carry a name of their own rather than a number say so: imageId, pieceId, groupId.
const QString kAttrCoverId = QStringLiteral("symbolId");
const QString kAttrCoverWidth = QStringLiteral("width");

//---------------------------------------------------------------------------------------------------------------------
auto HasCoverText(const QMap<QString, QString> &texts) -> bool
{
    for (auto it = texts.constBegin(); it != texts.constEnd(); ++it)
    {
        if (!it.value().isEmpty())
        {
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
/// A block that says nothing is not written: three empty groups are not a thing the pattern holds.
auto IsCoverPageEmpty(const VCoverPageData &data) -> bool
{
    if (!data.freeText.isEmpty())
    {
        return false;
    }

    // A symbol the user has added a row for but not yet chosen says nothing, and is not what makes a
    // block worth writing: the dock holds such a row so it can be answered, and the file holds the
    // answers.
    for (const QString &id : data.symbols)
    {
        if (!id.isEmpty())
        {
            return false;
        }
    }

    for (const VCoverPageMarginName &margin : data.marginNames)
    {
        if (HasCoverText(margin.texts))
        {
            return false;
        }
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
/// Reads the <text locale="…"> children of @p parent into one name per locale.
auto ReadCoverTexts(const QDomElement &parent) -> QMap<QString, QString>
{
    QMap<QString, QString> texts;
    for (QDomElement text = parent.firstChildElement(kTagCoverText); !text.isNull();
         text = text.nextSiblingElement(kTagCoverText))
    {
        const QString locale = text.attribute(kAttrCoverLocale);
        if (!locale.isEmpty())
        {
            texts.insert(locale, text.text());
        }
    }
    return texts;
}

//---------------------------------------------------------------------------------------------------------------------
/// Writes @p texts as <text locale="…"> children, skipping the locales that say nothing.
void WriteCoverTexts(VAbstractPattern &doc, QDomElement &parent, const QMap<QString, QString> &texts)
{
    for (auto it = texts.constBegin(); it != texts.constEnd(); ++it)
    {
        if (it.value().isEmpty())
        {
            continue;
        }
        QDomElement text = doc.createElement(kTagCoverText);
        text.setAttribute(kAttrCoverLocale, it.key());
        text.appendChild(doc.createTextNode(it.value()));
        parent.appendChild(text);
    }
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
VAbstractPattern::VAbstractPattern(QObject *parent)
  : VDomDocument(parent),
    toolsOnRemove(QVector<VDataTool *>()),
    history(QVector<VToolRecord>()),
    modified(false),
    m_patternGraph(new VPatternGraph()),
    m_patternBlockMapper(new VPatternBlockMapper())
{
    connect(qApp, &QCoreApplication::aboutToQuit, this, &VAbstractPattern::CancelFormulaDependencyChecks);
}

//---------------------------------------------------------------------------------------------------------------------
VAbstractPattern::~VAbstractPattern()
{
    CancelFormulaDependencyChecks();

    // Must be deleted before the QDomDocument base destructs. VDomDocument declares QObject before QDomDocument, so
    // QDomDocument::~QDomDocument() runs before QObject::~QObject(). The QDomElement objects stored in
    // VPatternBlockMapper must be released while the DOM nodes are still alive to avoid a double-free.
    qCDebug(vXML) << "Destroying VAbstractPattern: releasing block mapper" << m_patternBlockMapper->Size() << "blocks.";
    delete m_patternBlockMapper;

    qDeleteAll(toolsOnRemove);
    toolsOnRemove.clear();
    delete m_patternGraph;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::RequiresMeasurements() const -> bool
{
    return not ListMeasurements().isEmpty();
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ListMeasurements() const -> QStringList
{
    const QFuture<QStringList> futureIncrements = QtConcurrent::run([this]() { return ListIncrements(); });
    const QList<QString> tokens = ConvertToList(
        QtConcurrent::blockingMappedReduced(ListExpressions(), GetTokens, GatherTokens));

    QSet<QString> measurements;
    QSet<QString> others = ConvertToSet<QString>(futureIncrements.result());

    for (const auto &token : tokens)
    {
        if (token == '-'_L1 || measurements.contains(token) || others.contains(token))
        {
            continue;
        }

        IsVariable(token) || IsFunction(token) ? others.insert(token) : measurements.insert(token);
    }

    return {measurements.values()};
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetActivNodeElement find element in current pattern piece by name.
 * @param name name tag.
 * @param element element.
 * @return true if found.
 */
auto VAbstractPattern::GetActivNodeElement(const QString &name, QDomElement &element) const -> bool
{
    Q_ASSERT_X(not name.isEmpty(), Q_FUNC_INFO, "name draw is empty");

    QDomElement const drawElement = m_patternBlockMapper->GetActiveElement();
    if (drawElement.isNull())
    {
        return false;
    }

    const QDomNodeList listElement = drawElement.elementsByTagName(name);
    if (listElement.size() != 1)
    {
        return false;
    }
    element = listElement.at(0).toElement();
    return !element.isNull();
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::ParseGroups(const QDomElement &domElement)
{
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    QMap<quint32, quint32> itemTool;
    QMap<quint32, bool> itemVisibility;

    QDomNode domNode = domElement.firstChild();
    while (not domNode.isNull())
    {
        if (domNode.isElement())
        {
            if (const QDomElement groupElement = domNode.toElement();
                not groupElement.isNull() && groupElement.tagName() == TagGroup)
            {
                VContainer::UpdateId(GetParametrUInt(groupElement, AttrId, NULL_ID_STR), valentinaNamespace);

                const auto [groupDataFirst, groupDataSecond] = ParseItemElement(groupElement);
                const QMap<quint32, quint32> group = groupDataSecond;
                auto i = group.constBegin();
                while (i != group.constEnd())
                {
                    if (not itemTool.contains(i.key()))
                    {
                        itemTool.insert(i.key(), i.value());
                    }

                    const bool previous = itemVisibility.value(i.key(), false);
                    itemVisibility.insert(i.key(), previous || groupDataFirst);
                    ++i;
                }
            }
        }
        domNode = domNode.nextSibling();
    }

    auto i = itemTool.constBegin();
    while (i != itemTool.constEnd())
    {
        if (tools.contains(i.value()))
        {
            VDataTool *tool = tools.value(i.value());
            tool->GroupVisibility(i.key(), itemVisibility.value(i.key(), true));
        }
        ++i;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::CountPatternBlockTags() const -> int
{
    const QDomElement rootElement = this->documentElement();
    if (rootElement.isNull())
    {
        return 0;
    }

    return rootElement.elementsByTagName(TagDraw).count();
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::setXMLContent(const QString &fileName)
{
    Clear();
    VDomDocument::setXMLContent(fileName);
    m_patternNumber = ReadPatternNumber();
    m_patternName = ReadPatternName();
    m_MPath = ReadMPath();
    m_watermarkPath = ReadWatermarkPath();
    m_companyName = ReadCompanyName();
    m_units = ReadUnits();
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::Clear()
{
    CancelFormulaDependencyChecks();
    VDomDocument::Clear();
    m_patternNumber.clear();
    m_patternName.clear();
    m_MPath.clear();
    m_watermarkPath.clear();
    m_companyName.clear();
    m_units = Unit::LAST_UNIT_DO_NOT_USE;
    modified = false;
    m_patternBlockMapper->Clear();
    m_patternGraph->Clear();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getTool return tool from tool list.
 * @param id tool id.
 * @return tool.
 */
auto VAbstractPattern::getTool(quint32 id) -> VDataTool *
{
    ToolExists(id);
    return tools.value(id);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::FindTool(quint32 id) -> VDataTool *
{
    return tools.contains(id) ? tools.value(id) : nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AddTool add tool to list tools.
 * @param id tool id.
 * @param tool tool.
 */
void VAbstractPattern::AddTool(quint32 id, VDataTool *tool)
{
    Q_ASSERT_X(id != 0, Q_FUNC_INFO, "id == 0");
    SCASSERT(tool != nullptr)
    tools.insert(id, tool);
    // When a tool is deleted (e.g. when an undo command frees a piece and its children), remove
    // it from the map automatically so no dangling pointer remains for LiteParseTree.
    connect(tool, &QObject::destroyed, [id]() -> void { tools.remove(id); });
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::RemoveTool(quint32 id)
{
    tools.remove(id);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ParsePieceNodes(const QDomElement &domElement) -> VPiecePath
{
    VPiecePath path;
    const QDomNodeList nodeList = domElement.childNodes();
    QDOM_LOOP(nodeList, i)
    {
        if (const QDomElement element = QDOM_ELEMENT(nodeList, i).toElement(); not element.isNull())
        {
            path.Append(ParseSANode(element));
        }
    }
    return path;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ParsePieceCSARecords(const QDomElement &domElement) -> QVector<CustomSARecord>
{
    const QDomNodeList nodeList = domElement.childNodes();
    QVector<CustomSARecord> records;
    records.reserve(nodeList.size());
    QDOM_LOOP(nodeList, i)
    {
        if (const QDomElement element = QDOM_ELEMENT(nodeList, i).toElement(); not element.isNull())
        {
            records.append({.startPoint = GetParametrUInt(element, VAbstractPattern::AttrStart, NULL_ID_STR),
                            .path = GetParametrUInt(element, VAbstractPattern::AttrPath, NULL_ID_STR),
                            .endPoint = GetParametrUInt(element, VAbstractPattern::AttrEnd, NULL_ID_STR),
                            .reverse = GetParametrBool(element, VAbstractPattern::AttrNodeReverse, falseStr),
                            .includeType = static_cast<PiecePathIncludeType>(
                                GetParametrUInt(element, VAbstractPattern::AttrIncludeAs, QChar('1')))});
        }
    }
    return records;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ParsePieceInternalPaths(const QDomElement &domElement) -> QVector<quint32>
{
    const QDomNodeList nodeList = domElement.childNodes();
    QVector<quint32> records;
    records.reserve(nodeList.size());
    QDOM_LOOP(nodeList, i)
    {
        if (const QDomElement element = QDOM_ELEMENT(nodeList, i).toElement(); not element.isNull())
        {
            if (const quint32 path = GetParametrUInt(element, VAbstractPattern::AttrPath, NULL_ID_STR); path > NULL_ID)
            {
                records.append(path);
            }
        }
    }
    return records;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ParsePiecePointRecords(const QDomElement &domElement) -> QVector<quint32>
{
    const QDomNodeList nodeList = domElement.childNodes();
    QVector<quint32> records;
    records.reserve(nodeList.size());
    QDOM_LOOP(nodeList, i)
    {
        if (const QDomElement element = QDOM_ELEMENT(nodeList, i).toElement(); not element.isNull())
        {
            if (const quint32 path = element.text().toUInt(); path > NULL_ID)
            {
                records.append(path);
            }
        }
    }
    return records;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ParseSANode(const QDomElement &domElement) -> VPieceNode
{
    const quint32 id = VDomDocument::GetParametrUInt(domElement, AttrIdObject, NULL_ID_STR);
    const bool reverse = VDomDocument::GetParametrUInt(domElement, VAbstractPattern::AttrNodeReverse, QChar('0'));
    const bool excluded = VDomDocument::GetParametrBool(domElement, VAbstractPattern::AttrNodeExcluded, falseStr);
    const bool uniqeness = VDomDocument::GetParametrBool(domElement, VAbstractPattern::AttrCheckUniqueness, trueStr);
    const QString saBefore = VDomDocument::GetParametrString(domElement,
                                                             VAbstractPattern::AttrSABefore,
                                                             currentSeamAllowance);
    const QString saAfter = VDomDocument::GetParametrString(domElement,
                                                            VAbstractPattern::AttrSAAfter,
                                                            currentSeamAllowance);
    const auto angle = static_cast<PieceNodeAngle>(VDomDocument::GetParametrUInt(domElement, AttrAngle, QChar('0')));

    const bool passmark = VDomDocument::GetParametrBool(domElement, VAbstractPattern::AttrNodePassmark, falseStr);
    const PassmarkLineType passmarkLine = StringToPassmarkLineType(
        VDomDocument::GetParametrString(domElement, VAbstractPattern::AttrNodePassmarkLine, strOne));
    const PassmarkAngleType passmarkAngleType = StringToPassmarkAngleType(
        VDomDocument::GetParametrString(domElement, VAbstractPattern::AttrNodePassmarkAngle, strStraightforward));

    const bool showSecond = VDomDocument::GetParametrBool(domElement,
                                                          VAbstractPattern::AttrNodeShowSecondPassmark,
                                                          trueStr);
    const bool passmarkOpening = VDomDocument::GetParametrBool(domElement,
                                                               VAbstractPattern::AttrNodePassmarkOpening,
                                                               falseStr);
    const bool notMirrored = VDomDocument::GetParametrBool(domElement,
                                                           VAbstractPattern::AttrNodePassmarkNotMirrored,
                                                           falseStr);

    const bool manualPassmarkLength = VDomDocument::GetParametrBool(domElement,
                                                                    VAbstractPattern::AttrManualPassmarkLength,
                                                                    falseStr);
    const QString passmarkLength = VDomDocument::GetParametrEmptyString(domElement,
                                                                        VAbstractPattern::AttrPassmarkLength);

    const bool manualPassmarkWidth = VDomDocument::GetParametrBool(domElement,
                                                                   VAbstractPattern::AttrManualPassmarkWidth,
                                                                   falseStr);
    const QString passmarkWidth = VDomDocument::GetParametrEmptyString(domElement, VAbstractPattern::AttrPassmarkWidth);

    const bool manualPassmarkAngle = VDomDocument::GetParametrBool(domElement,
                                                                   VAbstractPattern::AttrManualPassmarkAngle,
                                                                   falseStr);
    const QString passmarkAngle = VDomDocument::GetParametrEmptyString(domElement, VAbstractPattern::AttrPassmarkAngle);

    const QString passmarkVisibility = VDomDocument::GetParametrString(domElement,
                                                                       VAbstractPattern::AttrPassmarkVisibility,
                                                                       QChar('1'));

    const bool turnPoint = VDomDocument::GetParametrBool(domElement, VAbstractPattern::AttrNodeTurnPoint, trueStr);

    const QString t = VDomDocument::GetParametrString(domElement, AttrType, VAbstractPattern::NodePoint);
    Tool tool = Tool::LAST_ONE_DO_NOT_USE;

    switch (const QStringList types{VAbstractPattern::NodePoint,
                                    VAbstractPattern::NodeArc,
                                    VAbstractPattern::NodeSpline,
                                    VAbstractPattern::NodeSplinePath,
                                    VAbstractPattern::NodeElArc};
            types.indexOf(t))
    {
        case 0: // VAbstractPattern::NodePoint
            tool = Tool::NodePoint;
            break;
        case 1: // VAbstractPattern::NodeArc
            tool = Tool::NodeArc;
            break;
        case 2: // VAbstractPattern::NodeSpline
            tool = Tool::NodeSpline;
            break;
        case 3: // VAbstractPattern::NodeSplinePath
            tool = Tool::NodeSplinePath;
            break;
        case 4: // NodeElArc
            tool = Tool::NodeElArc;
            break;
        default:
            throw VException(QCoreApplication::translate("VAbstractPattern", "Wrong tag name '%1'.").arg(t));
    }
    VPieceNode node(id, tool, reverse);
    node.SetFormulaSABefore(saBefore);
    node.SetFormulaSAAfter(saAfter);
    node.SetAngleType(angle);
    node.SetExcluded(excluded);
    node.SetCheckUniqueness(uniqeness);
    node.SetShowSecondPassmark(showSecond);
    node.SetPassmarkClockwiseOpening(passmarkOpening);
    node.SetPassmarkNotMirrored(notMirrored);
    node.SetPassmark(passmark);
    node.SetPassmarkLineType(passmarkLine);
    node.SetPassmarkAngleType(passmarkAngleType);
    node.SetManualPassmarkLength(manualPassmarkLength);
    node.SetFormulaPassmarkLength(passmarkLength);
    node.SetManualPassmarkWidth(manualPassmarkWidth);
    node.SetFormulaPassmarkWidth(passmarkWidth);
    node.SetManualPassmarkAngle(manualPassmarkAngle);
    node.SetFormulaPassmarkAngle(passmarkAngle);
    node.SetFormulaPassmarkVisibility(passmarkVisibility);
    node.SetTurnPoint(turnPoint);

    return node;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::AddToolOnRemove(VDataTool *tool)
{
    SCASSERT(tool != nullptr)
    toolsOnRemove.append(tool);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getHistory return list with list of history records.
 * @return list of history records.
 */
auto VAbstractPattern::getHistory() -> QVector<VToolRecord> *
{
    return &history;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::getHistory() const -> const QVector<VToolRecord> *
{
    return &history;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetLocalHistory(int blockIndex) const -> QVector<VToolRecord>
{
    const int index = blockIndex >= 0 ? blockIndex : PatternBlockMapper()->GetActiveId();
    if (index < 0)
    {
        return {};
    }

    QVector<VToolRecord> historyPatternBlock;
    historyPatternBlock.reserve(history.size());
    for (const auto &tool : history)
    {
        if (tool.GetPatternBlockIndex() == index)
        {
            historyPatternBlock.append(tool);
        }
    }
    return historyPatternBlock;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::MPath() const -> QString
{
    return m_MPath;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetMPath(const QString &path)
{
    if (QDomElement domElement = UniqueTag(TagMeasurements); not domElement.isNull())
    {
        auto RemoveDimensions = [&domElement]()
        {
            domElement.removeAttribute(AttrDimensionA);
            domElement.removeAttribute(AttrDimensionB);
            domElement.removeAttribute(AttrDimensionC);
        };

        if (not path.isEmpty())
        {
            SetAttribute(domElement, AttrPath, path);

            if (path.endsWith(QStringLiteral(".vit")) || path.endsWith(QStringLiteral(".otu")) || QFileInfo(m_MPath).fileName() != QFileInfo(path).fileName())
            {
                RemoveDimensions();
            }
        }
        else
        {
            domElement.removeAttribute(AttrPath);
            RemoveDimensions();
        }
        m_MPath = path;
        SetPatternWasChanged(true);
        modified = true;
        emit patternChanged(false);
    }
    else
    {
        qDebug() << "Can't save path to measurements" << Q_FUNC_INFO;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::SiblingNodeId(const quint32 &nodeId) const -> quint32
{
    // This check helps to find missed tools in the switch
    Q_STATIC_ASSERT_X(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 73, "Check if need to ignore modeling tools.");

    quint32 siblingId = NULL_ID;

    const QVector<VToolRecord> history = GetLocalHistory();
    for (qint32 i = 0; i < history.size(); ++i)
    {
        const VToolRecord tool = history.at(i);
        if (nodeId == tool.GetId())
        {
            if (i == 0)
            {
                siblingId = NULL_ID;
            }
            else
            {
                for (qint32 j = i; j > 0; --j)
                {
                    const VToolRecord tool = history.at(j - 1);
                    switch (tool.GetToolType())
                    {
                        case Tool::Arrow:
                        case Tool::Piece:
                        case Tool::UnionDetails:
                        case Tool::NodeArc:
                        case Tool::NodeElArc:
                        case Tool::NodePoint:
                        case Tool::NodeSpline:
                        case Tool::NodeSplinePath:
                        case Tool::PlaceLabel:
                        case Tool::PiecePath:
                        case Tool::InsertNode:
                        case Tool::DuplicateDetail:
                        case Tool::BackgroundImage:
                        case Tool::BackgroundImageControls:
                        case Tool::BackgroundPixmapImage:
                        case Tool::BackgroundSVGImage:
                            continue;
                        default:
                            siblingId = tool.GetId();
                            j = 0; // break loop
                            break;
                    }
                }
            }
        }
    }
    return siblingId;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetDescription() const -> QString
{
    return UniqueTagText(TagDescription);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetDescription(const QString &text)
{
    CheckTagExists(TagDescription);
    setTagText(TagDescription, text);
    modified = true;
    emit patternChanged(false);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetNotes() const -> QString
{
    return UniqueTagText(TagNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetNotes(const QString &text)
{
    CheckTagExists(TagNotes);
    setTagText(TagNotes, text);
    modified = true;
    emit patternChanged(false);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetPatternName() const -> QString
{
    return m_patternName;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetPatternName(const QString &name)
{
    CheckTagExists(TagPatternName);
    setTagText(TagPatternName, name);
    m_patternName = name;
    modified = true;
    emit patternChanged(false);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetCustomer() const -> QString
{
    return UniqueTagText(TagCustomerName);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetCustomer(const QString &text)
{
    CheckTagExists(TagCustomerName);
    setTagText(TagCustomerName, text);
    modified = true;
    emit patternChanged(false);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetCustomerBirthDate() const -> QString
{
    return UniqueTagText(TagCustomerBirthDate);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetCustomerBirthDate(const QString &text)
{
    CheckTagExists(TagCustomerBirthDate);
    setTagText(TagCustomerBirthDate, text);
    modified = true;
    emit patternChanged(false);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetCustomerEmail() const -> QString
{
    return UniqueTagText(TagCustomerEmail);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetCustomerEmail(const QString &text)
{
    CheckTagExists(TagCustomerEmail);
    setTagText(TagCustomerEmail, text);
    modified = true;
    emit patternChanged(false);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetCollection() const -> QString
{
    return UniqueTagText(TagCollection);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetCollection(const QString &text)
{
    CheckTagExists(TagCollection);
    setTagText(TagCollection, text);
    modified = true;
    emit patternChanged(false);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetFabricInfo() const -> QString
{
    return UniqueTagText(TagFabricInfo);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetFabricInfo(const QString &text)
{
    CheckTagExists(TagFabricInfo);
    setTagText(TagFabricInfo, text);
    modified = true;
    emit patternChanged(false);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetDefaultActivateSA() const -> bool
{
    const QString value = GetPatternVariable(AttrDefaultActivateSA);
    return value.isEmpty() ? true : value == QLatin1String("true");
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetDefaultActivateSA(bool on)
{
    SePatternVariable(AttrDefaultActivateSA, on ? QStringLiteral("true") : QStringLiteral("false"));
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetDefaultHideMainPath() const -> bool
{
    const QString value = GetPatternVariable(AttrDefaultHideMainPath);
    if (not value.isEmpty())
    {
        return value == QLatin1String("true");
    }
    return VAbstractValApplication::VApp()->ValentinaSettings()->IsHideMainPath();
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetDefaultHideMainPath(bool on)
{
    SePatternVariable(AttrDefaultHideMainPath, on ? QStringLiteral("true") : QStringLiteral("false"));
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetDefaultSewLineOnDrawing() const -> bool
{
    const QString value = GetPatternVariable(AttrDefaultSewLineOnDrawing);
    if (not value.isEmpty())
    {
        return value == QLatin1String("true");
    }
    return VAbstractValApplication::VApp()->ValentinaSettings()->GetSewLineOnDrawing();
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetDefaultSewLineOnDrawing(bool on)
{
    SePatternVariable(AttrDefaultSewLineOnDrawing, on ? QStringLiteral("true") : QStringLiteral("false"));
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetDefaultSecondPassmarkOnSeamLine() const -> bool
{
    const QString value = GetPatternVariable(AttrDefaultSecondPassmarkOnSeamLine);
    if (not value.isEmpty())
    {
        return value == QLatin1String("true");
    }
    return VAbstractValApplication::VApp()->Settings()->IsDoublePassmark();
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetDefaultSecondPassmarkOnSeamLine(bool on)
{
    SePatternVariable(AttrDefaultSecondPassmarkOnSeamLine,
                      on ? QStringLiteral("true") : QStringLiteral("false"));
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetCartoucheTemplate() const -> QString
{
    const QDomElement element = UniqueTag(TagCartouche);
    return element.isNull() ? QString() : element.attribute(AttrTemplate);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetCartoucheTemplate(const QString &name)
{
    // Empty name = no information block: remove the whole element (presence of the
    // tag carries both the activation and the template reference).
    if (name.isEmpty())
    {
        const QDomElement element = UniqueTag(TagCartouche);
        if (not element.isNull())
        {
            documentElement().removeChild(element);
            modified = true;
            emit patternChanged(false);
        }
        return;
    }

    // The empty case was handled above (whole element removed); here the name is
    // never empty, so a plain setAttribute is all that is needed.
    QDomElement element = CheckTagExists(TagCartouche);
    element.setAttribute(AttrTemplate, name);
    modified = true;
    emit patternChanged(false);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetCompanyName() const -> QString
{
    return m_companyName;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetPatternNumber() const -> QString
{
    return m_patternNumber;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::SetWatermarkPath(const QString &path) -> bool
{
    QDomElement tag = CheckTagExists(TagWatermark);

    if (path.isEmpty())
    {
        QDomNode parent = tag.parentNode();
        parent.removeChild(tag);

        emit patternChanged(false);
        SetPatternWasChanged(true);
        m_watermarkPath = path;
        modified = true;
        return true;
    }

    if (setTagText(tag, path))
    {
        emit patternChanged(false);
        SetPatternWasChanged(true);
        m_watermarkPath = path;
        modified = true;
        return true;
    }
    else
    {
        qDebug() << "Can't save path to watermark" << Q_FUNC_INFO;
        return false;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetWatermarkPath() const -> QString
{
    return m_watermarkPath;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetFinalMeasurements() const -> QVector<VFinalMeasurement>
{
    const QDomNodeList list = elementsByTagName(TagFinalMeasurements);
    if (list.isEmpty() || list.at(0).childNodes().isEmpty())
    {
        return {};
    }

    return GetFMeasurements(list.at(0).toElement());
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetFinalMeasurements(const QVector<VFinalMeasurement> &measurements)
{
    QDomElement tag = CheckTagExists(TagFinalMeasurements);
    RemoveAllChildren(tag);
    SetFMeasurements(tag, measurements);
    modified = true;
    emit patternChanged(false);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetCoverPage() const -> VCoverPageData
{
    VCoverPageData data;

    const QDomNodeList list = elementsByTagName(TagCoverPage);
    if (list.isEmpty())
    {
        return data;
    }

    const QDomElement element = list.at(0).toElement();

    data.freeText = element.firstChildElement(kTagCoverFreeText).text();

    // A symbol is its id and nothing else: what it draws is the library's to say, and the library
    // is read when the page is drawn rather than when the pattern is opened. **An id the library no
    // longer holds is kept rather than dropped** — a library that was reorganised may be put back,
    // and a reference thrown away here could not be.
    for (QDomElement symbol = element.firstChildElement(kTagCoverSymbol); !symbol.isNull();
         symbol = symbol.nextSiblingElement(kTagCoverSymbol))
    {
        const QString id = symbol.attribute(kAttrCoverId);
        if (!id.isEmpty() && !data.symbols.contains(id))
        {
            data.symbols.append(id);
        }
    }

    for (QDomElement margin = element.firstChildElement(kTagCoverMarginName); !margin.isNull();
         margin = margin.nextSiblingElement(kTagCoverMarginName))
    {
        const qreal mm = margin.attribute(kAttrCoverWidth).toDouble();
        // Zero is not a margin, and neither is a width with no name under it.
        if (mm <= 0.0)
        {
            continue;
        }
        VCoverPageMarginName name;
        name.mm = mm;
        name.texts = ReadCoverTexts(margin);
        data.marginNames.append(name);
    }

    return data;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetCoverPage(const VCoverPageData &data)
{
    const QDomNodeList list = elementsByTagName(TagCoverPage);

    if (IsCoverPageEmpty(data))
    {
        // **The block goes rather than standing there empty.** A pattern with nothing to say says it
        // by carrying nothing: an empty element would be a line in every file that was merely looked
        // at, and a reader would have to decide what it meant.
        if (!list.isEmpty())
        {
            documentElement().removeChild(list.at(0));
            modified = true;
            emit patternChanged(false);
        }
        return;
    }

    QDomElement element = CheckTagExists(TagCoverPage);
    RemoveAllChildren(element);

    if (!data.freeText.isEmpty())
    {
        QDomElement free = createElement(kTagCoverFreeText);
        free.appendChild(createTextNode(data.freeText));
        element.appendChild(free);
    }

    for (const QString &id : data.symbols)
    {
        if (id.isEmpty())
        {
            continue;
        }
        QDomElement symbol = createElement(kTagCoverSymbol);
        symbol.setAttribute(kAttrCoverId, id);
        element.appendChild(symbol);
    }

    for (const VCoverPageMarginName &margin : data.marginNames)
    {
        if (margin.mm <= 0.0 || !HasCoverText(margin.texts))
        {
            continue;
        }
        QDomElement marginElement = createElement(kTagCoverMarginName);
        marginElement.setAttribute(kAttrCoverWidth, QString::number(margin.mm, 'f', 2));
        WriteCoverTexts(*this, marginElement, margin.texts);
        element.appendChild(marginElement);
    }

    modified = true;
    emit patternChanged(false);
}


//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetPatternWasChanged(bool changed)
{
    patternLabelWasChanged = changed;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetPatternWasChanged() const -> bool
{
    return patternLabelWasChanged;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetPassmarkLengthVariable() const -> QString
{
    return GetPatternVariable(AttrPassmarkLength);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetPassmarkLengthVariable(const QString &name)
{
    SePatternVariable(AttrPassmarkLength, name);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetPassmarkWidthVariable() const -> QString
{
    return GetPatternVariable(AttrPassmarkWidth);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetPassmarkWidthVariable(const QString &name)
{
    SePatternVariable(AttrPassmarkWidth, name);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetImage() const -> VPatternImage
{
    VPatternImage image;

    if (const QDomNodeList list = elementsByTagName(TagImage); not list.isEmpty())
    {
        QDomElement const imgTag = list.at(0).toElement();
        if (not imgTag.isNull())
        {
            image.SetContentData(imgTag.text().toLatin1(), imgTag.attribute(AttrContentType));
        }
    }

    return image;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::SetImage(const VPatternImage &image) -> bool
{
    QDomElement imageElement = CheckTagExists(TagImage);
    setTagText(imageElement, image.ContentData());
    imageElement.setAttribute(AttrContentType, image.ContentType());
    modified = true;
    emit patternChanged(false);
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::DeleteImage()
{
    QDomElement pattern = documentElement();
    pattern.removeChild(CheckTagExists(TagImage));
    modified = true;
    emit patternChanged(false);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetBackgroundImages() const -> QVector<VBackgroundPatternImage>
{
    QVector<VBackgroundPatternImage> images;
    const QDomNodeList list = elementsByTagName(TagBackgroundImages);
    if (list.isEmpty())
    {
        return images;
    }

    if (QDomElement const imagesTag = list.at(0).toElement(); not imagesTag.isNull())
    {
        QDomNode imageNode = imagesTag.firstChild();
        while (not imageNode.isNull())
        {
            if (const QDomElement imageElement = imageNode.toElement(); not imageElement.isNull())
            {
                images.append(GetBackgroundPatternImage(imageElement));
            }
            imageNode = imageNode.nextSibling();
        }
    }

    return images;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SaveBackgroundImages(const QVector<VBackgroundPatternImage> &images)
{
    QDomElement imagesElement = CheckTagExists(TagBackgroundImages);
    RemoveAllChildren(imagesElement);

    for (const auto &image : images)
    {
        if (not image.Id().isNull())
        {
            QDomElement imageElement = createElement(TagBackgroundImage);
            WriteBackgroundImage(imageElement, image);
            imagesElement.appendChild(imageElement);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetBackgroundImage(const QUuid &id) const -> VBackgroundPatternImage
{
    if (const QDomElement imageElement = GetBackgroundImageElement(id); not imageElement.isNull())
    {
        return GetBackgroundPatternImage(imageElement);
    }

    QT_WARNING_PUSH
    QT_WARNING_DISABLE_GCC("-Wnoexcept")

    return {};

    QT_WARNING_POP
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SaveBackgroundImage(const VBackgroundPatternImage &image)
{
    if (QDomElement backgroundImageElement = GetBackgroundImageElement(image.Id()); backgroundImageElement.isNull())
    {
        QDomElement imageElement = createElement(TagBackgroundImage);
        WriteBackgroundImage(imageElement, image);
        QDomElement imagesElement = CheckTagExists(TagBackgroundImages);
        imagesElement.appendChild(imageElement);
    }
    else
    {
        WriteBackgroundImage(backgroundImageElement, image);
    }

    modified = true;
    emit patternChanged(false);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::DeleteBackgroundImage(const QUuid &id)
{
    const QDomNodeList list = elementsByTagName(TagBackgroundImages);
    if (list.isEmpty())
    {
        return;
    }

    if (QDomElement imagesTag = list.at(0).toElement(); not imagesTag.isNull())
    {
        QDomNode imageNode = imagesTag.firstChild();
        while (not imageNode.isNull())
        {
            if (const QDomElement imageElement = imageNode.toElement(); not imageElement.isNull())
            {
                if (auto const imageId = QUuid(GetParametrEmptyString(imageElement, AttrImageId)); imageId == id)
                {
                    imagesTag.removeChild(imageElement);

                    if (imagesTag.childNodes().isEmpty())
                    {
                        if (QDomNode parent = imagesTag.parentNode(); not parent.isNull())
                        {
                            parent.removeChild(imagesTag);
                        }
                    }

                    modified = true;
                    emit patternChanged(false);

                    return;
                }
            }
            imageNode = imageNode.nextSibling();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetVersion() const -> QString
{
    return UniqueTagText(TagVersion, VPatternConverter::PatternMaxVerStr);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetVersion()
{
    setTagText(TagVersion, VPatternConverter::PatternMaxVerStr);
    emit patternChanged(false);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief haveLiteChange we have unsaved change.
 */
void VAbstractPattern::haveLiteChange()
{
    emit patternChanged(false);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::NeedFullParsing()
{
    emit UndoCommand();
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::ClearScene()
{
    emit ClearMainWindow();
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::CheckInLayoutList()
{
    emit UpdateInLayoutList();
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SelectedDetail(quint32 id)
{
    emit ShowDetail(id);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::UpdateVisiblityGroups()
{
    emit UpdateGroups();
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::PatternGraph() const -> VPatternGraph *
{
    return m_patternGraph;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::ProcessFormulaDependencies(const QString &formula, quint32 id,
                                                   const QHash<QString, QList<quint32>> &variables,
                                                   quint64 generation)
{
    QList<QString> tokens;
    try
    {
        QScopedPointer<qmu::QmuTokenParser> const cal(new qmu::QmuTokenParser(formula, false, false));
        tokens = cal->GetTokens().values();
    }
    catch (qmu::QmuParserError &e)
    {
        qDebug() << "\nMath parser error:\n"
                 << "--------------------------------------\n"
                 << "Message:     " << e.GetMsg() << "\n"
                 << "Expression:  " << e.GetExpr() << "\n"
                 << "--------------------------------------";
        return;
    }

    int checkCounter = 0;
    const int checkInterval = 10; // Check every 10 iterations

    for (const auto &token : std::as_const(tokens))
    {
        if (++checkCounter >= checkInterval && m_dependencyCheckGeneration.load() != generation)
        {
            return;
        }

        if (!variables.contains(token))
        {
            continue;
        }

        QList<quint32> const references = variables.value(token);
        for (const auto &ref : references)
        {
            if (ref > NULL_ID && ref != id)
            {
                m_patternGraph->AddEdge(ref, id);
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::FindFormulaDependencies(const QString &formula,
                                               quint32 id,
                                               const QHash<QString, QList<quint32>> &variables)
{
    if (formula.isEmpty() || id == NULL_ID || variables.isEmpty())
    {
        return;
    }

    // The dependency-graph topology is fixed during a value-only (lite) reparse, so skip building
    // edges here. AddEdge would no-op anyway, but returning early also avoids the per-formula
    // tokenization and the QtConcurrent task spawned below.
    if (m_patternGraph == nullptr || !m_patternGraph->IsGraphRebuildEnabled())
    {
        return;
    }

    // Create a new watcher for this task
    auto *watcher = new QFutureWatcher<void>(this);

    // Connect to cleanup when finished
    connect(watcher, &QFutureWatcher<void>::finished, this, &VAbstractPattern::CleanDependenciesWatcher);

    // Create the async task
    const quint64 generation = m_dependencyCheckGeneration.load();
    QFuture<void> const future = QtConcurrent::run(
        [this, formula, id, variables, generation]() -> void
        { ProcessFormulaDependencies(formula, id, variables, generation); });

    watcher->setFuture(future);

    QMutexLocker const locker(&m_watchersMutex);
    m_formulaDependenciesWatchers.append(watcher);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::IsPatternGraphComplete() const -> bool
{
    if (!m_watchersMutex.tryLock(100))
    {
        return false;
    }

    auto Unlock = qScopeGuard([this]() -> auto { m_watchersMutex.unlock(); });

    return m_fileParsingCompleted && m_formulaDependenciesWatchers.isEmpty();
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::PatternBlockMapper() const -> VPatternBlockMapper *
{
    return m_patternBlockMapper;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::ToolExists(const quint32 &id)
{
    if (!tools.contains(id))
    {
        throw VExceptionBadId(QCoreApplication::translate("VAbstractPattern", "Can't find tool in table."), id);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ParsePathNodes(const QDomElement &domElement) -> VPiecePath
{
    VPiecePath path;
    const QDomNodeList nodeList = domElement.childNodes();
    QDOM_LOOP(nodeList, i)
    {
        if (const QDomElement element = QDOM_ELEMENT(nodeList, i).toElement();
            not element.isNull() && element.tagName() == VAbstractPattern::TagNode)
        {
            path.Append(ParseSANode(element));
        }
    }
    return path;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::RootElementOrder() -> const QStringList &
{
    // **The order of the <xs:sequence> of <pattern>, from ostara/v1.0.0.xsd.** Written once, here,
    // and read by InsertRootElement alone: a second list somewhere else would be a second opinion
    // about where an element goes, and the file can only have one.
    static const QStringList kOrder{
        TagVersion,             // 0  required
        TagUnit,                // 1  required
        TagImage,               // 2
        TagDescription,         // 3
        TagNotes,               // 4
        TagFabricInfo,          // 5
        TagCollection,          // 6
        TagPatternName,         // 7
        TagPatternNum,          // 8
        TagCompanyName,         // 9
        TagCustomerName,        // 10
        TagCustomerBirthDate,   // 11
        TagCustomerEmail,       // 12
        TagWatermark,           // 13
        TagCartouche,           // 14
        TagPatternMaterials,    // 15
        TagFinalMeasurements,   // 16
        TagBackgroundImages,    // 17
        TagCoverPage,           // 18
        TagMeasurements,        // 19 required
        TagIncrements,          // 20 required
        TagPreviewCalculations, // 21 required
        TagLineNames,           // 22
        TagDraw,                // 23 one or more
        TagPieceGroups,         // 24
        TagSizeRanges,          // 25
    };
    return kOrder;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::InsertRootElement(const QDomElement &element)
{
    QDomElement root = documentElement();
    if (root.isNull() || element.isNull())
    {
        return;
    }

    // **After the last one of its own tag, when the document holds any.** A tag that may repeat —
    // `draw` is the one — has to keep its own kind together: placing a second <draw> before the
    // first would leave the sequence valid and still reorder the pattern blocks, which are
    // addressed by their rank.
    const QDomNodeList same = elementsByTagName(element.tagName());
    if (not same.isEmpty())
    {
        root.insertAfter(element, same.at(same.size() - 1));
        return;
    }

    const QStringList &order = RootElementOrder();
    const vsizetype at = order.indexOf(element.tagName());
    if (at < 0)
    {
        // A name the schema does not carry. Nothing can be said about where it belongs, and
        // refusing it would throw away something a reader may still understand.
        root.appendChild(element);
        return;
    }

    // **After the nearest predecessor the document already holds.** Everything between it and this
    // element's place is absent — that is why the walk went past it — so this slot is the right
    // one. `version` and `unit` are required and come first, so the walk always finds somewhere.
    for (vsizetype i = at - 1; i >= 0; --i)
    {
        if (const QDomNodeList list = elementsByTagName(order.at(i)); not list.isEmpty())
        {
            root.insertAfter(element, list.at(list.size() - 1));
            return;
        }
    }

    // Nothing precedes it: it goes first. Unreachable while `version` is required, and kept so
    // that an element is never silently dropped if that ever stops being true.
    root.insertBefore(element, root.firstChild());
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::CheckTagExists(const QString &tag) -> QDomElement
{
    const QDomNodeList list = elementsByTagName(tag);
    if (not list.isEmpty())
    {
        return list.at(0).toElement();
    }

    // **What may be created and where it goes are two different questions.** This is the first, and
    // it is short on purpose: the elements an information is written into, and nothing else. The
    // mandatory ones are the file's skeleton and are made by CreateEmptyFile; `unit` belongs to the
    // pattern's settings; the rest have writers of their own. Placement is the second question, and
    // it has one answer for everybody — see RootElementOrder.
    static const QStringList kCreatable{
        TagImage,   TagDescription, TagNotes,      TagFabricInfo,     TagCollection,
        TagPatternName, TagPatternNum, TagCompanyName, TagCustomerName, TagCustomerBirthDate,
        TagCustomerEmail, TagWatermark, TagCartouche, TagFinalMeasurements, TagBackgroundImages,
        TagCoverPage};

    if (not kCreatable.contains(tag))
    {
        return {};
    }

    QDomElement element = createElement(tag);
    InsertRootElement(element);
    SetVersion();
    return element;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ListIncrements() const -> QStringList
{
    QStringList increments;

    auto GetExpressions = [&increments, this](const QString &type)
    {
        const QDomNodeList list = elementsByTagName(type);
        increments.reserve(list.size());
        for (int i = 0; i < list.size(); ++i)
        {
            const QString name = GetParametrEmptyString(list.at(i).toElement(), AttrName);
            if (not name.isEmpty())
            {
                increments.append(name);
            }
        }
    };

    GetExpressions(TagIncrement);
    GetExpressions(TagPreviewCalculations);

    return increments;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ListExpressions() const -> QVector<VFormulaField>
{
    // If new tool bring absolutely new type and has formula(s) create new method to cover it.
    // Note. Tool Union Details also contains formulas, but we don't use them for union and keep only to simplifying
    // working with nodes. Same code for saving reading.
    Q_STATIC_ASSERT(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 73);

    auto futurePointExpressions = QtConcurrent::run([this]() { return ListPointExpressions(); });
    auto futureArcExpressions = QtConcurrent::run([this]() { return ListArcExpressions(); });
    auto futureElArcExpressions = QtConcurrent::run([this]() { return ListElArcExpressions(); });
    auto futureSplineExpressions = QtConcurrent::run([this]() { return ListSplineExpressions(); });
    auto futureIncrementExpressions = QtConcurrent::run([this]() { return ListIncrementExpressions(); });
    auto futureOperationExpressions = QtConcurrent::run([this]() { return ListOperationExpressions(); });
    auto futurePathExpressions = QtConcurrent::run([this]() { return ListPathExpressions(); });
    auto futurePieceExpressions = QtConcurrent::run([this]() { return ListPieceExpressions(); });
    auto futureFinalMeasurementsExpressions = QtConcurrent::run([this]() { return ListFinalMeasurementsExpressions(); });

    QVector<VFormulaField> list;
    list << futurePointExpressions.result();
    list << futureArcExpressions.result();
    list << futureElArcExpressions.result();
    list << futureSplineExpressions.result();
    list << futureIncrementExpressions.result();
    list << futureOperationExpressions.result();
    list << futurePathExpressions.result();
    list << futurePieceExpressions.result();
    list << futureFinalMeasurementsExpressions.result();

    return list;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ListPointExpressions() const -> QVector<VFormulaField>
{
    // Check if new tool doesn't bring new attribute with a formula.
    // If no just increment a number.
    // If new tool bring absolutely new type and has formula(s) create new method to cover it.
    Q_STATIC_ASSERT(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 73);

    QVector<VFormulaField> expressions;
    const QDomNodeList list = elementsByTagName(TagPoint);
    for (int i = 0; i < list.size(); ++i)
    {
        const QDomElement dom = list.at(i).toElement();

        // Each tag can contains several attributes.
        ReadExpressionAttribute(expressions, dom, AttrLength);
        ReadExpressionAttribute(expressions, dom, AttrAngle);
        ReadExpressionAttribute(expressions, dom, AttrC1Radius);
        ReadExpressionAttribute(expressions, dom, AttrC2Radius);
        ReadExpressionAttribute(expressions, dom, AttrCRadius);
        ReadExpressionAttribute(expressions, dom, AttrRadius);
        ReadExpressionAttribute(expressions, dom, AttrWidth);
        ReadExpressionAttribute(expressions, dom, AttrHeight);
    }

    return expressions;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ListArcExpressions() const -> QVector<VFormulaField>
{
    // Check if new tool doesn't bring new attribute with a formula.
    // If no just increment number.
    // If new tool bring absolutely new type and has formula(s) create new method to cover it.
    Q_STATIC_ASSERT(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 73);

    QVector<VFormulaField> expressions;
    const QDomNodeList list = elementsByTagName(TagArc);
    for (int i = 0; i < list.size(); ++i)
    {
        const QDomElement dom = list.at(i).toElement();

        // Each tag can contains several attributes.
        ReadExpressionAttribute(expressions, dom, AttrAngle1);
        ReadExpressionAttribute(expressions, dom, AttrAngle2);
        ReadExpressionAttribute(expressions, dom, AttrRadius);
        ReadExpressionAttribute(expressions, dom, AttrLength);
    }

    return expressions;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ListElArcExpressions() const -> QVector<VFormulaField>
{
    // Check if new tool doesn't bring new attribute with a formula.
    // If no just increment number.
    // If new tool bring absolutely new type and has formula(s) create new method to cover it.
    Q_STATIC_ASSERT(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 73);

    QVector<VFormulaField> expressions;
    const QDomNodeList list = elementsByTagName(TagElArc);
    for (int i = 0; i < list.size(); ++i)
    {
        const QDomElement dom = list.at(i).toElement();

        // Each tag can contains several attributes.
        ReadExpressionAttribute(expressions, dom, AttrRadius1);
        ReadExpressionAttribute(expressions, dom, AttrRadius2);
        ReadExpressionAttribute(expressions, dom, AttrAngle1);
        ReadExpressionAttribute(expressions, dom, AttrAngle2);
        ReadExpressionAttribute(expressions, dom, AttrRotationAngle);
        ReadExpressionAttribute(expressions, dom, AttrLength);
    }

    return expressions;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ListSplineExpressions() const -> QVector<VFormulaField>
{
    // Check if new tool doesn't bring new attribute with a formula.
    // If no just increment number.
    // If new tool bring absolutely new type and has formula(s) create new method to cover it.
    Q_STATIC_ASSERT(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 73);

    QVector<VFormulaField> expressions;
    const QDomNodeList list = elementsByTagName(TagSpline);
    for (int i = 0; i < list.size(); ++i)
    {
        const QDomElement dom = list.at(i).toElement();

        // Each tag can contains several attributes.
        ReadExpressionAttribute(expressions, dom, AttrWidth);

        const QDomNodeList offestNodes = dom.elementsByTagName(TagOffset);
        for (int i = 0; i < offestNodes.size(); ++i)
        {
            const QDomElement dom = offestNodes.at(i).toElement();

            ReadExpressionAttribute(expressions, dom, AttrWidth);
        }

        const QDomNodeList pathPointNodes = dom.elementsByTagName(TagPathPoint);
        for (int i = 0; i < pathPointNodes.size(); ++i)
        {
            const QDomElement dom = pathPointNodes.at(i).toElement();

            // Each tag can contains several attributes.
            ReadExpressionAttribute(expressions, dom, AttrKAsm1);
            ReadExpressionAttribute(expressions, dom, AttrKAsm2);
            ReadExpressionAttribute(expressions, dom, AttrAngle);
            ReadExpressionAttribute(expressions, dom, AttrAngle1);
            ReadExpressionAttribute(expressions, dom, AttrAngle2);
            ReadExpressionAttribute(expressions, dom, AttrLength1);
            ReadExpressionAttribute(expressions, dom, AttrLength2);
        }
    }

    return expressions;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ListIncrementExpressions() const -> QVector<VFormulaField>
{
    QVector<VFormulaField> expressions;
    const QDomNodeList list = elementsByTagName(TagIncrement);
    for (int i = 0; i < list.size(); ++i)
    {
        const QDomElement dom = list.at(i).toElement();

        ReadExpressionAttribute(expressions, dom, AttrFormula);
    }

    return expressions;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ListOperationExpressions() const -> QVector<VFormulaField>
{
    // Check if new tool doesn't bring new attribute with a formula.
    // If no just increment number.
    // If new tool bring absolutely new type and has formula(s) create new method to cover it.
    Q_STATIC_ASSERT(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 73);

    QVector<VFormulaField> expressions;
    const QDomNodeList list = elementsByTagName(TagOperation);
    for (int i = 0; i < list.size(); ++i)
    {
        const QDomElement dom = list.at(i).toElement();

        // Each tag can contains several attributes.
        ReadExpressionAttribute(expressions, dom, AttrAngle);
        ReadExpressionAttribute(expressions, dom, AttrLength);
        // The only formula of the tool that closes a dart.
        ReadExpressionAttribute(expressions, dom, AttrClosingDivisor);
    }

    return expressions;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ListNodesExpressions(const QDomElement &nodes) const -> QVector<VFormulaField>
{
    // Check if new tool doesn't bring new attribute with a formula.
    // If no just increment number.
    // If new tool bring absolutely new type and has formula(s) create new method to cover it.
    Q_STATIC_ASSERT(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 73);

    QVector<VFormulaField> expressions;

    const QDomNodeList nodeList = nodes.childNodes();
    QDOM_LOOP(nodeList, i)
    {
        if (const QDomElement element = QDOM_ELEMENT(nodeList, i).toElement();
            !element.isNull() && element.tagName() == VAbstractPattern::TagNode)
        {
            ReadExpressionAttribute(expressions, element, VAbstractPattern::AttrSABefore);
            ReadExpressionAttribute(expressions, element, VAbstractPattern::AttrSAAfter);
            ReadExpressionAttribute(expressions, element, VAbstractPattern::AttrPassmarkLength);
        }
    }
    return expressions;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ListPathExpressions() const -> QVector<VFormulaField>
{
    // Check if new tool doesn't bring new attribute with a formula.
    // If no just increment number.
    // If new tool bring absolutely new type and has formula(s) create new method to cover it.
    Q_STATIC_ASSERT(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 73);

    QVector<VFormulaField> expressions;
    const QDomNodeList list = elementsByTagName(TagPath);
    for (int i = 0; i < list.size(); ++i)
    {
        const QDomElement dom = list.at(i).toElement();
        if (dom.isNull())
        {
            continue;
        }

        expressions << ListNodesExpressions(dom.firstChildElement(TagNodes));
    }

    return expressions;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ListGrainlineExpressions(const QDomElement &element) const -> QVector<VFormulaField>
{
    QVector<VFormulaField> expressions;
    if (not element.isNull())
    {
        // Each tag can contains several attributes.
        ReadExpressionAttribute(expressions, element, AttrRotation);
        ReadExpressionAttribute(expressions, element, AttrLength);
    }

    return expressions;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ListPieceExpressions() const -> QVector<VFormulaField>
{
    // Check if new tool doesn't bring new attribute with a formula.
    // If no just increment number.
    // If new tool bring absolutely new type and has formula(s) create new method to cover it.
    Q_STATIC_ASSERT(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 73);

    QVector<VFormulaField> expressions;
    const QDomNodeList list = elementsByTagName(TagDetail);
    for (int i = 0; i < list.size(); ++i)
    {
        const QDomElement dom = list.at(i).toElement();
        if (dom.isNull())
        {
            continue;
        }

        // Each tag can contains several attributes.
        ReadExpressionAttribute(expressions, dom, AttrWidth);

        expressions << ListNodesExpressions(dom.firstChildElement(TagNodes));
        expressions << ListGrainlineExpressions(dom.firstChildElement(TagGrainline));
    }

    return expressions;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ListFinalMeasurementsExpressions() const -> QVector<VFormulaField>
{
    QVector<VFormulaField> expressions;
    const QDomNodeList list = elementsByTagName(TagFMeasurement);
    for (int i = 0; i < list.size(); ++i)
    {
        const QDomElement dom = list.at(i).toElement();
        if (dom.isNull())
        {
            continue;
        }

        // Each tag can contains several attributes.
        ReadExpressionAttribute(expressions, dom, AttrFormula);
    }

    return expressions;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::IsVariable(const QString &token) -> bool
{
    for (const auto &var : BuilInVariables())
    {
        if (token.indexOf(var) == 0)
        {
            if (var == currentLength || var == currentSeamAllowance)
            {
                return token == var;
            }

            return true;
        }
    }

    return token.startsWith('#'_L1);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::IsFunction(const QString &token) -> bool
{
    for (const auto &fn : BuilInFunctions())
    {
        if (token.indexOf(fn) == 0)
        {
            return true;
        }
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ParseItemElement(const QDomElement &domElement) -> QPair<bool, QMap<quint32, quint32>>
{
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        const bool visible = GetParametrBool(domElement, AttrVisible, trueStr);

        QMap<quint32, quint32> items;

        const QDomNodeList nodeList = domElement.childNodes();
        QDOM_LOOP(nodeList, i)
        {
            if (const QDomElement element = QDOM_ELEMENT(nodeList, i).toElement();
                !element.isNull() && element.tagName() == TagGroupItem)
            {
                const quint32 tool = GetParametrUInt(element, AttrTool, NULL_ID_STR);
                const quint32 object = GetParametrUInt(element, AttrObject, QString::number(tool));

                items.insert(object, tool);
            }
        }

        QPair<bool, QMap<quint32, quint32>> group;
        group.first = visible;
        group.second = items;

        return group;
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(QCoreApplication::translate("VAbstractPattern", "Error creating or updating group"),
                                    domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetFMeasurements(const QDomElement &element) const -> QVector<VFinalMeasurement>
{
    QVector<VFinalMeasurement> measurements;

    if (not element.isNull())
    {
        QDomElement tagFMeasurement = element.firstChildElement();
        while (not tagFMeasurement.isNull())
        {
            if (tagFMeasurement.tagName() == TagFMeasurement)
            {
                measurements.append(
                    {.name = GetParametrString(tagFMeasurement,
                                               AttrName,
                                               QCoreApplication::translate("VAbstractPattern", "measurement")),
                     .formula = GetParametrString(tagFMeasurement, AttrFormula, QChar('0')),
                     .description = GetParametrEmptyString(tagFMeasurement, AttrDescription)});
            }
            tagFMeasurement = tagFMeasurement.nextSiblingElement(TagFMeasurement);
        }
    }

    return measurements;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetFMeasurements(QDomElement &element, const QVector<VFinalMeasurement> &measurements)
{
    if (not element.isNull())
    {
        for (const auto &m : measurements)
        {
            QDomElement tagFMeasurement = createElement(TagFMeasurement);

            SetAttribute(tagFMeasurement, AttrName, m.name);
            SetAttribute(tagFMeasurement, AttrFormula, m.formula);
            SetAttributeOrRemoveIf<QString>(tagFMeasurement,
                                            AttrDescription,
                                            m.description,
                                            [](const QString &description) noexcept { return description.isEmpty(); });

            element.appendChild(tagFMeasurement);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetBackgroundPatternImage(const QDomElement &element) const -> VBackgroundPatternImage
{
    VBackgroundPatternImage image;
    image.SetId(QUuid(GetParametrEmptyString(element, AttrImageId)));

    if (QString const path = GetParametrEmptyString(element, AttrPath); not path.isEmpty())
    {
        image.SetFilePath(path);
    }
    else
    {
        QString const contentType = GetParametrEmptyString(element, AttrContentType);
        QByteArray const contentData = element.text().toLatin1();
        image.SetContentData(contentData, contentType);
    }

    image.SetName(GetParametrEmptyString(element, AttrName));
    image.SetHold(GetParametrBool(element, AttrHold, falseStr));
    image.SetZValue(GetParametrUInt(element, AttrZValue, QChar('0')));
    image.SetVisible(GetParametrBool(element, AttrVisible, trueStr));

    VValentinaSettings const *settings = VAbstractValApplication::VApp()->ValentinaSettings();
    image.SetOpacity(
        GetParametrDouble(element, AttrOpacity, QString::number(settings->GetBackgroundImageDefOpacity() / 100.)));

    QString const matrix = GetParametrEmptyString(element, AttrTransform);
    image.SetMatrix(StringToTransfrom(matrix));

    return image;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetBackgroundImageElement(const QUuid &id) const -> QDomElement
{
    if (const QDomNodeList list = elementsByTagName(TagBackgroundImages); not list.isEmpty())
    {
        if (QDomElement const imagesTag = list.at(0).toElement(); not imagesTag.isNull())
        {
            QDomNode imageNode = imagesTag.firstChild();
            while (not imageNode.isNull())
            {
                if (imageNode.isElement())
                {
                    if (const QDomElement imageElement = imageNode.toElement(); not imageElement.isNull())
                    {
                        auto const imageId = QUuid(GetParametrEmptyString(imageElement, AttrImageId));
                        if (imageId == id)
                        {
                            return imageElement;
                        }
                    }
                }
                imageNode = imageNode.nextSibling();
            }
        }
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::WriteBackgroundImage(QDomElement &element, const VBackgroundPatternImage &image)
{
    SetAttribute(element, AttrImageId, image.Id().toString());

    if (not image.FilePath().isEmpty())
    {
        SetAttribute(element, AttrPath, image.FilePath());
        element.removeAttribute(AttrContentType);
        setTagText(element, QString());
    }
    else
    {
        SetAttributeOrRemoveIf<QString>(element,
                                        AttrContentType,
                                        image.ContentType(),
                                        [](const QString &contentType) noexcept { return contentType.isEmpty(); });
        setTagText(element, image.ContentData());
        SetAttributeOrRemoveIf<QString>(element,
                                        AttrPath,
                                        image.FilePath(),
                                        [](const QString &path) noexcept { return path.isEmpty(); });
    }

    SetAttributeOrRemoveIf<QString>(element,
                                    AttrName,
                                    image.Name(),
                                    [](const QString &name) noexcept { return name.isEmpty(); });
    SetAttribute(element, AttrTransform, TransformToString(image.Matrix()));

    SetAttributeOrRemoveIf<bool>(element, AttrHold, image.Hold(), [](bool hold) noexcept { return not hold; });
    SetAttributeOrRemoveIf<qreal>(element, AttrZValue, image.ZValue(), [](qreal z) noexcept { return qFuzzyIsNull(z); });
    SetAttributeOrRemoveIf<bool>(element, AttrVisible, image.Visible(), [](bool visible) noexcept { return visible; });
    SetAttributeOrRemoveIf<qreal>(element,
                                  AttrOpacity,
                                  image.Opacity(),
                                  [](qreal o) noexcept { return VFuzzyComparePossibleNulls(o, 1); });
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetPatternVariable(const QString &attribute) const -> QString
{
    const QDomElement pattern = documentElement();

    if (pattern.isNull())
    {
        return {};
    }

    return GetParametrEmptyString(pattern, attribute);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SePatternVariable(const QString &attribute, const QString &name)
{
    if (QDomElement pattern = documentElement(); not pattern.isNull())
    {
        SetAttributeOrRemoveIf<QString>(pattern,
                                        attribute,
                                        name,
                                        [](const QString &name) noexcept { return name.isEmpty(); });
        modified = true;
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief IsModified state of the document for cases that do not cover QUndoStack.
 * @return true if the document was modified without using QUndoStack.
 */
auto VAbstractPattern::IsModified() const -> bool
{
    return modified;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetModified(bool modified)
{
    this->modified = modified;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::CreateGroups(const QString &patternPieceName) -> QDomElement
{
    QDomElement draw = patternPieceName.isEmpty() ? m_patternBlockMapper->GetActiveElement()
                                                  : m_patternBlockMapper->GetElement(patternPieceName);

    if (draw.isNull())
    {
        return {};
    }

    QDomElement groups = draw.firstChildElement(TagGroups);

    if (groups.isNull())
    {
        groups = createElement(TagGroups);
        draw.appendChild(groups);
    }

    return groups;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::CreateGroup(quint32 id,
                                   const QString &name,
                                   const QStringList &tags,
                                   const QMap<quint32, quint32> &groupData,
                                   vidtype tool,
                                   const QString &color,
                                   const QString &penStyle) -> QDomElement
{
    if (id == NULL_ID)
    {
        return QDomElement();
    }

    const QString preparedTags = PrepareGroupTags(tags);

    QDomElement group = createElement(TagGroup);
    SetAttribute(group, AttrId, id);
    SetAttribute(group, AttrName, name);
    SetAttribute(group, AttrVisible, true);
    // The layer's own pen, written from the start: it is what the tool dialogs will open on.
    SetAttribute(group, AttrColor, color);
    SetAttribute(group, AttrPenStyle, penStyle);
    SetAttributeOrRemoveIf<vidtype>(group, AttrTool, tool, [](vidtype tool) noexcept { return tool == null_id; });
    SetAttributeOrRemoveIf<QString>(group,
                                    AttrTags,
                                    preparedTags,
                                    [](const QString &preparedTags) noexcept { return preparedTags.isEmpty(); });

    auto i = groupData.constBegin();
    while (i != groupData.constEnd())
    {
        QDomElement item = createElement(TagGroupItem);
        item.setAttribute(AttrTool, i.value());
        SetAttributeOrRemoveIf<vidtype>(item,
                                        AttrObject,
                                        i.key(),
                                        [i](vidtype object) noexcept { return object == i.value(); });
        group.appendChild(item);
        ++i;
    }

    return group;
}
//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GroupLinkedToTool(vidtype toolId) const -> vidtype
{
    const QDomNodeList groups = elementsByTagName(TagGroup);
    for (int i = 0; i < groups.size(); ++i)
    {
        const QDomElement group = groups.at(i).toElement();
        if (not group.isNull() && group.hasAttribute(AttrTool))
        {
            const quint32 id = GetParametrUInt(group, AttrTool, NULL_ID_STR);

            if (toolId == id)
            {
                return GetParametrUInt(group, AttrId, NULL_ID_STR);
            }
        }
    }

    return null_id;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetGroupName(quint32 id) -> QString
{
    QString name = QCoreApplication::translate("VAbstractPattern", "New layer");
    if (QDomElement const group = FindElementById(id, TagGroup); group.isElement())
    {
        name = GetParametrString(group, AttrName, name);
    }

    return name;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetGroupName(quint32 id, const QString &name)
{
    if (QDomElement group = FindElementById(id, TagGroup); group.isElement())
    {
        group.setAttribute(AttrName, name);
        modified = true;
        emit patternChanged(false);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetGroupColor(quint32 id) -> QString
{
    QString color = ColorBlack;
    if (QDomElement const group = FindElementById(id, TagGroup); group.isElement())
    {
        color = GetParametrString(group, AttrColor, ColorBlack);
    }

    return color;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetGroupPenStyle(quint32 id) -> QString
{
    QString penStyle = TypeLineLine;
    if (QDomElement const group = FindElementById(id, TagGroup); group.isElement())
    {
        penStyle = GetParametrString(group, AttrPenStyle, TypeLineLine);
    }

    return penStyle;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetGroupPen(quint32 id, const QString &color, const QString &penStyle)
{
    if (QDomElement group = FindElementById(id, TagGroup); group.isElement())
    {
        SetAttribute(group, AttrColor, color);
        SetAttribute(group, AttrPenStyle, penStyle);
        modified = true;
        emit patternChanged(false);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetGroupVisibility(quint32 id, bool visible)
{
    QDomElement group = FindElementById(id, TagGroup);
    if (group.isNull())
    {
        return;
    }

    SetAttribute(group, AttrVisible, visible);
    modified = true;
    emit patternChanged(false);

    if (QDomElement const groups = CreateGroups(); not groups.isNull())
    {
        ParseGroups(groups); // the drawing follows the layer
    }

    emit UpdateGroups();
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetActiveGroup(const QString &patternPieceName) -> quint32
{
    const QMap<quint32, VGroupData> groups = GetGroups(patternPieceName);
    for (auto it = groups.constBegin(); it != groups.constEnd(); ++it)
    {
        if (it.value().active)
        {
            return it.key();
        }
    }

    return NULL_ID;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetActiveGroup(quint32 id, const QString &patternPieceName)
{
    const QDomElement groups = CreateGroups(patternPieceName);
    if (groups.isNull())
    {
        return;
    }

    // The flag is a marker, not a value: only the active layer carries it. Removing the attribute from the other
    // layers is what keeps exactly one of them active — and a deleted layer takes its own flag with it.
    bool found = false;
    bool shown = false;
    for (QDomElement group = groups.firstChildElement(TagGroup); !group.isNull();
         group = group.nextSiblingElement(TagGroup))
    {
        if (GetParametrUInt(group, AttrId, NULL_ID_STR) == id)
        {
            SetAttribute(group, AttrActive, true);
            // The layer the canvas draws into is shown: an active but hidden layer would take every new object
            // and show none of them, which reads as a broken application.
            if (not GetParametrBool(group, AttrVisible, trueStr))
            {
                SetAttribute(group, AttrVisible, true);
                shown = true;
            }
            found = true;
        }
        else
        {
            group.removeAttribute(AttrActive);
        }
    }

    if (not found)
    {
        return;
    }

    modified = true;
    emit patternChanged(false);

    if (shown)
    {
        ParseGroups(groups); // the drawing has to follow the layer being shown
    }

    emit UpdateGroups();
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetGroupTags(vidtype id) -> QStringList
{
    QStringList tags;
    if (QDomElement const group = FindElementById(id, TagGroup); group.isElement())
    {
        tags = FilterGroupTags(GetParametrEmptyString(group, AttrTags));
    }

    return tags;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetGroupTags(quint32 id, const QStringList &tags)
{
    if (QDomElement group = FindElementById(id, TagGroup); group.isElement())
    {
        SetAttributeOrRemoveIf<QString>(group,
                                        AttrTags,
                                        tags.join(','_L1),
                                        [](const QString &rawTags) noexcept { return rawTags.isEmpty(); });
        modified = true;
        emit patternChanged(false);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetDimensionAValue() -> double
{
    if (QDomElement const domElement = UniqueTag(TagMeasurements); not domElement.isNull())
    {
        return GetParametrDouble(domElement, AttrDimensionA, *dimensionDefValue);
    }

    qDebug() << "Can't save dimension A of measurements" << Q_FUNC_INFO;
    return -1;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetDimensionAValue(double value)
{
    if (QDomElement domElement = UniqueTag(TagMeasurements); not domElement.isNull())
    {
        SetAttribute(domElement, AttrDimensionA, value);
        modified = true;
        emit patternChanged(false);
    }
    else
    {
        qDebug() << "Can't save dimension A of measurements" << Q_FUNC_INFO;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetDimensionBValue() -> double
{
    if (QDomElement const domElement = UniqueTag(TagMeasurements); not domElement.isNull())
    {
        return GetParametrDouble(domElement, AttrDimensionB, *dimensionDefValue);
    }

    qDebug() << "Can't save dimension B of measurements" << Q_FUNC_INFO;
    return -1;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetDimensionBValue(double value)
{
    if (QDomElement domElement = UniqueTag(TagMeasurements); not domElement.isNull())
    {
        SetAttribute(domElement, AttrDimensionB, value);
        modified = true;
        emit patternChanged(false);
    }
    else
    {
        qDebug() << "Can't save dimension B of measurements" << Q_FUNC_INFO;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetDimensionCValue() -> double
{
    if (QDomElement const domElement = UniqueTag(TagMeasurements); not domElement.isNull())
    {
        return GetParametrDouble(domElement, AttrDimensionC, *dimensionDefValue);
    }

    qDebug() << "Can't save dimension C of measurements" << Q_FUNC_INFO;
    return -1;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetDimensionCValue(double value)
{
    if (QDomElement domElement = UniqueTag(TagMeasurements); not domElement.isNull())
    {
        SetAttribute(domElement, AttrDimensionC, value);
        modified = true;
        emit patternChanged(false);
    }
    else
    {
        qDebug() << "Can't save dimension C of measurements" << Q_FUNC_INFO;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetGroupCategories() const -> QStringList
{
    QSet<QString> categories;

    const QDomNodeList groups = elementsByTagName(TagGroup);
    for (int i = 0; i < groups.size(); ++i)
    {
        const QDomElement group = groups.at(i).toElement();
        if (not group.isNull() && group.hasAttribute(AttrTags))
        {
            QStringList const groupTags = VAbstractPattern::FilterGroupTags(GetParametrEmptyString(group, AttrTags));
            categories.unite(ConvertToSet<QString>(groupTags));
        }
    }

    return ConvertToList(categories);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetGroups(const QString &patternPieceName) -> QMap<quint32, VGroupData>
{
    QMap<quint32, VGroupData> data;

    try
    {
        if (QDomElement const groups = CreateGroups(patternPieceName); not groups.isNull())
        {
            QVector<QPair<quint32, quint32>> items;

            QDomNode domNode = groups.firstChild();
            while (not domNode.isNull())
            {
                if (domNode.isElement())
                {
                    if (const QDomElement group = domNode.toElement(); not group.isNull() && group.tagName() == TagGroup)
                    {
                        items.resize(0);

                        const QDomNodeList nodeList = group.childNodes();
                        items.reserve(nodeList.size());
                        QDOM_LOOP(nodeList, i)
                        {
                            if (const QDomElement element = QDOM_ELEMENT(nodeList, i).toElement();
                                !element.isNull() && element.tagName() == TagGroupItem)
                            {
                                const quint32 tool = GetParametrUInt(element, AttrTool, NULL_ID_STR);
                                const quint32 object = GetParametrUInt(element, AttrObject, QString::number(tool));

                                items.append(QPair<quint32, quint32>(object, tool));
                            }
                        }

                        const quint32 id = GetParametrUInt(group, AttrId, QChar('0'));
                        data.insert(id,
                                    {.name = GetParametrString(group,
                                                               AttrName,
                                                               QCoreApplication::translate("VAbstractPattern",
                                                                                           "New layer")),
                                     .visible = GetParametrBool(group, AttrVisible, trueStr),
                                     .tags = FilterGroupTags(GetParametrEmptyString(group, AttrTags)),
                                     .tool = GetParametrUInt(group, AttrTool, NULL_ID_STR),
                                     .items = items,
                                     // A file written before layers carried a pen reads as the default one:
                                     // solid black. Nothing is written back until the document is saved.
                                     .color = GetParametrString(group, AttrColor, ColorBlack),
                                     .penStyle = GetParametrString(group, AttrPenStyle, TypeLineLine),
                                     .active = GetParametrBool(group, AttrActive, falseStr)});
                    }
                }
                domNode = domNode.nextSibling();
            }
        }
        else
        {
            qDebug("Can't get tag Groups.");
        }
    }
    catch (const VExceptionConversionError &)
    {
        return {};
    }

    return data;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Returns the groups that contain or do not contain the item identified by the toolid and the objectid
 * @param toolId tool id
 * @param objectId object id
 * @param containItem |true if the groups have to contain the given item, false if they musn't contain the item
 * @return groups that contain or do not contain the item identified by the toolid and the objectid
 */
auto VAbstractPattern::GetGroupsContainingItem(quint32 toolId, quint32 objectId, bool containItem)
    -> QMap<quint32, QString>
{
    QMap<quint32, QString> data;

    if (objectId == 0)
    {
        objectId = toolId;
    }

    // TODO : order in alphabetical order
    if (QDomElement const groups = CreateGroups(); not groups.isNull())
    {
        QDomNode domNode = groups.firstChild();
        while (domNode.isNull() == false) // iterate through the groups
        {
            if (domNode.isElement())
            {
                if (const QDomElement group = domNode.toElement();
                    group.isNull() == false && group.tagName() == TagGroup)
                {
                    bool const groupHasItem = GroupHasItem(group, toolId, objectId);
                    if ((containItem && groupHasItem) || (not containItem && not groupHasItem))
                    {
                        const quint32 groupId = GetParametrUInt(group, AttrId, QChar('0'));
                        const QString name = GetParametrString(group,
                                                               AttrName,
                                                               QCoreApplication::translate("VAbstractPattern",
                                                                                           "New layer"));
                        data.insert(groupId, name);
                    }
                }
            }
            domNode = domNode.nextSibling();
        }
    }
    else
    {
        qDebug("Can't get tag Groups.");
    }

    return data;
}
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Checks if the given group has the item with the given toolId and objectId
 * @param groupDomElement group dom element
 * @param toolId tool id
 * @param objectId object id
 * @return true if the given group has the item with the given toolId and objectId
 */
auto VAbstractPattern::GroupHasItem(const QDomElement &groupDomElement, quint32 toolId, quint32 objectId) -> bool
{
    bool result = false;

    QDomNode itemNode = groupDomElement.firstChild();
    while (itemNode.isNull() == false) // iterate through the items of the group
    {
        if (itemNode.isElement())
        {
            const QDomElement item = itemNode.toElement();
            if (item.isNull() == false)
            {
                quint32 const toolIdIterate = GetParametrUInt(item, AttrTool, QChar('0'));
                quint32 const objectIdIterate = GetParametrUInt(item, AttrObject, QString::number(toolIdIterate));

                if (toolIdIterate == toolId && objectIdIterate == objectId)
                {
                    result = true;
                    break;
                }
            }
        }
        itemNode = itemNode.nextSibling();
    }
    return result;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::SaveSchema() const -> QString
{
    return QStringLiteral("://schema/ostara/v1.0.0.xsd");
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ReadUnits() const -> Unit
{
    Unit units = StrToUnits(UniqueTagText(TagUnit, unitCM));

    if (units == Unit::Px)
    {
        units = Unit::Cm;
    }

    return units;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ReadPatternNumber() const -> QString
{
    return UniqueTagText(TagPatternNum);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ReadPatternName() const -> QString
{
    return UniqueTagText(TagPatternName);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ReadMPath() const -> QString
{
    if (QDomElement const domElement = UniqueTag(TagMeasurements); not domElement.isNull())
    {
        return domElement.attribute(AttrPath);
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ReadWatermarkPath() const -> QString
{
    return UniqueTagText(TagWatermark);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::ReadCompanyName() const -> QString
{
    return UniqueTagText(TagCompanyName);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::CancelFormulaDependencyChecks()
{
    ++m_dependencyCheckGeneration;

    QList<QFutureWatcher<void> *> watchersCopy;

    {
        QMutexLocker const locker(&m_watchersMutex);

        if (m_formulaDependenciesWatchers.isEmpty())
        {
            return;
        }

        watchersCopy.reserve(m_formulaDependenciesWatchers.size());
        watchersCopy = m_formulaDependenciesWatchers;
        m_formulaDependenciesWatchers.clear();

        // Disconnect to prevent cleanup from running
        for (auto *watcher : std::as_const(watchersCopy))
        {
            disconnect(watcher, &QFutureWatcher<void>::finished, this, &VAbstractPattern::CleanDependenciesWatcher);
        }
    }

    for (auto *watcher : std::as_const(watchersCopy))
    {
        watcher->cancel();
    }

    for (auto *watcher : std::as_const(watchersCopy))
    {
        watcher->waitForFinished();
        watcher->deleteLater(); // Manual cleanup since we disconnected
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::CleanDependenciesWatcher()
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    auto *watcher = static_cast<QFutureWatcher<void> *>(sender());
    QMutexLocker const locker(&m_watchersMutex);
    m_formulaDependenciesWatchers.removeOne(watcher);
    watcher->deleteLater();

    if (m_formulaDependenciesWatchers.isEmpty() && m_fileParsingCompleted)
    {
        emit PatternDependencyGraphCompleted();
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Adds an item to the given group with the given toolId and objectId
 * @param toolId tool id
 * @param objectId object id
 * @param groupId group id
 * @return group element
 */
auto VAbstractPattern::AddItemToGroup(quint32 toolId, quint32 objectId, quint32 groupId) -> QDomElement
{
    if (QDomElement group = FindElementById(groupId, TagGroup); not group.isNull())
    {
        if (objectId == 0)
        {
            objectId = toolId;
        }

        // A layer holds an object once. Both callers add a whole figure at a time (a rectangle is three points,
        // an operation everything it produced), so part of what they bring is often already there — and writing
        // the same <item> twice is not harmless: the removal workflow counts a layer's content literally, and a
        // duplicate would read as "another layer holds it", making an object removable that is not.
        if (GroupHasItem(group, toolId, objectId))
        {
            return QDomElement();
        }

        QDomElement item = createElement(TagGroupItem);
        item.setAttribute(AttrTool, toolId);
        SetAttributeOrRemoveIf<vidtype>(item,
                                        AttrObject,
                                        objectId,
                                        [toolId](vidtype object) noexcept -> bool { return object == toolId; });
        group.appendChild(item);

        // to signalised that the pattern was changed and need to be saved
        modified = true;
        emit patternChanged(false);

        // to update the group table of the gui
        emit UpdateGroups();

        // parse the groups to update the drawing, in case the item was added to an invisible group
        if (QDomElement const groups = CreateGroups(); not groups.isNull())
        {
            ParseGroups(groups);
        }

        return item;
    }

    qDebug() << "The group of id " << groupId << " doesn't exist";
    return QDomElement();
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::AddItemsToGroup(const QVector<QPair<quint32, quint32>> &items, quint32 groupId) -> bool
{
    QDomElement group = FindElementById(groupId, TagGroup);
    if (group.isNull() || items.isEmpty())
    {
        return false;
    }

    for (const auto &[objectId, toolId] : items)
    {
        // Same invariant as AddItemToGroup: a layer holds an object once. A batch that runs twice — a creation
        // replayed from the undo stack, a tool attached again — must not leave the same <item> in the layer
        // twice, or the removal workflow would count it as held by two layers.
        if (GroupHasItem(group, toolId, objectId))
        {
            continue;
        }

        QDomElement item = createElement(TagGroupItem);
        item.setAttribute(AttrTool, toolId);
        SetAttributeOrRemoveIf<vidtype>(item,
                                        AttrObject,
                                        objectId,
                                        [toolId](vidtype object) noexcept -> bool { return object == toolId; });
        group.appendChild(item);
    }

    // Once for the whole batch: a tool can create many objects, and refreshing per object would re-parse every
    // layer of the piece as many times.
    modified = true;
    emit patternChanged(false);
    emit UpdateGroups();

    if (QDomElement const groups = CreateGroups(); not groups.isNull())
    {
        ParseGroups(groups);
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::LayerableObjectsOfTool(const QDomElement &toolElement, quint32 toolId)
    -> QVector<QPair<quint32, quint32>>
{
    QVector<QPair<quint32, quint32>> objects;
    if (toolElement.isNull() || toolId == NULL_ID)
    {
        return objects;
    }

    const QString tag = toolElement.tagName();
    if (tag == TagPoint)
    {
        if (not(toolElement.hasAttribute(AttrPoint1) && toolElement.hasAttribute(AttrPoint2)))
        {
            objects.append({toolId, toolId});
            return objects;
        }

        // A double point — trueDarts, rectangle. Its points live as attributes of the same <point>, and the base
        // class answers to those two ids only. The <spline> family writes point1/point4 too, but on its own tag,
        // which is why the tag is read before the attributes.
        for (const QString &attribute : {AttrPoint1, AttrPoint2, AttrPoint4})
        {
            if (not toolElement.hasAttribute(attribute))
            {
                continue;
            }
            if (const quint32 pointId = toolElement.attribute(attribute).toUInt(); pointId != NULL_ID)
            {
                objects.append({pointId, toolId});
            }
        }
    }
    else if (tag == TagLine || tag == TagArc || tag == TagElArc || tag == TagSpline)
    {
        objects.append({toolId, toolId});
    }
    else if (tag == TagOperation)
    {
        // The tags below live on VAbstractOperation and OOperationLines; written verbatim because libs/ifc does
        // not depend on libs/vtools. <source> holds the inputs — the objects CREATED are the destinations.
        const QDomElement destination = toolElement.firstChildElement(QStringLiteral("destination"));
        for (QDomElement item = destination.firstChildElement(QStringLiteral("item")); !item.isNull();
             item = item.nextSiblingElement(QStringLiteral("item")))
        {
            if (const quint32 objectId = item.attribute(AttrIdObject).toUInt(); objectId != NULL_ID)
            {
                objects.append({objectId, toolId});
            }
        }

        const QDomElement lines = toolElement.firstChildElement(QStringLiteral("lines"));
        for (QDomElement item = lines.firstChildElement(QStringLiteral("item")); !item.isNull();
             item = item.nextSiblingElement(QStringLiteral("item")))
        {
            if (const quint32 resultId = item.attribute(QStringLiteral("idResult")).toUInt();
                resultId != NULL_ID)
            {
                objects.append({resultId, resultId});
            }
        }
    }

    return objects;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetActiveGroupColor() -> QString
{
    const quint32 id = GetActiveGroup();
    return id == NULL_ID ? ColorBlack : GetGroupColor(id);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetActiveGroupPenStyle() -> QString
{
    const quint32 id = GetActiveGroup();
    return id == NULL_ID ? TypeLineLine : GetGroupPenStyle(id);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Removes the item of given toolId and objectId from the group of given groupId
 * @param toolId tool id
 * @param objectId object id
 * @param groupId group id
 * @return item element or null element is none
 */
auto VAbstractPattern::RemoveItemFromGroup(quint32 toolId, quint32 objectId, quint32 groupId) -> QDomElement
{
    if (QDomElement group = FindElementById(groupId, TagGroup); not group.isNull())
    {
        if (objectId == 0)
        {
            objectId = toolId;
        }

        QDomNode itemNode = group.firstChild();
        while (itemNode.isNull() == false) // iterate through the items of the group
        {
            if (itemNode.isElement())
            {
                const QDomElement item = itemNode.toElement();
                if (not item.isNull())
                {
                    quint32 const toolIdIterate = GetParametrUInt(item, AttrTool, QChar('0'));
                    quint32 const objectIdIterate = GetParametrUInt(item, AttrObject, QString::number(toolIdIterate));

                    if (toolIdIterate == toolId && objectIdIterate == objectId)
                    {
                        group.removeChild(itemNode);

                        // to signalised that the pattern was changed and need to be saved
                        modified = true;
                        emit patternChanged(false);

                        // to update the group table of the gui
                        emit UpdateGroups();

                        // parse the groups to update the drawing, in case the item was removed from an invisible group
                        if (QDomElement const groups = CreateGroups(); not groups.isNull())
                        {
                            ParseGroups(groups);
                        }

                        return item;
                    }
                }
            }
            itemNode = itemNode.nextSibling();
        }
    }
    else
    {
        qDebug() << "The group of id " << groupId << " doesn't exist";
    }

    return QDomElement();
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetGroupVisibility(quint32 id) -> bool
{
    if (QDomElement const group = FindElementById(id, TagGroup); group.isElement())
    {
        return GetParametrBool(group, AttrVisible, trueStr);
    }
    qDebug("Can't get group by id = %u.", id);
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::ParseAttachments(const QDomElement &domElement)
{
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    QDomNode domNode = domElement.firstChild();
    while (not domNode.isNull())
    {
        if (domNode.isElement())
        {
            if (const QDomElement attachmentElement = domNode.toElement();
                not attachmentElement.isNull() && attachmentElement.tagName() == TagAttachment)
            {
                VContainer::UpdateId(GetParametrUInt(attachmentElement, AttrId, NULL_ID_STR), valentinaNamespace);
            }
        }
        domNode = domNode.nextSibling();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::CreateAttachments(const QString &patternPieceName) -> QDomElement
{
    QDomElement draw = patternPieceName.isEmpty() ? m_patternBlockMapper->GetActiveElement()
                                                  : m_patternBlockMapper->GetElement(patternPieceName);

    if (draw.isNull())
    {
        return {};
    }

    QDomElement attachments = draw.firstChildElement(TagAttachments);

    if (attachments.isNull())
    {
        attachments = createElement(TagAttachments);
        draw.appendChild(attachments);
    }

    return attachments;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::CreateAttachment(quint32 id, const QString &name, const QMap<quint32, quint32> &attachmentData)
    -> QDomElement
{
    if (id == NULL_ID)
    {
        return QDomElement();
    }

    QDomElement attachment = createElement(TagAttachment);
    SetAttribute(attachment, AttrId, id);
    SetAttributeOrRemoveIf<QString>(attachment,
                                    AttrName,
                                    name,
                                    [](const QString &n) noexcept { return n.isEmpty(); });

    auto i = attachmentData.constBegin();
    while (i != attachmentData.constEnd())
    {
        QDomElement item = createElement(TagAttachmentItem);
        item.setAttribute(AttrTool, i.value());
        SetAttributeOrRemoveIf<vidtype>(item,
                                        AttrObject,
                                        i.key(),
                                        [i](vidtype object) noexcept { return object == i.value(); });
        attachment.appendChild(item);
        ++i;
    }

    return attachment;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetAttachmentName(quint32 id) -> QString
{
    QString name = QCoreApplication::translate("VAbstractPattern", "New group");
    if (QDomElement const attachment = FindElementById(id, TagAttachment); attachment.isElement())
    {
        name = GetParametrString(attachment, AttrName, name);
    }

    return name;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetAttachmentName(quint32 id, const QString &name)
{
    if (QDomElement attachment = FindElementById(id, TagAttachment); attachment.isElement())
    {
        SetAttributeOrRemoveIf<QString>(attachment,
                                        AttrName,
                                        name,
                                        [](const QString &n) noexcept { return n.isEmpty(); });
        modified = true;
        emit patternChanged(false);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetAttachments(const QString &patternPieceName) -> QMap<quint32, VAttachmentData>
{
    QMap<quint32, VAttachmentData> data;

    try
    {
        if (QDomElement const attachments = CreateAttachments(patternPieceName); not attachments.isNull())
        {
            QVector<QPair<quint32, quint32>> items;

            QDomNode domNode = attachments.firstChild();
            while (not domNode.isNull())
            {
                if (domNode.isElement())
                {
                    if (const QDomElement attachment = domNode.toElement();
                        not attachment.isNull() && attachment.tagName() == TagAttachment)
                    {
                        items.resize(0);

                        const QDomNodeList nodeList = attachment.childNodes();
                        items.reserve(nodeList.size());
                        QDOM_LOOP(nodeList, i)
                        {
                            if (const QDomElement element = QDOM_ELEMENT(nodeList, i).toElement();
                                !element.isNull() && element.tagName() == TagAttachmentItem)
                            {
                                const quint32 tool = GetParametrUInt(element, AttrTool, NULL_ID_STR);
                                const quint32 object = GetParametrUInt(element, AttrObject, QString::number(tool));

                                items.append(QPair<quint32, quint32>(object, tool));
                            }
                        }

                        const quint32 id = GetParametrUInt(attachment, AttrId, QChar('0'));
                        data.insert(id,
                                    {.name = GetParametrString(attachment,
                                                               AttrName,
                                                               QCoreApplication::translate("VAbstractPattern",
                                                                                           "New group")),
                                     .items = items});
                    }
                }
                domNode = domNode.nextSibling();
            }
        }
        else
        {
            qDebug("Can't get tag Attachments.");
        }
    }
    catch (const VExceptionConversionError &)
    {
        return {};
    }

    return data;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetAttachmentsContainingItem(quint32 toolId, quint32 objectId, bool containItem)
    -> QMap<quint32, QString>
{
    QMap<quint32, QString> data;

    if (objectId == 0)
    {
        objectId = toolId;
    }

    const QDomElement draw = m_patternBlockMapper->GetActiveElement();
    if (draw.isNull())
    {
        return data;
    }

    const QDomElement attachments = draw.firstChildElement(TagAttachments);
    if (not attachments.isNull())
    {
        QDomNode domNode = attachments.firstChild();
        while (domNode.isNull() == false)
        {
            if (domNode.isElement())
            {
                if (const QDomElement attachment = domNode.toElement();
                    attachment.isNull() == false && attachment.tagName() == TagAttachment)
                {
                    bool const attachmentHasItem = AttachmentHasItem(attachment, toolId, objectId);
                    if ((containItem && attachmentHasItem) || (not containItem && not attachmentHasItem))
                    {
                        const quint32 attachmentId = GetParametrUInt(attachment, AttrId, QChar('0'));
                        const QString name = GetParametrString(attachment,
                                                               AttrName,
                                                               QCoreApplication::translate("VAbstractPattern",
                                                                                           "New group"));
                        data.insert(attachmentId, name);
                    }
                }
            }
            domNode = domNode.nextSibling();
        }
    }

    return data;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetAttachmentItemsForObject(quint32 objectId) -> QVector<QPair<quint32, quint32>>
{
    const QDomElement draw = m_patternBlockMapper->GetActiveElement();
    if (draw.isNull())
    {
        return {};
    }

    const QDomElement attachments = draw.firstChildElement(TagAttachments);
    if (attachments.isNull())
    {
        return {};
    }

    QDomNode domNode = attachments.firstChild();
    while (not domNode.isNull())
    {
        if (domNode.isElement())
        {
            const QDomElement attachment = domNode.toElement();
            if (not attachment.isNull() && attachment.tagName() == TagAttachment)
            {
                QVector<QPair<quint32, quint32>> items;

                const QDomNodeList nodeList = attachment.childNodes();
                items.reserve(nodeList.size());
                QDOM_LOOP(nodeList, i)
                {
                    const QDomElement element = QDOM_ELEMENT(nodeList, i).toElement();
                    if (!element.isNull() && element.tagName() == TagAttachmentItem)
                    {
                        const quint32 tool = GetParametrUInt(element, AttrTool, NULL_ID_STR);
                        const quint32 object = GetParametrUInt(element, AttrObject, QString::number(tool));
                        items.append(QPair<quint32, quint32>(object, tool));
                    }
                }

                for (const auto &item : items)
                {
                    if (item.first == objectId)
                    {
                        return items;
                    }
                }
            }
        }
        domNode = domNode.nextSibling();
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::AddItemToAttachment(quint32 toolId, quint32 objectId, quint32 attachmentId) -> QDomElement
{
    if (QDomElement attachment = FindElementById(attachmentId, TagAttachment); not attachment.isNull())
    {
        if (objectId == 0)
        {
            objectId = toolId;
        }

        QDomElement item = createElement(TagAttachmentItem);
        item.setAttribute(AttrTool, toolId);
        SetAttributeOrRemoveIf<vidtype>(item,
                                        AttrObject,
                                        objectId,
                                        [toolId](vidtype object) noexcept -> bool { return object == toolId; });
        attachment.appendChild(item);

        modified = true;
        emit patternChanged(false);
        emit UpdateAttachments();

        return item;
    }

    qDebug() << "The attachment of id " << attachmentId << " doesn't exist";
    return QDomElement();
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::RemoveItemFromAttachment(quint32 toolId, quint32 objectId, quint32 attachmentId) -> QDomElement
{
    if (QDomElement attachment = FindElementById(attachmentId, TagAttachment); not attachment.isNull())
    {
        if (objectId == 0)
        {
            objectId = toolId;
        }

        QDomNode itemNode = attachment.firstChild();
        while (itemNode.isNull() == false)
        {
            if (itemNode.isElement())
            {
                const QDomElement item = itemNode.toElement();
                if (not item.isNull())
                {
                    quint32 const toolIdIterate = GetParametrUInt(item, AttrTool, QChar('0'));
                    quint32 const objectIdIterate = GetParametrUInt(item, AttrObject, QString::number(toolIdIterate));

                    if (toolIdIterate == toolId && objectIdIterate == objectId)
                    {
                        attachment.removeChild(itemNode);

                        modified = true;
                        emit patternChanged(false);
                        emit UpdateAttachments();

                        return item;
                    }
                }
            }
            itemNode = itemNode.nextSibling();
        }
    }

    qDebug() << "The attachment of id " << attachmentId << " doesn't exist";
    return QDomElement();
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::AttachmentHasItem(const QDomElement &attachmentDomElement, quint32 toolId, quint32 objectId)
    -> bool
{
    bool result = false;

    QDomNode itemNode = attachmentDomElement.firstChild();
    while (itemNode.isNull() == false)
    {
        if (itemNode.isElement())
        {
            const QDomElement item = itemNode.toElement();
            if (item.isNull() == false)
            {
                quint32 const toolIdIterate = GetParametrUInt(item, AttrTool, QChar('0'));
                quint32 const objectIdIterate = GetParametrUInt(item, AttrObject, QString::number(toolIdIterate));

                if (toolIdIterate == toolId && objectIdIterate == objectId)
                {
                    result = true;
                    break;
                }
            }
        }
        itemNode = itemNode.nextSibling();
    }
    return result;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetIsolatedAttachment() const -> quint32
{
    return m_isolatedAttachmentId;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetIsolatedAttachment(quint32 attachmentId)
{
    if (m_isolatedAttachmentId != attachmentId || m_isolatedObjectId != NULL_ID)
    {
        m_isolatedAttachmentId = attachmentId;
        m_isolatedObjectId = NULL_ID; // Entering a group clears the drilled-down object
        emit IsolationChanged();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetIsolatedObject() const -> quint32
{
    return m_isolatedObjectId;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::SetIsolatedObject(quint32 objectId)
{
    if (m_isolatedObjectId != objectId)
    {
        m_isolatedObjectId = objectId;
        emit IsolationChanged();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractPattern::ClearIsolation()
{
    if (m_isolatedAttachmentId != NULL_ID || m_isolatedObjectId != NULL_ID)
    {
        m_isolatedAttachmentId = NULL_ID;
        m_isolatedObjectId = NULL_ID;
        emit IsolationChanged();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::IsIsolated() const -> bool
{
    return m_isolatedObjectId != NULL_ID;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::PieceDrawName(quint32 id) -> QString
{
    const QDomElement detail = FindElementById(id, VAbstractPattern::TagDetail);
    if (detail.isNull())
    {
        return QString();
    }

    const QDomElement draw = detail.parentNode().parentNode().toElement();
    if (draw.isNull() || not draw.hasAttribute(VAbstractPattern::AttrName))
    {
        return QString();
    }

    return draw.attribute(VAbstractPattern::AttrName);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetCompleteData() const -> VContainer
{
    return VContainer(nullptr, nullptr, VContainer::UniqueNamespace());
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::GetCompletePPData(const QString &name) const -> VContainer
{
    Q_UNUSED(name)
    return VContainer(nullptr, nullptr, VContainer::UniqueNamespace());
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::Units() const -> Unit
{
    return m_units;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractPattern::FilterGroupTags(const QString &tags) -> QStringList
{
    if (tags.isEmpty())
    {
        return QStringList();
    }

    QStringList list = tags.split(',');
    for (auto &tag : list)
    {
        tag = tag.simplified();
    }

    list.removeAll("");

    return ConvertToList(ConvertToSet<QString>(list));
}
