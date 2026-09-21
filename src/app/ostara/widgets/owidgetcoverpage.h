/************************************************************************
 **
 **  @file   owidgetcoverpage.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/09/2026
 **
 **  @brief  The Cover page dock: what a pattern contributes to its own
 **          cover page, gathered where the pattern is.
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

#ifndef OWIDGETCOVERPAGE_H
#define OWIDGETCOVERPAGE_H

#include <QPointer>
#include <QStringList>
#include <QWidget>

#include "core/oseamallowancelibrary.h"
#include "core/osymbollibrary.h"
#include "xml/vcoverpagedata.h"

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTimer;
class QVBoxLayout;
class VAbstractPattern;

/**
 * @brief The Cover page dock: the four things a cover page reads from the pattern.
 *
 * **A template says what to show and how; this says what it shows.** The name the pattern answers to,
 * the symbols it chose from the library, the text a template left to be filled in for it, and the
 * names it gives to its own seam allowances — four things no template can hold, because they differ
 * from one pattern to the next.
 *
 * **One field, one source.** The document is read and written directly, the way the pattern's own
 * information page does it: the widget holds a `QPointer` to the pattern the application has open,
 * writes into it as the user types, and re-reads it when the document says it has changed. Nothing
 * is kept here that is not also in the file — the dock is a view of the pattern, not a store.
 *
 * **Writings are immediate and are not undoable**, which is what every non-geometric field of a
 * pattern already is: a name, a note, a description are written as they are typed and taken back by
 * retyping them. The Cover page is the same kind of data, and is written the same way.
 *
 * **Nothing here holds a library of its own.** The symbols and the margin names are read from the
 * installation's files when the dock is filled, so a symbol added or a rule rewritten is seen the
 * next time the dock is shown rather than at the next launch.
 */
class OWidgetCoverPage final : public QWidget
{
    Q_OBJECT

public:
    explicit OWidgetCoverPage(QWidget *parent = nullptr);

    /** @brief Attach to whatever pattern is open, and show what it holds. */
    void Refresh();

    /** @brief Re-apply every word this widget shows by itself (LanguageChange). */
    void RetranslateUI();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void BuildUi();

    /** @brief The document has changed under us: drop what is pending and read it again. */
    void OnFullUpdate();

    void Populate();

    void RebuildSymbolRows();
    void RebuildMarginRows();

    /** @brief Hands the page being edited back to the document, if there is one to hand it to. */
    void WritePage();

    /** @brief The name this pattern gives @p mm in @p locale, written where it belongs. */
    void SetMarginName(qreal mm, const QString &locale, const QString &text);

    QLabel *m_nameLabel{nullptr};
    QLineEdit *m_nameEdit{nullptr};

    QLabel *m_symbolsLabel{nullptr};
    QVBoxLayout *m_symbolRows{nullptr};
    QLabel *m_symbolsHint{nullptr};
    QPushButton *m_addSymbolButton{nullptr};

    QLabel *m_freeTextLabel{nullptr};
    QPlainTextEdit *m_freeTextEdit{nullptr};

    QLabel *m_marginsLabel{nullptr};
    QVBoxLayout *m_marginRows{nullptr};
    QLabel *m_marginsHint{nullptr};

    QPointer<VAbstractPattern> m_doc{};

    /// What the dock is editing, in the shape the file keeps it — written whole on every change.
    VCoverPageData m_page{};

    OSymbolLibrary::Library m_symbolsLibrary{};
    OSeamAllowanceLibrary::Library m_marginLibrary{};

    /// Guards the handlers against the signals raised while the widgets are being filled.
    bool m_loading{false};

    /// Holds a text being typed back long enough that every letter is not a write to the document.
    QTimer *m_commitTimer{nullptr};
};

#endif // OWIDGETCOVERPAGE_H
