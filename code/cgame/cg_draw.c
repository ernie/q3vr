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
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"
#include "../vrcommon/vr_clientinfo.h"

extern vr_clientinfo_t *vr;

#ifdef MISSIONPACK
#include "../ui/ui_shared.h"

// used for scoreboard
extern displayContextDef_t cgDC;
menuDef_t *menuScoreboard = NULL;
#else
int drawTeamOverlayModificationCount = -1;
#endif

int sortedTeamPlayers[TEAM_MAXOVERLAY];
int	numSortedTeamPlayers;

extern vr_clientinfo_t* vr;

char systemChat[256];
char teamChat1[256];
char teamChat2[256];

#ifdef MISSIONPACK

int CG_Text_Width(const char *text, float scale, int limit) {
  int count,len;
	float out;
	glyphInfo_t *glyph;
	float useScale;
	const char *s = text;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= cg_smallFont.value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > cg_bigFont.value) {
		font = &cgDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  out = 0;
  if (text) {
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if ( Q_IsColorString(s) ) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[*s & 255];
				out += glyph->xSkip;
				s++;
				count++;
			}
    }
  }
  return out * useScale;
}

int CG_Text_Height(const char *text, float scale, int limit) {
  int len, count;
	float max;
	glyphInfo_t *glyph;
	float useScale;
	const char *s = text;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= cg_smallFont.value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > cg_bigFont.value) {
		font = &cgDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  max = 0;
  if (text) {
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if ( Q_IsColorString(s) ) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[*s & 255];
	      if (max < glyph->height) {
		      max = glyph->height;
			  }
				s++;
				count++;
			}
    }
  }
  return max * useScale;
}

void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader) {
  float w, h;
  w = width * scale;
  h = height * scale;
  CG_AdjustFrom640( &x, &y, &w, &h );
  trap_R_DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
  int len, count;
	vec4_t newColor;
	glyphInfo_t *glyph;
	float useScale;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= cg_smallFont.value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > cg_bigFont.value) {
		font = &cgDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  if (text) {
		const char *s = text;
		trap_R_SetColor( color );
		memcpy(&newColor[0], &color[0], sizeof(vec4_t));
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			glyph = &font->glyphs[*s & 255];
      //int yadj = Assets.textFont.glyphs[text[i]].bottom + Assets.textFont.glyphs[text[i]].top;
      //float yadj = scale * (Assets.textFont.glyphs[text[i]].imageHeight - Assets.textFont.glyphs[text[i]].height);
			if ( Q_IsColorString( s ) ) {
				memcpy( newColor, g_color_table[ColorIndex(*(s+1))], sizeof( newColor ) );
				newColor[3] = color[3];
				trap_R_SetColor( newColor );
				s += 2;
				continue;
			} else {
				float yadj = useScale * glyph->top;
				if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE) {
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					colorBlack[3] = newColor[3];
					trap_R_SetColor( colorBlack );
					CG_Text_PaintChar(x + ofs, y - yadj + ofs, 
														glyph->imageWidth,
														glyph->imageHeight,
														useScale, 
														glyph->s,
														glyph->t,
														glyph->s2,
														glyph->t2,
														glyph->glyph);
					colorBlack[3] = 1.0;
					trap_R_SetColor( newColor );
				}
				CG_Text_PaintChar(x, y - yadj, 
													glyph->imageWidth,
													glyph->imageHeight,
													useScale, 
													glyph->s,
													glyph->t,
													glyph->s2,
													glyph->t2,
													glyph->glyph);
				// CG_DrawPic(x, y - yadj, scale * cgDC.Assets.textFont.glyphs[text[i]].imageWidth, scale * cgDC.Assets.textFont.glyphs[text[i]].imageHeight, cgDC.Assets.textFont.glyphs[text[i]].glyph);
				x += (glyph->xSkip * useScale) + adjust;
				s++;
				count++;
			}
    }
	  trap_R_SetColor( NULL );
  }
}


#endif

/*
==============
CG_DrawField

Draws large numbers for status bar and powerups
==============
*/
#ifndef MISSIONPACK
static void CG_DrawField (int x, int y, int width, int value) {
	char	num[16], *ptr;
	int		l;
	int		frame;

	if ( width < 1 ) {
		return;
	}

	// draw number string
	if ( width > 5 ) {
		width = 5;
	}

	switch ( width ) {
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	Com_sprintf (num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
		l = width;
	x += 2 + CHAR_WIDTH*(width - l);

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		CG_DrawPic( x,y, CHAR_WIDTH, CHAR_HEIGHT, cgs.media.numberShaders[frame] );
		x += CHAR_WIDTH;
		ptr++;
		l--;
	}
}
#endif // MISSIONPACK

/*
================
CG_Draw3DModel

================
*/
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles ) {
	refdef_t		refdef;
	refEntity_t		ent;

	if ( !cg_draw3dIcons.integer || !cg_drawIcons.integer ) {
		return;
	}

	CG_SetHUDFlags(HUD_FLAGS_DRAWMODEL);
	CG_AdjustFrom640( &x, &y, &w, &h );
	CG_RemoveHUDFlags(HUD_FLAGS_DRAWMODEL);

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	ent.shaderRGBA.rgba[0] = 255;
	ent.shaderRGBA.rgba[1] = 255;
	ent.shaderRGBA.rgba[2] = 255;
	ent.shaderRGBA.rgba[3] = 255;

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	refdef.isHUD = qtrue;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene( &ent );
	trap_R_RenderScene( &refdef );
}

/*
================
CG_Draw3DModelColor

================
*/
void CG_Draw3DModelColor( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles, vec3_t color ) {
	refdef_t		refdef;
	refEntity_t		ent;

	if ( !cg_draw3dIcons.integer || !cg_drawIcons.integer ) {
		return;
	}

	CG_SetHUDFlags(HUD_FLAGS_DRAWMODEL);
	CG_AdjustFrom640( &x, &y, &w, &h );
	CG_RemoveHUDFlags(HUD_FLAGS_DRAWMODEL);

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	refdef.isHUD = qtrue;

	ent.shaderRGBA.rgba[0] = color[0] * 255;
	ent.shaderRGBA.rgba[1] = color[1] * 255;
	ent.shaderRGBA.rgba[2] = color[2] * 255;
	ent.shaderRGBA.rgba[3] = 255;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene( &ent );
	trap_R_RenderScene( &refdef );
}

/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
void CG_DrawHead( float x, float y, float w, float h, int clientNum, vec3_t headAngles ) {
	clipHandle_t	cm;
	clientInfo_t	*ci;
	float			len;
	vec3_t			origin;
	vec3_t			mins, maxs;

	ci = &cgs.clientinfo[ clientNum ];

	if ( cg_draw3dIcons.integer ) {
		cm = ci->headModel;
		if ( !cm ) {
			return;
		}

		// offset the origin y and z to center the head
		trap_R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the head nearly fills the box
		// assume heads are taller than wide
		len = 0.7 * ( maxs[2] - mins[2] );		
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		// allow per-model tweaking
		VectorAdd( origin, ci->headOffset, origin );

		CG_Draw3DModelColor( x, y, w, h, ci->headModel, ci->headSkin, origin, headAngles, ci->headColor );
	} else if ( cg_drawIcons.integer ) {
		CG_DrawPic( x, y, w, h, ci->modelIcon );
	}

	// if they are deferred, draw a cross out
	if ( ci->deferred ) {
		CG_DrawPic( x, y, w, h, cgs.media.deferShader );
	}
}

/*
================
CG_DrawFlagModel

Used for both the status bar and the scoreboard
================
*/
void CG_DrawFlagModel( float x, float y, float w, float h, int team, qboolean force2D ) {
	qhandle_t		cm;
	float			len;
	vec3_t			origin, angles;
	vec3_t			mins, maxs;
	qhandle_t		handle;

	if ( !force2D && cg_draw3dIcons.integer ) {

		VectorClear( angles );

		cm = cgs.media.redFlagModel;

		// offset the origin y and z to center the flag
		trap_R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the flag nearly fills the box
		// assume heads are taller than wide
		len = 0.5 * ( maxs[2] - mins[2] );		
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		angles[YAW] = 60 * sin( cg.time / 2000.0 );;

		if( team == TEAM_RED ) {
			handle = cgs.media.redFlagModel;
		} else if( team == TEAM_BLUE ) {
			handle = cgs.media.blueFlagModel;
		} else if( team == TEAM_FREE ) {
			handle = cgs.media.neutralFlagModel;
		} else {
			return;
		}
		CG_Draw3DModel( x, y, w, h, handle, 0, origin, angles );
	} else if ( cg_drawIcons.integer ) {
		gitem_t *item;

		if( team == TEAM_RED ) {
			item = BG_FindItemForPowerup( PW_REDFLAG );
		} else if( team == TEAM_BLUE ) {
			item = BG_FindItemForPowerup( PW_BLUEFLAG );
		} else if( team == TEAM_FREE ) {
			item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
		} else {
			return;
		}
		if (item) {
		  CG_DrawPic( x, y, w, h, cg_items[ ITEM_INDEX(item) ].icon );
		}
	}
}

/*
================
CG_DrawStatusBarHead

================
*/
#ifndef MISSIONPACK
#define STATUSBAR_HEIGHT 60
static void CG_DrawStatusBarHead( float x ) {
	vec3_t		angles;
	float		size, stretch;
	float		frac;

	VectorClear( angles );

	if ( cg.damageTime && cg.time - cg.damageTime < DAMAGE_TIME ) {
		frac = (float)(cg.time - cg.damageTime ) / DAMAGE_TIME;
		size = ICON_SIZE * 1.25 * ( 1.5 - frac * 0.5 );

		stretch = size - ICON_SIZE * 1.25;
		// kick in the direction of damage
		x -= stretch * 0.5 + cg.damageX * stretch * 0.5;

		cg.headStartYaw = 180 + cg.damageX * 45;

		cg.headEndYaw = 180 + 20 * cos( crandom()*M_PI );
		cg.headEndPitch = 5 * cos( crandom()*M_PI );

		cg.headStartTime = cg.time;
		cg.headEndTime = cg.time + 100 + random() * 2000;
	} else {
		if ( cg.time >= cg.headEndTime ) {
			// select a new head angle
			cg.headStartYaw = cg.headEndYaw;
			cg.headStartPitch = cg.headEndPitch;
			cg.headStartTime = cg.headEndTime;
			cg.headEndTime = cg.time + 100 + random() * 2000;

			cg.headEndYaw = 180 + 20 * cos( crandom()*M_PI );
			cg.headEndPitch = 5 * cos( crandom()*M_PI );
		}

		size = ICON_SIZE * 1.25;
	}

	// if the server was frozen for a while we may have a bad head start time
	if ( cg.headStartTime > cg.time ) {
		cg.headStartTime = cg.time;
	}

	frac = ( cg.time - cg.headStartTime ) / (float)( cg.headEndTime - cg.headStartTime );
	frac = frac * frac * ( 3 - 2 * frac );
	angles[YAW] = cg.headStartYaw + ( cg.headEndYaw - cg.headStartYaw ) * frac;
	angles[PITCH] = cg.headStartPitch + ( cg.headEndPitch - cg.headStartPitch ) * frac;

	CG_DrawHead( x, 480 - size, size, size, 
				cg.snap->ps.clientNum, angles );
}
#endif // MISSIONPACK

/*
================
CG_DrawStatusBarFlag

================
*/
#ifndef MISSIONPACK
static void CG_DrawStatusBarFlag( float x, int team ) {
	CG_DrawFlagModel( x, 480 - ICON_SIZE, ICON_SIZE, ICON_SIZE, team, qfalse );
}
#endif // MISSIONPACK

/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team )
{
	vec4_t		hcolor;

	hcolor[3] = alpha;
	if ( team == TEAM_RED ) {
		hcolor[0] = 1.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 0.0f;
	} else if ( team == TEAM_BLUE ) {
		hcolor[0] = 0.0f;
		hcolor[1] = 0.1f;
		hcolor[2] = 1.0f;
	} else {
		if ( !cg_drawFFABackground.integer ) {
			return;
		}
		hcolor[0] = 0.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 0.0f;
	}
	trap_R_SetColor( hcolor );
	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );
}

/*
================
CG_DrawStatusBar

================
*/
#ifndef MISSIONPACK
static void CG_DrawStatusBar( void ) {
	int			color;
	centity_t	*cent;
	playerState_t	*ps;
	int			value;
	vec4_t		hcolor;
	vec3_t		angles;
	vec3_t		origin;

	static float colors[4][4] = { 
//		{ 0.2, 1.0, 0.2, 1.0 } , { 1.0, 0.2, 0.2, 1.0 }, {0.5, 0.5, 0.5, 1} };
		{ 1.0f, 0.69f, 0.0f, 1.0f },    // normal
		{ 1.0f, 0.2f, 0.2f, 1.0f },     // low health
		{ 0.5f, 0.5f, 0.5f, 1.0f },     // weapon firing
		{ 1.0f, 1.0f, 1.0f, 1.0f } };   // health > 100

	if ( trap_Cvar_VariableValue( "vr_currentHudDrawStatus" ) == 0 ) {
		return;
	}

	// draw the team background
	CG_DrawTeamBackground( 0, 480 - STATUSBAR_HEIGHT + 1, 640, STATUSBAR_HEIGHT, 0.33f, cg.snap->ps.persistant[PERS_TEAM] );

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	VectorClear( angles );

	// draw any 3D icons first, so the changes back to 2D are minimized
	if ( cent->currentState.weapon && cg_weapons[ cent->currentState.weapon ].ammoModel ) {
		origin[0] = 70;
		origin[1] = 0;
		origin[2] = 0;
		angles[YAW] = 90 + 20 * sin( cg.time / 1000.0 );
		CG_Draw3DModel( CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE,
					   cg_weapons[ cent->currentState.weapon ].ammoModel, 0, origin, angles );
	}

	CG_DrawStatusBarHead( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE );

	if( cg.predictedPlayerState.powerups[PW_REDFLAG] ) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_RED );
	} else if( cg.predictedPlayerState.powerups[PW_BLUEFLAG] ) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_BLUE );
	} else if( cg.predictedPlayerState.powerups[PW_NEUTRALFLAG] ) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_FREE );
	}

	if ( ps->stats[ STAT_ARMOR ] ) {
		origin[0] = 90;
		origin[1] = 0;
		origin[2] = -10;
		angles[YAW] = ( cg.time & 2047 ) * 360 / 2048.0;
		CG_Draw3DModel( 370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE,
					   cgs.media.armorModel, 0, origin, angles );
	}
	//
	// ammo
	//
	if ( cent->currentState.weapon ) {
		value = ps->ammo[cent->currentState.weapon];
		if ( value > -1 ) {
			if ( cg.predictedPlayerState.weaponstate == WEAPON_FIRING
				&& cg.predictedPlayerState.weaponTime > 100 ) {
				// draw as dark grey when reloading
				color = 2;	// dark grey
			} else {
				if ( value >= 0 ) {
					color = 0;	// green
				} else {
					color = 1;	// red
				}
			}
#ifdef USE_NEW_FONT_RENDERER
			CG_SelectFont( 1 );
			CG_DrawString( CHAR_WIDTH*3, 432, va( "%i", value ), colors[ color ], CHAR_WIDTH, CHAR_HEIGHT, 0, DS_RIGHT | DS_PROPORTIONAL );
			CG_SelectFont( 0 );
#else
			trap_R_SetColor( colors[color] );
			CG_DrawField (0, 432, 3, value);
#endif
			trap_R_SetColor( NULL );

			// if we didn't draw a 3D icon, draw a 2D icon for ammo
			if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
				qhandle_t	icon;

				icon = cg_weapons[ cg.predictedPlayerState.weapon ].ammoIcon;
				if ( icon ) {
					CG_DrawPic( CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, icon );
				}
			}
		}
	}

	//
	// health
	//
	value = ps->stats[STAT_HEALTH];
	if ( value > 100 ) {
		color = 3;	// white
	} else if (value > 25) {
		color = 0;	// yellow
	} else if (value > 0) {
		color = (cg.time >> 8) & 1;	// red/yellow flashing
	} else {
		color = 1;	// red
	}

