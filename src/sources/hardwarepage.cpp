#include "hardwarepage.h"
#include "defines.h"
#include "configuration.h"

#include <wx/statbox.h>
#include <wx/combobox.h>
#include <wx/arrstr.h>

HardwarePage::HardwarePage(wxWindow* window) : wxStaticBoxSizer(wxHORIZONTAL, window, "Hardware")
{
  wxStaticBoxSizer *bladeDetect = new wxStaticBoxSizer(wxVERTICAL, GetStaticBox(), "Blade Detect");
  settings.bladeDetect = new wxCheckBox(bladeDetect->GetStaticBox(), wxID_ANY, "Blade Detect");
  settings.bladeDetectPinLabel = new wxStaticText(bladeDetect->GetStaticBox(), wxID_ANY, "Pin");
  settings.bladeDetectPin = new wxComboBox(bladeDetect->GetStaticBox(), wxID_ANY, "", wxDefaultPosition, wxDefaultSize, {"bladePin", "blade2Pin", "blade3Pin", "blade4Pin"});
  bladeDetect->Add(settings.bladeDetect, MENUITEMFLAGS);
  bladeDetect->Add(settings.bladeDetectPinLabel, wxSizerFlags(0).Border(wxTOP | wxLEFT | wxRIGHT, 10));
  bladeDetect->Add(settings.bladeDetectPin, wxSizerFlags(0).Border(wxBOTTOM | wxLEFT | wxRIGHT, 10));

  Add(bladeDetect, MENUITEMFLAGS);
}

void HardwarePage::update() {
  settings.bladeDetect->SetValue(Configuration::features.bladeDetect);
  settings.bladeDetectPin->SetValue(Configuration::features.bladeDetectPin);

  settings.bladeDetect->Show();
  settings.bladeDetectPin->Show();
  settings.bladeDetectPinLabel->Show();
}

decltype(HardwarePage::settings) HardwarePage::settings;
