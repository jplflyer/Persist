#pragma once

#include <string>

#include <showlib/JSONSerializable.h>
#include <showlib/StringVector.h>

class Configuration: public ShowLib::JSONSerializable
{
private:
    static Configuration * s_singleton;

    bool inLoad = false;
    ShowLib::StringVector recents;
    size_t recentsToKeep = 5;

    std::string getConfigurationDirectory();

    // Singleton.
    Configuration();
    void trimRecents();

public:
    static Configuration & singleton();

    void load();
    void save();

    void fromJSON(const JSON &) override;
    JSON & toJSON(JSON &) const override;

    size_t getRecentsToKeep() const { return recentsToKeep; }
    Configuration & setRecentsToKeep(size_t value);

    const ShowLib::StringVector & getRecents() const { return recents; };
    Configuration & pushRecent(const std::string &);
};