#ifdef USE_NEW_FONT_RENDERER
	CG_SelectFont( 1 );
	CG_DrawString( 185 + CHAR_WIDTH*3, 432, va( "%i", value ), colors[ color ], CHAR_WIDTH, CHAR_HEIGHT, 0, DS_RIGHT | DS_PROPORTIONAL );
	CG_SelectFont( 0 );
#else
	trap_R_SetColor( colors[color] );
	// stretch the health up when taking damage
	CG_DrawField ( 185, 432, 3, value);
#endif
	CG_ColorForHealth( hcolor );
	trap_R_SetColor( hcolor );


	//
	// armor
	//
	value = ps->stats[STAT_ARMOR];
	if (value > 0 ) {
#ifdef USE_NEW_FONT_RENDERER
		CG_SelectFont( 1 );
		CG_DrawString( 370 + CHAR_WIDTH*3, 432, va( "%i", value ), colors[ color ], CHAR_WIDTH, CHAR_HEIGHT, 0, DS_RIGHT | DS_PROPORTIONAL );
		CG_SelectFont( 0 );
#else
		trap_R_SetColor( colors[0] );
		CG_DrawField (370, 432, 3, value);
#endif
		trap_R_SetColor( NULL );
		// if we didn't draw a 3D icon, draw a 2D icon for armor
		if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
			CG_DrawPic( 370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, cgs.media.armorIcon );
		}

	}
}
#endif

/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

/*
================
CG_DrawAttacker

================
*/
static float CG_DrawAttacker( float y ) {
	int			t;
	float		size;
	vec3_t		angles;
	const char	*info;
	const char	*name;
	int			clientNum;

	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	if ( !cg.attackerTime ) {
		return y;
	}

	clientNum = cg.predictedPlayerState.persistant[PERS_ATTACKER];
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum == cg.snap->ps.clientNum ) {
		return y;
	}

	if ( !cgs.clientinfo[clientNum].infoValid ) {
		cg.attackerTime = 0;
		return y;
	}

	t = cg.time - cg.attackerTime;
	if ( t > ATTACKER_HEAD_TIME ) {
		cg.attackerTime = 0;
		return y;
	}

	size = ICON_SIZE * 1.25;

	angles[PITCH] = 0;
	angles[YAW] = 180;
	angles[ROLL] = 0;
	CG_DrawHead( 640 - size, y, size, size, clientNum, angles );

	info = CG_ConfigString( CS_PLAYERS + clientNum );
	name = Info_ValueForKey(  info, "n" );
	y += size;
	CG_DrawBigString( 640 - ( Q_PrintStrlen( name ) * BIGCHAR_WIDTH), y, name, 0.5 );

	return y + BIGCHAR_HEIGHT + 2;
}

/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot( float y ) {
	char		*s;
	int			w;

	s = va( "time:%i snap:%i cmd:%i", cg.snap->serverTime, 
		cg.latestSnapshotNum, cgs.serverCommandSequence );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( 635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
CG_DrawFPS
==================
*/
#define	FPS_FRAMES	4
static float CG_DrawFPS( float y ) {
	char		*s;
	int			w;
	static int	previousTimes[FPS_FRAMES];
	static int	index;
	int		i, total;
	int		fps;
	static	int	previous;
	int		t, frameTime;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;
	if ( index > FPS_FRAMES ) {
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0 ; i < FPS_FRAMES ; i++ ) {
			total += previousTimes[i];
		}
		if ( !total ) {
			total = 1;
		}
		fps = 1000 * FPS_FRAMES / total;

		s = va( "%ifps", fps );
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

		CG_DrawBigString( 635 - w, y + 2, s, 1.0F);
	}

	return y + BIGCHAR_HEIGHT + 4;
}

/*
=================
CG_DrawSpeedMeter
=================
*/
static float CG_DrawSpeedMeter( float y ) {
	char		*s;
	int			w;

	/* speed meter can get in the way of the scoreboard */
	if ( cg.scoreBoardShowing ) {
		return y;
	}

	s = va( "%1.0fups", cg.xyspeed );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	if ( cg_drawSpeed.integer == 1 ) {
		/* top right corner of screen */
		CG_DrawBigString( 635 - w, y + 2, s, 1.0F);
		return y + BIGCHAR_HEIGHT + 4;
	} else {
		/* center of screen (under crosshair) */
		CG_DrawBigString( 320 - w / 2, 300, s, 1.0F);
		return y;
	}
}

/*
=================
CG_DrawTimer
=================
*/
static float CG_DrawTimer( float y ) {
	const char	*s;
	int			w;
	int			mins, seconds;
	int			msec;
	vec4_t		color;

	msec = cg.time - cgs.levelStartTime;
	Vector4Copy( colorWhite, color );

	if ( cg.warmup > 0 ) {
		// warmup: count down to match start
		int remaining = cg.warmup - cg.time;
		if ( remaining < 0 ) remaining = 0;
		seconds = ( remaining + 999 ) / 1000;
		mins = seconds / 60;
		seconds -= mins * 60;
		s = va( "%i:%02i", mins, seconds );
	} else if ( cgs.timelimit > 0 && !cg.warmup ) {
		int timelimitMsec = cgs.timelimit * 60 * 1000;
		int overtimeElapsed = msec - timelimitMsec;

		if ( overtimeElapsed > 0 ) {
			if ( cgs.overtimelimit > 0 ) {
				// overtime with limit: count down
				int remaining = ( cgs.overtimelimit * 60 * 1000 ) - overtimeElapsed;
				if ( remaining < 0 ) remaining = 0;
				seconds = ( remaining + 999 ) / 1000;
				mins = seconds / 60;
				seconds -= mins * 60;
				s = va( "OT %i:%02i", mins, seconds );
				if ( remaining < 30000 ) {
					Vector4Copy( ( cg.time / 500 ) & 1 ? colorRed : colorWhite, color );
				} else {
					Vector4Copy( colorYellow, color );
				}
			} else {
				// unlimited overtime: count up from OT start
				seconds = overtimeElapsed / 1000;
				mins = seconds / 60;
				seconds -= mins * 60;
				s = va( "OT %i:%02i", mins, seconds );
				Vector4Copy( colorYellow, color );
			}
		} else {
			// regulation: count down to timelimit
			int remaining = timelimitMsec - msec;
			if ( remaining < 0 ) remaining = 0;
			seconds = ( remaining + 999 ) / 1000;
			mins = seconds / 60;
			seconds -= mins * 60;
			s = va( "%i:%02i", mins, seconds );
		}
	} else {
		// no timelimit: count up
		seconds = msec / 1000;
		mins = seconds / 60;
		seconds -= mins * 60;
		s = va( "%i:%02i", mins, seconds );
	}

	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigStringColor( 635 - w, y + 2, s, color );

	return y + BIGCHAR_HEIGHT + 4;
}


/*
=================
CG_DrawTeamOverlay
=================
*/

static float CG_DrawTeamOverlay( float y, qboolean right, qboolean upper ) {
	int x, w, h, xx;
	int i, j, len;
	const char *p;
	vec4_t		hcolor;
	int pwidth, lwidth;
	int plyrs;
	char st[16];
	clientInfo_t *ci;
	gitem_t	*item;
	int ret_y, count;

	if ( !cg_drawTeamOverlay.integer ) {
		return y;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] != TEAM_RED && cg.snap->ps.persistant[PERS_TEAM] != TEAM_BLUE ) {
		return y; // Not on any team
	}

	plyrs = 0;

	// max player name width
	pwidth = 0;
	count = (numSortedTeamPlayers > 8) ? 8 : numSortedTeamPlayers;
	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {
			plyrs++;
			len = CG_DrawStrlen(ci->name);
			if (len > pwidth)
				pwidth = len;
		}
	}

	if (!plyrs)
		return y;

	if (pwidth > TEAM_OVERLAY_MAXNAME_WIDTH)
		pwidth = TEAM_OVERLAY_MAXNAME_WIDTH;

	// max location name width
	lwidth = 0;
	for (i = 1; i < MAX_LOCATIONS; i++) {
		p = CG_ConfigString(CS_LOCATIONS + i);
		if (p && *p) {
			len = CG_DrawStrlen(p);
			if (len > lwidth)
				lwidth = len;
		}
	}

	if (lwidth > TEAM_OVERLAY_MAXLOCATION_WIDTH)
		lwidth = TEAM_OVERLAY_MAXLOCATION_WIDTH;

	w = (pwidth + lwidth + 4 + 7) * TINYCHAR_WIDTH;

	if ( right )
		x = 640 - w;
	else
		x = 0;

	h = plyrs * TINYCHAR_HEIGHT;

	if ( upper ) {
		ret_y = y + h;
	} else {
		y -= h;
		ret_y = y;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
		hcolor[0] = 1.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 0.0f;
		hcolor[3] = 0.33f;
	} else { // if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )
		hcolor[0] = 0.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 1.0f;
		hcolor[3] = 0.33f;
	}
	trap_R_SetColor( hcolor );
	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );

	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {

			hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1.0;

			xx = x + TINYCHAR_WIDTH;

			CG_DrawStringExt( xx, y,
				ci->name, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, TEAM_OVERLAY_MAXNAME_WIDTH);

			if (lwidth) {
				p = CG_ConfigString(CS_LOCATIONS + ci->location);
				if (!p || !*p)
					p = "unknown";
//				len = CG_DrawStrlen(p);
//				if (len > lwidth)
//					len = lwidth;

//				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth + 
//					((lwidth/2 - len/2) * TINYCHAR_WIDTH);
				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth;
				CG_DrawStringExt( xx, y,
					p, hcolor, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					TEAM_OVERLAY_MAXLOCATION_WIDTH);
			}

			CG_GetColorForHealth( ci->health, ci->armor, hcolor );

			Com_sprintf (st, sizeof(st), "%3i %3i", ci->health,	ci->armor);

			xx = x + TINYCHAR_WIDTH * 3 + 
				TINYCHAR_WIDTH * pwidth + TINYCHAR_WIDTH * lwidth;

			CG_DrawStringExt( xx, y,
				st, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );

			// draw weapon icon
			xx += TINYCHAR_WIDTH * 3;

			if ( cg_weapons[ci->curWeapon].weaponIcon ) {
				CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 
					cg_weapons[ci->curWeapon].weaponIcon );
			} else {
				CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 
					cgs.media.deferShader );
			}

			// Draw powerup icons
			if (right) {
				xx = x;
			} else {
				xx = x + w - TINYCHAR_WIDTH;
			}
			for (j = 0; j <= PW_NUM_POWERUPS; j++) {
				if (ci->powerups & (1 << j)) {

					item = BG_FindItemForPowerup( j );

					if (item) {
						CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 
						trap_R_RegisterShader( item->icon ) );
						if (right) {
							xx -= TINYCHAR_WIDTH;
						} else {
							xx += TINYCHAR_WIDTH;
						}
					}
				}
			}

			y += TINYCHAR_HEIGHT;
		}
	}

	return ret_y;
//#endif
}


/*
=================
CG_DrawVRFollowIcon

Shows VR icon in upper-right when following a VR player.
=================
*/
static float CG_DrawVRFollowIcon( float y ) {
	float	size = (BIGCHAR_HEIGHT + 4) * 2;

	CG_DrawPic( SCREEN_WIDTH - size - 2, y, size, size, cgs.media.vrPlayerShader );

	return y + size;
}

/*
=====================
CG_DrawUpperRight

=====================
*/
static void CG_DrawUpperRight( void )
{
	float	y;

	y = 0;

	if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 1 ) {
		y = CG_DrawTeamOverlay( y, qtrue, qtrue );
	} 
	if ( cg_drawSnapshot.integer ) {
		y = CG_DrawSnapshot( y );
	}
	if (cg_drawFPS.integer && (cg.stereoView == STEREO_CENTER || cg.stereoView == STEREO_RIGHT)) {
		y = CG_DrawFPS( y );
	}
	if ( cg_drawSpeed.integer ) {
		y = CG_DrawSpeedMeter( y );
	}
	if ( cg_drawTimer.integer ) {
		y = CG_DrawTimer( y );
	}
	if ( cg_drawAttacker.integer ) {
		y = CG_DrawAttacker( y );
	}
	if ( CG_IsVRFollow() ) {
		y = CG_DrawVRFollowIcon( y );
	}

}

/*
===========================================================================================

  LOWER RIGHT CORNER

===========================================================================================
*/

/*
=================
CG_DrawScores

Draw the small two score display
=================
*/
#ifndef MISSIONPACK
static float CG_DrawScores( float y ) {
	const char	*s;
	int			s1, s2, score;
	int			x, w;
	int			v;
	vec4_t		color;
	float		y1;
	gitem_t		*item;

	s1 = cgs.scores1;
	s2 = cgs.scores2;

	y -=  BIGCHAR_HEIGHT + 4;

	y1 = y;

	// draw from the right side to left
	if ( cgs.gametype >= GT_TEAM ) {
		x = 640;
		color[0] = 0.0f;
		color[1] = 0.0f;
		color[2] = 1.0f;
		color[3] = 0.33f;
		s = va( "%2i", s2 );
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
		x -= w;
		CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
			CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
		}
		CG_DrawBigString( x + 4, y, s, 1.0F);

		if ( cgs.gametype == GT_CTF ) {
			// Display flag status
			item = BG_FindItemForPowerup( PW_BLUEFLAG );

			if (item) {
				y1 = y - BIGCHAR_HEIGHT - 8;
				if( cgs.blueflag >= 0 && cgs.blueflag <= 2 ) {
					CG_DrawPic( x, y1-4, w, BIGCHAR_HEIGHT+8, cgs.media.blueFlagShader[cgs.blueflag] );
				}
			}
		}
		color[0] = 1.0f;
		color[1] = 0.0f;
		color[2] = 0.0f;
		color[3] = 0.33f;
		s = va( "%2i", s1 );
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
		x -= w;
		CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
			CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
		}
		CG_DrawBigString( x + 4, y, s, 1.0F);

		if ( cgs.gametype == GT_CTF ) {
			// Display flag status
			item = BG_FindItemForPowerup( PW_REDFLAG );

			if (item) {
				y1 = y - BIGCHAR_HEIGHT - 8;
				if( cgs.redflag >= 0 && cgs.redflag <= 2 ) {
					CG_DrawPic( x, y1-4, w, BIGCHAR_HEIGHT+8, cgs.media.redFlagShader[cgs.redflag] );
				}
			}
		}

		if ( cgs.gametype >= GT_CTF ) {
			v = cgs.capturelimit;
		} else {
			v = cgs.fraglimit;
		}
		if ( v ) {
			s = va( "%2i", v );
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
			x -= w;
			CG_DrawBigString( x + 4, y, s, 1.0F);
		}

	} else {
		qboolean	spectator;

		x = 640;
		score = cg.snap->ps.persistant[PERS_SCORE];
		spectator = ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR );

		// always show your score in the second box if not in first place
		if ( s1 != score ) {
			s2 = score;
		}
		if ( s2 != SCORE_NOT_PRESENT ) {
			s = va( "%2i", s2 );
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
			x -= w;
			if ( !spectator && score == s2 && score != s1 ) {
				color[0] = 1.0f;
				color[1] = 0.0f;
				color[2] = 0.0f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
				CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
			} else {
				color[0] = 0.5f;
				color[1] = 0.5f;
				color[2] = 0.5f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
			}	
			CG_DrawBigString( x + 4, y, s, 1.0F);
		}

		// first place
		if ( s1 != SCORE_NOT_PRESENT ) {
			s = va( "%2i", s1 );
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
			x -= w;
			if ( !spectator && score == s1 ) {
				color[0] = 0.0f;
				color[1] = 0.0f;
				color[2] = 1.0f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
				CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
			} else {
				color[0] = 0.5f;
				color[1] = 0.5f;
				color[2] = 0.5f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
			}	
			CG_DrawBigString( x + 4, y, s, 1.0F);
		}

		if ( cgs.fraglimit ) {
			s = va( "%2i", cgs.fraglimit );
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
			x -= w;
			CG_DrawBigString( x + 4, y, s, 1.0F);
		}

	}

	return y1 - 8;
}
#endif // MISSIONPACK

