/************************************************************************
 **
 **  @file   vtoppb_ops.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  ops property browser methods — declarative descriptors
 **
 **  @note   Extracted from vtooloptionspropertybrowser.cpp by Sophie Denys <contact@sophiedenys.com>
 **          as part of Ostara refactoring.
 **
 **  @copyright
 **  Copyright (C) 2013-2015 Valentina project
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

#include "vtooloptionspropertybrowser.h"
#include "vtoppb_templates.h"
#include "../vtools/tools/drawTools/operation/otoolclosedart.h"
#include "../vtools/tools/drawTools/operation/otoolmovebatch.h"
#include "../vtools/tools/drawTools/operation/otoolpleat.h"
#include "../vtools/tools/drawTools/operation/otoolspread.h"
#include "../vtools/tools/drawTools/operation/vtoolrotation.h"
#include "../vtools/tools/drawTools/operation/otooltransfert.h"
#include "../vtools/tools/drawTools/operation/vtoolmove.h"
#include "../vtools/tools/drawTools/operation/flipping/vtoolflippingbyline.h"
#include "../vtools/tools/drawTools/operation/flipping/vtoolflippingbyaxis.h"
#include "../vpatterndb/vformula.h"
#include "vformulaproperty.h"
#include "ocheckboxrowproperty.h"
#include "ocheckboxpairrowproperty.h"
#include "ogroupboxproperty.h"
#include "oformularowproperty.h"
#include "../vpropertyexplorer/plugins/vlabelproperty.h"
#include "../vpropertyexplorer/plugins/venumproperty.h"
#include "../vpropertyexplorer/plugins/vtextproperty.h"
#include "../vpropertyexplorer/plugins/vwidgetproperty.h"
#include "../vpropertyexplorer/plugins/vcolorpropertyeditor.h"
#include "../vtheme/vscenestylesheet.h"
#include "ifcdef.h"
#include "../vmisc/def.h"

#include <QPushButton>

namespace
{
auto T(const char *s) -> QString
{
    return VToolOptionsPropertyBrowser::tr(s);
}
} // anonymous namespace

// Forward declarations
auto RotationProperties() -> std::vector<PropertyDesc>;
auto CloseDartProperties() -> std::vector<PropertyDesc>;
auto MoveProperties() -> std::vector<PropertyDesc>;
auto TransfertProperties() -> std::vector<PropertyDesc>;
auto FlippingByLineProperties() -> std::vector<PropertyDesc>;
auto FlippingByAxisProperties() -> std::vector<PropertyDesc>;

//---------------------------------------------------------------------------------------------------------------------
/** @brief The "Notes:" descriptor every tool of this file ends its list with.
 *
 *  The three callbacks only differ by the concrete tool type, which is the one thing the caller knows. */
