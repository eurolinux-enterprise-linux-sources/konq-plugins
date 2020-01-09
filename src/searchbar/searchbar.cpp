/* This file is part of the KDE project
   Copyright (C) 2004 Arend van Beelen jr. <arend@auton.nl>
   Copyright (C) 2009 Fredy Yanardi <fyanardi@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "searchbar.h"

#include "OpenSearchManager.h"
#include "WebShortcutWidget.h"

#include <unistd.h>
#include <QLineEdit>
#include <QApplication>
#include <DOM/Element>
#include <DOM/HTMLCollection>
#include <DOM/HTMLDocument>
#include <DOM/HTMLElement>
#include <DOM/Node>
#include <DOM/NodeList>
#include <kaction.h>
#include <KBuildSycocaProgressDialog>
#include <KCompletionBox>
#include <kconfig.h>
#include <ksharedconfig.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kgenericfactory.h>
#include <kglobal.h>
#include <khtml_part.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kprotocolmanager.h>
#include <kstandarddirs.h>
#include <kurifilter.h>
#include <kservice.h>
#include <kparts/mainwindow.h>
#include <kactioncollection.h>

#include <qpainter.h>
#include <qmenu.h>
#include <qtimer.h>
#include <qstyle.h>
#include <QPixmap>
#include <QMouseEvent>
#include <QtCore/QTimer>
#include <QtDBus/QtDBus>

K_PLUGIN_FACTORY(SearchBarPluginFactory, registerPlugin<SearchBarPlugin>();)
K_EXPORT_PLUGIN(SearchBarPluginFactory("searchbarplugin"))

SearchBarPlugin::SearchBarPlugin(QObject *parent,
                                 const QVariantList &) :
    KParts::Plugin(parent),
    m_part(0),
    m_searchCombo(0),
    m_searchMode(UseSearchProvider),
    m_urlEnterLock(false),
    m_process(0),
    m_openSearchManager(new OpenSearchManager(this))
{
    m_searchCombo = new SearchBarCombo(0);
    m_searchCombo->lineEdit()->installEventFilter(this);
    connect(m_searchCombo, SIGNAL(activated(const QString &)),
            SLOT(startSearch(const QString &)));
    connect(m_searchCombo, SIGNAL(iconClicked()), SLOT(showSelectionMenu()));
    m_searchCombo->setWhatsThis(i18n("Search Bar<p>"
                                     "Enter a search term. Click on the icon to change search mode or provider.</p>"));
    connect(m_searchCombo, SIGNAL(suggestionEnabled(bool)), this, SLOT(enableSuggestion(bool)));

    m_popupMenu = 0;
    m_addWSWidget = 0;

    m_searchComboAction = actionCollection()->addAction("toolbar_search_bar");
    m_searchComboAction->setText(i18n("Search Bar"));
    m_searchComboAction->setDefaultWidget(m_searchCombo);
    m_searchComboAction->setShortcutConfigurable(false);

    KAction *a = actionCollection()->addAction("focus_search_bar");
    a->setText(i18n("Focus Searchbar"));
    a->setShortcut(Qt::CTRL+Qt::Key_S);
    connect(a, SIGNAL(triggered()), this, SLOT(focusSearchbar()));

    configurationChanged();

    // parent is the KonqMainWindow and we want to listen to PartActivateEvent events.
    parent->installEventFilter(this);

    connect(m_searchCombo->lineEdit(), SIGNAL(textEdited(const QString &)), SLOT(searchTextChanged(const QString &)));
    connect(m_openSearchManager, SIGNAL(suggestionReceived(const QStringList &)),
            SLOT(addSearchSuggestion(const QStringList &)));
    connect(m_openSearchManager, SIGNAL(openSearchEngineAdded(const QString &, const QString &, const QString &)),
            SLOT(openSearchEngineAdded(const QString &, const QString &, const QString &)));
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, SIGNAL(timeout()), SLOT(requestSuggestion()));
}

SearchBarPlugin::~SearchBarPlugin()
{
    KConfigGroup config(KGlobal::config(), "SearchBar");
    config.writeEntry("Mode", (int) m_searchMode);
    config.writeEntry("CurrentEngine", m_currentEngine);
    config.writeEntry("SuggestionEnabled", m_suggestionEnabled);

    delete m_searchCombo;
    m_searchCombo = 0L;
    delete m_process;
    m_process=0L;
}

bool SearchBarPlugin::eventFilter(QObject *o, QEvent *e)
{
    if (qobject_cast<KMainWindow*>(o) && KParts::PartActivateEvent::test(e)) {
        KParts::PartActivateEvent* partEvent = static_cast<KParts::PartActivateEvent *>(e);
        KParts::ReadOnlyPart *part = qobject_cast<KParts::ReadOnlyPart *>(partEvent->part());
        //kDebug() << "Embeded part changed to " << part;
        if (part && (part != m_part)) {
            m_part = part;

            // Delete the popup menu so a new one can be created with the
            // appropriate entries the next time it is shown...
            if (m_popupMenu) {
              delete m_popupMenu;
              m_popupMenu = 0;
            }

            // Change the search mode if it is set to FindInThisPage since
            // that feature is currently KHTML specific. It is also completely
            // redundant and unnecessary.
            if (m_searchMode == FindInThisPage && !qobject_cast<KHTMLPart*>(part))
              nextSearchEntry();

            connect(m_part, SIGNAL(completed()), this, SLOT(HTMLDocLoaded()));
            connect(m_part, SIGNAL(started(KIO::Job *)), this, SLOT(HTMLLoadingStarted()));
        }
        // Delay since when destroying tabs part 0 gets activated for a bit, before the proper part
        QTimer::singleShot(0, this, SLOT(updateComboVisibility()));
    }
    else if (o == m_searchCombo->lineEdit() && e->type() == QEvent::KeyPress) {
        QKeyEvent *k = (QKeyEvent *)e;
        if (k->modifiers() & Qt::ControlModifier) {
            if (k->key()==Qt::Key_Down) {
                nextSearchEntry();
                return true;
            }
            if (k->key()==Qt::Key_Up) {
                previousSearchEntry();
                return true;
            }
        }
    }
    return false;
}

void SearchBarPlugin::nextSearchEntry()
{
    if (m_searchMode == FindInThisPage) {
        m_searchMode = UseSearchProvider;
        if (!m_searchEngines.isEmpty()) {
            m_currentEngine = m_searchEngines.first();
        } else {
            m_currentEngine = "google";
        }
    } else {
        int index = m_searchEngines.indexOf(m_currentEngine);
        ++index;
        if (index >= m_searchEngines.count()) {
            m_searchMode = FindInThisPage;
        } else {
            m_currentEngine = m_searchEngines.at(index);
        }
    }
    setIcon();
}

void SearchBarPlugin::previousSearchEntry()
{
    if (m_searchMode == FindInThisPage) {
        m_searchMode = UseSearchProvider;
        if (!m_searchEngines.isEmpty()) {
            m_currentEngine = m_searchEngines.last();
        } else {
            m_currentEngine = "google";
        }
    } else {
        int index = m_searchEngines.indexOf(m_currentEngine);
        if (index == 0) {
            m_searchMode = FindInThisPage;
        } else {
            --index;
            m_currentEngine = m_searchEngines.at(index);
        }
    }
    setIcon();
}

void SearchBarPlugin::startSearch(const QString &search)
{
    if (m_urlEnterLock || search.isEmpty() || !m_part)
        return;
    if (m_searchMode == FindInThisPage) {
        KHTMLPart *part = qobject_cast<KHTMLPart*>(m_part);
        if (part) {
            part->findText(search, 0);
            part->findTextNext();
        }
    } else if (m_searchMode == UseSearchProvider) {
        m_urlEnterLock = true;
        KService::Ptr service;
        KUriFilterData data;
        QStringList list;
        list << "kurisearchfilter" << "kuriikwsfilter";
        if ( !m_currentEngine.isEmpty() )
            service = KService::serviceByDesktopPath(QString("searchproviders/%1.desktop").arg(m_currentEngine));
        if (service) {
            const QString searchProviderPrefix = service->property("Keys").toStringList().first() + m_delimiter;
            data.setData(searchProviderPrefix + search);
        }

        if (!service || !KUriFilter::self()->filterUri(data, list)) {
            data.setData(QLatin1String("google") + m_delimiter + search);
            KUriFilter::self()->filterUri(data, list);
        }

        KParts::BrowserExtension * ext = KParts::BrowserExtension::childObject(m_part);
        if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
            KParts::OpenUrlArguments arguments;
            KParts::BrowserArguments browserArguments;
            browserArguments.setNewTab(true);
            if (ext)
                emit ext->createNewWindow(data.uri(), arguments, browserArguments);
        } else {
            if (ext) {
                emit ext->openUrlRequest(data.uri());
                m_part->widget()->setFocus(); // #152923
            }
        }
    }

    m_searchCombo->addToHistory(search);
    m_searchCombo->setItemIcon(0, m_searchIcon);

    m_urlEnterLock = false;
}

void SearchBarPlugin::setIcon()
{
    if (m_searchMode == FindInThisPage) {
        m_searchIcon = SmallIcon("edit-find");
    } else {
        KService::Ptr service;
        KUriFilterData data;
        QStringList list;
        list << "kurisearchfilter" << "kuriikwsfilter";
        if (!m_currentEngine.isEmpty())
            service = KService::serviceByDesktopPath(QString("searchproviders/%1.desktop").arg(m_currentEngine));
        if (service) {
            const QString searchProviderPrefix = service->property("Keys").toStringList().first() + m_delimiter;
            data.setData(searchProviderPrefix + "some keyword");
        }

        if (service && KUriFilter::self()->filterUri(data, list)) {
            QString iconPath = KStandardDirs::locate("cache", KMimeType::favIconForUrl(data.uri()) + ".png");
            if (iconPath.isEmpty()) {
                m_searchIcon = SmallIcon("unknown");
            }
            else {
                m_searchIcon = QPixmap(iconPath);
            }
        }
        else {
            m_searchIcon = SmallIcon("google");
        }
    }

    // Create a bit wider icon with arrow
    QPixmap arrowmap = QPixmap(m_searchIcon.width()+5,m_searchIcon.height()+5);
    arrowmap.fill(m_searchCombo->lineEdit()->palette().color(m_searchCombo->lineEdit()->backgroundRole()));
    QPainter p(&arrowmap);
    p.drawPixmap(0, 2, m_searchIcon);
    QStyleOption opt;
    opt.state = QStyle::State_None;
    opt.rect = QRect(arrowmap.width()-6, arrowmap.height()-5, 6, 5);
    m_searchCombo->style()->drawPrimitive(QStyle::PE_IndicatorArrowDown, &opt, &p, m_searchCombo);
    p.end();
    m_searchIcon = arrowmap;

    m_searchCombo->setIcon(m_searchIcon);
}

void SearchBarPlugin::showSelectionMenu()
{
    if (!m_popupMenu) {
        KUriFilterData data;
        QStringList list;
        list << "kurisearchfilter" << "kuriikwsfilter";

        m_popupMenu = new QMenu(m_searchCombo);
        m_popupMenu->setObjectName("search selection menu");

        if (qobject_cast<KHTMLPart*>(m_part)) {
            m_popupMenu->addAction(KIcon("edit-find"), i18n("Find in This Page"), this, SLOT(useFindInThisPage()));
            m_popupMenu->addSeparator();
        }

        int i =- 1;
        for (QStringList::ConstIterator it = m_searchEngines.constBegin(); it != m_searchEngines.constEnd(); ++it) {
            i++;
            KService::Ptr service = KService::serviceByDesktopPath(QString("searchproviders/%1.desktop").arg(*it));
            if (!service) {
                continue;
            }
            const QString searchProviderPrefix = service->property("Keys").toStringList().first() + m_delimiter;
            data.setData(searchProviderPrefix + "some keyword");

            if (KUriFilter::self()->filterUri(data, list))
            {
                QIcon icon;
                const QString iconPath = KStandardDirs::locate("cache", KMimeType::favIconForUrl(data.uri()) + ".png");
                if (iconPath.isEmpty()) {
                    icon = KIcon("unknown");
                } else {
                    icon = QPixmap(iconPath);
                }
                QAction* action = m_popupMenu->addAction(icon, service->name());
                action->setData(qVariantFromValue(i));
            }
        }

        m_popupMenu->addSeparator();
        m_popupMenu->addAction(KIcon("preferences-web-browser-shortcuts"), i18n("Select Search Engines..."),
                               this, SLOT(selectSearchEngines()));
        connect(m_popupMenu, SIGNAL(triggered(QAction *)), SLOT(menuActionTriggered(QAction *)));
    }
    else { // the popup menu is already deleted, don't try to delete each action in m_addSearchActions, otherwise kaboom
        Q_FOREACH (KAction *action, m_addSearchActions) {
            m_popupMenu->removeAction(action);
            delete action;
        }
        m_addSearchActions.clear();
    }

    QList<QAction *> actions = m_popupMenu->actions();
    QAction *before = 0;
    if (actions.size() > 1) {
        before = actions[actions.size() - 2];
    }

    Q_FOREACH (const QString &title, m_openSearchDescs.keys()) {
        KAction *addSearchAction = new KAction(m_popupMenu);
        addSearchAction->setText(i18n("Add ") + title + "...");
        m_addSearchActions.append(addSearchAction);
        addSearchAction->setData(QVariant::fromValue(title));
        m_popupMenu->insertAction(before, addSearchAction);
    }

    m_popupMenu->popup(m_searchCombo->mapToGlobal(QPoint(0, m_searchCombo->height() + 1)));
}

void SearchBarPlugin::useFindInThisPage()
{
    m_searchMode = FindInThisPage;
    setIcon();
}

void SearchBarPlugin::menuActionTriggered(QAction *action)
{
    bool ok = false;
    const int id = action->data().toInt(&ok);
    if (ok) {
        m_searchMode = UseSearchProvider;
        m_currentEngine = m_searchEngines.at(id);
        setIcon();
        KService::Ptr service = KService::serviceByDesktopPath(QString("searchproviders/%1.desktop").arg(m_currentEngine));
        m_openSearchManager->setSearchProvider(m_currentEngine);
        m_searchCombo->lineEdit()->selectAll();
        return;
    }

    QString openSearchTitle = action->data().toString();
    if (!openSearchTitle.isEmpty()) {
        KHTMLPart *part = dynamic_cast<KHTMLPart *>(m_part);
        if (part) {
            QString openSearchHref = m_openSearchDescs.value(openSearchTitle);
            KUrl url;
            if (QUrl(openSearchHref).isRelative()) {
                KUrl docUrl = part->url();
                QString host = docUrl.scheme() + "://" + docUrl.host();
                if (docUrl.port() != -1) {
                    host += ":" + QString::number(docUrl.port());
                }
                url = KUrl(docUrl, openSearchHref);
            }
            else {
                url = KUrl(openSearchHref);
            }
            kDebug() << "Adding open search Engine: " << openSearchTitle << " : " << openSearchHref;
            m_openSearchManager->addOpenSearchEngine(url, openSearchTitle);
        }
    }
}

void SearchBarPlugin::selectSearchEngines()
{
    m_process = new KProcess;

    *m_process << "kcmshell4" << "ebrowsing";

    connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(searchEnginesSelected(int,QProcess::ExitStatus)));

    m_process->start();
    if(!m_process->waitForStarted())
    {
        kDebug(1202) << "Couldn't invoke kcmshell4";
        delete m_process;
        m_process = 0;
    }
}

void SearchBarPlugin::searchEnginesSelected(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);
    if(exitStatus == QProcess::NormalExit) {
        KConfigGroup config(KGlobal::config(), "SearchBar");
        config.writeEntry("CurrentEngine", m_currentEngine);
        config.sync();
        configurationChanged();
    }
    m_process->deleteLater();
    m_process = 0;
}

void SearchBarPlugin::configurationChanged()
{
    KConfigGroup config(KSharedConfig::openConfig("kuriikwsfilterrc"), "General");
    m_delimiter = config.readEntry("KeywordDelimiter", ":").at(0);
    const QString engine = config.readEntry("DefaultSearchEngine", "google");

    QStringList favoriteEngines;
    favoriteEngines << "google" << "google_groups" << "google_news" << "webster" << "dmoz" << "wikipedia";
    favoriteEngines = config.readEntry("FavoriteSearchEngines", favoriteEngines);

    delete m_popupMenu;
    m_popupMenu = 0;
    m_searchEngines.clear();
    m_searchEngines << engine;
    for (QStringList::ConstIterator it = favoriteEngines.constBegin(); it != favoriteEngines.constEnd(); ++it)
        if(*it!=engine)
            m_searchEngines << *it;
    m_addSearchActions.clear();

    config = KConfigGroup(KGlobal::config(), "SearchBar");
    m_searchMode = (SearchModes) config.readEntry("Mode", (int) UseSearchProvider);
    m_currentEngine = config.readEntry("CurrentEngine", engine);

    m_suggestionEnabled = config.readEntry("SuggestionEnabled", true);
    m_searchCombo->setSuggestionEnabled(m_suggestionEnabled);

    KService::Ptr service = KService::serviceByDesktopPath(QString("searchproviders/%1.desktop").arg(m_currentEngine));

    m_openSearchManager->setSearchProvider(m_currentEngine);
    setIcon();
}

void SearchBarPlugin::updateComboVisibility()
{
    // NOTE: We hide the search combobox if the embeded kpart is ReadWrite
    // because web browsers by their very nature are ReadOnly kparts...
    m_searchComboAction->setVisible((!qobject_cast<KParts::ReadWritePart *>(m_part) &&
                                     !m_searchComboAction->associatedWidgets().isEmpty()));
    m_openSearchDescs.clear();
}

void SearchBarPlugin::focusSearchbar()
{
    m_searchCombo->setFocus(Qt::ShortcutFocusReason);
}

void SearchBarPlugin::searchTextChanged(const QString &text)
{
    m_searchCombo->clearSuggestions();

    if (m_suggestionEnabled && m_searchMode != FindInThisPage &&
            m_openSearchManager->isSuggestionAvailable() && !text.isEmpty()) {
        if (m_timer->isActive()) {
            m_timer->stop();
        }

        // 400 ms delay before requesting for suggestions, so we don't flood the provider with suggestion request
        m_timer->start(400);
    }
}

void SearchBarPlugin::requestSuggestion() {
    if (!m_searchCombo->lineEdit()->text().isEmpty()) {
        m_openSearchManager->requestSuggestion(m_searchCombo->lineEdit()->text());
    }
}

void SearchBarPlugin::enableSuggestion(bool enable)
{
    m_suggestionEnabled = enable;
}

void SearchBarPlugin::HTMLDocLoaded()
{
    KHTMLPart *khtmlPart = qobject_cast<KHTMLPart *>(m_part);

    if (khtmlPart) {
        DOM::HTMLDocument htmlDoc = khtmlPart->htmlDocument();
        DOM::NodeList headNL = htmlDoc.getElementsByTagName("head");
        if (headNL.length() < 1) {
            return;
        }

        DOM::Node headNode = headNL.item(0);
        if (headNode.nodeType() != 1) {
            return;
        }

        DOM::HTMLElement headEl = (DOM::HTMLElement) headNode;
        DOM::NodeList linkNL = headEl.getElementsByTagName("link");

        for (unsigned int i = 0; i < linkNL.length(); i++) {
            if (linkNL.item(i).nodeType() == 1) {
                DOM::HTMLElement linkEl = (DOM::HTMLElement)linkNL.item(i);
                if (linkEl.getAttribute("rel") == "search" && linkEl.getAttribute("type") == "application/opensearchdescription+xml") {
                    if (headEl.getAttribute("profile") != "http://a9.com/-/spec/opensearch/1.1/") {
                        kWarning() << "Warning: there is no profile attribute or wrong profile attribute in <head>, as specified by open search specification 1.1";
                    }

                    QString title = linkEl.getAttribute("title").string();
                    QString href = linkEl.getAttribute("href").string();
                    m_openSearchDescs.insert(title, href);
                }
            }
        }
    }
}

void SearchBarPlugin::openSearchEngineAdded(const QString &name, const QString &searchUrl, const QString &fileName)
{
    kDebug() << "New Open Search Engine Added: " << name << ", searchUrl " << searchUrl;
    QString path = KGlobal::mainComponent().dirs()->saveLocation("services", "searchproviders/");
    
    KConfig _service(path + fileName + ".desktop", KConfig::SimpleConfig);
    KConfigGroup service(&_service, "Desktop Entry");
    service.writeEntry("Type", "Service");
    service.writeEntry("ServiceTypes", "SearchProvider");
    service.writeEntry("Name", name);
    service.writeEntry("Query", searchUrl);
    service.writeEntry("Keys", fileName);
    // TODO
    service.writeEntry("Charset", "" /* provider->charset() */);

    // we might be overwriting a hidden entry
    service.writeEntry("Hidden", false);

    // Show the add web shortcut widget
    if (!m_addWSWidget) {
        m_addWSWidget = new WebShortcutWidget(m_searchCombo);
        m_addWSWidget->setWindowFlags(Qt::Popup);

        connect(m_addWSWidget, SIGNAL(webShortcutSet(const QString &, const QString &, const QString &)),
                this, SLOT(webShortcutSet(const QString &, const QString &, const QString &)));
    }

    QPoint pos = m_searchCombo->mapToGlobal(QPoint(0, m_searchCombo->height() + 1));
    m_addWSWidget->setGeometry(QRect(pos, m_addWSWidget->size()));
    m_addWSWidget->show(name, fileName);
}

