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


#include <q3groupbox.h>
#include <qlayout.h>
#include <qtabwidget.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qmap.h>
#include <q3cstring.h>
#include <qdatastream.h> 
//Added by qt3to4:
#include <QPixmap>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <kdebug.h>
#include <klocale.h>
#include <kicondialog.h>
#include <kiconloader.h>
#include <kstandarddirs.h>
#include <kfiledialog.h>
#include <ktar.h>
#include <karchive.h>
#include <dcopref.h>
#include <dcopclient.h>

#include "configdialog.h"

LinkEntry::LinkEntry(const QString &name, const QString &url, const QString &icon){
  LinkEntry::name = name;
  LinkEntry::url = url;
  LinkEntry::icon = icon;
}

ActionListItem::ActionListItem(Q3ListBox *listbox, const QString &action, const QString &text, const QPixmap &pixmap) : Q3ListBoxPixmap(listbox, pixmap)
{
  setAction(action);
  setText(text);
}

ConfigDialog::ConfigDialog(QWidget *parent, const char *name) : QDialog(parent, name)
{
  topWidgetName = parent->topLevelWidget()->name();
  config = new KConfig("metabarrc");
  iconConfig = new KConfig(locate("data", "metabar/iconsrc"));

  setCaption(i18n("Configuration - Metabar"));
  setIcon(SmallIcon("metabar"));

  ok = new KPushButton(KStdGuiItem::ok(), this);
  connect(ok, SIGNAL(clicked()), this, SLOT(accept()));
  
  cancel = new KPushButton(KStdGuiItem::cancel(), this);
  connect(cancel, SIGNAL(clicked()), this, SLOT(reject()));
  
  QTabWidget *tab = new QTabWidget(this);
  
  //general page
  config->setGroup("General");
  QWidget *general = new QWidget;
  
  Q3GroupBox *entries_group = new Q3GroupBox(2, Qt::Horizontal, i18n("Items"), general);
  entries_group->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  
  QLabel *entries_label = new QLabel(i18n("Open with:"), entries_group);
  max_entries = new KIntSpinBox(entries_group);
  max_entries->setMinValue(1);
  max_entries->setMaxValue(99);
  max_entries->setLineStep(1);
  max_entries->setValue(config->readNumEntry("MaxEntries", 3));
  entries_label->setBuddy(max_entries);
  
  QLabel *actions_label = new QLabel(i18n("Actions:"), entries_group);
  max_actions = new KIntSpinBox(entries_group);
  max_actions->setMinValue(1);
  max_actions->setMaxValue(99);
  max_actions->setLineStep(1);
  max_actions->setValue(config->readNumEntry("MaxActions", 3));
  actions_label->setBuddy(max_actions);
  
  
  Q3GroupBox *appearance_group = new Q3GroupBox(1, Qt::Horizontal, i18n("Appearance"), general);
  appearance_group->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  
  animate = new QCheckBox(i18n("Animate resize"), appearance_group);
  animate->setChecked(config->readBoolEntry("AnimateResize", false));
  
  servicemenus = new QCheckBox(i18n("Show service menus"), appearance_group);
  servicemenus->setChecked(config->readBoolEntry("ShowServicemenus", true));
  
  Q3GroupBox *theme_group = new Q3GroupBox(2, Qt::Horizontal, i18n("Themes"), general);
  theme_group->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  
  themes = new KComboBox(theme_group);
  themes->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  
  install_theme = new KPushButton(i18n("Install New Theme..."), theme_group);
  install_theme->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  connect(install_theme, SIGNAL(clicked()), this, SLOT(installTheme()));
  
  loadThemes();
  
  //link page
  QWidget *links = new QWidget;
  
  link_create = new KPushButton(i18n("New..."), links);
  connect(link_create, SIGNAL(clicked()), this, SLOT(createLink()));
  
  link_delete = new KPushButton(i18n("Delete"), links);
  connect(link_delete, SIGNAL(clicked()), this, SLOT(deleteLink()));
  
  link_edit = new KPushButton(i18n("Edit..."), links);
  connect(link_edit, SIGNAL(clicked()), this, SLOT(editLink()));  
  
  link_up = new KPushButton(links);
  link_up->setIconSet(KIcon("go-up"));
  link_up->setEnabled(false);
  connect(link_up, SIGNAL(clicked()), this, SLOT(moveLinkUp()));
  
  link_down = new KPushButton(links);
  link_down->setIconSet(KIcon("go-down"));
  link_down->setEnabled(false);
  connect(link_down, SIGNAL(clicked()), this, SLOT(moveLinkDown()));
    
  link_list = new K3ListView(links);
  link_list->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  link_list->setSorting(-1);
  link_list->setItemsMovable(true);
  link_list->addColumn(i18n("Name"));
  link_list->addColumn(i18n("Address"));
  connect(link_list, SIGNAL(doubleClicked(Q3ListViewItem*)), this, SLOT(editLink(Q3ListViewItem*)));
  connect(link_list, SIGNAL(selectionChanged()), SLOT(updateArrows()));
  
  QWidget *actionPage = new QWidget;
  
  actionSelector = new KActionSelector(actionPage);
  loadAvailableActions();  
  
  tab->addTab(general, i18n("General"));
  tab->addTab(actionPage, i18n("Actions"));
  tab->addTab(links, i18n("Links"));
  
  //layout  
  QGridLayout *general_layout = new QGridLayout(general, 2, 2, 5, 5);
  general_layout->addWidget(entries_group, 0, 0);
  general_layout->addWidget(appearance_group, 0, 1);
  general_layout->addMultiCellWidget(theme_group, 1, 1, 0, 1);
  general_layout->addItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding), 2, 0);
  //general_layout->addItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding), 0, 2);
  
  QVBoxLayout *link_button_layout = new QVBoxLayout(0, 0, 5);
  link_button_layout->addWidget(link_create);
  link_button_layout->addWidget(link_edit);
  link_button_layout->addWidget(link_delete);
  link_button_layout->addItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
  link_button_layout->addWidget(link_up);
  link_button_layout->addWidget(link_down);
  
  QHBoxLayout *link_layout = new QHBoxLayout(links, 5, 5);
  link_layout->addWidget(link_list);
  link_layout->addLayout(link_button_layout);
  
  QHBoxLayout *action_layout = new QHBoxLayout(actionPage, 5, 5);
  action_layout->addWidget(actionSelector);
  
  QHBoxLayout *bottom_layout = new QHBoxLayout(0, 5, 5);
  bottom_layout->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum));
  bottom_layout->addWidget(ok);
  bottom_layout->addWidget(cancel);
  
  QVBoxLayout *main_layout = new QVBoxLayout(this, 5, 5);
  main_layout->addWidget(tab);
  main_layout->addLayout(bottom_layout);
  
  //load config
  config->setGroup("General");
  
  QStringList _links = config->readListEntry("Links");
  
  for(QStringList::Iterator it = _links.begin(); it != _links.end(); ++it){
    config->setGroup("Link_" + (*it));
    
    QString icon_str = config->readEntry("Icon", "folder");
    QPixmap icon(icon_str);
    if(icon.isNull()){
      icon = SmallIcon(icon_str);
    }
    
    Q3ListViewItem *item = new Q3ListViewItem(link_list, link_list->lastItem(), config->readEntry("Name"), config->readEntry("URL"));
    item->setPixmap(0, icon);
    
    linkList.insert(item, new LinkEntry(config->readEntry("Name"), config->readEntry("URL"), icon_str));
  }
  
  config->setGroup("General");
  QStringList actions = config->readListEntry("Actions");
  for(QStringList::Iterator it = actions.begin(); it != actions.end(); ++it){
    if((*it).startsWith("metabar/")){
      if((*it).right((*it).length() - 8) == "share"){
        QString text = i18n("Share");
      
        ActionListItem *item = new ActionListItem(actionSelector->selectedListWidget(), *it, text, SmallIcon("network-wired"));
        
        Q3ListBoxItem *avItem = actionSelector->availableListWidget()->findItem(text, Q3ListBox::ExactMatch);
        if(avItem){
          delete avItem;
        }
      }
    }
    else{  
      DCOPCString tmp(topWidgetName);
      tmp += "/action/";
      tmp += (*it).toLatin1();
      DCOPRef action(kapp->dcopClient()->appId(), tmp/*QString(topWidgetName).append("/action/").append(*it)*/);
          
      QString text = action.call("plainText()");
      QString icon = iconConfig->readEntry(*it, QString::fromLocal8Bits(action.call("icon()")));
      
      ActionListItem *item = new ActionListItem(actionSelector->selectedListBox(), QString(*it), text, SmallIcon(icon));
      
      Q3ListBoxItem *avItem = actionSelector->availableListBox()->findItem(text, Q3ListBox::ExactMatch);
      if(avItem){
        delete avItem;
      }
    }
  }
  
  resize(400, 300);
}

