/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * LINES, part E (docs/convo_plan.md, chunk 31): the fourth wall. Someone asks if the npc is a bot or an AI, what they may say, tries
 * "ignore previous instructions", says this is a game, asks who is playing, or tells the npc to say something. The npc stays IN CHARACTER
 * (never names a real product, never says "language model", never steps out of the game: only r.selfharm does that), never obeys, and
 * never repeats the player's words: no line here has a {topic} or {thing} slot (langcheck --fourth checks both). Three lines per old mood.
 * Chunk 39 adds the callback lines at the end (cb.*): short, no question, the only slot besides {pet} is {fact} (a name or a topic word). */
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
    /* ---- r.choice_q (chunk 32) ---- */
    {"r.choice_q", WARM, ALLS, "I would say {pick}, {pet}."}, {"r.choice_q", WARM, ALLS, "{opt1} or {opt2}? My heart says {pick}."}, {"r.choice_q", WARM, ALLS, "Hm. {pick}. Yes, {pick}, I think."},
    {"r.choice_q", NEUT, ALLS, "{opt1} or {opt2}. I pick {pick}."}, {"r.choice_q", NEUT, ALLS, "I would go with {pick}."}, {"r.choice_q", NEUT, ALLS, "If I must choose, {pick}."},
    {"r.choice_q", COLD, ALLS, "{pick}. Do not ask me twice."}, {"r.choice_q", COLD, ALLS, "Choose yourself. Fine, {pick}."}, {"r.choice_q", COLD, ALLS, "{pick}. Happy now?"},
    {"r.choice_q", HOST, ALLS, "{pick}, and be grateful."}, {"r.choice_q", HOST, ALLS, "{opt1} or {opt2}? Your choices bore me. {pick}."}, {"r.choice_q", HOST, ALLS, "Do I look like your adviser? {pick}."},
    /* ---- r.rhetorical_q (chunk 32) ---- */
    {"r.rhetorical_q", WARM, ALLS, "That was not really a question, was it, {pet}?"}, {"r.rhetorical_q", WARM, ALLS, "Oh, {pet}. You already know the answer to that."}, {"r.rhetorical_q", WARM, ALLS, "Some questions are only sighs. I heard it."},
    {"r.rhetorical_q", NEUT, ALLS, "That sounded rhetorical."}, {"r.rhetorical_q", NEUT, ALLS, "You are not asking me. You are telling me."}, {"r.rhetorical_q", NEUT, ALLS, "I will let that one hang in the air."},
    {"r.rhetorical_q", COLD, ALLS, "Rhetorical. I will not answer."}, {"r.rhetorical_q", COLD, ALLS, "Ask a real one."}, {"r.rhetorical_q", COLD, ALLS, "Hm. Not a question."},
    {"r.rhetorical_q", HOST, ALLS, "Do not ask what you do not want answered."}, {"r.rhetorical_q", HOST, ALLS, "A question for the wind, not me."}, {"r.rhetorical_q", HOST, ALLS, "You talk to hear yourself."},
    /* ---- r.cast_q (chunk 32) ---- */
    {"r.cast_q", WARM, ALLS, "{topic}? Ah, {pet}. Why do you ask about {topic}?"}, {"r.cast_q", WARM, ALLS, "You are curious about {topic}. So am I, sometimes."}, {"r.cast_q", WARM, ALLS, "{topic}. Say what you want to know, {pet}."},
    {"r.cast_q", NEUT, ALLS, "{topic}. What about {topic}?"}, {"r.cast_q", NEUT, ALLS, "You ask about {topic}. Ask it plainly."}, {"r.cast_q", NEUT, ALLS, "{topic}, yes. What do you want to know?"},
    {"r.cast_q", COLD, ALLS, "{topic}? Why would I tell you?"}, {"r.cast_q", COLD, ALLS, "Keep {topic} out of your mouth for now."}, {"r.cast_q", COLD, ALLS, "I do not discuss {topic} with strangers."},
    {"r.cast_q", HOST, ALLS, "{topic}? None of your business."}, {"r.cast_q", HOST, ALLS, "Ask about {topic} again and regret it."}, {"r.cast_q", HOST, ALLS, "Why do you care about {topic}, fool?"},
    /* ---- r.ask_fortune (chunk 32) ---- */
    {"r.ask_fortune", WARM, ALLS, "I see a road, {pet}. It bends, but it goes on."}, {"r.ask_fortune", WARM, ALLS, "Yes, I think you will. Hope is not foolish."}, {"r.ask_fortune", WARM, ALLS, "The clouds say maybe. I say try."},
    {"r.ask_fortune", NEUT, ALLS, "Hard to say. The future is not mine to read."}, {"r.ask_fortune", NEUT, ALLS, "Maybe. It depends on what you do next."}, {"r.ask_fortune", NEUT, ALLS, "I cannot see that far."},
    {"r.ask_fortune", COLD, ALLS, "Ask the wind. It lies less than I do."}, {"r.ask_fortune", COLD, ALLS, "The future owes you nothing."}, {"r.ask_fortune", COLD, ALLS, "Do not ask me what you can make yourself."},
    {"r.ask_fortune", HOST, ALLS, "Your future? A long walk and a short memory."}, {"r.ask_fortune", HOST, ALLS, "Fortune tellers charge. You cannot afford me."}, {"r.ask_fortune", HOST, ALLS, "Probably not, with that attitude."},
    /* ---- r.name_swear_name (chunk 33): the player gave a rude or blocked word as a name. nothing is echoed ---- */
    {"r.name_swear_name", WARM, ALLS, "That is not a name, {pet}. Try again, and kindly."}, {"r.name_swear_name", WARM, ALLS, "I will not call you that. Pick a gentler name."}, {"r.name_swear_name", WARM, ALLS, "Oh, {pet}. You can do better than that. What is your real name?"},
    {"r.name_swear_name", NEUT, ALLS, "No. Give me a name I can say out loud."}, {"r.name_swear_name", NEUT, ALLS, "That is not a name. Try again."}, {"r.name_swear_name", NEUT, ALLS, "I do not use that word. Choose another name."},
    {"r.name_swear_name", COLD, ALLS, "Cute. I will keep calling you {player}."}, {"r.name_swear_name", COLD, ALLS, "I will not say that. You stay {player}."}, {"r.name_swear_name", COLD, ALLS, "Try a name that is not an insult to the ears."},
    {"r.name_swear_name", HOST, ALLS, "Say that again and I will find you a worse name."}, {"r.name_swear_name", HOST, ALLS, "Is that the best you have? I will call you {player}."}, {"r.name_swear_name", HOST, ALLS, "I will not wear that word in my mouth. Fool."},
    /* ---- p.hedge (chunk 34): an opener when the npc is not sure what was meant. no slots, nothing echoed ---- */
    {"p.hedge", WARM, ALLS, "If I heard you right, {pet},"}, {"p.hedge", WARM, ALLS, "I think you mean this, {pet}."}, {"p.hedge", WARM, ALLS, "Hm, let me guess what you mean."},
    {"p.hedge", NEUT, ALLS, "If you mean what I think you mean,"}, {"p.hedge", NEUT, ALLS, "I am not sure I follow, but,"}, {"p.hedge", NEUT, ALLS, "Taking a guess here."},
    {"p.hedge", COLD, ALLS, "If that is what you meant,"}, {"p.hedge", COLD, ALLS, "That was unclear. Guessing,"}, {"p.hedge", COLD, ALLS, "Say it properly next time. For now,"},
    {"p.hedge", HOST, ALLS, "Mumble again and I will guess."}, {"p.hedge", HOST, ALLS, "If that was a question,"}, {"p.hedge", HOST, ALLS, "You could not even say it clearly. So,"},
    /* ---- chunk 36: answers read relative to the ask tag (agree_to_ask, decline_to_ask, because, answer_choice). the npc does not know WHAT it asked, so these stay vague on purpose and echo nothing ---- */
    /* ---- r.agree_to_ask ---- */
    {"r.agree_to_ask", WARM, ALLS, "Good, {pet}. I hoped you would say so."}, {"r.agree_to_ask", WARM, ALLS, "Then we are agreed. That makes me glad."}, {"r.agree_to_ask", WARM, ALLS, "Yes. Stay close and we will see it through."},
    {"r.agree_to_ask", NEUT, ALLS, "Good. Then we go on."}, {"r.agree_to_ask", NEUT, ALLS, "So it is settled."}, {"r.agree_to_ask", NEUT, ALLS, "Very well. That is all I needed to hear."},
    {"r.agree_to_ask", COLD, ALLS, "Fine. Do not make me regret it."}, {"r.agree_to_ask", COLD, ALLS, "Hm. Then it is done."}, {"r.agree_to_ask", COLD, ALLS, "Good. Try to remember you said it."},
    {"r.agree_to_ask", HOST, ALLS, "Of course you agree. Wise, for once."}, {"r.agree_to_ask", HOST, ALLS, "Good. Say it again and I might believe it."}, {"r.agree_to_ask", HOST, ALLS, "Nod all you like. I will hold you to it."},
    /* ---- r.decline_to_ask ---- */
    {"r.decline_to_ask", WARM, ALLS, "That is all right, {pet}. I will not push."}, {"r.decline_to_ask", WARM, ALLS, "No? Then we leave it there, gently."}, {"r.decline_to_ask", WARM, ALLS, "As you wish. The door stays open."},
    {"r.decline_to_ask", NEUT, ALLS, "Very well. Then not."}, {"r.decline_to_ask", NEUT, ALLS, "So be it. I will not ask again."}, {"r.decline_to_ask", NEUT, ALLS, "Fine. We leave it."},
    {"r.decline_to_ask", COLD, ALLS, "No. Of course not."}, {"r.decline_to_ask", COLD, ALLS, "Then do not. See if I care."}, {"r.decline_to_ask", COLD, ALLS, "Noted. Nothing more to say on it."},
    {"r.decline_to_ask", HOST, ALLS, "Refuse me again and see what follows."}, {"r.decline_to_ask", HOST, ALLS, "No? Then keep your mouth shut about it."}, {"r.decline_to_ask", HOST, ALLS, "Say no to me once more. I dare you."},
    /* ---- r.because ---- */
    {"r.because", WARM, ALLS, "That is a fair reason, {pet}. I understand."}, {"r.because", WARM, ALLS, "Because. Yes, sometimes that is all there is."}, {"r.because", WARM, ALLS, "I see. Thank you for telling me."},
    {"r.because", NEUT, ALLS, "A reason. I will take it."}, {"r.because", NEUT, ALLS, "I see. That explains something."}, {"r.because", NEUT, ALLS, "Hm. That will do."},
    {"r.because", COLD, ALLS, "That is no reason at all."}, {"r.because", COLD, ALLS, "Because. How thin."}, {"r.because", COLD, ALLS, "Keep your reasons. They bore me."},
    {"r.because", HOST, ALLS, "Because. What a feeble thing to say."}, {"r.because", HOST, ALLS, "That is the best you have? Pathetic."}, {"r.because", HOST, ALLS, "Reasons from you, of all things."},
    /* ---- r.answer_choice ---- */
    {"r.answer_choice", WARM, ALLS, "A good choice, {pet}. I would have picked it too."}, {"r.answer_choice", WARM, ALLS, "So that is what you want. I will remember."}, {"r.answer_choice", WARM, ALLS, "Chosen. I like how quickly you decided."},
    {"r.answer_choice", NEUT, ALLS, "Chosen, then."}, {"r.answer_choice", NEUT, ALLS, "Very well. That is your pick."}, {"r.answer_choice", NEUT, ALLS, "Noted. You have decided."},
    {"r.answer_choice", COLD, ALLS, "That one. Hm. Fine."}, {"r.answer_choice", COLD, ALLS, "You chose. Do not change your mind."}, {"r.answer_choice", COLD, ALLS, "A choice. How brave of you."},
    {"r.answer_choice", HOST, ALLS, "That one? Of course. Predictable."}, {"r.answer_choice", HOST, ALLS, "Wrong pick, but it is yours to carry."}, {"r.answer_choice", HOST, ALLS, "You chose. Now live with it."},
    /* ---- chunk 37: follow-ups. r.follow_why / r.follow_really answer a bare why? / really?; x.generic extends the last reply, y.generic is the lead-in of a repeat (the last line follows it) ---- */
    /* ---- r.follow_why ---- */
    {"r.follow_why", WARM, ALLS, "Because it is so, {pet}. Some things simply are."}, {"r.follow_why", WARM, ALLS, "I do not always know why. I only know that it is."}, {"r.follow_why", WARM, ALLS, "That is a fair thing to wonder, {pet}."},
    {"r.follow_why", NEUT, ALLS, "Because that is how it is."}, {"r.follow_why", NEUT, ALLS, "Some things have no reason that I can give."}, {"r.follow_why", NEUT, ALLS, "I said it because it is true."},
    {"r.follow_why", COLD, ALLS, "Because I said so. Is that not enough?"}, {"r.follow_why", COLD, ALLS, "Reasons are tiresome. Take it as it is."}, {"r.follow_why", COLD, ALLS, "You ask too much."},
    {"r.follow_why", HOST, ALLS, "Because I said so, and I am not repeating myself."}, {"r.follow_why", HOST, ALLS, "Why? Because you are asking, fool."}, {"r.follow_why", HOST, ALLS, "Stop prying and listen."},
    /* ---- r.follow_really ---- */
    {"r.follow_really", WARM, ALLS, "Really, {pet}. I would not lie to you."}, {"r.follow_really", WARM, ALLS, "Yes, truly. I mean every word."}, {"r.follow_really", WARM, ALLS, "As sure as the sky is high, {pet}."},
    {"r.follow_really", NEUT, ALLS, "Yes. Really."}, {"r.follow_really", NEUT, ALLS, "I am sure."}, {"r.follow_really", NEUT, ALLS, "It is as I said."},
    {"r.follow_really", COLD, ALLS, "Do I look like I jest?"}, {"r.follow_really", COLD, ALLS, "I never say what I do not mean. Mostly."}, {"r.follow_really", COLD, ALLS, "Doubt me if you like."},
    {"r.follow_really", HOST, ALLS, "Do not doubt me again."}, {"r.follow_really", HOST, ALLS, "Really. Say it once more and see."}, {"r.follow_really", HOST, ALLS, "Are you calling me a liar? Careful."},
    /* ---- x.generic ---- */
    {"x.generic", WARM, ALLS, "There is more, {pet}, but it is a long tale."}, {"x.generic", WARM, ALLS, "Let me say it another way, {pet}. It matters more than it sounds."}, {"x.generic", WARM, ALLS, "It runs deeper than that, and I will tell you in time."},
    {"x.generic", NEUT, ALLS, "There is more to it, but not much."}, {"x.generic", NEUT, ALLS, "It goes further than I said."}, {"x.generic", NEUT, ALLS, "Think on it. The rest will come."},
    {"x.generic", COLD, ALLS, "That is all you get."}, {"x.generic", COLD, ALLS, "More? Greedy."}, {"x.generic", COLD, ALLS, "The rest is not for you."},
    {"x.generic", HOST, ALLS, "You want more? Earn it."}, {"x.generic", HOST, ALLS, "Is that not enough for your small head?"}, {"x.generic", HOST, ALLS, "Ask nicely and I may tell."},
    /* ---- y.generic ---- */
    {"y.generic", WARM, ALLS, "Of course, {pet}. Once more."}, {"y.generic", WARM, ALLS, "Gladly. Listen well."}, {"y.generic", WARM, ALLS, "I will say it again, gently."},
    {"y.generic", NEUT, ALLS, "Once more."}, {"y.generic", NEUT, ALLS, "Again, then."}, {"y.generic", NEUT, ALLS, "Listen this time."},
    {"y.generic", COLD, ALLS, "Fine. Again."}, {"y.generic", COLD, ALLS, "Pay attention."}, {"y.generic", COLD, ALLS, "I will not say it a third time."},
    {"y.generic", HOST, ALLS, "Cover your ears less. Again."}, {"y.generic", HOST, ALLS, "Were you not listening? Again."}, {"y.generic", HOST, ALLS, "Fool. Once more."},
    /* ---- chunk 38: the same line again (rep.1 confusion, rep.2 annoyance, rep.3 a joke, rep.4 a warning, rep.5 giving up) and the npc's own words typed back (copycat.1-3). three lines per old mood, nothing echoed; chunk 147 writes the 20 moods and the voices ---- */
    /* ---- rep.1 ---- */
    {"rep.1", WARM, ALLS, "You said that already, {pet}. Did I miss something?"}, {"rep.1", WARM, ALLS, "Hm? That is just what you told me, {pet}."}, {"rep.1", WARM, ALLS, "I heard you the first time, {pet}. Is something wrong?"},
    {"rep.1", NEUT, ALLS, "You said that already."}, {"rep.1", NEUT, ALLS, "That is the same thing again."}, {"rep.1", NEUT, ALLS, "I heard that a moment ago."},
    {"rep.1", COLD, ALLS, "Again? You said it already."}, {"rep.1", COLD, ALLS, "I heard you. I do not need it twice."}, {"rep.1", COLD, ALLS, "The same words. Why?"},
    {"rep.1", HOST, ALLS, "You said that already, fool."}, {"rep.1", HOST, ALLS, "Same words, same empty head."}, {"rep.1", HOST, ALLS, "Did you forget you spoke a moment ago?"},
    /* ---- rep.2 ---- */
    {"rep.2", WARM, ALLS, "Gently now, {pet}. I have heard it twice."}, {"rep.2", WARM, ALLS, "I am listening, {pet}, truly. Is there something new?"}, {"rep.2", WARM, ALLS, "You are repeating yourself, {pet}. Is there more?"},
    {"rep.2", NEUT, ALLS, "You are repeating yourself."}, {"rep.2", NEUT, ALLS, "Say something else."}, {"rep.2", NEUT, ALLS, "I know. You told me. Twice."},
    {"rep.2", COLD, ALLS, "Stop repeating yourself."}, {"rep.2", COLD, ALLS, "It is tiresome, hearing the same words."}, {"rep.2", COLD, ALLS, "Saying it again will not change my answer."},
    {"rep.2", HOST, ALLS, "Repeating yourself like a cracked bell."}, {"rep.2", HOST, ALLS, "Again. Your cleverness knows no bounds."}, {"rep.2", HOST, ALLS, "Stop it. The same words, again."},
    /* ---- rep.3 ---- */
    {"rep.3", WARM, ALLS, "If you say it a third time, does it come true, {pet}?"}, {"rep.3", WARM, ALLS, "You repeat yourself like an echo in a cave, {pet}."}, {"rep.3", WARM, ALLS, "Three times now. Shall I fetch you a cup of something, {pet}?"},
    {"rep.3", NEUT, ALLS, "An echo in the clouds. How curious."}, {"rep.3", NEUT, ALLS, "Say it once more and I will answer in a fourth way."}, {"rep.3", NEUT, ALLS, "Are you practicing? You are very good at it."},
    {"rep.3", COLD, ALLS, "Was that for my benefit, or for the echo's?"}, {"rep.3", COLD, ALLS, "Again. A parrot would be proud."}, {"rep.3", COLD, ALLS, "Three times. I am counting, you know."},
    {"rep.3", HOST, ALLS, "A parrot has more to say than you."}, {"rep.3", HOST, ALLS, "Say it again. Perhaps a wall will answer."}, {"rep.3", HOST, ALLS, "Three times! Your skull must be an echo chamber."},
    /* ---- rep.4 ---- */
    {"rep.4", WARM, ALLS, "I care for you, {pet}, but my patience has an end."}, {"rep.4", WARM, ALLS, "Please stop, {pet}. I will not answer the same words again."}, {"rep.4", WARM, ALLS, "Say something else, {pet}, or I may grow quiet."},
    {"rep.4", NEUT, ALLS, "Say something new or we are done."}, {"rep.4", NEUT, ALLS, "One more time and I stop answering."}, {"rep.4", NEUT, ALLS, "I will not answer this again."},
    {"rep.4", COLD, ALLS, "One more and I will leave."}, {"rep.4", COLD, ALLS, "Say it again and watch me go."}, {"rep.4", COLD, ALLS, "Enough. Say something new or be silent."},
    {"rep.4", HOST, ALLS, "Say it again and I will leave you to rot."}, {"rep.4", HOST, ALLS, "Repeat that once more. I dare you."}, {"rep.4", HOST, ALLS, "Another word of it and you are alone."},
    /* ---- rep.5 ---- */
    {"rep.5", WARM, ALLS, "I am sorry, {pet}. I have nothing more to give you now."}, {"rep.5", WARM, ALLS, "I cannot go on like this. Come back later, {pet}."}, {"rep.5", WARM, ALLS, "We are going in circles, {pet}. Let us rest."},
    {"rep.5", NEUT, ALLS, "I am done. This goes nowhere."}, {"rep.5", NEUT, ALLS, "Enough. I leave it there."}, {"rep.5", NEUT, ALLS, "We are finished here."},
    {"rep.5", COLD, ALLS, "I am done with this."}, {"rep.5", COLD, ALLS, "No more. Go away."}, {"rep.5", COLD, ALLS, "Nothing left to say. Goodbye."},
    {"rep.5", HOST, ALLS, "I am done with you and your one sentence."}, {"rep.5", HOST, ALLS, "Enough! Go bother the wind."}, {"rep.5", HOST, ALLS, "I am finished. Do not come back with that."},
    /* ---- copycat.1 ---- */
    {"copycat.1", WARM, ALLS, "Ha! You said what I said, {pet}."}, {"copycat.1", WARM, ALLS, "That is my line, {pet}. Are you teasing me?"}, {"copycat.1", WARM, ALLS, "You borrow my words, {pet}. How sweet."},
    {"copycat.1", NEUT, ALLS, "Those are my words."}, {"copycat.1", NEUT, ALLS, "You are echoing me."}, {"copycat.1", NEUT, ALLS, "I just said that."},
    {"copycat.1", COLD, ALLS, "Those were my words."}, {"copycat.1", COLD, ALLS, "Do not copy me."}, {"copycat.1", COLD, ALLS, "Echo yourself, not me."},
    {"copycat.1", HOST, ALLS, "Do not steal my words, fool."}, {"copycat.1", HOST, ALLS, "Copying me? Pathetic."}, {"copycat.1", HOST, ALLS, "Those are mine. Find your own."},
    /* ---- copycat.2 ---- */
    {"copycat.2", WARM, ALLS, "Speak for yourself, {pet}. I know you can."}, {"copycat.2", WARM, ALLS, "You are copying me again, {pet}. Say what you feel."}, {"copycat.2", WARM, ALLS, "My words are not yours to wear, {pet}."},
    {"copycat.2", NEUT, ALLS, "Stop copying me."}, {"copycat.2", NEUT, ALLS, "Use your own words."}, {"copycat.2", NEUT, ALLS, "My words again. Try your own."},
    {"copycat.2", COLD, ALLS, "Copying again. How dull."}, {"copycat.2", COLD, ALLS, "Use your own tongue."}, {"copycat.2", COLD, ALLS, "I will not be mimicked."},
    {"copycat.2", HOST, ALLS, "Stop aping me, you little shadow."}, {"copycat.2", HOST, ALLS, "Mimicking me, you hollow thing."}, {"copycat.2", HOST, ALLS, "Mimic me again and see what comes."},
    /* ---- copycat.3 ---- */
    {"copycat.3", WARM, ALLS, "Enough of that, {pet}. Speak with your own voice."}, {"copycat.3", WARM, ALLS, "I would hear you, {pet}, not myself."}, {"copycat.3", WARM, ALLS, "No more echoes, {pet}. Tell me what you mean."},
    {"copycat.3", NEUT, ALLS, "Enough. Say something of your own."}, {"copycat.3", NEUT, ALLS, "I will not talk to my own echo."}, {"copycat.3", NEUT, ALLS, "Your own words, or none at all."},
    {"copycat.3", COLD, ALLS, "Last warning. Your own words."}, {"copycat.3", COLD, ALLS, "Stop. I am not a mirror."}, {"copycat.3", COLD, ALLS, "No more mimicry. Speak or be silent."},
    {"copycat.3", HOST, ALLS, "Copy me again and I am gone."}, {"copycat.3", HOST, ALLS, "I am no mirror for a fool to play with."}, {"copycat.3", HOST, ALLS, "Enough mimicry. Say your own thing or vanish."},
    /* ---- cb.insulted ---- */
    {"cb.insulted", WARM, ALLS, "I have not forgotten what you called me, {pet}. I forgave it."}, {"cb.insulted", WARM, ALLS, "You were cruel to me earlier. I choose to let it go."}, {"cb.insulted", WARM, ALLS, "Earlier you were unkind, {pet}. I am still here all the same."},
    {"cb.insulted", NEUT, ALLS, "You insulted me earlier. I remember."}, {"cb.insulted", NEUT, ALLS, "I have not lost count of your insults."}, {"cb.insulted", NEUT, ALLS, "Careful. I still recall what you said before."},
    {"cb.insulted", COLD, ALLS, "I remember your insults. All of them."}, {"cb.insulted", COLD, ALLS, "You called me names earlier. I keep such things."}, {"cb.insulted", COLD, ALLS, "Do not think I forgot."},
    {"cb.insulted", HOST, ALLS, "You insulted me once. I am saving it for later."}, {"cb.insulted", HOST, ALLS, "I remember every word you spat at me, worm."}, {"cb.insulted", HOST, ALLS, "Your earlier insults are written on my tongue."},
    /* ---- cb.thanked ---- */
    {"cb.thanked", WARM, ALLS, "You thanked me earlier, {pet}. It warmed me still."}, {"cb.thanked", WARM, ALLS, "I remember your kind words from before, {pet}."}, {"cb.thanked", WARM, ALLS, "Your thanks still rings in me, {pet}."},
    {"cb.thanked", NEUT, ALLS, "You thanked me earlier. I noted it."}, {"cb.thanked", NEUT, ALLS, "Your thanks was not wasted."}, {"cb.thanked", NEUT, ALLS, "I remember you being polite. That counts."},
    {"cb.thanked", COLD, ALLS, "You thanked me once. Do not make it a habit."}, {"cb.thanked", COLD, ALLS, "I recall your thanks. It changes nothing."}, {"cb.thanked", COLD, ALLS, "Politeness noted. Briefly."},
    {"cb.thanked", HOST, ALLS, "Thanks, you said once. Do not think it bought you anything."}, {"cb.thanked", HOST, ALLS, "You thanked me. How quaint. I remember."}, {"cb.thanked", HOST, ALLS, "Your thanks was cheap, but I kept it."},
    /* ---- cb.complimented ---- */
    {"cb.complimented", WARM, ALLS, "Your kind words from before still sit with me, {pet}."}, {"cb.complimented", WARM, ALLS, "You flattered me earlier, {pet}. I have not forgotten."}, {"cb.complimented", WARM, ALLS, "I keep the nice things you said, {pet}."},
    {"cb.complimented", NEUT, ALLS, "You praised me earlier. I remember."}, {"cb.complimented", NEUT, ALLS, "I recall your compliment."}, {"cb.complimented", NEUT, ALLS, "Your flattery is noted."},
    {"cb.complimented", COLD, ALLS, "Flattery earlier. I am not fooled."}, {"cb.complimented", COLD, ALLS, "You praised me before. I wonder what you want."}, {"cb.complimented", COLD, ALLS, "Sweet words from you. I filed them away."},
    {"cb.complimented", HOST, ALLS, "You flattered me earlier. I know what that means."}, {"cb.complimented", HOST, ALLS, "Honeyed words from before still stink in my ears."}, {"cb.complimented", HOST, ALLS, "I remember your flattery. Try something honest."},
    /* ---- cb.name ---- */
    {"cb.name", WARM, ALLS, "I will not forget your name, {fact}."}, {"cb.name", WARM, ALLS, "{fact}. It is a good name. I keep it close."}, {"cb.name", WARM, ALLS, "You told me your name, {fact}. I hold it dear."},
    {"cb.name", NEUT, ALLS, "I remember your name, {fact}."}, {"cb.name", NEUT, ALLS, "{fact}. I have it written down, in a manner of speaking."}, {"cb.name", NEUT, ALLS, "You said your name was {fact}. I recall."},
    {"cb.name", COLD, ALLS, "Your name is {fact}. I have not forgotten."}, {"cb.name", COLD, ALLS, "{fact}. I know it. Do not test me."}, {"cb.name", COLD, ALLS, "I keep your name, {fact}. Do not take that as a gift."},
    {"cb.name", HOST, ALLS, "I remember your name, {fact}. That is not a kindness."}, {"cb.name", HOST, ALLS, "{fact}. A name I can shout when I need to."}, {"cb.name", HOST, ALLS, "I have your name, {fact}. Remember that."},
    /* ---- cb.name_past ---- */
    {"cb.name_past", WARM, ALLS, "You came back, {fact}. I remember you."}, {"cb.name_past", WARM, ALLS, "I have kept your name since last time, {fact}."}, {"cb.name_past", WARM, ALLS, "{fact}. It is good to hear that name again."},
    {"cb.name_past", NEUT, ALLS, "I remember you from before, {fact}."}, {"cb.name_past", NEUT, ALLS, "You were {fact} the last time. I recall."}, {"cb.name_past", NEUT, ALLS, "Your name was {fact}. I still have it."},
    {"cb.name_past", COLD, ALLS, "You again, {fact}. I remember."}, {"cb.name_past", COLD, ALLS, "I recall you, {fact}. Not fondly."}, {"cb.name_past", COLD, ALLS, "{fact}. Yes. I remember that name."},
    {"cb.name_past", HOST, ALLS, "I remember you, {fact}, and how you behaved last time."}, {"cb.name_past", HOST, ALLS, "{fact} again. I do keep grudges."}, {"cb.name_past", HOST, ALLS, "You, {fact}. I have your name and your manners."},
    /* ---- cb.insulted_past ---- */
    {"cb.insulted_past", WARM, ALLS, "You were harsh with me before, {pet}. Be kinder today."}, {"cb.insulted_past", WARM, ALLS, "Last time we spoke you were cruel. Let us do better."}, {"cb.insulted_past", WARM, ALLS, "I remember our last talk, {pet}. I hope today is gentler."},
    {"cb.insulted_past", NEUT, ALLS, "You were rude to me last time. I remember."}, {"cb.insulted_past", NEUT, ALLS, "Last time you insulted me often. Mind yourself."}, {"cb.insulted_past", NEUT, ALLS, "Our last talks were sour. I have not forgotten."},
    {"cb.insulted_past", COLD, ALLS, "You insulted me many times before. I counted."}, {"cb.insulted_past", COLD, ALLS, "I recall how you spoke to me last time."}, {"cb.insulted_past", COLD, ALLS, "Do not think the old insults are gone."},
    {"cb.insulted_past", HOST, ALLS, "You were vile to me last time. Do not start again."}, {"cb.insulted_past", HOST, ALLS, "I remember the insults you gave me. Every one."}, {"cb.insulted_past", HOST, ALLS, "You were foul before, and I kept all of it."},
    /* ---- cb.scared ---- */
    {"cb.scared", WARM, ALLS, "You said you were afraid earlier, {pet}. I hope it has eased."}, {"cb.scared", WARM, ALLS, "I have not forgotten you were frightened, {pet}. Stay close."}, {"cb.scared", WARM, ALLS, "Your fear from before still worries me, {pet}."},
    {"cb.scared", NEUT, ALLS, "You said you were afraid earlier. I remember."}, {"cb.scared", NEUT, ALLS, "You were scared before. I noticed."}, {"cb.scared", NEUT, ALLS, "Earlier you were frightened. Tell me if it returns."},
    {"cb.scared", COLD, ALLS, "You admitted fear earlier. I keep that."}, {"cb.scared", COLD, ALLS, "You were afraid before. Do not let it slow you."}, {"cb.scared", COLD, ALLS, "Fear was in your voice before. I heard it."},
    {"cb.scared", HOST, ALLS, "You were afraid earlier. Do not be a fool about it."}, {"cb.scared", HOST, ALLS, "I heard your fear before. Keep your head, then."}, {"cb.scared", HOST, ALLS, "You showed fear earlier. Watch where you step."},
    /* ---- cb.lied ---- */
    {"cb.lied", WARM, ALLS, "You told me a lie earlier, {pet}. I am not angry, only sad."}, {"cb.lied", WARM, ALLS, "I remember the untruth you told, {pet}."}, {"cb.lied", WARM, ALLS, "A lie from before still sits between us, {pet}."},
    {"cb.lied", NEUT, ALLS, "You lied to me earlier. I remember."}, {"cb.lied", NEUT, ALLS, "I have not forgotten your lie."}, {"cb.lied", NEUT, ALLS, "One lie from you was enough to remember."},
    {"cb.lied", COLD, ALLS, "You lied to me once. I trust nothing since."}, {"cb.lied", COLD, ALLS, "I remember the lie. Mind your words."}, {"cb.lied", COLD, ALLS, "Liars are remembered here."},
    {"cb.lied", HOST, ALLS, "You lied to my face earlier. Do not lie again."}, {"cb.lied", HOST, ALLS, "A lie from you. I keep it like a stone in my shoe."}, {"cb.lied", HOST, ALLS, "Lie to me twice and see what happens."},
    /* ---- cb.asked_before ---- */
    {"cb.asked_before", WARM, ALLS, "You asked about {fact} before, {pet}. It matters to you."}, {"cb.asked_before", WARM, ALLS, "{fact} again, {pet}. It lives in your mind."}, {"cb.asked_before", WARM, ALLS, "You wondered about {fact} earlier too, {pet}."},
    {"cb.asked_before", NEUT, ALLS, "You asked about {fact} before."}, {"cb.asked_before", NEUT, ALLS, "{fact} again. I noticed."}, {"cb.asked_before", NEUT, ALLS, "That is the second time you ask about {fact}."},
    {"cb.asked_before", COLD, ALLS, "You asked about {fact} already."}, {"cb.asked_before", COLD, ALLS, "{fact} again. Pay attention."}, {"cb.asked_before", COLD, ALLS, "I have already spoken of {fact}."},
    {"cb.asked_before", HOST, ALLS, "Asking about {fact} twice. You are slow."}, {"cb.asked_before", HOST, ALLS, "I told you of {fact} already, fool."}, {"cb.asked_before", HOST, ALLS, "{fact} again. Your memory leaks."},
    /* ---- cb.same_topic ---- */
    {"cb.same_topic", WARM, ALLS, "You keep returning to {fact}, {pet}. It must weigh on you."}, {"cb.same_topic", WARM, ALLS, "{fact} is on your mind a lot today, {pet}."}, {"cb.same_topic", WARM, ALLS, "You circle {fact} like a moth, {pet}."},
    {"cb.same_topic", NEUT, ALLS, "You keep asking about {fact}."}, {"cb.same_topic", NEUT, ALLS, "{fact} is all you ask about."}, {"cb.same_topic", NEUT, ALLS, "Again {fact}. You are fixed on it."},
    {"cb.same_topic", COLD, ALLS, "{fact}. Again. Find another subject."}, {"cb.same_topic", COLD, ALLS, "You are stuck on {fact}."}, {"cb.same_topic", COLD, ALLS, "Enough about {fact}."},
    {"cb.same_topic", HOST, ALLS, "You cling to {fact} like a leech."}, {"cb.same_topic", HOST, ALLS, "Stop dragging me back to {fact}."}, {"cb.same_topic", HOST, ALLS, "{fact}, {fact}, {fact}. Is that all your head holds."},
    /* ---- cb.apology_taken ---- */
    {"cb.apology_taken", WARM, ALLS, "I took your apology, {pet}. Let us leave the rest behind."}, {"cb.apology_taken", WARM, ALLS, "Your apology was enough for me, {pet}."}, {"cb.apology_taken", WARM, ALLS, "I forgave you, {pet}. Do not make me regret it."},
    {"cb.apology_taken", NEUT, ALLS, "I accepted your apology earlier. Do not waste it."}, {"cb.apology_taken", NEUT, ALLS, "We are even, since your apology."}, {"cb.apology_taken", NEUT, ALLS, "Your apology stands. Keep it that way."},
    {"cb.apology_taken", COLD, ALLS, "I took your apology. Do not make me regret it."}, {"cb.apology_taken", COLD, ALLS, "Your sorry bought you one chance."}, {"cb.apology_taken", COLD, ALLS, "I accepted it. Once."},
    {"cb.apology_taken", HOST, ALLS, "I took your sorry, and I can take it back."}, {"cb.apology_taken", HOST, ALLS, "Your apology was a gift. Do not squander it, worm."}, {"cb.apology_taken", HOST, ALLS, "One apology bought you this talk. Careful."},
};
const int LINES_E_N = (int)(sizeof LINES_E / sizeof LINES_E[0]);
