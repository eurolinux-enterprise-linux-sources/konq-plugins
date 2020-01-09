/* This file is part of the KDE Project
   Copyright (C) 2001 Kurt Granroth <granroth@kde.org>
     Original code: plugin code, connecting to Babelfish and support for selected text
   Copyright (C) 2003 Rand2342 <rand2342@yahoo.com>
     Submenus, KConfig file and support for other translation engines
   Copyright (C) 2008 Montel Laurent <montel@kde.org>
     Add webkitkde support

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
#include "plugin_babelfish.h"

#include "config-babelfish.h"

#include <kparts/part.h>
#include <kparts/browserextension.h>

#include <kactionmenu.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <qmenu.h>
#include <kcomponentdata.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kconfig.h>
#include <kgenericfactory.h>
#include <kaboutdata.h>
#include <kicon.h>
#include <KConfigGroup>
#include <KHTMLPart>

#ifdef HAVE_WEBKITKDE
#include <webkitpart.h>
#include <QWebView>
#endif


static const KAboutData aboutdata("babelfish", 0, ki18n("Translate Web Page") , "1.0" );
K_PLUGIN_FACTORY(BabelFishFactory, registerPlugin<PluginBabelFish>();)
K_EXPORT_PLUGIN(BabelFishFactory( aboutdata ) )

PluginBabelFish::PluginBabelFish( QObject* parent,
	                          const QVariantList & )
  : Plugin( parent )
{
  setComponentData(BabelFishFactory::componentData());

  m_menu = new KActionMenu(KIcon("babelfish"), i18n("Translate Web &Page"),
                          actionCollection() );
  actionCollection()->addAction( "translatewebpage", m_menu );

  m_menu->setDelayed( false );

  m_en = new KActionMenu(KIcon("babelfish"), i18n("&English To"),
    actionCollection());
  actionCollection()->addAction( "translatewebpage_en", m_en );
  m_fr = new KActionMenu(KIcon("babelfish"), i18n("&French To"),
    actionCollection() );
  actionCollection()->addAction( "translatewebpage_fr", m_fr );
  m_de = new KActionMenu( KIcon("babelfish"),i18n("&German To"),
    actionCollection() );
  actionCollection()->addAction( "translatewebpage_de", m_de );

  m_es = new KActionMenu(KIcon("babelfish"), i18n("&Spanish To"),
    actionCollection() );
  actionCollection()->addAction( "translatewebpage_es", m_es );

  m_pt = new KActionMenu( KIcon("babelfish"),i18n("&Portuguese To"),
    actionCollection() );
  actionCollection()->addAction( "translatewebpage_pt", m_pt );

  m_it = new KActionMenu( KIcon("babelfish"),i18n("&Italian To"),
    actionCollection() );
  actionCollection()->addAction( "translatewebpage_it", m_it );

  m_nl = new KActionMenu( KIcon("babelfish"),i18n("&Dutch To"),
    actionCollection() );
  actionCollection()->addAction( "translatewebpage_nl", m_nl );


 static const char * const translations[] = {
      I18N_NOOP("&Chinese (Simplified)"), "en_zh",
      I18N_NOOP("Chinese (&Traditional)"), "en_zhTW",
      I18N_NOOP("&Dutch"), "en_nl",
      I18N_NOOP("&French"), "en_fr",
      I18N_NOOP("&German"), "en_de",
      I18N_NOOP("&Italian"), "en_it",
      I18N_NOOP("&Japanese"), "en_ja",
      I18N_NOOP("&Korean"), "en_ko",
      I18N_NOOP("&Norwegian"), "en_no",
      I18N_NOOP("&Portuguese"), "en_pt",
      I18N_NOOP("&Russian"), "en_ru",
      I18N_NOOP("&Spanish"), "en_es",
      I18N_NOOP("T&hai"), "en_th",
      I18N_NOOP("&Dutch"), "fr_nl",
      I18N_NOOP("&English"), "fr_en",
      I18N_NOOP("&German"), "fr_de",
      I18N_NOOP("&Italian"), "fr_it",
      I18N_NOOP("&Portuguese"), "fr_pt",
      I18N_NOOP("&Spanish"), "fr_es",
      I18N_NOOP("&English"), "de_en",
      I18N_NOOP("&French"), "de_fr",
      I18N_NOOP("&English"), "es_en",
      I18N_NOOP("&French"), "es_fr",
      I18N_NOOP("&English"), "pt_en",
      I18N_NOOP("&French"), "pt_fr",
      I18N_NOOP("&English"), "it_en",
      I18N_NOOP("&French"), "it_fr",
      I18N_NOOP("&English"), "nl_en",
      I18N_NOOP("&French"), "nl_fr",
      I18N_NOOP("&Chinese (Simplified) to English"), "zh_en",
      I18N_NOOP("Chinese (&Traditional) to English"), "zhTW_en",
      0, 0
  };

  for (int i = 0; translations[i]; i += 2) {
    QString translation = QString::fromLatin1(translations[i + 1]);
    QAction *a = actionCollection()->addAction(  translation);
    a->setText(i18n(translations[i]));
    connect(a, SIGNAL(triggered()), this, SLOT(translateURL()));

    const int underScorePos = translation.indexOf(QLatin1Char('_'));
    QString srcLang = translation.left(underScorePos);
    QAction *srcAction = actionCollection()->action(QString::fromLatin1("translatewebpage_") + srcLang);
    if (KActionMenu *actionMenu = qobject_cast<KActionMenu *>(srcAction))
      actionMenu->addAction(a);
  }

  QAction *a = actionCollection()->addAction(  "zh_en");
  a->setText(i18n("&Chinese (Simplified) to English"));
  connect(a, SIGNAL(triggered()), this, SLOT(translateURL()));
  m_menu->addAction(a);

  a = actionCollection()->addAction(  "zhTW_en");
  a->setText(i18n("Chinese (&Traditional) to English"));
  connect(a, SIGNAL(triggered()), this, SLOT(translateURL()));
  m_menu->addAction(a);

  m_menu->addAction( m_nl );
  m_menu->addAction( m_en );
  m_menu->addAction( m_fr );
  m_menu->addAction( m_de );
  m_menu->addAction( m_it );

  a = actionCollection()->addAction( "ja_en");
  a->setText(i18n("&Japanese to English"));
  connect(a, SIGNAL(triggered()), this, SLOT(translateURL()));
  m_menu->addAction(a);

  a = actionCollection()->addAction(  "ko_en");
  a->setText(i18n("&Korean to English"));
  connect(a, SIGNAL(triggered()), this, SLOT(translateURL()));
  m_menu->addAction(a);

  m_menu->addAction( m_pt );

  a = actionCollection()->addAction( "ru_en");
  a->setText(i18n("&Russian to English"));
  connect(a, SIGNAL(triggered()), this, SLOT(translateURL()));
  m_menu->addAction(a);

  m_menu->addAction( m_es );
  m_menu->setEnabled( true );

  // TODO: we could also support plain text viewers...
  if ( parent  )
  {
    KParts::ReadOnlyPart* part = static_cast<KParts::ReadOnlyPart *>(parent);
    connect( part, SIGNAL(started(KIO::Job*)), this,
             SLOT(slotStarted(KIO::Job*)) );
  }
}

PluginBabelFish::~PluginBabelFish()
{
  delete m_menu;
}

void PluginBabelFish::slotStarted( KIO::Job* )
{
  if ( ( parent()->inherits("KHTMLPart")||parent()->inherits("WebKitPart") ) &&
  // Babelfish wants http URLs only. No https.
       static_cast<KParts::ReadOnlyPart *>(parent())->url().protocol().toLower() == "http" )
  {
      m_menu->setEnabled( true );
  }
  else
  {
      m_menu->setEnabled( false );
  }
}

void PluginBabelFish::translateURL()
{
  // we need the sender() for the language name
  if ( !sender() )
    return;

  // The parent is assumed to be a KHTMLPart
  if ( !parent()->inherits("KHTMLPart")&& !parent()->inherits("WebKitPart") )
  {
    QString title = i18n( "Cannot Translate Source" );
    QString text = i18n( "Only web pages can be translated using "
                         "this plugin." );

    KMessageBox::sorry( 0L, text, title );
    return;
  }

  // Select engine
  KConfig cfg( "translaterc" );
  KConfigGroup grp(&cfg, QString());
  QString engine = grp.readEntry( sender()->objectName(), QString("babelfish") );

  // Get URL
  KParts::BrowserExtension * ext = KParts::BrowserExtension::childObject(parent());
  if ( !ext )
    return;

  // we check if we have text selected.  if so, we translate that. If
  // not, we translate the url
  QString totrans;


  KHTMLPart *part = dynamic_cast<KHTMLPart *>(parent());
  bool hasSelection = false;
  QString selection;
  KUrl url;
  if ( part )
  {
      hasSelection = part->hasSelection();
      selection = part->selectedText();
      url = part->url();
  }
  else
  {
#ifdef HAVE_WEBKITKDE
      WebKitPart *part = dynamic_cast<WebKitPart *>(parent());
      hasSelection = !part->view()->selectedText().isEmpty();
      selection = part->view()->selectedText();
      url = KUrl( part->view()->url() );
#endif
  }
  if ( hasSelection )
  {
    if( engine == "reverso" || engine == "tsail" )
    {
      KMessageBox::sorry( 0L,
              i18n( "Only full webpages can be translated for this language pair." ),i18n( "Translation Error" ) );
      return;
    }
    totrans = KUrl::encode_string( selection );
  } else {
    // Check syntax
    if ( !url.isValid() )
    {
      QString title = i18n( "Malformed URL" );
      QString text = i18n( "The URL you entered is not valid, please "
                           "correct it and try again." );
      KMessageBox::sorry( 0L, text, title );
      return;
    }
    totrans = KUrl::encode_string( url.url() );
  }

  // Create URL
  KUrl result;
  QString query;
  if( engine == "freetranslation" ) {
    query = "sequence=core&Submit=FREE Translation&language=";
    if( sender()->objectName() == QString( "en_es" ) )
      query += "English/Spanish";
    else if( sender()->objectName() == QString( "en_de" ) )
      query += "English/German";
    else if( sender()->objectName() == QString( "en_it" ) )
      query += "English/Italian";
    else if( sender()->objectName() == QString( "en_nl" ) )
      query += "English/Dutch";
    else if( sender()->objectName() == QString( "en_pt" ) )
      query += "English/Portuguese";
    else if( sender()->objectName() == QString( "en_no" ) )
      query += "English/Norwegian";
    else if( sender()->objectName() == QString( "en_zh" ) )
      query += "English/SimplifiedChinese";
    else if( sender()->objectName() == QString( "en_zhTW" ) )
      query += "English/TraditionalChinese";
    else if( sender()->objectName() == QString( "es_en" ) )
      query += "Spanish/English";
    else if( sender()->objectName() == QString( "fr_en" ) )
      query += "French/English";
    else if( sender()->objectName() == QString( "de_en" ) )
      query += "German/English";
    else if( sender()->objectName() == QString( "it_en" ) )
      query += "Italian/English";
    else if( sender()->objectName() == QString( "nl_en" ) )
      query += "Dutch/English";
    else if( sender()->objectName() == QString( "pt_en" ) )
      query += "Portuguese/English";
    else // Should be en_fr
      query += "English/French";
    if ( hasSelection )
    {
      result = KUrl( "http://ets.freetranslation.com" );
      query += "&mode=html&template=results_en-us.htm&srctext=";
    } else {
      result = KUrl( "http://www.freetranslation.com/web.asp" );
      query += "&url=";
    }
    query += totrans;
  } else if( engine == "parsit" ) {
    // Does only English -> Thai
    result = KUrl( "http://c3po.links.nectec.or.th/cgi-bin/Parsitcgi.exe" );
    query = "mode=test&inputtype=";
    if ( hasSelection )
      query += "text&TxtEng=";
    else
      query += "html&inputurl=";
    query += totrans;
  } else if( engine == "reverso" ) {
    result = KUrl( "http://www.reverso.net/url/frame.asp" );
    query = "autotranslate=on&templates=0&x=0&y=0&directions=";
    if( sender()->objectName() == QString( "de_fr" ) )
      query += "524292";
    else if( sender()->objectName() == QString( "fr_en" ) )
      query += "65544";
    else if( sender()->objectName() == QString( "fr_de" ) )
      query += "262152";
    else if( sender()->objectName() == QString( "de_en" ) )
      query += "65540";
    else if( sender()->objectName() == QString( "en_de" ) )
      query += "262145";
    else if( sender()->objectName() == QString( "en_es" ) )
      query += "2097153";
    else if( sender()->objectName() == QString( "es_en" ) )
      query += "65568";
    else if( sender()->objectName() == QString( "fr_es" ) )
      query += "2097160";
    else // "en_fr"
      query += "524289";
    query += "&url=";
    query += totrans;
  } else if( engine == "tsail" ) {
    result = KUrl( "http://www.t-mail.com/cgi-bin/tsail" );
    query = "sail=full&lp=";
    if( sender()->objectName() == QString( "zhTW_en" ) )
      query += "tw-en";
    else if( sender()->objectName() == QString( "en_zhTW" ) )
      query += "en-tw";
    else
    {
      query += sender()->objectName();
      query[15] = '-';
    }
    query += totrans;
  } else if( engine == "voila" ) {
    result = KUrl( "http://trans.voila.fr/voila" );
    query = "systran_id=Voila-fr&systran_lp=";
    query += sender()->objectName();
    if ( hasSelection )
      query += "&systran_charset=utf-8&systran_text=";
    else
      query += "&systran_url=";
    query += totrans;
  } else {
    // Using the altavista babelfish engine
    result = KUrl( "http://babelfish.altavista.com/babelfish/tr" );
    query = "lp=";
    query += sender()->objectName();
    if ( hasSelection )
      query += "&text=";
    else
      query += "&url=";
    query += totrans;
  }

  result.setQuery( query );

  // Connect to the fish
  emit ext->openUrlRequest( result );
}

#include <plugin_babelfish.moc>