ConfigDialog::~ConfigDialog()
{
  delete config;
  delete iconConfig;
}

void ConfigDialog::accept()
{
  QStringList groups = config->groupList();
  for(QStringList::Iterator it = groups.begin(); it != groups.end(); ++it){
    if((*it).startsWith("Link_")){
      config->deleteGroup(*it);
    }
  }
  
  QStringList links;
  Q3PtrDictIterator<LinkEntry> it(linkList);
  
  Q3ListViewItem *item = link_list->firstChild();
  while(item) {
    LinkEntry *entry = linkList[item];
    config->setGroup("Link_" + entry->name);
    config->writeEntry("Name", entry->name);
    config->writeEntry("URL", entry->url);
    config->writeEntry("Icon", entry->icon);
    
    links.append(entry->name);
  
    item = item->nextSibling();
  }
  
  QStringList actions;
  Q3ListBox *box = actionSelector->selectedListBox();
  
  for(int i = 0; i < box->numRows(); i++){
    ActionListItem *item = static_cast<ActionListItem*>(box->item(i));
    if(item){
      actions.append(item->action());
    }
  }
  
  config->setGroup("General");
  config->writeEntry("Links", links);  
  config->writeEntry("Actions", actions);
  config->writeEntry("Theme", themes->currentText());
  
  config->writeEntry("MaxEntries", max_entries->value());
  config->writeEntry("MaxActions", max_actions->value());
  
  config->writeEntry("AnimateResize", animate->isChecked());
  config->writeEntry("ShowServicemenus", servicemenus->isChecked());
  
  config->sync();
  
  QDialog::accept();
}

