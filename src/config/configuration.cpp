// ProffieConfig, All-In-One GUI Proffieboard Configuration Utility
// Copyright (C) 2023 Ryan Ogurek

#include "config/configuration.h"

#include "core/defines.h"
#include "core/mainwindow.h"
#include "pages/generalpage.h"
#include "pages/presetspage.h"
#include "pages/proppage.h"
#include "pages/bladespage.h"
#include "pages/bladeidpage.h"

#include <cstring>
#include <exception>
#include <wx/filedlg.h>

Configuration* Configuration::instance;
Configuration::Configuration() {
}

bool Configuration::outputConfig(const std::string& filePath) {
  PresetsPage::instance->update();
  BladesPage::instance->update();
  BladeIDPage::instance->update();

  if (!runPrechecks()) return false;

  std::ofstream configOutput(filePath);
  if (!configOutput.is_open()) {
    std::cerr << "Could not open config file for output." << std::endl;
    return false;
  }

  configOutput <<
      "/*" << std::endl <<
      "This configuration file was generated by ProffieConfig " VERSION ", created by Ryryog25." << std::endl <<
      "The tool can be found here: https://github.com/ryryog25/ProffieConfig" << std::endl <<
      "ProffieConfig is an All-In-One Proffieboard GUI Utility for managing configurations and applying them to your proffieboard" << std::endl <<
      "*/" << std::endl << std::endl;

  outputConfigTop(configOutput);
  outputConfigProp(configOutput);
  outputConfigPresets(configOutput);
  outputConfigButtons(configOutput);

  configOutput.close();
  return true;
}
bool Configuration::outputConfig() { return Configuration::outputConfig(CONFIG_PATH); }
bool Configuration::exportConfig() {
  wxFileDialog configLocation(MainWindow::instance, "Save ProffieOS Config File", "", "ProffieConfig_autogen.h", "C Header Files (*.h)|*.h", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

  if (configLocation.ShowModal() == wxID_CANCEL) return false; // User Closed

  return Configuration::outputConfig(configLocation.GetPath().ToStdString());
}

void Configuration::outputConfigTop(std::ofstream& configOutput) {
  configOutput << "#ifdef CONFIG_TOP" << std::endl;
  outputConfigTopGeneral(configOutput);
  outputConfigTopPropSpecific(configOutput);
  configOutput << "#endif" << std::endl << std::endl;

}
void Configuration::outputConfigTopGeneral(std::ofstream& configOutput) {
  if (GeneralPage::instance->massStorage->GetValue()) configOutput << "//PROFFIECONFIG ENABLE_MASS_STORAGE" << std::endl;
  if (GeneralPage::instance->webUSB->GetValue()) configOutput << "//PROFFIECONFIG ENABLE_WEBUSB" << std::endl;
  switch (parseBoardType(GeneralPage::instance->board->GetValue().ToStdString())) {
    case Configuration::ProffieBoard::V1:
      configOutput << "#include \"proffieboard_v1_config.h\"" << std::endl;
      break;
    case Configuration::ProffieBoard::V2:
      configOutput << "#include \"proffieboard_v2_config.h\"" << std::endl;
      break;
    case Configuration::ProffieBoard::V3:
      configOutput << "#include \"proffieboard_v3_config.h\"" << std::endl;
  }
  for (const auto& [ name, define ] : Settings::instance->generalDefines) {
    if (define->shouldOutput()) configOutput << "#define " << define->getOutput() << std::endl;
  }
}
void Configuration::outputConfigTopPropSpecific(std::ofstream& configOutput) {

}

void Configuration::outputConfigProp(std::ofstream& configOutput)
{
  configOutput << "#ifdef CONFIG_PROP" << std::endl;
  switch (Configuration::parsePropSel(PropPage::instance->prop->GetValue().ToStdString())) {
    case Configuration::SaberProp::SA22C:
      configOutput << "#include \"../props/saber_sa22c_buttons.h\"" << std::endl;
      break;
    case Configuration::SaberProp::FETT263:
      configOutput << "#include \"../props/saber_fett263_buttons.h\"" << std::endl;
      break;
    case Configuration::SaberProp::BC:
      configOutput << "#include \"../props/saber_BC_buttons.h\"" << std::endl;
      break;
    case Configuration::SaberProp::SHTOK:
      configOutput << "#include \"../props/saber_shtok_buttons.h\"" << std::endl;
      break;
    case Configuration::SaberProp::CAIWYN:
      configOutput << "#include \"../props/saber_caiwyn_buttons.h\"" << std::endl;
      break;
    default: break;
  }
  configOutput << "#endif" << std:: endl << std::endl; // CONFIG_PROP
}
void Configuration::outputConfigPresets(std::ofstream& configOutput) {
  configOutput << "#ifdef CONFIG_PRESETS" << std::endl;
  outputConfigPresetsStyles(configOutput);
  outputConfigPresetsBlades(configOutput);
  configOutput << "#endif" << std::endl << std::endl;
}
void Configuration::outputConfigPresetsStyles(std::ofstream& configOutput) {
  for (const BladeIDPage::BladeArray& bladeArray : BladeIDPage::instance->bladeArrays) {
    configOutput << "Preset " << bladeArray.name << "[] = {" << std::endl;
    for (const PresetsPage::PresetConfig& preset : bladeArray.presets) {
      configOutput << "\t{ \"" << preset.dirs << "\", \"" << preset.track << "\"," << std::endl;
      if (preset.styles.size() > 0) for (const wxString& style : preset.styles) configOutput << "\t\t" << style << "," << std::endl;
      else configOutput << "\t\t," << std::endl;
      configOutput << "\t\t\"" << preset.name << "\"}";
      // If not the last one, add comma
      if (&bladeArray.presets[bladeArray.presets.size() - 1] != &preset) configOutput << ",";
      configOutput << std::endl;
    }
    configOutput << "};" << std::endl;
  }
}
void Configuration::outputConfigPresetsBlades(std::ofstream& configOutput) {
  configOutput << "BladeConfig blades[] = {" << std::endl;
  for (const BladeIDPage::BladeArray& bladeArray : BladeIDPage::instance->bladeArrays) {
    configOutput << "\t{ " << (bladeArray.name == "no_blade" ? "NO_BLADE" : std::to_string(bladeArray.value)) << "," << std::endl;
    for (const BladesPage::BladeConfig& blade : bladeArray.blades) {
      if (blade.type == BD_PIXELRGB || blade.type == BD_PIXELRGBW) {
        bool firstSub = true;
        if (blade.isSubBlade) for (BladesPage::BladeConfig::subBladeInfo subBlade : blade.subBlades) {
            if (blade.subBladeWithStride) configOutput << "\t\tSubBladeWithStride( ";
            else /* if not with stride*/ configOutput << "\t\tSubBlade( ";
            configOutput << subBlade.startPixel << ", " << subBlade.endPixel << ", ";
            if (firstSub) {
              genWS281X(configOutput, blade);
              configOutput << ")," << std::endl;
            } else {
              configOutput << "NULL)," << std::endl;
            }
            firstSub = false;
          } else {
          configOutput << "\t\t";
          genWS281X(configOutput, blade);
          configOutput << "," << std::endl;
        }
      } else if (blade.type == BD_TRISTAR || blade.type == BD_QUADSTAR) {
        bool powerPins[4]{true, true, true, true};
        configOutput << "\t\tSimpleBladePtr<";
        if (blade.Star1 != BD_NORESISTANCE) configOutput << "CreeXPE2" << blade.Star1 << "Template<" << blade.Star1Resistance << ">, ";
        else {
          configOutput << "NoLED, ";
          powerPins[0] = false;
        }
        if (blade.Star2 != BD_NORESISTANCE) configOutput << "CreeXPE2" << blade.Star2 << "Template<" << blade.Star2Resistance << ">, ";
        else {
          configOutput << "NoLED, ";
          powerPins[1] = false;
        }
        if (blade.Star3 != BD_NORESISTANCE) configOutput << "CreeXPE2" << blade.Star3 << "Template<" << blade.Star3Resistance << ">, ";
        else {
          configOutput << "NoLED, ";
          powerPins[2] = false;
        }
        if (blade.Star4 != BD_NORESISTANCE && blade.type == BD_QUADSTAR) configOutput << "CreeXPE2" << blade.Star4 << "Template<" << blade.Star4Resistance << ">, ";
        else {
          configOutput << "NoLED, ";
          powerPins[3] = false;
        }

        BladesPage::BladeConfig tempBlade = blade;
        for (int32_t powerPin = 0; powerPin < 4; powerPin++) {
          if (powerPins[powerPin]) {
            if (tempBlade.usePowerPin1) {
              configOutput << "bladePowerPin1";
              tempBlade.usePowerPin1 = false;
            } else if (tempBlade.usePowerPin2) {
              configOutput << "bladePowerPin2";
              tempBlade.usePowerPin2 = false;
            } else if (tempBlade.usePowerPin3) {
              configOutput << "bladePowerPin3";
              tempBlade.usePowerPin3 = false;
            } else if (tempBlade.usePowerPin4) {
              configOutput << "bladePowerPin4";
              tempBlade.usePowerPin4 = false;
            } else if (tempBlade.usePowerPin5) {
              configOutput << "bladePowerPin5";
              tempBlade.usePowerPin5 = false;
            } else if (tempBlade.usePowerPin6) {
              configOutput << "bladePowerPin6";
              tempBlade.usePowerPin6 = false;
            } else configOutput << "-1";
          } else {
            configOutput << "-1";
          }

          if (powerPin != 3) configOutput << ", ";
        }
        configOutput << ">()," << std::endl;
      } else if (blade.type == BD_SINGLELED) {
        configOutput << "\t\tSimpleBladePtr<CreeXPE2WhiteTemplate<550>, NoLED, NoLED, NoLED, ";
        if (blade.usePowerPin1) {
          configOutput << "bladePowerPin1, ";
        } else if (blade.usePowerPin2) {
          configOutput << "bladePowerPin2, ";
        } else if (blade.usePowerPin3) {
          configOutput << "bladePowerPin3, ";
        } else if (blade.usePowerPin4) {
          configOutput << "bladePowerPin4, ";
        } else if (blade.usePowerPin5) {
          configOutput << "bladePowerPin5, ";
        } else if (blade.usePowerPin6) {
          configOutput << "bladePowerPin6, ";
        } else configOutput << "-1, ";
        configOutput << "-1, -1, -1>()," << std::endl;
      }
    }
    configOutput << "\t\tCONFIGARRAY(" << bladeArray.name << "), \"" << bladeArray.name << "\"" << std::endl << "\t}";
    if (&bladeArray != &BladeIDPage::instance->bladeArrays[BladeIDPage::instance->bladeArrays.size() - 1]) configOutput << ",";
    configOutput << std::endl;
  }
  configOutput << "};" << std::endl;
}
void Configuration::genWS281X(std::ofstream& configOutput, const BladesPage::BladeConfig& blade) {
  wxString bladePin = blade.dataPin;
  wxString bladeColor = blade.type == BD_PIXELRGB || blade.useRGBWithWhite ? blade.colorType : [=](wxString colorType) -> wxString { colorType.replace(colorType.find("W"), 1, "w"); return colorType; }(blade.colorType);

  configOutput << "WS281XBladePtr<" << blade.numPixels << ", " << bladePin << ", Color8::" << bladeColor << ", PowerPINS<";
  if (blade.usePowerPin1) {
    configOutput << "bladePowerPin1";
    if (blade.usePowerPin2 || blade.usePowerPin3 || blade.usePowerPin4 || blade.usePowerPin5 || blade.usePowerPin6) configOutput << ", ";
  }
  if (blade.usePowerPin2) {
    configOutput << "bladePowerPin2";
    if (blade.usePowerPin3 || blade.usePowerPin4 || blade.usePowerPin5 || blade.usePowerPin6) configOutput << ", ";
  }
  if (blade.usePowerPin3) {
    configOutput << "bladePowerPin3";
    if (blade.usePowerPin4 || blade.usePowerPin5 || blade.usePowerPin6) configOutput << ", ";
  }
  if (blade.usePowerPin4) {
    configOutput << "bladePowerPin4";
    if (blade.usePowerPin5 || blade.usePowerPin6) configOutput << ", ";
  }
  if (blade.usePowerPin5) {
    configOutput << "bladePowerPin5";
    if (blade.usePowerPin6) configOutput << ", ";
  }
  if (blade.usePowerPin6) {
    configOutput << "bladePowerPin6";
  }
  configOutput << ">>()";
};
void Configuration::outputConfigButtons(std::ofstream& configOutput) {
  configOutput << "#ifdef CONFIG_BUTTONS" << std::endl;
  configOutput << "Button PowerButton(BUTTON_POWER, powerButtonPin, \"pow\");" << std::endl;
  if (GeneralPage::instance->buttons->num->GetValue() >= 2) configOutput << "Button AuxButton(BUTTON_AUX, auxPin, \"aux\");" << std::endl;
  if (GeneralPage::instance->buttons->num->GetValue() == 3) configOutput << "Button Aux2Button(BUTTON_AUX2, aux2Pin, \"aux\");" << std::endl; // figure out aux2 syntax
  configOutput << "#endif" << std::endl << std::endl; // CONFIG_BUTTONS
}

void Configuration::readConfig(const std::string& filePath) {
  std::ifstream file(filePath);
  if (!file.is_open()) return;

  try {
    std::string section;
    BladeIDPage::instance->bladeArrays.clear();
    while (!file.eof()) {
      file >> section;
      if (section == "//") {
        getline(file, section);
        continue;
      }
      if (std::strstr(section.data(), "/*")) {
        while (!file.eof()) {
          if (std::strstr(section.data(), "*/")) break;
          file >> section;
        }
        continue;
      }
      if (section == "#ifdef") {
        file >> section;
        if (section == "CONFIG_TOP") Configuration::readConfigTop(file);
        if (section == "CONFIG_PROP") Configuration::readConfigProp(file);
        if (section == "CONFIG_PRESETS") Configuration::readConfigPresets(file);
        if (section == "CONFIG_STYLES") Configuration::readConfigStyles(file);
      }
    }
  } catch (std::exception& e) {
    std::string errorMessage = "There was an error parsing config, please ensure it is valid:\n\n";
    errorMessage += e.what();

    // Restore App State
    MainWindow::instance->Destroy();
    MainWindow::instance = new MainWindow();

    wxMessageBox(errorMessage, "Config Read Error", wxOK, MainWindow::instance);
  }

  //GeneralPage::update();
  PropPage::instance->update();
  BladesPage::instance->update();
  PresetsPage::instance->update();
}
void Configuration::readConfig() {
  struct stat buffer;
  if (stat(CONFIG_PATH, &buffer) != 0) {
    if (wxMessageBox("No existing configuration file was detected. Would you like to import one?", "ProffieConfig", wxICON_INFORMATION | wxYES_NO | wxYES_DEFAULT) == wxYES) {
      Configuration::importConfig();
      MainWindow::instance->Show(true);
      return;
    } else return;
  }

  Configuration::readConfig(CONFIG_PATH);
}
void Configuration::importConfig() {
  wxFileDialog configLocation(MainWindow::instance, "Choose ProffieOS Config File", "", "", "C Header Files (*.h)|*.h", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

  if (configLocation.ShowModal() == wxID_CANCEL) return; // User Closed

  MainWindow::instance->Destroy();
  MainWindow::instance = new MainWindow();

  Configuration::readConfig(configLocation.GetPath().ToStdString());
}

void Configuration::readConfigTop(std::ifstream& file) {
  std::string element;
  std::vector<std::string> defines;
  while (!file.eof() && element != "#endif") {
    file >> element;
    if (element == "//") {
      getline(file, element);
      continue;
    }
    if (std::strstr(element.data(), "/*")) {
      while (!file.eof()) {
        if (std::strstr(element.data(), "*/")) break;
        file >> element;
      }
      continue;
    }
    if (element == "#define" && !file.eof()) {
      getline(file, element);
      defines.push_back(element);
    } else if (element == "const" && !file.eof()) {
      getline(file, element);
      std::strtok(element.data(), "="); // unsigned int maxLedsPerStrip =
      element = std::strtok(nullptr, " ;");
      GeneralPage::instance->maxLEDs->num->SetValue(std::stoi(element));
    } else if (element == "#include" && !file.eof()) {
      file >> element;
      if (std::strstr(element.c_str(), "v1") != NULL) {
        GeneralPage::instance->board->SetSelection(0);
      } else if (std::strstr(element.c_str(), "v2") != NULL) {
        GeneralPage::instance->board->SetSelection(1);
      } else if (std::strstr(element.c_str(), "v3") != NULL) {
        GeneralPage::instance->board->SetSelection(2);
      }
    } else if (element == "//PROFFIECONFIG") {
      file >> element;
      if (element == "ENABLE_MASS_STORAGE") GeneralPage::instance->massStorage->SetValue(true);
      if (element == "ENABLE_WEBUSB") GeneralPage::instance->webUSB->SetValue(true);
    }
  }
}
void Configuration::readConfigProp(std::ifstream& file) {
  std::string element;
  while (!file.eof() && element != "#endif") {
    file >> element;
    if (std::strstr(element.data(), "sa22c") != nullptr) PropPage::instance->prop->SetValue("SA22C");
    if (std::strstr(element.data(), "fett263") != nullptr) PropPage::instance->prop->SetValue("Fett263");
    if (std::strstr(element.data(), "shtok") != nullptr) PropPage::instance->prop->SetValue("Shtok");
    if (std::strstr(element.data(), "BC") != nullptr) PropPage::instance->prop->SetValue("BC");
    if (std::strstr(element.data(), "caiwyn") != nullptr) PropPage::instance->prop->SetValue("Caiwyn");
  }
}
void Configuration::readConfigPresets(std::ifstream& file) {
  std::string element;
  while (!file.eof() && element != "#endif") {
    file >> element;
    if (element == "//") {
      getline(file, element);
      continue;
    }
    if (std::strstr(element.data(), "/*")) {
      while (!file.eof()) {
        if (std::strstr(element.data(), "*/")) break;
        file >> element;
      }
      continue;
    }
    if (element == "Preset") readPresetArray(file);
    if (element == "BladeConfig") readBladeArray(file);
  }
}
void Configuration::readConfigStyles(std::ifstream& file) {
  std::string element;
  std::string styleName;
  std::string style;

  file >> element;
  while (!file.eof() && element != "#endif") {
    if (element == "using") {
      file >> element;
      styleName = element;

      file >> element; // Clear "="

      style = "";
      while (style.find(";") == std::string::npos) {
        file >> element;
        style += element;
      }
      style.erase(style.rfind(";")); // remove trailing ";"
      style.erase(std::remove(style.begin(), style.end(), '\n'), style.end()); // remove newlines

      // Remove potential StylePtr<> syntax
      if (style.find("StylePtr") != std::string::npos) {
        style.erase(style.find("StylePtr<"), 9);
        style.erase(style.rfind(">()"), 3);
      }

      Configuration::replaceStyles(styleName, style);
    }
    file >> element;
  }
}
void Configuration::readPresetArray(std::ifstream& file) {
# define CHKSECT if (file.eof() || element == "#endif" || strstr(element.data(), "};") != NULL) return
# define RUNTOSECTION element.clear(); while (element != "{") { file >> element; CHKSECT; }

  BladeIDPage::instance->bladeArrays.push_back(BladeIDPage::BladeArray());
  BladeIDPage::BladeArray& bladeArray = BladeIDPage::instance->bladeArrays.at(BladeIDPage::instance->bladeArrays.size() - 1);

  char* tempData;
  std::string presetInfo;
  std::string element;
  std::string comment;
  element.clear();
  file >> element;
  bladeArray.name.assign(std::strtok(element.data(), "[]"));

  RUNTOSECTION;
  uint32_t preset = -1;
  bladeArray.presets.clear();
  while (!false) {
    presetInfo.clear();
    RUNTOSECTION;
    bladeArray.presets.push_back(PresetsPage::PresetConfig());
    preset++;

    while (std::strstr(presetInfo.data(), "}") == nullptr) {
      file >> element;
      CHKSECT;
      presetInfo.append(element);
    }

    // Directory
    element = presetInfo.substr(0, presetInfo.find(",")); // Get dir section
    presetInfo = presetInfo.substr(presetInfo.find(",") + 1); // increment presetInfo
    tempData = std::strtok(element.data(), ",\""); // Detokenize dir section
    bladeArray.presets[preset].dirs.assign(tempData == nullptr ? "" : tempData);

    // Same thing but for track
    element = presetInfo.substr(0, presetInfo.find(","));
    presetInfo = presetInfo.substr(presetInfo.find(",") + 1);
    tempData = std::strtok(element.data(), ",\"");
    bladeArray.presets[preset].track.assign(tempData == nullptr ? "" : tempData);

    // Deal with Fett's comments
    comment.clear();
    if (presetInfo.find("/*") != std::string::npos) {
      comment = presetInfo.substr(presetInfo.find("/*"), presetInfo.find("*/") + 2);
      presetInfo = presetInfo.substr(presetInfo.find("*/") + 2);
    }

    // Read actual styles
    for (uint32_t blade = 0; blade < numBlades; blade++) {
      if (presetInfo.find("&style_charging,") == 0) {
        presetInfo = presetInfo.substr(16 /* length of "&style_charging,"*/);
        bladeArray.presets[preset].styles.push_back("&style_charging");
      } else if (presetInfo.find("&style_pov") == 0) {
        presetInfo = presetInfo.substr(presetInfo.find(11 /* length of "&style_pov,"*/));
        bladeArray.presets[preset].styles.push_back("&style_pov");
      } else {
        element = presetInfo.substr(0, presetInfo.find("(),") + 2); // Copy in next

        presetInfo = presetInfo.substr(presetInfo.find("(),") + 3); // Increment
        bladeArray.presets[preset].styles.push_back(comment + element.substr(element.find("Style"), element.find("(),")));
      }
    }

    // Name
    tempData = std::strtok(presetInfo.data(), ",\"");
    bladeArray.presets[preset].name.assign(tempData == nullptr ? "" : tempData);
  }
# undef CHKSECT
# undef RUNTOSECTION
}
void Configuration::readBladeArray(std::ifstream& file) {
# define CHKSECT if (file.eof() || element == "#endif" || strstr(element.data(), "};") != NULL) return
# define RUNTOSECTION element.clear(); while (element != "{") { file >> element; CHKSECT; }
  // In future get detect val and presetarray association

  BladeIDPage::BladeArray bladeArray;
  std::string element;
  std::string data;
  uint32_t tempNumBlades;
  RUNTOSECTION;

  while (true) {
    bladeArray = {};
    RUNTOSECTION;
    file >> element;
    element = std::strtok(element.data(), " ,");
    bladeArray.value = std::strstr(element.data(), "NO_BLADE") ? 0 : std::stoi(element);
    CHKSECT;
    bladeArray.blades.clear();
    tempNumBlades = numBlades;
    for (uint32_t blade = 0; blade < tempNumBlades; blade++) {
      data.clear();

      do { // Gather entire blade data
        file >> element;
        CHKSECT;
        data.append(element);
      } while (std::strstr(data.data(), "),") == nullptr);

      if (std::strstr(data.data(), "SubBlade") != nullptr) {
        if (std::strstr(data.data(), "NULL") == nullptr) { // Top Level SubBlade
          bladeArray.blades.push_back(BladesPage::BladeConfig());
          if (std::strstr(data.data(), "WithStride")) bladeArray.blades[blade].subBladeWithStride = true;
        } else { // Lesser SubBlade
          blade--;
          tempNumBlades--;
          // Switch to operating on previous blade
        }

        bladeArray.blades[blade].isSubBlade = true;
        std::strtok(data.data(), "("); // SubBlade(
        bladeArray.blades[blade].subBlades.push_back({ (uint32_t)std::stoi(std::strtok(nullptr, "(,")), (uint32_t)std::stoi(std::strtok(nullptr, " (,")) });
        data = std::strtok(nullptr, ""); // Clear out mangled data from strtok, replace with rest of data ("" runs until end of what's left)
        // Rest will be handled by WS281X "if"
      }
      if (std::strstr(data.data(), "WS281XBladePtr") != nullptr) {
        if (bladeArray.blades.size() - 1 != blade) bladeArray.blades.push_back(BladesPage::BladeConfig());
        data = std::strstr(data.data(), "WS281XBladePtr"); // Shift start to blade data, in case of SubBlade;

        // This must be done first since std::strtok is destructive (adds null chars)
        if (std::strstr(data.data(), "bladePowerPin1") != nullptr) bladeArray.blades[blade].usePowerPin1 = true;
        if (std::strstr(data.data(), "bladePowerPin2") != nullptr) bladeArray.blades[blade].usePowerPin2 = true;
        if (std::strstr(data.data(), "bladePowerPin3") != nullptr) bladeArray.blades[blade].usePowerPin3 = true;
        if (std::strstr(data.data(), "bladePowerPin4") != nullptr) bladeArray.blades[blade].usePowerPin4 = true;
        if (std::strstr(data.data(), "bladePowerPin5") != nullptr) bladeArray.blades[blade].usePowerPin5 = true;
        if (std::strstr(data.data(), "bladePowerPin6") != nullptr) bladeArray.blades[blade].usePowerPin6 = true;

        std::strtok(data.data(), "<,"); // Clear WS281XBladePtr
        bladeArray.blades[blade].numPixels = std::stoi(std::strtok(nullptr, "<,"));
        bladeArray.blades[blade].dataPin = std::strtok(nullptr, ",");
        std::strtok(nullptr, ":"); // Clear Color8::
        element = std::strtok(nullptr, ":,"); // Set to color order;
        bladeArray.blades[blade].useRGBWithWhite = strstr(element.data(), "W") != nullptr;
        bladeArray.blades[blade].colorType.assign(element);

        continue;
      }
      if (std::strstr(data.data(), "SimpleBladePtr") != nullptr) {
        bladeArray.blades.push_back(BladesPage::BladeConfig());
        uint32_t numLEDs = 0;
        auto getStarTemplate = [](const std::string& element) -> std::string {
          if (std::strstr(element.data(), "RedOrange") != nullptr) return "RedOrange";
          if (std::strstr(element.data(), "Amber") != nullptr) return "Amber";
          if (std::strstr(element.data(), "White") != nullptr) return "White";
          if (std::strstr(element.data(), "Red") != nullptr) return "Red";
          if (std::strstr(element.data(), "Green") != nullptr) return "Green";
          if (std::strstr(element.data(), "Blue") != nullptr) return "Blue";
          // With this implementation, RedOrange must be before Red
          return BD_NORESISTANCE;
        };

        // These must be read first since std::strtok is destructive (adds null chars)
        if (std::strstr(data.data(), "bladePowerPin1") != nullptr) bladeArray.blades[blade].usePowerPin1 = true;
        if (std::strstr(data.data(), "bladePowerPin2") != nullptr) bladeArray.blades[blade].usePowerPin2 = true;
        if (std::strstr(data.data(), "bladePowerPin3") != nullptr) bladeArray.blades[blade].usePowerPin3 = true;
        if (std::strstr(data.data(), "bladePowerPin4") != nullptr) bladeArray.blades[blade].usePowerPin4 = true;
        if (std::strstr(data.data(), "bladePowerPin5") != nullptr) bladeArray.blades[blade].usePowerPin5 = true;
        if (std::strstr(data.data(), "bladePowerPin6") != nullptr) bladeArray.blades[blade].usePowerPin6 = true;

        std::strtok(data.data(), "<"); // Clear SimpleBladePtr and setup strtok

        element = std::strtok(nullptr, "<,");
        bladeArray.blades[blade].Star1.assign(getStarTemplate(element));
        if (bladeArray.blades[blade].Star1 != BD_NORESISTANCE) {
          numLEDs++;
          bladeArray.blades[blade].Star1Resistance = std::stoi(std::strtok(nullptr, "<>"));
        }
        element = std::strtok(nullptr, "<,");
        bladeArray.blades[blade].Star2.assign(getStarTemplate(element));
        if (bladeArray.blades[blade].Star2 != BD_NORESISTANCE) {
          numLEDs++;
          bladeArray.blades[blade].Star2Resistance = std::stoi(std::strtok(nullptr, "<>"));
        }
        element = std::strtok(nullptr, "<, ");
        bladeArray.blades[blade].Star3.assign(getStarTemplate(element));
        if (bladeArray.blades[blade].Star3 != BD_NORESISTANCE) {
          numLEDs++;
          bladeArray.blades[blade].Star3Resistance = std::stoi(std::strtok(nullptr, "<>"));
        }
        element = std::strtok(nullptr, "<, ");
        bladeArray.blades[blade].Star4.assign(getStarTemplate(element));
        if (bladeArray.blades[blade].Star4 != BD_NORESISTANCE) {
          numLEDs++;
          bladeArray.blades[blade].Star4Resistance = std::stoi(std::strtok(nullptr, "<>"));
        }

        if (numLEDs <= 2) bladeArray.blades[blade].type.assign(BD_SINGLELED);
        if (numLEDs == 3) bladeArray.blades[blade].type.assign("Tri-Star Star");
        if (numLEDs >= 4) bladeArray.blades[blade].type.assign("Quad-Star Star");
      }
    }

    data.clear();
    do {
      file >> element;
      CHKSECT;
      data.append(element);
    } while (std::strstr(data.data(), ")") == nullptr);

    int32_t nameStart = data.find("CONFIGARRAY(") + 12;
    int32_t nameEnd = data.find(")");
    bladeArray.name.assign(data.substr(nameStart, nameEnd - nameStart));

    if (bladeArray.blades.empty())
      bladeArray.blades.push_back(BladesPage::BladeConfig{});

    for (BladeIDPage::BladeArray& array : BladeIDPage::instance->bladeArrays) {
      if (array.name == bladeArray.name) {
        array.value = bladeArray.value;
        array.blades = bladeArray.blades;
      }
    }
  }

# undef CHKSECT
# undef RUNTOSECTION
}
void Configuration::replaceStyles(const std::string& styleName, const std::string& styleFill) {
  std::string styleCheck;
  for (PresetsPage::PresetConfig& preset : BladeIDPage::instance->bladeArrays[BladesPage::instance->bladeArray->GetSelection()].presets) {
    for (wxString& style : preset.styles) {
      styleCheck = (style.find(styleName) == std::string::npos) ? style : style.substr(style.find(styleName));
      while (styleCheck != style) {
        // If there are no comments in the style, we're fine.
        // if the start of the next comment comes before the end of a comment, we *should* be outside the comment, and we're good to go.
        // This potentially could be broken though...
        if (style.find("/*") == std::string::npos || styleCheck.find("/*") <= styleCheck.find("*/")) {
          style.replace(style.find(styleCheck), styleName.length(), styleFill);
        }
        styleCheck = styleCheck.find(styleName) == std::string::npos ? style : style.substr(styleCheck.find(styleName));
      }
    }
  }
}

bool Configuration::runPrechecks() {
# define ERR(msg) \
  Misc::MessageBoxEvent* msgEvent = new Misc::MessageBoxEvent(Misc::EVT_MSGBOX, wxID_ANY, msg, "Configuration Error"); \
  wxQueueEvent(MainWindow::instance->GetEventHandler(), msgEvent); \
  return false;

  if (BladeIDPage::instance->enableDetect->GetValue() && BladeIDPage::instance->detectPin->entry->GetValue() == "") {
    ERR("Blade Detect Pin cannot be empty.");
  }
  if (BladeIDPage::instance->enableID->GetValue() && BladeIDPage::instance->IDPin->entry->GetValue() == "") {
    ERR("Blade ID Pin cannot be empty.");
  }
  if ([]() { for (const BladeIDPage::BladeArray& array : BladeIDPage::instance->bladeArrays) if (array.name == "") return true; return false; }()) {
    ERR("Blade Array Name cannot be empty.");
  }
  if (BladeIDPage::instance->enableID->GetValue() && BladeIDPage::instance->mode->GetStringSelection() == BLADE_ID_MODE_BRIDGED && BladeIDPage::instance->pullupPin->entry->GetValue() == "") {
    ERR("Pullup Pin cannot be empty.");
  }
  if (BladeIDPage::instance->enableDetect->GetValue() && BladeIDPage::instance->enableID->GetValue() && BladeIDPage::instance->IDPin->entry->GetValue() == BladeIDPage::instance->detectPin->entry->GetValue()) {
    ERR("Blade ID Pin and Blade Detect Pin cannot be the same.");
  }
  if ([]() -> bool {
        auto getNumBlades = [](const BladeIDPage::BladeArray& array) {
          int32_t numBlades = 0;
          for (const BladesPage::BladeConfig& blade : array.blades) {
            blade.isSubBlade ? numBlades += blade.subBlades.size() : numBlades++;
          }
          return numBlades;
        };

        int32_t lastNumBlades = getNumBlades(BladeIDPage::instance->bladeArrays.at(0));
        for (const BladeIDPage::BladeArray& array : BladeIDPage::instance->bladeArrays) {
          if (getNumBlades(array) != lastNumBlades) return true;
          lastNumBlades = getNumBlades(array);
        }
        return false;
      }()) {
    ERR("All Blade Arrays must be the same length.\n\nPlease add/remove blades to make them equal");
  }


  return true;
# undef ERR
}

Configuration::ProffieBoard Configuration::parseBoardType(const std::string& value) {
  return value == "ProffieBoard V1" ? Configuration::ProffieBoard::V1 :
             value == "ProffieBoard V2" ? Configuration::ProffieBoard::V2 :
             Configuration::ProffieBoard::V3;
}
Configuration::SaberProp Configuration::parsePropSel(const std::string& value) {
  return value == PR_SA22C ? Configuration::SaberProp::SA22C :
             value == PR_FETT263 ? Configuration::SaberProp::FETT263 :
             value == PR_BC ? Configuration::SaberProp::BC :
             value == PR_CAIWYN ? Configuration::SaberProp::CAIWYN :
             value == PR_SHTOK ? Configuration::SaberProp::SHTOK :
             Configuration::SaberProp::DEFAULT;
}

