/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#ifndef PLUGINS_LACKMAN_CORE_H
#define PLUGINS_LACKMAN_CORE_H
#include <QObject>
#include <QModelIndex>
#include <interfaces/iinfo.h>
#include "repoinfo.h"

class QAbstractItemModel;
class QStandardItemModel;
class QDir;

namespace LeechCraft
{
namespace LackMan
{
	class RepoInfoFetcher;
	class ExternalResourceManager;
	class Storage;
	class PackagesModel;
	class PendingManager;
	class PackageProcessor;

	class Core : public QObject
	{
		Q_OBJECT

		ICoreProxy_ptr Proxy_;
		RepoInfoFetcher *RepoInfoFetcher_;
		ExternalResourceManager *ExternalResourceManager_;
		Storage *Storage_;
		PackagesModel *PackagesModel_;
		PendingManager *PendingManager_;
		PackageProcessor *PackageProcessor_;
		QStandardItemModel *ReposModel_;
		bool UpdatesEnabled_;

		enum ReposColumns
		{
			RCURL
		};

		Core ();
	public:
		static Core& Instance ();
		void FinishInitialization ();
		void Release ();

		void SecondInit ();

		void SetProxy (ICoreProxy_ptr);
		ICoreProxy_ptr GetProxy () const;
		QAbstractItemModel* GetPluginsModel () const;
		PendingManager* GetPendingManager () const;
		ExternalResourceManager* GetExtResourceManager () const;
		Storage* GetStorage () const;
		QAbstractItemModel* GetRepositoryModel () const;

		DependencyList GetDependencies (int) const;
		QList<ListPackageInfo> GetDependencyFulfillers (const Dependency&) const;
		bool IsVersionOk (const QString& candidate, QString refVer) const;
		bool IsFulfilled (const Dependency&) const;
		QIcon GetIconForLPI (const ListPackageInfo&);
		QList<QUrl> GetPackageURLs (int) const;
		ListPackageInfo GetListPackageInfo (int);
		QDir GetPackageDir (int) const;

		void AddRepo (const QUrl&);
		void UpdateRepo (const QUrl&, const QStringList&);

		QString NormalizePackageName (const QString&) const;

		QStringList GetAllTags () const;
	private:
		InstalledDependencyInfoList GetSystemInstalledPackages () const;
		InstalledDependencyInfoList GetLackManInstalledPackages () const;
		InstalledDependencyInfoList GetAllInstalledPackages () const;
		void PopulatePluginsModel ();
		void HandleNewPackages (const PackageShortInfoList& shorts,
				int componentId, const QString& component, const QUrl& repoUrl);
		void PerformRemoval (int);
		void UpdateRowFor (int);
		bool RecordInstalled (int);
		bool RecordUninstalled (int);
		int GetPackageRow (int packageId) const;
		void ReadSettings ();
		void WriteSettings ();
	public slots:
		void updateAllRequested ();
		void handleUpdatesIntervalChanged ();
		void timeredUpdateAllRequested ();
		void upgradeAllRequested ();
		void cancelPending ();
		void acceptPending ();
		void removeRequested (const QString&, const QModelIndexList&);
		void addRequested (const QString&, const QVariantList&);
	private slots:
		void handleInfoFetched (const RepoInfo&);
		void handleComponentFetched (const PackageShortInfoList&,
				const QString&, int);
		void handlePackageFetched (const PackageInfo&, int);
		void handlePackageInstallError (int, const QString&);
		void handlePackageInstalled (int);
		void handlePackageUpdated (int from, int to);
		void handlePackageRemoved (int);
	signals:
		void delegateEntity (const LeechCraft::Entity&,
				int*, QObject**);
		void gotEntity (const LeechCraft::Entity&);
		void tagsUpdated (const QStringList&);
		void packageRowActionFinished (int row);

		void openLackmanRequested ();
	};
}
}

#endif
