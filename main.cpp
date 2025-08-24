#define NOMINMAX
#include "rest_server.h"
#include <dpp/dpp.h>
#include <unordered_map>
#include <iostream>
#include "on_message.h"
#include "filter.h"
#include "on_ready.h"
#include "config_commands.h"
#include "on_button.h"
#include "globals.h"

const std::string BOT_TOKEN = "";

int main() {
    updFilterGlobals();
    testFilter();
    std::thread api_thread([&]() { rest_server(8080); });
    uint64_t intents = dpp::i_default_intents | dpp::i_message_content;
    dpp::cluster bot(
        BOT_TOKEN,
        intents,
        0,       // Shards (0 = auto)
        0,       // Cluster ID
        1,       // Max shards
        true    // Compression
    );

    bot.on_log(dpp::utility::cout_logger());

    setup_on_ready(bot);

    commands(bot);

    setup_on_message(bot, threshold);

    setup_on_button(bot);

    bot.start(dpp::st_wait);

    return 0;
}
