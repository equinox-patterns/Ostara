/************************************************************************
 **
 **  @file   tst_fullscreen.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/11/2026
 **
 **  @brief  Essai du plein écran que l'application possède, et du filet qui le rend après une dialogue.
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

#include "tst_fullscreen.h"

#include "core/ofullscreen.h"
#include "core/ofullscreenkeeper.h"
#include "vmisc/testvapplication.h"
#include "vmisc/vabstractvalapplication.h"

#include <QCloseEvent>
#include <QDialog>
#include <QMainWindow>
#include <QRect>
#include <QScreen>
#include <QTest>

namespace
{
//---------------------------------------------------------------------------------------------------------------------
// Une dialogue qui refuse de se fermer — le cas « modifications non enregistrées / Annuler ». Elle est
// là pour prouver que le filet ne rend pas l'écran sur un Close refusé : il attend que la dialogue ait
// réellement quitté l'écran.
class StubbornDialog final : public QDialog
{
public:
    using QDialog::QDialog;

protected:
    void closeEvent(QCloseEvent *event) override { event->ignore(); }
};
} // namespace

TST_FullScreen::TST_FullScreen(QObject *parent)
  : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
// La prise d'écran native : c'est celle de macOS, de Windows et de Wayland, et c'est l'état que le
// système pose lui-même. Ce que l'essai vérifie, c'est le contrat du module et non celui du système :
// après la sortie, la fenêtre n'est plus en plein écran et ne se croit plus prise.
void TST_FullScreen::TheNativeTakeoverFillsTheScreen()
{
    QMainWindow window;
    window.resize(500, 400);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window, 5000));
    QVERIFY(!OFullScreen::IsActive(&window));

    QVERIFY(OFullScreen::Enter(&window, OFullScreen::Takeover::Native));
    QTRY_VERIFY_WITH_TIMEOUT(window.isFullScreen(), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(OFullScreen::IsActive(&window), 5000);

    OFullScreen::Exit(&window);
    QVERIFY(!OFullScreen::IsActive(&window));
    QTRY_VERIFY_WITH_TIMEOUT(!window.isFullScreen(), 5000);
}

//---------------------------------------------------------------------------------------------------------------------
// La prise d'écran sans cadre : c'est celle du X11, où le gestionnaire de fenêtres possède l'état
// plein écran et peut le reprendre. Ici l'état n'appartient à personne — le cadre tombe, la géométrie
// de l'écran est prise — donc rien ne peut être repris. La taille est vérifiée, pas la position : un
// gestionnaire de fenêtres garde le droit de décaler une fenêtre pour ne pas passer sous une barre.
void TST_FullScreen::TheFramelessTakeoverCoversTheScreen()
{
    QMainWindow window;
    window.resize(500, 400);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window, 5000));

    const QRect before = window.geometry();
    const QScreen *const screen = window.screen();
    QVERIFY(screen != nullptr);

    QVERIFY(OFullScreen::Enter(&window, OFullScreen::Takeover::Frameless));
    QVERIFY(OFullScreen::IsActive(&window));
    QVERIFY(window.windowFlags().testFlag(Qt::FramelessWindowHint));
    QTRY_COMPARE_WITH_TIMEOUT(window.geometry().size(), screen->geometry().size(), 5000);

    OFullScreen::Exit(&window);
    QVERIFY(!window.windowFlags().testFlag(Qt::FramelessWindowHint));
    QVERIFY(!OFullScreen::IsActive(&window));
    QTRY_COMPARE_WITH_TIMEOUT(window.geometry(), before, 5000);
}

//---------------------------------------------------------------------------------------------------------------------
// La géométrie rendue n'est pas seulement une taille : une fenêtre agrandie avant de prendre l'écran
// doit revenir agrandie, sinon la sortie du plein écran la laisse petite au milieu de l'écran.
void TST_FullScreen::TheMaximizedWindowComesBackMaximized()
{
    QMainWindow window;
    window.resize(500, 400);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window, 5000));

    window.showMaximized();
    QTRY_VERIFY_WITH_TIMEOUT(window.isMaximized(), 5000);

    QVERIFY(OFullScreen::Enter(&window, OFullScreen::Takeover::Native));
    QTRY_VERIFY_WITH_TIMEOUT(window.isFullScreen(), 5000);

    OFullScreen::Exit(&window);
    QTRY_VERIFY_WITH_TIMEOUT(!window.isFullScreen(), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(window.isMaximized(), 5000);
}

//---------------------------------------------------------------------------------------------------------------------
// Ce que le système a repris n'est plus pris : Échap sous macOS sort du plein écran sans passer par
// l'entrée du menu. La case se décoche, et l'entrée suivante reprend l'écran au lieu de se croire
// déjà dessus.
void TST_FullScreen::TheStateTheSystemTookAwayIsNotActive()
{
    QMainWindow window;
    window.resize(500, 400);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window, 5000));

    QVERIFY(OFullScreen::Enter(&window, OFullScreen::Takeover::Native));
    QTRY_VERIFY_WITH_TIMEOUT(window.isFullScreen(), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(OFullScreen::IsActive(&window), 5000);

    window.showNormal();
    QTRY_VERIFY_WITH_TIMEOUT(!window.isFullScreen(), 5000);
    QVERIFY(!OFullScreen::IsActive(&window));

    QVERIFY(OFullScreen::Enter(&window, OFullScreen::Takeover::Native));
    QTRY_VERIFY_WITH_TIMEOUT(window.isFullScreen(), 5000);
}

//---------------------------------------------------------------------------------------------------------------------
// Le filet : le plein écran est perdu pendant qu'une dialogue est ouverte — ce que fait le
// gestionnaire de fenêtres sous X11 — et il est rendu quand la dialogue se ferme, une fois.
void TST_FullScreen::TheKeeperGivesTheScreenBackAfterTheDialog()
{
    QMainWindow window;
    window.resize(500, 400);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window, 5000));

    VAbstractValApplication::VApp()->setMainWindow(&window);
    OFullScreenKeeper::Install();

    window.showFullScreen();
    QTRY_VERIFY_WITH_TIMEOUT(window.isFullScreen(), 5000);

    QDialog dialog(&window);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog, 5000));

    // Le compositeur retire le plein écran alors que la dialogue est à l'écran : c'est l'ordre des
    // événements décrit par les gestionnaires de fenêtres, la dialogue est connue avant l'état.
    window.showNormal();
    QTRY_VERIFY_WITH_TIMEOUT(!window.isFullScreen(), 5000);

    dialog.close();
    QTRY_VERIFY_WITH_TIMEOUT(OFullScreen::IsActive(&window), 5000);

}

//---------------------------------------------------------------------------------------------------------------------
// Le filet ne prend jamais l'écran de lui-même : une fenêtre qui n'était pas en plein écran le reste
// après une dialogue, sinon chaque boîte de message mettrait l'application en plein écran.
void TST_FullScreen::TheKeeperLeavesAWindowAloneItNeverTook()
{
    QMainWindow window;
    window.resize(500, 400);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window, 5000));

    VAbstractValApplication::VApp()->setMainWindow(&window);
    OFullScreenKeeper::Install();
    QVERIFY(!window.isFullScreen());

    QDialog dialog(&window);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog, 5000));
    dialog.close();
    QTest::qWait(50);

    QVERIFY(!window.isFullScreen());
    QVERIFY(!OFullScreen::IsActive(&window));

}

//---------------------------------------------------------------------------------------------------------------------
// Une dialogue en amène une autre : la première qui se ferme ne doit pas rendre l'écran, sinon la
// fenêtre clignote une fois par dialogue. Le filet attend la dernière.
void TST_FullScreen::TheKeeperWaitsForTheLastDialog()
{
    QMainWindow window;
    window.resize(500, 400);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window, 5000));

    VAbstractValApplication::VApp()->setMainWindow(&window);
    OFullScreenKeeper::Install();

    window.showFullScreen();
    QTRY_VERIFY_WITH_TIMEOUT(window.isFullScreen(), 5000);

    QDialog first(&window);
    first.show();
    QVERIFY(QTest::qWaitForWindowExposed(&first, 5000));

    QDialog second(&window);
    second.show();
    QVERIFY(QTest::qWaitForWindowExposed(&second, 5000));

    window.showNormal();
    QTRY_VERIFY_WITH_TIMEOUT(!window.isFullScreen(), 5000);

    first.close();
    QTest::qWait(50);
    QVERIFY(!window.isFullScreen());

    second.close();
    QTRY_VERIFY_WITH_TIMEOUT(OFullScreen::IsActive(&window), 5000);

}

//---------------------------------------------------------------------------------------------------------------------
// `QVERIFY` et les `QTRY_*` sortent du cas d'essai au premier échec : la dernière ligne d'un test qui
// rend la fenêtre à l'application ne serait alors jamais atteinte, et l'essai suivant hériterait d'un
// pointeur mort. Le nettoyage vit donc ici, où il passe dans tous les cas.
void TST_FullScreen::cleanup()
{
    VAbstractValApplication::VApp()->setMainWindow(nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
// Une dialogue qui refuse de se fermer ne rend pas l'écran. Le filtre d'événements passe avant la
// dialogue : décider sur Close reviendrait à rendre le plein écran sous une boîte encore ouverte. C'est
// Hide qui fait foi, et lui seul arrive quand la dialogue est vraiment partie.
void TST_FullScreen::TheKeeperWaitsUntilTheDialogReallyLeaves()
{
    QMainWindow window;
    window.resize(500, 400);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window, 5000));

    VAbstractValApplication::VApp()->setMainWindow(&window);
    OFullScreenKeeper::Install();

    window.showFullScreen();
    QTRY_VERIFY_WITH_TIMEOUT(window.isFullScreen(), 5000);

    StubbornDialog dialog(&window);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog, 5000));

    window.showNormal();
    QTRY_VERIFY_WITH_TIMEOUT(!window.isFullScreen(), 5000);

    dialog.close();
    QTest::qWait(50);
    QVERIFY(dialog.isVisible());
    QVERIFY(!window.isFullScreen());

    dialog.hide();
    QTRY_VERIFY_WITH_TIMEOUT(OFullScreen::IsActive(&window), 5000);
}

//---------------------------------------------------------------------------------------------------------------------
// La sortie demandée par l'utilisatrice ne se défait pas : plein écran, une dialogue ouverte, elle
// décoche « Plein écran » — la fermeture de la dialogue ne doit pas la remettre en plein écran. C'est
// la seule chose que le filet ne peut pas deviner tout seul, d'où `IsGivingBack()`.
void TST_FullScreen::TheChoiceToLeaveIsNotUndoneByADialog()
{
    QMainWindow window;
    window.resize(500, 400);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window, 5000));

    VAbstractValApplication::VApp()->setMainWindow(&window);
    OFullScreenKeeper::Install();

    QVERIFY(OFullScreen::Enter(&window));
    QTRY_VERIFY_WITH_TIMEOUT(OFullScreen::IsActive(&window), 5000);

    QDialog dialog(&window);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog, 5000));

    OFullScreen::Exit(&window);
    QTRY_VERIFY_WITH_TIMEOUT(!OFullScreen::IsActive(&window), 5000);

    dialog.close();
    QTRY_VERIFY_WITH_TIMEOUT(!OFullScreen::IsActive(&window), 5000);
    QVERIFY(!window.isFullScreen());
}

//---------------------------------------------------------------------------------------------------------------------
// Les chemins d'échec ne doivent ni planter, ni mentir : fenêtre nulle, sortie sans prise, prise deux
// fois, sortie deux fois.
void TST_FullScreen::TheFailureCasesAreHarmless()
{
    QVERIFY(!OFullScreen::Enter(nullptr));
    QVERIFY(!OFullScreen::IsActive(nullptr));
    OFullScreen::Exit(nullptr);

    QMainWindow window;
    window.resize(500, 400);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window, 5000));

    OFullScreen::Exit(&window);
    QVERIFY(!OFullScreen::IsActive(&window));

    QVERIFY(OFullScreen::Enter(&window));
    QTRY_VERIFY_WITH_TIMEOUT(OFullScreen::IsActive(&window), 5000);
    QVERIFY(OFullScreen::Enter(&window));

    OFullScreen::Exit(&window);
    QTRY_VERIFY_WITH_TIMEOUT(!OFullScreen::IsActive(&window), 5000);

    OFullScreen::Exit(&window);
    QVERIFY(!OFullScreen::IsActive(&window));
}

//---------------------------------------------------------------------------------------------------------------------
// Une fenêtre déjà en plein écran hors du module — le bouton vert de macOS — n'est pas piégée : prendre
// l'écran puis le rendre doit laisser la fenêtre normale. Sans ça, la géométrie mémorisée aurait été
// celle du plein écran, et la sortie y ramènerait à chaque fois.
void TST_FullScreen::AWindowAlreadyFullScreenIsNotTrapped()
{
    QMainWindow window;
    window.resize(500, 400);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window, 5000));

    window.showFullScreen();
    QTRY_VERIFY_WITH_TIMEOUT(window.isFullScreen(), 5000);
    QVERIFY(!OFullScreen::IsActive(&window));

    QVERIFY(OFullScreen::Enter(&window));
    OFullScreen::Exit(&window);
    QTRY_VERIFY_WITH_TIMEOUT(!window.isFullScreen(), 5000);
    QVERIFY(!OFullScreen::IsActive(&window));
}

//---------------------------------------------------------------------------------------------------------------------
// Un `QApplication` nu ne suffit pas : le filet trouve la fenêtre à rendre par `VApp()`, qui rend un
// pointeur nul sur une application ordinaire. C'est donc l'application de test qui le construit.
auto main(int argc, char **argv) -> int
{
    TestVApplication const app(argc, argv);
    TST_FullScreen test;
    return QTest::qExec(&test, argc, argv);
}