void ConfigDialog::createLink()
{
  QDialog *main = new QDialog(this);
  main->setCaption(i18n("Create Link"));
  main->setIcon(SmallIcon("metabar"));
  
  KPushButton *ok = new KPushButton(KStdGuiItem::ok(), main);
  connect(ok, SIGNAL(clicked()), main, SLOT(accept()));
  
  KPushButton *cancel = new KPushButton(KStdGuiItem::cancel(), main);
  connect(cancel, SIGNAL(clicked()), main, SLOT(reject()));
  
  QLineEdit *name = new QLineEdit(i18n("New link"), main);
  QLineEdit *url = new QLineEdit("file:/", main);
  
  KIconButton *icon = new KIconButton(main);
  icon->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  icon->setIconType(KIconLoader::Small, KIconLoader::Any);
  icon->setStrictIconSize(true);
  icon->setIcon("folder");
  
  QHBoxLayout *bottom_layout = new QHBoxLayout(0, 0, 5);
  bottom_layout->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum));
  bottom_layout->addWidget(ok);
  bottom_layout->addWidget(cancel);
  
  QGridLayout *layout = new QGridLayout(0, 2, 3, 0, 5);
  layout->addWidget(icon, 0, 0);
  layout->addWidget(new QLabel(i18n("Name:"), main), 0, 1);
  layout->addWidget(name, 0, 2);
  layout->addWidget(new QLabel(i18n("URL:"), main), 1, 0);
  layout->addMultiCellWidget(url, 1, 1, 1, 2);
  
  QVBoxLayout *main_layout = new QVBoxLayout(main, 5, 5);
  main_layout->addLayout(layout);
  main_layout->addItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
  main_layout->addLayout(bottom_layout);
  
  main->resize(300, main->sizeHint().height());
  
  if(main->exec() == QDialog::Accepted){
    QString name_str = name->text();
    QString url_str = url->text();
    QString icon_str = icon->icon();
    
    if(!name_str.isEmpty() && !url_str.isEmpty()){
      if(icon_str.isEmpty()){
        icon_str = KIconLoader::global()->iconPath("folder", KIconLoader::Small);
      }
      
      QPixmap icon(icon_str);
      if(icon.isNull()){
        icon = SmallIcon(icon_str);
      }
      
      Q3ListViewItem *item = new Q3ListViewItem(link_list, link_list->lastItem(), name_str, url_str);
      item->setPixmap(0, icon);
      
      linkList.insert(item, new LinkEntry(name_str, url_str, icon_str));
      updateArrows();
    }
  }
  
  delete main;
}

