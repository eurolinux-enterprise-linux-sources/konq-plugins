/*
    Copyright (c) 2002-2003 Alexander Kellett <lypanov@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License (LGPL) as published by the Free Software Foundation;
    either version 2 of the License, or (at your option) any later
    version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <kdebug.h>
#include <kaction.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kcomponentdata.h>
#include <khtml_part.h>
#include <kgenericfactory.h>
#include <kicon.h>
#include <krun.h>
#include <kservice.h>
#include <kactionmenu.h>
#include <kmenu.h>
#include <kbookmarkimporter_crash.h>

#include "crashesplugin.h"
#include <kactioncollection.h>
#include <limits.h>

K_PLUGIN_FACTORY(CrashesPluginFactory, registerPlugin<CrashesPlugin>();)
K_EXPORT_PLUGIN(CrashesPluginFactory( "crashesplugin" ))


CrashesPlugin::CrashesPlugin( QObject* parent, const QVariantList & )
: KParts::Plugin( parent )
{
    m_part = qobject_cast<KParts::ReadOnlyPart *>( parent );

  m_pCrashesMenu = new KActionMenu(KIcon("core"), i18n("&Crashes"),
                               actionCollection() );

  actionCollection()->addAction("crashes", m_pCrashesMenu);
  m_pCrashesMenu->setDelayed( false );
  m_pCrashesMenu->setEnabled( true );

  connect( m_pCrashesMenu->menu(), SIGNAL( aboutToShow() ),
           this, SLOT( slotAboutToShow() ) );
}

CrashesPlugin::~CrashesPlugin()
{
}

void CrashesPlugin::slotAboutToShow()
{
  m_pCrashesMenu->menu()->clear();

  KCrashBookmarkImporter importer(KCrashBookmarkImporter::crashBookmarksDir());

  connect( &importer, SIGNAL( newBookmark( const QString &, const QString &, const QString &) ),
                      SLOT( newBookmarkCallback( const QString &, const QString &, const QString & ) ) );

  connect( &importer, SIGNAL( endFolder() ), SLOT( endFolderCallback() ) );

  int count = m_pCrashesMenu->menu()->count();

  m_crashesList.clear();
  m_crashRangesList.clear();
  importer.parseCrashBookmarks( false );

  bool gotSep = true; // don't start with a sep
  bool enable = true;
  int firstItem = count; // item ids grow up from count
  int crashGroup = INT_MAX; // group ids grow down from INT_MAX
  if (m_crashesList.count() > 0) {
     CrashesList::ConstIterator e = m_crashesList.constBegin();
     for( ; e != m_crashesList.constEnd(); ++e ) {
        if ( ((*e).first  == "-")
          && ((*e).second == "-")
        ) {
           if (!gotSep) {
              if (count - firstItem > 1)
              {
                 m_crashRangesList.append( CrashRange(firstItem, count) );
                 m_pCrashesMenu->menu()->insertItem(
                    i18n("All Pages of This Crash"), this,
                    SLOT(slotGroupSelected(int)),
                    0, crashGroup--);
              }
              m_pCrashesMenu->menu()->addSeparator();
           }
           gotSep = true;
           firstItem = ++count;
        } else {
           QString str = (*e).first;
           if (str.length() > 48) {
              str.truncate(48);
              str.append("...");
           }
           m_pCrashesMenu->menu()->insertItem(
              str, this,
              SLOT(slotItemSelected(int)),
              0, ++count );
           gotSep = false;
        }
     }
     if (count - firstItem > 1) {
        m_crashRangesList.append( CrashRange(firstItem, count) );
        m_pCrashesMenu->menu()->insertItem(
           i18n("All Pages of This Crash"), this,
           SLOT(slotGroupSelected(int)),
           0, crashGroup--);
     }
  } else {
     m_pCrashesMenu->menu()->insertItem(
        i18n("No Recovered Crashes"), this,
        SLOT(slotItemSelected(int)),
        0, ++count );
     gotSep = false;
     enable = false;
  }

  if (!gotSep) {
     // don't have an extra sep
     m_pCrashesMenu->menu()->addSeparator();
  }

  QAction *act =m_pCrashesMenu->menu()->addAction( i18n("&Clear List of Crashes"), this,
                                                   SLOT(slotClearCrashes()) );
  act->setEnabled( enable );
}

void CrashesPlugin::newBookmarkCallback( const QString & text, const QString & url,
                                         const QString & )
{
  m_crashesList.prepend(qMakePair(text,url));
}

void CrashesPlugin::endFolderCallback( )
{
  m_crashesList.prepend(qMakePair(QString('-'),QString('-')));
}

void CrashesPlugin::slotClearCrashes() {
  KCrashBookmarkImporter importer(KCrashBookmarkImporter::crashBookmarksDir());
  importer.parseCrashBookmarks( true );
  slotAboutToShow();
}

void CrashesPlugin::slotItemSelected( int id )
{
  if (m_crashesList.isEmpty())
     return;
  KUrl url( m_crashesList[id-1].second );
  KParts::BrowserExtension * ext = KParts::BrowserExtension::childObject(m_part);
  if (ext)
     emit ext->openUrlRequest( url );
}

void CrashesPlugin::slotGroupSelected( int range )
{
  if (!m_part)
     return;

  range = INT_MAX - range;

  if (m_crashesList.isEmpty() || m_crashRangesList.isEmpty())
     return;

  CrashRange r = m_crashRangesList[range];
  int from = r.first;
  int i = from;

  if (m_part) {
     KParts::OpenUrlArguments arguments;
     KParts::BrowserArguments browserArguments;
     browserArguments.setNewTab( true );
     KParts::BrowserExtension * ext = KParts::BrowserExtension::childObject(m_part);
     if ( ext )
     {
         do {
             KUrl url( m_crashesList[i].second );
             // Open first one in current tab
             if (i == from)
                 emit ext->openUrlRequest( url );
             else
                 emit ext->createNewWindow( url, arguments, browserArguments );
         } while (++i < r.second);
     }
  }
}

#include "crashesplugin.moc"