void SearchBarPlugin::webShortcutSet(const QString &name, const QString &webShortcut, const QString &fileName)
{
    kDebug() << "Web shortcut for: " << name << "set to: " << webShortcut;
    QString path = KGlobal::mainComponent().dirs()->saveLocation("services", "searchproviders/");
    KConfig _service(path + fileName + ".desktop", KConfig::SimpleConfig);
    KConfigGroup service(&_service, "Desktop Entry");
    service.writeEntry("Keys", webShortcut);

    // Update filters in running applications...
    QDBusMessage msg = QDBusMessage::createSignal("/", "org.kde.KUriFilterPlugin", "configure");
    QDBusConnection::sessionBus().send(msg);

    // If the providers changed, tell sycoca to rebuild its database...
    KBuildSycocaProgressDialog::rebuildKSycoca(m_searchCombo);
}

void SearchBarPlugin::HTMLLoadingStarted()
{
    // reset the open search availability, so that if there is previously detected engine,
    // it will not be shown
    m_openSearchDescs.clear();
}

void SearchBarPlugin::addSearchSuggestion(const QStringList &suggestions)
{
    m_searchCombo->setSuggestionItems(suggestions);
}

SearchBarCombo::SearchBarCombo(QWidget *parent) :
    KHistoryComboBox(true, parent)
{
    setDuplicatesEnabled(false);
    setFixedWidth(180);
    connect(this, SIGNAL(cleared()), SLOT(historyCleared()));

    Q_ASSERT(useCompletion());

    KConfigGroup config(KGlobal::config(), "SearchBar");
    const QStringList list = config.readEntry( "History list", QStringList() );
    setHistoryItems(list, true);
    Q_ASSERT(currentText().isEmpty()); // KHistoryComboBox calls clearEditText

    m_enableAction = new QAction(i18n("Enable Suggestion"), this);
    m_enableAction->setCheckable(true);
    connect(m_enableAction, SIGNAL(toggled(bool)), this, SIGNAL(suggestionEnabled(bool)));

    connect(this, SIGNAL(aboutToShowContextMenu(QMenu*)), SLOT(addEnableMenuItem(QMenu*)));

    // use our own item delegate to display our fancy stuff :D
    completionBox()->setItemDelegate(new SearchBarItemDelegate(this)); 
}

