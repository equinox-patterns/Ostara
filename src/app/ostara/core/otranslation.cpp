/************************************************************************
 **
 **  @file   otranslation.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   25/09/2026
 **
 **  @brief  Moteur unique de retraduction des textes construits en code.
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

#include "otranslation.h"

#include <QAbstractButton>
#include <QApplication>
#include <QByteArray>
#include <QComboBox>
#include <QCoreApplication>
#include <QEvent>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QString>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QWidget>

namespace
{
/// La mémoire du mécanisme vit sur le widget lui-même : une liste d'entrées, qui meurt avec lui.
/// Une liste, et non une entrée unique, parce qu'un même widget porte souvent plusieurs textes —
/// les onglets d'un `QTabWidget`, les items d'un `QComboBox`, les colonnes d'un `QTableWidget`.
const char *const kRecords = "oTrRecords";

/// Ce qu'il faut réécrire. La liste est close, et couvre les textes qu'on pose en code.
enum class Slot : int
{
    LabelText = 0,
    ButtonText = 1,
    GroupTitle = 2,
    ToolTip = 3,
    TabText = 4,
    ItemText = 5,
    HeaderText = 6,
    Placeholder = 7,
    PlaceholderPlain = 8
};

//---------------------------------------------------------------------------------------------------------------------
/// Traduit `source` dans la langue installée, pour le contexte donné. Rend la source si le
/// catalogue n'a rien — c'est le comportement de `tr()` lui-même, et c'est ce qui rend un texte
/// non traduit inoffensif.
auto Translate(const QString &context, const QString &source) -> QString
{
    const QByteArray ctx = context.toUtf8();
    const QByteArray src = source.toUtf8();
    return QCoreApplication::translate(ctx.constData(), src.constData());
}

//---------------------------------------------------------------------------------------------------------------------
/// Réécrit le texte à sa place. Le type du widget décide de la propriété à reprendre.
void Apply(QWidget *widget, Slot slot, int index, const QString &text)
{
    switch (slot)
    {
        case Slot::LabelText:
            if (auto *label = qobject_cast<QLabel *>(widget); label != nullptr)
            {
                label->setText(text);
            }
            break;
        case Slot::ButtonText:
            if (auto *button = qobject_cast<QAbstractButton *>(widget); button != nullptr)
            {
                button->setText(text);
            }
            break;
        case Slot::GroupTitle:
            if (auto *box = qobject_cast<QGroupBox *>(widget); box != nullptr)
            {
                box->setTitle(text);
            }
            break;
        case Slot::TabText:
            if (auto *tabs = qobject_cast<QTabWidget *>(widget); tabs != nullptr)
            {
                tabs->setTabText(index, text);
            }
            break;
        case Slot::ItemText:
            if (auto *combo = qobject_cast<QComboBox *>(widget); combo != nullptr)
            {
                combo->setItemText(index, text);
            }
            break;
        case Slot::HeaderText:
            if (auto *table = qobject_cast<QTableWidget *>(widget); table != nullptr)
            {
                // L'item peut ne pas exister encore : on ne crée jamais de colonne ici, on
                // n'écrit que dans une en-tête déjà posée.
                if (QTableWidgetItem *item = table->horizontalHeaderItem(index); item != nullptr)
                {
                    item->setText(text);
                }
            }
            break;
        case Slot::Placeholder:
            if (auto *edit = qobject_cast<QLineEdit *>(widget); edit != nullptr)
            {
                edit->setPlaceholderText(text);
            }
            break;
        case Slot::PlaceholderPlain:
            if (auto *edit = qobject_cast<QPlainTextEdit *>(widget); edit != nullptr)
            {
                edit->setPlaceholderText(text);
            }
            break;
        case Slot::ToolTip:
        default:
            widget->setToolTip(text);
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto Records(QWidget *widget) -> QVariantList
{
    return widget->property(kRecords).toList();
}

//---------------------------------------------------------------------------------------------------------------------
/// Mémorise une chaîne à sa place, puis l'applique tout de suite.
void Remember(QWidget *widget, const QObject *owner, const char *source, Slot slot, int index)
{
    const QString context = QString::fromLatin1(owner->metaObject()->className());
    const QString text = QString::fromUtf8(source);
    const int slotValue = static_cast<int>(slot);

    QVariantList list = Records(widget);
    for (QVariant &entry : list)
    {
        QVariantMap map = entry.toMap();
        // Une même place ne se mémorise qu'une fois : un constructeur qui repasse sur le même
        // widget met à jour son entrée, il n'en empile pas une seconde.
        if (map.value(QStringLiteral("slot")).toInt() == slotValue
            && map.value(QStringLiteral("index")).toInt() == index
            && map.value(QStringLiteral("context")).toString() == context)
        {
            map[QStringLiteral("source")] = text;
            entry = map;
            widget->setProperty(kRecords, list);
            Apply(widget, slot, index, Translate(context, text));
            return;
        }
    }

    QVariantMap map;
    map[QStringLiteral("source")] = text;
    map[QStringLiteral("context")] = context;
    map[QStringLiteral("slot")] = slotValue;
    map[QStringLiteral("index")] = index;
    list.append(map);
    widget->setProperty(kRecords, list);
    Apply(widget, slot, index, Translate(context, text));
}

//---------------------------------------------------------------------------------------------------------------------
/// Rejoue tout ce qu'un widget a mémorisé.
void ReplayOne(QWidget *widget)
{
    const QVariantList list = Records(widget);
    for (const QVariant &entry : list)
    {
        const QVariantMap map = entry.toMap();
        Apply(widget, static_cast<Slot>(map.value(QStringLiteral("slot")).toInt()),
              map.value(QStringLiteral("index")).toInt(),
              Translate(map.value(QStringLiteral("context")).toString(),
                        map.value(QStringLiteral("source")).toString()));
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Le moteur : il écoute `LanguageChange` sur l'objet application.
 *
 * **Pourquoi un filtre d'événements et non un `changeEvent`.** C'est le patron que la doc Qt
 * prescrit pour ce cas exact — « Classes dérivées de QObject : utilisez un filtre d'événements
 * […] quand la classe sert de conteneur de widgets et est donc responsable de leur transmettre
 * l'événement ». Un filtre posé sur l'objet application voit le `LanguageChange` **une fois**,
 * avant que Qt ne le descende dans l'arbre, et peut alors reparcourir tout ce qui a été mémorisé —
 * y compris des widgets que leur parent n'appellerait jamais.
 */
