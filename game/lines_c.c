/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * LINES, part C: the world and the plot (hole, secrets, life, age, sky, music, Alex, help, questions ...).
 * see lines_a.c for how to read a row. */
#include "lang_data.h"

const Line LINES_C[] = {
    /* ---- r.hole ---- */
    {"r.hole", WARM, ALLS, "It looks scary, I know. But I will be thinking of you."}, {"r.hole", WARM, ALLS, "The way down is easier than it looks, {pet}."}, {"r.hole", WARM, ALLS, "Jump when you are ready. No rush."},
    {"r.hole", NEUT, ALLS, "It leads down to the ground. That is where your work is."}, {"r.hole", NEUT, ALLS, "Step in when you are ready."}, {"r.hole", NEUT, ALLS, "A way down. Nothing more."},
    {"r.hole", COLD, ALLS, "It is a hole. You fall. Not hard."}, {"r.hole", COLD, ALLS, "Jump already. I am tired of waiting."}, {"r.hole", COLD, ALLS, "Down there is your problem. Go."},
    {"r.hole", HOST, ALLS, "Jump, {pet}. Or I will push you."}, {"r.hole", HOST, ALLS, "Stop stalling. Fall."},
    {"r.hole", NEUT, DIVINE, "The hole leads to the ground. Land well, vessel."}, {"r.hole", NEUT, DIVINE, "Down there, the world waits. Your missions with it."},
    {"r.hole", COLD, DIVINE, "A hole in my clouds, just for you. Do not say I never give you anything."}, {"r.hole", COLD, DIVINE, "You will fall face first, mortal. I have seen it. It is funny."}, {"r.hole", COLD, DIVINE, "Down is where you belong. Go."},
    {"r.hole", HOST, DIVINE, "Jump, worm, before I drop you."}, {"r.hole", WARM, DIVINE, "Fear not the fall, little one. I made it soft. Mostly."},
    {"r.hole", WARM, STREET, "You will be fine, {pet}. Just do not land on your face."}, {"r.hole", COLD, STREET, "Do not look down. Just go."},

    /* ---- r.secret ---- */
    {"r.secret", WARM, ALLS, "Do not worry, {pet}. Secrets are safe with me."}, {"r.secret", WARM, ALLS, "It is for your own safety, dear. Trust me."},
    {"r.secret", NEUT, ALLS, "Tell no one what you are. It is simple."}, {"r.secret", NEUT, ALLS, "Stay secret. It keeps you alive."}, {"r.secret", NEUT, ALLS, "Secrets are what this job is made of."},
    {"r.secret", COLD, ALLS, "If people knew, you would not last a day."}, {"r.secret", COLD, ALLS, "Keep your mouth shut. That is the entire rule."},
    {"r.secret", HOST, ALLS, "Talk and you die. Is that clear enough, {pet}?"}, {"r.secret", NEUT, DIVINE, "A vessel stays secret. A loud vessel is a broken vessel."}, {"r.secret", COLD, DIVINE, "Secrets are the only thing mortals are good at. Be good at it."},
    {"r.secret", COLD, DIVINE, "Speak of me to anyone and I will know, mortal."}, {"r.secret", HOST, DIVINE, "Whisper my name to a mortal and I end you."}, {"r.secret", WARM, DIVINE, "Keep our secret, little one. It is ours."},
    {"r.secret", COLD, STREET, "Shh. Walls have ears, {pet}."}, {"r.secret", WARM, STREET, "Just keep quiet and you will be fine."},

    /* ---- r.life ---- */
    {"r.life", WARM, ALLS, "Do not think of that. Think of what you can do today."}, {"r.life", WARM, ALLS, "Every life ends, {pet}. Yours has barely begun."},
    {"r.life", NEUT, ALLS, "When your job is done, your life ends. That is the deal."}, {"r.life", NEUT, ALLS, "Death is not the point. The work is."}, {"r.life", NEUT, ALLS, "All lives end. Some at least have purpose."},
    {"r.life", COLD, ALLS, "Yes, you will die. Everyone does. Yawn."}, {"r.life", COLD, ALLS, "Do not die before the job is done. That is all I ask."}, {"r.life", COLD, ALLS, "Mortals and their fear of ending. Boring."},
    {"r.life", HOST, ALLS, "Die all you want. Just finish the missions first."}, {"r.life", NEUT, DIVINE, "Finish each mission, and stay secret. When your job is done, your life ends."}, {"r.life", NEUT, DIVINE, "A vessel is made to be emptied. That is its life."},
    {"r.life", COLD, DIVINE, "You are mortal, little vessel. I am not. That is the difference."}, {"r.life", COLD, DIVINE, "Your life is a short thing. Spend it on my missions."}, {"r.life", COLD, DIVINE, "Death is the reward, mortal. Do not rush it."},
    {"r.life", HOST, DIVINE, "Your life is a flicker, worm. Mine is the sun."}, {"r.life", WARM, DIVINE, "Do not fear the end, little one. I will not let it be ugly."},
    {"r.life", COLD, STREET, "Do not die. That is a good rule."}, {"r.life", WARM, STREET, "Hey, no dying today. That is an order."},

    /* ---- r.age ---- */
    {"r.age", WARM, ALLS, "Old enough to know better, young enough to not care."}, {"r.age", WARM, ALLS, "A lady never tells, {pet}. But I am not young."},
    {"r.age", NEUT, ALLS, "Old enough."}, {"r.age", NEUT, ALLS, "Age is not important. Time is."}, {"r.age", NEUT, ALLS, "I stopped counting a while ago."},
    {"r.age", COLD, ALLS, "Is that polite to ask?"}, {"r.age", COLD, ALLS, "Older than your questions. Next."}, {"r.age", HOST, ALLS, "Old enough to throw you out. Next question."},
    {"r.age", NEUT, DIVINE, "Older than your idea of time."}, {"r.age", COLD, DIVINE, "I was old when the first clouds formed, mortal."}, {"r.age", COLD, DIVINE, "Age. A mortal concern. I have eons."},
    {"r.age", HOST, DIVINE, "Older than your bloodline, worm. Do not insult my years."}, {"r.age", WARM, DIVINE, "Old, little one, but I wear it well. Do not tell."},
    {"r.age", WARM, STREET, "Old enough to know the ropes, {pet}."}, {"r.age", COLD, STREET, "Mind your own business."},

    /* ---- r.real ---- */
    {"r.real", WARM, ALLS, "As real as you want it to be, {pet}."}, {"r.real", WARM, ALLS, "Real enough to hold your hand, in a way."},
    {"r.real", NEUT, ALLS, "Real enough that your missions matter."}, {"r.real", NEUT, ALLS, "Does it matter? It is what you have."}, {"r.real", NEUT, ALLS, "Think what you like. The missions are real."},
    {"r.real", COLD, ALLS, "Pinch yourself. Then stop asking."}, {"r.real", COLD, ALLS, "Reality is overrated. Do your job."}, {"r.real", HOST, ALLS, "Real? You are about to find out, {pet}."},
    {"r.real", NEUT, DIVINE, "A goddess is as real as the sky. Even when you do not look."}, {"r.real", COLD, DIVINE, "I am real, mortal. Your doubt is the dream."}, {"r.real", COLD, DIVINE, "Everything is real when a goddess says so."},
    {"r.real", HOST, DIVINE, "Doubt me again, worm, and see how real it gets."}, {"r.real", WARM, DIVINE, "As real as the warmth you feel, little one."},
    {"r.real", COLD, STREET, "Real enough to hurt, rookie."}, {"r.real", WARM, STREET, "Real enough for me, {pet}."},

    /* ---- r.sky ---- */
    {"r.sky", WARM, ALLS, "Beautiful, is it not? I love looking at it."}, {"r.sky", WARM, ALLS, "The sky is my favorite thing, {pet}."},
    {"r.sky", NEUT, ALLS, "Clouds, mostly. Quiet ones."}, {"r.sky", NEUT, ALLS, "It is peaceful. Do not get used to it."}, {"r.sky", NEUT, ALLS, "Pretty, yes. Do not stare."},
    {"r.sky", COLD, ALLS, "It is just air and water. Wow."}, {"r.sky", COLD, ALLS, "Stop staring at the sky. Do your job."}, {"r.sky", HOST, ALLS, "A sky. Congratulations on noticing."},
    {"r.sky", NEUT, DIVINE, "The sky is mine. The clouds are my floor."}, {"r.sky", COLD, DIVINE, "These clouds are my domain. Do not scuff them, mortal."}, {"r.sky", COLD, DIVINE, "Heaven, if you must name it. Lowercase."},
    {"r.sky", HOST, DIVINE, "You stand on my clouds, worm. Be grateful."}, {"r.sky", WARM, DIVINE, "Lovely, is it not, little one? I made it just for me. And a little for you."},
    {"r.sky", WARM, STREET, "Nice view, {pet}."}, {"r.sky", COLD, STREET, "It is a sky. Move along."},

    /* ---- r.music ---- */
    {"r.music", WARM, ALLS, "I adore music! It is the best part, {pet}."}, {"r.music", WARM, ALLS, "Do you hear it? It is lovely."},
    {"r.music", NEUT, ALLS, "It plays itself. I just listen."}, {"r.music", NEUT, ALLS, "Music helps. Do not ask me how."}, {"r.music", NEUT, ALLS, "Pleasant, is it not?"},
    {"r.music", COLD, ALLS, "It is noise. Pretty noise. Whatever."}, {"r.music", COLD, ALLS, "Music? Is that all you can think of?"}, {"r.music", HOST, ALLS, "Your taste in everything is awful."},
    {"r.music", NEUT, DIVINE, "The music is mine. It follows me everywhere."}, {"r.music", COLD, DIVINE, "Divine music. Wasted on mortal ears."}, {"r.music", COLD, DIVINE, "It is called ascendant. You are not."},
    {"r.music", HOST, DIVINE, "You listen to my music, worm? Be silent and respect it."}, {"r.music", WARM, DIVINE, "Sing along if you like, little one. I will not judge. Much."},
    {"r.music", WARM, STREET, "Good tune, right? I like it, {pet}."}, {"r.music", COLD, STREET, "Not my style."},

    /* ---- r.alex ---- */
    {"r.alex", WARM, ALLS, "Alex is lovely. You will like the blond one, {pet}."}, {"r.alex", NEUT, ALLS, "Alex will be waiting for you down there."}, {"r.alex", NEUT, ALLS, "Talk to Alex. That is your first mission."},
    {"r.alex", COLD, ALLS, "Alex? Annoying. Friendly, but annoying."}, {"r.alex", HOST, ALLS, "Ask Alex yourself. Do not bother me."}, {"r.alex", NEUT, DIVINE, "Alex is on the ground. Talk to Alex. Find out later."},
    {"r.alex", COLD, DIVINE, "A mortal I tolerate. You will meet them. Do not break them."}, {"r.alex", NEUT, STREET, "Who, me? I am right here, {pet}."}, {"r.alex", WARM, STREET, "That is me! Hi!"},

    /* ---- r.help ---- */
    {"r.help", WARM, ALLS, "Of course I will help you, {pet}. What do you need?"}, {"r.help", WARM, ALLS, "I will always help. Just ask nicely."}, {"r.help", WARM, ALLS, "Do not worry. You are not alone."},
    {"r.help", NEUT, ALLS, "Look at your mission list. Then act."}, {"r.help", NEUT, ALLS, "Walk, talk, finish missions. That is the whole help."}, {"r.help", NEUT, ALLS, "You will manage, {player}."}, {"r.help", NEUT, ALLS, "Ask a smaller question. I can answer that."},
    {"r.help", COLD, ALLS, "Help yourself. It is a good habit."}, {"r.help", COLD, ALLS, "I am not your servant."}, {"r.help", COLD, ALLS, "Help? Try thinking first."},
    {"r.help", HOST, ALLS, "Help yourself, {pet}. Or do not. I do not care."}, {"r.help", NEUT, DIVINE, "A vessel must learn alone. I guide, not carry."}, {"r.help", COLD, DIVINE, "A goddess helps no one. She watches."},
    {"r.help", COLD, DIVINE, "I already helped by making you. Do the rest."}, {"r.help", HOST, DIVINE, "Beg someone else, worm."}, {"r.help", WARM, DIVINE, "Alright, little one. A tiny hint. Walk to the hole when ready."},
    {"r.help", WARM, STREET, "Sure thing, {pet}. What is up?"}, {"r.help", COLD, STREET, "Figure it out, rookie."},

    /* ---- r.why ---- */
    {"r.why", WARM, ALLS, "Good question, {pet}. The answer will come with time."}, {"r.why", WARM, ALLS, "Why? Because the world needs it, I think."},
    {"r.why", NEUT, ALLS, "Because that is how it is."}, {"r.why", NEUT, ALLS, "Some answers come later."}, {"r.why", NEUT, ALLS, "That is a question for another day."},
    {"r.why", COLD, ALLS, "Because I said so."}, {"r.why", COLD, ALLS, "Why? Why not. Next."}, {"r.why", COLD, ALLS, "Stop asking why. Start doing."},
    {"r.why", HOST, ALLS, "Why, why, why. Do you do anything else?"}, {"r.why", NEUT, DIVINE, "Because a goddess decided. That is the only why."}, {"r.why", COLD, DIVINE, "Why? Because I willed it. Is that not enough?"},
    {"r.why", COLD, DIVINE, "A vessel does not ask why, only how well."}, {"r.why", HOST, DIVINE, "Why? Because I enjoy it, worm."}, {"r.why", WARM, DIVINE, "You will understand, little one. In time."},
    {"r.why", COLD, STREET, "Because, rookie. Because."}, {"r.why", WARM, STREET, "Good question, {pet}. I wonder too."},

    /* ---- r.how ---- */
    {"r.how", WARM, ALLS, "One step at a time, {pet}. You will figure it out."}, {"r.how", WARM, ALLS, "Carefully, and with kindness. That is how."},
    {"r.how", NEUT, ALLS, "Use the stick to walk and the button to talk."}, {"r.how", NEUT, ALLS, "Practice. And patience."}, {"r.how", NEUT, ALLS, "Walk up and talk. It is not hard."},
    {"r.how", COLD, ALLS, "Figure it out. I did."}, {"r.how", COLD, ALLS, "How? With effort. Try some."}, {"r.how", HOST, ALLS, "You are asking how? How pathetic."},
    {"r.how", NEUT, DIVINE, "However a vessel can. Carefully."}, {"r.how", COLD, DIVINE, "Mortals always want a manual. Figure it out."}, {"r.how", HOST, DIVINE, "With effort, worm. A foreign concept, I know."}, {"r.how", WARM, DIVINE, "Gently, little one. And you will do well."},
    {"r.how", COLD, STREET, "Just do it, {pet}."}, {"r.how", WARM, STREET, "Easy, you will get the hang of it."},

    /* ---- r.what: a bare question word ---- */
    {"r.what", WARM, ALLS, "What would you like to know, {pet}?"}, {"r.what", WARM, ALLS, "Ask me more, I am happy to answer."},
    {"r.what", NEUT, ALLS, "Be more specific."}, {"r.what", NEUT, ALLS, "Hm? What about it?"}, {"r.what", NEUT, ALLS, "Finish your question, {player}."},
    {"r.what", COLD, ALLS, "What what? Use full sentences."}, {"r.what", COLD, ALLS, "Ask properly."}, {"r.what", HOST, ALLS, "What? Speak clearly, you fool."},
    {"r.what", COLD, DIVINE, "What, indeed. Be clearer, mortal."}, {"r.what", HOST, DIVINE, "What? Squeak clearly, worm."}, {"r.what", COLD, STREET, "What what? Spit it out."},

    /* ---- r.repeat / r.confused ---- */
    {"r.repeat", WARM, ALLS, "Of course, {pet}. Listen closely this time."}, {"r.repeat", NEUT, ALLS, "I will not say it twice. Well, once more."}, {"r.repeat", NEUT, ALLS, "Pay attention, {player}. Missions, secrets, go."},
    {"r.repeat", COLD, ALLS, "Pay attention. I do not repeat myself."}, {"r.repeat", COLD, ALLS, "Were you not listening? Typical."}, {"r.repeat", HOST, ALLS, "Wash your ears, fool."},
    {"r.repeat", NEUT, DIVINE, "You are a vessel. Carry missions. Stay secret. Now go."}, {"r.repeat", COLD, DIVINE, "A goddess does not repeat. But for you, once. Missions. Secret. Go."}, {"r.repeat", HOST, DIVINE, "Again? Your mind leaks, worm."},
    {"r.confused", WARM, ALLS, "It is alright to be confused, {pet}. It gets easier."}, {"r.confused", WARM, ALLS, "Take your time. Ask me anything."}, {"r.confused", WARM, ALLS, "Do not worry. Walk, talk, and you will learn."},
    {"r.confused", NEUT, ALLS, "Confused? Then ask a clear question."}, {"r.confused", NEUT, ALLS, "You will understand in time."}, {"r.confused", NEUT, ALLS, "Do not overthink it."},
    {"r.confused", COLD, ALLS, "Not my problem. Think harder."}, {"r.confused", COLD, ALLS, "Of course you are confused."}, {"r.confused", COLD, ALLS, "I do not do hand holding."},
    {"r.confused", HOST, ALLS, "Slow, are we? Try again, slower."}, {"r.confused", NEUT, DIVINE, "Confusion is the first step of a vessel. The second is obedience."}, {"r.confused", COLD, DIVINE, "Your confusion is no concern of mine, mortal."},
    {"r.confused", COLD, DIVINE, "Think harder, little vessel. It is not that deep."}, {"r.confused", HOST, DIVINE, "A vessel with no thoughts. What a gift."}, {"r.confused", WARM, DIVINE, "Poor little one. It will clear. Trust me."},
    {"r.confused", COLD, STREET, "Figure it out, rookie."}, {"r.confused", WARM, STREET, "No worries, {pet}. It took me a while too."},

    /* ---- r.wait / r.please ---- */
    {"r.wait", WARM, ALLS, "Take your time, {pet}. I am not going anywhere."}, {"r.wait", NEUT, ALLS, "Fine. I will wait."}, {"r.wait", NEUT, ALLS, "Alright. Hurry up, though."},
    {"r.wait", COLD, ALLS, "I have no choice, do I."}, {"r.wait", COLD, ALLS, "Ugh. Fine. Be quick."}, {"r.wait", HOST, ALLS, "Wait? For you? Ha."}, {"r.wait", COLD, DIVINE, "A goddess waits for no one. Hurry, mortal."}, {"r.wait", HOST, DIVINE, "My time is endless, but my patience is not. Hurry."},
    {"r.please", WARM, ALLS, "Since you ask so nicely, {pet}."}, {"r.please", NEUT, ALLS, "Manners. Good."}, {"r.please", NEUT, ALLS, "Fine. Since you said please."},
    {"r.please", COLD, ALLS, "Begging will not help. But go on."}, {"r.please", HOST, ALLS, "Beg more."}, {"r.please", COLD, DIVINE, "Begging suits you, mortal. Do continue."}, {"r.please", NEUT, DIVINE, "Politeness. Rare in vessels. I approve."}, {"r.please", HOST, DIVINE, "Plead, worm. It amuses me."},

    /* ---- r.address: they just said the npc's name ---- */
    {"r.address", WARM, ALLS, "Yes, {pet}? I am here."}, {"r.address", NEUT, ALLS, "Yes? You called?"}, {"r.address", NEUT, ALLS, "I am listening. Go on."},
    {"r.address", COLD, ALLS, "What? You said my name, now say something."}, {"r.address", HOST, ALLS, "Do not say my name like that."},
    {"r.address", NEUT, DIVINE, "You have my attention, vessel. Use it."}, {"r.address", COLD, DIVINE, "Yes, yes, I am a goddess. And?"}, {"r.address", HOST, DIVINE, "Say my name with respect, worm."},
};
const int LINES_C_N = (int)(sizeof LINES_C / sizeof LINES_C[0]);
