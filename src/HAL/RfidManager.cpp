#include "RfidManager.h"

RfidManager::RfidManager() : _lastStableTag{0, false} {}
void RfidManager::begin(uint8_t ssPin, uint8_t rstPin) {}
void RfidManager::update() {}
RfidTag RfidManager::getCurrentTag() { return _lastStableTag; }
bool RfidManager::isTagChanged() { return false; }