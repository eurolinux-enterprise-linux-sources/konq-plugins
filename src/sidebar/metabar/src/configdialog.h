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
#ifndef _CONFIGDIALOG_H_
#define _CONFIGDIALOG_H_

#include <qdialog.h>
//Added by qt3to4:
#include <QPixmap>
#include <Q3CString>
#include <kpushbutton.h>
#include <k3listview.h>
#include <knuminput.h>
#include <kconfig.h>
#include <q3ptrdict.h>
#include <q3listbox.h>
#include <kactionselector.h>
#include <qcheckbox.h>
#include <kcombobox.h>

class LinkEntry{
  public:
    LinkEntry(const QString &name, const QString &url, const QString &icon);
    
    ~LinkEntry(){}
    
    QString name;
    QString url;
    QString icon;
};
 
class ConfigDialog : public QDialog
{
  Q_OBJECT
  
  public:
    explicit ConfigDialog(QWidget *parent = 0, const char *name = 0);
    ~ConfigDialog();
    
  protected:
    KPushButton *ok;
    KPushButton *cancel;
  
    KPushButton *link_create;
    KPushButton *link_delete;
    KPushButton *link_edit;
    KPushButton *link_up;
    KPushButton *link_down;
    
    KPushButton *install_theme;
    
    KIntSpinBox *max_entries;
    KIntSpinBox *max_actions;
    
    QCheckBox *animate;
    QCheckBox *servicemenus;
    
    K3ListView *link_list;
    
    KComboBox *themes;
    
    Q3CString topWidgetName;
    
    KActionSelector *actionSelector;
    
    Q3PtrDict<LinkEntry> linkList;
    
    KConfig *config;
    KConfig *iconConfig;
    
  protected slots:
    void accept();
    void createLink();
    void deleteLink();
    void editLink();
    void editLink(Q3ListViewItem *item);
    void moveLinkUp();
    void moveLinkDown();
    void updateArrows();
    void installTheme();
  
  private:
    void loadAvailableActions();
    void loadThemes();
};

class ActionListItem : public Q3ListBoxPixmap
{
  public:
    ActionListItem(Q3ListBox *listbox, const QString &action, const QString &text, const QPixmap &pixmap);
    ~ActionListItem(){}
  
    const QString action() { return act; };
    void setAction(const QString act){ ActionListItem::act = act; };
  
  private:
    QString act;
};

#endif
