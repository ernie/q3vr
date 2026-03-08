/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// cg_consolecmds.c -- text commands typed in at the local console, or
// executed by a key binding

#include "cg_local.h"
#include "../vrcommon/vr_clientinfo.h"
#ifdef MISSIONPACK
#include "../ui/ui_shared.h"
extern menuDef_t *menuScoreboard;
#endif

extern vr_clientinfo_t *vr;



void CG_TargetCommand_f( void ) {
	int		targetNum;
	char	test[4];

	targetNum = CG_CrosshairPlayer();
	if ( targetNum == -1 ) {
		return;
	}

	trap_Argv( 1, test, 4 );
	trap_SendClientCommand( va( "gc %i %i", targetNum, atoi( test ) ) );
}



/*
=================
CG_SizeUp_f

Keybinding command
=================
*/
static void CG_SizeUp_f (void) {
	trap_Cvar_Set("cg_viewsize", va("%i",(int)(cg_viewsize.integer+10)));
}


/*
=================
CG_SizeDown_f

Keybinding command
=================
*/
static void CG_SizeDown_f (void) {
	trap_Cvar_Set("cg_viewsize", va("%i",(int)(cg_viewsize.integer-10)));
}


/*
=============
CG_Viewpos_f

Debugging command to print the current position
=============
*/
static void CG_Viewpos_f (void) {
	CG_Printf ("(%i %i %i) : %i\n", (int)cg.refdef.vieworg[0],
		(int)cg.refdef.vieworg[1], (int)cg.refdef.vieworg[2], 
		(int)cg.refdefViewAngles[YAW]);
}


static void CG_ScoresDown_f( void ) {

#ifdef MISSIONPACK
		CG_BuildSpectatorString();
#endif
	if ( cg.scoresRequestTime + 2000 < cg.time ) {
		// the scores are more than two seconds out of data,
		// so request new ones
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand( "score" );

		// leave the current scores up if they were already
		// displayed, but if this is the first hit, clear them out
		if ( !cg.showScores ) {
			cg.showScores = qtrue;
			cg.numScores = 0;
		}
	} else {
		// show the cached contents even if they just pressed if it
		// is within two seconds
		cg.showScores = qtrue;
	}

	CG_SetScoreCatcher( cg.showScores );
}

static void CG_ScoresUp_f( void ) {
	if ( cg.showScores ) {
		cg.showScores = qfalse;
		cg.scoreFadeTime = cg.time;
	}
	CG_SetScoreCatcher( qfalse );
}

#ifdef MISSIONPACK
extern menuDef_t *menuScoreboard;
void Menu_Reset( void );			// FIXME: add to right include file

static void CG_LoadHud_f( void) {
  char buff[1024];
	const char *hudSet;
  memset(buff, 0, sizeof(buff));

	String_Init();
	Menu_Reset();
	
	trap_Cvar_VariableStringBuffer("cg_hudFiles", buff, sizeof(buff));
	hudSet = buff;
	if (hudSet[0] == '\0') {
		hudSet = "ui/hud.txt";
	}

	CG_LoadMenus(hudSet);
  menuScoreboard = NULL;
}


static void CG_scrollScoresDown_f( void) {
	if (menuScoreboard && cg.scoreBoardShowing) {
		Menu_ScrollFeeder(menuScoreboard, FEEDER_SCOREBOARD, qtrue);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_REDTEAM_LIST, qtrue);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_BLUETEAM_LIST, qtrue);
	}
}


static void CG_scrollScoresUp_f( void) {
	if (menuScoreboard && cg.scoreBoardShowing) {
		Menu_ScrollFeeder(menuScoreboard, FEEDER_SCOREBOARD, qfalse);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_REDTEAM_LIST, qfalse);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_BLUETEAM_LIST, qfalse);
	}
}


