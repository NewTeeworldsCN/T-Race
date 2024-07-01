#include "mod.h"

#include <base/math.h>

#include <engine/server.h>
#include <engine/shared/config.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/version.h>

#define MOD_VERSION "v0.1.0"

#define GAME_TYPE_NAME "T-Race"

CGameControllerMod::CGameControllerMod(class CGameContext *pGameServer) :
	IGameController(pGameServer)
{
	if(str_comp_nocase(g_Config.m_SvGametype, "hidden") == 0)
	{
		m_pGameType = GAME_TYPE_NAME "|Hidden";
		m_GameType = GAMETYPE_HIDDEN;
	}
	else if(str_comp_nocase(g_Config.m_SvGametype, "hiddendeath") == 0)
	{
		m_pGameType = GAME_TYPE_NAME "|HiddenDeath";
		m_GameType = GAMETYPE_HIDDENDEATH;
	}
	else if(str_comp_nocase(g_Config.m_SvGametype, "deathrun") == 0)
	{
		m_pGameType = GAME_TYPE_NAME "|DeathRun";
		m_GameType = GAMETYPE_DEATHRUN;
	}
	else if(str_comp_nocase(g_Config.m_SvGametype, "jail") == 0)
	{
		m_pGameType = GAME_TYPE_NAME "|Jail";
		m_GameType = GAMETYPE_JAIL;
	}
	else
	{
		m_pGameType = GAME_TYPE_NAME "|Team";
		m_GameType = GAMETYPE_TEAM;
	}

	m_Resetting = false;

	m_GameFlags = GAMEFLAG_TEAMS;
	GameServer()->m_ModGameType = m_GameType;

	g_Config.m_SvTeam = SV_TEAM_FORBIDDEN; // no ddnet team!
}

CGameControllerMod::~CGameControllerMod() = default;

CScore *CGameControllerMod::Score()
{
	return GameServer()->Score();
}

