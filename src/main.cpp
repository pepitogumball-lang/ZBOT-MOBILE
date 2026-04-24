#include "zBot.hpp"

// Source files for record/playback, speedhack and audio speedhack are
// split into their own translation units (recordmanager.cpp,
// playbackmanager.cpp, speedhack.cpp, speedhackaudio.cpp). This file
// is intentionally kept empty so the linker still picks it up via
// CMakeLists.txt without duplicating any hooks.
