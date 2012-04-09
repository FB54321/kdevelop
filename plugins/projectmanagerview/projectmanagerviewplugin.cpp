/* This file is part of KDevelop
   Copyright 2004 Roberto Raggi <roberto@kdevelop.org>
   Copyright 2007 Andreas Pakulat <apaku@gmx.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include "projectmanagerviewplugin.h"

#include <QtCore/QList>
#include <QtGui/QInputDialog>
#include <QtGui/QApplication>

#include <kaction.h>
#include <kactioncollection.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kparts/mainwindow.h>
#include <kparts/componentfactory.h>

#include <project/projectmodel.h>
#include <project/projectbuildsetmodel.h>
#include <interfaces/icore.h>
#include <interfaces/iproject.h>
#include <project/interfaces/iprojectfilemanager.h>
#include <project/interfaces/ibuildsystemmanager.h>
#include <interfaces/iuicontroller.h>
#include <interfaces/iruncontroller.h>
#include <interfaces/idocumentcontroller.h>
#include <project/interfaces/iprojectbuilder.h>
#include <interfaces/iprojectcontroller.h>
#include <interfaces/context.h>
#include <interfaces/contextmenuextension.h>
#include <interfaces/iselectioncontroller.h>

#include "projectmanagerview.h"
#include "builditembuilderjob.h"

using namespace KDevelop;

K_PLUGIN_FACTORY(ProjectManagerFactory, registerPlugin<ProjectManagerViewPlugin>(); )
K_EXPORT_PLUGIN(ProjectManagerFactory(KAboutData("kdevprojectmanagerview","kdevprojectmanagerview", ki18n("Project Management View"), "0.1", ki18n("Toolview to do all the project management stuff"), KAboutData::License_GPL)))

class KDevProjectManagerViewFactory: public KDevelop::IToolViewFactory
{
    public:
        KDevProjectManagerViewFactory( ProjectManagerViewPlugin *plugin ): mplugin( plugin )
        {}
        virtual QWidget* create( QWidget *parent = 0 )
        {
            return new ProjectManagerView( mplugin, parent );
        }
        virtual Qt::DockWidgetArea defaultPosition()
        {
            return Qt::LeftDockWidgetArea;
        }
        virtual QString id() const
        {
            return "org.kdevelop.ProjectsView";
        }
        virtual QList< QAction* > contextMenuActions(QWidget* viewWidget) const
        {
            QList<QAction*> actions;
            foreach(QAction* action, viewWidget->actions()) {
                if (!qobject_cast<ProjectManagerFilterAction*>(action)) {
                    actions << action;
                }
            }
            return actions;
        }
    private:
        ProjectManagerViewPlugin *mplugin;
};

class ProjectManagerViewPluginPrivate
{
public:
    ProjectManagerViewPluginPrivate()
    {}
    KDevProjectManagerViewFactory *factory;
    QList<KDevelop::ProjectBaseItem*> ctxProjectItemList;
    KAction* m_buildAll;
    KAction* m_build;
    KAction* m_install;
    KAction* m_clean;
    KAction* m_configure;
    KAction* m_prune;
};

ProjectManagerViewPlugin::ProjectManagerViewPlugin( QObject *parent, const QVariantList& )
        : IPlugin( ProjectManagerFactory::componentData(), parent ), d(new ProjectManagerViewPluginPrivate)
{
    d->m_buildAll = new KAction( i18n("Build all Projects"), this );
    d->m_buildAll->setIcon(KIcon("run-build"));
    connect( d->m_buildAll, SIGNAL(triggered()), this, SLOT(buildAllProjects()) );
    actionCollection()->addAction( "project_buildall", d->m_buildAll );
    d->m_build = new KAction( i18n("Build Selection"), this );
    d->m_build->setShortcut( Qt::Key_F8 );
    d->m_build->setIcon(KIcon("run-build"));
    d->m_build->setEnabled( false );
    connect( d->m_build, SIGNAL(triggered()), this, SLOT(buildProjectItems()) );
    actionCollection()->addAction( "project_build", d->m_build );
    d->m_install = new KAction( i18n("Install Selection"), this );
    d->m_install->setIcon(KIcon("run-build-install"));
    d->m_install->setEnabled( false );
    connect( d->m_install, SIGNAL(triggered()), this, SLOT(installProjectItems()) );
    actionCollection()->addAction( "project_install", d->m_install );
    d->m_clean = new KAction( i18n("Clean Selection"), this );
    d->m_clean->setIcon(KIcon("run-build-clean"));
    d->m_clean->setEnabled( false );
    connect( d->m_clean, SIGNAL(triggered()), this, SLOT(cleanProjectItems()) );
    actionCollection()->addAction( "project_clean", d->m_clean );
    d->m_configure = new KAction( i18n("Configure Selection"), this );
    d->m_configure->setIcon(KIcon("run-build-configure"));
    d->m_configure->setEnabled( false );
    connect( d->m_configure, SIGNAL(triggered()), this, SLOT(configureProjectItems()) );
    actionCollection()->addAction( "project_configure", d->m_configure );
    d->m_prune = new KAction( i18n("Prune Selection"), this );
    d->m_prune->setIcon(KIcon("run-build-prune"));
    d->m_prune->setEnabled( false );
    connect( d->m_prune, SIGNAL(triggered()), this, SLOT(pruneProjectItems()) );
    actionCollection()->addAction( "project_prune", d->m_prune );
    // only add the action so that its known in the actionCollection
    // and so that it's shortcut etc. pp. is restored
    // apparently that is not possible to be done in the view itself *sigh*
    actionCollection()->addAction( "locate_document" );
    setXMLFile( "kdevprojectmanagerview.rc" );
    d->factory = new KDevProjectManagerViewFactory( this );
    core()->uiController()->addToolView( i18n("Projects"), d->factory );
    connect( core()->selectionController(), SIGNAL(selectionChanged(KDevelop::Context*)),
             SLOT(updateActionState(KDevelop::Context*)));
    connect( ICore::self()->projectController()->buildSetModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
             SLOT(updateFromBuildSetChange()));
    connect( ICore::self()->projectController()->buildSetModel(), SIGNAL(rowsRemoved(QModelIndex,int,int)),
             SLOT(updateFromBuildSetChange()));
    connect( ICore::self()->projectController()->buildSetModel(), SIGNAL(modelReset()),
             SLOT(updateFromBuildSetChange()));
}

void ProjectManagerViewPlugin::updateFromBuildSetChange()
{
    updateActionState( core()->selectionController()->currentSelection() );
}

void ProjectManagerViewPlugin::updateActionState( KDevelop::Context* ctx )
{
    bool isEmpty = ICore::self()->projectController()->buildSetModel()->items().isEmpty();
    if( isEmpty )
    {
        isEmpty = !ctx || ctx->type() != Context::ProjectItemContext || dynamic_cast<ProjectItemContext*>( ctx )->items().isEmpty();
    }
    d->m_build->setEnabled( !isEmpty );
    d->m_install->setEnabled( !isEmpty );
    d->m_clean->setEnabled( !isEmpty );
    d->m_configure->setEnabled( !isEmpty );
    d->m_prune->setEnabled( !isEmpty );
}

ProjectManagerViewPlugin::~ProjectManagerViewPlugin()
{
    delete d;
}

void ProjectManagerViewPlugin::unload()
{
    kDebug() << "unloading manager view";
    core()->uiController()->removeToolView(d->factory);
}

ContextMenuExtension ProjectManagerViewPlugin::contextMenuExtension( KDevelop::Context* context )
{
    d->ctxProjectItemList.clear();
    if( context->type() != KDevelop::Context::ProjectItemContext )
        return IPlugin::contextMenuExtension( context );

    KDevelop::ProjectItemContext* ctx = dynamic_cast<KDevelop::ProjectItemContext*>( context );
    QList<KDevelop::ProjectBaseItem*> items = ctx->items();

    if( items.isEmpty() )
        return IPlugin::contextMenuExtension( context );

    //TODO: also needs: removeTarget, removeFileFromTarget, runTargetsFromContextMenu
    ContextMenuExtension menuExt;
    bool needsCreateFile = true;
    bool needsCreateFolder = true;
    bool needsCloseProjects = true;
    bool needsBuildItems = true;
    bool needsFolderItems = true;
    bool needsRemoveAndRename = true;
    bool needsRemoveTargetFiles = true;

    //needsCreateFile if there is one item and it's a folder or target
    needsCreateFile &= (items.count() == 1) && (items.first()->folder() || items.first()->target());
    //needsCreateFolder if there is one item and it's a folder
    needsCreateFolder &= (items.count() == 1) && (items.first()->folder());
    
    foreach( ProjectBaseItem* item, items ) {
        d->ctxProjectItemList << item;
        //needsBuildItems if items are limited to targets and buildfolders
        needsBuildItems &= item->target() || item->type() == ProjectBaseItem::BuildFolder;

        //needsCloseProjects if items are limited to top level folders (Project Folders)
        needsCloseProjects &= item->folder() && !item->folder()->parent();

        //needsFolderItems if items are limited to folders
        needsFolderItems &= (bool)item->folder();

        //needsRemove if items are limited to non-top-level folders or files that don't belong to targets
        needsRemoveAndRename &= (item->folder() && item->parent()) || (item->file() && !item->parent()->target());

        //needsRemoveTargets if items are limited to file items with target parents
        needsRemoveTargetFiles &= (item->file() && item->parent()->target());
    }

    if ( needsCreateFile ) {
        KAction* action = new KAction( i18n( "Create File" ), this );
        action->setIcon(KIcon("document-new"));
        connect( action, SIGNAL(triggered()), this, SLOT(createFileFromContextMenu()) );
        menuExt.addAction( ContextMenuExtension::FileGroup, action );
    }
    if ( needsCreateFolder ) {
        KAction* action = new KAction( i18n( "Create Folder" ), this );
        action->setIcon(KIcon("folder-new"));
        connect( action, SIGNAL(triggered()), this, SLOT(createFolderFromContextMenu()) );
        menuExt.addAction( ContextMenuExtension::FileGroup, action );
    }
    if ( needsBuildItems ) {
        KAction* action = new KAction( i18nc( "@action", "Build" ), this );
        action->setIcon(KIcon("run-build"));
        connect( action, SIGNAL(triggered()), this, SLOT(buildItemsFromContextMenu()) );
        menuExt.addAction( ContextMenuExtension::BuildGroup, action );
        action = new KAction( i18nc( "@action", "Install" ), this );
        action->setIcon(KIcon("run-install"));
        connect( action, SIGNAL(triggered()), this, SLOT(installItemsFromContextMenu()) );
        menuExt.addAction( ContextMenuExtension::BuildGroup, action );
        action = new KAction( i18nc( "@action", "Clean" ), this );
        action->setIcon(KIcon("run-clean"));
        connect( action, SIGNAL(triggered()), this, SLOT(cleanItemsFromContextMenu()) );
        menuExt.addAction( ContextMenuExtension::BuildGroup, action );
        action = new KAction( i18n( "Add to Buildset" ), this );
        connect( action, SIGNAL(triggered()), this, SLOT(addItemsFromContextMenuToBuildset()) );
        menuExt.addAction( ContextMenuExtension::BuildGroup, action );
    }
    if ( needsCloseProjects ) {
        KAction* close = new KAction( i18np( "Close Project", "Close Projects", items.count() ), this );
        close->setIcon(KIcon("project-development-close"));
        connect( close, SIGNAL(triggered()), this, SLOT(closeProjects()) );
        menuExt.addAction( ContextMenuExtension::ProjectGroup, close );
    }
    if ( needsFolderItems ) {
        KAction* action = new KAction( i18n( "Reload" ), this );
        action->setIcon(KIcon("view-refresh"));
        connect( action, SIGNAL(triggered()), this, SLOT(reloadFromContextMenu()) );
        menuExt.addAction( ContextMenuExtension::FileGroup, action );
    }
    if ( needsRemoveAndRename ) {
        KAction* remove = new KAction( i18n( "Remove" ), this );
        remove->setIcon(KIcon("user-trash"));
        connect( remove, SIGNAL(triggered()), this, SLOT(removeFromContextMenu()) );
        menuExt.addAction( ContextMenuExtension::FileGroup, remove );
        KAction* rename = new KAction( i18n( "Rename" ), this );
        rename->setIcon(KIcon("edit-rename"));
        connect( rename, SIGNAL(triggered()), this, SLOT(renameItemFromContextMenu()) );
        menuExt.addAction( ContextMenuExtension::FileGroup, rename );
    }
    if ( needsRemoveTargetFiles ) {
        KAction* remove = new KAction( i18n( "Remove From Target" ), this );
        remove->setIcon(KIcon("user-trash"));
        connect( remove, SIGNAL(triggered()), this, SLOT(removeTargetFilesFromContextMenu()) );
        menuExt.addAction( ContextMenuExtension::FileGroup, remove );
    }
    return menuExt;
}


KDevelop::IProjectBuilder* ProjectManagerViewPlugin::getProjectBuilder( KDevelop::ProjectBaseItem* item )
{
    if( !item )
        return 0;
    IProject* project = item->project();
    if (!project)
        return 0;

    ProjectFolderItem* prjitem = project->projectItem();
    IPlugin* fmgr = project->managerPlugin();
    IBuildSystemManager* mgr = fmgr->extension<IBuildSystemManager>();
    if( mgr )
    {
        return mgr->builder();
    }
    return 0;
}

void ProjectManagerViewPlugin::closeProjects()
{
    QList<KDevelop::IProject*> projectsToClose;
    foreach( KDevelop::ProjectBaseItem* item, d->ctxProjectItemList )
    {
        if( !projectsToClose.contains( item->project() ) )
        {
            projectsToClose << item->project();
        }
    }
    d->ctxProjectItemList.clear();
    foreach( KDevelop::IProject* proj, projectsToClose )
    {
        core()->projectController()->closeProject( proj );
    }
}


void ProjectManagerViewPlugin::installItemsFromContextMenu()
{
     ICore::self()->runController()->registerJob( new BuildItemBuilderJob( KDevelop::BuilderJob::Install, d->ctxProjectItemList ) );
    d->ctxProjectItemList.clear();
}

void ProjectManagerViewPlugin::cleanItemsFromContextMenu()
{
     ICore::self()->runController()->registerJob( new BuildItemBuilderJob( KDevelop::BuilderJob::Clean, d->ctxProjectItemList ) );
    d->ctxProjectItemList.clear();
}

void ProjectManagerViewPlugin::buildItemsFromContextMenu()
{
     ICore::self()->runController()->registerJob( new BuildItemBuilderJob( KDevelop::BuilderJob::Build, d->ctxProjectItemList ) );
    d->ctxProjectItemList.clear();
}

void ProjectManagerViewPlugin::buildAllProjects()
{
    QList<KDevelop::ProjectBaseItem*> items;
    foreach( KDevelop::IProject* project, core()->projectController()->projects() )
    {
        items << project->projectItem();
    }
    ICore::self()->runController()->registerJob( new BuildItemBuilderJob( KDevelop::BuilderJob::Build, items ) );
}

void ProjectManagerViewPlugin::runBuilderJob( KDevelop::BuilderJob::BuildType t )
{
    QList<ProjectBaseItem*> items;
    if( !ICore::self()->projectController()->buildSetModel()->items().isEmpty() )
    {
        ICore::self()->runController()->registerJob( new BuildItemBuilderJob( t, ICore::self()->projectController()->buildSetModel()->items() ) );
    } else
    {
        KDevelop::ProjectItemContext* ctx = dynamic_cast<KDevelop::ProjectItemContext*>(ICore::self()->selectionController()->currentSelection());
        ICore::self()->runController()->registerJob( new BuildItemBuilderJob( t, ctx->items() ) );
    }
}

void ProjectManagerViewPlugin::installProjectItems()
{
    runBuilderJob( KDevelop::BuilderJob::Install );
}

void ProjectManagerViewPlugin::pruneProjectItems()
{
    runBuilderJob( KDevelop::BuilderJob::Prune );
}

void ProjectManagerViewPlugin::configureProjectItems()
{
    runBuilderJob( KDevelop::BuilderJob::Configure );
}

void ProjectManagerViewPlugin::cleanProjectItems()
{
    runBuilderJob( KDevelop::BuilderJob::Clean );
}

void ProjectManagerViewPlugin::buildProjectItems()
{
    runBuilderJob( KDevelop::BuilderJob::Build );
}

void ProjectManagerViewPlugin::addItemsFromContextMenuToBuildset( )
{
    foreach( KDevelop::ProjectBaseItem* item, d->ctxProjectItemList )
    {
        ICore::self()->projectController()->buildSetModel()->addProjectItem( item );
    }
}

void ProjectManagerViewPlugin::runTargetsFromContextMenu( )
{
    foreach( KDevelop::ProjectBaseItem* item, d->ctxProjectItemList )
    {
        KDevelop::ProjectExecutableTargetItem* t=item->executable();
        if(t)
        {
            kDebug() << "Running target: " << t->text() << t->builtUrl();
        }
    }
}

void ProjectManagerViewPlugin::projectConfiguration( )
{
    if( !d->ctxProjectItemList.isEmpty() )
    {
        core()->projectController()->configureProject( d->ctxProjectItemList.at( 0 )->project() );
    }
}

void ProjectManagerViewPlugin::reloadFromContextMenu( )
{
    QList< KDevelop::ProjectFolderItem* > folders;
    foreach( KDevelop::ProjectBaseItem* item, d->ctxProjectItemList )
    {
        if ( item->folder() ) {
            // since reloading should be recursive, only pass the upper-most items
            bool found = false;
            foreach ( KDevelop::ProjectFolderItem* existing, folders ) {
                if ( existing->url().isParentOf(item->folder()->url()) ) {
                    // simply skip this child
                    found = true;
                    break;
                } else if ( item->folder()->url().isParentOf(existing->url()) ) {
                    // remove the child in the list and add the current item instead
                    folders.removeOne(existing);
                    // continue since there could be more than one existing child
                }
            }
            if ( !found ) {
                folders << item->folder();
            }
        }
    }
    foreach( KDevelop::ProjectFolderItem* folder, folders ) {
        folder->project()->projectFileManager()->reload(folder);
    }
}

void ProjectManagerViewPlugin::createFolderFromContextMenu( )
{
    foreach( KDevelop::ProjectBaseItem* item, d->ctxProjectItemList )
    {
        if ( item->folder() ) {
            QWidget* window(ICore::self()->uiController()->activeMainWindow()->window());
            QString name = QInputDialog::getText ( window,
                                i18n ( "Create Folder in %1", item->folder()->url().prettyUrl() ), i18n ( "Folder Name" ) );
            if (!name.isEmpty()) {
                KUrl url = item->folder()->url();
                url.addPath( name );
                item->project()->projectFileManager()->addFolder( url, item->folder() );
            }
        }
    }
}

void ProjectManagerViewPlugin::removeFromContextMenu()
{
    removeItems(d->ctxProjectItemList);
}

void ProjectManagerViewPlugin::removeItems(const QList< ProjectBaseItem* > items)
{
    if (items.isEmpty()) {
        return;
    }

    //copy the list of selected items and sort it to guarantee parents will come before children
    QMap< IProjectFileManager*, QList<KDevelop::ProjectBaseItem*> > filteredItems;
    filteredItems[0] = items;
    qSort(filteredItems[0].begin(), filteredItems[0].end(), ProjectBaseItem::urlLessThan);

    KUrl lastFolder;

    QStringList itemPaths;
    foreach( KDevelop::ProjectBaseItem* item, filteredItems[0] )
    {
        Q_ASSERT(item->folder() || item->file());
        Q_ASSERT(!item->file() || !item->file()->parent()->target());

        if (item->folder() || item->file())
        {
            //make sure no children of folders that will be deleted are listed
            if (lastFolder.isParentOf(item->url())) {
                continue;
            } else if (item->folder()) {
                lastFolder = item->url();
            }

            filteredItems[item->project()->projectFileManager()] << item;
            itemPaths << item->url().path();
        }
    }

    if (KMessageBox::warningYesNoList(
            QApplication::activeWindow(),
            i18np("Do you really want to delete this item?",
                  "Do you really want to delete these %1 items?",
                  itemPaths.size()),
            itemPaths, i18n("Delete Files"),
            KStandardGuiItem::del(), KStandardGuiItem::cancel()
        ) == KMessageBox::No) {
        return;
    }

    //Go though projectmanagers, have them remove the files and folders that they own
    QMap< IProjectFileManager*, QList<KDevelop::ProjectBaseItem*> >::iterator it;
    for (it = filteredItems.begin(); it != filteredItems.end(); ++it)
    {
        if (it.key())
            it.key()->removeFilesAndFolders(it.value());
    }
}

void ProjectManagerViewPlugin::removeTargetFilesFromContextMenu()
{
    QList<ProjectBaseItem*> items = d->ctxProjectItemList;
    QMap< IBuildSystemManager*, QList<KDevelop::ProjectFileItem*> > itemsByBuildSystem;
    foreach(ProjectBaseItem *item, items)
        itemsByBuildSystem[item->project()->buildSystemManager()].append(item->file());

    QMap< IBuildSystemManager*, QList<KDevelop::ProjectFileItem*> >::iterator it;
    for (it = itemsByBuildSystem.begin(); it != itemsByBuildSystem.end(); ++it)
        it.key()->removeFilesFromTargets(it.value());
}

void ProjectManagerViewPlugin::renameItemFromContextMenu()
{
    renameItems(d->ctxProjectItemList);
}

void ProjectManagerViewPlugin::renameItems(const QList< ProjectBaseItem* > items)
{
    if (items.isEmpty()) {
        return;
    }

    QWidget* window = ICore::self()->uiController()->activeMainWindow()->window();

    foreach( KDevelop::ProjectBaseItem* item, items )
    {
        if ((item->type()!=ProjectBaseItem::BuildFolder
                && item->type()!=ProjectBaseItem::Folder
                && item->type()!=ProjectBaseItem::File) || !item->parent())
        {
            continue;
        }

        const QString src = item->text();

        //Change QInputDialog->KFileSaveDialog?
        QString name = QInputDialog::getText(
            window, i18n("Rename..."),
            i18n("New name for '%1'", item->text()),
            QLineEdit::Normal, item->text()
        );

        if (!name.isEmpty() && name != src) {
            ProjectBaseItem::RenameStatus status = item->rename( name );

            switch(status) {
                case ProjectBaseItem::RenameOk:
                    break;
                case ProjectBaseItem::ExistingItemSameName:
                    KMessageBox::error(window, i18n("There is already a file named '%1'", name));
                    break;
                case ProjectBaseItem::ProjectManagerRenameFailed:
                    KMessageBox::error(window, i18n("Could not rename '%1'", name));
                    break;
                case ProjectBaseItem::InvalidNewName:
                    KMessageBox::error(window, i18n("'%1' is not a valid file name", name));
                    break;
            }
        }
    }
}

ProjectFileItem* createFile(const ProjectFolderItem* item)
{
    QWidget* window = ICore::self()->uiController()->activeMainWindow()->window();
    QString name = QInputDialog::getText(window, i18n("Create File in %1", item->url().prettyUrl()), i18n("File Name"));

    if(name.isEmpty())
        return 0;

    KUrl url=item->url();
    url.addPath( name );

    ProjectFileItem* ret=item->project()->projectFileManager()->addFile( url, item->folder() );
    ICore::self()->documentController()->openDocument( url );
    return ret;
}

void ProjectManagerViewPlugin::createFileFromContextMenu( )
{
    foreach( KDevelop::ProjectBaseItem* item, d->ctxProjectItemList )
    {
        if ( item->folder() ) {
            createFile(item->folder());
        } else if ( item->target() ) {
            ProjectFolderItem* folder=dynamic_cast<ProjectFolderItem*>(item->parent());
            if(folder)
            {
                ProjectFileItem* f=createFile(folder);
                if(f)
                    item->project()->buildSystemManager()->addFilesToTarget(QList<ProjectFileItem*>() << f, item->target());
            }
        }
    }
}

#include "projectmanagerviewplugin.moc"