class OTranslationWatcher : public QObject
{
public:
    explicit OTranslationWatcher(QObject *parent)
      : QObject(parent)
    {
    }

protected:
    auto eventFilter(QObject *watched, QEvent *event) -> bool override
    {
        if (watched == QCoreApplication::instance() && event->type() == QEvent::LanguageChange)
        {
            OTranslation::ReapplyAll();
        }
        return false;
    }
};

const char *const kWatcherInstalled = "oTrWatcherInstalled";
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void OTranslation::SetText(const QObject *owner, QLabel *widget, const char *source)
{
    if (owner == nullptr || widget == nullptr || source == nullptr)
    {
        return;
    }
    Remember(widget, owner, source, Slot::LabelText, 0);
}

//---------------------------------------------------------------------------------------------------------------------
void OTranslation::SetText(const QObject *owner, QAbstractButton *widget, const char *source)
{
    if (owner == nullptr || widget == nullptr || source == nullptr)
    {
        return;
    }
    Remember(widget, owner, source, Slot::ButtonText, 0);
}

//---------------------------------------------------------------------------------------------------------------------
void OTranslation::SetTitle(const QObject *owner, QGroupBox *widget, const char *source)
{
    if (owner == nullptr || widget == nullptr || source == nullptr)
    {
        return;
    }
    Remember(widget, owner, source, Slot::GroupTitle, 0);
}

//---------------------------------------------------------------------------------------------------------------------
void OTranslation::SetToolTip(const QObject *owner, QWidget *widget, const char *source)
{
    if (owner == nullptr || widget == nullptr || source == nullptr)
    {
        return;
    }
    Remember(widget, owner, source, Slot::ToolTip, 0);
}

//---------------------------------------------------------------------------------------------------------------------
void OTranslation::SetTabText(const QObject *owner, QTabWidget *widget, int index, const char *source)
{
    if (owner == nullptr || widget == nullptr || source == nullptr || index < 0)
    {
        return;
    }
    Remember(widget, owner, source, Slot::TabText, index);
}

//---------------------------------------------------------------------------------------------------------------------
void OTranslation::SetItemText(const QObject *owner, QComboBox *widget, int index, const char *source)
{
    if (owner == nullptr || widget == nullptr || source == nullptr || index < 0)
    {
        return;
    }
    Remember(widget, owner, source, Slot::ItemText, index);
}

//---------------------------------------------------------------------------------------------------------------------
void OTranslation::SetHeaderText(const QObject *owner, QTableWidget *widget, int column, const char *source)
{
    if (owner == nullptr || widget == nullptr || source == nullptr || column < 0)
    {
        return;
    }
    Remember(widget, owner, source, Slot::HeaderText, column);
}

//---------------------------------------------------------------------------------------------------------------------
void OTranslation::SetPlaceholderText(const QObject *owner, QLineEdit *widget, const char *source)
{
    if (owner == nullptr || widget == nullptr || source == nullptr)
    {
        return;
    }
    Remember(widget, owner, source, Slot::Placeholder, 0);
}

//---------------------------------------------------------------------------------------------------------------------
void OTranslation::SetPlaceholderText(const QObject *owner, QPlainTextEdit *widget, const char *source)
{
    if (owner == nullptr || widget == nullptr || source == nullptr)
    {
        return;
    }
    Remember(widget, owner, source, Slot::PlaceholderPlain, 0);
}

//---------------------------------------------------------------------------------------------------------------------
void OTranslation::ReapplyAll()
{
    const QWidgetList widgets = QApplication::allWidgets();
    for (QWidget *widget : widgets)
    {
        if (widget == nullptr)
        {
            continue;
        }
        ReplayOne(widget);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OTranslation::InstallWatcher()
{
    QCoreApplication *app = QCoreApplication::instance();
    if (app == nullptr || app->property(kWatcherInstalled).toBool())
    {
        return;
    }

    app->installEventFilter(new OTranslationWatcher(app));
    app->setProperty(kWatcherInstalled, true);
}
