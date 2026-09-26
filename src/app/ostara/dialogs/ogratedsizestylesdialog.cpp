/************************************************************************
 **
 **  @file   ogratedsizestylesdialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   06/09/2026
 **
 **  @brief  Implementation of the "Edit size styles…" dialog.
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

#include "ogratedsizestylesdialog.h"

#include "vmisc/def.h" // RoundForDisplay

#include <QDialogButtonBox>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QScreen>
#include <QScrollArea>
#include <QSizePolicy>
#include <QVBoxLayout>

#include <QtGlobal>

#include <cmath>

namespace
{

//---------------------------------------------------------------------------------------------------------------------
/// Normalize a color line to "#RRGGBB" (accepts "#RRGGBB" and "RRGGBB"). Empty
/// when the text is empty or not a valid hex color.
auto NormalizeHex(const QString &text) -> QString
{
    QString colorText = text.trimmed();
    if (colorText.isEmpty())
    {
        return {};
    }
    if (!colorText.startsWith(QLatin1Char('#')))
    {
        colorText.prepend(QLatin1Char('#'));
    }
    const QColor color(colorText);
    return color.isValid() ? color.name() : QString();
}

//---------------------------------------------------------------------------------------------------------------------
/// Parse one dash field (in points). Empty text is not an error (unset); a
/// filled field must be a strictly positive, finite pt number ('decimal comma'
/// accepted).
auto ParsePt(const QString &text, qreal *value) -> bool
{
    QString number = text.trimmed();
    if (number.isEmpty())
    {
        *value = 0.0;
        return true;
    }
    number.replace(QLatin1Char(','), QLatin1Char('.'));
    bool ok = false;
    const qreal parsed = number.toDouble(&ok);
    if (!ok || parsed <= 0.0 || !std::isfinite(parsed))
    {
        return false;
    }
    *value = parsed;
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void UpdateSwatch(QLabel *swatch, const QString &colorHex)
{
    if (colorHex.isEmpty())
    {
        swatch->setStyleSheet(QStringLiteral("background: transparent; border: 1px solid palette(mid);"));
    }
    else
    {
        swatch->setStyleSheet(
            QStringLiteral("background:%1; border: 1px solid palette(mid);").arg(colorHex));
    }
}

} // namespace

//---------------------------------------------------------------------------------------------------------------------
OGradedSizeStylesDialog::OGradedSizeStylesDialog(const QStringList &sizeValues,
                                                 const QHash<QString, OGradedSizeStyle> &styles,
                                                 QWidget *parent)
  : QDialog(parent)
{
    setWindowTitle(tr("Edit size styles"));
    setModal(true);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(12, 12, 12, 12);

    // Rows in a scroll area: the graded axis can hold many values.
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *content = new QWidget(scroll);
    auto *rowsLayout = new QVBoxLayout(content);
    rowsLayout->setContentsMargins(4, 8, 4, 8);
    rowsLayout->setSpacing(6);

    for (const QString &value : sizeValues)
    {
        Row row;
        row.value = value;
        const OGradedSizeStyle style = styles.value(value);

        auto *rowWidget = new QWidget(content);
        auto *rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(10);

        auto *valueLabel = new QLabel(value, rowWidget);
        valueLabel->setMinimumWidth(56);
        valueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        rowLayout->addWidget(valueLabel, 0, Qt::AlignTop);

        // Layer-name override (what the SVG export names the layer with). It is the
        // only column that grows: widening the dialog widens this field instead of
        // pushing the Dash/Space columns apart.
        row.layerName = new QLineEdit(rowWidget);
        row.layerName->setPlaceholderText(tr("Layer name for SVG export"));
        row.layerName->setMinimumWidth(220);
        row.layerName->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        row.layerName->setText(style.layerName);
        row.layerName->setToolTip(tr("Layer name for SVG export"));
        rowLayout->addWidget(row.layerName, 1, Qt::AlignTop);

        // Color hex (the export color of this size) + live swatch.
        row.color = new QLineEdit(rowWidget);
        row.color->setPlaceholderText(QStringLiteral("#000000")); // default black trace
        row.color->setFixedWidth(110);
        row.color->setText(style.HasColor() ? style.color.name() : QString());
        rowLayout->addWidget(row.color, 0, Qt::AlignTop);

        row.swatch = new QLabel(rowWidget);
        row.swatch->setFixedSize(26, 24);
        UpdateSwatch(row.swatch, row.color->text());
        rowLayout->addWidget(row.swatch, 0, Qt::AlignTop);

        connect(row.color, &QLineEdit::textChanged, row.swatch,
                [swatch = row.swatch](const QString &text) { UpdateSwatch(swatch, NormalizeHex(text)); });

        // Dash editor — 6 fields, Dash/Space alternated, one caption under each.
        // The field width follows the caption (translations like "Pointillé" are
        // wider than the English), so nothing ever clips.
        for (int k = 0; k < 6; ++k)
        {
            auto *column = new QWidget(rowWidget);
            auto *columnLayout = new QVBoxLayout(column);
            columnLayout->setContentsMargins(0, 0, 0, 0);
            columnLayout->setSpacing(1);

            // Literal tr() calls so lupdate can extract them (a tr() on a variable
            // like dashCaptions[k] would stay untranslated).
            const QString captionText = (k % 2 == 0) ? tr("Dash") : tr("Space");
            auto *caption = new QLabel(captionText, column);
            caption->setAlignment(Qt::AlignHCenter);
            const int columnWidth = qMax(60, caption->sizeHint().width() + 8);

            // The unit is shown inside the field (placeholder), like the layer-name
            // hint — "pt", so the user knows the dashes are typed in points.
            row.dash[k] = new QLineEdit(column);
            row.dash[k]->setPlaceholderText(QStringLiteral("pt"));
            row.dash[k]->setFixedWidth(columnWidth);
            row.dash[k]->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            if (k < style.dashes.size())
            {
                row.dash[k]->setText(QString::number(RoundForDisplay(style.dashes.at(k))));
            }
            columnLayout->addWidget(row.dash[k]);

            caption->setFixedWidth(columnWidth);
            columnLayout->addWidget(caption);

            // Fixed width — widening the dialog must never push the 6 columns apart.
            column->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            rowLayout->addWidget(column, 0, Qt::AlignTop);
        }

        rowsLayout->addWidget(rowWidget);
        m_rows.append(row);
    }

    rowsLayout->addStretch(1);
    scroll->setWidget(content);
    rootLayout->addWidget(scroll, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        QString message;
        if (!Validate(&message))
        {
            QMessageBox::warning(this, tr("Edit size styles"), message, QMessageBox::Ok);
            return;
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    rootLayout->addWidget(buttons);

    // Size the dialog against the active window: height up to 80% of it; width fits
    // the full row content (+ a little slack so the vertical scrollbar never forces
    // a horizontal one), without exceeding the screen. Enlarging further just grows
    // the "Layer name" field (see above) — the 6 Dash/Space columns stay tight.
    const QWidget *host = parentWidget();
    const QRect avail = [this]() {
        const QPoint center = geometry().center();
        const QScreen *screen = QGuiApplication::screenAt(center);
        if (screen == nullptr)
        {
            screen = QGuiApplication::primaryScreen();
        }
        return screen != nullptr ? screen->availableGeometry() : QRect(0, 0, 1000, 720);
    }();
    const int hostHeight =
        (host != nullptr && host->window()->isVisible()) ? host->window()->height() : avail.height();
    const int contentWidth = content->minimumSizeHint().width();
    resize(qMin(contentWidth + 40, avail.width() - 40),
           qMin(int(hostHeight * 0.8), avail.height() - 60));
}

//---------------------------------------------------------------------------------------------------------------------
auto OGradedSizeStylesDialog::Validate(QString *message) const -> bool
{
    for (const Row &row : m_rows)
    {
        const QString colorText = row.color->text().trimmed();
        if (!colorText.isEmpty() && NormalizeHex(colorText).isEmpty())
        {
            *message = tr("Size %1: “%2” is not a valid hex color.").arg(row.value, colorText);
            return false;
        }

        // A pair must be complete: a Dash without its Space (or the reverse) is a
        // typing mistake — we refuse it instead of silently dropping the value.
        for (int pair = 0; pair < 3; ++pair)
        {
            const int dashIndex = pair * 2;
            const int spaceIndex = dashIndex + 1;
            const bool hasDash = !row.dash[dashIndex]->text().trimmed().isEmpty();
            const bool hasSpace = !row.dash[spaceIndex]->text().trimmed().isEmpty();
            if (hasDash != hasSpace)
            {
                *message = tr("Size %1: a dash pair needs both a Dash and a Space value "
                              "(or leave both empty).")
                               .arg(row.value);
                return false;
            }
        }

        for (int k = 0; k < 6; ++k)
        {
            qreal parsed = 0.0;
            if (!ParsePt(row.dash[k]->text(), &parsed))
            {
                *message = tr("Size %1: dash value “%2” must be a positive number in points (pt).")
                               .arg(row.value, row.dash[k]->text().trimmed());
                return false;
            }
        }
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto OGradedSizeStylesDialog::styles() const -> QHash<QString, OGradedSizeStyle>
{
    QHash<QString, OGradedSizeStyle> result;
    for (const Row &row : m_rows)
    {
        OGradedSizeStyle style;
        style.layerName = row.layerName->text().trimmed(); // empty → derived name

        const QString colorHex = NormalizeHex(row.color->text());
        if (!colorHex.isEmpty())
        {
            style.color = QColor(colorHex);
        }

        // A pair is taken into account only when both fields carry a px value
        // (an incomplete pair — a filled dash with an empty space — is ignored).
        for (int pair = 0; pair < 3; ++pair)
        {
            const int dashIndex = pair * 2;
            const int spaceIndex = dashIndex + 1;
            if (row.dash[dashIndex]->text().trimmed().isEmpty()
                || row.dash[spaceIndex]->text().trimmed().isEmpty())
            {
                continue;
            }
            qreal dash = 0.0, space = 0.0;
            if (ParsePt(row.dash[dashIndex]->text(), &dash)
                && ParsePt(row.dash[spaceIndex]->text(), &space))
            {
                style.dashes.append(dash);
                style.dashes.append(space);
            }
        }

        if (style.HasContent())
        {
            result.insert(row.value, style);
        }
    }
    return result;
}
