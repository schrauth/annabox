#pragma once
#include <stdint.h>

// --- Card Mapping Configuration ---
// This file contains the mapping of RFID card UIDs to specific folder numbers.
// To add a new card:
// 1. Place the card on the reader and note the UID from the Serial Monitor.
// 2. Add a new line to the s_knownCards array below, e.g., {0xYOURUID, FOLDER_NUMBER}.

struct CardMapping {
    uint32_t uid;
    uint8_t folder;
};

static const CardMapping s_knownCards[] = {
    {0x03F44306, 1}, // Test Card 1 -> Folder 01
    {0x4652F705, 2}, // Test Card 2 -> Folder 02 
    {0x71D18EF5, 3}, // small orange prototype
    {0x41CA8DF5, 6}, // Baer
    {0x615C8EF5, 4}, // Mond und Sterne
    {0x41658DF5, 8}, // Pinguin
    {0x41C68DF5, 7}, // Wasserhahn
    {0x51618DF5, 9}, // Sonne
    {0x1C48CF5, 5}  // noch unbelegt
};

// Automatically calculate the number of cards in the array.
static const uint8_t s_numKnownCards = sizeof(s_knownCards) / sizeof(s_knownCards[0]);