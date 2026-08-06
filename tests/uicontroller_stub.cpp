#include "stdhdr.h"

#include "uicontroller.h"

// Minimal linker-satisfying stand-in for UIController in the headless test
// binary. GameRules::ReportError() reaches through gpUIController for its
// one UI touch-point - far cheaper to stub out than to link the real wx GUI
// layer (frames, panes, dialogs) into a binary that must build and run
// without a display. gpUIController points at a real, default-constructed
// instance (not null) so that call is well-defined even if a test ends up
// exercising it, instead of relying on ShowError() never touching `this`.
UIController::UIController()
{
    memset(m_Frames, 0, sizeof(m_Frames));
    memset(m_Panes , 0, sizeof(m_Panes ));
    memset(m_Fonts , 0, sizeof(m_Fonts ));
    memset(m_FontDescr, 0, sizeof(m_FontDescr));

    m_layout            = 0;
    m_DiscardChanges    = false;
    m_Brightness_Delta  = 0;
}

UIController::~UIController()
{
}

void UIController::ShowError(const char *, int, bool)
{
}

static UIController TheStubUIController;

UIController * gpUIController = &TheStubUIController;