template <class Tool> auto NotesProperty() -> PropertyDesc
{
    return {AttrNotes, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:")),
            [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
            {
                auto *t = static_cast<Tool *>(tool);
                auto *prop = new VPE::VTextProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:")));
                prop->setValue(t->GetNotes());
                return prop;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                {
                    QString const text = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                    if (text != t->GetNotes())
                        t->SetNotes(text);
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                    prop->setValue(t->GetNotes());
            }};
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The pen and the colour of ONE copy — the object the click landed on inside the operation's figure.
 *
 *  The operation's options describe the whole set; a copy has no options of its own, but it does carry its own
 *  pen. A copied curve keeps it on the `<source>` item it comes from, a copied line on its `<lines>` entry, and
 *  `VAbstractOperation::CopyStyleOf` is the one place that says which — and whether there is one at all. The
 *  group is therefore built only when the click landed on such a copy: a copied point carries no pen. Nothing is
 *  offered to rename it either, its name belonging to the operation that produced it.
 *
 *  Its title is the copy's own name, so the group answers the question the whole panel leaves open: which of the
 *  overlapping objects of a figure is being styled. */
//---------------------------------------------------------------------------------------------------------------------
/** @brief What the group of one copy is called: its KIND, never its name.
 *
 *  Ostara is worked without the objects' names on screen, and a copy's name is a technical one anyway
 *  ("Spl_A_B__r1"): printing it would say nothing. "Style de la courbe", "de l'arc", "de la ligne" says what is
 *  being styled, which is the only thing this group is about. One string per kind rather than a composed one,
 *  so a translator keeps the French article and elision right ("de la courbe", "de l'arc"). */
auto CopyStyleTitle(const VAbstractOperation::CopyStyle &style) -> QString
{
    if (style.isLine)
    {
        return T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line style"));
    }

    switch (style.type)
    {
        case GOType::Arc:
            return T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Arc style"));
        case GOType::EllipticalArc:
            return T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Elliptical arc style"));
        default:
            return T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Curve style"));
    }
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The row of a style list the given style stands on — never the first one by accident.
 *
 *  A value no picture stands for — `default` on a copy that inherits the pen of the object it comes from, a value
 *  read back from an older file — must not be shown as the first entry of the list: that is a lie about what is on
 *  screen, and Sophie hit exactly that (01-10-2026). The entry that matches the pen the object is DRAWN with is
 *  the honest answer, and the pen is what the drawing asks for anyway (LineStyleToPenStyle). */
auto StyleIndex(const QMap<QString, QIcon> &styles, const QString &penStyle) -> vpesizetype
{
    const vpesizetype index = VPE::VLineTypeProperty::IndexOfStyle(styles, penStyle);
    if (index >= 0)
    {
        return index;
    }

    const vpesizetype drawn = VPE::VLineTypeProperty::IndexOfStyle(
        styles, PenStyleToLineStyle(LineStyleToPenStyle(penStyle)));
    return drawn >= 0 ? drawn : 0;
}

/** @brief The pens the pleats draw their lines with — the departure lines, their copies, and the sides of the caps.
 *
 *  A group is what the browser dispatches a change to, a child having no entry of its own in its maps (see
 *  CopyStyleGroup), so every value it holds is read and written together, from the children, in the order they are
 *  added below.
 *
 *  The departure lines are pairs of points and their values belong to each of them: the dock has no editor for
 *  either, and the dialog is where they are changed — a button above this group opens it. What is left here is what
 *  belongs to the whole figure.
 *
 *  The pleats write explicit values, so their lines never take the pen of the layer they are put in — an exception
 *  Sophie asked for (see Pleat.md). A pen only redraws lines, so each setter edits the options with a lite parse,
 *  exactly as the display of a slash-and-spread does. */
template <class Tool> auto PleatGroup() -> PropertyDesc
{
    return {QStringLiteral("pleat"), QString(),
            [](VToolOptionsPropertyBrowser *b, void *tool) -> VPE::VProperty *
            {
                auto *t = static_cast<Tool *>(tool);

                // A titled frame, the way the dialog groups its options, and not a foldable header: the group is
                // what the rows below belong to, and it holds no switch of its own.
                auto *group = new OGroupBoxProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Pleat")),
                                                    /*checkable*/ false);

                const QPalette cp = b->ComboBoxPalette();
                const auto styles = LineStylesPics(cp.color(QPalette::Base), cp.color(QPalette::Text));

                auto *lineType = new VPE::VLineTypeProperty(
                    T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line type:")));
                lineType->setStyles(styles);
                lineType->setValue(StyleIndex(styles, t->DisplayLineType()));

                auto *lineColor = new VPE::VLineColorProperty(
                    T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line color:")));
                lineColor->SetDefaultColors(VAbstractTool::ColorsList());
                lineColor->SetCustomColors(VAbstractValApplication::VApp()->ValentinaSettings()->GetUserToolColors());
                lineColor->SetUseNativeDialog(!VAbstractApplication::VApp()->Settings()->IsDontUseNativeDialog());
                lineColor->setValue(t->DisplayLineColor());

                group->addChild(lineType);
                group->addChild(lineColor);
                return group;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                auto *t = qgraphicsitem_cast<Tool *>(b->currentItem());
                if (t == nullptr || prop->getRowCount() < 2)
                {
                    return;
                }

                t->SetDisplayLineType(prop->getChild(0)->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString());
                t->SetDisplayLineColor(prop->getChild(1)->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString());
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                auto *t = qgraphicsitem_cast<Tool *>(b->currentItem());
                if (t == nullptr || prop->getRowCount() < 2)
                {
                    return;
                }

                const QPalette cp = b->ComboBoxPalette();
                const auto styles = LineStylesPics(cp.color(QPalette::Base), cp.color(QPalette::Text));

                prop->getChild(0)->setValue(StyleIndex(styles, t->DisplayLineType()));
                prop->getChild(1)->setValue(t->DisplayLineColor());
            }};
}

/**
 * @brief The fold line of a pleat, and the pen it is drawn with — the very contents of the "Fold line" group of the
 * dialog.
 *
 *  The figure of a pleat is always drawn: there is nothing to turn on or off but this line, which joins the two caps
 * — without both of them it has nothing to join, and its row is greyed.
 */
template <class Tool> auto PleatFoldLineGroup() -> PropertyDesc
{
    return {QStringLiteral("pleatFigure"), QString(),
            [](VToolOptionsPropertyBrowser *b, void *tool) -> VPE::VProperty *
            {
                auto *t = static_cast<Tool *>(tool);

                // A titled frame, the way the dialog groups its options, with no switch of its own: what it holds is
                // the fold line, and the figure is drawn whether it is there or not.
                auto *group = new OGroupBoxProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Fold line")),
                                                    /*checkable*/ false);

                auto *fold = new OCheckBoxRowProperty(
                    T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Draw the fold line")));
                fold->setValue(t->IsFoldLineShown());
                fold->setEditorDisabled(!t->FoldLinePossible());

                const QPalette cp = b->ComboBoxPalette();
                const auto styles = LineStylesPics(cp.color(QPalette::Base), cp.color(QPalette::Text));

                auto *foldType = new VPE::VLineTypeProperty(
                    T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Fold line type:")));
                foldType->setStyles(styles);
                foldType->setValue(StyleIndex(styles, t->DisplayFoldLineType()));

                auto *foldColor = new VPE::VLineColorProperty(
                    T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Fold line color:")));
                foldColor->SetDefaultColors(VAbstractTool::ColorsList());
                foldColor->SetCustomColors(VAbstractValApplication::VApp()->ValentinaSettings()->GetUserToolColors());
                foldColor->SetUseNativeDialog(!VAbstractApplication::VApp()->Settings()->IsDontUseNativeDialog());
                foldColor->setValue(t->DisplayFoldLineColor());

                group->addChild(fold);
                group->addChild(foldType);
                group->addChild(foldColor);
                return group;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                auto *t = qgraphicsitem_cast<Tool *>(b->currentItem());
                if (t == nullptr || prop->getRowCount() < 3)
                {
                    return;
                }

                t->SetFoldLineShown(prop->getChild(0)->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool());
                t->SetDisplayFoldLineType(
                    prop->getChild(1)->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString());
                t->SetDisplayFoldLineColor(
                    prop->getChild(2)->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString());
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                auto *t = qgraphicsitem_cast<Tool *>(b->currentItem());
                if (t == nullptr || prop->getRowCount() < 3)
                {
                    return;
                }

                prop->getChild(0)->setValue(t->IsFoldLineShown());
                prop->getChild(0)->setEditorDisabled(!t->FoldLinePossible());

                const QPalette cp = b->ComboBoxPalette();
                const auto styles = LineStylesPics(cp.color(QPalette::Base), cp.color(QPalette::Text));

                prop->getChild(1)->setValue(StyleIndex(styles, t->DisplayFoldLineType()));
                prop->getChild(2)->setValue(t->DisplayFoldLineColor());
            }};
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> auto CopyStyleGroup() -> PropertyDesc
{
    return {QStringLiteral("copyStyle"), QString(),
            [](VToolOptionsPropertyBrowser *b, void *tool) -> VPE::VProperty *
            {
                auto *t = static_cast<Tool *>(tool);
                const quint32 object = b->ClickedObjectId();
                const VAbstractOperation::CopyStyle style = t->CopyStyleOf(object);
                if (!style.exists)
                {
                    return nullptr;
                }

                QPalette const cp = b->ComboBoxPalette();
                auto styles = style.isLine ? LineStylesPics(cp.color(QPalette::Base), cp.color(QPalette::Text))
                                           : CurvePenStylesPics(cp.color(QPalette::Base), cp.color(QPalette::Text));

                auto *group = new VPE::VProperty(CopyStyleTitle(style));
                group->setCollapsible(true);
                group->setExpandedByDefault(true);

                auto *pen = new VPE::VLineTypeProperty(
                    style.isLine ? T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line type:"))
                                 : T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Pen style:")));
                pen->setStyles(styles);
                pen->setValue(StyleIndex(styles, style.penStyle));

                auto *color
                    = new VPE::VLineColorProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Color:")));
                color->SetDefaultColors(VAbstractTool::ColorsList());
                color->SetCustomColors(VAbstractValApplication::VApp()->ValentinaSettings()->GetUserToolColors());
                color->SetUseNativeDialog(!VAbstractApplication::VApp()->Settings()->IsDontUseNativeDialog());
                color->setValue(style.color.isEmpty() || style.color == ColorDefault ? ColorBlack : style.color);

                group->addChild(pen);
                group->addChild(color);
                return group;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                // The change of a child is dispatched to the group — a child has no entry of its own in the
                // browser's maps — so both values are read from the group and written together.
                auto *t = qgraphicsitem_cast<Tool *>(b->currentItem());
                if (t == nullptr || prop->getRowCount() < 2)
                {
                    return;
                }

                const quint32 object = b->ClickedObjectId();
                const QString penStyle
                    = prop->getChild(0)->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                const QString color = prop->getChild(1)->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();

                const VAbstractOperation::CopyStyle style = t->CopyStyleOf(object);
                if (!style.exists || (style.penStyle == penStyle && style.color == color))
                {
                    return;
                }

                t->SetCopyStyle(object, penStyle, color);
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                auto *t = qgraphicsitem_cast<Tool *>(b->currentItem());
                if (t == nullptr || prop->getRowCount() < 2)
                {
                    return;
                }

                const VAbstractOperation::CopyStyle style = t->CopyStyleOf(b->ClickedObjectId());
                if (!style.exists)
                {
                    return;
                }

                QPalette const cp = b->ComboBoxPalette();
                auto styles = style.isLine ? LineStylesPics(cp.color(QPalette::Base), cp.color(QPalette::Text))
                                           : CurvePenStylesPics(cp.color(QPalette::Base), cp.color(QPalette::Text));
                prop->getChild(0)->setValue(StyleIndex(styles, style.penStyle));
                prop->getChild(1)->setValue(style.color.isEmpty() || style.color == ColorDefault ? ColorBlack
                                                                                                  : style.color);
            }};
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The visual representation of the rotations of a slash-and-spread.
 *
 *  Three rows in one group, and one setting: the two combos only mean something while the representation is
 *  drawn, so they go grey with the box. `setEditorDisabled` is how the dock says that: the same mechanism the
 *  dart cap uses for its two names. The group is what the browser dispatches a change to, a child having no entry
 *  of its own in its maps (see CopyStyleGroup), so the three values are read and written together.
 *
 *  Turning the box on or off creates or drops pattern objects, which is a full parse: the tool's own setter takes
 *  care of it, and is idempotent, so writing the three values here is also what keeps one change from writing
 *  twice. */
template <class Tool> auto RotationDisplayGroup() -> PropertyDesc
{
    return {QStringLiteral("rotationDisplay"), QString(),
            [](VToolOptionsPropertyBrowser *b, void *tool) -> VPE::VProperty *
            {
                auto *t = static_cast<Tool *>(tool);

                auto *group = new VPE::VProperty(
                    T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Rotation display")));
                group->setCollapsible(true);
                group->setExpandedByDefault(true);

                auto *show = new VPE::VBoolProperty(
                    T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show the rotation display")));
                show->setValue(t->IsDisplayShown());

                const QPalette cp = b->ComboBoxPalette();
                const auto styles = LineStylesPics(cp.color(QPalette::Base), cp.color(QPalette::Text));

                auto *pen = new VPE::VLineTypeProperty(
                    T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line type:")));
                pen->setStyles(styles);
                pen->setValue(StyleIndex(styles, t->DisplayTypeLine()));
                pen->setEditorDisabled(not t->IsDisplayShown());

                auto *color = new VPE::VLineColorProperty(
                    T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line color:")));
                color->SetDefaultColors(VAbstractTool::ColorsList());
                color->SetCustomColors(VAbstractValApplication::VApp()->ValentinaSettings()->GetUserToolColors());
                color->SetUseNativeDialog(!VAbstractApplication::VApp()->Settings()->IsDontUseNativeDialog());
                color->setValue(t->DisplayColor());
                color->setEditorDisabled(not t->IsDisplayShown());

                group->addChild(show);
                group->addChild(pen);
                group->addChild(color);
                return group;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                auto *t = qgraphicsitem_cast<Tool *>(b->currentItem());
                if (t == nullptr || prop->getRowCount() < 3)
                {
                    return;
                }

                t->SetDisplayShown(prop->getChild(0)->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool());
                t->SetDisplayTypeLine(prop->getChild(1)->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString());
                t->SetDisplayColor(prop->getChild(2)->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString());

                // The two combos below follow the box: the page is refreshed, which is also the only thing that
                // runs the refresh callbacks again.
                b->UpdateOptions();
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                auto *t = qgraphicsitem_cast<Tool *>(b->currentItem());
                if (t == nullptr || prop->getRowCount() < 3)
                {
                    return;
                }

                const QPalette cp = b->ComboBoxPalette();
                const auto styles = LineStylesPics(cp.color(QPalette::Base), cp.color(QPalette::Text));

                prop->getChild(0)->setValue(t->IsDisplayShown());
                prop->getChild(1)->setValue(StyleIndex(styles, t->DisplayTypeLine()));
                prop->getChild(1)->setEditorDisabled(not t->IsDisplayShown());
                prop->getChild(2)->setValue(t->DisplayColor());
                prop->getChild(2)->setEditorDisabled(not t->IsDisplayShown());
            }};
}

// --- VToolRotation ---
auto RotationProperties() -> std::vector<PropertyDesc>
{
    return {
        // Pivot point (read-only label)
        {AttrCenter, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Pivot point:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolRotation *>(tool);
             auto *prop = new VPE::VLabelProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Pivot point:")));
             prop->setValue(t->OriginPointName());
             return prop;
         },
         nullptr,
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolRotation *>(b->currentItem()))
                 prop->setValue(t->OriginPointName());
         }},

        // Angle formula
        {AttrAngle, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Angle:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolRotation *>(tool);
             auto *prop = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Angle:")));
             prop->SetFormula(t->GetFormulaAngle());
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolRotation *>(b->currentItem()))
             {
                 auto formula = prop->data(VPE::VProperty::DPC_Data, Qt::EditRole).value<VFormula>();
                 if (formula != t->GetFormulaAngle())
                     t->SetFormulaAngle(formula);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolRotation *>(b->currentItem()))
             {
                 QVariant v;
                 v.setValue(t->GetFormulaAngle());
                 prop->setValue(v);
             }
         }},

        // Length, the same rotation read as the distance from the reference point to its image. It is not
        // stored: it is computed from the angle and the pivot (see Rotation.md).
        {AttrLength, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolRotation *>(tool);
             if (!t->HasReferencePoint())
             {
                 // No point among the sources: there is no length to measure, and the row says so rather than
                 // showing a zero that would read as a real value.
                 auto *prop = new VPE::VLabelProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:")));
                 prop->setValue(QStringLiteral("—"));
                 return prop;
             }
             auto *prop = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:")));
             prop->SetFormula(t->GetFormulaLength());
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolRotation *>(b->currentItem()))
             {
                 auto formula = prop->data(VPE::VProperty::DPC_Data, Qt::EditRole).value<VFormula>();
                 if (formula != t->GetFormulaLength())
                 {
                     if (!t->SetFormulaLength(formula))
                     {
                         // The length is further than the circle the reference point draws: the tool kept its
                         // rotation, so the field goes back to what it really says instead of showing a length
                         // that was refused without a word (same revert as MakeStringPropWithValidator).
                         QVariant v;
                         v.setValue(t->GetFormulaLength());
                         prop->setValue(v);
                     }
                 }
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolRotation *>(b->currentItem()))
             {
                 if (!t->HasReferencePoint())
                 {
                     return; // the row is a plain label: there is nothing to refresh
                 }
                 QVariant v;
                 v.setValue(t->GetFormulaLength());
                 prop->setValue(v);
             }
         }},

        // Notes
        CopyStyleGroup<VToolRotation>(),
        NotesProperty<VToolRotation>(),
    };
}

// --- OToolCloseDart ---
auto CloseDartProperties() -> std::vector<PropertyDesc>
{
    return {
        // The closing divisor, and nothing else. The angle belongs to the dart and the three points are the
        // ones clicked in the pattern, so neither the angle nor a point name is shown here. The row is drawn
        // inline — label, field and fx button on one line — so that its long label cannot widen the label
        // column and push the Notes field of this same dock to the right.
        {AttrClosingDivisor, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Closing divisor:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<OToolCloseDart *>(tool);
             auto *prop = new OFormulaRowProperty(
                 T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Closing divisor:")));
             prop->SetFormula(t->GetFormulaDivisor());
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<OToolCloseDart *>(b->currentItem()))
             {
                 auto formula = prop->data(VPE::VProperty::DPC_Data, Qt::EditRole).value<VFormula>();
                 if (formula != t->GetFormulaDivisor())
                 {
                     t->SetFormulaDivisor(formula);
                     // A divisor that cannot divide is refused, and the dock must not go on showing it as if the
                     // tool had taken it: same revert as every setter that is allowed to say no. An inline row
                     // owns its field, so it takes the revert itself — the field has to give in even though the
                     // user is still typing in it.
                     if (t->GetFormulaDivisor() != formula)
                     {
                         if (auto *row = qobject_cast<OFormulaRowProperty *>(prop); row != nullptr)
                         {
                             row->RevertTo(t->GetFormulaDivisor());
                         }
                         else
                         {
                             QVariant v;
                             v.setValue(t->GetFormulaDivisor());
                             prop->setValue(v);
                         }
                     }
                 }
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<OToolCloseDart *>(b->currentItem()))
             {
                 QVariant v;
                 v.setValue(t->GetFormulaDivisor());
                 prop->setValue(v);
             }
         }},

        // Notes
        CopyStyleGroup<OToolCloseDart>(),
        NotesProperty<OToolCloseDart>(),
    };
}

// --- VToolMove ---
auto MoveProperties() -> std::vector<PropertyDesc>
{
    return {
        // Angle formula
        {AttrAngle, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Angle:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolMove *>(tool);
             auto *prop = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Angle:")));
             prop->SetFormula(t->GetFormulaAngle());
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolMove *>(b->currentItem()))
             {
                 auto formula = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (formula != t->GetFormulaAngle())
                     t->SetFormulaAngle(formula);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolMove *>(b->currentItem()))
             {
                 QVariant v;
                 v.setValue(t->GetFormulaAngle());
                 prop->setValue(v);
             }
         }},

        // Length formula
        {AttrLength, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolMove *>(tool);
             auto *prop = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:")));
             prop->SetFormula(t->GetFormulaLength());
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolMove *>(b->currentItem()))
             {
                 auto formula = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (formula != t->GetFormulaLength())
                     t->SetFormulaLength(formula);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolMove *>(b->currentItem()))
             {
                 QVariant v;
                 v.setValue(t->GetFormulaLength());
                 prop->setValue(v);
             }
         }},

        // Rotation angle formula
        {AttrRotationAngle, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Rotation angle:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolMove *>(tool);
             auto *prop = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Rotation angle:")));
             prop->SetFormula(t->GetFormulaRotationAngle());
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolMove *>(b->currentItem()))
             {
                 auto formula = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (formula != t->GetFormulaRotationAngle())
                     t->SetFormulaRotationAngle(formula);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolMove *>(b->currentItem()))
             {
                 QVariant v;
                 v.setValue(t->GetFormulaRotationAngle());
                 prop->setValue(v);
             }
         }},

        // Pivot point (read-only)
        {AttrCenter, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Pivot point:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolMove *>(tool);
             auto *prop = new VPE::VLabelProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Pivot point:")));
             prop->setValue(t->OriginPointName());
             return prop;
         },
         nullptr,
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolMove *>(b->currentItem()))
                 prop->setValue(t->OriginPointName());
         }},

        // Notes
        CopyStyleGroup<VToolMove>(),
        NotesProperty<VToolMove>(),
    };
}