/*
================
CG_DrawPowerups
================
*/
#ifndef MISSIONPACK
static float CG_DrawPowerups( float y ) {
	int		sorted[MAX_POWERUPS];
	int		sortedTime[MAX_POWERUPS];
	int		i, j, k;
	int		active;
	playerState_t	*ps;
	int		t;
	gitem_t	*item;
	int		x;
	int		color;
	float	size;
	float	f;
	static float colors[2][4] = { 
    { 0.2f, 1.0f, 0.2f, 1.0f } , 
    { 1.0f, 0.2f, 0.2f, 1.0f } 
  };

	ps = &cg.snap->ps;

	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	// sort the list by time remaining
	active = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( !ps->powerups[ i ] ) {
			continue;
		}

		// ZOID--don't draw if the power up has unlimited time
		// This is true of the CTF flags
		if ( ps->powerups[ i ] == INT_MAX ) {
			continue;
		}

		t = ps->powerups[ i ] - cg.time;
		if ( t <= 0 ) {
			continue;
		}

		// insert into the list
		for ( j = 0 ; j < active ; j++ ) {
			if ( sortedTime[j] >= t ) {
				for ( k = active - 1 ; k >= j ; k-- ) {
					sorted[k+1] = sorted[k];
					sortedTime[k+1] = sortedTime[k];
				}
				break;
			}
		}
		sorted[j] = i;
		sortedTime[j] = t;
		active++;
	}

	// draw the icons and timers
	x = 640 - ICON_SIZE - CHAR_WIDTH * 2;
	for ( i = 0 ; i < active ; i++ ) {
		item = BG_FindItemForPowerup( sorted[i] );

    if (item) {

		  color = 1;

		  y -= ICON_SIZE;

		  trap_R_SetColor( colors[color] );
		  CG_DrawField( x, y, 2, sortedTime[ i ] / 1000 );

		  t = ps->powerups[ sorted[i] ];
		  if ( t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME ) {
			  trap_R_SetColor( NULL );
		  } else {
			  vec4_t	modulate;

			  f = (float)( t - cg.time ) / POWERUP_BLINK_TIME;
			  f -= (int)f;
			  modulate[0] = modulate[1] = modulate[2] = modulate[3] = f;
			  trap_R_SetColor( modulate );
		  }

		  if ( cg.powerupActive == sorted[i] && 
			  cg.time - cg.powerupTime < PULSE_TIME ) {
			  f = 1.0 - ( ( (float)cg.time - cg.powerupTime ) / PULSE_TIME );
			  size = ICON_SIZE * ( 1.0 + ( PULSE_SCALE - 1.0 ) * f );
		  } else {
			  size = ICON_SIZE;
		  }

		  CG_DrawPic( 640 - size, y + ICON_SIZE / 2 - size / 2, 
			  size, size, trap_R_RegisterShader( item->icon ) );
    }
	}
	trap_R_SetColor( NULL );

	return y;
}
#endif // MISSIONPACK

/*
=====================
CG_DrawLowerRight

=====================
*/
#ifndef MISSIONPACK
static void CG_DrawLowerRight( void ) {
	float	y;

	y = 480 - STATUSBAR_HEIGHT;

	if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 2 ) {
		y = CG_DrawTeamOverlay( y, qtrue, qfalse );
	} 

	y = CG_DrawScores( y );
	CG_DrawPowerups( y );
}
#endif // MISSIONPACK

/*
===================
CG_DrawPickupItem
===================
*/
#ifndef MISSIONPACK
static int CG_DrawPickupItem( int y ) {
	int		value;
	float	*fadeColor;

	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	y -= ICON_SIZE + 12;

	value = cg.itemPickup;
	if ( value ) {
		fadeColor = CG_FadeColor( cg.itemPickupTime, 3000 );
		if ( fadeColor ) {
			CG_RegisterItemVisuals( value );
			trap_R_SetColor( fadeColor );
			CG_DrawPic( 0, y, ICON_SIZE, ICON_SIZE, cg_items[ value ].icon );
			CG_DrawBigString( ICON_SIZE + 8, y + (ICON_SIZE/2 - BIGCHAR_HEIGHT/2), bg_itemlist[ value ].pickup_name, fadeColor[0] );
			trap_R_SetColor( NULL );
		}
	}
	
	return y;
}
#endif // MISSIONPACK

/*
=====================
CG_DrawLowerLeft

=====================
*/
#ifndef MISSIONPACK
static void CG_DrawLowerLeft( void ) {
	float	y;

	y = 480 - ICON_SIZE;

	if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 3 ) {
		y = CG_DrawTeamOverlay( y, qfalse, qfalse );
	} 


	CG_DrawPickupItem( y );
}
#endif // MISSIONPACK


//===========================================================================================

/*
=================
CG_DrawTeamInfo
=================
*/
#ifndef MISSIONPACK
static void CG_DrawTeamInfo( void ) {
	int h;
	int i;
	vec4_t		hcolor;
	int		chatHeight;

#define CHATLOC_Y 420 // bottom end
#define CHATLOC_X 0

	if (cg_teamChatHeight.integer < TEAMCHAT_HEIGHT)
		chatHeight = cg_teamChatHeight.integer;
	else
		chatHeight = TEAMCHAT_HEIGHT;
	if (chatHeight <= 0)
		return; // disabled

	if (cgs.teamLastChatPos != cgs.teamChatPos) {
		if (cg.time - cgs.teamChatMsgTimes[cgs.teamLastChatPos % chatHeight] > cg_teamChatTime.integer) {
			cgs.teamLastChatPos++;
		}

		h = (cgs.teamChatPos - cgs.teamLastChatPos) * TINYCHAR_HEIGHT;

		if ( cgs.clientinfo[cg.clientNum].team == TEAM_RED ) {
			hcolor[0] = 1.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		} else if ( cgs.clientinfo[cg.clientNum].team == TEAM_BLUE ) {
			hcolor[0] = 0.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 1.0f;
			hcolor[3] = 0.33f;
		} else {
			hcolor[0] = 0.0f;
			hcolor[1] = 1.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		}

		trap_R_SetColor( hcolor );
		CG_DrawPic( CHATLOC_X, CHATLOC_Y - h, 640, h, cgs.media.teamStatusBar );
		trap_R_SetColor( NULL );

		hcolor[0] = hcolor[1] = hcolor[2] = 1.0f;
		hcolor[3] = 1.0f;

		for (i = cgs.teamChatPos - 1; i >= cgs.teamLastChatPos; i--) {
			CG_DrawStringExt( CHATLOC_X + TINYCHAR_WIDTH, 
				CHATLOC_Y - (cgs.teamChatPos - i)*TINYCHAR_HEIGHT, 
				cgs.teamChatMsgs[i % chatHeight], hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );
		}
	}
}
#endif // MISSIONPACK

/*
===================
CG_DrawHoldableItem
===================
*/
#ifndef MISSIONPACK
static void CG_DrawHoldableItem( void ) { 
	int		value;

	value = cg.snap->ps.stats[STAT_HOLDABLE_ITEM];
	if ( value ) {
		CG_RegisterItemVisuals( value );

		//If we are two handing the weapon or show in hand not enabled, move the item icon back to the HUD
		qboolean show_in_hand_enabled = toQBoolean(trap_Cvar_VariableValue( "vr_showItemInHand" ) != 0.0f);
		qboolean two_handed_enabled = toQBoolean(trap_Cvar_VariableValue("vr_twoHandedWeapons") != 0.0f);
		if (!show_in_hand_enabled || (two_handed_enabled && vr->weapon_stabilised))
		{
			CG_DrawPic(640 - ICON_SIZE, (SCREEN_HEIGHT - ICON_SIZE) / 2, ICON_SIZE, ICON_SIZE, cg_items[value].icon);
		}
	}

}
#endif // MISSIONPACK

#ifdef MISSIONPACK
/*
===================
CG_DrawPersistantPowerup
===================
*/
#if 0 // sos001208 - DEAD
static void CG_DrawPersistantPowerup( void ) { 
	int		value;

	value = cg.snap->ps.stats[STAT_PERSISTANT_POWERUP];
	if ( value ) {
		CG_RegisterItemVisuals( value );
		CG_DrawPic( 640-ICON_SIZE, (SCREEN_HEIGHT-ICON_SIZE)/2 - ICON_SIZE, ICON_SIZE, ICON_SIZE, cg_items[ value ].icon );
	}
}
#endif
#endif // MISSIONPACK


/*
===================
CG_DrawReward
===================
*/
static void CG_DrawReward( void ) { 
	float	*color;
	int		i, count;
	float	x, y;
	char	buf[32];

	if ( !cg_drawRewards.integer ) {
		return;
	}

	color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
	if ( !color ) {
		if (cg.rewardStack > 0) {
			for(i = 0; i < cg.rewardStack; i++) {
				cg.rewardSound[i] = cg.rewardSound[i+1];
				cg.rewardShader[i] = cg.rewardShader[i+1];
				cg.rewardCount[i] = cg.rewardCount[i+1];
			}
			cg.rewardTime = cg.time;
			cg.rewardStack--;
			color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
			trap_S_StartLocalSound(cg.rewardSound[0], CHAN_ANNOUNCER);
		} else {
			return;
		}
	}

	trap_R_SetColor( color );

	/*
	count = cg.rewardCount[0]/10;				// number of big rewards to draw

	if (count) {
		y = 4;
		x = 320 - count * ICON_SIZE;
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x, y, (ICON_SIZE*2)-4, (ICON_SIZE*2)-4, cg.rewardShader[0] );
			x += (ICON_SIZE*2);
		}
	}

	count = cg.rewardCount[0] - count*10;		// number of small rewards to draw
	*/

	if ( cg.rewardCount[0] >= 10 ) {
		y = 56;
		x = 320 - ICON_SIZE/2;
		CG_DrawPic( x, y, ICON_SIZE-4, ICON_SIZE-4, cg.rewardShader[0] );
		Com_sprintf(buf, sizeof(buf), "%d", cg.rewardCount[0]);
		x = ( SCREEN_WIDTH - SMALLCHAR_WIDTH * CG_DrawStrlen( buf ) ) / 2;
		CG_DrawStringExt( x, y+ICON_SIZE, buf, color, qfalse, qtrue,
								SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
	}
	else {

		count = cg.rewardCount[0];

		y = 56;
		x = 320 - count * ICON_SIZE/2;
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x, y, ICON_SIZE-4, ICON_SIZE-4, cg.rewardShader[0] );
			x += ICON_SIZE;
		}
	}
	trap_R_SetColor( NULL );
}


/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define	LAG_SAMPLES		128


typedef struct {
	int		frameSamples[LAG_SAMPLES];
	int		frameCount;
	int		snapshotFlags[LAG_SAMPLES];
	int		snapshotSamples[LAG_SAMPLES];
	int		snapshotCount;
} lagometer_t;

lagometer_t		lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo( void ) {
	int			offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & ( LAG_SAMPLES - 1) ] = offset;
	lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo( snapshot_t *snap ) {
	// dropped packet
	if ( !snap ) {
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->ping;
	lagometer.snapshotFlags[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect( void ) {
	float		x, y;
	int			cmdNum;
	usercmd_t	cmd;
	const char		*s;
	int			w;

	// don't show during paused demo/TV playback
	if ( ( cg.demoPlayback || cgs.tvPlayback ) && cg_timescale.value == 0.0f ) {
		return;
	}

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &cmd );
	if ( cmd.serverTime <= cg.snap->ps.commandTime
		|| cmd.serverTime > cg.time ) {	// special check for map_restart
		return;
	}

	// also add text in center of screen
	s = "Connection Interrupted";
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString( 320 - w/2, 100, s, 1.0F);

	// blink the icon
	if ( ( cg.time >> 9 ) & 1 ) {
		return;
	}

#ifdef MISSIONPACK
	x = 640 - 48;
	y = 480 - 144;
#else
	x = 640 - 48;
	y = 480 - 48;
#endif

	CG_DrawPic( x, y, 48, 48, trap_R_RegisterShader("gfx/2d/net.tga" ) );
}


#define	MAX_LAGOMETER_PING	900
#define	MAX_LAGOMETER_RANGE	300

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer( void ) {
	int		a, x, y, i;
	float	v;
	float	ax, ay, aw, ah, mid, range;
	int		color;
	float	vscale;

	if ( !cg_lagometer.integer || cgs.localServer ) {
		CG_DrawDisconnect();
		return;
	}

	//
	// draw the graph
	//
#ifdef MISSIONPACK
	x = 640 - 48;
	y = 480 - 144;
#else
	x = 640 - 48;
	y = 480 - 48;
#endif

	trap_R_SetColor( NULL );
	CG_DrawPic( x, y, 48, 48, cgs.media.lagometerShader );

	ax = x;
	ay = y;
	aw = 48;
	ah = 48;
	CG_AdjustFrom640( &ax, &ay, &aw, &ah );

	color = -1;
	range = ah / 3;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.frameCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.frameSamples[i];
		v *= vscale;
		if ( v > 0 ) {
			if ( color != 1 ) {
				color = 1;
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
			}
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic ( ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 2 ) {
				color = 2;
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_BLUE)] );
			}
			v = -v;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.snapshotCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.snapshotSamples[i];
		if ( v > 0 ) {
			if ( lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED ) {
				if ( color != 5 ) {
					color = 5;	// YELLOW for rate delay
					trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
				}
			} else {
				if ( color != 3 ) {
					color = 3;
					trap_R_SetColor( g_color_table[ColorIndex(COLOR_GREEN)] );
				}
			}
			v = v * vscale;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 4 ) {
				color = 4;		// RED for dropped snapshots
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_RED)] );
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	trap_R_SetColor( NULL );

	if ( cg_nopredict.integer || cg_synchronousClients.integer ) {
		CG_DrawBigString( x, y, "snc", 1.0 );
	}

	if ( !cg.demoPlayback ) {
		CG_DrawString( x+1, y, va( "%ims", cg.meanPing ), colorWhite, 5, 10, 0, DS_PROPORTIONAL );
	}

	CG_DrawDisconnect();
}



/*
===============================================================================

CENTER PRINTING

===============================================================================
*/


/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint( const char *str, int y, int charWidth ) {
	char	*s;

	Q_strncpyz( cg.centerPrint, str, sizeof(cg.centerPrint) );

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while( *s ) {
		if (*s == '\n')
			cg.centerPrintLines++;
		s++;
	}
}


/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString( void ) {
	char	*start;
	int		l;
	int		x, y, w;
#ifdef MISSIONPACK
	int h;
#endif
	float	*color;

	if ( !cg.centerPrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.centerPrintTime, 1000 * cg_centertime.value );
	if ( !color ) {
		return;
	}

	trap_R_SetColor( color );

	start = cg.centerPrint;

	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < 50; l++ ) {
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

#ifdef MISSIONPACK
		w = CG_Text_Width(linebuffer, 0.5, 0);
		h = CG_Text_Height(linebuffer, 0.5, 0);
		x = (SCREEN_WIDTH - w) / 2;
		CG_Text_Paint(x, y + h, 0.5, color, linebuffer, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
		y += h + 6;
#else
		w = cg.centerPrintCharWidth * CG_DrawStrlen( linebuffer );

		x = ( SCREEN_WIDTH - w ) / 2;

		CG_DrawStringExt( x, y, linebuffer, color, qfalse, qtrue,
			cg.centerPrintCharWidth, (int)(cg.centerPrintCharWidth * 1.5), 0 );

		y += cg.centerPrintCharWidth * 1.5;
#endif
		while ( *start && ( *start != '\n' ) ) {
			start++;
		}
		if ( !*start ) {
			break;
		}
		start++;
	}

	trap_R_SetColor( NULL );
}



/*
================================================================================

CROSSHAIR

================================================================================
*/