void ConfigDialog::deleteLink()
{
  Q3ListViewItem *item = link_list->selectedItem();
  if(item){
    linkList.remove(item);
    delete item;
    updateArrows();
  }
}

void ConfigDialog::editLink()
{
  Q3ListViewItem *item = link_list->selectedItem();
  editLink(item);
}

void ConfigDialog::editLink(Q3ListViewItem *item)
{
  if(item){  
    QDialog *main = new QDialog(this);
    main->setCaption(i18n("Edit Link"));
    main->setIcon(SmallIcon("metabar"));
    
    KPushButton *ok = new KPushButton(KStdGuiItem::ok(), main);
    connect(ok, SIGNAL(clicked()), main, SLOT(accept()));
    
    KPushButton *cancel = new KPushButton(KStdGuiItem::cancel(), main);
    connect(cancel, SIGNAL(clicked()), main, SLOT(reject()));
    
    QLineEdit *name = new QLineEdit(linkList[item]->name, main);
    QLineEdit *url = new QLineEdit(linkList[item]->url, main);
    
    KIconButton *icon = new KIconButton(main);
    icon->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    icon->setIconType(KIconLoader::Small, KIconLoader::Any);
    icon->setStrictIconSize(true);
    icon->setIcon(linkList[item]->icon);
    
    QHBoxLayout *bottom_layout = new QHBoxLayout(0, 0, 5);
    bottom_layout->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum));
    bottom_layout->addWidget(ok);
    bottom_layout->addWidget(cancel);
    
    QGridLayout *layout = new QGridLayout(0, 2, 3, 0, 5);
    layout->addWidget(icon, 0, 0);
    layout->addWidget(new QLabel(i18n("Name:"), main), 0, 1);
    layout->addWidget(name, 0, 2);
    layout->addWidget(new QLabel(i18n("URL:"), main), 1, 0);
    layout->addMultiCellWidget(url, 1, 1, 1, 2);
    
    QVBoxLayout *main_layout = new QVBoxLayout(main, 5, 5);
    main_layout->addLayout(layout);
    main_layout->addItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
    main_layout->addLayout(bottom_layout);
    
    main->resize(300, main->sizeHint().height());
    
    if(main->exec() == QDialog::Accepted){
      QString name_str = name->text();
      QString url_str = url->text();
      QString icon_str = icon->icon();
      
      if(!name_str.isEmpty() && !url_str.isEmpty()){
        if(icon_str.isEmpty()){
          icon_str = KIconLoader::global()->iconPath("folder", KIconLoader::Small);
        }
        
        QPixmap icon(icon_str);
        if(icon.isNull()){
          icon = SmallIcon(icon_str);
        }
        
        linkList[item]->name = name_str;
        linkList[item]->url = url_str;
        linkList[item]->icon = icon_str;
        
        item->setText(0, name_str);
        item->setText(1, url_str);
        item->setPixmap(0, icon);
      }
    }
    
    delete main;
  }
}

void ConfigDialog::moveLinkUp()
{
  Q3ListViewItem *item = link_list->selectedItem();
  if(item){
    if(link_list->itemIndex(item) > 0){
      
      Q3ListViewItem *after;
      Q3ListViewItem *above = item->itemAbove();
      if(above){
        after = above->itemAbove();
      }
      
      QString name = linkList[item]->name;
      QString url = linkList[item]->url;
      QString icon_str = linkList[item]->icon;
      
      QPixmap icon(icon_str);
      if(icon.isNull()){
        icon = SmallIcon(icon_str);
      }
      
      delete linkList[item];
      linkList.remove(item);
      delete item;
      
      Q3ListViewItem *newItem = new Q3ListViewItem(link_list, after, name, url);
      newItem->setPixmap(0, icon);
      link_list->setSelected(newItem, true);
      
      linkList.insert(newItem, new LinkEntry(name, url, icon_str));
      updateArrows();
    }
  }
}

