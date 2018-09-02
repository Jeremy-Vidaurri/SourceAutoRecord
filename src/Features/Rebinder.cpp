#include "Rebinder.hpp"

#include <string>

#include "Modules/Console.hpp"
#include "Modules/InputSystem.hpp"

#include "Command.hpp"
#include "Variable.hpp"

Variable sar_save_flag("sar_save_flag", "#SAVE#", "Echo message when using sar_bind_save.\n"
                                                  "Default is \"#SAVE#\", a SourceRuns standard.\n"
                                                  "Keep this empty if no echo message should be binded.\n",
    0);

Rebinder* rebinder;

Rebinder::Rebinder()
    : saveButton(0)
    , reloadButton(0)
    , saveName()
    , reloadName()
    , isSaveBinding(false)
    , isReloadBinding(false)
    , lastIndexNumber(0)
{
    this->hasLoaded = true;
}
void Rebinder::SetSaveBind(int button, const char* name)
{
    this->saveButton = button;
    this->saveName = std::string(name);
    this->isSaveBinding = true;
}
void Rebinder::SetReloadBind(int button, const char* name)
{
    this->reloadButton = button;
    this->reloadName = std::string(name);
    this->isReloadBinding = true;
}
void Rebinder::ResetSaveBind()
{
    this->isSaveBinding = false;
    inputSystem->KeySetBinding(this->saveButton, "");
}
void Rebinder::ResetReloadBind()
{
    this->isReloadBinding = true;
    inputSystem->KeySetBinding(this->reloadButton, "");
}
void Rebinder::RebindSave()
{
    if (!this->isSaveBinding)
        return;

    std::string cmd = std::string("save \"") + this->saveName;
    if (this->lastIndexNumber > 0) {
        cmd += std::string("_") + std::to_string(lastIndexNumber);
    }

    cmd += std::string("\"");

    if (sar_save_flag.GetString()[0] != '\0')
        cmd += std::string(";echo \"") + std::string(sar_save_flag.GetString()) + std::string("\"");

    inputSystem->KeySetBinding(this->saveButton, cmd.c_str());
}
void Rebinder::RebindReload()
{
    if (!this->isReloadBinding)
        return;

    std::string cmd = std::string("save \"") + this->reloadName;
    if (this->lastIndexNumber > 0) {
        cmd += std::string("_") + std::to_string(this->lastIndexNumber);
    }

    cmd += std::string("\";reload");
    inputSystem->KeySetBinding(this->reloadButton, cmd.c_str());
}
void Rebinder::UpdateIndex(int newIndex)
{
    this->lastIndexNumber = newIndex;
}

CON_COMMAND(sar_bind_save,
    "Automatic save rebinding when server has loaded.\n"
    "File indexing will be synced when recording demos.\n"
    "Usage: sar_bind_save <key> [save_name]\n")
{
    if (args.ArgC() != 3) {
        console->Print("sar_bind_save <key> [save_name] : Automatic save rebinding when server has loaded. File indexing will be synced when recording demos.\n");
        return;
    }

    int button = inputSystem->GetButton(args[1]);
    if (button == BUTTON_CODE_INVALID) {
        console->Print("\"%s\" isn't a valid key!\n", args[1]);
        return;
    } else if (button == KEY_ESCAPE) {
        console->Print("Can't bind ESCAPE key!\n", args[1]);
        return;
    }

    if (rebinder->isReloadBinding && button == rebinder->reloadButton) {
        rebinder->ResetReloadBind();
    }

    rebinder->SetSaveBind(button, args[2]);
    rebinder->RebindSave();
}
CON_COMMAND(sar_bind_reload,
    "Automatic save-reload rebinding when server has loaded.\n"
    "File indexing will be synced when recording demos.\n"
    "Usage: sar_bind_reload <key> [save_name]\n")
{
    if (args.ArgC() != 3) {
        console->Print("sar_bind_reload <key> [save_name] : Automatic save-reload rebinding when server has loaded. File indexing will be synced when recording demos.\n");
        return;
    }

    int button = inputSystem->GetButton(args[1]);
    if (button == BUTTON_CODE_INVALID) {
        console->Print("\"%s\" isn't a valid key!\n", args[1]);
        return;
    } else if (button == KEY_ESCAPE) {
        console->Print("Can't bind ESCAPE key!\n", args[1]);
        return;
    }

    if (rebinder->isSaveBinding && button == rebinder->saveButton) {
        rebinder->ResetSaveBind();
    }

    rebinder->SetReloadBind(button, args[2]);
    rebinder->RebindReload();
}
CON_COMMAND(sar_unbind_save,
    "Unbinds current save rebinder.\n")
{
    if (!rebinder->isSaveBinding) {
        console->Print("There's nothing to unbind.\n");
        return;
    }
    rebinder->ResetSaveBind();
}
CON_COMMAND(sar_unbind_reload,
    "Unbinds current save-reload rebinder.\n")
{
    if (!rebinder->isReloadBinding) {
        console->Print("There's nothing to unbind.\n");
        return;
    }
    rebinder->ResetReloadBind();
}
