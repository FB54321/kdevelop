/* KDevelop CMake Support
 *
 * Copyright 2006 Matt Rogers <mattr@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "cmakemodelitems.h"
#include <QString>

#include <iproject.h>

CMakeFolderItem::CMakeFolderItem( KDevelop::IProject *project, const QString &name, QStandardItem* item )
    : KDevelop::ProjectItem( project, name, item )
{}

CMakeTargetItem::CMakeTargetItem( KDevelop::IProject *project, const QString& name, CMakeFolderItem* item)
    : KDevelop::ProjectTargetItem( project, name, item )
{}


CMakeTargetItem::~CMakeTargetItem()
{}

const KDevelop::DomUtil::PairList& CMakeTargetItem::defines() const
{
    return m_defines;
}

const QHash< QString, QString >& CMakeTargetItem::environment() const
{
    return m_environment;
}

KUrl::List CMakeFolderItem::includeList() const
{
    KUrl::List urls(m_includeList);
    
    CMakeFolderItem *folder = dynamic_cast<CMakeFolderItem*>(parent());
    while(folder)
    {
        urls += folder->includeList();
        folder = dynamic_cast<CMakeFolderItem*>(folder->parent());
    }
    return urls;
}

const KUrl::List & CMakeTargetItem::includeDirectories() const
{
    CMakeFolderItem *folder = dynamic_cast<CMakeFolderItem*>(parent());
    return folder->includeDirectories();
}

