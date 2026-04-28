import os
import subprocess
import sys

USER = "pepitogumball-lang"
REPO = "pepitogumball-lang/ZBOT-MOBILE"
BRANCH = "main"
COMMIT_MSG = (
    "Fix macro playback on Android: qualified parent handleButton "
    "bypasses modify chain + mobile allowed-buttons filter; "
    "remove dead GEODE_IS_MOBILE typo blocks (real macro is GEODE_IS_ANDROID)"
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

    run(["git", "add", "."])

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