void CGameControllerMod::DoWincheck()
{
	if(m_GameOverTick != -1 || m_Resetting)
		return;

	m_TeamPlayersNum[TEAM_RED] = 0;
	m_TeamPlayersNum[TEAM_BLUE] = 0;

	for(auto &pPlayerA : GameServer()->m_apPlayers)
	{
		if(pPlayerA)
		{
			if(pPlayerA->GetTeam() == TEAM_RED)
				m_TeamPlayersNum[TEAM_RED] ++;
			if(pPlayerA->GetTeam() == TEAM_BLUE)
				m_TeamPlayersNum[TEAM_BLUE] ++;
		}
	}

	if(m_TeamPlayersNum[TEAM_BLUE] + m_TeamPlayersNum[TEAM_RED] < 2)
		return;

	if(m_GameType != GAMETYPE_TEAM)
	{
		if((!m_TeamPlayersNum[TEAM_BLUE] && m_TeamPlayersNum[TEAM_RED]) || (m_GameType == GAMETYPE_DEATHRUN && Server()->Tick() >= m_RoundStartTick + g_Config.m_SvTimelimit * 60 * Server()->TickSpeed()))
		{
			m_Winner = TEAM_RED;
			m_Resetting = true;
			EndRound();
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "%ss win!", GetTeamName(TEAM_RED));
			GameServer()->SendChatTarget(-1, aBuf);
			return;
		}
		else if((m_GameType != GAMETYPE_DEATHRUN && Server()->Tick() >= m_RoundStartTick + g_Config.m_SvTimelimit * 60 * Server()->TickSpeed()) || (m_TeamPlayersNum[TEAM_BLUE] && !m_TeamPlayersNum[TEAM_RED]))
		{
			m_Winner = TEAM_BLUE;
			m_Resetting = true;
			EndRound();
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "%ss win!", GetTeamName(TEAM_BLUE));
			GameServer()->SendChatTarget(-1, aBuf);
			return;
		}
	}
	// death run finish
	if(m_GameType == GAMETYPE_DEATHRUN)
	{
		int FinishPlayers = 0;
		int AlivePlayers = 0;
		for(auto &pPlayerA : GameServer()->m_apPlayers)
		{
			if(pPlayerA)
			{
				if(pPlayerA->GetTeam() == TEAM_BLUE)
				{
					if(Teams().GetDDRaceState(pPlayerA) == DDRACE_FINISHED)
						FinishPlayers ++;
					if(!pPlayerA->m_DeadSpec)
						AlivePlayers ++;
				}
			}
		}

		if(FinishPlayers >= AlivePlayers)
		{
			m_Winner = TEAM_BLUE;
			m_Resetting = true;
			EndRound();
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "Alive %ss win!", GetTeamName(TEAM_BLUE));
			GameServer()->SendChatTarget(-1, aBuf);
			return;
		}
	}
	if(m_GameType == GAMETYPE_TEAM)
	{
		int FinishPlayers[] = {0, 0};
		int AlivePlayers[] = {0, 0};
		for(auto &pPlayerA : GameServer()->m_apPlayers)
		{
			if(pPlayerA)
			{
				if(pPlayerA->GetTeam() == TEAM_RED)
				{
					if(Teams().GetDDRaceState(pPlayerA) == DDRACE_FINISHED)
						FinishPlayers[TEAM_RED] ++;
					if(!pPlayerA->m_DeadSpec)
						AlivePlayers[TEAM_RED] ++;
				}
				if(pPlayerA->GetTeam() == TEAM_BLUE)
				{
					if(Teams().GetDDRaceState(pPlayerA) == DDRACE_FINISHED)
						FinishPlayers[TEAM_BLUE] ++;
					if(!pPlayerA->m_DeadSpec)
						AlivePlayers[TEAM_BLUE] ++;
				}
			}
		}

		if(FinishPlayers[TEAM_RED] >= AlivePlayers[TEAM_RED])
		{
			m_Winner = TEAM_RED;
			m_Resetting = true;
			EndRound();
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "%ss win!", GetTeamName(TEAM_RED));
			GameServer()->SendChatTarget(-1, aBuf);
			return;
		}
		else if(FinishPlayers[TEAM_BLUE] >= AlivePlayers[TEAM_BLUE])
		{
			m_Winner = TEAM_BLUE;
			m_Resetting = true;
			EndRound();
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "%ss win!", GetTeamName(TEAM_BLUE));
			GameServer()->SendChatTarget(-1, aBuf);
			return;
		}
	}
}

const char *CGameControllerMod::GetTeamName(int Team)
{
	if(Team == TEAM_SPECTATORS)
		return "spectators";

	if(Team == TEAM_BLUE)
	{
		switch(m_GameType)
		{
			case GAMETYPE_HIDDEN: return "hider";
			case GAMETYPE_HIDDENDEATH: return "hider";
			case GAMETYPE_DEATHRUN: return "runner";
			case GAMETYPE_JAIL: return "prisoner";
			default: return "blue team";
		}
	}
	switch(m_GameType)
	{
		case GAMETYPE_HIDDEN: return "seeker";
		case GAMETYPE_HIDDENDEATH: return "seeker";
		case GAMETYPE_DEATHRUN: return "blocker";
		case GAMETYPE_JAIL: return "police";
		default: return "red team";
	}
}

bool CGameControllerMod::CanSpawn(int Team, vec2 *pOutPos, int DDTeam)
{
	// spectators can't spawn
	if(Team == TEAM_SPECTATORS)
		return false;

	CSpawnEval Eval;
	EvaluateSpawnType(&Eval, 0, DDTeam);
	EvaluateSpawnType(&Eval, Team + 1, DDTeam);

	*pOutPos = Eval.m_Pos;
	return Eval.m_Got;
}

