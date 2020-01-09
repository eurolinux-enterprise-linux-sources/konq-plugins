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
 
#include "protocolplugin.h"

#include <html_document.h>

ProtocolPlugin* ProtocolPlugin::activePlugin = 0;

ProtocolPlugin::ProtocolPlugin(KHTMLPart *html, MetabarFunctions *functions, const char* name) : QObject(html, name), m_html(html), m_functions(functions)
{
}

ProtocolPlugin::~ProtocolPlugin()
{
}
    
void ProtocolPlugin::setFileItems(const KFileItemList &items)
{
  m_items = items;
  
  killJobs();
  
  DOM::HTMLDocument doc = m_html->htmlDocument();
  DOM::HTMLElement actions = doc.getElementById("actions");
  DOM::HTMLElement applications = doc.getElementById("open");
  DOM::HTMLElement info = doc.getElementById("info");
  DOM::HTMLElement preview = doc.getElementById("preview");
  DOM::HTMLElement bookmarks = doc.getElementById("bookmarks");
  
  if(!actions.isNull()){
    loadActions(actions);
    m_functions->adjustSize("actions");
  }
    
  if(!applications.isNull()){
    loadApplications(applications);
    m_functions->adjustSize("open");
  }
  
  if(!info.isNull()){
    loadInformation(info);
    m_functions->adjustSize("info");
  }
  
  if(!preview.isNull()){
    loadPreview(preview);
    m_functions->adjustSize("preview");
  }
  
  if(!bookmarks.isNull()){
    loadBookmarks(bookmarks);
    m_functions->adjustSize("bookmarks");
  }
  
  doc.updateRendering();
}

bool ProtocolPlugin::isActive()
{
  return activePlugin == this;
}

#include "protocolplugin.moc"
