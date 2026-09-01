/************************************************************************
 **
 **  @file   oannotationstylespage.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  Annotation styles page of the cartouche editor (see oannotationstylespage.h).
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

#include "oannotationstylespage.h"

#include "oannotationpreview.h"

#include "vmisc/FactoryWidgets.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFontComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace
{
// Small colored square shown in the Background column of the style list.
QIcon MakeColorSwatch(const QColor &color)
{
    QPixmap pixmap(14, 14);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setPen(QPen(QColor(128, 128, 128), 1));
    painter.setBrush(color);
    painter.drawRect(QRectF(0.5, 0.5, 13.0, 13.0));
    return QIcon(pixmap);
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------
OAnnotationStylesPage::OAnnotationStylesPage(QWidget *parent) : QWidget(parent)
{
    auto *outer = new QHBoxLayout(this);
    outer->setContentsMargins(8, 8, 8, 8);
    outer->setSpacing(8);

    // ── Left column: buttons + list + edit form (structure of the Logo page). ──
    auto *left = new QWidget();
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(4);

    auto *listBar = new QHBoxLayout();
    m_addStyleBtn = new QPushButton(tr("Add annotation style"));
    connect(m_addStyleBtn, &QPushButton::clicked, this, &OAnnotationStylesPage::AddStyle);
    listBar->addWidget(m_addStyleBtn);
    m_removeStyleBtn = new QPushButton(tr("Remove style"));
    connect(m_removeStyleBtn, &QPushButton::clicked, this, &OAnnotationStylesPage::RemoveStyle);
    listBar->addWidget(m_removeStyleBtn);
    listBar->addStretch();
    leftLayout->addLayout(listBar);

    m_stylesTree = new QTreeWidget();
    m_stylesTree->setColumnCount(5);
    m_stylesTree->setHeaderLabels({tr("Name"), tr("Font"), tr("Weight"), tr("Size"), tr("Background")});
    m_stylesTree->setRootIsDecorated(false);
    m_stylesTree->setUniformRowHeights(true);
    connect(m_stylesTree, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem *current, QTreeWidgetItem *previous) {
                Q_UNUSED(previous)
                OnStyleSelected(current);
            });
    leftLayout->addWidget(m_stylesTree, 1);

    m_styleEditGroup = new QGroupBox(tr("Annotation style"));
    auto *form = new QFormLayout(m_styleEditGroup);
    form->setContentsMargins(8, 8, 8, 8);
    form->setSpacing(6);

    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText(tr("Style name"));
    connect(m_nameEdit, &QLineEdit::textChanged, this, &OAnnotationStylesPage::OnStyleNameEdited);
    form->addRow(tr("Name:"), m_nameEdit);

    m_fontCombo = new QFontComboBox();
    connect(m_fontCombo, &QFontComboBox::currentFontChanged, this, &OAnnotationStylesPage::OnStyleFieldChanged);
    form->addRow(tr("Font:"), m_fontCombo);

    m_weightCombo = MakeComboBox(this);
    RebuildWeightCombo();
    connect(m_weightCombo, &QComboBox::currentIndexChanged, this, &OAnnotationStylesPage::OnStyleFieldChanged);
    form->addRow(tr("Weight:"), m_weightCombo);

    m_sizeSpin = new QSpinBox();
    m_sizeSpin->setRange(0, 288);
    m_sizeSpin->setSpecialValueText(tr("Default"));
    connect(m_sizeSpin, &QSpinBox::valueChanged, this, &OAnnotationStylesPage::OnStyleFieldChanged);
    form->addRow(tr("Size:"), m_sizeSpin);

    // Background: free hex input (+#), color swatch committed at editingFinished,
    // two color presets.
    auto *bgRow = new QHBoxLayout();
    bgRow->setContentsMargins(0, 0, 0, 0);
    bgRow->setSpacing(6);
    m_backgroundEdit = new QLineEdit();
    m_backgroundEdit->setPlaceholderText(tr("#RRGGBB, empty = transparent"));
    connect(m_backgroundEdit, &QLineEdit::textChanged, this, &OAnnotationStylesPage::OnBackgroundTextChanged);
    connect(m_backgroundEdit, &QLineEdit::editingFinished, this, &OAnnotationStylesPage::OnBackgroundEditingFinished);
    m_backgroundSwatch = new QLabel();
    m_backgroundSwatch->setFixedSize(24, 24);
    m_backgroundSwatch->setAlignment(Qt::AlignCenter);
    bgRow->addWidget(m_backgroundEdit, 1);
    bgRow->addWidget(m_backgroundSwatch);
    form->addRow(tr("Background:"), bgRow);

    auto *presets = new QHBoxLayout();
    presets->setContentsMargins(0, 0, 0, 0);
    presets->setSpacing(6);
    presets->addWidget(new QLabel(tr("Presets:")));
    m_presetWhiteBtn = new QPushButton();
    m_presetWhiteBtn->setToolTip(tr("White"));
    m_presetWhiteBtn->setFixedSize(28, 24);
    m_presetWhiteBtn->setStyleSheet(QStringLiteral("QPushButton { background-color: #FFFFFF; border: 1px solid #888888; }"));
    connect(m_presetWhiteBtn, &QPushButton::clicked, this, [this]() { OnPresetClicked(QStringLiteral("#FFFFFF")); });
    presets->addWidget(m_presetWhiteBtn);
    m_presetBlackBtn = new QPushButton();
    m_presetBlackBtn->setToolTip(tr("Black"));
    m_presetBlackBtn->setFixedSize(28, 24);
    m_presetBlackBtn->setStyleSheet(QStringLiteral("QPushButton { background-color: #000000; border: 1px solid #888888; }"));
    connect(m_presetBlackBtn, &QPushButton::clicked, this, [this]() { OnPresetClicked(QStringLiteral("#000000")); });
    presets->addWidget(m_presetBlackBtn);
    presets->addStretch();
    form->addRow(QString(), presets);

    m_whiteTextCheck = new QCheckBox(tr("White text for dark backgrounds"));
    connect(m_whiteTextCheck, &QCheckBox::toggled, this, &OAnnotationStylesPage::OnStyleFieldChanged);
    form->addRow(QString(), m_whiteTextCheck);

    leftLayout->addWidget(m_styleEditGroup);

    // ── Right column: editable demo text on top, visual preview below. ──
    auto *right = new QWidget();
    auto *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(4);
    m_previewTextEdit = new QPlainTextEdit();
    m_previewTextEdit->setPlaceholderText(tr("Annotation text"));
    // Demo text only — never persisted, kept during the session.
    m_previewTextEdit->setPlainText(tr("This is an annotation"));
    connect(m_previewTextEdit, &QPlainTextEdit::textChanged, this, &OAnnotationStylesPage::OnPreviewTextChanged);
    m_preview = new OAnnotationPreview();
    rightLayout->addWidget(m_previewTextEdit);
    rightLayout->addWidget(m_preview, 1);

    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(left);
    splitter->addWidget(right);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({400, 400});  // 50/50 at open
    outer->addWidget(splitter, 1);

    // No style selected yet — the editor calls Show() when the page is opened.
    m_styleEditGroup->setEnabled(false);
    m_removeStyleBtn->setEnabled(false);
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationStylesPage::Show()
{
    m_styleEntries = OPieceCartoucheStorage::LoadAnnotationStyles();
    RefreshStylesList();
    SelectStyle(QStringLiteral("Default"));
    if (m_stylesTree->currentItem() == nullptr && m_stylesTree->topLevelItemCount() > 0)
    {
        m_stylesTree->setCurrentItem(m_stylesTree->topLevelItem(0));
    }
    // The demo text is session-only: it survives a page re-open untouched.
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationStylesPage::RefreshStylesList()
{
    m_stylesTree->clear();
    if (m_styleEntries.isEmpty())
    {
        auto *item = new QTreeWidgetItem();
        item->setText(0, tr("No styles yet. Add one."));
        item->setFlags(Qt::NoItemFlags);
        m_stylesTree->addTopLevelItem(item);
        return;
    }
    for (const OPieceCartoucheStorage::AnnotationStyleEntry &entry : m_styleEntries)
    {
        auto *item = new QTreeWidgetItem();
        UpdateRow(item, entry);
        m_stylesTree->addTopLevelItem(item);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationStylesPage::UpdateRow(QTreeWidgetItem *item,
                                      const OPieceCartoucheStorage::AnnotationStyleEntry &entry)
{
    item->setText(0, entry.name);
    item->setText(1, entry.fontFamily);
    item->setText(2, entry.fontWeight == 0 ? tr("Default") : QString::number(entry.fontWeight));
    item->setText(3, entry.fontSize == 0 ? tr("Default") : QString::number(entry.fontSize));
    if (entry.backgroundColor.isEmpty())
    {
        item->setText(4, tr("Transparent"));
        item->setIcon(4, QIcon());
    }
    else
    {
        item->setText(4, entry.backgroundColor);
        item->setIcon(4, MakeColorSwatch(QColor(entry.backgroundColor)));
    }
    item->setData(0, Qt::UserRole, entry.name);
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationStylesPage::SelectStyle(const QString &name)
{
    for (int i = 0; i < m_stylesTree->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem *item = m_stylesTree->topLevelItem(i);
        if (item->data(0, Qt::UserRole).toString() == name)
        {
            m_stylesTree->setCurrentItem(item);
            return;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationStylesPage::OnStyleSelected(QTreeWidgetItem *current)
{
    m_updatingStyles = true;

    if (current == nullptr)
    {
        m_styleEditGroup->setEnabled(false);
        m_removeStyleBtn->setEnabled(false);
        m_nameEdit->clear();
        m_fontCombo->setCurrentFont(QFont());
        m_weightCombo->setCurrentIndex(0);
        m_sizeSpin->setValue(0);
        m_backgroundEdit->clear();
        m_whiteTextCheck->setChecked(false);
        m_preview->SetStyle(OPieceCartoucheStorage::AnnotationStyleEntry());
        m_preview->SetText(m_previewTextEdit->toPlainText());
        m_updatingStyles = false;
        return;
    }

    const QString name = current->data(0, Qt::UserRole).toString();
    bool found = false;
    for (const OPieceCartoucheStorage::AnnotationStyleEntry &entry : m_styleEntries)
    {
        if (entry.name == name)
        {
            m_styleEditGroup->setEnabled(true);
            m_nameEdit->setEnabled(true);
            // At least one style must always remain in the library.
            m_removeStyleBtn->setEnabled(m_styleEntries.size() > 1);

            m_nameEdit->setText(entry.name);
            m_fontCombo->setCurrentFont(entry.fontFamily.isEmpty() ? QFont() : QFont(entry.fontFamily));
            m_weightCombo->setCurrentIndex(m_weightCombo->findData(entry.fontWeight));
            if (m_weightCombo->currentIndex() < 0)
            {
                m_weightCombo->setCurrentIndex(0);
            }
            m_sizeSpin->setValue(entry.fontSize);
            m_backgroundEdit->setText(entry.backgroundColor);
            m_whiteTextCheck->setChecked(entry.textColor == QStringLiteral("#FFFFFF"));

            m_preview->SetStyle(entry);
            m_preview->SetText(m_previewTextEdit->toPlainText());
            found = true;
            break;
        }
    }

    if (!found)
    {
        // The empty-state hint item (no UserRole) — disable the form.
        m_styleEditGroup->setEnabled(false);
        m_removeStyleBtn->setEnabled(false);
        m_preview->SetStyle(OPieceCartoucheStorage::AnnotationStyleEntry());
    }

    m_updatingStyles = false;
    ApplyBackgroundValidity();
    UpdateSwatch();
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationStylesPage::OnStyleNameEdited(const QString &text)
{
    if (m_updatingStyles)
    {
        return;
    }
    QTreeWidgetItem *item = m_stylesTree->currentItem();
    if (item == nullptr)
    {
        return;
    }
    const QString oldName = item->data(0, Qt::UserRole).toString();
    if (oldName.isEmpty())
    {
        return;  // empty-state hint item
    }
    const QString newName = text.trimmed();
    if (newName.isEmpty() || newName == oldName)
    {
        return;  // a style needs a non-empty key
    }
    // A duplicate key would shadow an existing entry — reject the rename and
    // restore the field to the previous name.
    const bool collision =
        std::any_of(m_styleEntries.cbegin(), m_styleEntries.cend(),
                    [&newName, &oldName](const OPieceCartoucheStorage::AnnotationStyleEntry &entry) {
                        return entry.name == newName && entry.name != oldName;
                    });
    if (collision)
    {
        m_updatingStyles = true;
        m_nameEdit->setText(oldName);
        m_updatingStyles = false;
        return;
    }
    for (OPieceCartoucheStorage::AnnotationStyleEntry &entry : m_styleEntries)
    {
        if (entry.name == oldName)
        {
            entry.name = newName;
            break;
        }
    }
    item->setText(0, newName);
    item->setData(0, Qt::UserRole, newName);
    PersistStyles();
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationStylesPage::OnStyleFieldChanged()
{
    if (m_updatingStyles)
    {
        return;
    }
    QTreeWidgetItem *item = m_stylesTree->currentItem();
    if (item == nullptr)
    {
        return;
    }
    const QString name = item->data(0, Qt::UserRole).toString();
    if (name.isEmpty())
    {
        return;
    }
    for (OPieceCartoucheStorage::AnnotationStyleEntry &entry : m_styleEntries)
    {
        if (entry.name == name)
        {
            entry.fontFamily = m_fontCombo->currentFont().family();
            entry.fontWeight = m_weightCombo->currentData().toInt();
            entry.fontSize = m_sizeSpin->value();
            // White text is only honored on a real background (the checkbox is
            // disabled otherwise).
            const QString hex = NormalizeHex(m_backgroundEdit->text());
            const bool whiteText = m_whiteTextCheck->isChecked() && !hex.isEmpty() && IsValidHex(hex);
            entry.textColor = whiteText ? QStringLiteral("#FFFFFF") : QString();
            UpdateRow(item, entry);
            UpdatePreview();
            break;
        }
    }
    PersistStyles();
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationStylesPage::OnBackgroundTextChanged(const QString &text)
{
    Q_UNUSED(text)
    if (m_updatingStyles)
    {
        return;
    }
    // Live validation: red outline while the hex is invalid; the white-text
    // checkbox is disabled and cleared while the background is transparent.
    const QString hex = NormalizeHex(m_backgroundEdit->text());
    const bool valid = IsValidHex(hex);
    if (!valid && m_whiteTextCheck->isChecked())
    {
        m_updatingStyles = true;
        m_whiteTextCheck->setChecked(false);
        m_updatingStyles = false;
    }
    ApplyBackgroundValidity();
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationStylesPage::OnBackgroundEditingFinished()
{
    if (m_updatingStyles)
    {
        return;
    }
    QTreeWidgetItem *item = m_stylesTree->currentItem();
    if (item == nullptr)
    {
        return;
    }
    const QString name = item->data(0, Qt::UserRole).toString();
    if (name.isEmpty())
    {
        return;
    }
    const QString hex = NormalizeHex(m_backgroundEdit->text());
    if (IsValidHex(hex))
    {
        // Valid (#RRGGBB) or empty (transparent): commit the background.
        for (OPieceCartoucheStorage::AnnotationStyleEntry &entry : m_styleEntries)
        {
            if (entry.name == name)
            {
                entry.backgroundColor = hex;
                // White text only makes sense on a real background: going
                // transparent clears the persisted textColor too (spec §3.2).
                if (hex.isEmpty() && entry.textColor == QStringLiteral("#FFFFFF"))
                {
                    entry.textColor.clear();
                    m_updatingStyles = true;
                    m_whiteTextCheck->setChecked(false);
                    m_updatingStyles = false;
                }
                UpdateRow(item, entry);
                UpdatePreview();
                break;
            }
        }
        PersistStyles();
    }
    // Invalid hex: nothing is recorded (spec §3.2) — the red outline stays until
    // the field becomes empty or valid again.
    UpdateSwatch();
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationStylesPage::OnPresetClicked(const QString &hex)
{
    m_backgroundEdit->setText(hex);
    OnBackgroundEditingFinished();
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationStylesPage::OnPreviewTextChanged()
{
    UpdatePreview();
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationStylesPage::AddStyle()
{
    QString err;
    const QString newName = OPieceCartoucheStorage::AddAnnotationStyle(&err);
    if (newName.isEmpty())
    {
        QMessageBox::critical(this, tr("Annotations"), err);
        return;
    }
    Show();
    SelectStyle(newName);
    m_nameEdit->setFocus();
    m_nameEdit->selectAll();
    emit StatusMessage(tr("Annotation style added"));
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationStylesPage::RemoveStyle()
{
    QTreeWidgetItem *item = m_stylesTree->currentItem();
    if (item == nullptr)
    {
        return;
    }
    const QString name = item->data(0, Qt::UserRole).toString();
    if (name.isEmpty())
    {
        return;  // empty-state hint item
    }
    const auto answer = QMessageBox::question(
        this, tr("Remove style"), tr("Remove the style \"%1\" from the library?").arg(item->text(0)));
    if (answer != QMessageBox::Yes)
    {
        return;
    }
    QString err;
    if (!OPieceCartoucheStorage::RemoveAnnotationStyle(name, &err))
    {
        QMessageBox::critical(this, tr("Annotations"), err);
        return;
    }
    Show();
    emit StatusMessage(tr("Annotation style removed"));
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationStylesPage::PersistStyles()
{
    QString err;
    if (!OPieceCartoucheStorage::SaveAnnotationStyles(m_styleEntries, &err))
    {
        QMessageBox::critical(this, tr("Annotations"), err);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationStylesPage::UpdatePreview()
{
    QTreeWidgetItem *item = m_stylesTree->currentItem();
    if (item == nullptr)
    {
        m_preview->SetStyle(OPieceCartoucheStorage::AnnotationStyleEntry());
        m_preview->SetText(m_previewTextEdit->toPlainText());
        return;
    }
    const QString name = item->data(0, Qt::UserRole).toString();
    for (const OPieceCartoucheStorage::AnnotationStyleEntry &entry : m_styleEntries)
    {
        if (entry.name == name)
        {
            m_preview->SetStyle(entry);
            m_preview->SetText(m_previewTextEdit->toPlainText());
            return;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationStylesPage::ApplyBackgroundValidity()
{
    const QString hex = NormalizeHex(m_backgroundEdit->text());
    const bool valid = IsValidHex(hex);
    m_backgroundEdit->setStyleSheet(valid ? QString()
                                          : QStringLiteral("QLineEdit { border: 1px solid #e06060; }"));
    const bool hasBackground = !hex.isEmpty();
    m_whiteTextCheck->setEnabled(valid && hasBackground);
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationStylesPage::UpdateSwatch()
{
    const QString hex = NormalizeHex(m_backgroundEdit->text());
    if (IsValidHex(hex) && !hex.isEmpty())
    {
        m_backgroundSwatch->setStyleSheet(
            QStringLiteral("background-color: %1; border: 1px solid #888888;").arg(hex));
        m_backgroundSwatch->setText(QString());
    }
    else
    {
        m_backgroundSwatch->setStyleSheet(QStringLiteral("border: 1px solid #888888;"));
        m_backgroundSwatch->setText(QString());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationStylesPage::RebuildWeightCombo()
{
    const QVariant current = m_weightCombo == nullptr ? QVariant() : m_weightCombo->currentData();
    if (m_weightCombo != nullptr)
    {
        m_weightCombo->blockSignals(true);
        m_weightCombo->clear();
    }
    m_weightCombo->addItem(tr("Default"), 0);
    m_weightCombo->addItem(tr("Thin"), QFont::Thin);
    m_weightCombo->addItem(tr("Extra Light"), QFont::ExtraLight);
    m_weightCombo->addItem(tr("Light"), QFont::Light);
    m_weightCombo->addItem(tr("Normal"), QFont::Normal);
    m_weightCombo->addItem(tr("Medium"), QFont::Medium);
    m_weightCombo->addItem(tr("Demi Bold"), QFont::DemiBold);
    m_weightCombo->addItem(tr("Bold"), QFont::Bold);
    m_weightCombo->addItem(tr("Extra Bold"), QFont::ExtraBold);
    m_weightCombo->addItem(tr("Black"), QFont::Black);
    if (m_weightCombo != nullptr)
    {
        const int index = m_weightCombo->findData(current);
        m_weightCombo->setCurrentIndex(index >= 0 ? index : 0);
        m_weightCombo->blockSignals(false);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationStylesPage::RetranslateUI()
{
    m_addStyleBtn->setText(tr("Add annotation style"));
    m_removeStyleBtn->setText(tr("Remove style"));
    m_stylesTree->setHeaderLabels({tr("Name"), tr("Font"), tr("Weight"), tr("Size"), tr("Background")});
    m_styleEditGroup->setTitle(tr("Annotation style"));
    m_nameEdit->setPlaceholderText(tr("Style name"));
    m_backgroundEdit->setPlaceholderText(tr("#RRGGBB, empty = transparent"));
    m_whiteTextCheck->setText(tr("White text for dark backgrounds"));
    m_previewTextEdit->setPlaceholderText(tr("Annotation text"));

    m_updatingStyles = true;
    RebuildWeightCombo();
    // Rebuild the rows with the translated "Default"/"Transparent" strings.
    if (!m_styleEntries.isEmpty() && m_stylesTree->topLevelItemCount() > 0)
    {
        for (int i = 0; i < m_stylesTree->topLevelItemCount(); ++i)
        {
            QTreeWidgetItem *item = m_stylesTree->topLevelItem(i);
            for (const OPieceCartoucheStorage::AnnotationStyleEntry &entry : m_styleEntries)
            {
                if (entry.name == item->data(0, Qt::UserRole).toString())
                {
                    UpdateRow(item, entry);
                    break;
                }
            }
        }
    }
    m_updatingStyles = false;
}

//---------------------------------------------------------------------------------------------------------------------
QString OAnnotationStylesPage::NormalizeHex(const QString &text)
{
    QString hex = text.trimmed();
    if (hex.startsWith(QLatin1Char('#')))
    {
        hex.remove(0, 1);
    }
    if (hex.isEmpty())
    {
        return QString();
    }
    if (hex.size() == 6)
    {
        bool ok = false;
        hex.toUInt(&ok, 16);
        if (ok)
        {
            return QLatin1Char('#') + hex.toUpper();
        }
    }
    // Invalid or partial (#RGB): keep what the user typed for display.
    return text.trimmed();
}

//---------------------------------------------------------------------------------------------------------------------
bool OAnnotationStylesPage::IsValidHex(const QString &text)
{
    const QString hex = NormalizeHex(text);
    // # + 6 hex digits — QColor re-validates the digits (a length-only check
    // would accept e.g. "#gggggg").
    return hex.isEmpty()
           || (hex.startsWith(QLatin1Char('#')) && hex.size() == 7 && QColor(hex).isValid());
}