/*
=================
CG_DrawCrosshair
=================
*/
#if 0
static void CG_DrawCrosshair(void)
{
	float		w, h;
	qhandle_t	hShader;
	float		f;
	float		x, y;
	int			ca;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	if ( cg.renderingThirdPerson ) {
		return;
	}

	// set color based on health
	if ( cg_crosshairHealth.integer ) {
		vec4_t		hcolor;

		CG_ColorForHealth( hcolor );
		trap_R_SetColor( hcolor );
	} else {
		trap_R_SetColor( NULL );
	}

	w = h = cg_crosshairSize.value;

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.itemPickupBlendTime;
	if ( f > 0 && f < ITEM_BLOB_TIME ) {
		f /= ITEM_BLOB_TIME;
		w *= ( 1 + f );
		h *= ( 1 + f );
	}

	x = cg_crosshairX.integer;
	y = cg_crosshairY.integer;
	CG_AdjustFrom640( &x, &y, &w, &h );

	ca = cg_drawCrosshair.integer;
	if (ca < 0) {
		ca = 0;
	}
	hShader = cgs.media.crosshairShader[ ca % NUM_CROSSHAIRS ];

	trap_R_DrawStretchPic( x + cg.refdef.x + 0.5 * (cg.refdef.width - w), 
		y + cg.refdef.y + 0.5 * (cg.refdef.height - h), 
		w, h, 0, 0, 1, 1, hShader );

	trap_R_SetColor( NULL );
}
#endif

/*
=================
CG_CrosshairColorFromInt
=================
*/
static void CG_CrosshairColorFromInt( int val, byte *color ) {
	if ( val < 1 || val > 7 ) {
		// Default to white
		color[0] = 255;
		color[1] = 255;
		color[2] = 255;
	} else {
		color[0] = (val & 4) ? 255 : 0;
		color[1] = (val & 2) ? 255 : 0;
		color[2] = (val & 1) ? 255 : 0;
	}
	color[3] = 255;
}

/*
=================
CG_DrawCrosshair3D
=================
*/
static void CG_DrawCrosshair3D(void)
{
	float		w;
	qhandle_t	hShader;
	float		f;
	int			ca;

	trace_t trace;
	vec3_t endpos;
	float stereoSep, zProj, maxdist, xmax;
	char rendererinfos[128];
	refEntity_t ent;

	if ( !cg_drawCrosshair.integer || vr->no_crosshair ) {
		return;
	}

	if (cg.snap->ps.pm_type == PM_INTERMISSION)
	{
		return;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	if ( cg.renderingThirdPerson || CG_IsDeathCam()) {
		return;
	}

	w = cg_crosshairSize.value;

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.itemPickupBlendTime;
	if ( f > 0 && f < ITEM_BLOB_TIME ) {
		f /= ITEM_BLOB_TIME;
		w *= ( 1 + f );
	}

	ca = cg_drawCrosshair.integer;
	if (ca < 0) {
		ca = 0;
	}
	hShader = cgs.media.crosshairShader[ ca % NUM_CROSSHAIRS ];

	// Use a different method rendering the crosshair so players don't see two of them when
	// focusing their eyes at distant objects with high stereo separation
	// We are going to trace to the next shootable object and place the crosshair in front of it.

	// first get all the important renderer information
	trap_Cvar_VariableStringBuffer("r_zProj", rendererinfos, sizeof(rendererinfos));
	zProj = atof(rendererinfos);
	trap_Cvar_VariableStringBuffer("r_stereoSeparation", rendererinfos, sizeof(rendererinfos));
	stereoSep = zProj / atof(rendererinfos);
	
	xmax = zProj * tan(cg.refdef.fov_x * M_PI / 360.0f);
	
	// let the trace run through until a change in stereo separation of the crosshair becomes less than one pixel.
	vec3_t viewaxis[3];
	vec3_t weaponangles;
	vec3_t origin;
	CG_CalculateVRWeaponPosition(origin, weaponangles);
	AnglesToAxis(weaponangles, viewaxis);
	maxdist = (cgs.glconfig.vidWidth * stereoSep * zProj / (2 * xmax)) * 1.5f;
	VectorMA(origin, maxdist, viewaxis[0], endpos);
	CG_Trace(&trace, origin, NULL, NULL, endpos, 0, MASK_SHOT);
#if 0
	maxdist = cgs.glconfig.vidWidth * stereoSep * zProj / (2 * xmax);
	VectorMA(cg.refdef.vieworg, maxdist, cg.refdef.viewaxis[0], endpos);
	CG_Trace(&trace, cg.refdef.vieworg, NULL, NULL, endpos, 0, MASK_SHOT);
#endif
	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_DEPTHHACK | RF_CROSSHAIR;

	VectorCopy(trace.endpos, ent.origin);

	// scale the crosshair so it appears the same size for all distances
	// Position is based on weapon aim, but size is based on distance from eyes
	{
		vec3_t delta;
		float distance;
		VectorSubtract(trace.endpos, cg.refdef.vieworg, delta);
		distance = VectorLength(delta);

		// Scale radius proportional to distance to maintain constant angular size
		// radius = (normalized_size) * distance * tan(half_fov)
		ent.radius = (w / 640.0f) * distance * tan(cg.refdef.fov_x * M_PI / 360.0f);
	}
	ent.customShader = hShader;

	// set crosshair color
	if ( cg_crosshairHealth.integer ) {
		vec4_t hcolor;
		CG_ColorForHealth( hcolor );
		ent.shaderRGBA.rgba[0] = (byte)(hcolor[0] * 255);
		ent.shaderRGBA.rgba[1] = (byte)(hcolor[1] * 255);
		ent.shaderRGBA.rgba[2] = (byte)(hcolor[2] * 255);
		ent.shaderRGBA.rgba[3] = (byte)(hcolor[3] * 255);
	} else {
		CG_CrosshairColorFromInt( cg_crosshairColor.integer, ent.shaderRGBA.rgba );
	}

	// ensure crosshair is aligned with world, not HMD/view
	// Don't apply roll when rendering to virtual screen
	ent.rotation = vr->virtual_screen ? 0 : vr->hmdorientation[ROLL];

	trap_R_AddRefEntityToScene(&ent);
}



/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity( void ) {
	trace_t		trace;
	vec3_t		start, end;
	int			content;
    vec3_t viewaxis[3];
    vec3_t weaponangles;
    CG_CalculateVRWeaponPosition(start, weaponangles);
    AnglesToAxis(weaponangles, viewaxis);
    VectorMA(start, 131072, viewaxis[0], end);

	CG_Trace( &trace, start, vec3_origin, vec3_origin, end, 
		cg.snap->ps.clientNum, CONTENTS_SOLID|CONTENTS_BODY );
	if ( trace.entityNum >= MAX_CLIENTS ) {
		return;
	}

	// if the player is in fog, don't show it
	content = CG_PointContents( trace.endpos, 0 );
	if ( content & CONTENTS_FOG ) {
		return;
	}

	// if the player is invisible, don't show it
	if ( cg_entities[ trace.entityNum ].currentState.powerups & ( 1 << PW_INVIS ) ) {
		return;
	}

	// update the fade timer
	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.time;
}


/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames( void ) {
	float		*color;
	char		*name;
	float		w;

	if ( trap_Cvar_VariableValue("vr_lasersight") == 0.0f && vr->no_crosshair ) {
		return;
	}
	if ( !cg_drawCrosshairNames.integer ) {
		return;
	}
	if ( cg.renderingThirdPerson ) {
		return;
	}

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	// draw the name of the player being looked at
	color = CG_FadeColor( cg.crosshairClientTime, 1000 );
	if ( !color ) {
		trap_R_SetColor( NULL );
		return;
	}

	name = cgs.clientinfo[ cg.crosshairClientNum ].name;
#ifdef MISSIONPACK
	color[3] *= 0.5f;
	w = CG_Text_Width(name, 0.3f, 0);
	CG_Text_Paint( 320 - w / 2, 190, 0.3f, color, name, 0, 0, ITEM_TEXTSTYLE_NORMAL);
#else
	w = CG_DrawStrlen( name ) * BIGCHAR_WIDTH;
	color[3] *= 0.5f;
	CG_DrawStringExt( 320 - w / 2, 170, name, color, qfalse, qfalse, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0 );
#endif
	trap_R_SetColor( NULL );
}


//==============================================================================

/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator(void) {
	CG_DrawBigString(320 - 9 * 8, 440, "SPECTATOR", 1.0F);
	if ( cgs.gametype == GT_TOURNAMENT ) {
		CG_DrawBigString(320 - 15 * 8, 460, "waiting to play", 1.0F);
	}
	else if ( cgs.gametype >= GT_TEAM ) {
		CG_DrawBigString(320 - 39 * 8, 460, "press ESC and use the JOIN menu to play", 1.0F);
	}
}

// ---- shared layout for TVD offer / download / vote dialog box ----
#define DIALOG_CHARW		4.0f
#define DIALOG_CHARH		8.0f
#define DIALOG_PAD_X		8.0f
#define DIALOG_PAD_TOP		4.0f
#define DIALOG_BAR_H		5.0f
#define DIALOG_LINE_GAP		2.0f
#define DIALOG_Y			95.0f
#define DIALOG_MIN_W		160.0f
// base box height (2 lines): pad + line + gap + line + gap + bar
#define DIALOG_H2	( DIALOG_PAD_TOP + DIALOG_CHARH + DIALOG_LINE_GAP \
					+ DIALOG_CHARH + DIALOG_LINE_GAP + DIALOG_BAR_H )
// 3-line box height: adds another line + gap
#define DIALOG_H3	( DIALOG_H2 + DIALOG_CHARH + DIALOG_LINE_GAP )
// 4-line box height: adds yet another line + gap
#define DIALOG_H4	( DIALOG_H3 + DIALOG_CHARH + DIALOG_LINE_GAP )


static float CG_DrawDialogBox( float boxY, const char *line1, const char *line2,
						   const char *line3, const char *line4,
						   float barFrac, vec4_t barColor, float alpha,
						   qboolean highlighted );

/*
=================
CG_DrawDialogBox

Shared helper: draws a left-aligned dimmed box with text lines
and a progress bar glued to the bottom.
line3 and line4 are optional (NULL to omit).
When highlighted, draws a soft blue glow behind the box.
Returns the Y position below the box (for stacking).
=================
*/
static float CG_DrawDialogBox( float boxY, const char *line1, const char *line2,
						   const char *line3, const char *line4,
						   float barFrac, vec4_t barColor, float alpha,
						   qboolean highlighted ) {
	int		len1, len2, len3, len4, maxLen;
	float	boxW, boxH, boxX, textX;
	float	lineY, barY;
	vec4_t	bgColor, barBg, textColor;

	len1 = CG_DrawStrlen( line1 );
	len2 = CG_DrawStrlen( line2 );
	maxLen = ( len1 > len2 ) ? len1 : len2;
	if ( line3 ) {
		len3 = CG_DrawStrlen( line3 );
		if ( len3 > maxLen )
			maxLen = len3;
	}
	if ( line4 ) {
		len4 = CG_DrawStrlen( line4 );
		if ( len4 > maxLen )
			maxLen = len4;
	}
	boxW = maxLen * DIALOG_CHARW + 2 * DIALOG_PAD_X;
	if ( boxW < DIALOG_MIN_W )
		boxW = DIALOG_MIN_W;
	if ( line3 && line4 )
		boxH = DIALOG_H4;
	else if ( line3 || line4 )
		boxH = DIALOG_H3;
	else
		boxH = DIALOG_H2;

	// left-align against safe area
	boxX = DIALOG_PAD_X;
	textX = boxX + DIALOG_PAD_X;

	// highlight glow (layered rectangles behind box)
	if ( highlighted ) {
		vec4_t glow;
		glow[0] = 0.3f; glow[1] = 0.6f; glow[2] = 1.0f;
		glow[3] = 0.15f * alpha;
		CG_FillRect( boxX - 3, boxY - 3, boxW + 6, boxH + 6, glow );
		glow[3] = 0.2f * alpha;
		CG_FillRect( boxX - 2, boxY - 2, boxW + 4, boxH + 4, glow );
		glow[3] = 0.25f * alpha;
		CG_FillRect( boxX - 1, boxY - 1, boxW + 2, boxH + 2, glow );
	}

	bgColor[0] = 0.0f; bgColor[1] = 0.0f; bgColor[2] = 0.0f; bgColor[3] = 0.5f * alpha;
	barBg[0] = 0.2f; barBg[1] = 0.2f; barBg[2] = 0.2f; barBg[3] = 0.3f * alpha;
	textColor[0] = 1.0f; textColor[1] = 1.0f; textColor[2] = 1.0f; textColor[3] = alpha;
	barColor[3] *= alpha;

	CG_FillRect( boxX, boxY, boxW, boxH, bgColor );

	lineY = boxY + DIALOG_PAD_TOP;
	CG_DrawString( textX, lineY, line1, textColor,
		DIALOG_CHARW, DIALOG_CHARH, 0, DS_SHADOW );
	lineY += DIALOG_CHARH + DIALOG_LINE_GAP;
	CG_DrawString( textX, lineY, line2, textColor,
		DIALOG_CHARW, DIALOG_CHARH, 0, DS_SHADOW );
	if ( line3 ) {
		lineY += DIALOG_CHARH + DIALOG_LINE_GAP;
		CG_DrawString( textX, lineY, line3, textColor,
			DIALOG_CHARW, DIALOG_CHARH, 0, DS_SHADOW );
	}
	if ( line4 ) {
		vec4_t pulseColor;
		// pulse blue channel between 0 (yellow) and 0.7 (warm white)
		float t = 0.35f + 0.35f * sin( cg.time * 0.005f );
		pulseColor[0] = 1.0f;
		pulseColor[1] = 1.0f;
		pulseColor[2] = t;	// 0 = yellow, 1 = white
		pulseColor[3] = alpha;
		lineY += DIALOG_CHARH + DIALOG_LINE_GAP;
		CG_DrawString( textX, lineY, line4, pulseColor,
			DIALOG_CHARW, DIALOG_CHARH, 0, DS_SHADOW );
	}

	barY = boxY + boxH - DIALOG_BAR_H;
	CG_FillRect( boxX, barY, boxW, DIALOG_BAR_H, barBg );
	if ( barFrac > 0.0f )
		CG_FillRect( boxX, barY, boxW * barFrac, DIALOG_BAR_H, barColor );

	return boxY + boxH + DIALOG_LINE_GAP;
}


