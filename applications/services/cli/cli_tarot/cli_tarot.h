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
#include <applications\services\cli\cli.h>

static void example_apps_data_print_file_content(Storage* storage, const char* path);
int32_t example_apps_assets_main(void* p);
void cli_command_tarot(Cli* cli, FuriString* user_input, void* context);