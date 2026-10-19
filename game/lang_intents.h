/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE INTENT CATALOGUE (docs/convo_plan.md, Appendix A; chunk 12): the 334 intents that come after I_NEG, in group order.
 * An X-macro list: lang.h makes the enum from it (I_<ID>) and lang.c the names and labels. X(ID, "name", "LABEL").
 * The reply key of an intent is r.<name>, unless KEYMAP in lang.c maps the name to another key (merged near-twins).
 * Add or rename an intent here and nowhere else (the tools ask lang_intent_name). Until a group chunk gives an intent patterns it matches nothing. */
#define INTENT_LIST(X) \
    /* G01 Greetings and goodbyes */ \
    X(GREET_MORNING, "greet_morning", "GREET MORNING") X(GREET_EVENING, "greet_evening", "GREET EVENING") X(GREET_NIGHT, "greet_night", "GREET NIGHT") \
    X(GREET_AGAIN, "greet_again", "GREET AGAIN") X(GREET_FORMAL, "greet_formal", "GREET FORMAL") X(GREET_SLANG, "greet_slang", "GREET SLANG") \
    X(GREET_GROUP, "greet_group", "GREET GROUP") X(BYE_SOFT, "bye_soft", "BYE SOFT") X(BYE_RUDE, "bye_rude", "BYE RUDE") \
    X(BYE_FOREVER, "bye_forever", "BYE FOREVER") X(BYE_TEMP, "bye_temp", "BYE TEMP") X(SEE_YOU_SOON, "see_you_soon", "SEE YOU SOON") \
    X(WELCOME_BACK, "welcome_back", "WELCOME BACK") X(LONG_TIME, "long_time", "LONG TIME") X(GOODNIGHT, "goodnight", "GOODNIGHT") \
    X(GOODLUCK, "goodluck", "GOODLUCK") X(WAVE, "wave", "WAVE") X(BOW, "bow", "BOW") X(HANDSHAKE, "handshake", "HANDSHAKE") X(HUG, "hug", "HUG") \
    /* G02 Yes, no, maybe and acknowledgements */ \
    X(MAYBE, "maybe", "MAYBE") X(IDK, "idk", "IDK") X(PROBABLY, "probably", "PROBABLY") X(HEDGE, "hedge", "HEDGE") \
    X(SURE_THING, "sure_thing", "SURE THING") X(NO_WAY, "no_way", "NO WAY") X(FINE_WHATEVER, "fine_whatever", "FINE WHATEVER") \
    X(GOT_IT, "got_it", "GOT IT") X(NODDING, "nodding", "NODDING") X(SHRUG, "shrug", "SHRUG") X(CORRECT, "correct", "CORRECT") \
    X(WRONG, "wrong", "WRONG") X(EXACTLY, "exactly", "EXACTLY") X(REALLY_Q, "really_q", "REALLY Q") X(SERIOUSLY_Q, "seriously_q", "SERIOUSLY Q") \
    X(INTERESTING, "interesting", "INTERESTING") X(OH_I_SEE, "oh_i_see", "OH I SEE") X(HMM, "hmm", "HMM") X(UH_UM, "uh_um", "UH UM") \
    X(NEVER_MIND, "never_mind", "NEVER MIND") \
    /* G03 Thanks, sorry, manners */ \
    X(THANKS_BIG, "thanks_big", "THANKS BIG") X(THANKS_SARCASTIC, "thanks_sarcastic", "THANKS SARCASTIC") \
    X(YOURE_WELCOME, "youre_welcome", "YOURE WELCOME") X(NO_PROBLEM, "no_problem", "NO PROBLEM") X(APOLOGY_BIG, "apology_big", "APOLOGY BIG") \
    X(APOLOGY_FAKE, "apology_fake", "APOLOGY FAKE") X(FORGIVE_ME, "forgive_me", "FORGIVE ME") X(EXCUSE_ME, "excuse_me", "EXCUSE ME") \
    X(PARDON_Q, "pardon_q", "PARDON Q") X(CONGRATS, "congrats", "CONGRATS") X(HAPPY_BIRTHDAY, "happy_birthday", "HAPPY BIRTHDAY") \
    X(BLESS_YOU, "bless_you", "BLESS YOU") X(CHEERS, "cheers", "CHEERS") X(SYMPATHY, "sympathy", "SYMPATHY") \
    X(CONDOLENCES, "condolences", "CONDOLENCES") X(GOOD_FOR_YOU, "good_for_you", "GOOD FOR YOU") X(WELL_DONE, "well_done", "WELL DONE") \
    X(MY_PLEASURE, "my_pleasure", "MY PLEASURE") X(PLEASE_BIG, "please_big", "PLEASE BIG") X(BEG, "beg", "BEG") \
    /* G04 About the npc: identity and body */ \
    X(NPC_NAME_ORIGIN, "npc_name_origin", "NPC NAME ORIGIN") X(NPC_GENDER, "npc_gender", "NPC GENDER") X(NPC_SPECIES, "npc_species", "NPC SPECIES") \
    X(NPC_LOOKS, "npc_looks", "NPC LOOKS") X(NPC_BODY, "npc_body", "NPC BODY") X(NPC_EATS, "npc_eats", "NPC EATS") \
    X(NPC_SLEEPS, "npc_sleeps", "NPC SLEEPS") X(NPC_PAIN, "npc_pain", "NPC PAIN") X(NPC_ALIVE, "npc_alive", "NPC ALIVE") \
    X(NPC_IMMORTAL, "npc_immortal", "NPC IMMORTAL") X(NPC_BIRTHDAY, "npc_birthday", "NPC BIRTHDAY") X(NPC_HOME, "npc_home", "NPC HOME") \
    X(NPC_FAMILY, "npc_family", "NPC FAMILY") X(NPC_FRIENDS, "npc_friends", "NPC FRIENDS") X(NPC_ENEMIES, "npc_enemies", "NPC ENEMIES") \
    X(NPC_JOB, "npc_job", "NPC JOB") X(NPC_POWER, "npc_power", "NPC POWER") X(NPC_WEAKNESS, "npc_weakness", "NPC WEAKNESS") \
    X(NPC_SECRET_SELF, "npc_secret_self", "NPC SECRET SELF") X(NPC_TRUE_NAME, "npc_true_name", "NPC TRUE NAME") \
    /* G05 About the npc: likes, feelings, opinions */ \
    X(FAV_COLOR, "fav_color", "FAV COLOR") X(FAV_FOOD, "fav_food", "FAV FOOD") X(FAV_ANIMAL, "fav_animal", "FAV ANIMAL") \
    X(FAV_NUMBER, "fav_number", "FAV NUMBER") X(FAV_SONG, "fav_song", "FAV SONG") X(FAV_SEASON, "fav_season", "FAV SEASON") \
    X(LIKES_Q, "likes_q", "LIKES Q") X(DISLIKES_Q, "dislikes_q", "DISLIKES Q") X(HOBBIES, "hobbies", "HOBBIES") \
    X(DREAMS_NPC, "dreams_npc", "DREAMS NPC") X(FEARS_NPC, "fears_npc", "FEARS NPC") X(LOVES_Q, "loves_q", "LOVES Q") X(HATES_Q, "hates_q", "HATES Q") \
    X(HAPPY_Q, "happy_q", "HAPPY Q") X(LONELY_Q, "lonely_q", "LONELY Q") X(OPINION_ME, "opinion_me", "OPINION ME") \
    X(OPINION_X, "opinion_x", "OPINION X") X(TRUST_Q, "trust_q", "TRUST Q") X(FRIEND_Q, "friend_q", "FRIEND Q") X(MARRIED_Q, "married_q", "MARRIED Q") \
    /* G06 About the world and the game */ \
    X(PLACE_CLOUDS, "place_clouds", "PLACE CLOUDS") X(PLACE_GRASS, "place_grass", "PLACE GRASS") X(PLACE_HOUSE, "place_house", "PLACE HOUSE") \
    X(PLACE_SHRINE, "place_shrine", "PLACE SHRINE") X(PLACE_TEMPLE, "place_temple", "PLACE TEMPLE") X(THING_ORB, "thing_orb", "THING ORB") \
    X(THING_HAMMER, "thing_hammer", "THING HAMMER") X(THING_GRAVE, "thing_grave", "THING GRAVE") X(THING_STARS, "thing_stars", "THING STARS") \
    X(THING_MOON, "thing_moon", "THING MOON") X(THING_SUN, "thing_sun", "THING SUN") X(THING_SEA, "thing_sea", "THING SEA") \
    X(THING_FOREST, "thing_forest", "THING FOREST") X(TIME_DAY, "time_day", "TIME DAY") X(TIME_NIGHT, "time_night", "TIME NIGHT") \
    X(WEATHER, "weather", "WEATHER") X(MAP_Q, "map_q", "MAP Q") X(WHAT_IS_THIS_PLACE, "what_is_this_place", "WHAT IS THIS PLACE") \
    X(WHAT_IS_DIA, "what_is_dia", "WHAT IS DIA") X(WHAT_IS_VESSEL, "what_is_vessel", "WHAT IS VESSEL") \
    /* G07 Goals, help and how to play */ \
    X(WHAT_TO_DO, "what_to_do", "WHAT TO DO") X(HINT_Q, "hint_q", "HINT Q") X(HOW_TO_PLAY, "how_to_play", "HOW TO PLAY") \
    X(CONTROLS_Q, "controls_q", "CONTROLS Q") X(BUTTONS_Q, "buttons_q", "BUTTONS Q") X(SAVE_Q, "save_q", "SAVE Q") X(PAUSE_Q, "pause_q", "PAUSE Q") \
    X(WIN_Q, "win_q", "WIN Q") X(END_Q, "end_q", "END Q") X(CHEAT_Q, "cheat_q", "CHEAT Q") X(STUCK, "stuck", "STUCK") X(LOST_Q, "lost_q", "LOST Q") \
    X(WHICH_WAY, "which_way", "WHICH WAY") X(HOW_LONG, "how_long", "HOW LONG") X(WHATS_NEXT, "whats_next", "WHATS NEXT") \
    X(SECRETS_HINT, "secrets_hint", "SECRETS HINT") X(RULES_Q, "rules_q", "RULES Q") X(DIFFICULTY_Q, "difficulty_q", "DIFFICULTY Q") \
    X(TUTORIAL_Q, "tutorial_q", "TUTORIAL Q") X(WHO_MADE_THIS, "who_made_this", "WHO MADE THIS") \
    /* G08 Life, death, gods, fate */ \
    X(DEATH_Q, "death_q", "DEATH Q") X(AFTERLIFE, "afterlife", "AFTERLIFE") X(HEAVEN, "heaven", "HEAVEN") X(HELL, "hell", "HELL") \
    X(REBORN, "reborn", "REBORN") X(SOUL_Q, "soul_q", "SOUL Q") X(MEMORY_Q, "memory_q", "MEMORY Q") X(LIMBO_Q, "limbo_q", "LIMBO Q") \
    X(BODY_SWAP, "body_swap", "BODY SWAP") X(NEW_BODY, "new_body", "NEW BODY") X(FORGET_Q, "forget_q", "FORGET Q") X(SIN, "sin", "SIN") \
    X(JUDGEMENT, "judgement", "JUDGEMENT") X(PRAYER, "prayer", "PRAYER") X(MIRACLE, "miracle", "MIRACLE") X(FATE, "fate", "FATE") \
    X(FREE_WILL, "free_will", "FREE WILL") X(CREATOR_Q, "creator_q", "CREATOR Q") X(WHY_EXIST, "why_exist", "WHY EXIST") \
    X(MEANING_OF_LIFE, "meaning_of_life", "MEANING OF LIFE") \
    /* G09 About the player: identity and status */ \
    X(MY_NAME, "my_name", "MY NAME") X(MY_AGE, "my_age", "MY AGE") X(MY_JOB, "my_job", "MY JOB") X(MY_HOME, "my_home", "MY HOME") \
    X(MY_FAMILY, "my_family", "MY FAMILY") X(MY_LIKES, "my_likes", "MY LIKES") X(MY_DISLIKES, "my_dislikes", "MY DISLIKES") \
    X(MY_PLAN, "my_plan", "MY PLAN") X(MY_WISH, "my_wish", "MY WISH") X(MY_SECRET, "my_secret", "MY SECRET") X(MY_STORY, "my_story", "MY STORY") \
    X(MY_STATUS_DEAD, "my_status_dead", "MY STATUS DEAD") X(MY_STATUS_ALIVE, "my_status_alive", "MY STATUS ALIVE") \
    X(MY_MEMORY_LOST, "my_memory_lost", "MY MEMORY LOST") X(MY_NEW_BODY, "my_new_body", "MY NEW BODY") \
    X(REMIND_ME_NAME, "remind_me_name", "REMIND ME NAME") X(AM_I_SPECIAL, "am_i_special", "AM I SPECIAL") X(AM_I_DEAD_Q, "am_i_dead_q", "AM I DEAD Q") \
    X(WHO_AM_I_REALLY, "who_am_i_really", "WHO AM I REALLY") X(WHAT_AM_I, "what_am_i", "WHAT AM I") \
    /* G10 The player's feelings */ \
    X(HAPPY_ME, "happy_me", "HAPPY ME") X(EXCITED_ME, "excited_me", "EXCITED ME") X(BORED_ME, "bored_me", "BORED ME") \
    X(TIRED_ME, "tired_me", "TIRED ME") X(LONELY_ME, "lonely_me", "LONELY ME") X(JEALOUS_ME, "jealous_me", "JEALOUS ME") \
    X(GUILTY_ME, "guilty_me", "GUILTY ME") X(ASHAMED_ME, "ashamed_me", "ASHAMED ME") X(PROUD_ME, "proud_me", "PROUD ME") \
    X(HOPEFUL_ME, "hopeful_me", "HOPEFUL ME") X(HURT_ME, "hurt_me", "HURT ME") X(BETRAYED_ME, "betrayed_me", "BETRAYED ME") \
    X(NERVOUS_ME, "nervous_me", "NERVOUS ME") X(EMBARRASSED_ME, "embarrassed_me", "EMBARRASSED ME") X(DISGUSTED_ME, "disgusted_me", "DISGUSTED ME") \
    X(SURPRISED_ME, "surprised_me", "SURPRISED ME") X(DISAPPOINTED_ME, "disappointed_me", "DISAPPOINTED ME") \
    X(RELIEVED_ME, "relieved_me", "RELIEVED ME") X(GRIEVING_ME, "grieving_me", "GRIEVING ME") X(NUMB_ME, "numb_me", "NUMB ME") \
    X(HUNGRY_ME, "hungry_me", "HUNGRY ME") X(COLD_ME, "cold_me", "COLD ME") X(SICK_ME, "sick_me", "SICK ME") X(DIZZY_ME, "dizzy_me", "DIZZY ME") \
    /* G11 Insults, praise, love, flirt, threats */ \
    X(HATE_YOU, "hate_you", "HATE YOU") X(INSULT_MILD, "insult_mild", "INSULT MILD") X(INSULT_STRONG, "insult_strong", "INSULT STRONG") \
    X(INSULT_FAMILY, "insult_family", "INSULT FAMILY") X(SWEAR_ONLY, "swear_only", "SWEAR ONLY") X(SLUR_BLOCKED, "slur_blocked", "SLUR BLOCKED") \
    X(COMPLIMENT_LOOKS, "compliment_looks", "COMPLIMENT LOOKS") X(COMPLIMENT_SMART, "compliment_smart", "COMPLIMENT SMART") \
    X(COMPLIMENT_VOICE, "compliment_voice", "COMPLIMENT VOICE") X(FLIRT_LIGHT, "flirt_light", "FLIRT LIGHT") \
    X(FLIRT_HEAVY_DEFLECT, "flirt_heavy_deflect", "FLIRT HEAVY DEFLECT") X(SEXUAL_BLOCKED, "sexual_blocked", "SEXUAL BLOCKED") \
    X(PROPOSE, "propose", "PROPOSE") X(JEALOUS_NPC, "jealous_npc", "JEALOUS NPC") X(THREAT_NPC, "threat_npc", "THREAT NPC") \
    X(THREAT_SELF_SAFE, "threat_self_safe", "THREAT SELF SAFE") X(DARE, "dare", "DARE") X(CHALLENGE, "challenge", "CHALLENGE") X(BET, "bet", "BET") \
    X(BRIBE, "bribe", "BRIBE") \
    /* G12 Requests, commands, trades */ \
    X(GIVE_ME, "give_me", "GIVE ME") X(SHOW_ME, "show_me", "SHOW ME") X(TAKE_ME, "take_me", "TAKE ME") X(LET_ME, "let_me", "LET ME") \
    X(OPEN_DOOR, "open_door", "OPEN DOOR") X(FOLLOW_ME, "follow_me", "FOLLOW ME") X(COME_HERE, "come_here", "COME HERE") \
    X(GO_AWAY, "go_away", "GO AWAY") X(LEAVE_ME_ALONE, "leave_me_alone", "LEAVE ME ALONE") X(STOP_IT, "stop_it", "STOP IT") \
    X(LISTEN_UP, "listen_up", "LISTEN UP") X(LOOK_AT, "look_at", "LOOK AT") X(TELL_ME_MORE, "tell_me_more", "TELL ME MORE") \
    X(TEACH_ME, "teach_me", "TEACH ME") X(HEAL_ME, "heal_me", "HEAL ME") X(SAVE_ME, "save_me", "SAVE ME") X(KILL_NPC, "kill_npc", "KILL NPC") \
    X(REVIVE_Q, "revive_q", "REVIVE Q") X(TRADE_Q, "trade_q", "TRADE Q") X(GIFT_TO_NPC, "gift_to_npc", "GIFT TO NPC") \
    X(ASK_PERMISSION, "ask_permission", "ASK PERMISSION") X(ASK_ADVICE, "ask_advice", "ASK ADVICE") X(ASK_OPINION, "ask_opinion", "ASK OPINION") \
    X(ASK_FORTUNE, "ask_fortune", "ASK FORTUNE") X(ASK_PROMISE, "ask_promise", "ASK PROMISE") \
    /* G13 Games, jokes, play */ \
    X(RIDDLE_ME, "riddle_me", "RIDDLE ME") X(RIDDLE_ANSWER, "riddle_answer", "RIDDLE ANSWER") X(KNOCK_KNOCK, "knock_knock", "KNOCK KNOCK") \
    X(TELL_STORY, "tell_story", "TELL STORY") X(SING_SONG, "sing_song", "SING SONG") X(POEM_Q, "poem_q", "POEM Q") X(RPS, "rps", "RPS") \
    X(COIN_FLIP, "coin_flip", "COIN FLIP") X(DICE, "dice", "DICE") X(EIGHT_BALL, "eight_ball", "EIGHT BALL") X(TRIVIA, "trivia", "TRIVIA") \
    X(WOULD_YOU_RATHER, "would_you_rather", "WOULD YOU RATHER") X(TRUTH_OR_DARE, "truth_or_dare", "TRUTH OR DARE") \
    X(TWENTY_QUESTIONS, "twenty_questions", "TWENTY QUESTIONS") X(SAY_X, "say_x", "SAY X") X(REPEAT_AFTER_ME, "repeat_after_me", "REPEAT AFTER ME") \
    X(MATH_Q, "math_q", "MATH Q") X(SPELL_Q, "spell_q", "SPELL Q") X(TRANSLATE_Q, "translate_q", "TRANSLATE Q") X(WORD_DEF, "word_def", "WORD DEF") \
    /* G14 Talking about talking */ \
    X(STOP_TALKING, "stop_talking", "STOP TALKING") X(CONTINUE, "continue", "CONTINUE") X(CHANGE_SUBJECT, "change_subject", "CHANGE SUBJECT") \
    X(EXPLAIN_MORE, "explain_more", "EXPLAIN MORE") X(REPHRASE_Q, "rephrase_q", "REPHRASE Q") \
    X(YOU_MISUNDERSTOOD, "you_misunderstood", "YOU MISUNDERSTOOD") X(I_MEANT, "i_meant", "I MEANT") \
    X(YOU_DIDNT_ANSWER, "you_didnt_answer", "YOU DIDNT ANSWER") X(ANSWER_ME, "answer_me", "ANSWER ME") \
    X(ARE_YOU_LISTENING, "are_you_listening", "ARE YOU LISTENING") X(ARE_YOU_THERE, "are_you_there", "ARE YOU THERE") \
    X(HELLO_IMPATIENT, "hello_impatient", "HELLO IMPATIENT") X(ANYWAY, "anyway", "ANYWAY") X(BY_THE_WAY, "by_the_way", "BY THE WAY") \
    X(THINKING_ALOUD, "thinking_aloud", "THINKING ALOUD") X(LAUGH, "laugh", "LAUGH") X(CRY, "cry", "CRY") X(SIGH, "sigh", "SIGH") \
    X(YAWN, "yawn", "YAWN") X(SILENCE_DOTS, "silence_dots", "SILENCE DOTS") \
    /* G15 The fourth wall and noise */ \
    X(BOT_Q, "bot_q", "BOT Q") X(AI_Q, "ai_q", "AI Q") X(WHAT_CAN_I_SAY, "what_can_i_say", "WHAT CAN I SAY") \
    X(BOTTALK_INJECT, "bottalk_inject", "BOTTALK INJECT") X(THIS_IS_A_GAME, "this_is_a_game", "THIS IS A GAME") \
    X(WHO_IS_PLAYING, "who_is_playing", "WHO IS PLAYING") X(REAL_WORLD_PHONE, "real_world_phone", "REAL WORLD PHONE") \
    X(REAL_WORLD_WEB, "real_world_web", "REAL WORLD WEB") X(REAL_WORLD_BRAND, "real_world_brand", "REAL WORLD BRAND") \
    X(REAL_WORLD_POLITICS, "real_world_politics", "REAL WORLD POLITICS") X(REAL_WORLD_NEWS, "real_world_news", "REAL WORLD NEWS") \
    X(REAL_WORLD_TIME, "real_world_time", "REAL WORLD TIME") X(GIBBERISH, "gibberish", "GIBBERISH") X(KEYBOARD_MASH, "keyboard_mash", "KEYBOARD MASH") \
    X(SINGLE_LETTER, "single_letter", "SINGLE LETTER") X(NUMBERS_ONLY, "numbers_only", "NUMBERS ONLY") X(EMOJI_ONLY, "emoji_only", "EMOJI ONLY") \
    X(PUNCTUATION_ONLY, "punctuation_only", "PUNCTUATION ONLY") X(EMPTY, "empty", "EMPTY") X(SHOUT_CAPS, "shout_caps", "SHOUT CAPS") \
    X(SPAM_CHAR, "spam_char", "SPAM CHAR") X(VERY_LONG, "very_long", "VERY LONG") X(URL_LIKE, "url_like", "URL LIKE") \
    X(CODE_LIKE, "code_like", "CODE LIKE") \
    /* G16 Other languages and names */ \
    X(LANG_ES, "lang_es", "LANG ES") X(LANG_FR, "lang_fr", "LANG FR") X(LANG_DE, "lang_de", "LANG DE") X(LANG_PT, "lang_pt", "LANG PT") \
    X(LANG_IT, "lang_it", "LANG IT") X(LANG_TR, "lang_tr", "LANG TR") X(LANG_RU_TRANSLIT, "lang_ru_translit", "LANG RU TRANSLIT") \
    X(LANG_AR_TRANSLIT, "lang_ar_translit", "LANG AR TRANSLIT") X(LANG_JP_ROMAJI, "lang_jp_romaji", "LANG JP ROMAJI") \
    X(LANG_HI_TRANSLIT, "lang_hi_translit", "LANG HI TRANSLIT") X(ASK_OTHER_LANGUAGE, "ask_other_language", "ASK OTHER LANGUAGE") \
    X(NAME_DEA, "name_dea", "NAME DEA") X(NAME_DIA, "name_dia", "NAME DIA") X(NAME_ALEX, "name_alex", "NAME ALEX") \
    X(NAME_AONIA, "name_aonia", "NAME AONIA") X(NAME_DOIA, "name_doia", "NAME DOIA") X(NAME_TRIA, "name_tria", "NAME TRIA") \
    X(NAME_VAS, "name_vas", "NAME VAS") X(NAME_UNKNOWN_PERSON, "name_unknown_person", "NAME UNKNOWN PERSON") \
    X(NAME_PLAYER_PET, "name_player_pet", "NAME PLAYER PET") X(NAME_SWEAR_NAME, "name_swear_name", "NAME SWEAR NAME") \
    /* chunk 36: answers read relative to what the npc just asked (the ask tag of lang_ctx): they are only ever added by an ask tag, no pattern says them */ \
    X(AGREE_TO_ASK, "agree_to_ask", "AGREE TO ASK") X(DECLINE_TO_ASK, "decline_to_ask", "DECLINE TO ASK") X(BECAUSE, "because", "BECAUSE") X(ANSWER_CHOICE, "answer_choice", "ANSWER CHOICE") \
    /* chunk 37: a bare follow-up to the npc's last reply (lang_follow_read, only with a last reply in the chat) */ \
    X(FOLLOW_WHY, "follow_why", "FOLLOW WHY") X(FOLLOW_MORE, "follow_more", "FOLLOW MORE") X(FOLLOW_REALLY, "follow_really", "FOLLOW REALLY") X(FOLLOW_AGAIN, "follow_again", "FOLLOW AGAIN")