/*
=================
CG_DrawVote
=================
*/
static float CG_DrawVote( float y, qboolean highlighted ) {
	const char	*keyYes, *keyNo;
	char		caller[64], desc[MAX_STRING_TOKENS + 32], tally[64], keys[128];
	float		frac;
	int			elapsed;
	vec4_t		barFg;

	if ( !cgs.voteTime ) {
		return y;
	}

	// play a talk beep whenever it is modified
	if ( cgs.voteModified ) {
		cgs.voteModified = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	elapsed = cg.time - cgs.voteTime;
	if ( elapsed >= VOTE_TIME ) {
		return y;
	}

	frac = 1.0f - (float)elapsed / VOTE_TIME;
	if ( frac < 0.0f ) frac = 0.0f;

	// line 1: caller name with clientnum (or fallback)
	if ( cgs.voteCaller >= 0 && cgs.voteCaller < MAX_CLIENTS
			&& cgs.clientinfo[cgs.voteCaller].infoValid ) {
		Com_sprintf( caller, sizeof( caller ), "Vote: %s^7 (%i)",
			cgs.clientinfo[cgs.voteCaller].name, cgs.voteCaller );
	} else {
		Q_strncpyz( caller, "Vote:", sizeof( caller ) );
	}

	// line 2: vote description — resolve clientkick <N> to player name
	if ( !Q_strncmp( cgs.voteString, "clientkick ", 11 ) ) {
		int cn = atoi( cgs.voteString + 11 );
		if ( cn >= 0 && cn < MAX_CLIENTS && cgs.clientinfo[cn].infoValid )
			Com_sprintf( desc, sizeof( desc ), "kick %s^7 (%i)", cgs.clientinfo[cn].name, cn );
		else
			Q_strncpyz( desc, cgs.voteString, sizeof( desc ) );
	} else {
		Q_strncpyz( desc, cgs.voteString, sizeof( desc ) );
	}

	// line 3: tally with highlight on player's choice
	if ( cg.myVote == 1 )
		Com_sprintf( tally, sizeof( tally ), "^3yes^7:%i    no:%i",
			cgs.voteYes, cgs.voteNo );
	else if ( cg.myVote == -1 )
		Com_sprintf( tally, sizeof( tally ), "yes:%i    ^3no^7:%i",
			cgs.voteYes, cgs.voteNo );
	else
		Com_sprintf( tally, sizeof( tally ), "yes:%i    no:%i",
			cgs.voteYes, cgs.voteNo );

	// line 4: key hints (only on active dialog, before voting)
	keyYes = cg_voteYesKey.string;
	keyNo = cg_voteNoKey.string;
	if ( highlighted && keyYes[0] && keyNo[0] && !cg.myVote ) {
		Com_sprintf( keys, sizeof( keys ), "%s: yes    %s: no",
			keyYes, keyNo );
	}

	barFg[0] = 0.2f; barFg[1] = 0.6f; barFg[2] = 0.8f; barFg[3] = 0.7f;
	return CG_DrawDialogBox( y, caller, desc, tally,
		( highlighted && keyYes[0] && keyNo[0] && !cg.myVote ) ? keys : NULL,
		frac, barFg, 1.0f, highlighted );
}


/*
=================
CG_DrawTeamVote
=================
*/
static float CG_DrawTeamVote( float y, qboolean highlighted ) {
	const char	*keyYes, *keyNo;
	char		caller[64], desc[MAX_STRING_TOKENS + 32], tally[64], keys[128];
	float		frac;
	int			elapsed, cs_offset;
	vec4_t		barFg;

	if ( cgs.clientinfo[ cg.clientNum ].team == TEAM_RED )
		cs_offset = 0;
	else if ( cgs.clientinfo[ cg.clientNum ].team == TEAM_BLUE )
		cs_offset = 1;
	else
		return y;

	if ( !cgs.teamVoteTime[cs_offset] ) {
		return y;
	}

	// play a talk beep whenever it is modified
	if ( cgs.teamVoteModified[cs_offset] ) {
		cgs.teamVoteModified[cs_offset] = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	elapsed = cg.time - cgs.teamVoteTime[cs_offset];
	if ( elapsed >= VOTE_TIME ) {
		return y;
	}

	frac = 1.0f - (float)elapsed / VOTE_TIME;
	if ( frac < 0.0f ) frac = 0.0f;

	// line 1: caller name with clientnum (or fallback)
	if ( cgs.teamVoteCaller[cs_offset] >= 0
			&& cgs.teamVoteCaller[cs_offset] < MAX_CLIENTS
			&& cgs.clientinfo[cgs.teamVoteCaller[cs_offset]].infoValid ) {
		Com_sprintf( caller, sizeof( caller ), "Team Vote: %s^7 (%i)",
			cgs.clientinfo[cgs.teamVoteCaller[cs_offset]].name,
			cgs.teamVoteCaller[cs_offset] );
	} else {
		Q_strncpyz( caller, "Team Vote:", sizeof( caller ) );
	}

	// line 2: vote description — resolve leader <N> to player name
	if ( !Q_strncmp( cgs.teamVoteString[cs_offset], "leader ", 7 ) ) {
		int cn = atoi( cgs.teamVoteString[cs_offset] + 7 );
		if ( cn >= 0 && cn < MAX_CLIENTS && cgs.clientinfo[cn].infoValid )
			Com_sprintf( desc, sizeof( desc ), "leader %s^7 (%i)", cgs.clientinfo[cn].name, cn );
		else
			Q_strncpyz( desc, cgs.teamVoteString[cs_offset], sizeof( desc ) );
	} else {
		Q_strncpyz( desc, cgs.teamVoteString[cs_offset], sizeof( desc ) );
	}

	// line 3: tally with highlight on player's choice
	if ( cg.myTeamVote == 1 )
		Com_sprintf( tally, sizeof( tally ), "^3yes^7:%i    no:%i",
			cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
	else if ( cg.myTeamVote == -1 )
		Com_sprintf( tally, sizeof( tally ), "yes:%i    ^3no^7:%i",
			cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
	else
		Com_sprintf( tally, sizeof( tally ), "yes:%i    no:%i",
			cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );

	// line 4: key hints (only on active dialog, before voting)
	keyYes = cg_voteYesKey.string;
	keyNo = cg_voteNoKey.string;
	if ( highlighted && keyYes[0] && keyNo[0] && !cg.myTeamVote ) {
		Com_sprintf( keys, sizeof( keys ), "%s: yes    %s: no",
			keyYes, keyNo );
	}

	barFg[0] = 0.2f; barFg[1] = 0.6f; barFg[2] = 0.8f; barFg[3] = 0.7f;
	return CG_DrawDialogBox( y, caller, desc, tally,
		( highlighted && keyYes[0] && keyNo[0] && !cg.myTeamVote ) ? keys : NULL,
		frac, barFg, 1.0f, highlighted );
}


static qboolean CG_DrawScoreboard( void ) {
#ifdef MISSIONPACK
	static qboolean firstTime = qtrue;
	static qboolean scoreboardCursorActive = qfalse;
	static int lastFollowedClient = -1;
	qboolean spectator;

	if (cg_paused.integer) {
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}

	// should never happen in Team Arena
	if (cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if ( cg.warmup && !cg.showScores ) {
		return qfalse;
	}

	if ( cg.showScores || (cg.predictedPlayerState.pm_type == PM_DEAD && !CG_IsThirdPersonFollowMode(VRFM_THIRDPERSON_2)) || cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
	} else {
		if ( !CG_FadeColor( cg.scoreFadeTime, FADE_TIME ) ) {
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			cg.killerName[0] = 0;
			firstTime = qtrue;
			// Disable VR scoreboard cursor when scoreboard fades
			if ( scoreboardCursorActive ) {
				vr->scoreboardCursorX = NULL;
				vr->scoreboardCursorY = NULL;
				scoreboardCursorActive = qfalse;
				if (menuScoreboard) {
					menuScoreboard->window.flags &= ~WINDOW_FORCED;
				}
			}
			return qfalse;
		}
	}

	// Find scoreboard menu if not already cached
	if (menuScoreboard == NULL) {
		if ( cgs.gametype >= GT_TEAM ) {
			menuScoreboard = Menus_FindByName("teamscore_menu");
		} else {
			menuScoreboard = Menus_FindByName("score_menu");
		}
	}

	if (menuScoreboard == NULL) {
		return qfalse;
	}

	// Clear WINDOW_FORCED unless scoreboard cursor is active
	// (keeping it set allows Menu_HandleMouseMove to work for click-to-follow)
	if (!scoreboardCursorActive) {
		menuScoreboard->window.flags &= ~WINDOW_FORCED;
	}

	if (firstTime) {
		CG_SetScoreSelection(menuScoreboard);
		firstTime = qfalse;
	}

	// Update selection when followed player changes
	if ( cg.snap && ((cg.snap->ps.pm_flags & PMF_FOLLOW) || cgs.tvPlayback) ) {
		if ( cg.snap->ps.clientNum != lastFollowedClient ) {
			CG_SetScoreSelection(menuScoreboard);
			lastFollowedClient = cg.snap->ps.clientNum;
		}
	}

	Menu_Paint(menuScoreboard, qtrue);

	// VR scoreboard cursor for spectators (enables click-to-follow)
	spectator = cg.snap && (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ||
	            ( cg.snap->ps.pm_flags & PMF_FOLLOW ) || cg.demoPlayback || cgs.tvPlayback);
	if ( cg.showScores && spectator && !scoreboardCursorActive ) {
		// Center cursor and link VR input to cgs cursor
		cgs.cursorX = SCREEN_WIDTH / 2;
		cgs.cursorY = SCREEN_HEIGHT / 2;
		vr->scoreboardCursorX = &cgs.cursorX;
		vr->scoreboardCursorY = &cgs.cursorY;
		// Enable KEYCATCH_CGAME so key events reach CG_KeyEvent
		trap_Key_SetCatcher( trap_Key_GetCatcher() | KEYCATCH_CGAME );
		// Set WINDOW_FORCED so Menu_HandleMouseMove processes the scoreboard
		menuScoreboard->window.flags |= WINDOW_FORCED;
		scoreboardCursorActive = qtrue;
	} else if ( !cg.showScores && scoreboardCursorActive ) {
		vr->scoreboardCursorX = NULL;
		vr->scoreboardCursorY = NULL;
		trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_CGAME );
		scoreboardCursorActive = qfalse;
		menuScoreboard->window.flags &= ~WINDOW_FORCED;
	}

	// Draw VR cursor if scoreboard cursor is active
	if ( scoreboardCursorActive ) {
		CG_DrawPic( cgs.cursorX - 12, cgs.cursorY - 12, 24, 24, cgDC.Assets.cursor );
	}

	// load any models that have been deferred
	if ( ++cg.deferredPlayerLoading > 10 ) {
		CG_LoadDeferredPlayers();
	}

	return qtrue;
#else
	return CG_DrawOldScoreboard();
#endif
}

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission( void ) {
//	int key;
#ifdef MISSIONPACK
	//if (cg_singlePlayer.integer) {
	//	CG_DrawCenterString();
	//	return;
	//}
#else
	if ( cgs.gametype == GT_SINGLE_PLAYER ) {
		CG_DrawCenterString();
		return;
	}
#endif
	cg.scoreFadeTime = cg.time;
	cg.scoreBoardShowing = CG_DrawScoreboard();
}

/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow( void ) {
	float		x;
	vec4_t		color;
	const char	*name;

	if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW) && !cgs.tvPlayback ) {
		return qfalse;
	}
	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	CG_DrawSmallString( 320 - 9 * SMALLCHAR_WIDTH / 2, 56, "following", 1.0F );

	name = cgs.clientinfo[ cg.snap->ps.clientNum ].name;

	x = 0.5 * ( 640 - BIGCHAR_WIDTH * CG_DrawStrlen( name ) );

	CG_DrawStringExt( x, 72, name, color, qfalse, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0 );

	return qtrue;
}



/*
=================
CG_DrawDownloadProgress

Draws a download progress indicator in a centered box.
Shows a completion animation (pulse + fade) when the download ends.
=================
*/
static float CG_DrawDownloadProgress( float y ) {
	const char	*line1, *line2;
	float		frac, alpha;
	vec4_t		barFg;
	int			size, count;

	// active download
	if ( cg_downloadName.string[0] != '\0' ) {
		cg.downloadActive = qtrue;
		Q_strncpyz( cg.downloadFinishName, cg_downloadName.string,
			sizeof( cg.downloadFinishName ) );

		size  = cg_downloadSize.integer;
		count = cg_downloadCount.integer;
		line1 = cg_downloadName.string;

		if ( size > 0 ) {
			if ( size > 0x200000 )
				frac = (float)( count >> 8 ) / (float)( size >> 8 );
			else
				frac = (float)count / (float)size;
			if ( frac > 1.0f ) frac = 1.0f;
			if ( frac < 0.0f ) frac = 0.0f;
			line2 = va( "%d%%", (int)( frac * 100.0f ) );
		} else {
			frac = 0.0f;
			if ( count >= 1024 * 1024 )
				line2 = va( "%d.%d MB received",
					count / ( 1024 * 1024 ),
					( count % ( 1024 * 1024 ) ) * 10 / ( 1024 * 1024 ) );
			else
				line2 = va( "%d KB received", count / 1024 );
		}

		barFg[0] = 0.8f; barFg[1] = 0.8f; barFg[2] = 0.2f; barFg[3] = 0.7f;
		return CG_DrawDialogBox( y, line1, line2, NULL, NULL, frac, barFg, 1.0f, qfalse );
	}

	// transition: download just ended
	if ( cg.downloadActive ) {
		cg.downloadActive = qfalse;
		size  = cg_downloadSize.integer;
		count = cg_downloadCount.integer;
		cg.downloadFinishError = ( size > 0 && count < size ) ? qtrue : qfalse;
		cg.downloadFinishTime = cg.time;
	}

	// completion animation
	if ( cg.downloadFinishTime != 0 ) {
		int		t;
		float	pulseFrac;

		t = cg.time - cg.downloadFinishTime;
		if ( t >= 1500 ) {
			cg.downloadFinishTime = 0;
			return y;
		}

		if ( t < 500 ) {
			pulseFrac = sin( t * M_PI / 125.0f );
			alpha = 0.75f + 0.25f * pulseFrac;
		} else {
			alpha = 1.0f - (float)( t - 500 ) / 1000.0f;
			if ( alpha < 0.0f ) alpha = 0.0f;
		}

		line1 = cg.downloadFinishName;
		line2 = cg.downloadFinishError ? "download failed" : "complete";
		frac  = 1.0f;

		if ( cg.downloadFinishError ) {
			barFg[0] = 0.8f; barFg[1] = 0.2f; barFg[2] = 0.2f; barFg[3] = 0.7f;
		} else {
			barFg[0] = 0.8f; barFg[1] = 0.8f; barFg[2] = 0.2f; barFg[3] = 0.7f;
		}

		return CG_DrawDialogBox( y, line1, line2, NULL, NULL, frac, barFg, alpha, qfalse );
	}

	return y;
}


/*
=================
CG_DrawTVTimeline
=================
*/
static void CG_DrawTVTimeline( void ) {
	int		time, duration;
	float	frac;
	int		timeSec, durationSec;
	vec4_t	bgColor = { 0.0f, 0.0f, 0.0f, 0.5f };
	vec4_t	fgColor = { 0.8f, 0.8f, 0.2f, 0.7f };

	if ( !cgs.tvPlayback || !cg_tvTimeline.integer ) {
		return;
	}

	// Auto-cancel scrub if input capture was lost
	if ( cgs.tvScrubActive && !( trap_Key_GetCatcher() & KEYCATCH_CGAME ) ) {
		cgs.tvScrubActive = qfalse;
		vr->menuYawLocked = qfalse;
		vr->menuYaw = cgs.tvScrubSavedMenuYaw;
		if ( !cgs.score_catched ) {
			vr->scoreboardCursorX = NULL;
			vr->scoreboardCursorY = NULL;
		}
	}

	time = cg_tvTime.integer;
	duration = cg_tvDuration.integer;
	if ( duration <= 0 ) {
		return;
	}

	frac = (float)time / (float)duration;
	if ( frac < 0.0f ) frac = 0.0f;
	if ( frac > 1.0f ) frac = 1.0f;

	// progress bar at screen bottom
	CG_FillRect( 0, 474, 640, 6, bgColor );

	// dim progress fill when scrubbing to emphasize scrub indicator
	if ( cgs.tvScrubActive ) {
		vec4_t dimFgColor = { 0.8f, 0.8f, 0.2f, 0.35f };
		CG_FillRect( 0, 474, 640 * frac, 6, dimFgColor );
	} else {
		CG_FillRect( 0, 474, 640 * frac, 6, fgColor );
	}

	// time text above the bar (right-aligned)
	timeSec = time / 1000;
	durationSec = duration / 1000;
	CG_DrawString( 636, 474 - SMALLCHAR_HEIGHT,
		va( "%d:%02d / %d:%02d",
			timeSec / 60, timeSec % 60,
			durationSec / 60, durationSec % 60 ),
		colorWhite, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0,
		DS_SHADOW | DS_RIGHT );

	// scrub indicator (only when actively scrubbing)
	if ( cgs.tvScrubActive ) {
		vec4_t	lineColor = { 1.0f, 1.0f, 1.0f, 0.9f };
		int		scrubX;
		float	scrubFrac;
		int		scrubMs, scrubSec;
		float	textX;
		int		flags;
		const char *timeStr;
		int		textWidth;

		scrubX = cgs.cursorX;
		if ( scrubX < 0 ) scrubX = 0;
		if ( scrubX > 640 ) scrubX = 640;

		// vertical line extending up from timeline
		CG_FillRect( scrubX - 1, 454, 2, 26, lineColor );

		// time label above the line, shifted to stay within screen bounds
		scrubFrac = scrubX / 640.0f;
		scrubMs = (int)( scrubFrac * duration );
		scrubSec = scrubMs / 1000;
		timeStr = va( "%d:%02d", scrubSec / 60, scrubSec % 60 );
		textWidth = CG_DrawStrlen( timeStr ) * SMALLCHAR_WIDTH;

		textX = (float)scrubX;
		flags = DS_SHADOW;
		if ( textX - textWidth / 2 < 0 ) {
			// near left edge: left-align to avoid clipping
			textX = 0;
		} else if ( textX + textWidth / 2 > 640 ) {
			// near right edge: right-align to avoid clipping
			textX = 640;
			flags |= DS_RIGHT;
		} else {
			flags |= DS_CENTER;
		}

		CG_DrawString( textX, 454 - SMALLCHAR_HEIGHT,
			timeStr, colorWhite, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0,
			flags );
	}
}

/*
=================
CG_DrawTVOffer

Draws a download offer prompt when the server offers a TV demo.
Polls cl_tvdOffer cvar each frame; countdown shown as a progress bar.
=================
*/
static float CG_DrawTVOffer( float y ) {
	const char	*offer;
	const char	*keyYes, *keyNo;
	char		keys[128];
	float		frac;
	int			mode;
	char		buf[16];
	vec4_t		barFg;

	offer = cg_tvdOffer.string;

	// offer cleared by engine — release state
	if ( offer[0] == '\0' ) {
		if ( cg.tvdOfferName[0] ) {
			cg.tvdOfferName[0] = '\0';
			cg.tvdOfferTime = 0;
		}
		return y;
	}

	// new or changed offer
	if ( Q_stricmp( offer, cg.tvdOfferName ) != 0 ) {
		Q_strncpyz( cg.tvdOfferName, offer, sizeof( cg.tvdOfferName ) );
		cg.tvdOfferTime = cg.time;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	trap_Cvar_VariableStringBuffer( "cl_tvDownload", buf, sizeof( buf ) );
	mode = atoi( buf );

	{
		int timeout = (int)( cg_tvdTimeout.value * 1000.0f );
		if ( timeout < 0 )
			timeout = 0;

		// timeout expired (0 = instant)
		if ( cg.time - cg.tvdOfferTime >= timeout ) {
			if ( mode <= 1 )
				trap_SendConsoleCommand( "tvdno\n" );
			else
				trap_SendConsoleCommand( "tvdyes\n" );
			return y;
		}

		// countdown bar: full -> empty over timeout
		frac = 1.0f - (float)( cg.time - cg.tvdOfferTime ) / timeout;
		if ( frac < 0.0f ) frac = 0.0f;
	}

	// use resolved key names
	keyYes = cg_voteYesKey.string;
	keyNo = cg_voteNoKey.string;
	if ( keyYes[0] && keyNo[0] ) {
		Com_sprintf( keys, sizeof( keys ),
			mode <= 1 ? "^7%s: yes    ^3%s: no"
			          : "^3%s: yes    ^7%s: no",
			keyYes, keyNo );
	} else {
		Com_sprintf( keys, sizeof( keys ),
			mode <= 1 ? "^7vote yes    ^3vote no"
			          : "^3vote yes    ^7vote no" );
	}

	barFg[0] = 0.8f; barFg[1] = 0.8f; barFg[2] = 0.2f; barFg[3] = 0.7f;
	return CG_DrawDialogBox( y, "Download last match?", cg.tvdOfferName,
		NULL, keys, frac, barFg, 1.0f, qfalse );
}


/*
=================
CG_DrawTVOverlay

Consolidates all TV/download/vote HUD draws.
Sets vr->vote_active when any yes/no dialog is visible.
=================
*/
static void CG_DrawTVOverlay( void ) {
	float y = DIALOG_Y;
	int target = CG_ActiveVoteTarget();
	int voteRemain, teamRemain, cs_offset;

	y = CG_DrawDownloadProgress( y );
	y = CG_DrawTVOffer( y );

	// sort vote + teamvote by remaining time (soonest expiration first)
	voteRemain = CG_VoteActive() ? VOTE_TIME - ( cg.time - cgs.voteTime ) : 0;
	cs_offset = -1;
	if ( cgs.clientinfo[ cg.clientNum ].team == TEAM_RED )
		cs_offset = 0;
	else if ( cgs.clientinfo[ cg.clientNum ].team == TEAM_BLUE )
		cs_offset = 1;
	teamRemain = ( cs_offset >= 0 && CG_TeamVoteActive() )
		? VOTE_TIME - ( cg.time - cgs.teamVoteTime[cs_offset] ) : 0;

	if ( teamRemain > 0 && ( voteRemain <= 0 || teamRemain < voteRemain ) ) {
		y = CG_DrawTeamVote( y, target == 2 );
		y = CG_DrawVote( y, target == 1 );
	} else {
		y = CG_DrawVote( y, target == 1 );
		y = CG_DrawTeamVote( y, target == 2 );
	}

	CG_DrawTVTimeline();

	// Check if any yes/no dialog is active for VR button intercept.
	vr->vote_active = CG_TVDOfferActive() || CG_VoteActive() || CG_TeamVoteActive();
}


/*
=================
CG_DrawAmmoWarning
=================
*/
static void CG_DrawAmmoWarning( void ) {
	const char	*s;
	int			w;

	if ( cg_drawAmmoWarning.integer == 0 ) {
		return;
	}

	if ( !cg.lowAmmoWarning ) {
		return;
	}

	if ( cg.lowAmmoWarning == 2 ) {
		s = "OUT OF AMMO";
	} else {
		s = "LOW AMMO WARNING";
	}
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString(320 - w / 2, 64, s, 1.0F);
}


#ifdef MISSIONPACK
/*
=================
CG_DrawProxWarning
=================
*/
static void CG_DrawProxWarning( void ) {
	char s [32];
	int			w;
  static int proxTime;
  int proxTick;

	if( !(cg.snap->ps.eFlags & EF_TICKING ) ) {
    proxTime = 0;
		return;
	}

  if (proxTime == 0) {
    proxTime = cg.time;
  }

  proxTick = 10 - ((cg.time - proxTime) / 1000);

  if (proxTick > 0 && proxTick <= 5) {
    Com_sprintf(s, sizeof(s), "INTERNAL COMBUSTION IN: %i", proxTick);
  } else {
    Com_sprintf(s, sizeof(s), "YOU HAVE BEEN MINED");
  }

	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigStringColor( 320 - w / 2, 64 + BIGCHAR_HEIGHT, s, g_color_table[ColorIndex(COLOR_RED)] );
}
#endif


/*
=================
CG_DrawWarmup
=================
*/
static void CG_DrawWarmup( void ) {
	int			w;
	int			i;
#ifdef MISSIONPACK
	float scale;
#endif
	clientInfo_t *ci1, *ci2;
	int			cw;
	const char	*s;

	if ( !cg.warmup ) {
		return;
	}

	if ( cg.warmup < 0 ) {
		CG_DrawString( 320,24, "Waiting for players", colorWhite, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0,
			DS_PROPORTIONAL | DS_CENTER | DS_SHADOW );
		return;
	}

	if ( cgs.gametype == GT_TOURNAMENT ) {
		// find the two active players
		ci1 = NULL;
		ci2 = NULL;
		for ( i = 0 ; i < cgs.maxclients ; i++ ) {
			if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_FREE ) {
				if ( !ci1 ) {
					ci1 = &cgs.clientinfo[i];
				} else {
					ci2 = &cgs.clientinfo[i];
				}
			}
		}

		if ( ci1 && ci2 ) {
			s = va( "%s vs %s", ci1->name, ci2->name );
#ifdef MISSIONPACK
			w = CG_Text_Width(s, 0.6f, 0);
			CG_Text_Paint(320 - w / 2, 60, 0.6f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
#else
			w = CG_DrawStrlen( s );
			if ( w > 640 / GIANT_WIDTH ) {
				cw = 640 / w;
			} else {
				cw = GIANT_WIDTH;
			}
			CG_DrawString( 320, 20, s, colorWhite, cw, cw*1.5, 0, DS_SHADOW | DS_CENTER | DS_PROPORTIONAL );
#endif
		}
	} else {
		if ( cgs.gametype == GT_FFA ) {
			s = "Free For All";
		} else if ( cgs.gametype == GT_TEAM ) {
			s = "Team Deathmatch";
		} else if ( cgs.gametype == GT_CTF ) {
			s = "Capture the Flag";
#ifdef MISSIONPACK
		} else if ( cgs.gametype == GT_1FCTF ) {
			s = "One Flag CTF";
		} else if ( cgs.gametype == GT_OBELISK ) {
			s = "Overload";
		} else if ( cgs.gametype == GT_HARVESTER ) {
			s = "Harvester";
#endif
		} else {
			s = "";
		}
#ifdef MISSIONPACK
		w = CG_Text_Width(s, 0.6f, 0);
		CG_Text_Paint(320 - w / 2, 90, 0.6f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
#else
		w = CG_DrawStrlen( s );
		if ( w > 640 / GIANT_WIDTH ) {
			cw = 640 / w;
		} else {
			cw = GIANT_WIDTH;
		}
		CG_DrawString( 320, 25, s, colorWhite, cw, cw*1.1f, 0, DS_PROPORTIONAL | DS_SHADOW | DS_CENTER );
#endif
	}

	if ( cg.warmupCount <= 0 )
		return;

	s = va( "Starts in: %i", cg.warmupCount );

	switch ( cg.warmupCount ) {
	case 1:
		cw = 28;
#ifdef MISSIONPACK
		scale = 0.54f;
#endif
		break;
	case 2:
		cw = 24;
#ifdef MISSIONPACK
		scale = 0.51f;
#endif
		break;
	case 3:
		cw = 20;
#ifdef MISSIONPACK
		scale = 0.48f;
#endif
		break;
	default:
		cw = 16;
#ifdef MISSIONPACK
		scale = 0.45f;
#endif
		break;
	}

#ifdef MISSIONPACK
	w = CG_Text_Width(s, scale, 0);
	CG_Text_Paint(320 - w / 2, 125, scale, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
#else
	CG_DrawString( 320, 70, s, colorWhite, cw, cw * 1.5, 0, DS_CENTER | DS_SHADOW | DS_PROPORTIONAL );
#endif
}

//==================================================================================
#ifdef MISSIONPACK
/* 
=================
CG_DrawTimedMenus
=================
*/
void CG_DrawTimedMenus( void ) {
	if (cg.voiceTime) {
		int t = cg.time - cg.voiceTime;
		if ( t > 2500 ) {
			Menus_CloseByName("voiceMenu");
			trap_Cvar_Set("cl_conXOffset", "0");
			cg.voiceTime = 0;
		}
	}
}
#endif

/*
==============
CG_GetProjectionCenter

Calculates the optical center of the VR projection in virtual 640x480 coordinates.

VR headsets have asymmetric FOV (more down-look than up-look), which causes the
OpenXR projection matrix to shift the optical center away from the geometric
framebuffer center. This function computes that offset based on the current FOV
angles and weapon zoom level.

The OpenGL projection matrix elements m[8] and m[9] determine the horizontal and
vertical offsets:
  m[8] = (tanRight + tanLeft) / (tanRight - tanLeft)
  m[9] = (tanUp + tanDown) / (tanUp - tanDown)

A point along the view axis (straight ahead) projects to NDC position (-m[8], -m[9]).
Converting to screen coordinates (with 0,0 at top-left):
  screenX = 320 * (1 + m[8])
  screenY = 240 * (1 + m[9])
==============
*/
void CG_GetProjectionCenter( float *outX, float *outY )
{
	// Default to geometric center
	float x = 320.0f;
	float y = 240.0f;

	// Get the effective FOV angles, accounting for weapon zoom
	// The projection matrix in vr_renderer.c divides angles by weapon_zoomLevel
	float zoomLevel = vr->weapon_zoomLevel;
	if (zoomLevel < 1.0f) zoomLevel = 1.0f;

	float angleUp = vr->fov_angle_up / zoomLevel;
	float angleDown = vr->fov_angle_down / zoomLevel;
	float angleLeft = vr->fov_angle_left / zoomLevel;
	float angleRight = vr->fov_angle_right / zoomLevel;

	float tanUp = tanf(angleUp);
	float tanDown = tanf(angleDown);
	float tanLeft = tanf(angleLeft);
	float tanRight = tanf(angleRight);

	// Vertical: m[9] = (tanUp + tanDown) / (tanUp - tanDown)
	float tanHeightV = tanUp - tanDown;
	if (fabsf(tanHeightV) > 0.001f) {
		float m9 = (tanUp + tanDown) / tanHeightV;
		y = 240.0f * (1.0f + m9);
	}

	// Horizontal: m[8] = (tanRight + tanLeft) / (tanRight - tanLeft)
	float tanWidthH = tanRight - tanLeft;
	if (fabsf(tanWidthH) > 0.001f) {
		float m8 = (tanRight + tanLeft) / tanWidthH;
		x = 320.0f * (1.0f + m8);
	}

	if (outX) *outX = x;
	if (outY) *outY = y;
}

/*
==============
CG_DrawWeapReticle

Draws the railgun scope reticle overlay.
==============
*/
static void CG_DrawWeapReticle( void )
{
	vec4_t light_color = {0.7f, 0.7f, 0.7f, 0.5f};
	vec4_t black = {0.0f, 0.0f, 0.0f, 1.0f};
	vec4_t red = {0.8f, 0.0f, 0.0f, 0.5f};

	float indentX = 0.16f;
	float indentY = 0.21f;  // larger Y indent to make scope circular (compensates for 4:3 aspect)
	float X_WIDTH = 640;
	float Y_HEIGHT = 480;

	// OpenXR has asymmetric FOV, so get the actual optical center
	float centerX, centerY;
	CG_GetProjectionCenter(&centerX, &centerY);

	// Get the Y offset: projection center is above geometric center (lower Y value),
	// so we need a negative offset to shift elements UP toward the projection center
	float reticleYOffset = centerY - 240.0f;  // negative when proj center is above geometric center

	float x = (X_WIDTH * indentX);
	float y = (Y_HEIGHT * indentY) + reticleYOffset;
	float w = (X_WIDTH * (1-(2*indentX))) / 2.0f;
	float h = (Y_HEIGHT * (1-(2*indentY))) / 2;

	CG_AdjustFrom640( &x, &y, &w, &h );

	// sides - widen by asymmetry offset (scaled 2x for IPD compensation) to prevent world showing through
	float asymmetryExtra = CG_GetMaxAsymmetryPixels() * 2.0f;
	float sideWidth = (X_WIDTH * indentX) + asymmetryExtra;
	CG_FillRect( -asymmetryExtra, 0, sideWidth, Y_HEIGHT, black );
	CG_FillRect( X_WIDTH * (1 - indentX), 0, sideWidth, Y_HEIGHT, black );
	// top/bottom
	CG_FillRect( X_WIDTH * indentX, 0, X_WIDTH * (1-2*indentX), (Y_HEIGHT * indentY) + reticleYOffset, black );
	CG_FillRect( X_WIDTH * indentX, Y_HEIGHT * (1-indentY) + reticleYOffset, X_WIDTH * (1-2*indentX), (Y_HEIGHT * indentY), black );

	{
		// center
		if ( cgs.media.reticleShader ) {
			trap_R_DrawStretchPic( x, y, w, h, 0, 0, 1, 1, cgs.media.reticleShader );    // tl
			trap_R_DrawStretchPic( x + w, y, w, h, 1, 0, 0, 1, cgs.media.reticleShader );  // tr
			trap_R_DrawStretchPic( x, y + h, w, h, 0, 1, 1, 0, cgs.media.reticleShader );    // bl
			trap_R_DrawStretchPic( x + w, y + h, w, h, 1, 1, 0, 0, cgs.media.reticleShader );  // br
		}

		// crosshairs - coming from scope edges toward center
		float hairThick = 1.0f;
		float hairLength = 160.0f;

		// Scope edges
		float leftEdge = X_WIDTH * indentX;
		float rightEdge = X_WIDTH * (1.0f - indentX);
		float topEdge = Y_HEIGHT * indentY + reticleYOffset;
		float bottomEdge = Y_HEIGHT * (1.0f - indentY) + reticleYOffset;

		CG_FillRect( leftEdge, centerY - hairThick/2.66f, hairLength, hairThick * 0.75f, light_color );                 // left
		CG_FillRect( rightEdge - hairLength, centerY - hairThick/2.66f, hairLength, hairThick * 0.75f, light_color );   // right
		CG_FillRect( centerX - hairThick/2, topEdge, hairThick, hairLength * 0.65f, light_color );                  // top
		CG_FillRect( centerX - hairThick/2, bottomEdge - (hairLength * 0.65f), hairThick, hairLength * 0.75f, light_color );  // bottom
		CG_FillRect( centerX - hairThick/2, centerY - 6, hairThick, 12, red ); // Vertical center
		CG_FillRect( centerX - 8, centerY - hairThick/2.66f, 16, hairThick * 0.75f, red ); // Horizontal center
	}
}

/*
==============
CG_GetMaxAsymmetryPixels

Calculate max horizontal asymmetry offset in pixels across both eyes.
Used to widen vignettes to cover full stereo FOV.
==============
*/
float CG_GetMaxAsymmetryPixels( void )
{
	int eye;
	float maxOffset = 0.0f;

	for (eye = 0; eye < 2; eye++) {
		float tanLeft = tanf(vr->eye_fov_angle_left[eye]);
		float tanRight = tanf(vr->eye_fov_angle_right[eye]);
		float tanWidth = tanRight - tanLeft;

		if (fabsf(tanWidth) > 0.001f) {
			float m8 = (tanRight + tanLeft) / tanWidth;
			float offset = fabsf(m8);
			if (offset > maxOffset) {
				maxOffset = offset;
			}
		}
	}

	return maxOffset * cg.refdef.width / 2.0f;
}

/*
==============
CG_GetCombinedFovScale

Return ratio of combined binocular FOV to single eye FOV.
Used to scale vignette width for full stereo coverage.
==============
*/
float CG_GetCombinedFovScale( void )
{
	float leftEyeLeft = vr->eye_fov_angle_left[0];
	float leftEyeRight = vr->eye_fov_angle_right[0];
	float rightEyeRight = vr->eye_fov_angle_right[1];

	float singleEyeTanWidth = tanf(leftEyeRight) - tanf(leftEyeLeft);
	float combinedTanWidth = tanf(rightEyeRight) - tanf(leftEyeLeft);

	if (fabsf(singleEyeTanWidth) < 0.001f) {
		return 1.0f;
	}

	return combinedTanWidth / singleEyeTanWidth;
}

/*
==============
CG_DrawVignette
==============
*/
float currentComfortVignetteValue = 0.0f;
float filteredViewYawDelta = 0.0f;

static void CG_DrawVignette( void )
{

	float comfortVignetteValue = trap_Cvar_VariableValue( "vr_comfortVignette" );
	if (comfortVignetteValue <= 0.0f || comfortVignetteValue > 1.0f)
	{
		return;
	}

	float yawDelta = fabsf(vr->clientview_yaw_delta);
	if (yawDelta > 180)
	{
		yawDelta = fabs(yawDelta - 360);
	}
	filteredViewYawDelta = filteredViewYawDelta * 0.75f + yawDelta * 0.25f;
	if (VectorLength(cg.predictedPlayerState.velocity) > 30.0 || (filteredViewYawDelta > 1))
	{
		if (currentComfortVignetteValue <  comfortVignetteValue)
		{
			currentComfortVignetteValue += comfortVignetteValue * 0.05;
			if (currentComfortVignetteValue > 1.0f)
				currentComfortVignetteValue = 1.0f;
		}
	} else{
		if (currentComfortVignetteValue >  0.0f)
			currentComfortVignetteValue -= comfortVignetteValue * 0.05;
	}

	if (currentComfortVignetteValue > 0.0f && currentComfortVignetteValue <= 1.0f && !(vr->weapon_zoomed))
	{
		// Calculate combined FOV scale - the ratio of binocular FOV to single eye FOV
		// This tells us how much wider the total view is than a single eye's view
		float combinedFovScale = CG_GetCombinedFovScale();

		// How much extra width on each side to cover the combined FOV
		float extraWidth = (cg.refdef.width * (combinedFovScale - 1.0f)) / 2.0f;

		// Calculate vertical FOV asymmetry offset
		// OpenXR typically has more FOV below optical center than above
		float projCenterX, projCenterY;
		CG_GetProjectionCenter(&projCenterX, &projCenterY);
		// projCenterY is in 640x480 coords where 240 is geometric center
		// Convert to screen pixel offset: positive means optical center is below geometric center
		float verticalAsymmetryOffset = (projCenterY - 240.0f) / 480.0f * cg.refdef.height;

		// Base inset from the edges based on comfort vignette value
		float baseInsetX = currentComfortVignetteValue * cg.refdef.width / 3.5f;
		float baseInsetY = currentComfortVignetteValue * cg.refdef.height / 3.5f;

		// Adjust top/bottom insets for vertical asymmetry
		// verticalAsymmetryOffset is positive when optical center is below geometric center
		// Adding it to top inset and subtracting from bottom shifts the opening upward
		int insetTop = (int)(baseInsetY + verticalAsymmetryOffset);
		int insetBottom = (int)(baseInsetY - verticalAsymmetryOffset);
		if (insetTop < 0) insetTop = 0;
		if (insetBottom < 0) insetBottom = 0;

		int insetX = (int)baseInsetX;

		// Vignette covers the normal viewport minus insets (not stretched into extended FOV)
		int vignetteX = insetX;
		int vignetteW = cg.refdef.width - 2 * insetX;
		int vignetteH = cg.refdef.height - insetTop - insetBottom;

		// Account for vertical offset when viewport is centered (e.g., virtual screen mode)
		int yOffset = cg.refdef.y;

		// Extended FOV edges
		int leftEdge = (int)(-extraWidth);
		int rightEdge = (int)(cg.refdef.width + extraWidth);

		// Black borders to fill the solid black areas around the vignette
		vec4_t black = {0.0, 0.0, 0.0, 1};
		trap_R_SetColor( black );

		// Left border: from extended left edge to vignette start
		trap_R_DrawStretchPic( leftEdge, yOffset, vignetteX - leftEdge, cg.refdef.height, 0, 0, 1, 1, cgs.media.whiteShader );
		// Right border: from vignette end to extended right edge
		trap_R_DrawStretchPic( vignetteX + vignetteW, yOffset, rightEdge - (vignetteX + vignetteW), cg.refdef.height, 0, 0, 1, 1, cgs.media.whiteShader );

		// Top border: between the side borders, above the vignette
		trap_R_DrawStretchPic( vignetteX, yOffset, vignetteW, insetTop, 0, 0, 1, 1, cgs.media.whiteShader );
		// Bottom border: between the side borders, below the vignette
		trap_R_DrawStretchPic( vignetteX, yOffset + cg.refdef.height - insetBottom, vignetteW, insetBottom, 0, 0, 1, 1, cgs.media.whiteShader );

		// Vignette shader - covers normal viewport minus insets
		trap_R_DrawStretchPic( vignetteX, yOffset + insetTop, vignetteW, vignetteH, 0, 0, 1, 1, cgs.media.vignetteShader );

		trap_R_SetColor( NULL );
	}
}

/*
=================
CG_DrawHUD2D - Draw 2D elements always intended for the in-world HUD
=================
*/
static void CG_DrawHUD2D(void)
{
#ifdef MISSIONPACK
	if (cgs.orderPending && cg.time > cgs.orderTime) {
		CG_CheckOrderPending();
	}
#endif
	// if we are taking a levelshot for the menu, don't draw anything
	if ( cg.levelShot ) {
		return;
	}

#if 0
	if ( cg_draw2D.integer == 0 ) {
		return;
	}
#endif

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		CG_DrawIntermission();
		CG_DrawTVOverlay();
		return;
	}

/*
	if (cg.cameraMode) {
		return;
	}
*/
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		CG_DrawSpectator();

#if 0
		if(stereoFrame == STEREO_CENTER)
			CG_DrawCrosshair();
#endif

		CG_DrawCrosshairNames();
	} else {
		// don't draw any status if dead or the scoreboard is being explicitly shown
		if ( !cg.showScores && cg.snap->ps.stats[STAT_HEALTH] > 0 ) {

			// If weapon selector is active, check whether draw HUD
			if (cg.weaponSelectorTime != 0 && trap_Cvar_VariableValue("vr_weaponSelectorWithHud") == 0) {
				return;
			}

#ifdef MISSIONPACK
			if ( trap_Cvar_VariableValue( "vr_currentHudDrawStatus" ) != 0.0f ) {
				Menu_PaintAll();
				CG_DrawTimedMenus();
			}
#else
			CG_DrawStatusBar();
#endif
      
			CG_DrawAmmoWarning();

#ifdef MISSIONPACK
			CG_DrawProxWarning();
#endif      
#if 0
			if(stereoFrame == STEREO_CENTER)
				CG_DrawCrosshair();
#endif
			CG_DrawCrosshairNames();
			CG_DrawWeaponSelect();

#ifndef MISSIONPACK
			CG_DrawHoldableItem();
#else
			//CG_DrawPersistantPowerup();
#endif
			CG_DrawReward();
		}
	}

	if ( cgs.gametype >= GT_TEAM ) {
#ifndef MISSIONPACK
		CG_DrawTeamInfo();
#endif
	}

	CG_DrawLagometer();

#ifdef MISSIONPACK
	if (!cg_paused.integer) {
		CG_DrawUpperRight();
	}
#else
	CG_DrawUpperRight();
#endif

#ifndef MISSIONPACK
	CG_DrawLowerRight();
	CG_DrawLowerLeft();
#endif

	if ( !CG_DrawFollow() ) {
		CG_DrawWarmup();
	}
	CG_DrawTVOverlay();

	// don't draw center string if scoreboard is up
	cg.scoreBoardShowing = CG_DrawScoreboard();
	if ( !cg.scoreBoardShowing) {
		CG_DrawCenterString();
	}

#ifndef MISSIONPACK
	// Draw scoreboard cursor if scoreboard is active (Team Arena has its own cursor handling)
	if ( cgs.score_catched ) {
		float x, y, w, h;
		trap_R_SetColor( NULL );
		x = cgs.cursorX - 12;
		y = cgs.cursorY - 12;
		w = 24;
		h = 24;
		CG_AdjustFrom640( &x, &y, &w, &h );
		trap_R_DrawStretchPic( x, y, w, h, 0, 0, 1, 1, cgs.media.scoreboardCursor );
	}
#endif
}

/*
=================
CG_DrawHUD2DMinimal - Draws minimal 2D HUD elements for weapon zoomed state
There are some checks here that are  overkill for current use case, given
the current usage for specifically vr->weapon_zoomed, but keeping the checks
more or less identical to non-minimal HUD, just in case.
=================
*/
static void CG_DrawHUD2DMinimal(void)
{
	// If the HUD is disabled, we don't want this content
	if ( trap_Cvar_VariableValue( "vr_currentHudDrawStatus" ) == 0.0f ) {
		return;
	}

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		return;
	}

	// Skip if spectator - no minimal HUD needed
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		CG_DrawCrosshairNames();
		return;
	}

	// don't draw any status if dead or the scoreboard is being explicitly shown
	if ( !cg.showScores && cg.snap->ps.stats[STAT_HEALTH] > 0 ) {
		CG_DrawAmmoWarning();
		CG_DrawCrosshairNames();
		CG_DrawReward();
	}

	CG_DrawLagometer();

#ifdef MISSIONPACK
	if (!cg_paused.integer) {
		CG_DrawUpperRight();
	}
#else
	CG_DrawUpperRight();
#endif

#ifndef MISSIONPACK
	CG_DrawLowerRight();
	CG_DrawLowerLeft();
#endif

	CG_DrawWarmup();

	// don't draw center string if scoreboard is up
	if ( !cg.scoreBoardShowing ) {
		CG_DrawCenterString();
	}
}

/*
=================
CG_DrawScreen2D - Draws 2D elements always intended for the screen
=================
*/
static void CG_DrawScreen2D(void)
{
	// if we are taking a levelshot for the menu, don't draw anything
	if ( cg.levelShot ) {
		return;
	}

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		return;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR   &&
	    !cg.showScores && cg.snap->ps.stats[STAT_HEALTH] > 0 ) {

        CG_DrawVignette();

        // Draw modern damage indicator if enabled
        if ( cg_damageEffect.integer && !cg.renderingThirdPerson ) {
            CG_DamageBorderVignette();
        }

        if(vr->weapon_zoomed) {
            CG_DrawWeapReticle();
        }
    }

	// Weapon adjustment overlay (drawn regardless of team/health since we auto-exit on those)
	CG_WeaponAdjustDraw();
}