// --- VToolFlippingByLine ---
auto FlippingByLineProperties() -> std::vector<PropertyDesc>
{
    return {
        // First line point (read-only)
        {AttrFirstPoint, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First line point:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolFlippingByLine *>(tool);
             auto *prop = new VPE::VLabelProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First line point:")));
             prop->setValue(t->FirstLinePointName());
             return prop;
         },
         nullptr,
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolFlippingByLine *>(b->currentItem()))
                 prop->setValue(t->FirstLinePointName());
         }},

        // Second line point (read-only)
        {AttrSecondPoint, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Second line point:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolFlippingByLine *>(tool);
             auto *prop = new VPE::VLabelProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Second line point:")));
             prop->setValue(t->SecondLinePointName());
             return prop;
         },
         nullptr,
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolFlippingByLine *>(b->currentItem()))
                 prop->setValue(t->SecondLinePointName());
         }},

        // Notes
        CopyStyleGroup<VToolFlippingByLine>(),
        NotesProperty<VToolFlippingByLine>(),
    };
}

// --- VToolFlippingByAxis ---
auto FlippingByAxisProperties() -> std::vector<PropertyDesc>
{
    return {
        // Axis type enum
        {AttrAxisType, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Axis type:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolFlippingByAxis *>(tool);
             auto *prop = new VPE::VEnumProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Axis type:")));
             prop->setLiterals(QStringList({VToolOptionsPropertyBrowser::tr("Vertical axis"),
                                            VToolOptionsPropertyBrowser::tr("Horizontal axis")}));
             prop->setValue(static_cast<int>(t->GetAxisType()) - 1);
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolFlippingByAxis *>(b->currentItem()))
             {
                 const QVariant value = prop->data(VPE::VProperty::DPC_Data, Qt::EditRole);
                 bool ok = false;
                 const int val = value.toInt(&ok);
                 if (ok)
                 {
                     AxisType type = AxisType::VerticalAxis;
                     switch (val)
                     {
                         case 0: type = AxisType::VerticalAxis; break;
                         case 1: type = AxisType::HorizontalAxis; break;
                         default: break;
                     }
                     if (type != t->GetAxisType())
                         t->SetAxisType(type);
                 }
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolFlippingByAxis *>(b->currentItem()))
                 prop->setValue(static_cast<int>(t->GetAxisType()) - 1);
         }},

        // Origin point (read-only)
        {AttrCenter, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Origin point:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolFlippingByAxis *>(tool);
             auto *prop = new VPE::VLabelProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Origin point:")));
             prop->setValue(t->OriginPointName());
             return prop;
         },
         nullptr,
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolFlippingByAxis *>(b->currentItem()))
                 prop->setValue(t->OriginPointName());
         }},

        // Notes
        CopyStyleGroup<VToolFlippingByAxis>(),
        NotesProperty<VToolFlippingByAxis>(),
    };
}

