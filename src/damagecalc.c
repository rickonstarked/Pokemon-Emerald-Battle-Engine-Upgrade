#include "types.h"
#include "defines.h"
#include "battle_locations.h"
#include "battle_structs.h"
#include "vanilla_functions.h"
#include "battlescripts.h"
#include "new_battle_struct.h"
#include "move_table.h"

#define MOVE_PHYSICAL 0
#define MOVE_SPECIAL 1
#define MOVE_STATUS 2

struct natural_gift{
    u8 move_power;
    u8 move_type;
};

struct natural_gift natural_gift_table[] = { {0xFF, 0} ,{80, TYPE_FIRE}, {80, TYPE_WATER}, {80, TYPE_ELECTRIC}, {80, TYPE_GRASS}, {80, TYPE_ICE}, {80, TYPE_FIGHTING}, {80, TYPE_POISON}, {80, TYPE_GROUND}, {80, TYPE_FLYING}, {80, TYPE_PSYCHIC}, {80, TYPE_BUG}, {80, TYPE_ROCK}, {80, TYPE_GHOST}, {80, TYPE_DRAGON}, {80, TYPE_DARK}, {80, TYPE_STEEL}, {90, TYPE_FIRE}, {90, TYPE_WATER}, {90, TYPE_ELECTRIC}, {90, TYPE_GRASS}, {90, TYPE_ICE}, {90, TYPE_FIGHTING}, {90, TYPE_POISON}, {90, TYPE_GROUND}, {90, TYPE_FLYING}, {90, TYPE_PSYCHIC}, {90, TYPE_BUG}, {90, TYPE_ROCK}, {90, TYPE_GHOST}, {90, TYPE_DRAGON}, {90, TYPE_DARK}, {90, TYPE_STEEL}, {100, TYPE_FIRE}, {100, TYPE_WATER}, {100, TYPE_ELECTRIC}, {100, TYPE_FIGHTING}, {100, TYPE_POISON}, {100, TYPE_GROUND}, {100, TYPE_FLYING}, {100, TYPE_PSYCHIC}, {100, TYPE_BUG} };;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

struct fling{
    u16 item_id;
    u8 move_power;
    u8 move_effect;
};

struct fling fling_table[] = { {0xFFFF, 0, 0} };;

struct stat_fractions{
    u8 dividend;
    u8 divisor;
};

struct stat_fractions stat_buffs[] = { {2, 8}, {2, 7}, {2, 6}, {2, 5}, {2, 4}, {2, 3}, {2, 2}, {3, 2}, {4, 2}, {5, 2}, {6, 2}, {7, 2}, {8, 2} };;;;;;;;;;;;;;

u32 percent_boost(u32 number, u16 percent)
{
    return __udivsi3(number * (100 + percent), 100);
}

u32 percent_lose(u32 number, u16 percent)
{
    return __udivsi3(number * (100 - percent), 100);
}

u16 chain_modifier(u16 curr_modifier, u16 new_modifier)
{
    u16 updated_modifier;
    if (curr_modifier == 0)
    {
        updated_modifier = new_modifier;
    }
    else
    {
        u32 calculations = (curr_modifier * new_modifier + 0x800) >> 12;
        updated_modifier = calculations;
    }
    return updated_modifier;
}

u32 apply_modifier(u16 modifier, u32 value)
{
    u32 multiplication = modifier * value;
    u32 anding_result = 0xFFF & multiplication;
    u32 new_value = multiplication >> 0xC;
    if (anding_result > 0x800)
    {
        new_value++;
    }
    return new_value;
}

u16 percent_to_modifier(u8 percent) //20 gives exactly 0x1333, 30 is short on 1
{
    return 0x1000 + __udivsi3(percent * 819, 20);
}