// OpenGL workaround: Render an empty scene to fix out-of-body issue when HUD isn't drawn
static void CG_EmptySceneHackHackHack( void )
{
	refdef_t refdef;
	memset( &refdef, 0, sizeof( refdef ) );

	refdef.rdflags = RDF_NOWORLDMODEL;
	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = 0;
	refdef.y = 0;
	refdef.width = cgs.glconfig.vidWidth;
	refdef.height = cgs.glconfig.vidHeight;

	refdef.time = cg.time;

	trap_R_ClearScene();
	trap_R_RenderScene( &refdef );
}

static void CG_CalculatePing( void ) {
	int count, i, v;

	cg.meanPing = 0;

	for ( i = 0, count = 0; i < LAG_SAMPLES; i++ ) {

		v = lagometer.snapshotSamples[i];
		if ( v >= 0 ) {
			cg.meanPing += v;
			count++;
		}

	}

	if ( count ) {
		cg.meanPing /= count;
	}
}

static void CG_WarmupEvents( void ) {

	int	count;

	if ( !cg.warmup )
		return;

	if ( cg.warmup < 0 ) {
		cg.warmupCount = -1;
		return;
	}

	if ( cg.warmup < cg.time ) {
		cg.warmup = 0;
		count = 0;
	} else {
		count = ( cg.warmup - cg.time + 999 ) / 1000;
	}

	if ( cg.warmupCount == -2 && cg.demoPlayback ) {
		cg.warmupCount = 0;
	}

	if ( cg.warmupCount == count ) {
		return;
	}

	cg.warmupCount = count;
	cg.timelimitWarnings = 0;

	switch ( count ) {
		case 0:
			if ( cg.warmupFightSound <= cg.time ) {
				trap_S_StartLocalSound( cgs.media.countFightSound, CHAN_ANNOUNCER );
				cg.warmupFightSound = cg.time + 750;
			}
			CG_CenterPrint( "FIGHT!", 120, GIANTCHAR_WIDTH*2 );
			break;

		case 1:
			trap_S_StartLocalSound( cgs.media.count1Sound, CHAN_ANNOUNCER );
			break;

		case 2:
			trap_S_StartLocalSound( cgs.media.count2Sound, CHAN_ANNOUNCER );
			break;

		case 3:
			trap_S_StartLocalSound( cgs.media.count3Sound, CHAN_ANNOUNCER );
			break;

		default:
			break;
	}
}



