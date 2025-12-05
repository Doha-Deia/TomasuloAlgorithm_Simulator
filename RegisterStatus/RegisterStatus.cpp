#include "RegisterStatus.h"

RegisterStatus::RegisterStatus() {

    for (int i = 0; i < 32; ++i) {
        robTag[i] = 0;
    }
}
RegisterStatus:: RegisterStatus(vector <int>tag) {

    for (int i = 0; i < 32; ++i) {
        robTag[i] = tag[i];
    }
}
void RegisterStatus::setTag(int regIndex, int robEntry) {
    if (regIndex >= 0 && regIndex < REG_COUNT) {
        robTag[regIndex] = robEntry;
    }
}
int RegisterStatus::getTag(int regIndex) {
    if (regIndex >= 0 && regIndex < REG_COUNT) {
        return robTag[regIndex];
    }
    return -1; // Invalid index
}
bool RegisterStatus::isBusy() {
    for (int i = 0; i < REG_COUNT; ++i) {
        if (robTag[i] != 0) {
            return true;
        }
    }
    return false;
}
void RegisterStatus::clearTag(int regIndex){
    if (regIndex >= 0 && regIndex < REG_COUNT) {
        robTag[regIndex] = 0;
    }
}
void RegisterStatus::clearAllTags() {
    for (int i = 0; i < REG_COUNT; ++i) {
        robTag[i] = 0;
    }
}