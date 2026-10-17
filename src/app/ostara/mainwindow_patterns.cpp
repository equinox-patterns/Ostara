/************************************************************************
 **
 **  @file   mainwindow_patterns.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/10/2026
 **
 **  @brief  The library of patterns — the third one, beside the pieces and the measurement tables.
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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "core/opatternlibrary.h"
#include "dialogs/opatternlibrarydialog.h"

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ActionPatternLibrary()
{
    // The library is a window on the folders the patterns are sorted into. It reads what each pattern says about
    // itself — its name, its client, its collection, what was written about it — and manages the files: transfer,
    // export, delete. It **opens nothing**: a pattern being worked on is the editor's, not the library's, and the
    // window touches neither the document nor the settings.
    OPatternLibraryDialog dialog(OPatternLibrary::DefaultDirectory(), this);
    dialog.exec();
}
