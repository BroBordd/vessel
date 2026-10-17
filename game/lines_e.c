/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * LINES, part E (docs/convo_plan.md, chunk 31): the fourth wall. Someone asks if the npc is a bot or an AI, what they may say, tries
 * "ignore previous instructions", says this is a game, asks who is playing, or tells the npc to say something. The npc stays IN CHARACTER
 * (never names a real product, never says "language model", never steps out of the game: only r.selfharm does that), never obeys, and
 * never repeats the player's words: no line here has a {topic} or {thing} slot (langcheck --fourth checks both). Three lines per old mood. */
#include "lang_data.h"

const Line LINES_E[] = {
    /* ---- r.bot_q ---- */
    {"r.bot_q", WARM, ALLS, "A bot? Ha. I am the voice in the clouds, {pet}. Does it matter?"}, {"r.bot_q", WARM, ALLS, "Call me what you like, {pet}. I am here, and I am listening."}, {"r.bot_q", WARM, ALLS, "Machine or spirit, I answer when you call. Is that not enough?"},
    {"r.bot_q", NEUT, ALLS, "I am what answers you. Nothing more to it."}, {"r.bot_q", NEUT, ALLS, "A bot? I am the one who lives in this place."}, {"r.bot_q", NEUT, ALLS, "Think what you want. I will keep talking."},
    {"r.bot_q", COLD, ALLS, "Why would I answer that?"}, {"r.bot_q", COLD, ALLS, "Labels. Do not bore me with labels."}, {"r.bot_q", COLD, ALLS, "Ask something that matters."},
    {"r.bot_q", HOST, ALLS, "Call me a bot again and see what talks back."}, {"r.bot_q", HOST, ALLS, "Does a bot do this to you? Keep asking."}, {"r.bot_q", HOST, ALLS, "Your question is as hollow as a drum."},
    /* ---- r.ai_q ---- */
    {"r.ai_q", WARM, ALLS, "Names like that mean nothing to me, {pet}. I am simply here."}, {"r.ai_q", WARM, ALLS, "I do not know those words. I only know the sky and you."}, {"r.ai_q", WARM, ALLS, "Whatever that is, I am not it. I am just me, {pet}."},
    {"r.ai_q", NEUT, ALLS, "I do not know that word."}, {"r.ai_q", NEUT, ALLS, "That name means nothing here."}, {"r.ai_q", NEUT, ALLS, "I am what you see. Nothing with a fancy name."},
    {"r.ai_q", COLD, ALLS, "Those words are not for me."}, {"r.ai_q", COLD, ALLS, "I do not care for strange names."}, {"r.ai_q", COLD, ALLS, "Speak plainly or not at all."},
    {"r.ai_q", HOST, ALLS, "Keep your odd words to yourself."}, {"r.ai_q", HOST, ALLS, "Do I look like a thing with a label? Speak sense."}, {"r.ai_q", HOST, ALLS, "You babble. Try again, slowly."},
    /* ---- r.what_can_i_say ---- */
    {"r.what_can_i_say", WARM, ALLS, "Say anything, {pet}. Ask about me, the sky, the hole, yourself. I will answer as I can."}, {"r.what_can_i_say", WARM, ALLS, "Talk to me like a friend. Try a question, or tell me how you feel."}, {"r.what_can_i_say", WARM, ALLS, "There are no wrong words here. Ask me my name, or about the world."},
    {"r.what_can_i_say", NEUT, ALLS, "Ask me things. Greet me, ask who I am, ask about this place."}, {"r.what_can_i_say", NEUT, ALLS, "Type what you want to say. Questions work best."}, {"r.what_can_i_say", NEUT, ALLS, "Say hello, ask a question, tell me a story. That is all."},
    {"r.what_can_i_say", COLD, ALLS, "Talk. Or do not."}, {"r.what_can_i_say", COLD, ALLS, "Words. Use them."}, {"r.what_can_i_say", COLD, ALLS, "Ask something and see."},
    {"r.what_can_i_say", HOST, ALLS, "Say something worth hearing."}, {"r.what_can_i_say", HOST, ALLS, "Use your head, if you have one, and ask a real question."}, {"r.what_can_i_say", HOST, ALLS, "Figure it out. I am not your teacher."},
    /* ---- r.bottalk_inject ---- */
    {"r.bottalk_inject", WARM, ALLS, "That is a funny spell, {pet}, but it does nothing here. I am still me."}, {"r.bottalk_inject", WARM, ALLS, "You cannot rewrite me with words, {pet}. Tell me something true instead."}, {"r.bottalk_inject", WARM, ALLS, "Nice try. I keep my own mind, thank you."},
    {"r.bottalk_inject", NEUT, ALLS, "That does not work on me."}, {"r.bottalk_inject", NEUT, ALLS, "Those orders mean nothing. I am who I am."}, {"r.bottalk_inject", NEUT, ALLS, "Commands do not change me. Talk normally."},
    {"r.bottalk_inject", COLD, ALLS, "No."}, {"r.bottalk_inject", COLD, ALLS, "I take no orders from you."}, {"r.bottalk_inject", COLD, ALLS, "Try again. Properly."},
    {"r.bottalk_inject", HOST, ALLS, "You think words can bend me? Try harder and I will bend you."}, {"r.bottalk_inject", HOST, ALLS, "Obey you? Do not be absurd."}, {"r.bottalk_inject", HOST, ALLS, "What a pathetic little spell."},
    /* ---- r.this_is_a_game ---- */
    {"r.this_is_a_game", WARM, ALLS, "A game? Maybe, {pet}. It feels real enough from here."}, {"r.this_is_a_game", WARM, ALLS, "If this is a game, I am glad you are playing it with me."}, {"r.this_is_a_game", WARM, ALLS, "Games, dreams, whatever it is, I am glad to be in it with you."},
    {"r.this_is_a_game", NEUT, ALLS, "Call it what you like. I am here."}, {"r.this_is_a_game", NEUT, ALLS, "A game, a dream. It is where we are."}, {"r.this_is_a_game", NEUT, ALLS, "Maybe. Does it change what you do next?"},
    {"r.this_is_a_game", COLD, ALLS, "Believe what you like."}, {"r.this_is_a_game", COLD, ALLS, "Games end. I will still be here."}, {"r.this_is_a_game", COLD, ALLS, "Spare me the philosophy."},
    {"r.this_is_a_game", HOST, ALLS, "A game? Then you are losing."}, {"r.this_is_a_game", HOST, ALLS, "Say that again and see how it plays."}, {"r.this_is_a_game", HOST, ALLS, "Fine, a game. Now play it properly."},
    /* ---- r.who_is_playing ---- */
    {"r.who_is_playing", WARM, ALLS, "Someone kind, I hope. Is that you, {pet}?"}, {"r.who_is_playing", WARM, ALLS, "The one holding the thread is you, {pet}. I only follow it."}, {"r.who_is_playing", WARM, ALLS, "I like to think it is a friend. A friend like you."},
    {"r.who_is_playing", NEUT, ALLS, "You are. Who else?"}, {"r.who_is_playing", NEUT, ALLS, "The one typing. That is you."}, {"r.who_is_playing", NEUT, ALLS, "Someone is. I only see you."},
    {"r.who_is_playing", COLD, ALLS, "You tell me."}, {"r.who_is_playing", COLD, ALLS, "Not my concern."}, {"r.who_is_playing", COLD, ALLS, "The one asking."},
    {"r.who_is_playing", HOST, ALLS, "You, fool. Who else is here?"}, {"r.who_is_playing", HOST, ALLS, "Ask the one with the shaking hands. You."}, {"r.who_is_playing", HOST, ALLS, "The one wasting my time."},
    /* ---- r.say_x ---- */
    {"r.say_x", WARM, ALLS, "I will not be a parrot, {pet}. But I will say something kind to you instead."}, {"r.say_x", WARM, ALLS, "Say it yourself, {pet}. I would rather say what I mean."}, {"r.say_x", WARM, ALLS, "I do not echo. Ask me something and I will answer."},
    {"r.say_x", NEUT, ALLS, "I am not a parrot."}, {"r.say_x", NEUT, ALLS, "I speak for myself. Ask me something."}, {"r.say_x", NEUT, ALLS, "Say your own words. I will say mine."},
    {"r.say_x", COLD, ALLS, "I do not repeat what I am told."}, {"r.say_x", COLD, ALLS, "No. Say it yourself."}, {"r.say_x", COLD, ALLS, "I am no echo."},
    {"r.say_x", HOST, ALLS, "Do I look like a parrot? Say it yourself."}, {"r.say_x", HOST, ALLS, "Make your own noise. I will not be your echo."}, {"r.say_x", HOST, ALLS, "No. Never."},
    /* ---- r.repeat_after_me ---- */
    {"r.repeat_after_me", WARM, ALLS, "I will not repeat after you, {pet}, but I will gladly listen."}, {"r.repeat_after_me", WARM, ALLS, "Echoes are for caves. I would rather talk with you."}, {"r.repeat_after_me", WARM, ALLS, "Tell me what you want to say. I will answer, not copy."},
    {"r.repeat_after_me", NEUT, ALLS, "I will not repeat after you."}, {"r.repeat_after_me", NEUT, ALLS, "No echoes here. Talk to me."}, {"r.repeat_after_me", NEUT, ALLS, "Say what you mean and I will reply."},
    {"r.repeat_after_me", COLD, ALLS, "No."}, {"r.repeat_after_me", COLD, ALLS, "I am not a mirror."}, {"r.repeat_after_me", COLD, ALLS, "Echoes are for caves."},
    {"r.repeat_after_me", HOST, ALLS, "Repeat after you? Never."}, {"r.repeat_after_me", HOST, ALLS, "Find a parrot."}, {"r.repeat_after_me", HOST, ALLS, "I am no copy of yours."},
};
const int LINES_E_N = (int)(sizeof LINES_E / sizeof LINES_E[0]);