u16 get_poke_weight(u8 bank)
{
    u16 poke_weight = get_height_or_weight(species_to_national_dex(battle_participants[bank].poke_species), 1);
    if (has_ability_effect(bank, 1, 1))
    {
        switch (battle_participants[bank].ability_id)
        {
            case ABILITY_HEAVY_METAL:
                poke_weight *= 2;
                break;
            case ABILITY_LIGHT_METAL:
                poke_weight >>= 1;
                break;
        }
    }
    if (get_item_effect(bank, 1) == ITEM_EFFECT_FLOATSTONE)
    {
        poke_weight *= 2;
    }

    poke_weight -= 1000 * new_battlestruct.ptr->bank_affecting[bank].autonomize_uses;
    if (poke_weight < 1)
        poke_weight = 1;

    return poke_weight;
}

u8 count_stat_increases(u8 bank, u8 eva_acc)
{
    u8* stat_ptr = &battle_participants[bank].atk_buff;
    u8 increases = 0;
    for (u8 i = 0; i < 5; i++)
    {
        stat_ptr += i;
        if (*stat_ptr > 6)
        {
            increases += *stat_ptr - 6;
        }
    }
    if (eva_acc)
    {
        if (*stat_ptr > 6)
        {
            increases += *stat_ptr - 6;
        }
        if (*(stat_ptr + 1) > 6)
        {
            increases += *stat_ptr - 6;
        }
    }
    return increases;
}

u16 get_speed(u8 bank)
{
    u16 speed = battle_participants[bank].spd;
    //take items into account
    switch (get_item_effect(bank, 1))
    {
    case ITEM_EFFECT_IRONBALL:
        speed >>= 1;
        break;
    case ITEM_EFFECT_CHOICESCARF:
        speed = percent_boost(speed, 50);
        break;
    }
    //take abilities into account
    if (has_ability_effect(bank, 0, 1))
    {
        u8 weather_effect = weather_abilities_effect();
        switch (battle_participants[bank].ability_id)
        {
        case ABILITY_CHLOROPHYLL:
            if (weather_effect && (battle_weather.flags.harsh_sun || battle_weather.flags.permament_sun || battle_weather.flags.sun))
            {
                speed *= 2;
            }
            break;
        case ABILITY_SWIFT_SWIM:
            if (weather_effect && (battle_weather.flags.rain || battle_weather.flags.downpour || battle_weather.flags.permament_rain || battle_weather.flags.heavy_rain))
            {
                speed *= 2;
            }
            break;
        case ABILITY_SAND_RUSH:
            if (weather_effect && (battle_weather.flags.sandstorm || battle_weather.flags.permament_sandstorm))
            {
                speed *= 2;
            }
            break;
        case ABILITY_QUICK_FEET:
            if (battle_participants[bank].status.flags.poison || battle_participants[bank].status.flags.toxic_poison || battle_participants[bank].status.flags.burn)
            {
                speed *= 2;
            }
            else if (battle_participants[bank].status.flags.paralysis)
            {
                speed *= 4;
            }
            break;
        }
    }
    //paralysis
    if (battle_participants[bank].status.flags.paralysis)
    {
        speed >>= 1;
    }
    //tailwind
    if (new_battlestruct.ptr->side_affecting[is_bank_from_opponent_side(bank)].tailwind)
    {
        speed *= 2;
    }

    speed = __udivsi3(speed * stat_buffs[battle_participants[bank].spd_buff].dividend, stat_buffs[battle_participants[bank].spd_buff].divisor);

    return speed;
}

