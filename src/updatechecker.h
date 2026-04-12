#pragma once

#include <QWidget>

class UpdateChecker {
public:
    // silent=true  → dialog only shown when a newer version is found (startup check)
    // silent=false → always shows result (manual "Check for Updates")
    static void check(QWidget* parent, bool silent = true);
};
