#include "cli_tarot.h"
#include <furi.h>
#include <storage/storage.h>
#include <toolbox/stream/stream.h>
#include <toolbox/stream/file_stream.h>
#include <firmware\targets\furi_hal_include\furi_hal.h>
#include <build\f7-firmware-C\sdk_headers\f7_sdk\firmware\targets\furi_hal_include\furi_hal_info.h>
#include <task_control_block.h>
#include <time.h>
#include <notification/notification_messages.h>
#include <loader/loader.h>
#include <lib/toolbox/args.h>

// Define log tag
#define TAG "example_apps_assets"

static void example_apps_data_print_file_content(Storage* storage, const char* path) {
    Stream* stream = file_stream_alloc(storage);
    FuriString* line = furi_string_alloc();

    FURI_LOG_I(TAG, "----------------------------------------");
    FURI_LOG_I(TAG, "File \"%s\" content:", path);
    if(file_stream_open(stream, path, FSAM_READ, FSOM_OPEN_EXISTING)) {
        while(stream_read_line(stream, line)) {
            furi_string_replace_all(line, "\r", "");
            furi_string_replace_all(line, "\n", "");
            FURI_LOG_I(TAG, "%s", furi_string_get_cstr(line));
        }
    } else {
        FURI_LOG_E(TAG, "Failed to open file");
    }
    FURI_LOG_I(TAG, "----------------------------------------");

    furi_string_free(line);
    file_stream_close(stream);
    stream_free(stream);
}

// Application entry point
int32_t example_apps_assets_main(void* p) {
    // Mark argument as unused
    UNUSED(p);

    // Open storage
    Storage* storage = furi_record_open(RECORD_STORAGE);

    example_apps_data_print_file_content(storage, APP_ASSETS_PATH("test_asset.txt"));
    example_apps_data_print_file_content(storage, APP_ASSETS_PATH("poems/a jelly-fish.txt"));
    example_apps_data_print_file_content(storage, APP_ASSETS_PATH("poems/theme in yellow.txt"));
    example_apps_data_print_file_content(storage, APP_ASSETS_PATH("poems/my shadow.txt"));

    // Close storage
    furi_record_close(RECORD_STORAGE);

    return 0;
}

typedef struct {
    char name[100];
    char description[500];
    int is_reversed;
} TarotCard;

void shuffle_deck(TarotCard* deck, int num_cards) {
    srand(time(NULL));
    for (int i = num_cards - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        TarotCard temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
}

// void cli_command_tarot(Cli* cli, FuriString* user_input, void* context) {
//     // create a tarot card type, create an array of tarot cards as a deck, 
//     // and let the user enter a number to do a draw of n cards. Default 3. 
//     // Cards can be reversed or not. 

//     // The deck list is too big to be compiled into internal flash memory. 
//     // So it must be stored in Apps Asset folder on SD card and then retrieved 
//     // at runtime. Use STORAGE_APP_ASSETS_PATH_PREFIX for file path to SD card.

//     UNUSED(cli);
//     UNUSED(context);

//     TarotCard deck[] = {
//     { "The Fool", "Upright: A new beginning, innocence, spontaneity.", 0 },
//     { "The Magician", "Upright: Manifestation, resourcefulness, power.", 0 },
//     { "The High Priestess", "Upright: Intuition, sacred knowledge, divine feminine.", 0 },
    
// };
    
//     int total_cards = sizeof(deck) / sizeof(deck[0]);

//     int num_cards = 3;
//     char* cstr = furi_string_get_cstr(user_input);
//     if (strlen(cstr) > 0) {
//         num_cards = atoi(cstr);
//     }

//     if (num_cards > total_cards) {
//         printf("There are only %d cards in the deck.\r\n", total_cards);
//         return;
//     }

//     shuffle_deck(deck, total_cards);

//     for (int i = 0; i < num_cards; i++) {
//         TarotCard card = deck[i];
//         printf("Card: %s\n", card.name);
//         printf("Orientation: %s\n", card.is_reversed ? "Reversed" : "Upright");
//         printf("Description: %s\n", card.description);
//         printf("\n");
//     }
// }