u16 get_base_power(u16 move, u8 atk_bank, u8 def_bank)
{
    u16 base_power = move_table[move].base_power;
    u8 atk_banks_side = get_battle_side(atk_bank) & 1;
    u8 atk_ally_bank = atk_bank ^ 2;
    switch (move)
    {
        case MOVE_GRASS_PLEDGE:
            if (chosen_move_by_banks[atk_ally_bank] == MOVE_FIRE_PLEDGE || chosen_move_by_banks[atk_ally_bank] == MOVE_WATER_PLEDGE)
            {
                base_power = 150;
            }
            break;
        case MOVE_FIRE_PLEDGE:
            if (chosen_move_by_banks[atk_ally_bank] == MOVE_GRASS_PLEDGE || chosen_move_by_banks[atk_ally_bank] == MOVE_WATER_PLEDGE)
            {
                base_power = 150;
            }
            break;
        case MOVE_WATER_PLEDGE:
            if (chosen_move_by_banks[atk_ally_bank] == MOVE_FIRE_PLEDGE || chosen_move_by_banks[atk_ally_bank] == MOVE_GRASS_PLEDGE)
            {
                base_power = 150;
            }
            break;
        case MOVE_FLING: //make fling table; before calling damage calc should check if can use this, move effect is applied here
            {            //item deletion should happen elsewhere
                for (u16 i = 0; fling_table[i].item_id != 0xFFFF; i++)
                {
                    if (fling_table[i].item_id == battle_participants[atk_bank].held_item)
                    {
                        base_power = fling_table[i].move_power;
                        battle_communication_struct.move_effect = fling_table[i].move_effect;
                        break;
                    }
                }
                break;
            }
        case MOVE_ROLLOUT:
        case MOVE_MAGNITUDE:
        case MOVE_PRESENT:
        case MOVE_FURY_CUTTER:
        case MOVE_FLAIL:
        case MOVE_REVERSAL:
        case MOVE_ERUPTION:
        case MOVE_WATER_SPOUT:
        case MOVE_RETURN:
        case MOVE_FRUSTRATION:
        case MOVE_SPIT_UP:
        case MOVE_TRIPLE_KICK:
            if (dynamic_base_power)
            {
                base_power = dynamic_base_power;
            }
            break;
        case MOVE_REVENGE:
        case MOVE_AVALANCHE:
        case MOVE_SMELLING_SALTS:
        case MOVE_WEATHER_BALL:
            base_power *= battle_scripting.damage_multiplier;
            break;
        case MOVE_NATURAL_GIFT: //checking for held item and the capability of using an item should happen before damage calculation
            {                   //dynamic type will be set here
                u8 berryID = itemid_to_berryid(battle_participants[atk_bank].held_item);
                base_power = natural_gift_table[berryID].move_power;
                battle_stuff_ptr.ptr->dynamic_move_type = natural_gift_table[berryID].move_type + 0x80;
            }
            break;
        case MOVE_WAKEUP_SLAP:
            if (battle_participants[def_bank].status.flags.sleep)
            {
                base_power *= 2;
            }
            break;
        case MOVE_WRING_OUT:
        case MOVE_CRUSH_GRIP:
            {
                u16 wringout_power = __udivsi3(battle_participants[def_bank].current_hp, battle_participants[def_bank].max_hp);
                wringout_power = 120 * wringout_power;
                if (wringout_power < 1)
                {
                    wringout_power = 1;
                }
                base_power = wringout_power;
                break;
            }
        case MOVE_HEX:
            if (battle_participants[def_bank].status.int_status)
            {
                base_power *= 2;
            }
            break;
        case MOVE_ASSURANCE:
            if (special_statuses[def_bank].moveturn_losthp_physical || special_statuses[def_bank].moveturn_losthp_special)
            {
                base_power *= 2;
            }
            break;
        case MOVE_TRUMP_CARD:
            {
                s8 pp_slot = get_move_position(atk_bank, MOVE_TRUMP_CARD);
                u8 pp;
                if (pp_slot == -1)
                {
                    pp = 4;
                }
                else
                {
                    pp = battle_participants[atk_bank].current_pp[pp_slot];
                }
                switch (pp)
                {
                case 0:
                    base_power = 200;
                    break;
                case 1:
                    base_power = 80;
                    break;
                case 2:
                    base_power = 60;
                    break;
                case 3:
                    base_power = 50;
                    break;
                default:
                    base_power = 40;
                    break;
                }
                break;
            }
        case MOVE_ACROBATICS:
            if (!battle_participants[atk_bank].held_item)
            {
                base_power *= 2;
            }
            break;
        case MOVE_LOW_KICK:
        case MOVE_GRASS_KNOT:
            {
                u16 defender_weight = get_poke_weight(def_bank);
                if (defender_weight < 100)
                    base_power = 20;
                else if (defender_weight < 250)
                    base_power = 40;
                else if (defender_weight < 500)
                    base_power = 60;
                else if (defender_weight < 1000)
                    base_power = 80;
                else if (defender_weight < 2000)
                    base_power = 100;
                else
                    base_power = 120;
                break;
            }
        case MOVE_HEAT_CRASH:
        case MOVE_HEAVY_SLAM:
            {
                u16 weight_difference = __udivsi3(get_poke_weight(atk_bank), get_poke_weight(def_bank));

                if (weight_difference >= 5)
                    base_power = 120;
                else if (weight_difference == 4)
                    base_power = 100;
                else if (weight_difference == 3)
                    base_power = 80;
                else if (weight_difference == 2)
                    base_power = 60;
                else
                    base_power = 40;
                break;
            }
        case MOVE_PUNISHMENT:
            base_power = 60 + 20 * count_stat_increases(def_bank, 0);
            if (base_power > 200)
                base_power = 200;
            break;
        case MOVE_STORED_POWER:
            base_power = base_power + 20 * count_stat_increases(atk_bank, 1);
            break;
        case MOVE_ELECTRO_BALL:
            switch (__udivsi3(get_speed(atk_bank), get_speed(def_bank)))
                {
                case 0:
                    base_power = 40;
                    break;
                case 1:
                    base_power = 60;
                    break;
                case 2:
                    base_power = 80;
                case 3:
                    base_power = 120;
                default:
                    base_power = 150;
                    break;
                }
            break;
        case MOVE_GYRO_BALL:
            base_power =  __udivsi3(25 * get_speed(def_bank), get_speed(atk_bank)) + 1;
            if (base_power > 150)
                base_power = 150;
            break;
        case MOVE_ECHOED_VOICE:
            base_power += 40 * new_battlestruct.ptr->side_affecting[atk_banks_side].echo_voice_counter;
            if (base_power > 200)
                base_power = 200;
            break;
        case MOVE_PAYBACK:
            if (get_bank_turn_order(def_bank) < turn_order)
            {
                base_power *= 2;
            }
            break;
        case MOVE_GUST:
        case MOVE_TWISTER:
            if (new_battlestruct.ptr->bank_affecting[def_bank].sky_drop_attacker || new_battlestruct.ptr->bank_affecting[def_bank].sky_drop_target || status3[def_bank].on_air)
            {
                base_power *= 2;
            }
            break;
        case MOVE_ROUND:
            if (chosen_move_by_banks[atk_ally_bank] == MOVE_ROUND)
            {
                base_power *= 2;
            }
            break;
        case MOVE_PURSUIT: //not sure where the game applies boost, whether in the damage multiplier or in the dynamic base power location
            break;
    }
    return base_power;
}