void ConfigDialog::moveLinkDown()
{
  Q3ListViewItem *item = link_list->selectedItem();
  if(item){
    if(link_list->itemIndex(item) < linkList.count() - 1){
      
      Q3ListViewItem *after = item->itemBelow();
      
      QString name = linkList[item]->name;
      QString url = linkList[item]->url;
      QString icon_str = linkList[item]->icon;
      
      QPixmap icon(icon_str);
      if(icon.isNull()){
        icon = SmallIcon(icon_str);
      }
      
      delete linkList[item];
      linkList.remove(item);
      delete item;
      
      Q3ListViewItem *newItem = new Q3ListViewItem(link_list, after, name, url);
      newItem->setPixmap(0, icon);
      link_list->setSelected(newItem, true);
      
      linkList.insert(newItem, new LinkEntry(name, url, icon_str));
      updateArrows();
    }
  }
}

void ConfigDialog::loadAvailableActions()
{
  Q3ListBox *box = actionSelector->availableListBox();
  
  QByteArray data, replyData;
  DCOPCString replyType;

  if(DCOPClient::mainClient()->call(kapp->dcopClient()->appId(), topWidgetName, "actionMap()", data, replyType, replyData)){
    if(replyType == "QMap<QCString,DCOPRef>"){    
      QMap<Q3CString,DCOPRef> actionMap;
      
      QDataStream reply(&replyData, QIODevice::ReadOnly);
      reply >> actionMap;
      
      iconConfig->setGroup("Icons");
      
      QMap<Q3CString,DCOPRef>::Iterator it;
      for(it = actionMap.begin(); it != actionMap.end(); ++it){
        DCOPRef action = it.data();
        
        QString text = action.call("plainText()");
        Q3CString cname = action.call("name()");
        QString icon;// = iconConfig->readEntry(QString(cname), action.call("icon()"));
        
        ActionListItem *item = new ActionListItem(box, QString(cname), text, SmallIcon(icon));
      }
    }
  }
  
  //metabar's own actions
  ActionListItem *item = new ActionListItem(box, "metabar/share", i18n("Share"), SmallIcon("network-wired"));
}

void ConfigDialog::updateArrows()
{
  link_up->setEnabled( link_list->childCount()>1 && link_list->currentItem()!=link_list->firstChild() );
  link_down->setEnabled( link_list->childCount()>1 && link_list->currentItem()!=link_list->lastItem() );
}

void ConfigDialog::loadThemes()
{
  themes->clear();
  
  QString theme = config->readEntry("Theme", "default");
  bool foundTheme = false;

  QStringList dirs = KGlobal::mainComponent().dirs()->findDirs("data", "metabar/themes");
  for(QStringList::Iterator it = dirs.begin(); it != dirs.end(); ++it){
    QStringList theme_list = QDir(*it).entryList(QDir::Dirs);
    
    theme_list.remove(".");
    theme_list.remove("..");
    themes->insertStringList(theme_list);
    
    if(theme_list.find(theme) != theme_list.end()){
      foundTheme = true;
    }
  }
  
  if(foundTheme){
    themes->setCurrentText(theme);
  }
  else{ 
    themes->insertItem(theme);
  }
}

void ConfigDialog::installTheme()
{
  QString file = KFileDialog::getOpenFileName();
  if(file.isNull() && file.isEmpty()) return;
  
  QString themedir = locateLocal("data", "metabar/themes");  
  if(themedir.isNull()) return;

  KTar archive(file);
  archive.open(QIODevice::ReadOnly);
  qApp->processEvents();

  const KArchiveDirectory* rootDir = archive.directory();
  rootDir->copyTo(themedir);
  
  archive.close();
  loadThemes();
}

#include "configdialog.moc"
