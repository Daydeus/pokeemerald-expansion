#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Magma Armor negates damage from Water-type moves")
{
    GIVEN {
        ASSUME(gMovesInfo[MOVE_BUBBLE].type == TYPE_WATER);
        PLAYER(SPECIES_SLUGMA) { Ability(ABILITY_MAGMA_ARMOR); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponent, MOVE_BUBBLE); }
    } SCENE {
        NONE_OF { HP_BAR(player); }
    }
}

SINGLE_BATTLE_TEST("Magma Armor increases Defense by one stage when hit by a Water-type move")
{
    GIVEN {
        ASSUME(gMovesInfo[MOVE_BUBBLE].type == TYPE_WATER);
        PLAYER(SPECIES_SLUGMA) { Ability(ABILITY_MAGMA_ARMOR); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponent, MOVE_BUBBLE); }
    } SCENE {
        ABILITY_POPUP(player, ABILITY_MAGMA_ARMOR);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, player);
        MESSAGE("Slugma's Defense rose!");
    }
}

SINGLE_BATTLE_TEST("Magma Armor does not increase Defense if already maxed")
{
    GIVEN {
        ASSUME(gMovesInfo[MOVE_BUBBLE].type == TYPE_WATER);
        PLAYER(SPECIES_SLUGMA) { Ability(ABILITY_MAGMA_ARMOR); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_IRON_DEFENSE);}
        TURN { MOVE(player, MOVE_IRON_DEFENSE);}
        TURN { MOVE(player, MOVE_IRON_DEFENSE);}
        TURN { MOVE(opponent, MOVE_BUBBLE); }
    } SCENE {
        ABILITY_POPUP(player, ABILITY_MAGMA_ARMOR);
        NONE_OF {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, player);
            MESSAGE("Slugma's Defense rose!");
        }
    }
}

SINGLE_BATTLE_TEST("Magma Armor blocks multi-hit Water-type moves")
{
    GIVEN {
        ASSUME(gMovesInfo[MOVE_WATER_SHURIKEN].type == TYPE_WATER);
        ASSUME(gMovesInfo[MOVE_WATER_SHURIKEN].effect == EFFECT_MULTI_HIT);
        PLAYER(SPECIES_SLUGMA) { Ability(ABILITY_MAGMA_ARMOR); }
        OPPONENT(SPECIES_POLIWHIRL) { Ability(ABILITY_SKILL_LINK); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_WATER_SHURIKEN); }
    } SCENE {
        MESSAGE("Foe Poliwhirl used Water Shuriken!");
        ABILITY_POPUP(player, ABILITY_MAGMA_ARMOR);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, player);
        MESSAGE("Slugma's Defense rose!");
        NONE_OF {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_WATER_SHURIKEN, opponent);
            HP_BAR(player);
            MESSAGE("Hit 5 time(s)!");
        }
    }
}

SINGLE_BATTLE_TEST("Magma Armor prevents Absorb Bulb and Luminous Moss from activating")
{
    u32 item;
    PARAMETRIZE { item = ITEM_ABSORB_BULB; }
    PARAMETRIZE { item = ITEM_LUMINOUS_MOSS; }
    GIVEN {
        ASSUME(gMovesInfo[MOVE_BUBBLE].type == TYPE_WATER);
        PLAYER(SPECIES_SLUGMA) { Ability(ABILITY_MAGMA_ARMOR); Item(item); }
        OPPONENT(SPECIES_POLIWHIRL);
    } WHEN {
        TURN { MOVE(opponent, MOVE_BUBBLE); }
    } SCENE {
        ABILITY_POPUP(player, ABILITY_MAGMA_ARMOR);
        NONE_OF {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
        }
    }
}
