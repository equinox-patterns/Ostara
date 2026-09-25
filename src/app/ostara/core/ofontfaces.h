/************************************************************************
 **
 **  @file   ofontfaces.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   25/09/2026
 **
 **  @brief  The faces a font family really has, and not a list of numbers
 **          wearing their names — what every control that chooses a weight
 **          must offer, so that the two cannot disagree.
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

#ifndef OFONTFACES_H
#define OFONTFACES_H

#include <QString>

class QComboBox;

/**
 * @brief Fill @p combo with the faces @p family really has, and select @p stored among them.
 *
 * **A list of numbers cannot name faces.** The nine numbered entries the weight controls used to
 * offer were CSS weights wearing Qt's names: « Light » (300) asked a family for a face it calls
 * « ExtraLight », « Black » (900) for one it calls « ExtraBold » — measured on the families
 * installed here (eighteen faces for Jost, eighteen for Inter, six for Helvetica). A weight is a
 * number; a face has a name, and only the family knows it.
 *
 * ⚠️ **The italic faces are not offered — decision of Sophie, 22-09-2026.** A template sets a FACE,
 * and the PDF redraws the words from their outlines: the re-emission reads the family, the size and
 * the weight, and never `font-style`. An italic chosen here would therefore be drawn UPRIGHT, in
 * silence — and a page is supposed to look the way the application shows it. The frontend's answer
 * is a veto rather than a rendering: the choice is not offered, so it cannot be made by mistake.
 * A family whose only faces are italic keeps them: a control offering nothing at all is worse than
 * one offering the wrong thing, and no font anybody sets a pattern in is like that.
 *
 * **@p stored may be a number.** A file written before the controls named faces holds a weight, and
 * it is resolved to the face it really draws — the same answer the renderer gets — so that a list
 * cannot show one face while the page is drawn in another. Nought is an unset weight, and an unset
 * weight is the family's Regular.
 *
 * The combo is left exactly as it was when @p family offers no face at all; a control with nothing
 * to choose must not lose what it had.
 *
 * @param combo  the weight control to fill; a null combo is ignored.
 * @param family the family whose faces are wanted, already resolved by the caller (an empty family
 *               is a family with no face to offer here, not a request for a default).
 * @param stored what the file asks for: a face name, or the number whose face it must show.
 */
void FillFontFaces(QComboBox *combo, const QString &family, const QString &stored);

/**
 * @brief The face a weight really draws in @p family.
 *
 * A weight is a number and a face has a name: 300 asks most families for a face the family calls
 * « ExtraLight ». This is the face the renderer would get, so a control that shows it and a page that
 * draws it cannot end up naming two different faces — which is exactly how the numbered lists used
 * to be wrong.
 *
 * @return the face name, or an empty string when @p family is empty or has no face for @p weight.
 */
auto FontFaceFromWeight(const QString &family, int weight) -> QString;

#endif // OFONTFACES_H