u8 find_move_in_table(u16 move, u16 table_ptr[])
{
    for (u8 i = 0; table_ptr[i] != 0xFFFF; i++)
    {
        if (table_ptr[i] == move)
        {
            return true;
        }
    }
    return false;
}

u16 apply_base_power_modifiers(u16 move, u8 atk_bank, u8 def_bank, u16 base_power)
{
    u16 modifier = 0x1000;
    u8 move_type = move_table[move].type;
    u8 move_split = move_table[move].split;
    u16 quality_atk_modifier = percent_to_modifier(get_item_quality(battle_participants[atk_bank].held_item));
    if (has_ability_effect(atk_bank, 0, 1))
    {
        switch (battle_participants[atk_bank].ability_id)
        {
        case ABILITY_TECHNICIAN:
            if (base_power <= 60)
            {
                modifier = chain_modifier(modifier, 0x1800);
            }
            break;
        case ABILITY_FLARE_BOOST:
            if (battle_participants[atk_bank].status.flags.burn && move_split == MOVE_SPECIAL)
            {
                modifier = chain_modifier(modifier, 0x1800);
            }
            break;
        case ABILITY_TOXIC_BOOST:
            if ((battle_participants[atk_bank].status.flags.toxic_poison || battle_participants[atk_bank].status.flags.poison) && move_split == MOVE_PHYSICAL)
            {
                modifier = chain_modifier(modifier, 0x1800);
            }
            break;
        case ABILITY_RECKLESS:
            if (find_move_in_table(move, &reckless_moves_table[0]))
            {
                modifier = chain_modifier(modifier, 0x1333);
            }
            break;
        case ABILITY_IRON_FIST:
            if (find_move_in_table(move, &ironfist_moves_table[0]))
            {
                modifier = chain_modifier(modifier, 0x1333);
            }
            break;
        case ABILITY_SHEER_FORCE:
            if (find_move_in_table(move, &sheerforce_moves_table[0]))
            {
                modifier = chain_modifier(modifier, 0x14CD);
                new_battlestruct.ptr->bank_affecting[atk_bank].sheerforce_bonus = 1;
            }
            break;
        case ABILITY_SAND_FORCE:
            if (move_type == TYPE_STEEL || move_type == TYPE_ROCK || move_type == TYPE_GROUND)
            {
                modifier = chain_modifier(modifier, 0x14CD);
            }
            break;
        case ABILITY_RIVALRY:
            {
                u8 attacker_gender = gender_from_pid(battle_participants[atk_bank].poke_species, battle_participants[atk_bank].pid);
                u8 target_gender = gender_from_pid(battle_participants[def_bank].poke_species, battle_participants[def_bank].pid);
                if (attacker_gender != 0xFF && target_gender != 0xFF)
                {
                    if (attacker_gender == target_gender)
                    {
                        modifier = chain_modifier(modifier, 0x1400);
                    }
                    else
                    {
                        modifier = chain_modifier(modifier, 0xC00);
                    }
                }
                break;
            }
        case ABILITY_ANALYTIC:
            if (get_bank_turn_order(def_bank) < turn_order && move != MOVE_FUTURE_SIGHT && move != MOVE_DOOM_DESIRE)
            {
                modifier = chain_modifier(modifier, 0x14CD);
            }
            break;
        }
    }
    if (has_ability_effect(def_bank, 0, 1))
    {
        switch (battle_participants[def_bank].ability_id)
        {
        case ABILITY_HEATPROOF:
            if (move_type == TYPE_FIRE)
            {
                modifier = chain_modifier(modifier, 0x800);
            }
            break;
        case ABILITY_DRY_SKIN:
            if (move_type == TYPE_FIRE)
            {
                modifier = chain_modifier(modifier, 0x1400);
            }
            break;
        }
    }

    switch (get_item_effect(atk_bank, 1))
    {
    case ITEM_EFFECT_NOEFFECT:
        if (new_battlestruct.ptr->bank_affecting[atk_bank].gem_boost)
        {
            modifier = chain_modifier(modifier, percent_to_modifier(new_battlestruct.ptr->bank_affecting[atk_bank].gem_boost));
        }
        break;
    case ITEM_EFFECT_MUSCLEBAND:
        if (move_split == MOVE_PHYSICAL)
        {
            modifier = chain_modifier(modifier, 0x1199);
        }
        break;
    case ITEM_EFFECT_WISEGLASSES:
        if (move_split == MOVE_SPECIAL)
        {
            modifier = chain_modifier(modifier, 0x1199);
        }
        break;
    case ITEM_EFFECT_LUSTROUSORB:
        if ((move_type == TYPE_WATER || move_type == TYPE_DRAGON) && battle_participants[atk_bank].poke_species == POKE_PALKIA)
        {
            modifier = chain_modifier(modifier, 0x1333);
        }
        break;
    case ITEM_EFFECT_ADAMANTORB:
        if ((move_type == TYPE_STEEL || move_type == TYPE_DRAGON) && battle_participants[atk_bank].poke_species == POKE_DIALGA)
        {
            modifier = chain_modifier(modifier, 0x1333);
        }
        break;
    case ITEM_EFFECT_GRISEOUSORB:
        if ((move_type == TYPE_GHOST || move_type == TYPE_DRAGON) && (battle_participants[atk_bank].poke_species == POKE_GIRATINA))
        {
            modifier = chain_modifier(modifier, 0x1333);
        }
        break;
    case ITEM_EFFECT_FAIRYPLATE:
        if (move_type == TYPE_FAIRY)
        {
            modifier = chain_modifier(modifier, quality_atk_modifier);
        }
        break;
    case ITEM_EFFECT_SILKSCARF:
        if (move_type == TYPE_NORMAL)
        {
            modifier = chain_modifier(modifier, quality_atk_modifier);
        }
        break;
    case ITEM_EFFECT_SHARPBEAK:
        if (move_type == TYPE_FLYING)
        {
            modifier = chain_modifier(modifier, quality_atk_modifier);
        }
        break;
    case ITEM_EFFECT_BLACKBELT:
        if (move_type == TYPE_FIGHTING)
        {
            modifier = chain_modifier(modifier, quality_atk_modifier);
        }
        break;
    case ITEM_EFFECT_SOFTSAND:
        if (move_type == TYPE_GROUND)
        {
            modifier = chain_modifier(modifier, quality_atk_modifier);
        }
        break;
    case ITEM_EFFECT_HARDSTONE:
        if (move_type == TYPE_ROCK)
        {
            modifier = chain_modifier(modifier, quality_atk_modifier);
        }
        break;
    case ITEM_EFFECT_MAGNET:
        if (move_type == TYPE_ELECTRIC)
        {
            modifier = chain_modifier(modifier, quality_atk_modifier);
        }
        break;
    case ITEM_EFFECT_NEVERMELTICE:
        if (move_type == TYPE_ICE)
        {
            modifier = chain_modifier(modifier, quality_atk_modifier);
        }
        break;
    case ITEM_EFFECT_BLACKGLASSES:
        if (move_type == TYPE_DARK)
        {
            modifier = chain_modifier(modifier, quality_atk_modifier);
        }
        break;
    case ITEM_EFFECT_SILVERPOWDER:
        if (move_type == TYPE_BUG)
        {
            modifier = chain_modifier(modifier, quality_atk_modifier);
        }
        break;
    case ITEM_EFFECT_SPELLTAG:
        if (move_type == TYPE_GHOST)
        {
            modifier = chain_modifier(modifier, quality_atk_modifier);
        }
        break;
    case ITEM_EFFECT_DRAGONFANG:
        if (move_type == TYPE_DRAGON)
        {
            modifier = chain_modifier(modifier, quality_atk_modifier);
        }
        break;
    case ITEM_EFFECT_MYSTICWATER:
        if (move_type == TYPE_WATER)
        {
            modifier = chain_modifier(modifier, quality_atk_modifier);
        }
        break;
    case ITEM_EFFECT_CHARCOAL:
        if (move_type == TYPE_FIRE)
        {
            modifier = chain_modifier(modifier, quality_atk_modifier);
        }
        break;
    case ITEM_EFFECT_MIRACLESEED:
        if (move_type == TYPE_GRASS)
        {
            modifier = chain_modifier(modifier, quality_atk_modifier);
        }
        break;
    case ITEM_EFFECT_TWISTEDSPOON:
        if (move_type == TYPE_PSYCHIC)
        {
            modifier = chain_modifier(modifier, quality_atk_modifier);
        }
        break;
    case ITEM_EFFECT_METALCOAT:
        if (move_type == TYPE_STEEL)
        {
            modifier = chain_modifier(modifier, quality_atk_modifier);
        }
        break;
    case ITEM_EFFECT_POISONBARB:
        if (move_type == TYPE_POISON)
        {
            modifier = chain_modifier(modifier, quality_atk_modifier);
        }
        break;
    }

    switch (move)
    {
    case MOVE_FACADE:
        if (battle_participants[atk_bank].status.flags.poison || battle_participants[atk_bank].status.flags.toxic_poison || battle_participants[atk_bank].status.flags.paralysis || battle_participants[atk_bank].status.flags.burn)
        {
            modifier = chain_modifier(modifier, 0x2000);
        }
        break;
    case MOVE_BRINE:
        if (battle_participants[def_bank].current_hp < (battle_participants[def_bank].max_hp >> 1))
        {
            modifier = chain_modifier(modifier, 0x2000);
        }
        break;
    case MOVE_VENOSHOCK:
        if (battle_participants[def_bank].status.flags.poison || battle_participants[def_bank].status.flags.toxic_poison)
        {
            modifier = chain_modifier(modifier, 0x2000);
        }
        break;
    case MOVE_RETALIATE:
        if (new_battlestruct.ptr->bank_affecting[atk_bank].ally_fainted_last_turn)
        {
            modifier = chain_modifier(modifier, 0x2000);
        }
        break;
    case MOVE_SOLAR_BEAM:
        if (!(battle_weather.flags.sun || battle_weather.flags.permament_sun || battle_weather.flags.harsh_sun || battle_weather.int_bw == 0))
        {
            modifier = chain_modifier(modifier, 0x800);
        }
        break;
    }

    if (protect_structs[atk_bank].flag1_helpinghand)
    {
        modifier = chain_modifier(modifier, 0x1800);
    }

    if (status3[atk_bank].charged_up)
    {
        modifier = chain_modifier(modifier, 0x2000);
    }

    if ((move_type == TYPE_ELECTRIC && ability_battle_effects(0xE, 0, 0, 0xFD, 0)) || (move_type == TYPE_FIRE && ability_battle_effects(0xE, 0, 0, 0xFE, 0))) //mud and water sports
    {
        modifier = chain_modifier(modifier, 0x548);
    }

    if (new_battlestruct.ptr->bank_affecting[atk_bank].me_first)
    {
        modifier = chain_modifier(modifier, 0x1800);
    }

    return apply_modifier(modifier, base_power);
}

