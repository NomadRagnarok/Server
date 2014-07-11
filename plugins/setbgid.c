//===== Hercules Plugin ======================================
//= Battleground System without waitingroom
//===== By: ==================================================
//= AnnieRuru
//===== Current Version: =====================================
//= 1.3
//===== Compatible With: ===================================== 
//= Hercules 2014-02-23
//===== Description: =========================================
//= Introduce 3 new Script Commands
//= *createbgid = creates a battleground team
//= *setbgid = join a battleground team
//= *getbgusers = return an array $@arenamembers of battleground team
//===== Topic ================================================
//= http://hercules.ws/board/topic/4570-
//===== Additional Comments: =================================  
//= really frustrating to know someone put 'bg' but needs to read as "battlegrounds" <_<
//============================================================
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../common/HPMi.h"
#include "../map/script.h"
#include "../map/pc.h"
#include "../map/mapreg.h"
#include "../map/battleground.h"
 
#include "../common/HPMDataCheck.h" // should always be the last file included! (if you don't make it last, it'll intentionally break compile time)
 
HPExport struct hplugin_info pinfo = {
	"setbgid",		// Plugin name
	SERVER_TYPE_MAP,// Which server types this plugin works with?
	"0.1",			// Plugin version
	HPM_VERSION,	// HPM Version (don't change, macro is automatically updated)
};
 
//	createbgid <respawn map>, <respawn x>, <respawn y>, <On Quit event>, <On Death event>;
BUILDIN(createbgid) {
	unsigned int bg_id;
	if ( ( bg_id = bg->create( mapindex->name2id( script_getstr(st,2) ), script_getnum(st,3), script_getnum(st,4), script_getstr(st,5), script_getstr(st,6) ) ) > 0 )
		script_pushint( st, bg_id );
	else
		script_pushint( st, 0 );
	return true;
}
 
//	setbgid <battleground ID> {, <player name> };
//	setbgid <battleground ID> {, <player account ID> };
BUILDIN(setbgid) {
	unsigned int bg_id = script_getnum(st,2);
	struct battleground_data *bgd = bg->team_search( bg_id );
	struct map_session_data *sd;
	if ( script_hasdata( st, 3 ) ) {
		if ( data_isstring( script_getdata(st,3) ) )
			sd = map->nick2sd( script_getstr(st,3) );
		else
			sd = map->id2sd( script_getnum(st,3) );
	} else
		sd = script->rid2sd(st);
	if ( !sd ) {
		script_pushint( st, -3 ); // player no attach
		return true;
	}
	if ( !bgd && bg_id > 0 ) {
		script_pushint( st, -1 ); // battleground team haven't created
		return true;
	}
	if ( bg_id && sd->bg_id == bg_id ) {
		script_pushint( st, -5 ); // the player has already join this battleground team
		return true;
	}
	if ( sd->bg_id )
		bg->team_leave( sd, 0 );
	if ( bg_id == 0 ) {
		script_pushint( st, 0 );
		return true;
	}
	if ( !bg->team_join( bg_id, sd ) )
		script_pushint( st, -2 ); // cannot join anymore, because has reached MAX_BG_MEMBERS
	else
		script_pushint( st, bg_id );
	return true;
}
 
//	getbgusers <battleground ID>;
BUILDIN(getbgusers) {
	struct battleground_data *bgd = bg->team_search( script_getnum(st,2) );
	int i;
	if ( !bgd ) {
		script_pushint( st, -1 );
		return true;
	}
	for ( i = 0; bgd->members[i].sd; i++ )
		mapreg->setreg( reference_uid( script->add_str("$@arenamembers"), i ), bgd->members[i].sd->bl.id );
	mapreg->setreg( script->add_str("$@arenamembersnum"), i );
	script_pushint( st, i );
	return true;
}
 
HPExport void plugin_init (void) {
	script = GET_SYMBOL("script");
	mapreg = GET_SYMBOL("mapreg");
	bg = GET_SYMBOL("battlegrounds");
	mapindex = GET_SYMBOL("mapindex");
	map = GET_SYMBOL("map");
 
	addScriptCommand( "createbgid", "siiss", createbgid );
	addScriptCommand( "setbgid", "i?", setbgid );
	addScriptCommand( "getbgusers", "i", getbgusers );
}