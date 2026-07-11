#include "zBot.hpp"
#include <Geode/utils/web.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/loader/Loader.hpp>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

using namespace geode::prelude;

//
//
//
// Design rules:
//
// thread
//

namespace {

constexpr const char* kReleasesUrl =
  "https://api.github.com/repos/pepitogumball-lang/ZBOT-MOBILE/releases/latest";

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
  if (Mod::get()->getSettingValue<bool>("disable-update-check")) {
    return;
  }

  std::thread([]{
    web::WebRequest req;
    req.userAgent("ZBOT-MOBILE/" ZBOT_VERSION " (+update-check)");
    req.timeout(std::chrono::seconds(10));
    req.header("Accept", "application/vnd.github+json");

    web::WebResponse res = req.getSync(kReleasesUrl);
    if (!res.ok()) return;

    std::string body = res.string().unwrapOr(std::string());
    std::string tag = extractStringField(body, "tag_name");
    if (tag.empty()) return;

    int latest = parseSemver(tag);
    int current = parseSemver(ZBOT_VERSION);
    if (latest <= 0 || current <= 0) return;
    if (latest <= current) return;

    std::string msg = "ZBOT-MOBILE " + tag + " disponible - mira las releases en GitHub";
    Loader::get()->queueInMainThread([msg]() {
      Notification::create(msg.c_str(),
        NotificationIcon::Info, 5.f)->show();
    });
  }).detach();
}
