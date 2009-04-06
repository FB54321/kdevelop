/***************************************************************************
 *   Copyright 2008 Evgeniy Ivanov <powerfox@kde.ru>                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef DVCS_PLUGIN_H
#define DVCS_PLUGIN_H

#include <KDE/KUrl>
#include <KDE/KJob>
#include <KDE/KComponentData>

#include <QtCore/QObject>

#include <vcs/interfaces/idistributedversioncontrol.h>
#include <interfaces/iuicontroller.h>
#include <interfaces/iplugin.h>

#include "dvcsevent.h"
#include <vcs/vcsexport.h>
#include <vcs/vcsstatusinfo.h>

class QString;
class KDevDVCSViewFactory;
class DVcsJob;

namespace KDevelop
{
class VcsJob;
class ContextMenuExtension;
struct DistributedVersionControlPluginPrivate;

/**
 * DistributedVersionControlPlugin is a base class for git/hg/bzr plugins. This class implements
 * KDevelop::IBasicVersionControl, KDevelop::IDistributedVersionControl and KDevelop::IPlugin (contextMenuExtension).
 * DistributedVersionControlPlugin class uses IDVCSexecutor to get all jobs
 * from real DVCS plugins like Git. It is based on KDevelop's CVS plugin (also looks like svn plugin is it's relative too).
 * @note Create only special items in contextMenuExtension, all standart menu items are created in vcscommon plugin!
 */
class KDEVPLATFORMVCS_EXPORT DistributedVersionControlPlugin : public IPlugin, public IDistributedVersionControl
{
    Q_OBJECT
    Q_INTERFACES(KDevelop::IBasicVersionControl KDevelop::IDistributedVersionControl)
public:

    DistributedVersionControlPlugin(QObject *parent, KComponentData compData);
    virtual ~DistributedVersionControlPlugin();

    // Begin: KDevelop::IBasicVersionControl
    virtual QString name() const = 0;
    virtual bool isVersionControlled(const KUrl& localLocation) = 0;
    virtual VcsJob* repositoryLocation(const KUrl& localLocation);
    virtual VcsJob* add(const KUrl::List& localLocations,
                        IBasicVersionControl::RecursionMode recursion = IBasicVersionControl::Recursive) = 0;
    virtual VcsJob* remove(const KUrl::List& localLocations) = 0;
    virtual VcsJob* status(const KUrl::List& localLocations,
                           IBasicVersionControl::RecursionMode recursion = IBasicVersionControl::Recursive) = 0;
    virtual VcsJob* copy(const KUrl& localLocationSrc,
                         const KUrl& localLocationDst);
    virtual VcsJob* move(const KUrl& localLocationSrc,
                         const KUrl& localLocationDst);
    virtual VcsJob* revert(const KUrl::List& localLocations,
                           IBasicVersionControl::RecursionMode recursion = IBasicVersionControl::Recursive);
    virtual VcsJob* update(const KUrl::List& localLocations,
                           const VcsRevision& rev,
                           IBasicVersionControl::RecursionMode recursion = IBasicVersionControl::Recursive);
    virtual VcsJob* commit(const QString& message,
                           const KUrl::List& localLocations,
                           IBasicVersionControl::RecursionMode recursion  = IBasicVersionControl::Recursive) = 0;
    virtual VcsJob* diff(const KUrl& fileOrDirectory,
                         const VcsRevision& srcRevision,
                         const VcsRevision& dstRevision,
                         VcsDiff::Type = VcsDiff::DiffUnified,
                         IBasicVersionControl::RecursionMode recursion
                         = IBasicVersionControl::Recursive);
    virtual VcsJob* log(const KUrl& localLocation,
                        const VcsRevision& rev,
                        unsigned long limit) = 0;
    virtual VcsJob* log(const KUrl& localLocation,
                        const VcsRevision& rev,
                        const VcsRevision& limit);
    virtual VcsJob* annotate(const KUrl& localLocation,
                             const VcsRevision& rev);
    virtual VcsJob* resolve(const KUrl::List& localLocations,
                            IBasicVersionControl::RecursionMode recursion);
    virtual VcsJob* createWorkingCopy(const VcsLocation & sourceRepository, const KUrl & destinationDirectory, RecursionMode recursion = IBasicVersionControl::Recursive);
    // End:  KDevelop::IBasicVersionControl

    // Begin:  KDevelop::IDistributedVersionControl
    virtual VcsJob* init(const KUrl& localRepositoryRoot) = 0;
    virtual VcsJob* push(const KUrl& localRepositoryLocation,
                         const VcsLocation& localOrRepoLocationDst);
    virtual VcsJob* pull(const VcsLocation& localOrRepoLocationSrc,
                         const KUrl& localRepositoryLocation);
    virtual VcsJob* reset(const KUrl& repository,
                          const QStringList &args, const KUrl::List& files) = 0;
    // End:  KDevelop::IDistributedVersionControl

