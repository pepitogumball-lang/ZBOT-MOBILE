#include "zBot.hpp"
#include <Geode/utils/web.hpp>
#include <Geode/loader/Mod.hpp>
#include <chrono>
#include <cstdio>
#include <string>

using namespace geode::prelude;

// ---------------------------------------------------------------------------
// Self-contained "is there a newer release?" check.
//
// On mod load:
//   1. Hit GitHub's Releases API for the upstream repo.
//   2. Parse the `tag_name` field out of the JSON response.
//   3. Compare it to ZBOT_VERSION.
//   4. If the remote tag is newer, pop a single Notification telling
//      the user where to grab it.
//
// Design rules:
//   - Single shot per launch — no polling, no timers.
//   - Opt-out via the `disable-update-check` mod setting (Geode wires
//     this into the standard mod settings UI automatically when
//     declared in mod.json).
//   - Every failure path (no network, rate-limited, malformed JSON,
//     unparseable tag) silently no-ops. The user must never get a
//     scary error from a background HTTP call.
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

// Listener has to outlive the WebTask it's bound to, so park it at
// namespace scope.
EventListener<web::WebTask> g_updateListener;

} // namespace

$execute {
    // Honour the user opt-out. The setting is declared in mod.json,
    // so Geode shows it in the standard mod settings panel.
    if (Mod::get()->getSettingValue<bool>("disable-update-check")) {
        return;
    }

    web::WebRequest req;
    req.userAgent("ZBOT-MOBILE/" ZBOT_VERSION " (+update-check)");
    req.timeout(std::chrono::seconds(10));
    req.header("Accept", "application/vnd.github+json");

    g_updateListener.bind([](web::WebTask::Event* e) {
        auto* res = e->getValue();
        if (!res) return;
        if (!res->ok()) return;

        auto body = res->string().unwrapOr("");
        auto tag  = extractStringField(body, "tag_name");
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
        Notification::create(msg.c_str(),
            NotificationIcon::Info, 5.f)->show();
    });
    g_updateListener.setFilter(req.get(kReleasesUrl));
}