void CGameControllerMod::HandleCharacterTiles(CCharacter *pChr, int MapIndex)
{
	bool HiddenType = m_GameType == GAMETYPE_HIDDEN || m_GameType == GAMETYPE_HIDDENDEATH;
	bool FinishType = m_GameType == GAMETYPE_DEATHRUN || m_GameType == GAMETYPE_TEAM;

	int TileIndex = GameServer()->Collision()->GetTileIndex(MapIndex);
	int TileFIndex = GameServer()->Collision()->GetFrontTileIndex(MapIndex);

	// hidden part
	if(HiddenType)
	{
		if(((TileIndex == TILE_SOLO_ENABLE) || (TileFIndex == TILE_SOLO_ENABLE)))
		{
			pChr->SetHidden(true);
		}
	}
	else if(FinishType)
	{
		CPlayer *pPlayer = pChr->GetPlayer();
		const int ClientId = pPlayer->GetCid();
		//Sensitivity
		int S1 = GameServer()->Collision()->GetPureMapIndex(vec2(pChr->GetPos().x + pChr->GetProximityRadius() / 3.f, pChr->GetPos().y - pChr->GetProximityRadius() / 3.f));
		int S2 = GameServer()->Collision()->GetPureMapIndex(vec2(pChr->GetPos().x + pChr->GetProximityRadius() / 3.f, pChr->GetPos().y + pChr->GetProximityRadius() / 3.f));
		int S3 = GameServer()->Collision()->GetPureMapIndex(vec2(pChr->GetPos().x - pChr->GetProximityRadius() / 3.f, pChr->GetPos().y - pChr->GetProximityRadius() / 3.f));
		int S4 = GameServer()->Collision()->GetPureMapIndex(vec2(pChr->GetPos().x - pChr->GetProximityRadius() / 3.f, pChr->GetPos().y + pChr->GetProximityRadius() / 3.f));
		int Tile1 = GameServer()->Collision()->GetTileIndex(S1);
		int Tile2 = GameServer()->Collision()->GetTileIndex(S2);
		int Tile3 = GameServer()->Collision()->GetTileIndex(S3);
		int Tile4 = GameServer()->Collision()->GetTileIndex(S4);
		int FTile1 = GameServer()->Collision()->GetFTileIndex(S1);
		int FTile2 = GameServer()->Collision()->GetFTileIndex(S2);
		int FTile3 = GameServer()->Collision()->GetFTileIndex(S3);
		int FTile4 = GameServer()->Collision()->GetFTileIndex(S4);

		const int PlayerDDRaceState = pChr->m_DDRaceState;
		bool IsOnStartTile = (TileIndex == TILE_START) || (TileFIndex == TILE_START) || FTile1 == TILE_START || FTile2 == TILE_START || FTile3 == TILE_START || FTile4 == TILE_START || Tile1 == TILE_START || Tile2 == TILE_START || Tile3 == TILE_START || Tile4 == TILE_START;
		// start
		if(IsOnStartTile && PlayerDDRaceState != DDRACE_CHEAT)
		{
			Teams().OnCharacterStart(ClientId);
			pChr->m_LastTimeCp = -1;
			pChr->m_LastTimeCpBroadcasted = -1;
			for(float &CurrentTimeCp : pChr->m_aCurrentTimeCp)
			{
				CurrentTimeCp = 0.0f;
			}
		}

		// finish
		if(((TileIndex == TILE_FINISH) || (TileFIndex == TILE_FINISH) || FTile1 == TILE_FINISH || FTile2 == TILE_FINISH || FTile3 == TILE_FINISH || FTile4 == TILE_FINISH || Tile1 == TILE_FINISH || Tile2 == TILE_FINISH || Tile3 == TILE_FINISH || Tile4 == TILE_FINISH) && PlayerDDRaceState == DDRACE_STARTED)
			Teams().OnCharacterFinish(ClientId);

		// solo part
		if(((TileIndex == TILE_SOLO_ENABLE) || (TileFIndex == TILE_SOLO_ENABLE)) && !Teams().m_Core.GetSolo(ClientId))
		{
			GameServer()->SendChatTarget(ClientId, "You are now in a solo part");
			pChr->SetSolo(true);
		}
		else if(((TileIndex == TILE_SOLO_DISABLE) || (TileFIndex == TILE_SOLO_DISABLE)) && Teams().m_Core.GetSolo(ClientId))
		{
			GameServer()->SendChatTarget(ClientId, "You are now out of the solo part");
			pChr->SetSolo(false);
		}
	}


}

