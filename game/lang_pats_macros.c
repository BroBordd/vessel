/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE PATTERN MACROS (docs/convo_plan.md, chunk 26; chunk 62 grows the library to >= 150).
 * a row is "name|alt|alt|...": a pattern writes <name> and means any one of the alternatives (which may hold spaces, never * ? $ [ ( <).
 * built in, not rows: <x> (any 1-3 words). three names also match more than their row: <number> (digits), <time> (3pm 10:30), <npc> (the npc's name,
 * lpat_set_npc); a row with those names only ADDS words. every alternative is lower case; a later row of the same name replaces the earlier. */
#include "lang_data.h"

const char *const MACROS_A[] = {
    "you|you|u|ya|yourself|yourselves|thee|thou",
    "me|me|myself|i|im|i'm",
    "yes|yes|yeah|yep|yup|yea|ya|sure|ok|okay|aye|indeed|affirmative|of course|definitely",
    "no|no|nope|nah|naw|never|negative|not really|no way",
    "please|please|pls|plz|plss|pretty please|if you please|kindly",
    "very|very|so|really|extremely|super|quite|too|well|damn|totally|truly",
    "is|is|are|am|was|were|be|been|s|re",
    "want|want|wanna|need|wish|would like|d like|desire|crave",
    "tell|tell|say|show|explain|teach|give me|let me know",
    "give|give|hand|pass|bring|lend|get|fetch|grant",
    "take|take|bring|lead|carry|send|walk|guide",
    "npc|dea|goddess|alex|dia|hey you",
    "pet|child|mortal|girl|boy|kid|fool|human|little one|dear|friend",
    "greetword|hi|hello|hey|heya|hiya|howdy|yo|sup|greetings|hail|good day|well met|ahoy|hullo|hallo",
    "byeword|bye|goodbye|farewell|cya|see ya|see you|later|laters|adios|ciao|so long|take care|bye bye|night|goodnight",
    "thanksword|thanks|thank you|thx|ty|tysm|cheers|much obliged|appreciated|thanks a lot|many thanks",
    "sorryword|sorry|apologies|my bad|forgive me|pardon me|my apologies|excuse me|oops",
    "filler|um|uh|er|erm|hmm|well|so|like|okay so|anyway|you know|i mean|actually|basically",
    "polite|please|kindly|if you do not mind|if you dont mind|would you|could you|can you|may i",
    "rude|stupid|dumb|idiot|fool|moron|useless|pathetic|worthless|annoying|ugly",
    "nice|nice|kind|good|sweet|lovely|great|wonderful|beautiful|gentle|warm",
    "bad|bad|awful|terrible|horrible|evil|cruel|mean|cold|nasty|rotten",
    "a_or_the|a|an|the|this|that|your|my|some|any",
    "this|this|that|these|those|it",
    "how_are|how are|how r|how is|hows|how's|how have|how do",
    "what_is|what is|what's|whats|wats|what are|what was",
    "who_are|who are|who r|who is|who's|whos|who were",
    "where_is|where is|where's|wheres|where are|where was",
    "name|name|called|nickname|title",
    "number|one|two|three|four|five|six|seven|eight|nine|ten|eleven|twelve|twenty|thirty|forty|fifty|hundred|a few|several|many",
    "time|noon|midnight|morning|evening|night|dawn|dusk|tonight|today|tomorrow|yesterday",
    "orb|orb|sphere|ball|glow|glowing thing|shiny thing|light",
    0
};