static void CG_spWin_f( void) {
#if 0
	trap_Cvar_Set("cg_cameraOrbit", "2");
	trap_Cvar_Set("cg_cameraOrbitDelay", "35");
#endif
	trap_Cvar_Set("cg_thirdPerson", "1");
	trap_Cvar_Set("cg_thirdPersonAngle", "0");
	trap_Cvar_Set("cg_thirdPersonRange", "100");
	CG_AddBufferedSound(cgs.media.winnerSound);
	//trap_S_StartLocalSound(cgs.media.winnerSound, CHAN_ANNOUNCER);
	CG_CenterPrint("YOU WIN!", SCREEN_HEIGHT * .30, 0);
}

static void CG_spLose_f( void) {
#if 0
	trap_Cvar_Set("cg_cameraOrbit", "2");
	trap_Cvar_Set("cg_cameraOrbitDelay", "35");
#endif
	trap_Cvar_Set("cg_thirdPerson", "1");
	trap_Cvar_Set("cg_thirdPersonAngle", "0");
	trap_Cvar_Set("cg_thirdPersonRange", "100");
	CG_AddBufferedSound(cgs.media.loserSound);
	//trap_S_StartLocalSound(cgs.media.loserSound, CHAN_ANNOUNCER);
	CG_CenterPrint("YOU LOSE...", SCREEN_HEIGHT * .30, 0);
}

#endif

static void CG_TellTarget_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "tell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_TellAttacker_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_LastAttacker();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "tell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

#ifdef MISSIONPACK
static void CG_VoiceTellTarget_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "vtell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_VoiceTellAttacker_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_LastAttacker();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "vtell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_NextTeamMember_f( void ) {
  CG_SelectNextPlayer();
}

static void CG_PrevTeamMember_f( void ) {
  CG_SelectPrevPlayer();
}

// ASS U ME's enumeration order as far as task specific orders, OFFENSE is zero, CAMP is last
//
static void CG_NextOrder_f( void ) {
	clientInfo_t *ci = cgs.clientinfo + cg.snap->ps.clientNum;
	if (ci) {
		if (!ci->teamLeader && sortedTeamPlayers[cg_currentSelectedPlayer.integer] != cg.snap->ps.clientNum) {
			return;
		}
	}
	if (cgs.currentOrder < TEAMTASK_CAMP) {
		cgs.currentOrder++;

		if (cgs.currentOrder == TEAMTASK_RETRIEVE) {
			if (!CG_OtherTeamHasFlag()) {
				cgs.currentOrder++;
			}
		}

		if (cgs.currentOrder == TEAMTASK_ESCORT) {
			if (!CG_YourTeamHasFlag()) {
				cgs.currentOrder++;
			}
		}

	} else {
		cgs.currentOrder = TEAMTASK_OFFENSE;
	}
	cgs.orderPending = qtrue;
	cgs.orderTime = cg.time + 3000;
}


static void CG_ConfirmOrder_f (void ) {
	trap_SendConsoleCommand(va("cmd vtell %d %s\n", cgs.acceptLeader, VOICECHAT_YES));
	trap_SendConsoleCommand("+button5; wait; -button5");
	if (cg.time < cgs.acceptOrderTime) {
		trap_SendClientCommand(va("teamtask %d\n", cgs.acceptTask));
		cgs.acceptOrderTime = 0;
	}
}

static void CG_DenyOrder_f (void ) {
	trap_SendConsoleCommand(va("cmd vtell %d %s\n", cgs.acceptLeader, VOICECHAT_NO));
	trap_SendConsoleCommand("+button6; wait; -button6");
	if (cg.time < cgs.acceptOrderTime) {
		cgs.acceptOrderTime = 0;
	}
}

static void CG_TaskOffense_f (void ) {
	if (cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF) {
		trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONGETFLAG));
	} else {
		trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONOFFENSE));
	}
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_OFFENSE));
}

static void CG_TaskDefense_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONDEFENSE));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_DEFENSE));
}

static void CG_TaskPatrol_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONPATROL));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_PATROL));
}

static void CG_TaskCamp_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONCAMPING));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_CAMP));
}

static void CG_TaskFollow_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONFOLLOW));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_FOLLOW));
}

