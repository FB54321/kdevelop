/* This file is part of KDevelop

   Copyright 2018 Anton Anikin <anton@anikin.xyz>
   Copyright 2020 Friedrich W. H. Kossebau <kossebau@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "plugin.h"

// plugin
#include "checksdb.h"
#include "globalsettings.h"
#include "analyzer.h"
#include "debug.h"
#include "config/globalconfigpage.h"
#include "config/projectconfigpage.h"
// KDevPlatform
#include <interfaces/contextmenuextension.h>
#include <project/projectconfigpage.h>
// KF
#include <KPluginFactory>
// Qt
#include <QApplication>


K_PLUGIN_FACTORY_WITH_JSON(ClazyFactory, "kdevclazy.json",
                           registerPlugin<Clazy::Plugin>();)

namespace Clazy
{

Plugin::Plugin(QObject* parent, const QVariantList&)
    : IPlugin(QStringLiteral("kdevclazy"), parent)
    , m_db(nullptr)
{
    setXMLFile(QStringLiteral("kdevclazy.rc"));

    // create after ui.rc file is set with action ids
    m_analyzer = new Analyzer(this, this);
}

Plugin::~Plugin() = default;

void Plugin::unload()
{
    delete m_analyzer;
    m_analyzer = nullptr;
}

KDevelop::ContextMenuExtension Plugin::contextMenuExtension(KDevelop::Context* context, QWidget* parent)
{
    KDevelop::ContextMenuExtension extension = KDevelop::IPlugin::contextMenuExtension(context, parent);

    m_analyzer->fillContextMenuExtension(extension, context, parent);

    return extension;
}

KDevelop::ConfigPage* Plugin::perProjectConfigPage(int number, const KDevelop::ProjectConfigOptions& options, QWidget* parent)
{
    if (m_db.isNull()) {
        reloadDB();
    }

    return number ? nullptr : new ProjectConfigPage(this, options.project, parent);
}

KDevelop::ConfigPage* Plugin::configPage(int number, QWidget* parent)
{
    return number ? nullptr : new GlobalConfigPage(this, parent);
}

QSharedPointer<const ChecksDB> Plugin::checksDB() const
{
    return m_db;
}

QSharedPointer<const ChecksDB> Plugin::loadedChecksDB()
{
    if (m_db.isNull()) {
        reloadDB();
    }

    return m_db;
}

void Plugin::reloadDB()
{
    m_db.reset(new ChecksDB(GlobalSettings::docsPath()));
    connect(GlobalSettings::self(), &GlobalSettings::docsPathChanged, this, &Plugin::reloadDB);
}

}

#include "plugin.moc"
