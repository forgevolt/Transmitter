#include "FPSToggle.h"
#include <Preferences.h>

// ---- FPSToggle -------------------------------------------------------------------------

const String cMenuNameOn  = "Show FPS";
const String cMenuNameOff = "Hide FPS";

static const char* cPreferencesNS = "FPS";
static const char* cKey           = "showFPS";

bool showFPS; // global

// ----------------------------------------------------------------------------------------
FPSToggle::FPSToggle(UserInterface& ui, Menu& menu)
: MenuItem(ui, menu, "")
{
  // Read FPS configuration from preferences
  Preferences p;
  p.begin(cPreferencesNS);
  showFPS = p.getBool(cKey, true);
  p.end();

  myMenuName = showFPS ? cMenuNameOff : cMenuNameOn;
}

// ----------------------------------------------------------------------------------------
void FPSToggle::draw(bool forceRedraw)
{
  MenuItem::draw(needsRedraw() || forceRedraw);
  clearRedrawFlags();
}

// ----------------------------------------------------------------------------------------
void FPSToggle::start()
{
  showFPS = !showFPS;

  // Set FPS configuration
  Preferences p;
  p.begin(cPreferencesNS);
  p.putBool(cKey, showFPS);
  p.end();

  myMenuName = showFPS ? cMenuNameOff : cMenuNameOn;
  markRedraw();
}