    /** Used in KDevelop's appwizardplugin (creates import widget) */
    virtual VcsImportMetadataWidget* createImportMetadataWidget(QWidget* parent);

    // From KDevelop::IPlugin
    /** Creates context menu
     * @note Create only special items here (like checkout), all standart menu items are created in vcscommon plugin!
     */
    virtual ContextMenuExtension contextMenuExtension(Context*);

    /**
      * Parses the output generated by a @code dvcs log @endcode command and
      * fills the given QList with all commits infos found in the given output.
      * @param job The finished job of a @code dvcs log @endcode call
      * @param revisions Will be filled with all revision infos found in @p jobOutput
      * TODO: Change it to pass the results in @code job->getResults() @endcode
      */
    virtual void parseLogOutput(const DVcsJob * job,
                                QList<DVcsEvent>& revisions) const = 0;



    // In tree branch-management
    virtual DVcsJob* switchBranch(const QString &repository, const QString &branch) = 0;

    /** Branch. */
    virtual DVcsJob* branch(const QString &repository, const QString &basebranch = QString(),
                            const QString &branch = QString(), const QStringList &args = QStringList()) = 0;
    //parsers for branch:
    /** Returns current branch. */
    virtual QString curBranch(const QString &repository) = 0;

    /** Returns the list of branches. */
    virtual QStringList branches(const QString &repository) = 0;
    // End: In tree branch-management

public Q_SLOTS:
    //slots for context menu
    void ctxPush();
    void ctxPull();
//     void ctxLog();
    void ctxRevHistory();

    // slots for menu
    void slotInit();

    /**
     * Updates project state after checkout (simply reloads it now)
     */
    void checkoutFinished(KJob*);

Q_SIGNALS:
    /**
     * Some actions like commit, add, remove... will connect the job's
     * result() signal to this signal. Anybody, like for instance the
     * DVCSMainView class, that is interested in getting notified about
     * jobs that finished can connect to this signal.
     * @see class GitMainView
     */
    void jobFinished(KJob* job);

    /**
     * Gets emmited when a job like log, editors... was created.
     * GitPlugin will connect the newly created view to the result() signal
     * of a job. So the new view will show the output of that job as
     * soon as it has finished.
     */
    void addNewTabToMainView(QWidget* tab, QString label);

protected:
    ///////////////////
    /** Checks if dirPath is located in DVCS repository */
    virtual bool isValidDirectory(const KUrl &dirPath) = 0;

    // Additional interface to be implemented by derived plugins
    //commit dialog helpers:
    /** Returns the list of modified files (diff between workdir and index). */
    virtual QList<QVariant> getModifiedFiles(const QString &directory);
    /** Returns the list of already cached files (diff between index and HEAD). */
    virtual QList<QVariant> getCachedFiles(const QString &directory);
    /** Files are not in the repo, but in the repository location. */
    virtual QList<QVariant> getOtherFiles(const QString &directory);

    /** empty_cmd is used when something is not implemented, but has to return any job */
    virtual DVcsJob* empty_cmd();

    /** Returs the list of all commits (in all branches).
     * @see CommitView and CommitViewDelegate to see how this list is used.
     */
    virtual QList<DVcsEvent> getAllCommits(const QString &repo) = 0;

    KDevDVCSViewFactory * dvcsViewFactory() const;

    /** RequestedOperation is used to know if we should check the repo with isValidDirectory
     * or we want to create new repo (init/clone).
     */
    enum RequestedOperation {
        NormalOperation, /**< add/commit/etc, check if we are in the repo */
        Init             /**< we need init/clone, so don't call isValidDirectory, we're not in the repo, but yet ;) */
    };

    /** This method checks RequestedOperation, clears the job and sets working directory.
     * Returns false only if op == NormalOperation and we are not in the repository.
     * @param job the DVCSjob to be prepared
     * @param repository working directiry
     * @param op shows if the method should run isValidDirectory
     */
    virtual bool prepareJob(DVcsJob* job, const QString& repository,
                            enum RequestedOperation op = NormalOperation);
    /** Add files as args to the job. It changes absolute pathes to relatives */
    static bool addFileList(DVcsJob* job, const KUrl::List& urls);

    /** Always returns directory path.
     * @param path a path of a file or a directory.
     * @return if path argument if file then returns parent directory, otherwice path arg is returned.
     * @todo it will be nice to change prepareJob() so it can change its repository argument.
     */
    static QString stripPathToDir(const QString &path);

private:
    DistributedVersionControlPluginPrivate * const d;
};

}

class KDevDVCSViewFactory: public KDevelop::IToolViewFactory
{
public:
    KDevDVCSViewFactory(KDevelop::DistributedVersionControlPlugin *plugin): m_plugin(plugin) {}
    virtual QWidget* create(QWidget *parent = 0);
    virtual Qt::DockWidgetArea defaultPosition();
    virtual QString id() const;
private:
    KDevelop::DistributedVersionControlPlugin *m_plugin;
};

#endif
