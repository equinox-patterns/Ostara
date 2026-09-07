/************************************************************************
 **
 **  @file   dialogplacelabel.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   14 10, 2017
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2017 Valentina project
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
#ifndef DIALOGPLACELABEL_H
#define DIALOGPLACELABEL_H

#include <QVector>

#include "../dialogtool.h"

class QCheckBox;
class QComboBox;
class QGridLayout;
class QLabel;
class QLineEdit;
class QTimer;
class QToolButton;

class VPlaceLabelItem;

class DialogPlaceLabel final : public DialogTool
{
    Q_OBJECT // NOLINT

public:
    explicit DialogPlaceLabel(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent = nullptr);
    ~DialogPlaceLabel() override;

    void EnbleShowMode(bool disable);

    auto GetCenterPoint() const -> quint32;
    void SetCenterPoint(quint32 id);

    /** @brief Ancres de construction retenues pendant la collecte multi-points. */
    auto GetCenterPoints() const -> QVector<quint32>;

    /** @brief Fin de la collecte (Entrée) : ouvre la dialog modale si des points sont retenus. */
    void ShowDialog(bool click) override;

    auto GetLabelType() const -> PlaceLabelType;
    void SetLabelType(PlaceLabelType type);

    auto GetWidth() const -> QString;
    void SetWidth(const QString &value);

    auto GetHeight() const -> QString;
    void SetHeight(const QString &value);

    auto GetAngle() const -> QString;
    void SetAngle(const QString &value);

    auto GetPieceId() const -> quint32;
    void SetPieceId(quint32 id);

    auto GetFormulaVisible() const -> QString;
    void SetFormulaVisible(const QString &formula);

    auto IsNotMirrored() const -> bool;
    void SetNotMirrored(bool value);

    void SetPiecesList(const QVector<quint32> &list) override;

    void CheckDependencyTreeComplete() override;

public slots:
    void ChosenObject(quint32 id, const SceneObject &type) override;

protected:
    void ShowVisualization() override;
    void changeEvent(QEvent *event) override;
    auto IsValid() const -> bool override;

private slots:
    void EvalWidth();
    void EvalHeight();
    void EvalAngle();
    void EvalVisible();

    void FXWidth();
    void FXHeight();
    void FXAngle();
    void FXVisible();

private:
    Q_DISABLE_COPY_MOVE(DialogPlaceLabel) // NOLINT
    bool m_showMode;

    /** @brief Ancre du repère édité (mode édition, verrouillée — pas de champ Point). */
    quint32 m_centerPoint{NULL_ID};

    /** @brief Points retenus pendant la collecte (mode création lot). */
    QVector<quint32> m_collectedPoints{};

    QTimer *timerAngle;
    QTimer *timerWidth;
    QTimer *timerHeight;
    QTimer *m_timerVisible;

    // ---- Widgets (fenêtre unique, réplique du layout du PB details) ----
    QLabel *m_pieceLabel{nullptr};
    QComboBox *m_pieceCombo{nullptr};

    QLabel *m_widthLabel{nullptr};
    QLineEdit *m_widthEdit{nullptr};
    QToolButton *m_widthFx{nullptr};

    QLabel *m_heightLabel{nullptr};
    QLineEdit *m_heightEdit{nullptr};
    QToolButton *m_heightFx{nullptr};

    QLabel *m_angleLabel{nullptr};
    QLineEdit *m_angleEdit{nullptr};
    QToolButton *m_angleFx{nullptr};

    QLabel *m_typeLabel{nullptr};
    QComboBox *m_typeCombo{nullptr};

    QLabel *m_visibleLabel{nullptr};
    QLineEdit *m_visibleEdit{nullptr};
    QToolButton *m_visibleFx{nullptr};
    QLabel *m_visibleHint{nullptr};

    QCheckBox *m_notMirroredCheck{nullptr};

    bool m_flagPoint;
    bool m_flagWidth;
    bool m_flagHeight;
    bool m_flagAngle;
    bool m_flagFormulaVisible;
    bool m_flagError;

    void BuildUi();
    void RetranslateUi();
    void SetFxIcons();
    void FillPlaceLabelTypes();
    void EvalFormulaField(QLineEdit *edit, QLabel *label, bool &flag, const QString &postfix,
                          const bool checkLessThanZero);

    void CheckPieces();
    void CheckPoint();
};

//---------------------------------------------------------------------------------------------------------------------
inline auto DialogPlaceLabel::IsValid() const -> bool
{
    // Mode édition (m_showMode) : la validité repose sur le point verrouillé du repère édité.
    // Mode création en lot : pas de point dans la combo — la validité repose sur la collecte non vide.
    const bool pointOk = m_showMode ? m_flagPoint : not m_collectedPoints.isEmpty();
    return pointOk && m_flagError && m_flagWidth && m_flagHeight && m_flagAngle && m_flagFormulaVisible;
}

#endif // DIALOGPLACELABEL_H
