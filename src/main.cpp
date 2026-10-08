/*
    This project is licensed under CC BY-NC 4.0
    https://creativecommons.org/licenses/by-nc/4.0*

    You are free to:
    • Share: Copy and redistribute the material in any medium or format.
    • Adapt: Remix, transform, and build upon the material*

    Under the following terms:
    • Attribution: You must give appropriate credit, provide a link to the original source repository, and indicate if changes were made.
    • Non-Commercial: You may not use the material for commercial purposes*

    You are not allowed to:
    • Sell: This license forbids selling original or modified material for commercial purposes.
    • Sublicense: This license forbids sublicensing original or modified material.

    © Copyright by IMXNOOBX (https://github.com/IMXNOOBX) and contributors.
    See https://github.com/IMXNOOBX/cs2-external-esp/-/blob/main/LICENSE for full details.
*/

#include <iostream>
#include <string_view>
#include <vector>

#include "updater/Updater.hpp"
#include "core/engine/Engine.hpp"
#include "core/version/AppVersion.hpp"
#include "gui/renderer/Renderer.hpp"

#include <external/exception.hpp>

int wmain(const int argc, wchar_t* argv[])
{
    c_exception_handler::setup();

    LogHelper::Init();

    LOGF(INFO, "Pluto v{} compiled {} {}", app_version::current_text, __DATE__, __TIME__);

    std::vector<std::wstring_view> arguments;
    arguments.reserve(static_cast<std::size_t>(argc));
    for (int index = 0; index < argc; ++index) {
        arguments.emplace_back(argv[index]);
    }
    const auto startup_action = updater::Updater::ProcessStartup(arguments);
    if (startup_action != updater::StartupAction::continue_launch) {
        LogHelper::Destroy();
        return startup_action == updater::StartupAction::exit_success ? 0 : 1;
    }

    // Needs to be ran as ADMINISTRATOR
    if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
        LOGF(WARNING, "Could not set application process priority to HIGH");

    if (!Engine::Init()) {
        LOGF(FATAL, "Engine failed to initialize, cannot continue execution");
        goto exit;
    }

    if (!Renderer::Init()) {
        LOGF(FATAL, "Renderer failed to initialize, cannot continue execution");
        goto exit;
    }

    LOGF(INFO, "Everything setup and ready, just... make sure you are not in \"Full Screen\"!");

    // Locking
    Renderer::Thread();

exit:
    LOGF(INFO, "Thats it, im done, hope you had a great time!");
    LogHelper::Destroy();
    std::cin.get();
}
