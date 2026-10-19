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
};
const int LINES_E_N = (int)(sizeof LINES_E / sizeof LINES_E[0]);
