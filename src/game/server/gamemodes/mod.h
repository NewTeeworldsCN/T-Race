#ifndef GAME_SERVER_GAMEMODES_MOD_H
#define GAME_SERVER_GAMEMODES_MOD_H

#include <game/server/gamecontroller.h>

class CGameControllerMod : public IGameController
{
	int m_TeamPlayersNum[2];
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
};
#endif // GAME_SERVER_GAMEMODES_MOD_H
