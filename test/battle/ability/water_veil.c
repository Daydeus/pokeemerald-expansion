#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Water Veil prevents OHKO moves")
{
    GIVEN {
        ASSUME(gMovesInfo[MOVE_FISSURE].effect == EFFECT_OHKO);
        PLAYER(SPECIES_GOLDEEN) { Ability(ABILITY_WATER_VEIL); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponent, MOVE_FISSURE); }
    } SCENE {
        MESSAGE("Foe Wobbuffet used Fissure!");
        ABILITY_POPUP(player, ABILITY_WATER_VEIL);
        MESSAGE("Goldeen was protected by Water Veil!");
    } THEN {
        EXPECT_EQ(player->hp, player->maxHP);
    }
}

SINGLE_BATTLE_TEST("Water Veil prevents OHKOs")
{
    GIVEN {
        PLAYER(SPECIES_GOLDEEN) { Ability(ABILITY_WATER_VEIL); MaxHP(100); HP(100); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponent, MOVE_SEISMIC_TOSS); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SEISMIC_TOSS, opponent);
        HP_BAR(player, hp: 1);
        ABILITY_POPUP(player, ABILITY_WATER_VEIL);
        MESSAGE("Goldeen endured the hit using Water Veil!");
    }
}

SINGLE_BATTLE_TEST("Water Veil does not prevent non-OHKOs")
{
    GIVEN {
        PLAYER(SPECIES_GOLDEEN) { Ability(ABILITY_WATER_VEIL); MaxHP(100); HP(99); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponent, MOVE_SEISMIC_TOSS); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SEISMIC_TOSS, opponent);
        HP_BAR(player, hp: 0);
    }
}
