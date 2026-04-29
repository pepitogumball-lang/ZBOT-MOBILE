import os
import subprocess
import sys

USER = "pepitogumball-lang"
REPO = "pepitogumball-lang/ZBOT-MOBILE"
BRANCH = "main"
COMMIT_MSG = (
    "Fix CI build + macro playback on Android. "
    "CI: drop phantom ZBOT-MOBILE submodule gitlink that broke "
    "actions/checkout@v4 (no .gitmodules URL -> fatal exit 128). "
    "Playback: full EclipseMenu pattern (clear m_allowedButtons under "
    "GEODE_IS_MOBILE + qualified parent GJBaseGameLayer::handleButton) "
    "at all 5 input injection sites (playback + 4 spammer paths). "
    "Validated against EclipseMenu Bot.cpp:484-486 and "
    "zBot playbackmanager.cpp:24."
)

TOKEN = os.getenv("GITHUB_PERSONAL_ACCESS_TOKEN")
if not TOKEN:
    print("ERROR: GITHUB_PERSONAL_ACCESS_TOKEN no esta definido en Secrets.")
    sys.exit(1)


def run(cmd, check=True):
    print("$", " ".join(cmd))
    return subprocess.run(cmd, check=check)


authed_url = f"https://{USER}:{TOKEN}@github.com/{REPO}.git"
clean_url = f"https://github.com/{REPO}.git"

try:
    run(["git", "remote", "set-url", "origin", authed_url])

    # Drop the phantom `ZBOT-MOBILE` submodule gitlink from the index.
    # An earlier accidental nested clone left a mode-160000 entry
    # without a matching `.gitmodules` URL, which made GitHub Actions
    # `actions/checkout@v4` fail at the "Fetching submodules" step
    # ("fatal: No url found for submodule path 'ZBOT-MOBILE' in
    # .gitmodules") and skip the build entirely. --ignore-unmatch
    # makes this idempotent so re-runs after the cleanup are no-ops.
    subprocess.run(
        ["git", "rm", "--cached", "--ignore-unmatch", "-rf", "ZBOT-MOBILE"],
        check=False,
    )

    run(["git", "add", "-A"])

    status = subprocess.run(
        ["git", "diff", "--cached", "--quiet"],
        check=False,
    )
    if status.returncode != 0:
        run(["git", "commit", "-m", COMMIT_MSG])
    else:
        print("(no hay cambios staged, salto el commit)")

    run(["git", "push", "origin", BRANCH])

    print("\nPush completado a", REPO, "rama", BRANCH)
finally:
    subprocess.run(
        ["git", "remote", "set-url", "origin", clean_url],
        check=False,
    )
    print("Remote limpiado: token removido de .git/config")
