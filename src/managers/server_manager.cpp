#include "server_manager.hpp"
#include <util/time.hpp>
#include <util/rng.hpp>
#include <util/net.hpp>

GLOBED_SINGLETON_DEF(GlobedServerManager)

GlobedServerManager::GlobedServerManager() {
    auto storedActive = geode::Mod::get()->getSavedValue<std::string>("active-central-server");
    if (storedActive.empty()) {
        // storedActive = "https://globed.dankmeme.dev";
        // TODO ^^^
        storedActive = "http://127.0.0.1:41000";
    }

    _data.write()->central = storedActive;
}

void GlobedServerManager::setCentral(std::string address) {
    if (!address.empty() && address.ends_with('/')) {
        address.pop_back();
    }
    
    geode::Mod::get()->setSavedValue("active-central-server", address);

    auto data = _data.write();
    data->central = address;
    data->servers.clear();
}

std::string GlobedServerManager::getCentral() {
    return _data.read()->central;
}

void GlobedServerManager::addGameServer(const std::string& serverId, const std::string& name, const std::string& address, const std::string& region) {
    auto addr = util::net::splitAddress(address);
    auto data = _data.write();
    data->servers[serverId] = GameServerInfo {
        .name = name,
        .region = region,
        .address = {.ip = addr.first, .port = addr.second},
        .ping = -1,
        .playerCount = 0,
    };
}

std::string GlobedServerManager::getActiveGameServer() {
    return _data.read()->game;
}

void GlobedServerManager::clearGameServers() {
    auto data = _data.write();
    data->servers.clear();
    data->activePingId = 0;
    data->game = "";
}

uint32_t GlobedServerManager::pingStart(const std::string& serverId) {
    uint32_t pingId = util::rng::Random::get().generate<uint32_t>();

    auto data = _data.write();
    auto& gsi = data->servers.at(serverId);

    if (gsi.pendingPings.size() > 50) {
        geode::log::warn("over 50 pending pings for the game server {}, clearing", serverId);
        gsi.pendingPings.clear();
    }

    gsi.pendingPings[pingId] = util::time::now();

    return pingId;
}

void GlobedServerManager::pingStartActive() {
    auto data = _data.write();
    if (!data->game.empty()) {
        data->activePingId = pingStart(data->game);
    }
}

void GlobedServerManager::pingFinish(uint32_t pingId, uint32_t playerCount) {
    auto data = _data.write();
    for (auto& server : util::collections::mapValues(data->servers)) {
        if (server.pendingPings.contains(pingId)) {
            auto start = server.pendingPings.at(pingId);
            auto timeTook = util::time::now() - start;
            server.ping = chrono::duration_cast<chrono::milliseconds>(timeTook).count();
            server.playerCount = playerCount;
            server.pingHistory.push(timeTook);
            server.pendingPings.erase(pingId);
            return;
        }
    }

    geode::log::warn("Ping ID doesn't exist in any known server: {}", pingId);
}

void GlobedServerManager::pingFinishActive(uint32_t playerCount) {
    pingFinish(_data.read()->activePingId, playerCount);
}

GameServerView GlobedServerManager::getGameServer(const std::string& serverId) {
    auto data = _data.read();
    auto& gsi = data->servers.at(serverId);
    return GameServerView {
        .ping = gsi.ping,
        .playerCount = gsi.playerCount
    };
}

std::vector<chrono::milliseconds> GlobedServerManager::getPingHistory(const std::string& serverId) {
    auto data = _data.read();
    auto& gsi = data->servers.at(serverId);
    return gsi.pingHistory.extract();
}

std::unordered_map<std::string, GameServerView> GlobedServerManager::extractGameServers() {
    std::unordered_map<std::string, GameServerView> out;

    auto data = _data.read();
    for (const auto& [serverId, gsi] : data->servers) {
        out[serverId] = GameServerView {
            .name = gsi.name,
            .address = gsi.address,
            .ping = gsi.ping,
            .playerCount = gsi.playerCount,
        };
    }

    return out;
}