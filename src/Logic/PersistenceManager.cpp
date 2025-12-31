#include "PersistenceManager.h"

void PersistenceManager::begin() {}
void PersistenceManager::savePlaybackState(const PlaybackState& state) {}
PlaybackState PersistenceManager::loadPlaybackState() { return {0,0,0,false}; }