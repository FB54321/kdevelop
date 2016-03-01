/*
 * This file is part of KDevelop
 *
 * Copyright 2015 Sergey Kalinichev <kalinichev.so.0@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef SIMPLEREFACTORING_H
#define SIMPLEREFACTORING_H

#include "clangprivateexport.h"

#include <language/codegen/basicrefactoring.h>

namespace KDevelop
{
class Context;
class ContextMenuExtension;
class Declaration;
}

class KDEVCLANGPRIVATE_EXPORT ClangRefactoring : public KDevelop::BasicRefactoring
{
    Q_OBJECT

public:
    explicit ClangRefactoring(QObject* parent = 0);

    void fillContextMenu(KDevelop::ContextMenuExtension& extension, KDevelop::Context* context) override;

    QString moveIntoSource(const KDevelop::IndexedDeclaration& iDecl);

    using KDevelop::BasicRefactoring::executeRenameAction;

public slots:
    void executeMoveIntoSourceAction();

private:
    bool validCandidateToMoveIntoSource(KDevelop::Declaration* decl);
};

#endif