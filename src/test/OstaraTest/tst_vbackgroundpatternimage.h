/************************************************************************
 **
 **  @file   tst_vbackgroundpatternimage.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/10/2026
 **
 **  @brief  Ce qu'une image de fond doit valider pour entrer dans un patron : un SVG est accepté quand son
 **          canvas est raisonnable, refusé quand il est démesuré — et cela dans les **deux** façons de la
 **          fournir, liée par un chemin et intégrée en base64. Le second point est le plus fragile : les deux
 **          branches d'`IsValid()` sont séparées, en oublier une laisse le crash passer par l'autre.
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

#ifndef TST_VBACKGROUNDPATTERNIMAGE_H
#define TST_VBACKGROUNDPATTERNIMAGE_H

#include <QObject>
#include <QTemporaryDir>

class TST_BackgroundPatternImage : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit TST_BackgroundPatternImage(QObject *parent = nullptr);

private slots:
    void initTestCase();

    void SvgOnANormalCanvasIsAccepted();
    void OversizedSvgIsRejectedWhenLinked();
    void OversizedSvgIsRejectedWhenEmbedded();
    void MalformedEmbeddedSvgIsRejected();
    void ValidityCacheIsInvalidated();

private:
    auto WriteFile(const QString &name, const QByteArray &content) -> QString;

    QTemporaryDir m_dir;
};

#endif // TST_VBACKGROUNDPATTERNIMAGE_H
