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
 
#ifndef METABAR_WIDGET_H
#define METABAR_WIDGET_H

#include <khtml_part.h>

#include <kconfig.h>
#include <kfileitem.h>
#include <kurl.h>
#include <kservice.h>
#include <kmenu.h>
#include <kdirwatch.h>

#include <qmap.h>
#include <Q3Dict>
#include "protocolplugin.h"
#include "metabarfunctions.h"

class MetabarWidget : public QWidget
{
  Q_OBJECT
    
  public:
    explicit MetabarWidget(QWidget *parent = 0, const char* name=0);
    ~MetabarWidget();
    
    void setFileItems(const KFileItemList &items, bool check = true);
    
    static QString getIconPath(const QString &name);
    static void addEntry(DOM::DOMString &html, const QString name, const QString url, const QString icon, const QString id = QString(), const QString nameatt = QString(), bool hidden = false);
    
  private:
    KFileItemList *currentItems;
    KConfig *config;

    KHTMLPart *html;
    
    ProtocolPlugin *currentPlugin;
    ProtocolPlugin *defaultPlugin;
    
    MetabarFunctions *functions;
    
    KDirWatch *dir_watch;
    KMenu *popup;
  
    Q3Dict<ProtocolPlugin> plugins;
    
    bool skip;
    bool loadComplete;

    void callAction(const QString &action);
    void openUrl(const QString &url);
    void openTab(const QString &url);
    void loadLinks();
    void setTheme();
    
    QString getCurrentURL();
    
  private slots:
    void loadCompleted();
    void slotShowSharingDialog();
    void slotShowConfig();
    void slotShowPopup(const QString &url, const QPoint &pos);
    void handleURLRequest(const KUrl &url, const KParts::URLArgs &args);
    void slotUpdateCurrentInfo(const QString &path);
    void slotDeleteCurrentInfo(const QString &path);
};

#endif
