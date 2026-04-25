#!/bin/bash
# Post-merge setup for ZBOT-MOBILE.
#
# This is a Geode SDK C++ mod for Geometry Dash. The actual mod cannot
# be compiled inside Replit (it needs the Geode SDK + Android NDK +
# Geometry Dash itself); production builds happen on GitHub Actions.
#
# Inside Replit we only run a tiny static info-page server (serve.py
# on port 5000) configured by `.replit`. There is nothing to install,
# migrate or rebuild after a merge — Python's stdlib http.server is
# all we use.
#
# Keep this script idempotent and side-effect free so reruns are safe.
set -e

echo "[post-merge] ZBOT-MOBILE: nothing to install or rebuild on Replit."
echo "[post-merge] Production builds run via .github/workflows/build.yml on GitHub Actions."
exit 0