u16 get_attack_stat(u16 move, u8 atk_bank, u8 def_bank)
{
    u8 move_split = move_table[move].split;
    u8 stat_bank;
    if (move == MOVE_FOUL_PLAY)
    {
        stat_bank = def_bank;
    }
    else
    {
        stat_bank = atk_bank;
    }
    u16 attack_stat;
    u8 attack_boost;
    if (move_split == MOVE_PHYSICAL)
    {
        attack_stat = battle_participants[stat_bank].atk;
        attack_boost = battle_participants[stat_bank].atk_buff;
    }
    else
    {
        attack_stat = battle_participants[stat_bank].sp_atk;
        attack_boost = battle_participants[stat_bank].sp_atk_buff;
    }

    if (has_ability_effect(def_bank, 1, 1) && battle_participants[def_bank].ability_id == ABILITY_UNAWARE)
    {
        attack_boost = 6;
    }
    else if (crit_loc == 2 && attack_boost < 6)
    {
        attack_boost = 6;
    }

    attack_stat = __udivsi3(attack_stat * stat_buffs[attack_boost].dividend, stat_buffs[attack_boost].divisor);

    //final modifiactions
    u16 modifier = 0x1000;
    if (has_ability_effect(atk_bank, 0, 1))
    {
        u8 move_type = move_table[move].type;
        u8 pinch_abilities;
        if (battle_participants[atk_bank].current_hp >= __udivsi3(battle_participants[atk_bank].max_hp, 3))
            pinch_abilities = false;
        else
            pinch_abilities = true;

        switch (battle_participants[atk_bank].ability_id)
        {
        case ABILITY_PURE_POWER:
        case ABILITY_HUGE_POWER:
            if (move_split == MOVE_PHYSICAL)
            {
                modifier = chain_modifier(modifier, 0x2000);
            }
            break;
        case ABILITY_SLOW_START:
            if (new_battlestruct.ptr->bank_affecting[atk_bank].slowstart_duration)
            {
                modifier = chain_modifier(modifier, 0x800);
            }
            break;
        case ABILITY_SOLAR_POWER:
            if (move_split == MOVE_SPECIAL && (battle_weather.flags.sun || battle_weather.flags.permament_sun || battle_weather.flags.harsh_sun))
            {
                modifier = chain_modifier(modifier, 0x1800);
            }
            break;
        case ABILITY_DEFEATIST:
            if (battle_participants[atk_bank].current_hp <= (battle_participants[atk_bank].max_hp >> 1))
            {
                modifier = chain_modifier(modifier, 0x800);
            }
            break;
        case ABILITY_FLASH_FIRE:
            if (move_type == TYPE_FIRE && battle_resources_ptr.ptr->field0_ptr->ability_flags_ptr->flags_ability[atk_bank].flag1_flashfire)
            {
                modifier = chain_modifier(modifier, 0x1800);
            }
            break;
        case ABILITY_SWARM:
            if (move_type == TYPE_BUG && pinch_abilities)
            {
                modifier = chain_modifier(modifier, 0x1800);
            }
            break;
        case ABILITY_OVERGROW:
            if (move_type == TYPE_GRASS && pinch_abilities)
            {
                modifier = chain_modifier(modifier, 0x1800);
            }
            break;
        case ABILITY_TORRENT:
            if (move_type == TYPE_GRASS && pinch_abilities)
            {
                modifier = chain_modifier(modifier, 0x1800);
            }
            break;
        case ABILITY_BLAZE:
            if (move_type == TYPE_FIRE && pinch_abilities)
            {
                modifier = chain_modifier(modifier, 0x1800);
            }
            break;
        case ABILITY_THICK_FAT:
            if (move_type == TYPE_FIRE || move_type == TYPE_ICE)
            {
                modifier = chain_modifier(modifier, 0x800);
            }
            break;
        case ABILITY_PLUS:
        case ABILITY_MINUS:
            if (move_split == MOVE_SPECIAL && (ability_battle_effects(20, atk_bank, ABILITY_PLUS, 0, 0) || ability_battle_effects(20, atk_bank, ABILITY_MINUS, 0, 0)))
            {
                modifier = chain_modifier(modifier, 0x1800);
            }
            break;
        case ABILITY_HUSTLE:
            if (move_split == MOVE_PHYSICAL)
            {
                attack_stat = apply_modifier(0x1800, attack_stat);
            }
            break;
        }
    }

    if (battle_weather.flags.harsh_sun || battle_weather.flags.permament_sun || battle_weather.flags.sun)
    {
        u8 flower_gift_bank = ability_battle_effects(13, atk_bank, ABILITY_FLOWER_GIFT, 0, 0);
        if (flower_gift_bank)
        {
            flower_gift_bank--;
            if (new_battlestruct.ptr->bank_affecting[flower_gift_bank].sunshine_form)
            {
                modifier = chain_modifier(modifier, 0x1800);
            }
        }
    }

    return apply_modifier(modifier, attack_stat);
}

void damage_calc(u16 move, u8 atk_bank, u8 def_bank)
{
    u16 base_power = apply_base_power_modifiers(move, atk_bank, def_bank, get_base_power(move, atk_bank, def_bank));
    damage_loc = base_power;
}

void damage_calc_cmd_05()
{
    damage_calc(current_move, bank_attacker, bank_target);
    battlescripts_curr_instruction++;
    return;
}