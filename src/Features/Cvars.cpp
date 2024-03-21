#include "Cvars.hpp"

#include "Command.hpp"
#include "Game.hpp"
#include "Modules/Client.hpp"
#include "Modules/Console.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"
#include "Modules/Tier1.hpp"
#include "Offsets.hpp"
#include "SAR.hpp"
#include "Utils/Memory.hpp"
#include "Variable.hpp"

#include <cstring>

Cvars *cvars;

Cvars::Cvars()
	: locked(true) {
	this->hasLoaded = true;
}
int Cvars::Dump(std::ofstream &file) {
	this->Lock();

	auto cmd = tier1->m_pConCommandList;
	auto count = 0;
	do {
		file << cmd->m_pszName;
		file << "[cvar_data]";

		auto IsCommand = reinterpret_cast<bool (*)(void *)>(Memory::VMT(cmd, Offsets::IsCommand));
		if (!IsCommand(cmd)) {
			auto cvar = reinterpret_cast<ConVar *>(cmd);
			file << cvar->m_pszDefaultValue;
		} else {
			file << "cmd";
		}
		file << "[cvar_data]";

		file << cmd->m_nFlags;
		file << "[cvar_data]";

		file << cmd->m_pszHelpString;
		file << "[end_of_cvar]";
		++count;

	} while (cmd = cmd->m_pNext);

	this->Unlock();

	return count;
}
int Cvars::DumpDoc(std::ofstream &file) {
	auto InternalDump = [&file](ConCommandBase *cmd, std::string games, bool isCommand) {
		file << cmd->m_pszName;
		file << "[cvar_data]";

		if (!isCommand) {
			auto cvar = reinterpret_cast<ConVar *>(cmd);
			file << cvar->m_pszDefaultValue;
		} else {
			file << "cmd";
		}
		file << "[cvar_data]";

		file << games.c_str();
		file << "[cvar_data]";

		file << cmd->m_pszHelpString;
		file << "[end_of_cvar]";
	};

	auto count = 0;
	for (const auto &var : Variable::GetList()) {
		if (var && !var->isReference) {
			InternalDump(var->ThisPtr(), Game::VersionToString(var->version), false);
			++count;
		}
	}
	for (const auto &com : Command::GetList()) {
		if (com && !com->isReference) {
			InternalDump(com->ThisPtr(), Game::VersionToString(com->version), true);
			++count;
		}
	}

	return count;
}
void Cvars::ListAll() {
	console->Msg("Commands:\n");
	for (auto &command : Command::GetList()) {
		if (!!command && command->isRegistered) {
			auto ptr = command->ThisPtr();
			console->Print("\n%s\n", ptr->m_pszName);
			console->Msg("%s", ptr->m_pszHelpString);
		}
	}
	console->Msg("\nVariables:\n");
	for (auto &variable : Variable::GetList()) {
		if (!variable) {
			continue;
		}

		auto ptr = variable->ThisPtr();
		if (variable->isRegistered) {
			console->Print("\n%s ", ptr->m_pszName);
			if (ptr->m_bHasMin) {
				console->Print("<number>\n");
			} else {
				console->Print("<string>\n");
			}
			console->Msg("%s", ptr->m_pszHelpString);
		} else if (variable->isReference) {
			std::string str = "";

			if (variable->hasCustomCallback && variable->isUnlocked)
				str = "(custom callback && unlocked)";
			else if (variable->hasCustomCallback)
				str += "(custom callback)";
			else if (variable->isUnlocked)
				str += "(unlocked)";

			console->Print("\n%s %s\n", ptr->m_pszName, str.c_str());
			if (std::strlen(ptr->m_pszHelpString) != 0) {
				console->Msg("%s\n", ptr->m_pszHelpString);
			}
		}
	}
}
void Cvars::PrintHelp(const CCommand &args) {
	if (args.ArgC() != 2) {
		return console->Print("help <cvar> - prints information about a cvar.\n");
	}

	auto cmd = reinterpret_cast<ConCommandBase *>(tier1->FindCommandBase(tier1->g_pCVar->ThisPtr(), args[1]));
	if (cmd) {
		auto IsCommand = reinterpret_cast<bool (*)(void *)>(Memory::VMT(cmd, Offsets::IsCommand));
		auto flags = GetFlags(*cmd);
		if (!IsCommand(cmd)) {
			auto cvar = reinterpret_cast<ConVar *>(cmd);
			console->Print("%s\n", cvar->m_pszName);
			console->Msg("Default: %s\n", cvar->m_pszDefaultValue);
			if (cvar->m_bHasMin) {
				console->Msg("Min: %f\n", cvar->m_fMinVal);
			}
			if (cvar->m_bHasMax) {
				console->Msg("Max: %f\n", cvar->m_fMaxVal);
			}
			console->Msg("Flags: %i - %s\n", cvar->m_nFlags, flags.c_str());
			console->Msg("Description: %s\n", cvar->m_pszHelpString);
		} else {
			console->Print("%s\n", cmd->m_pszName);
			console->Msg("Flags: %i - %s\n", cmd->m_nFlags, flags.c_str());
			console->Msg("Description: %s\n", cmd->m_pszHelpString);
		}
	} else {
		console->Print("Unknown cvar name!\n");
	}
}
std::string Cvars::GetFlags(const ConCommandBase &cmd) {
	std::vector<std::string> flags {
		"none",
		"unregistered",
		"developmentonly",
		"game",
		"client",
		"hidden",
		"protected",
		"sponly",
		"archive",
		"notify",
		"userinfo",
		"printableonly",
		"unlogged",
		"never_as_string",
		"replicated",
		"cheat",
		"ss",
		"demo",
		"dontrecord",
		"ss_added",
		"release",
		"reload_materials",
		"reload_textures",
		"not_connected",
		"material_system_thread",
		"archive_gameconsole",
		"accessible_from_threads",
		"f26", "f27",
		"server_can_execute",
		"server_cannot_query",
		"clientcmd_can_execute",
		"f31"
	};
	auto result = std::string();
	for (auto i = -1; i < 32; ++i) {
		if ((i == -1 && cmd.m_nFlags == 0) || cmd.m_nFlags & (1 << i)) {
			result += flags[i + 1] + " ";
		}
	}
	return result;
}
void Cvars::Lock() {
	if (!this->locked) {
		sv_accelerate.Lock();
		sv_airaccelerate.Lock();
		sv_friction.Lock();
		sv_maxspeed.Lock();
		sv_stopspeed.Lock();
		sv_maxvelocity.Lock();
		sv_footsteps.Lock();
		net_showmsg.Lock();

		sv_bonus_challenge.Lock();
		sv_laser_cube_autoaim.Lock();
		ui_loadingscreen_transition_time.Lock();
		ui_loadingscreen_fadein_time.Lock();
		ui_loadingscreen_mintransition_time.Lock();
		ui_transition_effect.Lock();
		ui_transition_time.Lock();
		hide_gun_when_holding.Lock();
		cl_viewmodelfov.Lock();
		r_flashlightbrightness.Lock();
		r_flashlightbrightness.AddFlag(FCVAR_CHEAT);

		cl_forwardspeed.AddFlag(FCVAR_CHEAT);
		cl_sidespeed.AddFlag(FCVAR_CHEAT);
		cl_backspeed.AddFlag(FCVAR_CHEAT);

		Variable("soundfade").RemoveFlag(FCVAR_CLIENTCMD_CAN_EXECUTE);
		Variable("leaderboard_open").RemoveFlag(FCVAR_CLIENTCMD_CAN_EXECUTE | FCVAR_SERVER_CAN_EXECUTE);
		Variable("gameui_activate").RemoveFlag(FCVAR_SERVER_CAN_EXECUTE);
		Variable("gameui_allowescape").RemoveFlag(FCVAR_SERVER_CAN_EXECUTE);
		Variable("gameui_preventescape").RemoveFlag(FCVAR_SERVER_CAN_EXECUTE);
		Variable("setpause").RemoveFlag(FCVAR_SERVER_CAN_EXECUTE);
		Variable("snd_ducktovolume").RemoveFlag(FCVAR_SERVER_CAN_EXECUTE);

		this->locked = true;
	}
}
void Cvars::Unlock() {
	if (this->locked) {
		sv_accelerate.Unlock();
		sv_airaccelerate.Unlock();
		sv_friction.Unlock();
		sv_maxspeed.Unlock();
		sv_stopspeed.Unlock();
		sv_maxvelocity.Unlock();
		sv_footsteps.Unlock();
		net_showmsg.Unlock();

		// Don't find a way to abuse this, ok?
		sv_bonus_challenge.Unlock(false);
		sv_laser_cube_autoaim.Unlock();
		ui_loadingscreen_transition_time.Unlock(false);
		ui_loadingscreen_fadein_time.Unlock(false);
		ui_loadingscreen_mintransition_time.Unlock(false);
		ui_transition_effect.Unlock(false);
		ui_transition_time.Unlock(false);
		hide_gun_when_holding.Unlock(false);
		cl_viewmodelfov.Unlock(false);
		r_flashlightbrightness.Unlock(false);
		r_flashlightbrightness.RemoveFlag(FCVAR_CHEAT);

		cl_forwardspeed.RemoveFlag(FCVAR_CHEAT);
		cl_sidespeed.RemoveFlag(FCVAR_CHEAT);
		cl_backspeed.RemoveFlag(FCVAR_CHEAT);

		Variable("soundfade").AddFlag(FCVAR_CLIENTCMD_CAN_EXECUTE);
		Variable("leaderboard_open").AddFlag(FCVAR_CLIENTCMD_CAN_EXECUTE | FCVAR_SERVER_CAN_EXECUTE);
		Variable("gameui_activate").AddFlag(FCVAR_CLIENTCMD_CAN_EXECUTE | FCVAR_SERVER_CAN_EXECUTE);
		Variable("gameui_allowescape").AddFlag(FCVAR_CLIENTCMD_CAN_EXECUTE | FCVAR_SERVER_CAN_EXECUTE);
		Variable("gameui_preventescape").AddFlag(FCVAR_CLIENTCMD_CAN_EXECUTE | FCVAR_SERVER_CAN_EXECUTE);
		Variable("setpause").AddFlag(FCVAR_SERVER_CAN_EXECUTE);
		Variable("snd_ducktovolume").AddFlag(FCVAR_SERVER_CAN_EXECUTE);

		this->locked = false;
	}
}
