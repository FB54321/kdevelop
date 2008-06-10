/* This file is part of KDevelop
    Copyright 2008 David Nolden <david.nolden.kdevelop@art-master.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef USES_H
#define USES_H

#include <QObject>
#include "language/languageexport.h"
#include "language/duchain/declarationid.h"

/**
 * Global mapping of Declaration-Ids to top-contexts, protected through DUChainLock.
 * 
 * To retrieve the actual uses, query the duchain for the files.
 * */

namespace KDevelop {

  class Declaration;
  class DeclarationId;
  class Use;
  class TopDUContext;

  class KDEVPLATFORMLANGUAGE_EXPORT Uses {
    public:
    Uses();
    ~Uses();
    /**
     * Adds a top-context to the users-list of the given id
     * */
    void addUse(const DeclarationId& id, TopDUContext* topContext);
    /**
     * Removes the given top-context from the list of uses
     * */
    void removeUse(const DeclarationId& id, TopDUContext* topContext);

    ///Gets the users assigned to the declaration-id
    QList<TopDUContext*> uses(const DeclarationId& id) const;

    private:
      class UsesPrivate* d;
  };
}

#endif

