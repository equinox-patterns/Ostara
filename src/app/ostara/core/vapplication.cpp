/************************************************************************
 **
 **  @file   vapplication.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   November 15, 2013
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 27/10/2026.
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

#include "vapplication.h"
#include "../ifc/exception/vexceptionbadid.h"
#include "../ifc/exception/vexceptionconversionerror.h"
#include "../ifc/exception/vexceptionemptyparameter.h"
#include "../ifc/exception/vexceptioninvalidnotch.h"
#include "../ifc/exception/vexceptionobjecterror.h"
#include "../ifc/exception/vexceptionwrongid.h"
#include "../mainwindow.h"
#include "../qmuparser/qmuparsererror.h"
#include "../version.h"
#include "../vformat/knownmeasurements/vknownmeasurementsdatabase.h"
#include "../vganalytics/vganalytics.h"
#include "../vdispatch/qt_dispatch.h"
#include "otranslation.h"
#include "vtheme.h"
#include "omenuglyph.h"
#include "../vmisc/commandoptions.h"
#include "../vmisc/vsysexits.h"
#include "../vmisc/vvalentinasettings.h"
#include "vvalentinashortcutmanager.h"

#include "QtConcurrent/qtconcurrentrun.h"
#include <vcsRepoState.h>
#include <QAtomicInt>
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileSystemWatcher>
#include <QFuture>
#include <QIcon>
#include <QLoggingCategory>
#include <QMenu>
#include <QMessageBox>
#include <QObject>
#include <QProcess>
#include <QScopeGuard>
#include <QStandardPaths>
#include <QStringList>
#include <QStyleFactory>
#include <QSysInfo>
#include <QTemporaryFile>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <QUrl>
#include <QtDebug>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtXmlPatterns>
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

#if defined(APPIMAGE) && defined(Q_OS_LINUX)
#include "../vmisc/appimage.h"
#endif // defined(APPIMAGE) && defined(Q_OS_LINUX)

using namespace Qt::Literals::StringLiterals;

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wmissing-prototypes")
QT_WARNING_DISABLE_INTEL(1418)

Q_LOGGING_CATEGORY(vApp, "v.application") // NOLINT

QT_WARNING_POP

// Defined further down, once the report writer it calls is in scope. The fatal path below needs it before that
// point, and a declaration inside the anonymous namespace would make a second, distinct function.
inline auto ShowNoisyMessageBox(QtMsgType type) -> bool;

namespace
{
// ::exit(), called from VApplication::notify()'s fatal-exception catch blocks, runs Qt's own static/atexit
// teardown, which can itself emit a stray Qt warning (seen on macOS). Routing that warning back through this
// file's message handler mid-teardown touches Qt internals the teardown may have already torn down, which
// crashed with EXC_BAD_ACCESS (null deref in QApplication::translate()). Once a catch block below has decided
// to terminate, every later message handler call is dropped instead of processed.
QAtomicInt g_terminatingOnFatalException{0};

Q_NORETURN void TerminateOnFatalException(int exitCode)
{
    // The box that announces a critical is deferred to the next event-loop turn (see noisyFailureMsgHandler),
    // and ::exit() below runs before that turn ever comes: the reader saw Ostara disappear without a word, and
    // the report was never written. This is the last moment anything can be shown, so it is shown here, and
    // synchronously - the way the QtFatalMsg path already does it. The guard goes up first, so that whatever
    // the box or Qt's own teardown emits from here on is dropped instead of processed.
    g_terminatingOnFatalException.storeRelease(1);

    if (VApplication::IsGUIMode())
    {
        ShowNoisyMessageBox(QtFatalMsg);
    }

    ::exit(exitCode); // NOLINT(concurrency-mt-unsafe)
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
/// @brief Whether the debug lines also go to the console.
///
/// Every message used to go to both the console and the log file, which left no way to watch the file alone and made
/// the console unreadable on any normal use. The file keeps everything — it is what a crash report carries — and the
/// console now shows warnings and criticals only, unless the run asked for --verbose.
///
/// The arguments are read here rather than through VCommandLine because the first messages are logged before that
/// parser exists (see VApplication::InitOptions), and this must answer from the very first one.
auto ConsoleVerbose() -> bool
{
    static const bool verbose = []()
    {
        const QStringList args = QCoreApplication::arguments();
        return args.contains(QStringLiteral("--") + LONG_OPTION_VERBOSE);
    }();
    return verbose;
}

//---------------------------------------------------------------------------------------------------------------------
inline void LogMessageDetails(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString debugdate = "["_L1 + QDateTime::currentDateTime().toString(QStringLiteral("yyyy.MM.dd hh:mm:ss"));
    QString const file = VAbstractApplication::ReduceLogContextFilePath(context.file);
    const bool isPatternMessage = VAbstractApplication::IsWarningMessage(msg);

    // The console spells the level itself, never through tr(): its lines are read by tools and by whoever reports a
    // bug, so their shape must not depend on the interface language. The file has always done this.
    bool consoleOut = false;
    bool consoleErr = false;

    switch (type)
    {
        case QtDebugMsg:
            debugdate += QStringLiteral(":DEBUG:%1(%2)] %3: %4: %5")
                             .arg(file)
                             .arg(context.line)
                             .arg(context.function, context.category, msg);
            if (ConsoleVerbose())
            {
                vStdOut() << QStringLiteral("DEBUG: ") << msg << "\n";
                consoleOut = true;
            }
            break;
        case QtWarningMsg:
            if (isPatternMessage)
            {
                VAbstractValApplication::VApp()->PostWarningMessage(msg, type);
            }
            debugdate += QStringLiteral(":WARNING:%1(%2)] %3: %4: %5")
                             .arg(file)
                             .arg(context.line)
                             .arg(context.function, context.category, msg);
            vStdErr() << QStringLiteral("WARNING: ") << msg << "\n";
            consoleErr = true;
            break;
        case QtCriticalMsg:
            if (isPatternMessage)
            {
                VAbstractValApplication::VApp()->PostWarningMessage(msg, type);
            }
            debugdate += QStringLiteral(":CRITICAL:%1(%2)] %3: %4: %5")
                             .arg(file)
                             .arg(context.line)
                             .arg(context.function, context.category, msg);
            vStdErr() << QStringLiteral("CRITICAL: ") << msg << "\n";
            consoleErr = true;
            break;
        case QtFatalMsg:
            debugdate += QStringLiteral(":FATAL:%1(%2)] %3: %4: %5")
                             .arg(file)
                             .arg(context.line)
                             .arg(context.function, context.category, msg);
            vStdErr() << QStringLiteral("FATAL: ") << msg << "\n";
            consoleErr = true;
            break;
        case QtInfoMsg:
            if (isPatternMessage)
            {
                VAbstractValApplication::VApp()->PostWarningMessage(msg, type);
            }
            debugdate += QStringLiteral(":INFO:%1(%2)] %3: %4: %5")
                             .arg(file)
                             .arg(context.line)
                             .arg(context.function, context.category, msg);
            vStdOut() << QStringLiteral("INFO: ") << msg << "\n";
            consoleOut = true;
            break;
        default:
            break;
    }
    // Only the stream that was written to is flushed: flushing both cost a write on the other one for every line,
    // and most lines do not touch the console at all any more.
    if (consoleOut)
    {
        vStdOut().flush();
    }
    if (consoleErr)
    {
        vStdErr().flush();
    }

    (*VApplication::VApp()->LogFile()) << debugdate << Qt::endl;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief One error box at a time, and no nesting.
 *
 * Showing the box runs its own event loop, which processes the messages queued before it - and each of those
 * used to schedule its own box, opened from inside the first. The nesting has no end: the process dies at 255
 * levels with "Too many nested CFRunLoopRuns". A message logged while a box is planned or on screen now stays in
 * the log alone - which is exactly where the report offered by the box takes it from. */
