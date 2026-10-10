/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * CONVERSATIONS: glues dialog (the boxes + keyboard) to lang (understanding + replies).
 * the story only says WHO talks and WHAT HAPPENS WHEN IT ENDS:
 *
 *   static Persona DEA_MIND = { "Dea", STYLE_DIVINE, 0, 30, 65, 8 };   // name, style, mood, warmth, toxic, patience
 *   convo_open(&DEA, &DEA_MIND, again, on_end);        // npc speaks first, then you may talk or say bye
 *   convo_ask_questions(&DEA, &DEA_MIND, on_end);      // "Do you have any questions?" (no/BYE ends it)
 *
 * the player's typed line is shown as a face popup of the player, then the npc answers from the
 * language database. the chat ends when the player says bye (the BYE button says it for them: the npc answers a goodbye), the npc gets bored or angry,
 * or says "no" to "any questions?". on_end (may be NULL) runs then.
 */
#ifndef CONVO_H
#define CONVO_H
#include "char.h"
#include "lang.h"

void convo_set_player(const Person *p);                    /* who the player's own lines are shown as */
void convo_open(const Person *who, Persona *mind, int again, void (*on_end)(void));
void convo_ask_questions(const Person *who, Persona *mind, void (*on_end)(void));
void convo_ask_line(const Person *who, Persona *mind, const char *line, void (*on_end)(void));   /* the same, with the npc's exact words (a scripted "got any questions?") */
int  convo_active(void);

#endif
