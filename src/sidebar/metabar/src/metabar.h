#ifndef METABAR_H
#define METABAR_H

#include <konqsidebarplugin.h>
#include <qstring.h>
#include <kconfig.h>


class Metabar : public KonqSidebarPlugin
{
    Q_OBJECT

  public:
    Metabar(const KComponentData &inst,QObject *parent,QWidget *widgetParent, QString &desktopName, const char* name=0);
    ~Metabar();

    virtual QWidget *getWidget(){ return widget; }
    virtual void *provides(const QString &) { return 0; }

  protected:
    MetabarWidget *widget;

    void handleURL(const KUrl &url);
    void handlePreview(const KFileItemList &items);

};

#endif
