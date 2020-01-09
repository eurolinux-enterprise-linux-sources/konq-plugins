/***************************************************************************
 *   Copyright (C) 2005 by Florian Roth   *
 *   florian@synatic.net   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/
 
#include "metabarwidget.h"
#include "httpplugin.h"

#include <kbookmark.h>
#include <kstandarddirs.h>
#include <kicontheme.h>
#include <khtmlview.h>
#include <kurl.h>
#include <klocale.h>

#include <dcopref.h>
#include <dcopclient.h>

#include <qregexp.h>
#include <qfile.h>

#include <dom_node.h>
#include <html_inline.h>

HTTPPlugin::HTTPPlugin(KHTMLPart* html, MetabarFunctions *functions, const char *name) : ProtocolPlugin (html, functions, name)
{
  //tmpfile = locateLocal("tmp", "metabartmpdoc");

  QString bookmarksFile = locateLocal("data", QLatin1String("konqueror/bookmarks.xml"));
  bookmarkManager = KBookmarkManager::managerForFile(bookmarksFile, "konqueror");
  
  dirWatch = new KDirWatch(this);
  dirWatch->addFile(bookmarksFile);
  
  connect(dirWatch, SIGNAL(dirty(const QString&)), this, SLOT(slotUpdateBookmarks(const QString&)));
  connect(dirWatch, SIGNAL(created(const QString&)), this, SLOT(slotUpdateBookmarks(const QString&)));
  connect(dirWatch, SIGNAL(deleted(const QString&)), this, SLOT(slotUpdateBookmarks(const QString&)));
  
  loadBookmarks();
}

HTTPPlugin::~HTTPPlugin()
{
  //delete bookmarkManager;
}

void HTTPPlugin::deactivate()
{  
  m_functions->hide("actions");
  m_functions->hide("info");
}

void HTTPPlugin::killJobs()
{
}

void HTTPPlugin::loadInformation(DOM::HTMLElement node)
{  
  /*DOM::DOMString innerHTML;
  innerHTML += "<form action=\"find:///\" method=\"GET\">";
  innerHTML += "<ul>";
  innerHTML += i18n("Keyword");
  innerHTML += " <input onFocus=\"this.value = ''\" type=\"text\" name=\"find\" id=\"find_text\" value=\"";
  innerHTML += i18n("Web search");
  innerHTML += "\"></ul>";
  innerHTML += "<ul><input type=\"submit\" id=\"find_button\" value=\"";
  innerHTML += i18n("Find");
  innerHTML += "\"></ul>";
  innerHTML += "</form>";
  
  node.setInnerHTML(innerHTML);
  m_functions->show("info");*/

  m_functions->hide("info");
}

void HTTPPlugin::loadActions(DOM::HTMLElement node)
{
  m_functions->hide("actions");
}

void HTTPPlugin::loadApplications(DOM::HTMLElement node)
{
   m_functions->hide("open");
}

void HTTPPlugin::loadPreview(DOM::HTMLElement node)
{
   m_functions->hide("preview");
}

void HTTPPlugin::loadBookmarks(DOM::HTMLElement node)
{
  node.setInnerHTML(bookmarks);
   m_functions->show("bookmarks");
}

bool HTTPPlugin::handleRequest(const KUrl &url)
{
  if(url.protocol() == "find"){
    QString keyword = url.queryItem("find");
    QString type = url.queryItem("type");

    if(!keyword.isNull() && !keyword.isEmpty()){
      KUrl url("http://www.google.com/search");
      url.addQueryItem("q", keyword);
      
      DCOPRef ref(kapp->dcopClient()->appId(), m_html->view()->topLevelWidget()->name());
      DCOPReply reply = ref.call("openUrl", url.url());
    }
    
    return true;
  }
  return false;
}

void HTTPPlugin::loadBookmarkGroup(const KBookmarkGroup &group, DOM::DOMString &innerHTML, int groupID)
{
  QString groupID_str;

  for(KBookmark bm = group.first(); !bm.isNull(); bm = group.next(bm)){
    if(bm.isGroup()){
      groupID_str.setNum(groupID + 1);
      
      innerHTML += "<ul><a onClick=\"\" class=\"infotitle\" style=\"background-image: url(";
      innerHTML += MetabarWidget::getIconPath(bm.icon());
      innerHTML += ");\">";
      innerHTML += bm.text();
      innerHTML += "</a></ul>";
      
      loadBookmarkGroup(bm.toGroup(), innerHTML, groupID + 1);
    }
    else{
      if(bm.isSeparator()){
        innerHTML += "<ul class=\"separator\"><hr></ul>";
      }
      else{        
        MetabarWidget::addEntry(innerHTML, bm.text(), bm.url().url(), bm.icon());
      }
    }
  }
}

void HTTPPlugin::loadBookmarks()
{
  DOM::DOMString innerHTML; 

  int groupID = 0;
  KBookmarkGroup root = bookmarkManager->root();
  loadBookmarkGroup(root, innerHTML, groupID);
  
  bookmarks = innerHTML;
}

void HTTPPlugin::slotUpdateBookmarks(const QString &file)
{
  loadBookmarks();
  
  if(isActive()){
    DOM::HTMLDocument doc = m_html->htmlDocument();
    DOM::HTMLElement node = doc.getElementById("bookmarks");
    node.setInnerHTML(bookmarks);
  }
}

#include "httpplugin.moc"
