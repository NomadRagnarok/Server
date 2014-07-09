// Copyright (c) Hercules Dev Team, licensed under GNU GPL.
// See the LICENSE file
// CostumeItem Hercules Plugin
// Special Thanks for Mr. [Hercules/Ind]

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/HPMi.h"
#include "../common/mmo.h"
#include "../map/battle.h"
#include "../map/script.h"
#include "../map/status.h"
#include "../map/clif.h"
#include "../map/pc.h"
#include "../map/pet.h"
#include "../map/map.h"
#include "../map/itemdb.h"

#include "../common/HPMDataCheck.h" /* should always be the last file included! (if you don't make it last, it'll intentionally break compile time) */

/*
1.0 Initial Release [Mhalicot]
1.0a Fixed Typo (usage: @ci <item name/ID>) changed to (usage: @costumeitem <item name/ID>) thx to DP
1.2 Fixed Sinx Can't Equipt dagger/sword on both arms(L/R), Special Thanks to Haru for Help [Mhalicot]
*/
HPExport struct hplugin_info pinfo = {
	"costumeitem",		// Plugin name
	SERVER_TYPE_MAP,	// Which server types this plugin works with?
	"1.0",				// Plugin version
	HPM_VERSION,		// HPM Version (don't change, macro is automatically updated)
};

// Costume System
int reserved_costume_id = 999998;
void parse_my_setting(const char *val) {
	reserved_costume_id = atoi(val);
}

uint16 GetWord(uint32 val, int idx) {
	switch( idx ) {
	case 0: return (uint16)( (val & 0x0000FFFF)         );
	case 1: return (uint16)( (val & 0xFFFF0000) >> 0x10 );
	default:
#if defined(DEBUG)
#endif
		ShowDebug("GetWord: invalid index (idx=%d)\n", idx);
		return 0;
	}
}

uint32 MakeDWord(uint16 word0, uint16 word1) {
	return
		( (uint32)(word0        ) )|
		( (uint32)(word1 << 0x10) );
}

int HPM_map_reqnickdb(struct map_session_data * sd, int *char_id) {
	int i = *char_id;

	if( !sd ) return 0;

	if( reserved_costume_id && reserved_costume_id == i ) {
		clif->solved_charname(sd->fd, i, sd->status.name);
	}
	hookStop();
	return 1;
}

int HPM_pc_equippoint(int ret,struct map_session_data *sd,int *nn) { 
	
	int char_id = 0, n = *nn;

	if( !sd ) return 0;
	
	if( !ret ) return 0; // If the original function returned zero, we don't need to process it anymore

	if( reserved_costume_id &&
		sd->status.inventory[n].card[0] == CARD0_CREATE &&
		(char_id = MakeDWord(sd->status.inventory[n].card[2],sd->status.inventory[n].card[3])) == reserved_costume_id )
	{ // Costume Item - Converted
		if( ret&EQP_HEAD_TOP ) { ret &= ~EQP_HEAD_TOP; ret |= EQP_COSTUME_HEAD_TOP; }
		if( ret&EQP_HEAD_LOW ) { ret &= ~EQP_HEAD_LOW; ret |= EQP_COSTUME_HEAD_LOW; }
		if( ret&EQP_HEAD_MID ) { ret &= ~EQP_HEAD_MID; ret |= EQP_COSTUME_HEAD_MID; }
	}
	return ret;
}

