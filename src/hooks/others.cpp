/*
 * others.cpp
 *
 *  Created on: Jan 8, 2017
 *      Author: nullifiedcat
 */

#include "../common.h"
#include "../netmessage.h"
#include "../hack.h"
#include "hookedmethods.h"

bool CanPacket_hook(void* thisptr) {
	SEGV_BEGIN;
	return send_packets && ((CanPacket_t*)hooks::hkNetChannel->GetMethod(hooks::offCanPacket))(thisptr);
	SEGV_END;
	return false;
}

int IN_KeyEvent_hook(void* thisptr, int eventcode, int keynum, const char* pszCurrentBinding) {
	SEGV_BEGIN;
	if (g_pGUI->ConsumesKey((ButtonCode_t)keynum)) {
		return 0;
	}
	return ((IN_KeyEvent_t*)hooks::hkClient->GetMethod(hooks::offKeyEvent))(thisptr, eventcode, keynum, pszCurrentBinding);
	SEGV_END;
	return 0;
}

static CatVar log_sent(CV_SWITCH, "debug_log_sent_messages", "0", "Log sent messages");

bool SendNetMsg_hook(void* thisptr, INetMessage& msg, bool bForceReliable = false, bool bVoice = false) {
	SEGV_BEGIN;
	if (log_sent) {
		logging::Info("=> %s [%i] %s", msg.GetName(), msg.GetType(), msg.ToString());
	}
	//logging::Info("Sending NetMsg! %i", msg.GetType());
	if (hacks::shared::airstuck::IsStuck() && cathook && !g_Settings.bInvalid) {
		switch (msg.GetType()) {
		case net_NOP:
		case net_SignonState:
		case net_StringCmd:
		case 16: // cmdKeyValues
			break;
		default:
			//logging::Info("Blocked net message %i %s", msg.GetType(), msg.GetName());
			return false;
		}
	}
	return ((SendNetMsg_t*)hooks::hkNetChannel->GetMethod(hooks::offSendNetMsg))(thisptr, msg, bForceReliable, bVoice);
	SEGV_END;
	return false;
}

CatVar disconnect_reason(CV_STRING, "disconnect_reason", "", "Disconnect reason", "A custom disconnect reason");

void Shutdown_hook(void* thisptr, const char* reason) {
	SEGV_BEGIN;
	if (cathook && (disconnect_reason.convar_parent->m_StringLength > 3) && strstr(reason, "user")) {
		((Shutdown_t*)hooks::hkNetChannel->GetMethod(hooks::offShutdown))(thisptr, disconnect_reason.GetString());
	} else {
		((Shutdown_t*)hooks::hkNetChannel->GetMethod(hooks::offShutdown))(thisptr, reason);
	}
	SEGV_END;
}

void FrameStageNotify_hook(void* thisptr, int stage) {
	SEGV_BEGIN;
	//logging::Info("FrameStageNotify %i", stage);
	// Ambassador to festive ambassador changer. simple.
	if (!g_IEngine->IsInGame()) g_Settings.bInvalid = true;
	//logging::Info("fsi begin");// TODO dbg
	SVDBG("FSN %i", __LINE__);
	// TODO hack FSN hook
	if (TF && cathook && !g_Settings.bInvalid && stage == FRAME_RENDER_START) {
		SVDBG("FSN %i", __LINE__);
		if (force_thirdperson && !g_pLocalPlayer->life_state && CE_GOOD(g_pLocalPlayer->entity)) {
			SVDBG("FSN %i", __LINE__);
			CE_INT(g_pLocalPlayer->entity, netvar.nForceTauntCam) = 1;
		}
		SVDBG("FSN %i", __LINE__);
		if (stage == 5 && show_antiaim && g_IInput->CAM_IsThirdPerson()) {
			SVDBG("FSN %i", __LINE__);
			if (CE_GOOD(g_pLocalPlayer->entity)) {
				CE_FLOAT(g_pLocalPlayer->entity, netvar.deadflag + 4) = g_Settings.last_angles.x;
				CE_FLOAT(g_pLocalPlayer->entity, netvar.deadflag + 8) = g_Settings.last_angles.y;
			}
			SVDBG("FSN %i", __LINE__);
		}
	}
	SVDBG("FSN %i", __LINE__);
	SAFE_CALL(((FrameStageNotify_t*)hooks::hkClient->GetMethod(hooks::offFrameStageNotify))(thisptr, stage));
	SEGV_END;
}

