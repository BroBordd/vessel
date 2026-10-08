/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * LINES, part B: answers to the social basics (hello, bye, thanks, sorry, how are you, who are you ...).
 * see lines_a.c for how to read a row. keys are "r." + the intent name from lang.c. */
#include "lang_data.h"

const Line LINES_B[] = {
    /* ---- r.greet ---- */
    {"r.greet", WARM, ALLS, "{greet}, {pet}. I am glad you came."}, {"r.greet", WARM, ALLS, "Hi hi! Welcome, {pet}."}, {"r.greet", WARM, ALLS, "Hello there, {player}. You look well."},
    {"r.greet", WARM, ALLS, "{greet}! It is good to see you."}, {"r.greet", WARM, ALLS, "Hello! What a lovely surprise."},
    {"r.greet", NEUT, ALLS, "Hello, {player}."}, {"r.greet", NEUT, ALLS, "{greet}. What is it?"}, {"r.greet", NEUT, ALLS, "Hi. State your business."}, {"r.greet", NEUT, ALLS, "Greetings, {pet}."}, {"r.greet", NEUT, ALLS, "Hello. I was expecting you."},
    {"r.greet", COLD, ALLS, "Oh. Hi. I guess."}, {"r.greet", COLD, ALLS, "{greet}. Do not get comfortable."}, {"r.greet", COLD, ALLS, "Hello. That is all you have?"}, {"r.greet", COLD, ALLS, "Hi, {pet}. Try harder."}, {"r.greet", COLD, ALLS, "Yeah, hi. Moving on."},
    {"r.greet", HOST, ALLS, "Hello? That is your big opening, {pet}?"}, {"r.greet", HOST, ALLS, "Oh, it talks. Hi, I guess."}, {"r.greet", HOST, ALLS, "Spare me your greetings."}, {"r.greet", HOST, ALLS, "Do not say hi to me like we are friends."},
    {"r.greet", NEUT, DIVINE, "Hello, {player}. A polite vessel. How rare."}, {"r.greet", NEUT, DIVINE, "Greetings. I allow it."}, {"r.greet", NEUT, DIVINE, "Hello, vessel. Proceed."},
    {"r.greet", COLD, DIVINE, "Hello, {pet}. Do you want a medal for talking?"}, {"r.greet", COLD, DIVINE, "A greeting. Thrilling. I almost felt something."}, {"r.greet", COLD, DIVINE, "Hi. Is that all your tiny mind produced?"}, {"r.greet", COLD, DIVINE, "Yes, yes. Hello. I am a goddess. You are not."},
    {"r.greet", HOST, DIVINE, "You dare greet me like an equal, {pet}?"}, {"r.greet", HOST, DIVINE, "Hi? To a goddess? Show some manners, worm."}, {"r.greet", HOST, DIVINE, "Do not waste my breath on hello."},
    {"r.greet", WARM, DIVINE, "Hello, little one. Your voice is a pleasant surprise."},
    {"r.greet", WARM, STREET, "Yo, {pet}! Good to see you."}, {"r.greet", NEUT, STREET, "Hey. Sup."}, {"r.greet", COLD, STREET, "Yeah, hey. Whatever."}, {"r.greet", HOST, STREET, "Oh great, you again. Hi."},

    /* ---- r.bye ---- */
    {"r.bye", WARM, ALLS, "{bye}, {pet}. Come back soon."}, {"r.bye", WARM, ALLS, "Goodbye! Be careful out there."}, {"r.bye", WARM, ALLS, "Take care, {player}. I will miss the company."}, {"r.bye", WARM, ALLS, "See you soon, {pet}!"},
    {"r.bye", NEUT, ALLS, "{bye}, {player}."}, {"r.bye", NEUT, ALLS, "Farewell. Do not be late."}, {"r.bye", NEUT, ALLS, "Go on, then. Goodbye."}, {"r.bye", NEUT, ALLS, "Until next time."},
    {"r.bye", COLD, ALLS, "Bye. Do not let the door hit you."}, {"r.bye", COLD, ALLS, "Finally. Bye."}, {"r.bye", COLD, ALLS, "{bye}. I will survive without you."}, {"r.bye", COLD, ALLS, "Already leaving? Shame. Not."},
    {"r.bye", HOST, ALLS, "Good riddance."}, {"r.bye", HOST, ALLS, "Go on, get lost. Do not come back."}, {"r.bye", HOST, ALLS, "Bye! No, really. Bye."},
    {"r.bye", NEUT, DIVINE, "Go, vessel. The world will not wait."}, {"r.bye", COLD, DIVINE, "Leave, then. I will not weep, mortal."}, {"r.bye", COLD, DIVINE, "Go. Your absence is a gift."}, {"r.bye", HOST, DIVINE, "Begone, worm. I have clouds to watch."},
    {"r.bye", WARM, DIVINE, "Go in peace, little one. I will be watching. Gently."},
    {"r.bye", WARM, STREET, "Later, {pet}! Stay safe."}, {"r.bye", COLD, STREET, "Yeah, bye. See you never."}, {"r.bye", NEUT, STREET, "Catch you later."},

    /* ---- r.yes / r.no / r.ok: when it is all they said ---- */
    {"r.yes", WARM, ALLS, "Great! I am glad to hear it."}, {"r.yes", WARM, ALLS, "Wonderful, {pet}."}, {"r.yes", NEUT, ALLS, "Good. I expected that."}, {"r.yes", NEUT, ALLS, "Yes, what, exactly?"}, {"r.yes", NEUT, ALLS, "Alright then."},
    {"r.yes", COLD, ALLS, "Yes what? Use a full sentence."}, {"r.yes", COLD, ALLS, "Wow. Yes. Riveting."}, {"r.yes", HOST, ALLS, "Yes? Yes what, you fool?"},
    {"r.yes", COLD, DIVINE, "Yes. Brilliant. A vessel with an opinion."}, {"r.yes", HOST, DIVINE, "Yes? To what, worm? Speak properly."},
    {"r.no", WARM, ALLS, "Oh. That is alright, {pet}."}, {"r.no", WARM, ALLS, "No? Okay. I understand."}, {"r.no", NEUT, ALLS, "No. Fine."}, {"r.no", NEUT, ALLS, "Alright. No it is."}, {"r.no", NEUT, ALLS, "Noted. No."},
    {"r.no", COLD, ALLS, "No what? You are boring me."}, {"r.no", COLD, ALLS, "Fine. No. Whatever."}, {"r.no", HOST, ALLS, "No? You do not get to say no to me."},
    {"r.no", COLD, DIVINE, "No. How decisive. How dull."}, {"r.no", HOST, DIVINE, "You say no to a goddess? Bold. Foolish."},
    {"r.ok", WARM, ALLS, "Good. We understand each other."}, {"r.ok", WARM, ALLS, "Lovely, {pet}."}, {"r.ok", NEUT, ALLS, "Good."}, {"r.ok", NEUT, ALLS, "Then we are agreed."}, {"r.ok", NEUT, ALLS, "Glad that is settled."},
    {"r.ok", COLD, ALLS, "Okay. Wow. Such enthusiasm."}, {"r.ok", COLD, ALLS, "Fine. Great. Whatever."}, {"r.ok", HOST, ALLS, "Okay? That is all you can say?"},
    {"r.ok", COLD, DIVINE, "Good. Obedience suits you, vessel."}, {"r.ok", HOST, DIVINE, "Okay, he says. To a goddess. Amusing."},

    /* ---- r.thanks ---- */
    {"r.thanks", WARM, ALLS, "You are very welcome, {pet}."}, {"r.thanks", WARM, ALLS, "Aww, no need to thank me."}, {"r.thanks", WARM, ALLS, "Anytime, {player}. I mean it."}, {"r.thanks", WARM, ALLS, "My pleasure!"},
    {"r.thanks", NEUT, ALLS, "You are welcome."}, {"r.thanks", NEUT, ALLS, "Do not mention it."}, {"r.thanks", NEUT, ALLS, "Mm. Noted."}, {"r.thanks", NEUT, ALLS, "That was nothing."},
    {"r.thanks", COLD, ALLS, "Thanks? I did not do it for you."}, {"r.thanks", COLD, ALLS, "Do not thank me. It gets awkward."}, {"r.thanks", COLD, ALLS, "Yes, yes. You are welcome. Moving on."}, {"r.thanks", COLD, ALLS, "Save your thanks for someone who cares."},
    {"r.thanks", HOST, ALLS, "Your thanks are worthless to me."}, {"r.thanks", HOST, ALLS, "Do not thank me. Just go away."},
    {"r.thanks", NEUT, DIVINE, "Gratitude. A fine quality in a vessel."}, {"r.thanks", COLD, DIVINE, "You thank me? As you should, mortal."}, {"r.thanks", COLD, DIVINE, "Of course you are thankful. I am wonderful."},
    {"r.thanks", HOST, DIVINE, "Thank me by leaving, worm."}, {"r.thanks", WARM, DIVINE, "Such sweet manners, little one. You may thank me again."},
    {"r.thanks", WARM, STREET, "No problem, {pet}!"}, {"r.thanks", COLD, STREET, "Yeah, yeah. Do not mention it."},

    /* ---- r.sorry ---- */
    {"r.sorry", WARM, ALLS, "It is alright, {pet}. Do not worry."}, {"r.sorry", WARM, ALLS, "Oh, do not apologize. We are fine."}, {"r.sorry", WARM, ALLS, "Forgiven. Now smile."}, {"r.sorry", WARM, ALLS, "Everyone slips up, {player}. It is okay."},
    {"r.sorry", NEUT, ALLS, "Apology accepted."}, {"r.sorry", NEUT, ALLS, "Fine. Let us not speak of it again."}, {"r.sorry", NEUT, ALLS, "Hm. I will allow it."}, {"r.sorry", NEUT, ALLS, "You should be. But alright."},
    {"r.sorry", COLD, ALLS, "Sorry does not fix anything. But fine."}, {"r.sorry", COLD, ALLS, "Sure you are. Save it."}, {"r.sorry", COLD, ALLS, "Sorry? That is what they all say."}, {"r.sorry", COLD, ALLS, "An apology. Wow. Is it supposed to help?"},
    {"r.sorry", HOST, ALLS, "Sorry? You are sorry all right. Sorry sight."}, {"r.sorry", HOST, ALLS, "I do not want your apology. I want your silence."},
    {"r.sorry", NEUT, DIVINE, "Humility. Good. A vessel must learn it."}, {"r.sorry", COLD, DIVINE, "Grovel some more, mortal. I might notice."}, {"r.sorry", COLD, DIVINE, "Sorry, he says. The gods have heard worse. Barely."},
    {"r.sorry", HOST, DIVINE, "Sorry? Words from a worm. Ignored."}, {"r.sorry", WARM, DIVINE, "Shh. Forgiven. You are trying, little one."},
    {"r.sorry", WARM, STREET, "Hey, no sweat, {pet}."}, {"r.sorry", COLD, STREET, "Yeah, whatever. Sorry accepted. Barely."},

    /* ---- r.howru ---- */
    {"r.howru", WARM, ALLS, "I am wonderful, {pet}, thank you for asking!"}, {"r.howru", WARM, ALLS, "Doing great. You are sweet to ask."}, {"r.howru", WARM, ALLS, "Better now that you are here."}, {"r.howru", WARM, ALLS, "Oh, I am well. And you, {player}?"},
    {"r.howru", NEUT, ALLS, "I am fine. Thank you."}, {"r.howru", NEUT, ALLS, "Same as always. And you?"}, {"r.howru", NEUT, ALLS, "Fine. Do not make it weird."}, {"r.howru", NEUT, ALLS, "Surviving. You?"},
    {"r.howru", COLD, ALLS, "Why do you care how I am?"}, {"r.howru", COLD, ALLS, "I was fine before you asked."}, {"r.howru", COLD, ALLS, "I am fine. Do not make small talk."}, {"r.howru", COLD, ALLS, "Tired. Of this conversation."},
    {"r.howru", HOST, ALLS, "Better when you are not around."}, {"r.howru", HOST, ALLS, "How am I? Annoyed. Guess why."},
    {"r.howru", NEUT, DIVINE, "A goddess does not have bad days. Only mortals have those."}, {"r.howru", COLD, DIVINE, "Eternal. Bored. Slightly more bored now."}, {"r.howru", COLD, DIVINE, "Magnificent, as always. You may envy me."},
    {"r.howru", HOST, DIVINE, "You ask a goddess how she is? Presumptuous, worm."}, {"r.howru", WARM, DIVINE, "Radiant, little one. Your concern warms me."},
    {"r.howru", WARM, STREET, "Pretty good, {pet}! You?"}, {"r.howru", COLD, STREET, "Meh. Do not ask."}, {"r.howru", NEUT, STREET, "Can not complain. Well, I can, but I will not."},

    /* ---- r.whoru ---- */
    {"r.whoru", WARM, ALLS, "I am {npc}. It is lovely to meet you, {pet}."}, {"r.whoru", WARM, ALLS, "The name is {npc}. Glad to know you!"}, {"r.whoru", WARM, ALLS, "I am {npc}, your friend, I hope."},
    {"r.whoru", NEUT, ALLS, "I am {npc}."}, {"r.whoru", NEUT, ALLS, "They call me {npc}. Remember it."}, {"r.whoru", NEUT, ALLS, "{npc}. That is all you need to know."}, {"r.whoru", NEUT, ALLS, "I am {npc}. Pleased to meet you."},
    {"r.whoru", COLD, ALLS, "{npc}. Not that it matters to you."}, {"r.whoru", COLD, ALLS, "Someone more important than you."}, {"r.whoru", COLD, ALLS, "I am {npc}. Write it down, you will forget."},
    {"r.whoru", HOST, ALLS, "I am {npc}, and you are bothering me."}, {"r.whoru", HOST, ALLS, "Who I am is none of your business, {pet}."},
    {"r.whoru", NEUT, DIVINE, "I am {npc}. Goddess. The one who decides what you are."}, {"r.whoru", NEUT, DIVINE, "{npc}. Dea, goddess. Latin for exactly what I am."}, {"r.whoru", NEUT, DIVINE, "I am the one who named you. Do not forget it."},
    {"r.whoru", COLD, DIVINE, "I am {npc}, goddess. The sky bows to me, you may too."}, {"r.whoru", COLD, DIVINE, "{npc}. You asked a goddess her name. Brave. Silly."}, {"r.whoru", COLD, DIVINE, "Dea. Goddess. Your boss, technically."},
    {"r.whoru", HOST, DIVINE, "I am {npc}. A goddess. And you are standing too close."}, {"r.whoru", HOST, DIVINE, "Your better, worm. That is all you need."},
    {"r.whoru", WARM, DIVINE, "I am {npc}, little one. Goddess, guide, and, today, kind."},
    {"r.whoru", WARM, STREET, "Name is {npc}. Nice to meet you, {pet}!"}, {"r.whoru", NEUT, STREET, "I am {npc}. Just a guy trying to get by."}, {"r.whoru", COLD, STREET, "Name is {npc}. Do not wear it out."},

    /* ---- r.where ---- */
    {"r.where", WARM, ALLS, "You are somewhere safe, {pet}. Do not worry."}, {"r.where", WARM, ALLS, "A good place, I promise. You will get used to it."}, {"r.where", WARM, ALLS, "Right here with me. That is what matters."},
    {"r.where", NEUT, ALLS, "Between places, for now."}, {"r.where", NEUT, ALLS, "Somewhere you will understand later."}, {"r.where", NEUT, ALLS, "You will learn soon enough."},
    {"r.where", COLD, ALLS, "Somewhere you do not belong yet."}, {"r.where", COLD, ALLS, "Does it matter? You will leave soon."}, {"r.where", COLD, ALLS, "Look around, {pet}. Figure it out."},
    {"r.where", HOST, ALLS, "Lost already? Of course you are."}, {"r.where", HOST, ALLS, "Somewhere too good for you."},
    {"r.where", NEUT, DIVINE, "Between places. A cloud, if your mind needs a name for it."}, {"r.where", COLD, DIVINE, "In my domain, mortal. Do not touch anything."}, {"r.where", COLD, DIVINE, "Above the world. Where gods sit and mortals wonder."},
    {"r.where", HOST, DIVINE, "In the sky, fool. Look down. Do not fall. Yet."}, {"r.where", WARM, DIVINE, "In my little corner of heaven, dear vessel."},
    {"r.where", WARM, STREET, "Pretty nice spot, right, {pet}?"}, {"r.where", COLD, STREET, "Not my problem where you are."},

    /* ---- r.whoami ---- */
    {"r.whoami", WARM, ALLS, "You are {player}, {pet}. And you are doing just fine."}, {"r.whoami", WARM, ALLS, "Someone worth knowing, {player}."},
    {"r.whoami", NEUT, ALLS, "You are {player}. Remember that."}, {"r.whoami", NEUT, ALLS, "You will find out who you are."}, {"r.whoami", NEUT, ALLS, "That is for you to answer, {pet}."},
    {"r.whoami", COLD, ALLS, "A nobody with a name. Congratulations."}, {"r.whoami", COLD, ALLS, "You are {player}. Try not to embarrass it."}, {"r.whoami", HOST, ALLS, "A fool with an echo for a mind."},
    {"r.whoami", NEUT, DIVINE, "You are a vessel. A vessel needs a name. You are {player}."}, {"r.whoami", NEUT, DIVINE, "Vas was Latin for vessel. Aonia plays on one. You are the first."}, {"r.whoami", NEUT, DIVINE, "A vessel. Empty, for now. Filled by missions."},
    {"r.whoami", COLD, DIVINE, "A vessel, mortal. A cup. I fill it."}, {"r.whoami", COLD, DIVINE, "You are what I made you. Do not get ideas."}, {"r.whoami", COLD, DIVINE, "A vessel with a name I gave. Be grateful."},
    {"r.whoami", HOST, DIVINE, "A worm I named for fun. Do not overthink it."}, {"r.whoami", WARM, DIVINE, "You are my vessel, little one. Precious, in your way."},
    {"r.whoami", COLD, STREET, "You tell me, {pet}."}, {"r.whoami", WARM, STREET, "A newcomer with potential, {player}."},

    /* ---- r.intro: "my name is X" ---- */
    {"r.intro", WARM, ALLS, "{topic}. What a nice name! I am happy to meet you."}, {"r.intro", WARM, ALLS, "Pleased to meet you, {topic}. I will remember."},
    {"r.intro", NEUT, ALLS, "{topic}, huh? Alright, {topic}."}, {"r.intro", NEUT, ALLS, "I will call you {topic}, then."}, {"r.intro", NEUT, ALLS, "Noted. {topic}."},
    {"r.intro", COLD, ALLS, "{topic}. Cute. I did not ask."}, {"r.intro", COLD, ALLS, "I will forget {topic} by tomorrow."}, {"r.intro", COLD, ALLS, "Good for you. I do not care."},
    {"r.intro", HOST, ALLS, "I did not ask your name, {pet}."}, {"r.intro", NEUT, DIVINE, "{topic}. Interesting. But you are {player}, as I decided."},
    {"r.intro", COLD, DIVINE, "Call yourself what you like. I call you {player}."}, {"r.intro", COLD, DIVINE, "{topic}? A goddess names, mortal. You do not."},
    {"r.intro", HOST, DIVINE, "Your name is whatever I say. Currently, {player}."}, {"r.intro", WARM, DIVINE, "{topic} is lovely. But {player} suits you best, little one."},
    {"r.intro", WARM, STREET, "Nice to meet you, {topic}!"}, {"r.intro", COLD, STREET, "Cool. {topic}. Whatever."},

    /* ---- r.mission ---- */
    {"r.mission", WARM, ALLS, "Do your best, {pet}. I believe in you."}, {"r.mission", WARM, ALLS, "Take it one step at a time, {player}."}, {"r.mission", WARM, ALLS, "You will do wonderfully. I know it."},
    {"r.mission", NEUT, ALLS, "Carry out your missions. Nothing more, nothing less."}, {"r.mission", NEUT, ALLS, "Finish each one. That is the whole job."}, {"r.mission", NEUT, ALLS, "Follow the list on your screen."}, {"r.mission", NEUT, ALLS, "One mission at a time. Then the next."},
    {"r.mission", COLD, ALLS, "Do what you are told. It is not complicated."}, {"r.mission", COLD, ALLS, "Missions. You do them. That is all."}, {"r.mission", COLD, ALLS, "Do I look like a guide to you?"},
    {"r.mission", HOST, ALLS, "Your purpose is to stop asking me things."}, {"r.mission", HOST, ALLS, "Do the work, {pet}. Stop whining."},
    {"r.mission", NEUT, DIVINE, "Your purpose is to carry out missions. Nothing more, nothing less."}, {"r.mission", NEUT, DIVINE, "Finish each mission, and stay secret."},
    {"r.mission", COLD, DIVINE, "Missions, mortal. Finish them. Stay secret. Then your life ends."}, {"r.mission", COLD, DIVINE, "You are a tool, vessel. Tools do not ask what for."}, {"r.mission", COLD, DIVINE, "Do the missions. Do not die early. Do not be annoying."},
    {"r.mission", HOST, DIVINE, "Purpose? You are a worm with a task. Do it."}, {"r.mission", WARM, DIVINE, "Carry the missions, little one. I trust you, a little."},
    {"r.mission", WARM, STREET, "Just do your thing, {pet}, you will be fine."}, {"r.mission", COLD, STREET, "Figure it out, rookie."},
};
const int LINES_B_N = (int)(sizeof LINES_B / sizeof LINES_B[0]);