void CGameControllerMod::OnPlayerConnect(CPlayer *pPlayer)
{
	IGameController::OnPlayerConnect(pPlayer);
	int ClientId = pPlayer->GetCid();

	// init the player
	Score()->PlayerData(ClientId)->Reset();

	// Can't set score here as LoadScore() is threaded, run it in
	// LoadScoreThreaded() instead
	Score()->LoadPlayerData(ClientId);

	int PlayerNum = 0;
	int TeamPlayersNum[] = {0, 0};
	for(auto &pPlayerA : GameServer()->m_apPlayers)
	{
		if(pPlayerA)
		{
			if(pPlayerA->GetTeam() == TEAM_SPECTATORS)		
				continue;
			if(pPlayerA->GetTeam() == TEAM_RED)
				TeamPlayersNum[TEAM_RED]++;
			else
				TeamPlayersNum[TEAM_BLUE]++;
			PlayerNum ++;
		}
	}
	if(PlayerNum == 2)
		EndRound();

	if(PlayerNum > 2)
	{
		if(m_GameType == GAMETYPE_TEAM || m_GameType == GAMETYPE_HIDDENDEATH || m_GameType == GAMETYPE_DEATHRUN)
		{
			pPlayer->SetTeam(TEAM_BLUE, false);
			if(m_GameType == GAMETYPE_TEAM && TeamPlayersNum[TEAM_RED] < TeamPlayersNum[TEAM_BLUE])
			{
				pPlayer->SetForceTeam(TEAM_RED);
			}
			if(Server()->Tick() >= m_RoundStartTick + g_Config.m_SvReservedTime * Server()->TickSpeed())
			{
				pPlayer->m_DeadSpec = true;
			}

			if(m_GameType == GAMETYPE_TEAM)
			{
				Teams().SetForceCharacterTeam(pPlayer->GetCid(), pPlayer->GetTeam() ? 2 : 1);
			}
		}
		else
		{
			pPlayer->SetTeam(TEAM_BLUE, false);
			if(Server()->Tick() >= m_RoundStartTick + g_Config.m_SvReservedTime * Server()->TickSpeed())
			{
				if(m_GameType != GAMETYPE_JAIL)
					pPlayer->SetForceTeam(TEAM_RED);
			}
		}
	}
	else
	{
		pPlayer->SetTeam(TEAM_BLUE, false);
	}

	if(!Server()->ClientPrevIngame(ClientId))
	{
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "'%s' entered and joined the %s", Server()->ClientName(ClientId), GetTeamName(pPlayer->GetTeam()));
		GameServer()->SendChat(-1, TEAM_ALL, aBuf, -1, CGameContext::FLAG_SIX);

		GameServer()->SendChatTarget(ClientId, "T-Race Mod. Version: " MOD_VERSION);
	}
}

void CGameControllerMod::OnPlayerDisconnect(CPlayer *pPlayer, const char *pReason)
{
	int ClientId = pPlayer->GetCid();
	bool WasModerator = pPlayer->m_Moderating && Server()->ClientIngame(ClientId);

	IGameController::OnPlayerDisconnect(pPlayer, pReason);

	if(!GameServer()->PlayerModerating() && WasModerator)
		GameServer()->SendChat(-1, TEAM_ALL, "Server kick/spec votes are no longer actively moderated.");

	if(g_Config.m_SvTeam != SV_TEAM_FORCED_SOLO)
		Teams().SetForceCharacterTeam(ClientId, TEAM_FLOCK);

	for(int Team = TEAM_FLOCK + 1; Team < TEAM_SUPER; Team++)
		if(Teams().IsInvited(Team, ClientId))
			Teams().SetClientInvited(Team, ClientId, false);

	int PlayerNum = 0;
	for(auto &pPlayerA : GameServer()->m_apPlayers)
	{
		if(pPlayerA)
		{
			if(pPlayerA->GetTeam() != TEAM_SPECTATORS)
				PlayerNum ++;
		}
	}
	if(PlayerNum < 2)
	{
		EndRound();
	}
}