const QPixmap &SearchBarCombo::icon() const
{
    return m_icon;
}

void SearchBarCombo::setIcon(const QPixmap &icon)
{
    m_icon = icon;
    const QString editText = currentText();
    if (count() == 0) {
        insertItem(0, m_icon, 0);
    } else {
        for(int i = 0; i < count(); i++) {
            setItemIcon(i, m_icon);
        }
    }
    setEditText(editText);
}

void SearchBarCombo::setSuggestionEnabled(bool enable)
{
    m_enableAction->setChecked(enable);
}

int SearchBarCombo::findHistoryItem(const QString &searchText)
{
    for(int i = 0; i < count(); i++) {
        if (itemText(i) == searchText) {
            return i;
        }
    }

    return -1;
}

void SearchBarCombo::mousePressEvent(QMouseEvent *e)
{
    QStyleOptionComplex opt;
    int x0 = QStyle::visualRect(layoutDirection(), style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxEditField, this), rect()).x();

    if (e->x() > x0 + 2 && e->x() < lineEdit()->x()) {
        emit iconClicked();
        e->accept();
    } else {
        KHistoryComboBox::mousePressEvent(e);
    }
}

void SearchBarCombo::historyCleared()
{
    setIcon(m_icon);
}

void SearchBarCombo::setSuggestionItems(const QStringList &suggestions)
{
    if (!m_suggestions.isEmpty()) {
        clearSuggestions();
    }

    m_suggestions = suggestions;
    if (!suggestions.isEmpty()) {
        const int size = completionBox()->count();
        QListWidgetItem *item = new QListWidgetItem(suggestions.at(0));
        item->setData(Qt::UserRole, "suggestion");
        completionBox()->insertItem(size + 1, item);
        const int suggestionCount = suggestions.count();
        for (int i = 1; i < suggestionCount; i++) {
            completionBox()->insertItem(size + 1 + i, suggestions.at(i));
        }
        completionBox()->popup();
    }
}

