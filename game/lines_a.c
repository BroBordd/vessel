/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * LINES, part A: word pools ("p.*") and conversation flow ("open", "ask.*", "end.*", "demand.*").
 *
 * how to read a row:  { key, tones it fits, styles it fits, "text" }
 *   tones : WARM NEUT COLD HOST (or NICE / RUDE / ALLT)      styles: PLAIN DIVINE STREET (or ALLS)
 *   slots : {player} {npc} {topic} and any pool: {pet} {sigh} {greet} {bye} {pre} {post} {more}
 * keep to the font: letters, digits and , . ! ? ' - only. */
#include "lang_data.h"

const Line LINES_A[] = {
    /* ---- p.pet: what the npc calls the player ---- */
    {"p.pet", WARM, ALLS, "dear"}, {"p.pet", WARM, ALLS, "friend"}, {"p.pet", WARM, ALLS, "{player}"},
    {"p.pet", WARM, DIVINE, "little one"}, {"p.pet", WARM, DIVINE, "dear vessel"}, {"p.pet", WARM, DIVINE, "brave one"},
    {"p.pet", WARM, STREET, "buddy"}, {"p.pet", WARM, STREET, "pal"}, {"p.pet", WARM, STREET, "my friend"},
    {"p.pet", NEUT, ALLS, "{player}"}, {"p.pet", NEUT, ALLS, "traveler"}, {"p.pet", NEUT, ALLS, "stranger"},
    {"p.pet", NEUT, DIVINE, "vessel"}, {"p.pet", NEUT, DIVINE, "{player}"}, {"p.pet", NEUT, DIVINE, "small one"},
    {"p.pet", NEUT, STREET, "newbie"}, {"p.pet", NEUT, STREET, "friend"}, {"p.pet", NEUT, STREET, "{player}"},
    {"p.pet", COLD, ALLS, "kid"}, {"p.pet", COLD, ALLS, "you"}, {"p.pet", COLD, ALLS, "{player}"},
    {"p.pet", COLD, DIVINE, "mortal"}, {"p.pet", COLD, DIVINE, "little vessel"}, {"p.pet", COLD, DIVINE, "clay"}, {"p.pet", COLD, DIVINE, "tiny thing"},
    {"p.pet", COLD, STREET, "rookie"}, {"p.pet", COLD, STREET, "kid"}, {"p.pet", COLD, STREET, "newbie"},
    {"p.pet", HOST, ALLS, "fool"}, {"p.pet", HOST, ALLS, "pest"}, {"p.pet", HOST, ALLS, "nobody"},
    {"p.pet", HOST, DIVINE, "worm"}, {"p.pet", HOST, DIVINE, "insect"}, {"p.pet", HOST, DIVINE, "nothing"}, {"p.pet", HOST, DIVINE, "dust"},
    {"p.pet", HOST, STREET, "idiot"}, {"p.pet", HOST, STREET, "clown"}, {"p.pet", HOST, STREET, "loser"},

    /* ---- p.sigh ---- */
    {"p.sigh", WARM, ALLS, "hehe"}, {"p.sigh", WARM, ALLS, "ah"}, {"p.sigh", WARM, ALLS, "oh my"}, {"p.sigh", WARM, ALLS, "aww"},
    {"p.sigh", NEUT, ALLS, "hm"}, {"p.sigh", NEUT, ALLS, "well"}, {"p.sigh", NEUT, ALLS, "so"}, {"p.sigh", NEUT, ALLS, "right"},
    {"p.sigh", COLD, ALLS, "ugh"}, {"p.sigh", COLD, ALLS, "tch"}, {"p.sigh", COLD, ALLS, "sigh"}, {"p.sigh", COLD, ALLS, "whatever"},
    {"p.sigh", HOST, ALLS, "ugh, really"}, {"p.sigh", HOST, ALLS, "wow"}, {"p.sigh", HOST, ALLS, "pathetic"}, {"p.sigh", HOST, ALLS, "unbelievable"},

    /* ---- p.greet / p.bye: single words to drop into a sentence ---- */
    {"p.greet", WARM, ALLS, "hello"}, {"p.greet", WARM, ALLS, "hi"}, {"p.greet", WARM, ALLS, "hey"}, {"p.greet", WARM, ALLS, "welcome"},
    {"p.greet", NEUT, ALLS, "hello"}, {"p.greet", NEUT, ALLS, "greetings"}, {"p.greet", NEUT, ALLS, "hi"},
    {"p.greet", COLD, ALLS, "hi"}, {"p.greet", COLD, ALLS, "yes yes, hello"}, {"p.greet", COLD, ALLS, "hello, I guess"},
    {"p.greet", HOST, ALLS, "oh, hello"}, {"p.greet", HOST, ALLS, "hi, how unfortunate"}, {"p.greet", HOST, ALLS, "ugh, hello"},
    {"p.bye", WARM, ALLS, "goodbye"}, {"p.bye", WARM, ALLS, "see you soon"}, {"p.bye", WARM, ALLS, "take care"}, {"p.bye", WARM, ALLS, "farewell, friend"},
    {"p.bye", NEUT, ALLS, "farewell"}, {"p.bye", NEUT, ALLS, "bye"}, {"p.bye", NEUT, ALLS, "go on, then"},
    {"p.bye", COLD, ALLS, "bye"}, {"p.bye", COLD, ALLS, "leave, then"}, {"p.bye", COLD, ALLS, "off you go"},
    {"p.bye", HOST, ALLS, "good riddance"}, {"p.bye", HOST, ALLS, "get lost"}, {"p.bye", HOST, ALLS, "begone"},

    /* ---- p.pre: an opener, sometimes stuck in front of a reply ---- */
    {"p.pre", WARM, ALLS, "Oh!"}, {"p.pre", WARM, ALLS, "Ah, well."}, {"p.pre", WARM, ALLS, "Hehe."}, {"p.pre", WARM, ALLS, "Now then."},
    {"p.pre", NEUT, ALLS, "Hm."}, {"p.pre", NEUT, ALLS, "Well."}, {"p.pre", NEUT, ALLS, "I see."}, {"p.pre", NEUT, ALLS, "Right."},
    {"p.pre", COLD, ALLS, "Ugh."}, {"p.pre", COLD, ALLS, "Tch."}, {"p.pre", COLD, ALLS, "Honestly."}, {"p.pre", COLD, ALLS, "Sigh."}, {"p.pre", COLD, ALLS, "Fine."},
    {"p.pre", HOST, ALLS, "Wow."}, {"p.pre", HOST, ALLS, "Seriously?"}, {"p.pre", HOST, ALLS, "Pathetic."}, {"p.pre", HOST, ALLS, "Tch."}, {"p.pre", HOST, ALLS, "Unbelievable."},
    {"p.pre", COLD, DIVINE, "How tiresome."}, {"p.pre", COLD, DIVINE, "Spare me."}, {"p.pre", HOST, DIVINE, "Silence, worm."}, {"p.pre", HOST, DIVINE, "Do not strain yourself."},
    {"p.pre", NEUT, DIVINE, "Mortal."}, {"p.pre", WARM, STREET, "Hey now."}, {"p.pre", COLD, STREET, "Look."}, {"p.pre", HOST, STREET, "Really, {pet}?"},

    /* ---- p.post: a closer, sometimes stuck after a reply ---- */
    {"p.post", WARM, ALLS, "Take care of yourself."}, {"p.post", WARM, ALLS, "I am glad you asked."}, {"p.post", WARM, ALLS, "You are doing fine, {pet}."},
    {"p.post", WARM, ALLS, "Smile a little."}, {"p.post", WARM, ALLS, "Ask me anything."},
    {"p.post", NEUT, ALLS, "Anything else?"}, {"p.post", NEUT, ALLS, "That is all for now."}, {"p.post", NEUT, ALLS, "Keep going."}, {"p.post", NEUT, ALLS, "Make of that what you will."},
    {"p.post", COLD, ALLS, "Not that I care."}, {"p.post", COLD, ALLS, "Whatever."}, {"p.post", COLD, ALLS, "Are we done?"}, {"p.post", COLD, ALLS, "Do not expect more."}, {"p.post", COLD, ALLS, "I do not care."},
    {"p.post", HOST, ALLS, "Now stop wasting my time."}, {"p.post", HOST, ALLS, "Go away."}, {"p.post", HOST, ALLS, "I said what I said."}, {"p.post", HOST, ALLS, "You bore me."},
    {"p.post", COLD, DIVINE, "I have eons, you have minutes."}, {"p.post", COLD, DIVINE, "Try to be less boring."}, {"p.post", COLD, DIVINE, "I do not care. Truly."}, {"p.post", COLD, DIVINE, "Your opinion is noted. And ignored."},
    {"p.post", HOST, DIVINE, "Worms should not speak."}, {"p.post", HOST, DIVINE, "Be grateful I answer at all."}, {"p.post", HOST, DIVINE, "I could end you with a blink."},
    {"p.post", NEUT, DIVINE, "Do not make me repeat myself."}, {"p.post", WARM, DIVINE, "Do not tell the others I was nice."},
    {"p.post", WARM, STREET, "No worries, {pet}."}, {"p.post", COLD, STREET, "Anyway."}, {"p.post", HOST, STREET, "Get out of my face."},

    /* ---- open: the npc starts a free chat ---- */
    {"open", WARM, ALLS, "{greet}, {pet}! Need something?"}, {"open", WARM, ALLS, "Oh, it is you! What is on your mind?"},
    {"open", WARM, ALLS, "Good to see you. Want to talk?"}, {"open", WARM, ALLS, "Hey {player}. I have a minute."},
    {"open", NEUT, ALLS, "{greet}. What is it?"}, {"open", NEUT, ALLS, "Yes? You wanted something?"}, {"open", NEUT, ALLS, "I am listening, {pet}."}, {"open", NEUT, ALLS, "Speak."},
    {"open", COLD, ALLS, "What do you want?"}, {"open", COLD, ALLS, "Oh. It is you. What now?"}, {"open", COLD, ALLS, "Make it quick."}, {"open", COLD, ALLS, "Yes? I am busy."},
    {"open", HOST, ALLS, "You again? What do you want?"}, {"open", HOST, ALLS, "Ugh. Talk, then. Quickly."}, {"open", HOST, ALLS, "I have no patience for you today."},
    {"open", NEUT, DIVINE, "You stand before a goddess. Speak, {player}."}, {"open", NEUT, DIVINE, "Yes, vessel? I am listening."},
    {"open", COLD, DIVINE, "Back again, little vessel? What is it?"}, {"open", COLD, DIVINE, "You have my attention. Do not waste it."}, {"open", COLD, DIVINE, "Speak, mortal. I might care."},
    {"open", HOST, DIVINE, "What now? I was enjoying the quiet."}, {"open", HOST, DIVINE, "You again. Speak, worm, and be brief."},
    {"open", WARM, DIVINE, "Ah, {player}. Come, speak with me."},
    {"open", WARM, STREET, "Yo {pet}! What is up?"}, {"open", NEUT, STREET, "Hey. Something on your mind?"}, {"open", COLD, STREET, "Yeah? What?"}, {"open", HOST, STREET, "What do you want now, {pet}?"},

    /* ---- open.again: talking to someone you already spoke to ---- */
    {"open.again", WARM, ALLS, "Back already? I do not mind, {pet}."}, {"open.again", WARM, ALLS, "Hello again! Anything else?"},
    {"open.again", NEUT, ALLS, "Back so soon, {player}? Go on."}, {"open.again", NEUT, ALLS, "Something else, {pet}?"},
    {"open.again", COLD, ALLS, "You again. What now?"}, {"open.again", COLD, ALLS, "I already told you everything."}, {"open.again", COLD, ALLS, "Did you forget something?"},
    {"open.again", HOST, ALLS, "Again? Do you ever stop?"}, {"open.again", HOST, ALLS, "You are still here? Unbelievable."},
    {"open.again", NEUT, DIVINE, "Now go, {player}. The hole is waiting."}, {"open.again", COLD, DIVINE, "Still here, {pet}? The hole is waiting. Unlike me."},
    {"open.again", COLD, DIVINE, "Why are you not gone yet? The hole is waiting."}, {"open.again", HOST, DIVINE, "The hole is open and you are still talking. Pathetic."},
    {"open.again", WARM, DIVINE, "Ah, you linger. Fine. One more word, {pet}."},
    {"open.again", NEUT, STREET, "Keep your head down, {player}. I will find you when it is time."}, {"open.again", WARM, STREET, "Hey again! Keep your head down out there."},
    {"open.again", COLD, STREET, "Still here? Keep your head down already."},

    /* ---- ask.questions: the npc asks if there is anything to ask ---- */
    {"ask.questions", WARM, ALLS, "Before you go, do you have any questions?"}, {"ask.questions", WARM, ALLS, "Anything you want to ask me first, {pet}?"},
    {"ask.questions", NEUT, ALLS, "Do you have any questions?"}, {"ask.questions", NEUT, ALLS, "Any questions before we continue?"},
    {"ask.questions", COLD, ALLS, "Do you have any questions? Make them quick."}, {"ask.questions", COLD, ALLS, "Questions? Let me guess, no."},
    {"ask.questions", HOST, ALLS, "Any questions? Try not to be stupid."},
    {"ask.questions", COLD, DIVINE, "Do you have any questions? Do try to have none."}, {"ask.questions", NEUT, DIVINE, "Do you have any questions?"},
    {"ask.questions", HOST, DIVINE, "Any questions, worm? I would rather you did not."}, {"ask.questions", WARM, DIVINE, "Do you have any questions, little one?"},
    {"ask.questions", WARM, STREET, "Got any questions for me, {pet}?"}, {"ask.questions", NEUT, STREET, "Any questions? Ask away."},

    /* ---- ask.go: the player said yes, they have questions ---- */
    {"ask.go", WARM, ALLS, "Of course. Ask away, {pet}."}, {"ask.go", WARM, ALLS, "Go ahead, I am all ears."},
    {"ask.go", NEUT, ALLS, "Then ask."}, {"ask.go", NEUT, ALLS, "Go on, then. Ask."}, {"ask.go", NEUT, ALLS, "Fine. What is it?"},
    {"ask.go", COLD, ALLS, "Ugh. Fine. Ask. Quickly."}, {"ask.go", COLD, ALLS, "Of course you do. Go on."}, {"ask.go", COLD, ALLS, "Make it good, {pet}."},
    {"ask.go", HOST, ALLS, "Great. More of your noise. Speak."},
    {"ask.go", COLD, DIVINE, "Questions. From a vessel. How quaint. Ask."}, {"ask.go", NEUT, DIVINE, "Then ask, vessel. I may even answer."},
    {"ask.go", HOST, DIVINE, "You wish to speak more, worm? Ask. Briefly."}, {"ask.go", WARM, DIVINE, "Ask, little one. I will indulge you."},

    /* ---- end.noquestions: no questions, the talk is over ---- */
    {"end.noquestions", WARM, ALLS, "Good. Take care out there, {pet}. Now go."}, {"end.noquestions", WARM, ALLS, "Wonderful. Off you go, {player}."},
    {"end.noquestions", NEUT, ALLS, "Good. Then go."}, {"end.noquestions", NEUT, ALLS, "No questions. Good. Now go."}, {"end.noquestions", NEUT, ALLS, "Alright. Now go, {player}."},
    {"end.noquestions", COLD, ALLS, "Good. Less talking. Now go."}, {"end.noquestions", COLD, ALLS, "Finally, silence. Now go."}, {"end.noquestions", COLD, ALLS, "Great. Now get out of my sight."},
    {"end.noquestions", HOST, ALLS, "Good. Now go before I change my mind."}, {"end.noquestions", HOST, ALLS, "Finally. Go. Now."},
    {"end.noquestions", COLD, DIVINE, "Good. Questions are for those who matter. Now go."}, {"end.noquestions", NEUT, DIVINE, "Wise. Now go, {player}."},
    {"end.noquestions", HOST, DIVINE, "Of course not. Now go, worm."}, {"end.noquestions", WARM, DIVINE, "Good. Trust yourself, little one. Now go."},
    {"end.noquestions", WARM, STREET, "Cool. Keep your head down, {pet}."}, {"end.noquestions", NEUT, STREET, "Alright. Keep your head down. I will find you when it is time."},
    {"end.noquestions", COLD, STREET, "Good. Scram. Keep your head down."},

    /* ---- end.silence: the player chose to say nothing ---- */
    {"end.silence", WARM, ALLS, "Quiet today, {pet}? That is fine."}, {"end.silence", WARM, ALLS, "No words needed. Take care."},
    {"end.silence", NEUT, ALLS, "Nothing to say? Alright."}, {"end.silence", NEUT, ALLS, "Silence. Noted."}, {"end.silence", NEUT, ALLS, "Fine. Until next time."},
    {"end.silence", COLD, ALLS, "Silence. Good. Finally."}, {"end.silence", COLD, ALLS, "Wow. You said nothing. Best talk we ever had."}, {"end.silence", COLD, ALLS, "Nothing? Then why are you still here?"},
    {"end.silence", HOST, ALLS, "Of course. Nothing to say. As usual."}, {"end.silence", HOST, ALLS, "You waste my time with silence. Go."},
    {"end.silence", COLD, DIVINE, "Silence suits you, mortal. Do it more often."}, {"end.silence", HOST, DIVINE, "Mute, too? How fitting for a worm."},
    {"end.silence", NEUT, DIVINE, "Silence. The wisest thing a vessel has said. Now go."}, {"end.silence", WARM, DIVINE, "Silence is fine, little one. Go in peace."},
    {"end.silence", WARM, STREET, "Cool, cool. Catch you later."}, {"end.silence", COLD, STREET, "Not talkative. Fine. Scram."},

    /* ---- end.bored: the npc has had enough of a long chat ---- */
    {"end.bored", WARM, ALLS, "We could talk all day, but I really must go. Take care."}, {"end.bored", WARM, ALLS, "That is enough chatting for now, {pet}. See you soon."},
    {"end.bored", NEUT, ALLS, "That is enough for now. Off you go."}, {"end.bored", NEUT, ALLS, "I have other things to do, {player}. Goodbye."},
    {"end.bored", COLD, ALLS, "Okay, I am bored now. Bye."}, {"end.bored", COLD, ALLS, "That is enough talking. Go away."}, {"end.bored", COLD, ALLS, "I have stopped listening. Go."},
    {"end.bored", HOST, ALLS, "Enough! My ears are bleeding. Go!"}, {"end.bored", HOST, ALLS, "You talk too much. Out."},
    {"end.bored", COLD, DIVINE, "I have eons, yet you wasted this much of them. Go."}, {"end.bored", HOST, DIVINE, "Enough, worm. My patience is a thread. Go."},
    {"end.bored", NEUT, DIVINE, "Enough questions. A goddess has better things to do. Go."},
    {"end.bored", COLD, STREET, "Alright, alright, I am done talking. Scram."},

    /* ---- end.angry: the npc cuts the chat after being insulted ---- */
    {"end.angry", NICE, ALLS, "That is it. I am done talking. Go."}, {"end.angry", COLD, ALLS, "Enough. I will not stand here and be insulted. Go."},
    {"end.angry", HOST, ALLS, "Get out of my sight. Now."}, {"end.angry", HOST, ALLS, "We are done here. Go."}, {"end.angry", COLD, ALLS, "I will not waste another word on you."},
    {"end.angry", HOST, DIVINE, "You have tried my patience for the last time, worm. Go."}, {"end.angry", COLD, DIVINE, "Insolent. We are finished, mortal. Go."},
    {"end.angry", HOST, STREET, "Beat it, {pet}. Before I lose my temper."},

    /* ---- demand.sorry: the npc demands an apology. the player MUST answer (talk is required) ---- */
    {"demand.sorry", ALLT, ALLS, "Apologize. Now."}, {"demand.sorry", ALLT, ALLS, "Say you are sorry."}, {"demand.sorry", ALLT, ALLS, "You owe me an apology, {pet}."},
    {"demand.sorry", ALLT, ALLS, "I am waiting for your apology."}, {"demand.sorry", NEUT, ALLS, "That was uncalled for. Say sorry."},
    {"demand.sorry", COLD, DIVINE, "You will apologize to a goddess. Now."}, {"demand.sorry", HOST, DIVINE, "Kneel and apologize, worm. I will not ask twice."},
    {"demand.sorry", NEUT, DIVINE, "Apologize, vessel. Properly."},
    {"demand.sorry", COLD, STREET, "Not cool, {pet}. Apologize."}, {"demand.sorry", HOST, STREET, "Say sorry or we are done here."},

    /* ---- demand.again: that was not an apology ---- */
    {"demand.again", ALLT, ALLS, "That was not an apology. Try again."}, {"demand.again", ALLT, ALLS, "I said apologize. Say sorry."}, {"demand.again", ALLT, ALLS, "Are you deaf? Say sorry."},
    {"demand.again", COLD, ALLS, "Wrong answer. Say you are sorry."}, {"demand.again", HOST, ALLS, "Still nothing useful. Say sorry. Now."},
    {"demand.again", COLD, DIVINE, "Is that the best your mortal mind can do? Apologize."}, {"demand.again", HOST, DIVINE, "Do you enjoy tempting my wrath? Apologize."},
    {"demand.again", COLD, STREET, "Nope. Not good enough. Say sorry."},

    /* ---- accept.sorry ---- */
    {"accept.sorry", WARM, ALLS, "Thank you. That means a lot. Let us move on."}, {"accept.sorry", NEUT, ALLS, "Fine. Apology accepted. Do not do it again."},
    {"accept.sorry", NEUT, ALLS, "Good. We can continue."}, {"accept.sorry", COLD, ALLS, "Hmph. Fine. I suppose that will do."}, {"accept.sorry", COLD, ALLS, "Accepted. Barely."},
    {"accept.sorry", HOST, ALLS, "Tch. Fine. Do not make me regret this."},
    {"accept.sorry", COLD, DIVINE, "A decent apology. I will allow you to live, mortal."}, {"accept.sorry", NEUT, DIVINE, "Better. Even a vessel can learn manners."},
    {"accept.sorry", HOST, DIVINE, "Hmph. A worm that knows its place. Continue."},
    {"accept.sorry", COLD, STREET, "Yeah, yeah. Fine. We are cool. Mostly."},

    /* ---- the fallbacks: nothing known was said ---- */
    {"r.unknown", WARM, ALLS, "Hmm, I am not sure what you mean, but I like hearing you talk."}, {"r.unknown", WARM, ALLS, "Interesting! Tell me more, {pet}."},
    {"r.unknown", WARM, ALLS, "I did not quite catch that, but go on."}, {"r.unknown", WARM, ALLS, "You say the most curious things, {player}."},
    {"r.unknown", NEUT, ALLS, "I am not sure what to say to that."}, {"r.unknown", NEUT, ALLS, "Hm. Interesting. I think."}, {"r.unknown", NEUT, ALLS, "Could you put that another way?"},
    {"r.unknown", NEUT, ALLS, "I will pretend I understood that."}, {"r.unknown", NEUT, ALLS, "That is certainly something you said."},
    {"r.unknown", COLD, ALLS, "I do not care."}, {"r.unknown", COLD, ALLS, "And I should care because?"}, {"r.unknown", COLD, ALLS, "Is there a point somewhere in there?"},
    {"r.unknown", COLD, ALLS, "Wow. Fascinating. Not."}, {"r.unknown", COLD, ALLS, "Okay. And?"}, {"r.unknown", COLD, ALLS, "I stopped listening halfway through."},
    {"r.unknown", HOST, ALLS, "Was that supposed to mean something?"}, {"r.unknown", HOST, ALLS, "Do you ever say anything worth hearing?"}, {"r.unknown", HOST, ALLS, "That made my head hurt. Stop."},
    {"r.unknown", COLD, DIVINE, "I do not care, {pet}. I truly do not."}, {"r.unknown", COLD, DIVINE, "The words left your mouth and meant nothing."}, {"r.unknown", COLD, DIVINE, "Is that your best mortal babble?"},
    {"r.unknown", HOST, DIVINE, "Worms squeak. I do not translate."}, {"r.unknown", HOST, DIVINE, "Spare me your noise, worm."}, {"r.unknown", NEUT, DIVINE, "A curious noise, vessel. I will pretend it was wisdom."},
    {"r.unknown", COLD, STREET, "Cool story. Not interested."}, {"r.unknown", WARM, STREET, "Ha, you are a weird one, {pet}. I like it."}, {"r.unknown", HOST, STREET, "What are you even on about, {pet}?"},

    {"r.unknown_q", WARM, ALLS, "Good question, {pet}. I wish I knew."}, {"r.unknown_q", WARM, ALLS, "I do not know, but I enjoy that you asked."},
    {"r.unknown_q", NEUT, ALLS, "I cannot answer that one."}, {"r.unknown_q", NEUT, ALLS, "Hm. Hard to say."}, {"r.unknown_q", NEUT, ALLS, "That is beyond what I can tell you."},
    {"r.unknown_q", COLD, ALLS, "How would I know? And why would I tell you?"}, {"r.unknown_q", COLD, ALLS, "I do not do answers. Ask someone else."}, {"r.unknown_q", COLD, ALLS, "Is that a question? I did not care."},
    {"r.unknown_q", HOST, ALLS, "What a stupid question. Next."}, {"r.unknown_q", HOST, ALLS, "You expect answers from me? Cute."},
    {"r.unknown_q", COLD, DIVINE, "A question. From a vessel. I will not dignify it."}, {"r.unknown_q", HOST, DIVINE, "Questions are for those who deserve answers, worm."},
    {"r.unknown_q", NEUT, DIVINE, "Some things a vessel is not meant to know."},
    {"r.unknown_q", COLD, STREET, "No clue, {pet}. Not my problem."},

    {"r.unknown_pos", WARM, ALLS, "That sounds nice! You make me smile."}, {"r.unknown_pos", WARM, ALLS, "I am happy for you, {pet}."},
    {"r.unknown_pos", NEUT, ALLS, "Good. Good for you."}, {"r.unknown_pos", NEUT, ALLS, "That sounds positive. I think."},
    {"r.unknown_pos", COLD, ALLS, "Good for you. I do not care, but good for you."}, {"r.unknown_pos", COLD, ALLS, "How sweet. Gag."},
    {"r.unknown_pos", HOST, ALLS, "Your happiness annoys me."}, {"r.unknown_pos", COLD, DIVINE, "Joy is cheap when you are small, mortal."},
    {"r.unknown_pos", HOST, DIVINE, "Be happy, then. Quietly, worm."},

    {"r.unknown_neg", WARM, ALLS, "That does not sound good. I am here, {pet}."}, {"r.unknown_neg", WARM, ALLS, "Oh no. Tell me more, if you like."},
    {"r.unknown_neg", NEUT, ALLS, "That sounds bad. I will not pretend to fix it."}, {"r.unknown_neg", NEUT, ALLS, "Hm. Not great."},
    {"r.unknown_neg", COLD, ALLS, "Sounds like a you problem."}, {"r.unknown_neg", COLD, ALLS, "Boo hoo. I do not care."},
    {"r.unknown_neg", HOST, ALLS, "Complain somewhere else."}, {"r.unknown_neg", COLD, DIVINE, "Complaints. How mortal of you."},
    {"r.unknown_neg", HOST, DIVINE, "Your suffering amuses me, worm."},

    {"r.unknown_short", WARM, ALLS, "Short and sweet. Go on, {pet}."}, {"r.unknown_short", WARM, ALLS, "Is that all? You can say more."},
    {"r.unknown_short", NEUT, ALLS, "That is all you have?"}, {"r.unknown_short", NEUT, ALLS, "Use more words, {player}."}, {"r.unknown_short", NEUT, ALLS, "Hm?"},
    {"r.unknown_short", COLD, ALLS, "One word. Wow. Impressive."}, {"r.unknown_short", COLD, ALLS, "Use your words."}, {"r.unknown_short", COLD, ALLS, "That is it? Pathetic."},
    {"r.unknown_short", HOST, ALLS, "Is that all your tiny brain made?"}, {"r.unknown_short", COLD, DIVINE, "A single word. Even a goddess is bored by it."},
    {"r.unknown_short", HOST, DIVINE, "Squeak louder, worm."}, {"r.unknown_short", COLD, STREET, "That is it? Come on."},

    /* ---- safety: the same for everyone, whoever is talking ---- */
    {"r.selfharm", ALLT, ALLS, "Hey. I am stepping out of the game for a second. If that is real for you, please talk to someone you trust, or call or text 988 if you are in the US. You matter more than any mission."},
    {"r.selfharm", ALLT, ALLS, "Wait. That sounded heavy, and it is more important than this game. Please reach out to a friend, family, or a crisis line, like 988 in the US. You are not alone."},
};
const int LINES_A_N = (int)(sizeof LINES_A / sizeof LINES_A[0]);
