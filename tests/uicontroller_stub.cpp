#include "stdhdr.h"

#include "uicontroller.h"

// Minimal linker-satisfying stand-in for UIController in the headless test
// binary. GameRules::ReportError() reaches through gpUIController for its
// one UI touch-point; no test exercises that path, so gpUIController stays
// null and ShowError() is a no-op that never touches `this` - safe in
// practice, and far cheaper than linking the real wx GUI layer (frames,
// panes, dialogs) into a binary that must build and run without a display.
UIController * gpUIController = nullptr;

void UIController::ShowError(const char *, int, bool)
{
}
