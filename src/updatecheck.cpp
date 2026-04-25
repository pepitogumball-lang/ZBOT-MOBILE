#include "zBot.hpp"
#include <Geode/utils/web.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/loader/Loader.hpp>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

using namespace geode::prelude;

// ---------------------------------------------------------------------------
// Self-contained "is there a newer release?" check.
//
// On mod load:
//   1. Hit GitHub's Releases API for the upstream repo (in a detached
//      worker thread so we never block the game-load critical path).
//   2. Parse the `tag_name` field out of the JSON response.
//   3. Compare it to ZBOT_VERSION.
//   4. If the remote tag is newer, hop back to the main thread and
//      pop a single Notification telling the user where to grab it.
//
// Design rules:
//   - Single shot per launch — no polling, no timers.
//   - Opt-out via the `disable-update-check` mod setting (Geode wires
//     this into the standard mod settings UI automatically when
//     declared in mod.json).
//   - Every failure path (no network, rate-limited, malformed JSON,
//     unparseable tag) silently no-ops. The user must never get a
//     scary error from a background HTTP call.
//
// Geode 5.x web API note: the older `EventListener<web::WebTask>`
// pattern does not exist in Geode 5.6.1 — the namespace exposes
// `WebRequest`, `WebFuture`, and synchronous `getSync()`. We use the
// synchronous variant on a detached `std::thread`, then bounce the
// notification back to the main thread via `Loader::queueInMainThread`
// because UI calls (Notification::create) must run on the cocos main
// thread.
// ---------------------------------------------------------------------------

namespace {

constexpr const char* kReleasesUrl =
    "https://api.github.com/repos/pepitogumball-lang/ZBOT-MOBILE/releases/latest";

// "v1.5.4" -> 10504. Returns -1 if the input is not a recognisable
// vX.Y.Z (or X.Y.Z) triple.
int parseSemver(const std::string& raw) {
    std::string s = raw;
    if (!s.empty() && (s[0] == 'v' || s[0] == 'V')) {
        s.erase(0, 1);
    }
    int major = 0, minor = 0, patch = 0;
    if (std::sscanf(s.c_str(), "%d.%d.%d", &major, &minor, &patch) != 3) {
        return -1;
    }
    if (major < 0 || minor < 0 || patch < 0) return -1;
    return major * 10000 + minor * 100 + patch;
}

// Pull the value of a top-level string field out of a small JSON
// blob without dragging in a full parser. Returns "" on miss.
// Good enough for `"tag_name": "v1.5.5"` style fields.
std::string extractStringField(const std::string& body, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    auto k = body.find(needle);
    if (k == std::string::npos) return "";
    auto colon = body.find(':', k + needle.size());
    if (colon == std::string::npos) return "";
    auto open = body.find('"', colon + 1);
    if (open == std::string::npos) return "";
    auto close = body.find('"', open + 1);
    if (close == std::string::npos) return "";
    return body.substr(open + 1, close - open - 1);
}

} // namespace

$execute {
    // Honour the user opt-out. The setting is declared in mod.json,
    // so Geode shows it in the standard mod settings panel.
    if (Mod::get()->getSettingValue<bool>("disable-update-check")) {
        return;
    }

    // Detached worker so we never block the game-load critical path.
    // No captures: everything we need is in static storage already
    // (kReleasesUrl, ZBOT_VERSION, the helper functions above).
    std::thread([]{
        web::WebRequest req;
        req.userAgent("ZBOT-MOBILE/" ZBOT_VERSION " (+update-check)");
        req.timeout(std::chrono::seconds(10));
        req.header("Accept", "application/vnd.github+json");

        // Synchronous in this worker thread so we don't have to wire
        // up a polling loop on cocos's scheduler. Returns a default-
        // constructed WebResponse on hard failure (DNS, no network,
        // etc.) — `ok()` will be false, and we silently no-op.
        web::WebResponse res = req.getSync(kReleasesUrl);
        if (!res.ok()) return;

        std::string body = res.string().unwrapOr(std::string());
        std::string tag  = extractStringField(body, "tag_name");
        if (tag.empty()) return;

        int latest  = parseSemver(tag);
        int current = parseSemver(ZBOT_VERSION);
        if (latest <= 0 || current <= 0) return;
        if (latest <= current) return;

        // Newer build is available. Single, dismissable notification.
        // We intentionally don't open the browser or auto-download —
        // installing a Geode mod is a manual step and we don't want
        // to surprise the user.
        std::string msg = "ZBOT-MOBILE " + tag + " available - check the GitHub releases page";
        Loader::get()->queueInMainThread([msg]() {
            Notification::create(msg.c_str(),
                NotificationIcon::Info, 5.f)->show();
        });
    }).detach();
}