static void CG_TaskRetrieve_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONRETURNFLAG));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_RETRIEVE));
}

static void CG_TaskEscort_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONFOLLOWCARRIER));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_ESCORT));
}

static void CG_TaskOwnFlag_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_IHAVEFLAG));
}

static void CG_TauntKillInsult_f (void ) {
	trap_SendConsoleCommand("cmd vsay kill_insult\n");
}

static void CG_TauntPraise_f (void ) {
	trap_SendConsoleCommand("cmd vsay praise\n");
}

static void CG_TauntTaunt_f (void ) {
	trap_SendConsoleCommand("cmd vtaunt\n");
}

static void CG_TauntDeathInsult_f (void ) {
	trap_SendConsoleCommand("cmd vsay death_insult\n");
}

static void CG_TauntGauntlet_f (void ) {
	trap_SendConsoleCommand("cmd vsay kill_gauntlet\n");
}

static void CG_TaskSuicide_f (void ) {
	int		clientNum;
	char	command[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	Com_sprintf( command, 128, "tell %i suicide", clientNum );
	trap_SendClientCommand( command );
}



/*
==================
CG_TeamMenu_f
==================
*/
/*
static void CG_TeamMenu_f( void ) {
  if (trap_Key_GetCatcher() & KEYCATCH_CGAME) {
    CG_EventHandling(CGAME_EVENT_NONE);
    trap_Key_SetCatcher(0);
  } else {
    CG_EventHandling(CGAME_EVENT_TEAMMENU);
    //trap_Key_SetCatcher(KEYCATCH_CGAME);
  }
}
*/

/*
==================
CG_EditHud_f
==================
*/
/*
static void CG_EditHud_f( void ) {
  //cls.keyCatchers ^= KEYCATCH_CGAME;
  //VM_Call (cgvm, CG_EVENT_HANDLING, (cls.keyCatchers & KEYCATCH_CGAME) ? CGAME_EVENT_EDITHUD : CGAME_EVENT_NONE);
}
*/

#endif

/*
==================
CG_StartOrbit_f
==================
*/

static void CG_StartOrbit_f( void ) {
	char var[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer( "developer", var, sizeof( var ) );
	if ( !atoi(var) ) {
		return;
	}
	if (cg_cameraOrbit.value != 0) {
		trap_Cvar_Set ("cg_cameraOrbit", "0");
		trap_Cvar_Set("cg_thirdPerson", "0");
	} else {
		trap_Cvar_Set("cg_cameraOrbit", "5");
		trap_Cvar_Set("cg_thirdPerson", "1");
		trap_Cvar_Set("cg_thirdPersonAngle", "0");
		trap_Cvar_Set("cg_thirdPersonRange", "100");
	}
}

/*
static void CG_Camera_f( void ) {
	char name[1024];
	trap_Argv( 1, name, sizeof(name));
	if (trap_loadCamera(name)) {
		cg.cameraMode = qtrue;
		trap_startCamera(cg.time);
	} else {
		CG_Printf ("Unable to load camera %s\n",name);
	}
}
*/


static void CG_FollowNext_f( void ) {
	if ( cgs.tvPlayback ) {
		trap_SendConsoleCommand( "tv_view_next\n" );
	} else {
		trap_SendClientCommand( "follownext" );
	}
}

static void CG_FollowPrev_f( void ) {
	if ( cgs.tvPlayback ) {
		trap_SendConsoleCommand( "tv_view_prev\n" );
	} else {
		trap_SendClientCommand( "followprev" );
	}
}

static void CG_Follow_f( void ) {
	char arg[MAX_TOKEN_CHARS];

	if ( trap_Argc() < 2 ) {
		trap_SendClientCommand( "follow" );
		return;
	}
	trap_Argv( 1, arg, sizeof( arg ) );
	if ( cgs.tvPlayback ) {
		trap_SendConsoleCommand( va( "tv_view %s\n", arg ) );
	} else {
		trap_SendClientCommand( va( "follow %s", arg ) );
	}
}

static void CG_DemoPause_f( void ) {
	if ( !cg.demoPlayback && !cgs.tvPlayback ) {
		return;
	}
	if ( cg_timescale.value != 0.0f ) {
		trap_Cvar_Set( "timescale", "0" );
		trap_Cvar_Set( "cg_timescaleFadeEnd", "0" );
	} else {
		trap_Cvar_Set( "timescale", "1" );
		trap_Cvar_Set( "cg_timescaleFadeEnd", "1" );
	}
}

static void CG_FollowRecenter_f( void ) {
	if ( !cg.snap ) return;
	if ( !cg.demoPlayback && !(cg.snap->ps.pm_flags & PMF_FOLLOW) ) return;
	vr->recenter_follow_camera = qtrue;
}

static void CG_TVForward_f( void ) {
	int	ms, target;

	if ( !cgs.tvPlayback ) {
		return;
	}
	ms = cg_tvTime.integer + cg_tvSkip.integer * 1000;
	target = cg_tvDuration.integer;
	if ( ms > target ) {
		ms = target;
	}
	trap_SendConsoleCommand( va( "tv_seek %i\n", ms / 1000 ) );
}

static void CG_TVBackward_f( void ) {
	int	ms;

	if ( !cgs.tvPlayback ) {
		return;
	}
	ms = cg_tvTime.integer - cg_tvSkip.integer * 1000;
	if ( ms < 0 ) {
		ms = 0;
	}
	trap_SendConsoleCommand( va( "tv_seek %i\n", ms / 1000 ) );
}

static void CG_TVScrubDown_f( void ) {
	int		currentCatcher, newCatcher;
	int		old_state, new_state;
	float	playbackX, controllerYaw;

	if ( !cgs.tvPlayback || cg_tvDuration.integer <= 0 ) {
		return;
	}

	// Release scoreboard if held, so it doesn't stay stuck during scrub
	if ( cg.showScores ) {
		CG_ScoresUp_f();
	}

	cgs.tvScrubActive = qtrue;

	// Compute playback position in screen coordinates
	playbackX = 640.0f * (float)cg_tvTime.integer / (float)cg_tvDuration.integer;
	if ( playbackX < 0.0f ) playbackX = 0.0f;
	if ( playbackX > 640.0f ) playbackX = 640.0f;

	// Read current controller yaw (must match cursor tracking logic in vr_input.c)
	if ( vr->menuLeftHanded ) {
		controllerYaw = vr->right_handed ? vr->offhandangles[YAW] : vr->weaponangles[YAW];
	} else {
		controllerYaw = vr->right_handed ? vr->weaponangles[YAW] : vr->offhandangles[YAW];
	}

	// Save current menuYaw and adjust so cursor maps to playback position
	cgs.tvScrubSavedMenuYaw = vr->menuYaw;
	vr->menuYaw = controllerYaw - RAD2DEG( atan( (320.0f - playbackX) / 400.0f ) );
	vr->menuYawLocked = qtrue;
	cgs.cursorX = (int)playbackX;

	// Enable VR hand-pointing cursor tracking
	vr->scoreboardCursorX = &cgs.cursorX;
	vr->scoreboardCursorY = &cgs.cursorY;

	// Capture input so key events reach CG_KeyEvent
	cgs.tvScrubKey = trap_Key_GetKey( "+tv_scrub" );
	currentCatcher = trap_Key_GetCatcher();
	newCatcher = currentCatcher | KEYCATCH_CGAME;

	if ( newCatcher != currentCatcher ) {
		if ( cgs.tvScrubKey ) {
			old_state = trap_Key_IsDown( cgs.tvScrubKey );
			trap_Key_SetCatcher( newCatcher );
			new_state = trap_Key_IsDown( cgs.tvScrubKey );
			if ( new_state != old_state ) {
				cgs.tvScrubFilterKeyUp = qtrue;
			}
		} else {
			trap_Key_SetCatcher( newCatcher );
		}
	}
}

static void CG_TVScrubUp_f( void ) {
	int		currentCatcher;
	float	frac;
	int		ms;

	// Filter spurious key-up from input capture change
	if ( cgs.tvScrubFilterKeyUp ) {
		cgs.tvScrubFilterKeyUp = qfalse;
		return;
	}

	if ( !cgs.tvScrubActive ) {
		return;
	}

	cgs.tvScrubActive = qfalse;

	// Restore menuYaw
	vr->menuYawLocked = qfalse;
	vr->menuYaw = cgs.tvScrubSavedMenuYaw;

	// Convert cursor position to seek time
	frac = cgs.cursorX / 640.0f;
	if ( frac < 0.0f ) frac = 0.0f;
	if ( frac > 1.0f ) frac = 1.0f;
	ms = (int)( frac * cg_tvDuration.integer );

	// Release input capture (but preserve if scoreboard is also active)
	if ( !cgs.score_catched ) {
		vr->scoreboardCursorX = NULL;
		vr->scoreboardCursorY = NULL;
		currentCatcher = trap_Key_GetCatcher();
		trap_Key_SetCatcher( currentCatcher & ~KEYCATCH_CGAME );
	}

	// Send seek command to engine
	trap_SendConsoleCommand( va( "tv_seek %i\n", ms / 1000 ) );
}

static void CG_TVScrubCancel_f( void ) {
	int		currentCatcher;

	if ( !cgs.tvScrubActive ) {
		return;
	}

	cgs.tvScrubActive = qfalse;

	// Restore menuYaw
	vr->menuYawLocked = qfalse;
	vr->menuYaw = cgs.tvScrubSavedMenuYaw;

	// Release input capture (but preserve if scoreboard is also active)
	if ( !cgs.score_catched ) {
		vr->scoreboardCursorX = NULL;
		vr->scoreboardCursorY = NULL;
		currentCatcher = trap_Key_GetCatcher();
		trap_Key_SetCatcher( currentCatcher & ~KEYCATCH_CGAME );
	}
}


/*
==================
CG_CallVote_f / CG_Vote_f / CG_CallTeamVote_f / CG_TeamVote_f

Track local vote direction for UI highlighting, then forward
the command to the server via trap_SendClientCommand.
==================
*/
static void CG_CallVote_f( void ) {
	char args[MAX_STRING_CHARS];
	cg.myVote = 1;		// caller automatically votes yes
	trap_Args( args, sizeof( args ) );
	trap_SendClientCommand( va( "callvote %s", args ) );
}

static void CG_CallTeamVote_f( void ) {
	char args[MAX_STRING_CHARS];
	cg.myTeamVote = 1;
	trap_Args( args, sizeof( args ) );
	trap_SendClientCommand( va( "callteamvote %s", args ) );
}

qboolean CG_VoteActive( void ) {
	return cgs.voteTime && ( cg.time - cgs.voteTime < VOTE_TIME );
}

qboolean CG_TeamVoteActive( void ) {
	int cs_offset = -1;
	if ( cgs.clientinfo[ cg.clientNum ].team == TEAM_RED )
		cs_offset = 0;
	else if ( cgs.clientinfo[ cg.clientNum ].team == TEAM_BLUE )
		cs_offset = 1;
	return cs_offset >= 0
		&& cgs.teamVoteTime[cs_offset]
		&& ( cg.time - cgs.teamVoteTime[cs_offset] < VOTE_TIME );
}

qboolean CG_TVDOfferActive( void ) {
	return cg_tvdOffer.string[0] && cg.tvdOfferName[0];
}

/*
=================
CG_ActiveVoteTarget

Returns which dialog should receive the next vote input:
  0 = none active / all voted on
  1 = regular vote
  2 = team vote
TVD offer is handled separately (always takes priority).
Among vote and teamvote, picks the one expiring soonest
that the player hasn't voted on yet.
=================
*/
int CG_ActiveVoteTarget( void ) {
	int voteRemain = 0, teamRemain = 0;

	if ( CG_VoteActive() && cg.myVote == 0 )
		voteRemain = VOTE_TIME - ( cg.time - cgs.voteTime );
	if ( CG_TeamVoteActive() && cg.myTeamVote == 0 ) {
		int cs_offset = ( cgs.clientinfo[ cg.clientNum ].team == TEAM_RED ) ? 0 : 1;
		teamRemain = VOTE_TIME - ( cg.time - cgs.teamVoteTime[cs_offset] );
	}

	if ( voteRemain <= 0 && teamRemain <= 0 )
		return 0;
	if ( teamRemain > 0 && ( voteRemain <= 0 || teamRemain < voteRemain ) )
		return 2;
	return 1;
}

void CG_VoteSubmit( qboolean yes ) {
	const char *arg = yes ? "yes" : "no";
	int target;

	// TVD offer always takes priority
	if ( CG_TVDOfferActive() ) {
		trap_SendConsoleCommand( yes ? "tvdyes\n" : "tvdno\n" );
		return;
	}

	// route to the active unvoted dialog expiring soonest
	target = CG_ActiveVoteTarget();
	if ( target == 1 ) {
		cg.myVote = yes ? 1 : -1;
		trap_SendClientCommand( va( "vote %s", arg ) );
	} else if ( target == 2 ) {
		cg.myTeamVote = yes ? 1 : -1;
		trap_SendClientCommand( va( "teamvote %s", arg ) );
	} else {
		// no unvoted dialog — fall through to server
		trap_SendClientCommand( va( "vote %s", arg ) );
	}
}

static void CG_Vote_f( void ) {
	const char *arg = CG_Argv( 1 );
	qboolean yes = ( arg[0] == 'y' || arg[0] == 'Y' || arg[0] == '1' );
	CG_VoteSubmit( yes );
}

static void CG_TeamVote_f( void ) {
	if ( cg.myTeamVote == 0 ) {
		const char *arg = CG_Argv( 1 );
		if ( arg[0] == 'y' || arg[0] == 'Y' || arg[0] == '1' )
			cg.myTeamVote = 1;
		else
			cg.myTeamVote = -1;
	}
	trap_SendClientCommand( va( "teamvote %s", CG_Argv( 1 ) ) );
}


typedef struct {
	char	*cmd;
	void	(*function)(void);
} consoleCommand_t;

static consoleCommand_t	commands[] = {
	{ "testgun", CG_TestGun_f },
	{ "testmodel", CG_TestModel_f },
	{ "nextframe", CG_TestModelNextFrame_f },
	{ "prevframe", CG_TestModelPrevFrame_f },
	{ "nextskin", CG_TestModelNextSkin_f },
	{ "prevskin", CG_TestModelPrevSkin_f },
	{ "viewpos", CG_Viewpos_f },
	{ "+scores", CG_ScoresDown_f },
	{ "-scores", CG_ScoresUp_f },
	{ "+zoom", CG_ZoomDown_f },
	{ "-zoom", CG_ZoomUp_f },
	{ "sizeup", CG_SizeUp_f },
	{ "sizedown", CG_SizeDown_f },
	{ "weapnext", CG_NextWeapon_f },
	{ "weapprev", CG_PrevWeapon_f },
	{ "weapon", CG_Weapon_f },
	{ "tcmd", CG_TargetCommand_f },
	{ "tell_target", CG_TellTarget_f },
	{ "tell_attacker", CG_TellAttacker_f },
	{ "weapon_select", CG_WeaponSelectorSelect_f },
	{ "weapon_adjust", CG_WeaponAdjust_f },
	{ "weapon_adjust_reset", CG_WeaponAdjustReset_f },
	{ "weapon_adjust_reset_all", CG_WeaponAdjustResetAll_f },
#ifdef MISSIONPACK
	{ "vtell_target", CG_VoiceTellTarget_f },
	{ "vtell_attacker", CG_VoiceTellAttacker_f },
	{ "loadhud", CG_LoadHud_f },
	{ "nextTeamMember", CG_NextTeamMember_f },
	{ "prevTeamMember", CG_PrevTeamMember_f },
	{ "nextOrder", CG_NextOrder_f },
	{ "confirmOrder", CG_ConfirmOrder_f },
	{ "denyOrder", CG_DenyOrder_f },
	{ "taskOffense", CG_TaskOffense_f },
	{ "taskDefense", CG_TaskDefense_f },
	{ "taskPatrol", CG_TaskPatrol_f },
	{ "taskCamp", CG_TaskCamp_f },
	{ "taskFollow", CG_TaskFollow_f },
	{ "taskRetrieve", CG_TaskRetrieve_f },
	{ "taskEscort", CG_TaskEscort_f },
	{ "taskSuicide", CG_TaskSuicide_f },
	{ "taskOwnFlag", CG_TaskOwnFlag_f },
	{ "tauntKillInsult", CG_TauntKillInsult_f },
	{ "tauntPraise", CG_TauntPraise_f },
	{ "tauntTaunt", CG_TauntTaunt_f },
	{ "tauntDeathInsult", CG_TauntDeathInsult_f },
	{ "tauntGauntlet", CG_TauntGauntlet_f },
	{ "spWin", CG_spWin_f },
	{ "spLose", CG_spLose_f },
	{ "scoresDown", CG_scrollScoresDown_f },
	{ "scoresUp", CG_scrollScoresUp_f },
#endif
	{ "startOrbit", CG_StartOrbit_f },
	//{ "camera", CG_Camera_f },
	{ "loaddeferred", CG_LoadDeferredPlayers },
	{ "followrecenter", CG_FollowRecenter_f },
	{ "follownext", CG_FollowNext_f },
	{ "followprev", CG_FollowPrev_f },
	{ "follow", CG_Follow_f },
	{ "demopause", CG_DemoPause_f },
	{ "tv_forward", CG_TVForward_f },
	{ "tv_backward", CG_TVBackward_f },
	{ "+tv_scrub", CG_TVScrubDown_f },
	{ "-tv_scrub", CG_TVScrubUp_f },
	{ "tv_scrub_cancel", CG_TVScrubCancel_f },
	{ "callvote", CG_CallVote_f },
	{ "vote", CG_Vote_f },
	{ "callteamvote", CG_CallTeamVote_f },
	{ "teamvote", CG_TeamVote_f }
};


/*
=================
CG_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean CG_ConsoleCommand( void ) {
	const char	*cmd;
	int		i;

	cmd = CG_Argv(0);

	for ( i = 0 ; i < ARRAY_LEN( commands ) ; i++ ) {
		if ( !Q_stricmp( cmd, commands[i].cmd ) ) {
			commands[i].function();
			return qtrue;
		}
	}

	return qfalse;
}


/*
=================
CG_InitConsoleCommands

Let the client system know about all of our commands
so it can perform tab completion
=================
*/
void CG_InitConsoleCommands( void ) {
	int		i;

	for ( i = 0 ; i < ARRAY_LEN( commands ) ; i++ ) {
		trap_AddCommand( commands[i].cmd );
	}

	//
	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally
	//
	trap_AddCommand ("kill");
	trap_AddCommand ("say");
	trap_AddCommand ("say_team");
	trap_AddCommand ("tell");
#ifdef MISSIONPACK
	trap_AddCommand ("vsay");
	trap_AddCommand ("vsay_team");
	trap_AddCommand ("vtell");
	trap_AddCommand ("vtaunt");
	trap_AddCommand ("vosay");
	trap_AddCommand ("vosay_team");
	trap_AddCommand ("votell");
#endif
	trap_AddCommand ("give");
	trap_AddCommand ("god");
	trap_AddCommand ("notarget");
	trap_AddCommand ("noclip");
	trap_AddCommand ("where");
	trap_AddCommand ("team");
	trap_AddCommand ("levelshot");
	trap_AddCommand ("addbot");
	trap_AddCommand ("setviewpos");
	trap_AddCommand ("stats");
	trap_AddCommand ("teamtask");
	trap_AddCommand ("loaddefered");	// spelled wrong, but not changing for demo
}