void CGameControllerMod::OnReset()
{
	Teams().Reset();
	int PlayerNum = 0;
	for(auto &pPlayer : GameServer()->m_apPlayers)
	{
		if(pPlayer)
		{
			pPlayer->m_DeadSpec = false;
			pPlayer->m_Sleep = false;
			if(pPlayer->GetTeam() != TEAM_SPECTATORS)
			{
				pPlayer->SetForceTeam(TEAM_BLUE);
				PlayerNum ++;
			}
		}
	}
	if(PlayerNum == 0)
	{
		IGameController::OnReset();

		m_Winner = -1;
		return;
	}

	// choose red team;
	switch(m_GameType)
	{
		case GAMETYPE_HIDDEN:
		case GAMETYPE_HIDDENDEATH:
		case GAMETYPE_JAIL:
		{
			int ChangeNum;
			if(PlayerNum < 4)
				ChangeNum = 1;
			else if(PlayerNum < 8)
				ChangeNum = 2;
			else if(PlayerNum < 16)
				ChangeNum = 3;
			else if(PlayerNum < 32)
				ChangeNum = 4;
			else if(PlayerNum < 48)
				ChangeNum = 5;
			else
				ChangeNum = 6;

			for(int i = 0; i < ChangeNum; i++)
			{
				while(true)
				{
					CPlayer *pPlayer = GameServer()->m_apPlayers[rand() % MAX_CLIENTS];
					if(pPlayer && pPlayer->GetTeam() != TEAM_SPECTATORS)
					{
						pPlayer->SetForceTeam(TEAM_RED);
						pPlayer->m_Sleep = true;
						break;
					}
				}
			}
		}
		break;
		default:
		{
			bool Blue = false;
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				CPlayer *pPlayer = GameServer()->m_apPlayers[i];
				if(pPlayer && pPlayer->GetTeam() != TEAM_SPECTATORS)
				{
					pPlayer->SetForceTeam(Blue);
					if(g_Config.m_SvTeamMode && m_GameType == GAMETYPE_TEAM)
					{
						Teams().SetForceCharacterTeam(pPlayer->GetCid(), Blue ? 2 : 1);
					}
					Blue = !Blue;
				}
			}
			if(m_GameType == GAMETYPE_TEAM)
			{
				Teams().SetTeamFlock(1, true);
				Teams().SetTeamFlock(2, true);
				Teams().SetTeamLock(1, true);
				Teams().SetTeamLock(2, true);
			}
		}
		break;
	}
	IGameController::OnReset();

	m_Winner = -1;
	m_Resetting = false;
}

void CGameControllerMod::Tick()
{
	IGameController::Tick();
	if(Server()->Tick() == m_RoundStartTick + g_Config.m_SvReservedTime * Server()->TickSpeed())
	{
		if(m_GameType == GAMETYPE_HIDDEN || m_GameType == GAMETYPE_HIDDENDEATH || m_GameType == GAMETYPE_JAIL)
		{
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "%ss have been released", GetTeamName(TEAM_RED));
			GameServer()->SendBroadcast(aBuf, -1);
			for(auto &pPlayer : GameServer()->m_apPlayers)
			{
				if(pPlayer)
				{
					pPlayer->m_Sleep = false;
				}
			}
		}
	}
	else if(Server()->Tick() < m_RoundStartTick + g_Config.m_SvReservedTime * Server()->TickSpeed())
	{
		if(m_GameType == GAMETYPE_HIDDEN || m_GameType == GAMETYPE_HIDDENDEATH || m_GameType == GAMETYPE_JAIL)
		{
			for(auto &pPlayer : GameServer()->m_apPlayers)
			{
				if(!pPlayer)
					continue;
				if(pPlayer->GetTeam() == TEAM_RED)
				{
					pPlayer->m_Sleep = true;
				}
				else if(pPlayer->GetTeam() == TEAM_BLUE)
				{
					pPlayer->m_Sleep = false;
				}
			}
		}
	}

	DoWincheck();

	Teams().ProcessSaveTeam();
	Teams().Tick();
}

