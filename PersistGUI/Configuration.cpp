#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

#include <showlib/Ranges.h>

#include "Configuration.h"

using std::cout;
using std::endl;
using std::string;

using namespace ShowLib;

Configuration * Configuration::s_singleton = nullptr;

/**
 * Get our singleton. Do this from main so it's thread-safe.
 */
Configuration &
Configuration::singleton() {
    if (s_singleton == nullptr) {
        s_singleton = new Configuration();
        s_singleton->load();
    }
    return *s_singleton;
}

/**
 * Private constructor.
 */
Configuration::Configuration()
{
}

/**
 * Return our path to our configuration directory.
 */
string
Configuration::getConfigurationDirectory() {
    struct passwd* pwd = getpwuid(getuid());
    string retVal = string(pwd->pw_dir) + "/Library/Application Support/Persist";

    return retVal;
}

/**
 * Read our config.
 */
void Configuration::load() {
    string dirName = getConfigurationDirectory();
    string fileName = dirName + "/config.json";

    inLoad = true;

    if (std::filesystem::exists(fileName) && std::filesystem::is_regular_file(fileName)) {
        std::ifstream configFile(fileName);
        std::stringstream buffer;
        buffer << configFile.rdbuf();
        string contents = buffer.str();

        if (contents.size() > 0u && contents.at(0) == '{') {
            JSON json = JSON::parse(buffer.str());
            fromJSON(json);
        }
    }

    inLoad = false;
}

/**
 * Write our config.
 */
void Configuration::save() {
    try {
        cout << "save file..." << endl;
        string dirName = getConfigurationDirectory();
        string fileName = dirName + "/config.json";
        std::filesystem::path path(dirName);

        if (!std::filesystem::exists(dirName)) {
            std::filesystem::create_directories(path);
        }

        JSON json = JSON::object();
        toJSON(json);

        std::ofstream output(fileName);
        output << json.dump(2) << endl;
    }
    catch (std::exception &e) {
        cout << "Exception: " << e.what() << endl;
    }

    cout << "save is complete" << endl;
}

/**
 * Parse from this JSON.
 */
void Configuration::fromJSON(const JSON &json) {
    recentsToKeep = intValue(json, "recentsToKeep");
    recents.fromJSON(jsonArray(json, "recents"));
}

/**
 * Write to this JSON.
 */
JSON & Configuration::toJSON(JSON &json) const {
    cout << "Convert to JSON." << endl;
    JSON recentsJSON { JSON::array() };

    recents.toJSON(recentsJSON);

    json["recentsToKeep"] = recentsToKeep;
    json["recents"] = recentsJSON;

    return json;
}

/**
 * How many recents do we store? Default is 5.
 */
Configuration & Configuration::setRecentsToKeep(size_t value) {
    if (value > 0) {
        recentsToKeep = value;
        trimRecents();
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

    return *this;
}

void Configuration::trimRecents() {
    while (recents.size() > recentsToKeep) {
        recents.pop_back();
    }
}