/*
===============
CG_ResetSeekState

Clear all transient state that goes stale after a TVD seek.
Most fields use (cg.time - startTime) comparisons in draw code;
after a seek cg.time jumps discontinuously, so stale timestamps
produce incorrect durations.  Other state (scoreboard flags,
sound buffer, VR smoothing) is simply invalid in the new timeline.

Called from the tv_seek_sync server command handler (all seeks)
and the backward-seek detector in cg_snapshot.c.

See also: CG_WarmupEvent (similar subset for warmup/client changes)
          CG_ResetViewOffsets (view bob/damage, called internally)
===============
*/
void CG_ResetSeekState( void ) {

	// -- HUD display timers --

	// Medal display — CG_DrawReward: CG_FadeColor(rewardTime, 3000)
	cg.rewardStack = 0;
	cg.rewardTime = 0;

	// Center print — CG_DrawCenterString: CG_FadeColor(centerPrintTime, 1000*cg_centertime)
	cg.centerPrintTime = 0;

	// Item pickup notification — CG_DrawPickupItem: CG_FadeColorTime(itemPickupTime, 3000, 250)
	cg.itemPickupTime = 0;
	cg.itemPickupBlendTime = 0;

	// Weapon select bar — CG_DrawWeaponSelect: CG_FadeColor(weaponSelectTime, 1400)
	cg.weaponSelectTime = 0;

	// Attacker head — CG_DrawAttacker: (cg.time - attackerTime) vs 10000
	cg.attackerTime = 0;

	// Killer name display
	cg.killerTime = 0;

	// Crosshair target name — CG_DrawCrosshairNames: CG_FadeColor(crosshairClientTime, 1000)
	cg.crosshairClientTime = 0;

	// Powerup icon pulse — CG_DrawStatusBar: (cg.time - powerupTime) vs PULSE_TIME
	cg.powerupTime = 0;

	// Voice chat menu — CG_DrawTimedMenus: (cg.time - voiceTime) vs 2500
	cg.voiceTime = 0;

	// Download finish animation — (cg.time - downloadFinishTime) vs 1500
	cg.downloadFinishTime = 0;

	// Auto-follow killer — CG_DrawActiveFrame: (followTime < cg.time)
	// Shouldn't actually be used during TV playback, but technically stale.
	cg.followTime = 0;

	// Low ammo warning — not time-based but stale across seeks
	cg.lowAmmoWarning = 0;

	// Damage vignette — CG_DamageBlendBlob: (cg.time - damageTime) vs DAMAGE_TIME
	cg.damageTime = 0;

	// Scoreboard
	cg.showScores = qfalse;
	cg.scoreFadeTime = 0;
	cg.scoreBoardShowing = qfalse;
	CG_SetScoreCatcher( qfalse );

	// -- Non-HUD time-dependent state --

	// View bob, damage kick, weapon kick offsets
	CG_ResetViewOffsets();

	// Flush queued announcer sounds (e.g. stale "Excellent!")
	CG_AddBufferedSound( -1 );

	// Force VR head-tracking EMA to re-seed from the new timeline
	cg.vrViewInitialized = qfalse;
}