void CGameControllerMod::Snap(int SnappingClient)
{
	CNetObj_GameInfo *pGameInfoObj = Server()->SnapNewItem<CNetObj_GameInfo>(0);
	if(!pGameInfoObj)
		return;

	pGameInfoObj->m_GameFlags = m_GameFlags;
	pGameInfoObj->m_GameStateFlags = 0;
	if(m_GameOverTick != -1)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_GAMEOVER;
	if(m_SuddenDeath)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_SUDDENDEATH;
	if(GameServer()->m_World.m_Paused)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_PAUSED;
	pGameInfoObj->m_RoundStartTick = m_RoundStartTick;
	pGameInfoObj->m_WarmupTimer = m_Warmup;

	pGameInfoObj->m_RoundNum = 0;
	pGameInfoObj->m_RoundCurrent = m_RoundCount + 1;

	CCharacter *pChr;
	CPlayer *pPlayer = SnappingClient != SERVER_DEMO_CLIENT ? GameServer()->m_apPlayers[SnappingClient] : 0;
	CPlayer *pPlayer2;

	if(pPlayer && (pPlayer->m_TimerType == CPlayer::TIMERTYPE_GAMETIMER || pPlayer->m_TimerType == CPlayer::TIMERTYPE_GAMETIMER_AND_BROADCAST) && pPlayer->GetClientVersion() >= VERSION_DDNET_GAMETICK)
	{
		if((pPlayer->GetTeam() == TEAM_SPECTATORS || pPlayer->IsPaused()) && pPlayer->m_SpectatorId != SPEC_FREEVIEW && (pPlayer2 = GameServer()->m_apPlayers[pPlayer->m_SpectatorId]))
		{
			if((pChr = pPlayer2->GetCharacter()) && pChr->m_DDRaceState == DDRACE_STARTED)
			{
				pGameInfoObj->m_WarmupTimer = -pChr->m_StartTime;
				pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_RACETIME;
			}
		}
		else if((pChr = pPlayer->GetCharacter()) && pChr->m_DDRaceState == DDRACE_STARTED)
		{
			pGameInfoObj->m_WarmupTimer = -pChr->m_StartTime;
			pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_RACETIME;
		}
	}

	CNetObj_GameData *pGameData = Server()->SnapNewItem<CNetObj_GameData>(0);
	if(!pGameData)
		return;

	pGameData->m_FlagCarrierBlue = FLAG_TAKEN;
	pGameData->m_FlagCarrierRed = FLAG_TAKEN;;
	pGameData->m_TeamscoreBlue = m_Winner == -1 ? m_TeamPlayersNum[TEAM_BLUE] : m_Winner == TEAM_BLUE;
	pGameData->m_TeamscoreRed = m_Winner == -1 ? m_TeamPlayersNum[TEAM_RED] : m_Winner == TEAM_RED;

	CNetObj_GameInfoEx *pGameInfoEx = Server()->SnapNewItem<CNetObj_GameInfoEx>(0);
	if(!pGameInfoEx)
		return;

	pGameInfoEx->m_Flags =
		GAMEINFOFLAG_TIMESCORE |
		GAMEINFOFLAG_GAMETYPE_RACE |
		GAMEINFOFLAG_GAMETYPE_DDRACE |
		GAMEINFOFLAG_GAMETYPE_DDNET |
		GAMEINFOFLAG_UNLIMITED_AMMO |
		GAMEINFOFLAG_RACE_RECORD_MESSAGE |
		GAMEINFOFLAG_ALLOW_EYE_WHEEL |
		GAMEINFOFLAG_ALLOW_HOOK_COLL |
		GAMEINFOFLAG_ALLOW_ZOOM |
		GAMEINFOFLAG_BUG_DDRACE_GHOST |
		GAMEINFOFLAG_BUG_DDRACE_INPUT |
		GAMEINFOFLAG_PREDICT_DDRACE |
		GAMEINFOFLAG_PREDICT_DDRACE_TILES |
		GAMEINFOFLAG_ENTITIES_DDNET |
		GAMEINFOFLAG_ENTITIES_DDRACE |
		GAMEINFOFLAG_ENTITIES_RACE |
		GAMEINFOFLAG_RACE;
	pGameInfoEx->m_Flags2 = GAMEINFOFLAG2_HUD_DDRACE | GAMEINFOFLAG2_DDRACE_TEAM;
	if(g_Config.m_SvNoWeakHook)
		pGameInfoEx->m_Flags2 |= GAMEINFOFLAG2_NO_WEAK_HOOK;
	pGameInfoEx->m_Version = GAMEINFO_CURVERSION;

	if(Server()->IsSixup(SnappingClient))
	{
		protocol7::CNetObj_GameData *pGameData7 = Server()->SnapNewItem<protocol7::CNetObj_GameData>(0);
		if(!pGameData7)
			return;

		pGameData7->m_GameStartTick = m_RoundStartTick;
		pGameData7->m_GameStateFlags = 0;
		if(m_GameOverTick != -1)
			pGameData7->m_GameStateFlags |= protocol7::GAMESTATEFLAG_GAMEOVER;
		if(m_SuddenDeath)
			pGameData7->m_GameStateFlags |= protocol7::GAMESTATEFLAG_SUDDENDEATH;
		if(GameServer()->m_World.m_Paused)
			pGameData7->m_GameStateFlags |= protocol7::GAMESTATEFLAG_PAUSED;

		pGameData7->m_GameStateEndTick = 0;

		protocol7::CNetObj_GameDataTeam *pTeamData = Server()->SnapNewItem<protocol7::CNetObj_GameDataTeam>(0);
		if(!pTeamData)
			return;
		pTeamData->m_TeamscoreBlue = pGameData->m_TeamscoreBlue;
		pTeamData->m_TeamscoreRed = pGameData->m_TeamscoreRed;

		protocol7::CNetObj_GameDataRace *pRaceData = Server()->SnapNewItem<protocol7::CNetObj_GameDataRace>(0);
		if(!pRaceData)
			return;

		pRaceData->m_BestTime = round_to_int(m_CurrentRecord * 1000);
		pRaceData->m_Precision = 2;
		pRaceData->m_RaceFlags = protocol7::RACEFLAG_KEEP_WANTED_WEAPON;
	}

	GameServer()->SnapSwitchers(SnappingClient);
}