CatVar override_fov_zoomed(CV_FLOAT, "fov_zoomed", "0", "FOV override (zoomed)", "Overrides FOV with this value when zoomed in (default FOV when zoomed is 20)");
CatVar override_fov(CV_FLOAT, "fov", "0", "FOV override", "Overrides FOV with this value");

void PostRender_hook(void* thisptr) {
	((PostRender_t*)hooks::hkClientMode->GetMethod(off_PostRender))(thisptr);
}

void SceneEnd_hook(void* thisptr) {
	IMatRenderContext* ctx = g_IMaterialSystem->GetRenderContext();
	logging::Info("ctx: 0x%08x", ctx);
	/*ctx->PushRenderTargetAndViewport();
	ITexture* tx = g_IMaterialSystem->FindTexture("_rt_FullFrameFB", "RenderTargets");
	ctx->SetRenderTarget(tx);
	ctx->Viewport(0, 0, 1920, 1080);// FIXME
	logging::Info("blend %f", g_IVRenderView->GetBlend());
	ctx->ClearColor3ub(0, 0, 0);
	ctx->ClearBuffers(false, true, false);
	static float mod[] = { 0.0f, 1.0f, 0.0f, 0.0f};
	g_IVRenderView->SetColorModulation(mod);*/
	((SceneEnd_t*)hooks::hkRenderView->GetMethod(off_SceneEnd))(thisptr);
	//ctx->PopRenderTargetAndViewport();
}

void OverrideView_hook(void* thisptr, CViewSetup* setup) {
	SEGV_BEGIN;
	((OverrideView_t*)hooks::hkClientMode->GetMethod(hooks::offOverrideView))(thisptr, setup);
	if (!cathook) return;
	bool zoomed = g_pLocalPlayer->bZoomed;
	if (zoomed && override_fov_zoomed) {
		setup->fov = override_fov_zoomed;
	} else {
		if (override_fov) {
			setup->fov = override_fov;
		}
	}
	SEGV_END;
}

static CatVar clean_chat(CV_SWITCH, "clean_chat", "0", "Clean chat", "Removes newlines from chat");
static CatVar dispatch_log(CV_SWITCH, "debug_log_usermessages", "0", "Log dispatched user messages");

bool DispatchUserMessage_hook(void* thisptr, int type, bf_read& buf) {
	SEGV_BEGIN;
	if (clean_chat) {
		if (type == 4) {
			int s = buf.GetNumBytesLeft();
			char* data = new char[s];
			for (int i = 0; i < s; i++)
				data[i] = buf.ReadByte();
			int j = 0;
			for (int i = 0; i < 3; i++) {
				while (char c = data[j++]) {
					if ((c == '\n' || c == '\r') && (i == 1 || i == 2)) data[j - 1] = '?';
				}
			}
			buf = bf_read(data, s);
			buf.Seek(0);
		}
	}
	if (dispatch_log) {
		logging::Info("D> %i", type);
	}
	//if (type != net_Tick) logging::Info("Got message: %s", type);
	return ((DispatchUserMessage_t*)hooks::hkClient->GetMethod(hooks::offFrameStageNotify + 1))(thisptr, type, buf);
	SEGV_END; return false;
}

void LevelInit_hook(void* thisptr, const char* newmap) {
	((LevelInit_t*) hooks::hkClientMode->GetMethod(hooks::offLevelInit))(thisptr, newmap);
	g_IEngine->ExecuteClientCmd("exec cat_matchexec");
	hacks::shared::aimbot::Reset();
	hacks::shared::airstuck::Reset();
//	LEVEL_SHUTDOWN(FollowBot);
	//if (TF) LEVEL_INIT(SpyAlert);
	chat_stack::Reset();
	hacks::shared::spam::Reset();
}

bool CanInspect_hook(IClientEntity*) { return true; }

void LevelShutdown_hook(void* thisptr) {
	((LevelShutdown_t*) hooks::hkClientMode->GetMethod(hooks::offLevelShutdown))(thisptr);
	g_Settings.bInvalid = true;
	hacks::shared::aimbot::Reset();
	hacks::shared::airstuck::Reset();
	chat_stack::Reset();
	hacks::shared::spam::Reset();
}