void SearchBarCombo::clearSuggestions()
{
    int size = completionBox()->count();
    if (!m_suggestions.isEmpty() && size >= m_suggestions.count()) {
        for (int i = size - 1; i >= size - m_suggestions.size(); i--) {
            completionBox()->takeItem(i);
        }
    }
    m_suggestions.clear();
}


void SearchBarCombo::addEnableMenuItem(QMenu *menu)
{
    if (menu) {
        menu->addAction(m_enableAction);
    }
}

SearchBarCombo::~SearchBarCombo()
{
    KConfigGroup config(KGlobal::config(), "SearchBar");
    config.writeEntry( "History list", historyItems() );

    delete m_enableAction;
}

SearchBarItemDelegate::SearchBarItemDelegate(QObject *parent)
    : QItemDelegate(parent)
{
}

void SearchBarItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QString userText = index.data(Qt::UserRole).toString();
    QString text = index.data(Qt::DisplayRole).toString();

    // Get item data
    if (!userText.isEmpty()) {
        // This font is for the "information" text, small size + italic + gray in color
        QFont usrTxtFont = option.font;
        usrTxtFont.setItalic(true);
        usrTxtFont.setPointSize(6);

        QFontMetrics usrTxtFontMetrics(usrTxtFont);
        int width = usrTxtFontMetrics.width(userText);
        QRect rect(option.rect.x(), option.rect.y(), option.rect.width() - width, option.rect.height());
        QFontMetrics textFontMetrics(option.font);
        QString elidedText = textFontMetrics.elidedText(text,
                Qt::ElideRight, option.rect.width() - width - option.decorationSize.width());

        QAbstractItemModel *itemModel = const_cast<QAbstractItemModel *>(index.model());
        itemModel->setData(index, elidedText, Qt::DisplayRole);
        QItemDelegate::paint(painter, option, index);
        itemModel->setData(index, text, Qt::DisplayRole);

        painter->setFont(usrTxtFont);
        painter->setPen(QPen(QColor(Qt::gray)));
        painter->drawText(option.rect, Qt::AlignRight, userText);

        // Draw a separator above this item
        if (index.row() > 0) {
            painter->drawLine(option.rect.x(), option.rect.y(), option.rect.x() + option.rect.width(), option.rect.y());
        }
    }
    else {
        QItemDelegate::paint(painter, option, index);
    }
}

#include "searchbar.moc"