// --- OToolTransfert ---
auto TransfertProperties() -> std::vector<PropertyDesc>
{
    // Notes, and nothing else: the two merge points are read in the pattern, and neither the portion nor the
    // layer it landed in is edited after the tool is created.
    return {
        CopyStyleGroup<OToolTransfert>(),
        NotesProperty<OToolTransfert>(),
    };
}

// --- OToolSpread ---
auto SpreadProperties() -> std::vector<PropertyDesc>
{
    // The rotations are a list, and the dock has no editor for one: a button opens the dialog that holds it.
    // It comes before the pen of the active copy, which is the usual tail of an operation
    // (see SlashAndSpread.md).
    return {
        {QStringLiteral("editRotations"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Edit rotations")),
         [](VToolOptionsPropertyBrowser *browser, void *tool) -> VPE::VProperty *
         {
             Q_UNUSED(tool)
             auto *button = new QPushButton(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Edit rotations")));
             QObject::connect(button,
                              &QPushButton::clicked,
                              browser,
                              [browser]()
                              {
                                  if (auto *spread = qgraphicsitem_cast<OToolSpread *>(browser->currentItem()))
                                  {
                                      spread->OpenOptionsDialog();
                                  }
                              });
             return new VPE::VWidgetProperty(
                 T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Edit rotations")), button);
         },
         nullptr,
         nullptr},

        RotationDisplayGroup<OToolSpread>(),

        CopyStyleGroup<OToolSpread>(),
        NotesProperty<OToolSpread>(),
    };
}

// --- OToolPleat ---
auto PleatProperties() -> std::vector<PropertyDesc>
{
    // The departure lines are pairs of points with a value each, and the dock has no editor for either: a button
    // opens the dialog that holds them, as the rotations of a slash-and-spread do. What is left here is what belongs
    // to the whole figure — the pens of its lines, and the fold line (see Pleat.md).
    return {
        {QStringLiteral("editPleats"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Edit pleats")),
         [](VToolOptionsPropertyBrowser *browser, void *tool) -> VPE::VProperty *
         {
             Q_UNUSED(tool)
             auto *button = new QPushButton(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Edit pleats")));
             QObject::connect(button,
                              &QPushButton::clicked,
                              browser,
                              [browser]()
                              {
                                  if (auto *pleat = qgraphicsitem_cast<OToolPleat *>(browser->currentItem()))
                                  {
                                      pleat->OpenOptionsDialog();
                                  }
                              });
             return new VPE::VWidgetProperty(
                 T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Edit pleats")), button);
         },
         nullptr,
         nullptr},

        PleatGroup<OToolPleat>(),
        PleatFoldLineGroup<OToolPleat>(),

        CopyStyleGroup<OToolPleat>(),
        NotesProperty<OToolPleat>(),
    };
}

// --- OToolMoveBatch ---
auto MoveBatchProperties() -> std::vector<PropertyDesc>
{
    // The moves are a list, and the dock has no editor for one: a button opens the dialog that holds it.
    // It comes before the pen of the active copy, which is the usual tail of an operation
    // (see MoveBatch.md).
    return {
        {QStringLiteral("editMoves"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Edit moves")),
         [](VToolOptionsPropertyBrowser *browser, void *tool) -> VPE::VProperty *
         {
             Q_UNUSED(tool)
             auto *button = new QPushButton(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Edit moves")));
             QObject::connect(button,
                              &QPushButton::clicked,
                              browser,
                              [browser]()
                              {
                                  if (auto *moveBatch = qgraphicsitem_cast<OToolMoveBatch *>(browser->currentItem()))
                                  {
                                      moveBatch->OpenOptionsDialog();
                                  }
                              });
             return new VPE::VWidgetProperty(
                 T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Edit moves")), button);
         },
         nullptr,
         nullptr},

        CopyStyleGroup<OToolMoveBatch>(),
        NotesProperty<OToolMoveBatch>(),
    };
}
