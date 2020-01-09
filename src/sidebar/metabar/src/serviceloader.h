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

#ifndef _SERVICELOADER_H_
#define _SERVICELOADER_H_

#include <qstring.h>
#include <q3dict.h>
#include <qmap.h>
#include <qwidget.h>

#include <kmenu.h>
#include <kfileitem.h>
#include <kmimetype.h>
#include <kurl.h>

#include <dom_string.h>
#include <kdesktopfileactions.h>
#include "metabarwidget.h"

class ServiceLoader : public QObject
{
  Q_OBJECT

  public:
    explicit ServiceLoader(QWidget *parent, const char *name = 0);
    ~ServiceLoader();
    
    void  loadServices(const KFileItem item, DOM::DOMString &html, int &count);
    void runAction(const QString &name);
    void showPopup(const QString &popup, const QPoint &point);
    
  private:
    Q3Dict<KMenu> popups;
    QMap<QString,KDesktopFileActions::Service> services;
    KUrl::List urlList;
    
  private slots:
    void runAction();
};

#endif
