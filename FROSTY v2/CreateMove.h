#pragma once
#include "HookIncludes.h"
#include "Hooks.h"
#include "Misc.h"
#include "LegitBot.h"
#include "GrenadePrediction.h"
#include "LagComp.h"
#include "ESP.h"
#include "AutoPistol.hpp"
#include "Trigger.hpp"

using create_move_t = void(__thiscall *)(IBaseClientDLL *, int, float, bool);
QAngle qLastTickAngles = QAngle(0.0f, 0.0f, 0.0f);
void __stdcall CHLCreateMove(int sequence_number, float input_sample_frametime, bool active, bool& bSendPacket)
{
	static auto ofunc = hooks::client.get_original<create_move_t>(21);
	ofunc(g_CHLClient, sequence_number, input_sample_frametime, active);
	C_BaseEntity *local = g_EntityList->GetClientEntity(g_Engine->GetLocalPlayer());
	CInput::CUserCmd *cmd = g_Input->GetUserCmd(0, sequence_number);
	Globals::LocalPlayer = g_EntityList->GetClientEntity(g_Engine->GetLocalPlayer());
	Globals::CurrentCmd = cmd;
	if (!cmd)
		return;

	static float next_time = 0;
	if (!Globals::ySwitch && local->IsAlive() && local->GetVelocity().Length2D() == 0)
	{

		float TickStuff = TICKS_TO_TIME(local->GetTickBase());

		Globals::shouldflip = false;

		bool moveFlip;

		Globals::NextTime = next_time;

		if (next_time - TICKS_TO_TIME(local->GetTickBase()) > 1.1)
		{
			next_time = 0;
		}

		if (local->GetVelocity().Length2D() > 1)
		{
			moveFlip = true;
		}

		if (local->GetVelocity().Length2D() == 0 && !moveFlip)
		{
			if (TickStuff > next_time + 1.1f)
			{
				next_time = TickStuff + TICKS_TO_TIME(1);
				Globals::shouldflip = true;
			}
		}
		else if (moveFlip && local->GetVelocity().Length2D() == 0)
		{
			if (TickStuff > next_time + 0.22f)
			{
				next_time = TickStuff + TICKS_TO_TIME(1);
				Globals::shouldflip = true;
				moveFlip = false;
			}
		}
	}

	if (!cmd->command_number)
		return;

	CInput::CVerifiedUserCmd *verifiedCommands = *(CInput::CVerifiedUserCmd **)(reinterpret_cast< uint32_t >(g_Input) + 0xF0);
	CInput::CVerifiedUserCmd *verified = &verifiedCommands[sequence_number % 150];
	if (!verified)
		return;

	if (!local)
		return;
	QAngle oldAngle = cmd->viewangles;
	float oldForward = cmd->forwardmove;
	float oldSideMove = cmd->sidemove;

	backtracking->legitBackTrack(cmd, local);

	RankReveal(cmd);

	misc::instance().OnCreateMove(cmd, local);

	if (g_Options.Misc.AutoStrafe)
		misc::AutoStrafe(cmd, local, cmd->viewangles);

	legitbot::instance().OnCreateMove(cmd, local);
	grenade_prediction::instance().Tick(cmd->buttons);

	trigger.CreateMove_Post(local, cmd);
	apist.CreateMove_Post(local, cmd);

	if (g_Options.Misc.antiuntrusted)
		if (!sanitize_angles(cmd->viewangles))
			return;

	movementfix(oldAngle, cmd);

	if (g_Options.Misc.antiuntrusted)
		clampMovement(cmd);

	verified->m_cmd = *cmd;
	verified->m_crc = cmd->GetChecksum();


	if (Globals::ySwitch) {
		Globals::FakeAngle = cmd->viewangles.y;
	}
	else {
		Globals::RealAngle = cmd->viewangles.y;
	}
}
#pragma warning(disable : 4409)
__declspec(naked) void __stdcall hkCreateMove(int sequence_number, float input_sample_frametime, bool active)
{
	__asm
	{
		PUSH	EBP
		MOV		EBP, ESP
		PUSH	EBX
		LEA		ECX, [ESP]
		PUSH	ECX
		PUSH	active
		PUSH	input_sample_frametime
		PUSH	sequence_number
		CALL	CHLCreateMove
		POP		EBX
		POP		EBP
		RETN	0xC
	}
}