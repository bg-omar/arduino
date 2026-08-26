#include "autonomy_menu.h"

#include "avoid_objects.h"
#include "brain_link.h"
#include "config.h"
#include "displayAdafruit.h"
#include "logger.h"
#include "main_ra.h"
#include "oled_mode.h"
#include "robot_modes.h"
#include "sense_react.h"

namespace {
constexpr int kCount = 5;
const char* const kLabels[kCount] = {
    "Brain Auto",
    "Brain Imitation",
    "Sense React",
    "Free Roam",
    "Stop / Manual",
};
int sSelected = 0;
bool sOpen = false;

void finishToLog() {
    sOpen = false;
    if (FEATURE_ENABLED(main::use_adafruit, USE_ADAFRUIT) && main::Found_Display) {
        displayAdafruit::setMode(OledMode::Log);
    }
}
}

namespace autonomy_menu {
void draw() {
    if (!sOpen || !FEATURE_ENABLED(main::use_adafruit, USE_ADAFRUIT) || !main::Found_Display) return;
    displayAdafruit::setMode(OledMode::AutonomyMenu);
    auto& d = displayAdafruit::display;
    d.clearDisplay();
    d.setTextColor(SH110X_WHITE);
    d.setTextSize(1);
    d.setCursor(0, 7);
    d.println("AUTONOMY - X select");
    d.drawLine(0, 10, 127, 10, SH110X_WHITE);
    for (int i = 0; i < kCount; ++i) {
        d.setCursor(2, 20 + i * 9);
        d.print(i == sSelected ? "> " : "  ");
        d.print(kLabels[i]);
    }
    d.display();
}

void open() {
    sSelected = 0;
    sOpen = true;
    draw();
    logger::logln("Autonomy menu open; autonomy stopped");
}

void close() {
    if (!sOpen) return;
    finishToLog();
    logger::logln("Autonomy menu close; remains stopped/manual");
}

bool isOpen() { return sOpen; }

void up() {
    if (!sOpen) return;
    sSelected = (sSelected + kCount - 1) % kCount;
    draw();
}

void down() {
    if (!sOpen) return;
    sSelected = (sSelected + 1) % kCount;
    draw();
}

void select() {
    if (!sOpen) return;
    const int choice = sSelected;
    finishToLog();

    // Every selection is a clean authority handoff.
    robot_modes::stopAll();
    switch (choice) {
        case 0:
            brain_link::userStartBrain(false);
            logger::logln("Autonomy: Brain Auto");
            break;
        case 1:
            brain_link::userStartBrain(true);
            logger::logln("Autonomy: Brain Imitation");
            break;
        case 2:
            brain_link::userSelectRobotMode("SENSE");
            sense_react::start();
            logger::logln("Autonomy: Sense React");
            break;
        case 3:
            brain_link::userSelectRobotMode("FREE_ROAM");
            avoid_objects::start();
            logger::logln("Autonomy: Free Roam");
            break;
        case 4:
        default:
            brain_link::userStop();
            logger::logln("Autonomy: Stop / Manual");
            break;
    }
}
}
