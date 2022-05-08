#include <iostream>

#include <QSettings>

#include <showlib/CommonUsing.h>

#include "Configuration.h"

using std::cout;
using std::endl;
using std::string;

using namespace ShowLib;

/**
 * Get our singleton. Do this from main so it's thread-safe.
 */
Configuration &
Configuration::singleton() {
    static Configuration * s_singleton = nullptr;

    if (s_singleton == nullptr) {
        s_singleton = new Configuration();
        s_singleton->load();
    }
    return *s_singleton;
}

/**
 * Read our config.
 */
void Configuration::load() {
    QSettings settings("showpage.org", "PersistGUI");

    recentsToKeep = settings.value("Recents/numberToKeep").toInt();
    if (recentsToKeep == 0) {
        recentsToKeep = 5;
    }

    QStringList list = settings.value("Recents/list").toStringList();
    for (const QString &str: list) {
        string fname = str.toStdString();
        recents.add(fname);
    }
}

/**
 * Write our config.
 */
void Configuration::save() {
    QSettings settings("showpage.org", "PersistGUI");

    settings.setValue("Recents/numberToKeep", static_cast<int>(recentsToKeep));

    if ( !recents.empty() ) {
        QStringList list;
        for (const StringVector::Pointer &ptr: recents) {
            list.append(QString::fromStdString(*ptr));
        }

        settings.setValue("Recents/list", list);
    }
}

/**
 * How many recents do we store? Default is 5.
 */
Configuration & Configuration::setRecentsToKeep(size_t value) {
    if (value > 0) {
        recentsToKeep = value;
        trimRecents();
        save();
    }
    return *this;
}

/**
 * Add this file as the most recent.
 */
Configuration & Configuration::pushRecent(const std::string &fileName) {
    recents.remove(fileName);
    recents.addFront(fileName);
    trimRecents();
    save();

    return *this;
}

void Configuration::trimRecents() {
    while (recents.size() > recentsToKeep) {
        recents.pop_back();
    }
}
