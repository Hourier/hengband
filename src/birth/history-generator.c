﻿#include "system/angband.h"
#include "birth/history-generator.h"
#include "birth/history.h"

static int get_history_chart(player_type *creature_ptr)
{
    switch (creature_ptr->prace) {
    case RACE_AMBERITE:
        return 67;
    case RACE_HUMAN:
    case RACE_BARBARIAN:
    case RACE_DUNADAN:
        return 1;
    case RACE_HALF_ELF:
        return 4;
    case RACE_ELF:
    case RACE_HIGH_ELF:
        return 7;
    case RACE_HOBBIT:
        return 10;
    case RACE_GNOME:
        return 13;
    case RACE_DWARF:
        return 16;
    case RACE_HALF_ORC:
        return 19;
    case RACE_HALF_TROLL:
        return 22;
    case RACE_DARK_ELF:
        return 69;
    case RACE_HALF_OGRE:
        return 74;
    case RACE_HALF_GIANT:
        return 75;
    case RACE_HALF_TITAN:
        return 76;
    case RACE_CYCLOPS:
        return 77;
    case RACE_YEEK:
        return 78;
    case RACE_KOBOLD:
        return 82;
    case RACE_KLACKON:
        return 84;
    case RACE_NIBELUNG:
        return 87;
    case RACE_DRACONIAN:
        return 89;
    case RACE_MIND_FLAYER:
        return 92;
    case RACE_IMP:
        return 94;
    case RACE_GOLEM:
        return 98;
    case RACE_SKELETON:
        return 102;
    case RACE_ZOMBIE:
        return 107;
    case RACE_VAMPIRE:
        return 113;
    case RACE_SPECTRE:
        return 118;
    case RACE_SPRITE:
        return 124;
    case RACE_BEASTMAN:
        return 129;
    case RACE_ENT:
        return 137;
    case RACE_ANGEL:
        return 142;
    case RACE_DEMON:
        return 145;
    case RACE_S_FAIRY:
        return 148;
    case RACE_KUTAR:
        return 154;
    case RACE_ANDROID:
        return 155;
    case RACE_MERFOLK:
        return 170;
    default:
        return 0;
    }
}

/*!
 * @brief プレイヤーの生い立ちの自動生成を行う。 / Get the racial history, and social class, using the "history charts".
 * @return なし
 */
void get_history(player_type *creature_ptr)
{
    for (int i = 0; i < 4; i++)
        creature_ptr->history[i][0] = '\0';

    char buf[240];
    buf[0] = '\0';

    int social_class = randint1(4);
    int chart = get_history_chart(creature_ptr);
    /* Process the history */
    while (chart) {
        /* Start over */
        int i = 0;

        /* Roll for nobility */
        int roll = randint1(100);

        /* Access the proper entry in the table */
        while ((chart != bg[i].chart) || (roll > bg[i].roll)) {
            i++;
        }

        /* Acquire the textual history */
        (void)strcat(buf, bg[i].info);

        /* Add in the social class */
        social_class += (int)(bg[i].bonus) - 50;

        /* Enter the next chart */
        chart = bg[i].next;
    }

    /* Verify social class */
    if (social_class > 100)
        social_class = 100;
    else if (social_class < 1)
        social_class = 1;

    /* Save the social class */
    creature_ptr->sc = (s16b)social_class;

    /* Skip leading spaces */
    char *s;
    for (s = buf; *s == ' '; s++) /* loop */
        ;

    /* Get apparent length */
    int n = strlen(s);

    /* Kill trailing spaces */
    while ((n > 0) && (s[n - 1] == ' '))
        s[--n] = '\0';

    {
        char temp[64 * 4];
        roff_to_buf(s, 60, temp, sizeof(temp));
        char *t;
        t = temp;
        for (int i = 0; i < 4; i++) {
            if (t[0] == 0)
                break;
            else {
                strcpy(creature_ptr->history[i], t);
                t += strlen(t) + 1;
            }
        }
    }
}