// Touched from the GUI thread only: a message logged from another thread is dispatched to the GUI thread just
// above, and returns there. It needs no protection for that reason.
static bool noisyBoxPending = false;

//---------------------------------------------------------------------------------------------------------------------
/// @brief Writes what the error box offers: the environment of the run, then the session log.
///
/// The report is the only artefact the support can work from, so it carries what a bug report asks for - the
/// version, the system, the screens - and the log, which already contains every message of the session. It never
/// carries the pattern: the log holds its path and its name, not its content.
/// @return the path of the report, or an empty string when it could not be written.
auto WriteDiagnosticReport() -> QString
{
    const QString folder = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (folder.isEmpty() || !QDir().mkpath(folder))
    {
        return {};
    }

    const QString reportPath =
        folder
        + QStringLiteral("/ostara-report-%1.txt")
              .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd-hhmmss")));

    QFile report(reportPath);
    if (!report.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return {};
    }

    QTextStream out(&report);
    out << "Ostara " << AppDisplayVersion() << "\n";
    out << "Qt " << QT_VERSION_STR << "\n";
    out << "System: " << QSysInfo::prettyProductName() << " (" << QSysInfo::currentCpuArchitecture() << ")\n";
    out << "Screens: " << QGuiApplication::screens().size() << "\n";
    out << "Log file: " << VApplication::LogPath() << "\n";
    out << "\n--- session log ---\n";

    // The log is written line by line and flushed, but ask for the tail to be on disk before copying it.
    if (auto *stream = VApplication::VApp()->LogFile())
    {
        stream->flush();
    }

    QFile log(VApplication::LogPath());
    if (log.open(QIODevice::ReadOnly))
    {
        out << QString::fromUtf8(log.readAll());
    }
    else
    {
        out << "(the session log could not be read)\n";
    }

    return reportPath;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The rupture box: one sentence about what happened, and a report the reader can send to the support.
 *
 * The technical text never reaches the reader. A message written for a developer - a parser exception, an
 * identifier, "QWidget::setWindowModified" - teaches nothing about what to do next, and the sentence that does is
 * the same whatever failed. It stays in the log, which the report copies.
 * @return true when the box was really shown. Nothing is displayed while a file dialog is up.
 */
inline auto ShowNoisyMessageBox(QtMsgType type) -> bool
{
    // fixme: trying to make sure there are no save/load dialogs are opened, because error message
    //  during them will lead to crash
    const bool topWinAllowsPop = (QApplication::activeModalWidget() == nullptr) ||
                                 !QApplication::activeModalWidget()->inherits("QFileDialog");

    // Only a rupture earns a modal box, and a warning never does whatever it says: Qt itself raises warnings for a
    // monitor handle it cannot find or for a window title without the "[*]" marker, and in both cases the
    // application goes on behaving exactly the same - the box would only interrupt the work to announce that
    // nothing happened. Warnings go to the log; the ones carrying the [PATTERN MESSAGE] signature go to the
    // "Pattern messages" pane.
    if (!topWinAllowsPop || (type != QtCriticalMsg && type != QtFatalMsg))
    {
        return false;
    }

    // The fatal path shows its box synchronously (see TerminateOnFatalException), and that box pumps a nested
    // event loop of its own. A critical whose box was deferred just before then arrives inside that loop and
    // would open a second box stacked on the first, over an application that is already on its way out. Once
    // termination has begun, only the fatal box is allowed on screen.
    if (type != QtFatalMsg && g_terminatingOnFatalException.loadAcquire() != 0)
    {
        return false;
    }

    auto Tr = [](const char *text) { return QCoreApplication::translate("vNoisyHandler", text); };

    const bool fatal = type == QtFatalMsg;

    QMessageBox messageBox;
    messageBox.setIcon(QMessageBox::Critical);
    messageBox.setWindowTitle(Tr(fatal ? "Fatal error" : "Critical error"));
    messageBox.setText(Tr(fatal ? "Ostara has to close." : "Ostara could not complete this operation."));
    messageBox.setInformativeText(
        Tr("The report holds the technical details the support needs. Save it, then send it to your Ostara contact — "
           "without it, the cause cannot be found."));
    QAbstractButton *const saveButton = messageBox.addButton(Tr("Save the report"), QMessageBox::ActionRole);
    messageBox.addButton(QMessageBox::Close);
    messageBox.setWindowModality(Qt::ApplicationModal);
    messageBox.setModal(true);
#ifndef QT_NO_CURSOR
    QGuiApplication::setOverrideCursor(Qt::ArrowCursor);
#endif
    messageBox.exec();
#ifndef QT_NO_CURSOR
    QGuiApplication::restoreOverrideCursor();
#endif

    if (messageBox.clickedButton() != saveButton)
    {
        return true;
    }

    const QString reportPath = WriteDiagnosticReport();
    const bool saved = !reportPath.isEmpty();
    const QString folder = QFileInfo(saved ? reportPath : VApplication::LogPath()).absolutePath();

    QMessageBox confirmation;
    confirmation.setIcon(saved ? QMessageBox::Information : QMessageBox::Warning);
    confirmation.setWindowTitle(saved ? Tr("Report saved") : Tr("Report not saved"));
    confirmation.setText(saved ? Tr("The report is saved here:") : Tr("The report could not be written."));
    confirmation.setInformativeText(
        saved ? reportPath + "\n\n"
                    + Tr("Attach this file to a message to your Ostara contact. It holds the Ostara version, the "
                         "system, the screens and the session log; it does not contain your pattern.")
              : Tr("The session log is here: %1\n\nSend this file to your Ostara contact.").arg(folder));
    QAbstractButton *const openButton = confirmation.addButton(Tr("Open the folder"), QMessageBox::ActionRole);
    confirmation.addButton(QMessageBox::Close);
    confirmation.setWindowModality(Qt::ApplicationModal);
    confirmation.setModal(true);
    confirmation.exec();

    if (confirmation.clickedButton() == openButton)
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
inline void noisyFailureMsgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (g_terminatingOnFatalException.loadAcquire() != 0)
    {
        // A fatal exception already decided to terminate (see TerminateOnFatalException). Handling a further
        // message here would touch Qt internals that exit()'s own teardown may already have torn down, so drop
        // it instead of crashing on it.
        return;
    }

    // only the GUI thread should display message boxes.  If you are
    // writing a multithreaded application and the error happens on
    // a non-GUI thread, you'll have to queue the message to the GUI
    QCoreApplication  const*instance = QCoreApplication::instance();
    const bool isGuiThread = instance && (QThread::currentThread() == instance->thread());

    if (not isGuiThread)
    {
        auto Handler = [](QtMsgType type, const QMessageLogContext &context, const QString &msg)
        { noisyFailureMsgHandler(type, context, msg); };

        q_dispatch_async_main(Handler, type, context, msg);
        return;
    }

    // Why on earth didn't Qt want to make failed signal/slot connections qWarning?
    if ((type == QtDebugMsg) && msg.contains(QStringLiteral("::connect")))
    {
        type = QtWarningMsg;
    }

#if defined(V_NO_ASSERT)
    // I have decided to hide this annoing message for release builds.
    if ((type == QtWarningMsg) && msg.contains(QStringLiteral("QSslSocket: cannot resolve")))
    {
        type = QtDebugMsg;
    }

    if ((type == QtWarningMsg) && msg.contains(QStringLiteral("setGeometry: Unable to set geometry")))
    {
        type = QtDebugMsg;
    }
#endif // defined(V_NO_ASSERT)

#if defined(Q_OS_MAC)
    // Hide anything that starts with QMacCGContext
    if ((type == QtWarningMsg) && msg.contains(QStringLiteral("QMacCGContext::")))
    {
        type = QtDebugMsg;
    }

    // See issue #568
    if (msg.contains(QStringLiteral("Error receiving trust for a CA certificate")))
    {
        type = QtDebugMsg;
    }

    // Benign Qt-internal noise emitted while a QComboBox popup is torn down (e.g. inside a modal dialog
    // such as Preferences). Harmless, but as warnings they used to pop a modal error box from the
    // message handler; reentering the event loop over the half-destroyed popup then crashed. Keep them
    // out of the user's face.
    if ((type == QtWarningMsg) && (msg.contains(QStringLiteral("parent must be in parent hierarchy")) ||
                                   (msg.contains(QStringLiteral("QBackingStore::flush()")) &&
                                    msg.contains(QStringLiteral("does not have a handle")))))
    {
        type = QtDebugMsg;
    }
#endif

    // Qt reports a broken paint device ("QPainter::begin: Paint device returned engine == 0") and then every
    // call on the dead painter ("QPainter::setPen: Painter not active", ...) as warnings, and the message
    // handler turns a warning into a modal error box. One failed begin() therefore produces a dialog per
    // painter call, per repaint, raised from inside the paint stack itself. Crash report a9979fa1 (1.1.0,
    // b75c9bff3) ends exactly that way: twelve painter warnings, twelve modal boxes, then an access
    // violation in Qt's paint code. The user can act on none of it, so keep it in the log only.
    if ((type == QtWarningMsg) && msg.startsWith("QPainter::"_L1))
    {
        type = QtDebugMsg;
    }

    // Same family, same reason, found the same way: a pen refuses a width that is not a positive finite
    // number ("QPen::setWidth/setWidthF: Setting a pen width that is out of range") and says so. Raised
    // from a paint() — which is where a pen is built — that warning became **one modal box per repaint**,
    // and each box repainted the canvas that raised it: the queue never drained, and the application
    // could neither be used nor closed. The miniature drew a rotated owner with a width derived from
    // `m11()` — zero at 90°, negative past it (see OThumbnailItem::paint); the arithmetic is fixed at
    // the source, and this keeps the log the only place any future slip is heard.
    if ((type == QtWarningMsg) && msg.startsWith("QPen::"_L1))
    {
        type = QtDebugMsg;
    }

    // This qWarning about "Cowardly refusing to send clipboard message to hung application..."
    // is something that can easily happen if you are debugging and the application is paused.
    // As it is so common, not worth popping up a dialog.
    if ((type == QtWarningMsg) && msg.contains(QStringLiteral("QClipboard::event")) &&
        msg.contains(QStringLiteral("Cowardly refusing")))
    {
        type = QtDebugMsg;
    }

    // Annoying warning that we can ignore
    if ((type == QtWarningMsg) && (msg.contains(QStringLiteral("OpenType support missing for")) ||
                                   msg.contains(QStringLiteral("DirectWrite: CreateFontFaceFromHDC() failed (Indicates "
                                                               "an error in an input file such as a font file.)"))))
    {
        type = QtDebugMsg;
    }

    // QObject::connect with null sender/receiver ("invalid nullptr parameter") is a
    // benign side-effect of FullParseFile tearing down and reconnecting spline/path
    // tools. The handler promotes any ::connect message to warning, so without this
    // filter each one pops a modal. Keep them in the log but out of the user's face.
    if ((type == QtWarningMsg) && msg.contains(QStringLiteral("QObject::connect")) &&
        msg.contains(QStringLiteral("invalid nullptr parameter")))
    {
        type = QtDebugMsg;
    }

    // Qt's network stack reports transport hiccups (HTTP/2 GOAWAY, aborted streams, TLS chatter) as
    // warnings, and the message handler turns a warning into a modal error box. The only traffic here is
    // the sanctions check and the crash reporter, which the user never asked for and cannot act on, so the
    // box is pure noise - and popping it from a network callback has crashed before. Valentina's own
    // network code reports its failures itself.
    if ((type == QtWarningMsg) && context.category != nullptr &&
        QLatin1String(context.category).startsWith("qt.network"_L1))
    {
        type = QtDebugMsg;
    }

    // macOS asks Qt for the cells of a table through its accessibility bridge — VoiceOver, Switch
    // Control, or any assistive client querying the application. When that table is empty, Qt warns
    // ("Cell requested for row 0 is out of bounds for table with 0 rows! Resizing table model.") and
    // rebuilds its own representation of the table, which is exactly the recovery the message
    // announces. The user can act on nothing here, but as a warning the handler turns it into a modal
    // box — and since the query follows the window tree, the box lands over whatever dialog is open,
    // Save As first of all. The whole accessibility family stays in the log: it only ever reports
    // Qt's own bridge, never anything Valentina can fix.
    if ((type == QtWarningMsg) && context.category != nullptr &&
        QLatin1String(context.category).startsWith("qt.accessibility"_L1))
    {
        type = QtDebugMsg;
    }

    // Qt's Windows platform plugin announces, when a screen cannot be matched to a monitor handle, that it
    // falls back to 96 DPI ("monitorData: Unable to obtain handle for monitor '\\\\.\\DISPLAY2', defaulting
    // to 96 DPI"). Nothing changes for the reader, and the message says so itself. It stays in the log at
    // debug level, where a genuine DPI problem can still be traced, but it is not a warning about their work.
    if ((type == QtWarningMsg) && msg.contains(QStringLiteral("Unable to obtain handle for monitor")))
    {
        type = QtDebugMsg;
    }

    LogMessageDetails(type, context, msg);

    if (isGuiThread)
    {
        // A warning never takes the slot: no box is shown for it (see ShowNoisyMessageBox), so arming the slot
        // would only swallow the critical logged right behind it in the same event-loop turn.
        if ((type == QtCriticalMsg || type == QtFatalMsg) && VApplication::IsGUIMode() && !noisyBoxPending)
        {
            auto ShowMessageBox = [type]()
            {
                ShowNoisyMessageBox(type);
                // Cleared once the box is closed, not before: a message logged while it is on screen must not
                // open a second box nested inside it.
                noisyBoxPending = false;
            };

            noisyBoxPending = true;

            if (type == QtFatalMsg)
            {
                // A fatal message aborts right below. The dialog must be shown synchronously here or it
                // will never appear.
                ShowMessageBox();
            }
            else
            {
                // Never pop a modal dialog synchronously from whatever call stack logged the message (e.g. in
                // the middle of a QGraphicsItem geometry rebuild or a tool dialog's Apply handler): the nested
                // event loop from QMessageBox::exec() then pumps events over not-yet-consistent state, which
                // has repeatedly caused crashes in this codebase (see e.g. commits 714cd5d89, 06f755ed6).
                // Defer to the next event loop iteration, where the stack that logged the message has fully
                // unwound.
                QTimer::singleShot(0, qApp, ShowMessageBox);
            }
        }

        if (QtFatalMsg == type)
        {
            abort();
        }
    }
    else
    {
        if (type != QtDebugMsg)
        {
            abort(); // be NOISY unless overridden!
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VApplication constructor.
 * @param argc number arguments.
 * @param argv command line.
 */
VApplication::VApplication(int &argc, char **argv)
  : VAbstractValApplication(argc, argv)
{
    setApplicationDisplayName(QStringLiteral(VER_PRODUCTNAME_STR));
    setApplicationName(QStringLiteral(VER_INTERNALNAME_STR));
    setOrganizationName(QStringLiteral(VER_COMPANYNAME_STR));
    setOrganizationDomain(QStringLiteral(VER_COMPANYDOMAIN_STR));
    // Setting the Application version
    // The version Qt reports, and therefore the one the welcome screen shows. The build
    // string keeps its fourth field — see AppDisplayVersion — and the log below still
    // writes it in full, so a build stays identifiable where that matters.
    setApplicationVersion(AppDisplayVersion());
    // making sure will create new instance...just in case we will ever do 2 objects of VApplication
    VCommandLine::Reset();

    // ⚠️ **Le moteur de retraduction, installé une fois pour toute l'application.** Il écoute
    // `LanguageChange` sur l'objet application et rejoue les textes que le poseur a mémorisés sur
    // leurs widgets (voir `otranslation.h`). Sans lui, chaque classe doit se souvenir de réécrire
    // ses propres libellés — c'est ce qui laissait des surfaces entières dans la langue du premier
    // affichage, dock Options en tête.
    OTranslation::InstallWatcher();

}

//---------------------------------------------------------------------------------------------------------------------
VApplication::~VApplication()
{
    qCDebug(vApp, "Application closing.");

    // The close-event statistic is flushed from AboutToQuit(), while the event loop is still alive and
    // all objects are intact. We must not flush it here: doing so pumps a nested event loop during
    // application teardown, which delivered the network reply into half-destroyed objects and crashed
    // in VGAnalyticsWorker::SendAnalyticsFinished().

    qInstallMessageHandler(nullptr); // Resore the message handler
    delete m_trVars;
    VCommandLine::Reset();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief NewValentina start Valentina in new process, send path to pattern file in argument.
 * @param fileName path to pattern file.
 */
void VApplication::NewValentina(const QString &fileName)
{
    qCDebug(vApp, "Open new detached process.");
    if (fileName.isEmpty())
    {
        qCDebug(vApp, "New process without arguments. program = %s",
                qUtf8Printable(QCoreApplication::applicationFilePath()));
        // Path can contain spaces.
        if (QProcess::startDetached(QCoreApplication::applicationFilePath(), QStringList()))
        {
            qCDebug(vApp, "The process was started successfully.");
        }
        else
        {
            qCWarning(vApp, "Could not run process. The operation timed out or an error occurred.");
        }
    }
    else
    {
        const auto run = QStringLiteral("\"%1\" \"%2\"").arg(QCoreApplication::applicationFilePath(), fileName);
        qCDebug(vApp, "New process with arguments. program = %s", qUtf8Printable(run));
        if (QProcess::startDetached(QCoreApplication::applicationFilePath(), QStringList{fileName}))
        {
            qCDebug(vApp, "The process was started successfully.");
        }
        else
        {
            qCWarning(vApp, "Could not run process. The operation timed out or an error occurred.");
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief notify Reimplemented from QApplication::notify().
 * @param receiver receiver.
 * @param event event.
 * @return value that is returned from the receiver's event handler.
 */
// reimplemented from QApplication so we can throw exceptions in slots
auto VApplication::notify(QObject *receiver, QEvent *event) -> bool
{
    // Un menu paraît : ses entrées reçoivent nos glyphes et la variante de survol. Les menus
    // contextuels des champs texte sont construits par Qt lui-même, sans qu'on puisse les
    // recevoir autrement — on ne les remplace pas, on les habille au moment où ils s'ouvrent.
    if (event->type() == QEvent::Show)
    {
        if (auto *menu = qobject_cast<QMenu *>(receiver); menu != nullptr)
        {
            OMenuGlyph::StyleStandardMenu(menu);
        }
    }

    // Every catch below must terminate via TerminateOnFatalException(), not a bare exit() call. Inside a
    // QCoreApplication-derived member function, unqualified exit() resolves to the inherited static slot
    // QCoreApplication::exit(int), which does not terminate the process: it just flags the nearest event
    // loop(s) to quit and returns control here, running on data already known bad. TerminateOnFatalException()
    // calls the qualified ::exit() and also flags the message handler to drop any message Qt's own teardown
    // emits afterward, which has separately crashed (see its definition).
    try
    {
        return QApplication::notify(receiver, event);
    }
    catch (const VExceptionObjectError &e)
    {
        qCCritical(vApp, "%s\n\n%s\n\n%s",
                   qUtf8Printable(tr("Error parsing file. Program will be terminated.")), //-V807
                   qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        TerminateOnFatalException(V_EX_DATAERR);
    }
    catch (const VExceptionBadId &e)
    {
        qCCritical(vApp, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error bad id. Program will be terminated.")),
                   qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        TerminateOnFatalException(V_EX_DATAERR);
    }
    catch (const VExceptionConversionError &e)
    {
        qCCritical(vApp, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error can't convert value. Program will be terminated.")),
                   qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        TerminateOnFatalException(V_EX_DATAERR);
    }
    catch (const VExceptionEmptyParameter &e)
    {
        qCCritical(vApp, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error empty parameter. Program will be terminated.")),
                   qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        TerminateOnFatalException(V_EX_DATAERR);
    }
    catch (const VExceptionWrongId &e)
    {
        qCCritical(vApp, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error wrong id. Program will be terminated.")),
                   qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        TerminateOnFatalException(V_EX_DATAERR);
    }
    catch (const VExceptionToolWasDeleted &e)
    {
        // A developer invariant, not something the reader can act on: the object was deleted while a tool still
        // held it. The termination that follows is announced by the fatal box (see TerminateOnFatalException).
        qCDebug(vApp, "%s\n\n%s\n\n%s",
                qUtf8Printable(QStringLiteral("Unhandled deleting tool. The object is used after being deleted")),
                   qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        TerminateOnFatalException(V_EX_DATAERR);
    }
    catch (const VExceptionInvalidNotch &e)
    {
        qCCritical(vApp, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Invalid notch.")), qUtf8Printable(e.ErrorMessage()),
                   qUtf8Printable(e.DetailedInformation()));
        TerminateOnFatalException(V_EX_DATAERR);
    }
    catch (const VException &e)
    {
        qCCritical(vApp, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Something's wrong!!")), qUtf8Printable(e.ErrorMessage()),
                   qUtf8Printable(e.DetailedInformation()));
        return true;
    }
    catch (const qmu::QmuParserWarning &e)
    {
        qCCritical(vApp, "%s", qUtf8Printable(tr("Formula warning: %1. Program will be terminated.").arg(e.GetMsg())));
        TerminateOnFatalException(V_EX_DATAERR);
    }
    // These last two cases are special. I found that we can't show here a modal dialog with an error message.
    // Somehow program doesn't wait until an error dialog will be closed. But if ignore the exception the program will
    // hang.
    catch (const qmu::QmuParserError &e)
    {
        qCCritical(vApp, "%s", qUtf8Printable(tr("Parser error: %1. Program will be terminated.").arg(e.GetMsg())));
        TerminateOnFatalException(V_EX_DATAERR);
    }
    catch (std::exception &e)
    {
        qCCritical(vApp, "%s", qUtf8Printable(tr("Exception thrown: %1. Program will be terminated.").arg(e.what())));
        TerminateOnFatalException(V_EX_SOFTWARE);
    }
    return false;
}


//---------------------------------------------------------------------------------------------------------------------
auto VApplication::LogPath() -> QString
{
    // Keep in sync with VCrashPaths::GetAttachmentPath, which builds the name from the application name
    // ("ostara"): while this said "valentina-pid", the crash report's attached log never existed.
    return QStringLiteral("%1/ostara-pid%2.log").arg(LogDirPath()).arg(applicationPid());
}

//---------------------------------------------------------------------------------------------------------------------
void VApplication::BeginLogging()
{
    VlpCreateLock(m_lockLog, LogPath(), []() { return new QFile(LogPath()); });

    if (m_lockLog->IsLocked())
    {
        if (m_lockLog->GetProtected()->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        {
            m_out.reset(new QTextStream(m_lockLog->GetProtected().data()));
            qInstallMessageHandler(noisyFailureMsgHandler);
            qCDebug(vApp, "Log file %s was locked.", qUtf8Printable(LogPath()));
        }
        else
        {
            qCDebug(vApp, "Error opening log file \'%s\'. All debug output redirected to console.",
                    qUtf8Printable(LogPath()));
        }
    }
    else
    {
        qCDebug(vApp, "Failed to lock %s", qUtf8Printable(LogPath()));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VApplication::InitOptions()
{
    OpenSettings();

    StartLogging();

    // Four lines on the console, always. The console is silent otherwise (see ConsoleVerbose), and a terminal that
    // says nothing at all leaves the user unable to tell the application started -- nor where the log is, which is
    // the first thing a bug report needs. Info, not debug: info is what the console keeps.
    qInfo("Version: %s", qUtf8Printable(AppVersionStr()));
    qInfo("Build revision: %s", qUtf8Printable(QStringLiteral(VCS_REPO_STATE_REVISION)));
    qInfo("Log file: %s", qUtf8Printable(LogPath()));
    qInfo("Process ID: %s", qUtf8Printable(QString::number(applicationPid())));

    // The rest of the banner is only ever read to diagnose a crash report: it stays in the file.
    qCDebug(vApp, "%s", qUtf8Printable(buildCompatibilityString()));
    qCDebug(vApp, "Built on %s at %s", __DATE__, __TIME__);
    qCDebug(vApp, "Command-line arguments: %s", qUtf8Printable(arguments().join(QLatin1Char(' '))));

    LoadTranslation(QString()); // By default the console version uses system locale

    // Create command line parser after loading translations to show localized version.
    VCommandLine::Get(*this);

    CheckSystemLocale();

    if (IsGUIMode())
    {
        QTimer::singleShot(0, this,
                           []()
                           {
                               VGAnalytics::CheckCountryCodeAsync(
                                   [](const QString &country)
                                   {
                                       if (country == "ru"_L1 || country == "by"_L1 || country == "ir"_L1)
                                       {
                                           QCoreApplication::exit();
                                       }
                                   });
                           });
    }

    if (VApplication::IsGUIMode()) // By default console version uses system locale
    {
        LoadTranslation(ValentinaSettings()->GetLocale());

        VTheme::InitApplicationStyle(); // set Fusion + VApplicationStyle once
        VTheme::Instance()->ResetThemeSettings();

        m_shortcutManager = new VValentinaShortcutManager(this);
    }

    // VGAnalyticsLib: Init removed — no GA4 tracking (sanctions check kept above)
}

//---------------------------------------------------------------------------------------------------------------------
void VApplication::StartDetachedProcess(const QString &program, const QStringList &arguments)
{
#if !defined(Q_OS_MACOS)
    const QString workingDirectory = QFileInfo(program).absoluteDir().absolutePath();
    QProcess::startDetached(program, arguments, workingDirectory);
#else
    if (not program.endsWith(".app"_L1))
    {
        const QString workingDirectory = QFileInfo(program).absoluteDir().absolutePath();
        QProcess::startDetached(program, arguments, workingDirectory);
    }
    else
    {
        QStringList openArguments{"-n", QStringLiteral("/Applications/%1").arg(program)};
        if (not arguments.isEmpty())
        {
            openArguments.append("--args"_L1);
            openArguments += arguments;
        }

        QProcess::startDetached("open"_L1, openArguments);
    }
#endif
}

//---------------------------------------------------------------------------------------------------------------------
auto VApplication::LabelLanguages() -> QStringList
{
    QStringList list{
        "de", // German
        "en", // English
        "fr", // French
        "ru", // Russian
        "uk", // Ukrainian
        "hr", // Croatian
        "sr", // Serbian
        "bs", // Bosnian
        "cs"  // Czech
    };
    return list;
}

//---------------------------------------------------------------------------------------------------------------------
void VApplication::StartLogging()
{
    if (CreateLogDir())
    {
        BeginLogging();
        ClearOldLogs();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VApplication::LogFile() -> QTextStream *
{
    return m_out.get();
}

//---------------------------------------------------------------------------------------------------------------------
auto VApplication::KnownMeasurementsDatabase() -> VKnownMeasurementsDatabase *
{
    if (m_knownMeasurementsDatabase == nullptr)
    {
        m_knownMeasurementsDatabase = new VKnownMeasurementsDatabase();

        RestartKnownMeasurementsDatabaseWatcher();
    }

    if (!m_knownMeasurementsDatabase->IsPopulated())
    {
        m_knownMeasurementsDatabase->PopulateMeasurementsDatabase();
    }

    return m_knownMeasurementsDatabase;
}

//---------------------------------------------------------------------------------------------------------------------
void VApplication::RestartKnownMeasurementsDatabaseWatcher()
{
    if (m_knownMeasurementsDatabase != nullptr)
    {
        delete m_knownMeasurementsDatabaseWatcher;
        m_knownMeasurementsDatabaseWatcher = new QFileSystemWatcher({settings->GetPathKnownMeasurements()}, this);
        connect(m_knownMeasurementsDatabaseWatcher, &QFileSystemWatcher::directoryChanged, this,
                &VApplication::RepopulateMeasurementsDatabase);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VApplication::TrVars() -> const VTranslateVars *
{
    return m_trVars;
}

//---------------------------------------------------------------------------------------------------------------------
void VApplication::InitTrVars()
{
    if (m_trVars != nullptr)
    {
        m_trVars->Retranslate();
    }
    else
    {
        m_trVars = new VTranslateVars();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VApplication::event(QEvent *e) -> bool
{
    switch (e->type())
    {
        // In Mac OS X the QFileOpenEvent event is generated when user perform "Open With" from Finder (this event is
        // Mac specific).
        case QEvent::FileOpen:
        {
            auto *fileOpenEvent = dynamic_cast<QFileOpenEvent *>(e);
            if (const QString macFileOpen = fileOpenEvent->file(); not macFileOpen.isEmpty())
            {
                if (auto *window = qobject_cast<MainWindow *>(mainWindow); window != nullptr)
                {
                    window->LoadPattern(macFileOpen); // open file in existing window
                }
                return true;
            }
            break;
        }
#if defined(Q_OS_MAC)
        case QEvent::ApplicationStateChange:
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            if (static_cast<QApplicationStateChangeEvent *>(e)->applicationState() == Qt::ApplicationActive)
            {
                if (mainWindow && not mainWindow->isMinimized())
                {
                    mainWindow->show();
                }
                return true;
            }
            break;
#endif // defined(Q_OS_MAC)
        default:
            return VAbstractApplication::event(e);
    }
    return VAbstractApplication::event(e);
}

//---------------------------------------------------------------------------------------------------------------------
void VApplication::AboutToQuit()
{
    // If try to use the method QApplication::exit program can't sync settings and show warning about QApplication
    // instance. Solution is to call sync() before quit.
    // Connect this slot with VApplication::aboutToQuit.
    Settings()->sync();

    // Flush the close-event statistic while the event loop is still alive and all objects are intact.
    // Doing this here (instead of in the destructor) avoids pumping a nested event loop during teardown.
    SendAppCloseStatistic();
}

//---------------------------------------------------------------------------------------------------------------------
void VApplication::SendAppCloseStatistic()
{
    // VGAnalyticsLib removed — no tracking
}

//---------------------------------------------------------------------------------------------------------------------
void VApplication::RepopulateMeasurementsDatabase(const QString &path)
{
    Q_UNUSED(path)
    if (m_knownMeasurementsDatabase != nullptr)
    {
        QFuture<void> const future =
            QtConcurrent::run([this]() { m_knownMeasurementsDatabase->PopulateMeasurementsDatabase(); });
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VApplication::KnownMeasurementsPathChanged(const QString &oldPath, const QString &newPath)
{
    if (oldPath != newPath && m_knownMeasurementsDatabase != nullptr)
    {
        RestartKnownMeasurementsDatabaseWatcher();
        RepopulateMeasurementsDatabase(newPath);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VApplication::IsGUIMode() -> bool
{
    return (VCommandLine::instance != nullptr) && VCommandLine::instance->IsGuiEnabled();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief IsAppInGUIMode little hack that allow to have access to application state from VAbstractApplication class.
 */
auto VApplication::IsAppInGUIMode() const -> bool
{
    return IsGUIMode();
}

//---------------------------------------------------------------------------------------------------------------------
auto VApplication::IsPedantic() const -> bool
{
    return (VCommandLine::instance != nullptr) && VCommandLine::instance->IsPedantic();
}

//---------------------------------------------------------------------------------------------------------------------
auto VApplication::GlobalCurveApproximationScale() -> qreal
{
    if (VCommandLine::instance != nullptr)
    {
        if (qreal const scale = VCommandLine::instance->CurveApproximationScale(); !qFuzzyIsNull(scale))
        {
            return scale;
        }
    }
    return defGlobalCurveApproximationScale;
}

//---------------------------------------------------------------------------------------------------------------------
void VApplication::OpenSettings()
{
    VAbstractValApplication::OpenSettings();
    connect(settings, &VValentinaSettings::KnownMeasurementsPathChanged, this,
            &VApplication::KnownMeasurementsPathChanged);
}

//---------------------------------------------------------------------------------------------------------------------
auto VApplication::VApp() -> VApplication *
{
    return qobject_cast<VApplication *>(QCoreApplication::instance());
}

//---------------------------------------------------------------------------------------------------------------------
auto VApplication::CommandLine() -> VCommandLinePtr
{
    return VCommandLine::instance;
}