// will be called on warmup end and when client changed
void CG_WarmupEvent( void ) {

	cg.attackerTime = 0;
	cg.attackerName[0] = '\0';

	cg.itemPickupTime = 0;
	cg.itemPickupBlendTime = 0;
	cg.itemPickupCount = 0;

	cg.killerTime = 0;
	cg.killerName[0] = '\0';
	
	cg.damageTime = 0;

	cg.rewardStack = 0;
	cg.rewardTime = 0;
	
	cg.weaponSelectTime = cg.time;

	cg.lowAmmoWarning = 0;

	cg.followTime = 0;
}


/*
=====================
CG_ApplyClientChange

Called each time client team changed
=====================
*/
static void CG_ApplyClientChange( void ) {
	CG_WarmupEvent();
	CG_ForceModelChange();
}


/*
=====================
CG_TrackClientTeamChange

Detects when local player changes team and refreshes all client models
=====================
*/
void CG_TrackClientTeamChange( void ) {
	static int spec_client = -1;
	static int spec_team = -1;
	static int curr_team = -1;

	int		ti; // team from clientinfo
	int		tp; // persistant team from snapshot

	if ( !cg.snap )
		return;

	tp = cg.snap->ps.persistant[ PERS_TEAM ];
	ti = cgs.clientinfo[ cg.snap->ps.clientNum ].team;

	if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW) && tp != TEAM_SPECTATOR ) {
		ti = tp; // use team from persistant info
	}

	// team changed
	if ( curr_team != ti )
	{
		curr_team = ti;
		spec_client = cg.snap->ps.clientNum;
		spec_team = tp;

		if ( spec_team == TEAM_SPECTATOR )
			spec_team = curr_team;

		CG_ApplyClientChange();
		CG_ResetPlayerEntity( &cg.predictedPlayerEntity );
		return;
	}

	if ( curr_team == TEAM_SPECTATOR )
	{
		if ( spec_team != tp )
		{
			spec_team  = tp;
			spec_client = cg.snap->ps.clientNum;

			CG_ApplyClientChange();
			CG_ResetPlayerEntity( &cg.predictedPlayerEntity );
			return;
		}

		if ( cgs.gametype >= GT_TEAM )
		{
			spec_client = cg.snap->ps.clientNum;
			return;
		}
		// pass through to spec client checks
	}

	if ( spec_client != cg.snap->ps.clientNum )
	{
		spec_client = cg.snap->ps.clientNum;
		spec_team = tp;

		if ( spec_team == TEAM_SPECTATOR )
			spec_team = cgs.clientinfo[ cg.snap->ps.clientNum ].team;

		CG_ApplyClientChange();
		CG_ResetPlayerEntity( &cg.predictedPlayerEntity );
	}
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive( void ) {
	// optionally draw the info screen instead
	if ( !cg.snap ) {
		CG_DrawInformation();
		return;
	}

	if ( !cg.demoPlayback ) {
		CG_CalculatePing();
	}

	// optionally draw the tournement scoreboard instead
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR &&
		( cg.snap->ps.pm_flags & PMF_SCOREBOARD ) ) {
		CG_DrawTourneyScoreboard();
		return;
	}

	// clear around the rendered view if sized down
	CG_TileClear();

	if(!vr->weapon_zoomed && (!vr->virtual_screen || vr->first_person_following))
		CG_DrawCrosshair3D();

	// offset vieworg appropriately if we're doing stereo separation
	vec3_t baseOrg;
	VectorCopy( cg.refdef.vieworg, baseOrg );

	float heightOffset = 0.0f;
	float worldscale = cg.worldscale;
	if ( CG_IsThirdPersonFollowMode(VRFM_THIRDPERSON_1) )
	{
		worldscale *= SPECTATOR_WORLDSCALE_MULTIPLIER;
		trap_Cvar_SetValue("vr_worldscaleScaler", SPECTATOR_WORLDSCALE_MULTIPLIER);
		//Just move camera down about 20cm
		heightOffset = -0.2f;
	}
	else if (CG_IsDeathCam() || CG_IsThirdPersonFollowMode(VRFM_THIRDPERSON_2))
	{
		worldscale *= SPECTATOR2_WORLDSCALE_MULTIPLIER;
		trap_Cvar_SetValue("vr_worldscaleScaler", SPECTATOR2_WORLDSCALE_MULTIPLIER);
		//Just move camera down about 50cm
		heightOffset = -0.5f;
	}
	else
	{
		float zoomCoeff =  ((2.5f-vr->weapon_zoomLevel)/1.5f); // normally 1.0
		trap_Cvar_SetValue("vr_worldscaleScaler", zoomCoeff);
	}

	if (vr->virtual_screen || CG_IsDeathCam() || CG_IsThirdPersonFollowMode(VRFM_QUERY))
	{
		// Do nothing to view height if we are viewing the virtual screen or in a camera mode
		// view is already positioned correctly
	}
	else
	{
		cg.refdef.vieworg[2] -= PLAYER_HEIGHT;
		cg.refdef.vieworg[2] += (vr->hmdposition[1] + heightOffset) * worldscale;
	}

	if (vr->use_fake_6dof && !vr->virtual_screen)
	{
		//If running multiplayer, allow some amount of faked positional tracking
		if (cg.snap->ps.stats[STAT_HEALTH] > 0 &&
		    //Don't use fake positional if following another player  - this is handled in  the
		    //VR third person code
		    !( cg.demoPlayback || CG_IsThirdPersonFollowMode(VRFM_QUERY)))
		{
			vec3_t pos, hmdposition, vieworg;
			VectorClear(pos);
			VectorSubtract(vr->hmdposition, vr->hmdorigin, hmdposition);

			float angleYaw = SHORT2ANGLE(cg.predictedPlayerState.delta_angles[YAW]) + (vr->clientviewangles[YAW] - vr->hmdorientation[YAW]);
			rotateAboutOrigin(hmdposition[2], hmdposition[0], angleYaw, pos);
			VectorScale(pos, worldscale, pos);
			VectorSubtract(cg.refdef.vieworg, pos, vieworg);

			//Prevent player clipping through solid objects
			trace_t trace;
			vec3_t			mins = {-8, -8, -8};
			vec3_t			maxs = {8, 8, 8};
			CG_Trace(&trace, cg.refdef.vieworg, mins, maxs, vieworg, cg.snap->ps.clientNum, CONTENTS_SOLID|CONTENTS_BODY);

			VectorCopy(trace.endpos, cg.refdef.vieworg);
		}
	}

	// Draw the HUD sprite in the world (HUD mode 1)
	// Also used for SP intermission UI (world-locked at podium position)
	qboolean isSPIntermission = (cg.snap->ps.pm_type == PM_INTERMISSION) &&
	                            (cgs.gametype == GT_SINGLE_PLAYER);
	qboolean drawHUDSprite = (trap_Cvar_VariableValue("vr_currentHudDrawStatus") != 2.0f &&
	                          !vr->weapon_zoomed && !vr->virtual_screen) || isSPIntermission;

	if (drawHUDSprite)
	{
		refEntity_t ent;
		vec3_t endpos, angles;
		vec3_t forward, right, up;
		vec3_t spriteAxis[3];  // For world-oriented sprites
		qboolean worldOrientedSprite = qfalse;

		float scale = trap_Cvar_VariableValue("vr_worldscaleScaler");
		// Distance formula: depth 0-5 maps to 9-54 units (before worldscale)
		float dist = (trap_Cvar_VariableValue("vr_currentHudDepth") + 1) * 9 * scale;
		float radius = (dist / 3.0f) * trap_Cvar_VariableValue("vr_hudScale");

		if (isSPIntermission)
		{
			// SP intermission: position HUD sprite at podium (world-locked)
			// Use the absolute world position calculated in CG_CalculateSPIntermissionHUD
			VectorCopy(vr->sp_intermission_hud_origin, endpos);

			// Use pre-calculated fixed radius (computed once in CG_CalculateSPIntermissionHUD)
			// This ensures the HUD doesn't resize when leaning forward/backward
			radius = vr->sp_intermission_hud_radius;

			// Orient sprite to face the viewer
			angles[YAW] = cg.snap->ps.viewangles[YAW];
			angles[PITCH] = 0;
			angles[ROLL] = 0;
			AngleVectors(angles, forward, right, up);

			// Store orientation for world-anchored rendering (fixed orientation, no billboarding)
			worldOrientedSprite = qtrue;
			VectorCopy(forward, spriteAxis[0]);
			VectorNegate(right, spriteAxis[1]);  // Negate: sprite expects "left", not "right"
			VectorCopy(up, spriteAxis[2]);
		}
		else if (cg.snap->ps.stats[STAT_HEALTH] > 0 &&
		         cg.snap->ps.pm_type != PM_INTERMISSION &&
		         !(cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW)))
		{
			// Normal gameplay: account for the yaw of the player vs worldspace
			static float hmd_yaw_x = 0.0f;
			static float hmd_yaw_y = 1.0f;
			static float prevPitch = 0.0f;

			// Smooth only the HMD orientation
			hmd_yaw_x = 0.95f * hmd_yaw_x + 0.05f * cosf(DEG2RAD(vr->hmdorientation[YAW]));
			hmd_yaw_y = 0.95f * hmd_yaw_y + 0.05f * sinf(DEG2RAD(vr->hmdorientation[YAW]));

			if (vr->use_fake_6dof)
			{
				// Multiplayer: use clientviewangles logic
				float viewYaw = SHORT2ANGLE(cg.predictedPlayerState.delta_angles[YAW]) +
				    (vr->clientviewangles[YAW] - vr->hmdorientation[YAW]);
				angles[YAW] = viewYaw + RAD2DEG(atan2(hmd_yaw_y, hmd_yaw_x));
			}
			else
			{
				// Single player: use refdefViewAngles - HMD offset + smoothed HMD
				angles[YAW] = cg.refdefViewAngles[YAW] - vr->hmdorientation[YAW] + RAD2DEG(atan2(hmd_yaw_y, hmd_yaw_x));
			}

			angles[PITCH] = 0.95f * prevPitch + 0.05f * vr->hmdorientation[PITCH];
			prevPitch = angles[PITCH];
			angles[ROLL] = 0;
			AngleVectors(angles, forward, right, up);

			VectorMA(cg.refdef.vieworg, dist, forward, endpos);
			VectorMA(endpos, trap_Cvar_VariableValue("vr_hudYOffset") / 20, up, endpos);
		}
		else
		{
			// Lock to face
			VectorMA(cg.refdef.vieworg, dist, cg.refdef.viewaxis[0], endpos);
		}

		memset(&ent, 0, sizeof(ent));
		ent.reType = RT_SPRITE;
		ent.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON;

		if (worldOrientedSprite) {
			// Use world-oriented rendering (fixed orientation, no billboarding)
			ent.renderfx |= RF_WORLD_ORIENTED;
			AxisCopy(spriteAxis, ent.axis);
		}

		VectorCopy(endpos, ent.origin);

		ent.radius = radius;
		ent.invert = qtrue;
		ent.customShader = cgs.media.hudShader;

		trap_R_AddRefEntityToScene(&ent);
	}

	// draw 3D view
	trap_R_RenderScene( &cg.refdef );

	// Apply bloom now, BEFORE 2D drawing begins
	// This ensures bloom only affects the 3D scene, not UI elements
	trap_R_FinishBloom();

	VectorCopy( baseOrg, cg.refdef.vieworg );

	{
		float hudStatus = trap_Cvar_VariableValue( "vr_currentHudDrawStatus" );

		// Draw screen 2D overlays (vignette, damage effects, reticle) directly to XR swapchain
		CG_DrawScreen2D();

		if (vr->weapon_zoomed)
		{
			// Weapon zoomed: render minimal HUD with scaled coordinates
			cg.drawingHUD = qtrue;
			cg.drawingZoomedHUD = qtrue;
			CG_WarmupEvents();
			CG_DrawHUD2DMinimal();
			cg.drawingZoomedHUD = qfalse;
			cg.drawingHUD = qfalse;
		}
		else if (hudStatus == 2 && !vr->virtual_screen)
		{
			// HUD mode 2: render directly to main swapchain with stereo parallax
			cg.drawingHUD = qtrue;
			trap_R_HUDBufferStart(qfalse);
			CG_WarmupEvents();
			CG_DrawHUD2D();
			trap_R_HUDBufferEnd();
			cg.drawingHUD = qfalse;
		}

		if (!vr->weapon_zoomed && (!vr->virtual_screen || vr->first_person_following))
		{
			if (hudStatus != 0)
			{
				cg.drawingHUD = qtrue;

				if (hudStatus == 2 && vr->first_person_following)
				{
					// HUD mode 2 in first person following: direct-to-screen with stereo offset
					trap_R_HUDBufferStart(qfalse);
					CG_WarmupEvents();
					CG_DrawHUD2D();
					trap_R_HUDBufferEnd();
				}
				else if (hudStatus == 1)
				{
					// HUD mode 1: use existing HUD buffer (floating in-world)
					trap_R_HUDBufferStart(qtrue);
					CG_WarmupEvents();
					CG_DrawHUD2D();
					trap_R_HUDBufferEnd();
				}

				cg.drawingHUD = qfalse;
			}
			else
			{
				// HUD disabled - just clear the HUD buffer to remove any stale content
				trap_R_HUDBufferStart(qtrue);
				trap_R_HUDBufferEnd();
			}
		}
	}

	if (cg_usingOpenGL) {
		CG_EmptySceneHackHackHack();
	}
}