void CGameControllerMod::DoTeamChange(class CPlayer *pPlayer, int Team, bool DoChatMsg)
{
	Team = ClampTeam(Team);
	if(Team == pPlayer->GetTeam())
		return;
		
	if(m_GameType == GAMETYPE_HIDDEN && Team == TEAM_BLUE)
		return;

	int PlayerNum = 0;
	for(auto &pPlayerA : GameServer()->m_apPlayers)
	{
		if(pPlayerA)
		{
			if(pPlayerA->GetTeam() != TEAM_SPECTATORS)
				PlayerNum ++;
		}
	}

	if(PlayerNum > 2)
	{
		if(m_GameType == GAMETYPE_TEAM || m_GameType == GAMETYPE_HIDDENDEATH || m_GameType == GAMETYPE_DEATHRUN)
		{
			Team = TEAM_BLUE;
			if(Server()->Tick() >= m_RoundStartTick + g_Config.m_SvReservedTime * Server()->TickSpeed())
			{
				pPlayer->m_DeadSpec = true;
			}
		}
		else
		{
			Team = TEAM_BLUE;
			if(Server()->Tick() >= m_RoundStartTick + g_Config.m_SvReservedTime * Server()->TickSpeed())
			{
				if(m_GameType != GAMETYPE_JAIL)
					Team = TEAM_RED;
			}
		}
	}

	IGameController::DoTeamChange(pPlayer, Team, DoChatMsg);

	if(PlayerNum == 1 && Team != TEAM_SPECTATORS)
		EndRound();
}