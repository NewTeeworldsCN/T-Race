#ifndef GAME_SERVER_GAMEMODES_MOD_H
#define GAME_SERVER_GAMEMODES_MOD_H

#include <game/server/gamecontroller.h>

class CGameControllerMod : public IGameController
{
	int m_TeamPlayersNum[2];
	int m_aPlayersJail[SERVER_MAX_CLIENTS]; // jail
	int m_WardenId; // jail
	bool m_IsJailSet;  // jail
	vec2 m_JailPos; // jail
	int m_aJailIds[10]; // jail

	int m_GameType;
	int m_Winner;
	bool m_Resetting;
public:
	CGameControllerMod(class CGameContext *pGameServer);
	~CGameControllerMod();

	CScore *Score();
	int GameType() const { return m_GameType; }

	void DoWincheck();

	const char *GetTeamName(int Team) override;
	//spawn
	bool CanSpawn(int Team, vec2 *pOutPos, int DDTeam) override;

	void HandleCharacterTiles(class CCharacter *pChr, int MapIndex) override;

	void OnPlayerConnect(class CPlayer *pPlayer) override;
	void OnPlayerDisconnect(class CPlayer *pPlayer, const char *pReason) override;

	void OnReset() override;

	void Tick() override;
	void Snap(int SnappingClient) override;

	void DoTeamChange(class CPlayer *pPlayer, int Team, bool DoChatMsg = true) override;
	bool CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize) override;
	void OnCharacterDamage(class CCharacter *pVictim, class CPlayer *pFrom, int Weapon, int Damage) override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
};
#endif // GAME_SERVER_GAMEMODES_MOD_H