ACMD(costumeitem) {
	char item_name[100];
	int item_id, flag = 0;
	struct item item_tmp;
	struct item_data *item_data;

	if( !sd ) return 0;

	if (!message || !*message || (
		sscanf(message, "\"%99[^\"]\"", item_name) < 1 && 
		sscanf(message, "%99s", item_name) < 1 )) {
 			clif->message(fd, "Please enter an item name or ID (usage: @costumeitem <item name/ID>).");
			return false;
	}

	if ((item_data = itemdb->search_name(item_name)) == NULL &&
	    (item_data = itemdb->exists(atoi(item_name))) == NULL) {
			clif->message(fd, "Invalid item ID or name.");
			return false;
	}

	if( !reserved_costume_id ) {
			clif->message(fd, "Costume conversion is disabled.");
			return false;
	}
	if( !(item_data->equip&EQP_HEAD_LOW) &&
		!(item_data->equip&EQP_HEAD_MID) &&
		!(item_data->equip&EQP_HEAD_TOP) &&
		!(item_data->equip&EQP_COSTUME_HEAD_LOW) &&
		!(item_data->equip&EQP_COSTUME_HEAD_MID) &&
		!(item_data->equip&EQP_COSTUME_HEAD_TOP) ) {
			clif->message(fd, "You cannot costume this item. Costume only work for headgears.");
			return false;
		}

	item_id = item_data->nameid;
	//Check if it's stackable.
	if (!itemdb->isstackable2(item_data)) {
		if( (item_data->type == IT_PETEGG || item_data->type == IT_PETARMOR) ) {
			clif->message(fd, "Cannot create costume pet eggs or pet armors.");
			return false;
		}
	}

	// if not pet egg
	if (!pet->create_egg(sd, item_id)) {
		memset(&item_tmp, 0, sizeof(item_tmp));
		item_tmp.nameid = item_id;
		item_tmp.identify = 1;
		item_tmp.card[0] = CARD0_CREATE;
		item_tmp.card[2] = GetWord(reserved_costume_id, 0);
		item_tmp.card[3] = GetWord(reserved_costume_id, 1);

		if ((flag = pc->additem(sd, &item_tmp, 1, LOG_TYPE_COMMAND)))
			clif->additem(sd, 0, 0, flag);
	}

	if (flag == 0)
		clif->message(fd,"item created.");
	return true;
}

/*==========================================
 * Costume Items Hercules/[Mhalicot]
 *------------------------------------------*/
BUILDIN(costume) {
	int i = -1, num, ep;
	TBL_PC *sd;

	num = script_getnum(st,2); // Equip Slot
	sd = script->rid2sd(st);

	if( sd == NULL )
		return 0;
	if( num > 0 && num <= ARRAYLENGTH(script->equip) )
		i = pc->checkequip(sd, script->equip[num - 1]);
	if( i < 0 )
		return 0;
	ep = sd->status.inventory[i].equip;
	if( !(ep&EQP_HEAD_LOW) && !(ep&EQP_HEAD_MID) && !(ep&EQP_HEAD_TOP) )
		return 0;

	logs->pick_pc(sd, LOG_TYPE_SCRIPT, -1, &sd->status.inventory[i],sd->inventory_data[i]);
	pc->unequipitem(sd,i,2);
	clif->delitem(sd,i,1,3);
	// --------------------------------------------------------------------
	sd->status.inventory[i].refine = 0;
	sd->status.inventory[i].attribute = 0;
	sd->status.inventory[i].card[0] = CARD0_CREATE;
	sd->status.inventory[i].card[1] = 0;
	sd->status.inventory[i].card[2] = GetWord(reserved_costume_id, 0);
	sd->status.inventory[i].card[3] = GetWord(reserved_costume_id, 1);

	if( ep&EQP_HEAD_TOP ) { ep &= ~EQP_HEAD_TOP; ep |= EQP_COSTUME_HEAD_TOP; }
	if( ep&EQP_HEAD_LOW ) { ep &= ~EQP_HEAD_LOW; ep |= EQP_COSTUME_HEAD_LOW; }
	if( ep&EQP_HEAD_MID ) { ep &= ~EQP_HEAD_MID; ep |= EQP_COSTUME_HEAD_MID; }
	// --------------------------------------------------------------------
	logs->pick_pc(sd, LOG_TYPE_SCRIPT, 1, &sd->status.inventory[i],sd->inventory_data[i]);

	clif->additem(sd,i,1,0);
	pc->equipitem(sd,i,ep);
	clif->misceffect(&sd->bl,3);

	return true;
}

/* triggered when server starts loading, before any server-specific data is set */
HPExport void server_preinit (void) {
	/* makes map server listen to mysetting:value in any "battleconf" file (including imported or custom ones) */
	/* value is not limited to numbers, its passed to our plugins handler (parse_my_setting) as const char *,
	 * and thus can be manipulated at will */
	addBattleConf("parse_my_setting",parse_my_setting);
};

/* Server Startup */
HPExport void plugin_init (void) {
	clif = GET_SYMBOL("clif");
	pet = GET_SYMBOL("pet");
	script = GET_SYMBOL("script");
	map = GET_SYMBOL("map");
	logs = GET_SYMBOL("logs");
	itemdb = GET_SYMBOL("itemdb");
	pc = GET_SYMBOL("pc");

		//Hook
	addHookPre("map->reqnickdb",HPM_map_reqnickdb);
	addHookPost("pc->equippoint",HPM_pc_equippoint);
	
		//atCommand
	addAtcommand("costumeitem",costumeitem);

		//scriptCommand
	addScriptCommand("costume","i",costume);
};
