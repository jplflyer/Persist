#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Configuration.h"

using std::cout;
using std::endl;
using std::string;

using namespace ShowLib;

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

        if (contents.at(0) == '{') {
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
    string dirName = getConfigurationDirectory();
    string fileName = dirName + "/config.json";
    std::filesystem::path path(dirName);

    if (!std::filesystem::exists(dirName)) {
        std::filesystem::create_directories(path);
    }
    std::ofstream output(fileName);
    JSON json;
    toJSON(json);
    output << json.dump(2) << endl;
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
JSON Configuration::toJSON(JSON &json) const {
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
    recents.insert(recents.begin(), std::make_shared<string>(fileName));
    trimRecents();
}

void Configuration::trimRecents() {
    while (recents.size() > recentsToKeep) {
        recents.erase(recents.begin());
    }
}
