#pragma once

#include <string>

#include <showlib/JSONSerializable.h>
#include <showlib/StringVector.h>

class Configuration
{
public:
    static Configuration & singleton();

    void load();
    void save();

    size_t getRecentsToKeep() const { return recentsToKeep; }
    Configuration & setRecentsToKeep(size_t value);

    const ShowLib::StringVector & getRecents() const { return recents; };
    Configuration & pushRecent(const std::string &);

private:
    ShowLib::StringVector recents;
    size_t recentsToKeep = 5;

    // Singleton.
    Configuration() {}
    void trimRecents();

};

