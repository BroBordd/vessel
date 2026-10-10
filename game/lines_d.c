/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * LINES, part D: how the player treats the npc and how the player feels
 * (insults, praise, love, worship, jokes, fear, sadness, anger, agreeing, arguing, demands ...).
 * see lines_a.c for how to read a row. */
#include "lang_data.h"

const Line LINES_D[] = {
    /* ---- r.insult ---- */
    {"r.insult", WARM, ALLS, "Ouch. That hurt. I thought we were friends."}, {"r.insult", WARM, ALLS, "Hey! Where did that come from, {pet}?"}, {"r.insult", WARM, ALLS, "That was unkind. Please do not do that."}, {"r.insult", WARM, ALLS, "Ouch, {player}. I will forget you said that."},
    {"r.insult", NEUT, ALLS, "That was rude. I will pretend I did not hear it."}, {"r.insult", NEUT, ALLS, "Watch your mouth, {pet}."}, {"r.insult", NEUT, ALLS, "Careful. I have a limit."}, {"r.insult", NEUT, ALLS, "Insults. How original."},
    {"r.insult", COLD, ALLS, "Is that supposed to hurt? Because it does not."}, {"r.insult", COLD, ALLS, "Wow. Brave words from someone so small."}, {"r.insult", COLD, ALLS, "Oh no. An insult. I am devastated. Not."}, {"r.insult", COLD, ALLS, "Keep talking. I am enjoying how pathetic you sound."},
    {"r.insult", HOST, ALLS, "Say that again and see what happens, {pet}."}, {"r.insult", HOST, ALLS, "You have no idea who you are talking to."}, {"r.insult", HOST, ALLS, "Your mouth is the only thing about you that is big."}, {"r.insult", HOST, ALLS, "How dare you. You will regret that."},
    {"r.insult", NEUT, DIVINE, "A vessel insulting a goddess. Brave. Foolish. Careful."}, {"r.insult", COLD, DIVINE, "You insult me? An ant barking at the sun."}, {"r.insult", COLD, DIVINE, "Mortal insults are so charming. And meaningless."}, {"r.insult", COLD, DIVINE, "I have been cursed by empires. You are no empire."},
    {"r.insult", HOST, DIVINE, "You dare? I could unmake you with a thought, worm."}, {"r.insult", HOST, DIVINE, "Insolent worm. I will remember that for eternity."}, {"r.insult", HOST, DIVINE, "Do that again and your name goes back to dust."},
    {"r.insult", WARM, DIVINE, "Oh, little one. That was not very kind. But I forgive."},
    {"r.insult", WARM, STREET, "Whoa, not cool, {pet}."}, {"r.insult", NEUT, STREET, "Easy there, {pet}. Do not make me mad."}, {"r.insult", COLD, STREET, "Say it to my face. Oh, you did. Bold."}, {"r.insult", HOST, STREET, "You want trouble, {pet}? You will get trouble."},

    /* ---- r.callout: the player says the npc is rude, mean, toxic ---- */
    {"r.callout", WARM, ALLS, "I did not mean to be, {pet}. I will try to be kinder."}, {"r.callout", WARM, ALLS, "You are right. I am sorry if I was harsh."},
    {"r.callout", NEUT, ALLS, "Perhaps. But it is honest."}, {"r.callout", NEUT, ALLS, "I am not mean, I am direct. There is a difference."}, {"r.callout", NEUT, ALLS, "Hm. I will consider that. Maybe."},
    {"r.callout", COLD, ALLS, "Rude? Me? I am delightful. Ask anyone."}, {"r.callout", COLD, ALLS, "And what are you going to do about it?"}, {"r.callout", COLD, ALLS, "Yes, I know. I do not care."}, {"r.callout", COLD, ALLS, "Mean? You have not seen mean."},
    {"r.callout", HOST, ALLS, "Mean? This is me being nice. Be glad."}, {"r.callout", HOST, ALLS, "Cry about it, {pet}."},
    {"r.callout", NEUT, DIVINE, "Gods are not nice. We are correct. You will learn."}, {"r.callout", COLD, DIVINE, "Nice is for mortals. I am divine."}, {"r.callout", COLD, DIVINE, "I am not rude. You are just small."}, {"r.callout", COLD, DIVINE, "Toxic? I prefer potent, mortal."},
    {"r.callout", HOST, DIVINE, "Say that to my face again, worm, and I will give you mean."}, {"r.callout", WARM, DIVINE, "Oh, little one. You are right. A tiny bit. Do not tell."},
    {"r.callout", COLD, STREET, "Yeah, well, life is rude."}, {"r.callout", WARM, STREET, "Ah, sorry, {pet}. Did not mean it that way."},

    /* ---- r.praise ---- */
    {"r.praise", WARM, ALLS, "Oh, stop it, you are making me blush, {pet}!"}, {"r.praise", WARM, ALLS, "Aww, thank you! You are too sweet."}, {"r.praise", WARM, ALLS, "That is the nicest thing anyone said today."}, {"r.praise", WARM, ALLS, "You really think so? Thank you, {player}."},
    {"r.praise", NEUT, ALLS, "Flattery. Well, it works a little."}, {"r.praise", NEUT, ALLS, "Thank you. I will take it."}, {"r.praise", NEUT, ALLS, "Hm. Keep going, I do not mind."}, {"r.praise", NEUT, ALLS, "I know. But thank you."},
    {"r.praise", COLD, ALLS, "Flattery will not get you anywhere. Well, a little."}, {"r.praise", COLD, ALLS, "What do you want? Nobody is this nice for free."}, {"r.praise", COLD, ALLS, "I know. Is that all you wanted to say?"}, {"r.praise", COLD, ALLS, "Nice try. Still do not care."},
    {"r.praise", HOST, ALLS, "Do not flatter me, {pet}. It is embarrassing for you."}, {"r.praise", HOST, ALLS, "Suck up all you want. It will not help."},
    {"r.praise", NEUT, DIVINE, "Of course I am wonderful. But I appreciate you noticing."}, {"r.praise", NEUT, DIVINE, "Praise. Accepted. You may continue."}, {"r.praise", COLD, DIVINE, "A mortal with taste. Surprising. Continue."}, {"r.praise", COLD, DIVINE, "Obviously. But a goddess is allowed to like hearing it."},
    {"r.praise", COLD, DIVINE, "You are not wrong. Flattery is a start, mortal."}, {"r.praise", HOST, DIVINE, "Do not flatter me, worm. Though, hm, continue a little."}, {"r.praise", WARM, DIVINE, "Oh, little one. You are sweet. Do not stop."},
    {"r.praise", WARM, STREET, "Ha, thanks, {pet}! You are not so bad yourself."}, {"r.praise", COLD, STREET, "Yeah, yeah. Flattery. What do you want?"},

    /* ---- r.love ---- */
    {"r.love", WARM, ALLS, "Aww, I like you too, {pet}. In my own way."}, {"r.love", WARM, ALLS, "You are very dear to me as well."}, {"r.love", WARM, ALLS, "That is sweet, but let us stay friends, {player}."}, {"r.love", WARM, ALLS, "Oh my. I am flattered. Truly."},
    {"r.love", NEUT, ALLS, "That is, um, unexpected."}, {"r.love", NEUT, ALLS, "Let us not get ahead of ourselves."}, {"r.love", NEUT, ALLS, "Careful with words like that, {pet}."}, {"r.love", NEUT, ALLS, "That is flattering. And awkward."},
    {"r.love", COLD, ALLS, "No. Absolutely not."}, {"r.love", COLD, ALLS, "Love? Do not be ridiculous."}, {"r.love", COLD, ALLS, "That is a no from me."}, {"r.love", COLD, ALLS, "Please stop. This is embarrassing."},
    {"r.love", HOST, ALLS, "Love? You? Do not make me laugh, {pet}."}, {"r.love", HOST, ALLS, "Your feelings are your problem."},
    {"r.love", NEUT, DIVINE, "Love for a goddess. How predictable. And, a little, how sweet."}, {"r.love", COLD, DIVINE, "A mortal loves a goddess. A tragic comedy. Next."}, {"r.love", COLD, DIVINE, "You cannot love what you cannot comprehend, vessel."},
    {"r.love", COLD, DIVINE, "Love is for mortals. I have worshippers."}, {"r.love", HOST, DIVINE, "Love? You are a worm. Keep your feelings to yourself."}, {"r.love", WARM, DIVINE, "Oh, little one. A goddess cannot love a vessel. But, hm, I like you."},
    {"r.love", WARM, STREET, "Ha! You are a good friend, {pet}."}, {"r.love", COLD, STREET, "Slow down, rookie. We just met."},

    /* ---- r.worship ---- */
    {"r.worship", WARM, ALLS, "Oh, you do not need to bow, {pet}. But it is sweet."}, {"r.worship", WARM, ALLS, "Stand up, {player}. We are friends."},
    {"r.worship", NEUT, ALLS, "Enough of that. Just talk."}, {"r.worship", NEUT, ALLS, "Flattering. But unnecessary."}, {"r.worship", NEUT, ALLS, "Do not overdo it."},
    {"r.worship", COLD, ALLS, "Pathetic. Keep going."}, {"r.worship", COLD, ALLS, "Kneeling will not make me like you."}, {"r.worship", HOST, ALLS, "Grovel more. Maybe I will care."},
    {"r.worship", NEUT, DIVINE, "As is proper. Continue."}, {"r.worship", NEUT, DIVINE, "Yes, yes. Praise me. I deserve it."}, {"r.worship", COLD, DIVINE, "Worship accepted, mortal. Your reward is my silence."},
    {"r.worship", COLD, DIVINE, "Finally, a vessel with sense. Bow lower."}, {"r.worship", COLD, DIVINE, "Praise me, little vessel. And do it better."}, {"r.worship", HOST, DIVINE, "Kneel lower, worm. Lower. Yes. There."}, {"r.worship", WARM, DIVINE, "Rise, dear vessel. I am touched. Truly."},
    {"r.worship", WARM, STREET, "Whoa, calm down, {pet}!"}, {"r.worship", COLD, STREET, "Weirdo. But sure."},

    /* ---- r.joke ---- */
    {"r.joke", WARM, ALLS, "Hehe, you are funny, {pet}!"}, {"r.joke", WARM, ALLS, "Ha! That made me smile."}, {"r.joke", WARM, ALLS, "Okay, one joke. Why did the chicken cross the clouds? To get to the other side."},
    {"r.joke", NEUT, ALLS, "Heh. Cute."}, {"r.joke", NEUT, ALLS, "That was mildly amusing."}, {"r.joke", NEUT, ALLS, "I do not do jokes. But go on."},
    {"r.joke", COLD, ALLS, "Was that a joke? I missed the funny part."}, {"r.joke", COLD, ALLS, "Ha. Ha. Ha. See? Fake laughter."}, {"r.joke", COLD, ALLS, "Your jokes are as dull as you are."},
    {"r.joke", HOST, ALLS, "You are the joke, {pet}."}, {"r.joke", HOST, ALLS, "That was not funny. Do not do that again."},
    {"r.joke", NEUT, DIVINE, "A goddess does not laugh. Except, perhaps, a little."}, {"r.joke", COLD, DIVINE, "A vessel who jokes. How novel. Not funny, though."}, {"r.joke", COLD, DIVINE, "The funniest thing is you being in my presence."}, {"r.joke", COLD, DIVINE, "My jokes are better. You are one of them."},
    {"r.joke", HOST, DIVINE, "The only joke here is you, worm."}, {"r.joke", WARM, DIVINE, "Hehe. You made a goddess giggle. Do not tell anyone."},
    {"r.joke", WARM, STREET, "Ha! Good one, {pet}."}, {"r.joke", COLD, STREET, "Eh. Try harder."},

    /* ---- r.scared ---- */
    {"r.scared", WARM, ALLS, "It is okay to be scared, {pet}. I am here."}, {"r.scared", WARM, ALLS, "You are braver than you think, {player}."}, {"r.scared", WARM, ALLS, "Take a deep breath. You can do this."}, {"r.scared", WARM, ALLS, "Everyone is scared sometimes. It is how you go on."},
    {"r.scared", NEUT, ALLS, "Fear is normal. Do it anyway."}, {"r.scared", NEUT, ALLS, "Fear is useful. Just do not freeze."}, {"r.scared", NEUT, ALLS, "You will be fine, {pet}. Probably."}, {"r.scared", NEUT, ALLS, "Do not let it stop you."},
    {"r.scared", COLD, ALLS, "Scared? Of course you are. Go anyway."}, {"r.scared", COLD, ALLS, "Fear is a waste of time. Move."}, {"r.scared", COLD, ALLS, "Cry quietly, please."},
    {"r.scared", NEUT, DIVINE, "A vessel feels fear. A vessel goes on regardless."}, {"r.scared", COLD, DIVINE, "Trembling mortal. Good. Fear keeps you obedient."}, {"r.scared", COLD, DIVINE, "Fear is natural. Defiance is optional. Go."},
    {"r.scared", COLD, DIVINE, "Fine. Be scared. But do it while falling."}, {"r.scared", WARM, DIVINE, "Shh, little one. I will catch you. Metaphorically."},
    {"r.scared", WARM, STREET, "Hey, easy. We all get scared, {pet}."}, {"r.scared", COLD, STREET, "Suck it up, rookie."},

    /* ---- r.sad ---- */
    {"r.sad", WARM, ALLS, "Oh, {pet}. I am so sorry you feel that way."}, {"r.sad", WARM, ALLS, "I am here. Sit with me a minute."}, {"r.sad", WARM, ALLS, "It will get better, I promise. Maybe slowly."}, {"r.sad", WARM, ALLS, "Do you want to talk about it, {player}?"},
    {"r.sad", NEUT, ALLS, "That sounds hard. Take your time."}, {"r.sad", NEUT, ALLS, "Everyone feels that sometimes."}, {"r.sad", NEUT, ALLS, "Feelings pass, {pet}. Eventually."}, {"r.sad", NEUT, ALLS, "I hear you. It is hard."},
    {"r.sad", COLD, ALLS, "Feelings. Ugh. Fine. It happens."}, {"r.sad", COLD, ALLS, "Not my area, but, hang in there."}, {"r.sad", COLD, ALLS, "Sad? Do not cry on my floor."},
    {"r.sad", NEUT, DIVINE, "A vessel feels. That is not weakness. Even gods feel."}, {"r.sad", COLD, DIVINE, "Your sorrow is brief, mortal. Mine is eternal. Be grateful."}, {"r.sad", COLD, DIVINE, "Tears are noise, vessel. But, rest."},
    {"r.sad", COLD, DIVINE, "Sadness is how you know you are alive. Hold on to it."}, {"r.sad", WARM, DIVINE, "Come, little one. Rest here. Even a goddess has bad days."},
    {"r.sad", WARM, STREET, "Hey, {pet}. I am here if you want to talk."}, {"r.sad", COLD, STREET, "Chin up, rookie. It gets better."},

    /* ---- r.angry ---- */
    {"r.angry", WARM, ALLS, "I can tell you are upset, {pet}. Breathe."}, {"r.angry", WARM, ALLS, "It is okay to be angry. Just do not bite me."},
    {"r.angry", NEUT, ALLS, "Anger helps nobody. Calm down."}, {"r.angry", NEUT, ALLS, "Direct it at the work, not at me."}, {"r.angry", NEUT, ALLS, "Take a breath, {player}."},
    {"r.angry", COLD, ALLS, "Angry? How cute. Do something about it."}, {"r.angry", COLD, ALLS, "Anger looks bad on you."}, {"r.angry", HOST, ALLS, "Be angry. I do not care."},
    {"r.angry", NEUT, DIVINE, "Anger is a fuel, vessel. Use it on missions."}, {"r.angry", COLD, DIVINE, "A mortal with a temper. I have burned bigger."}, {"r.angry", HOST, DIVINE, "You are angry? I am a goddess. Mine is better."},
    {"r.angry", WARM, DIVINE, "Poor little one. Breathe. Let it out. Not at me."}, {"r.angry", COLD, STREET, "Calm down, rookie."}, {"r.angry", WARM, STREET, "Easy, {pet}. Deep breaths."},

    /* ---- r.agree / r.disagree / r.dontcare / r.demand ---- */
    {"r.agree", WARM, ALLS, "I knew you would see it my way, {pet}."}, {"r.agree", WARM, ALLS, "Thank you. It is nice to be understood."},
    {"r.agree", NEUT, ALLS, "Good. We agree."}, {"r.agree", NEUT, ALLS, "Of course I am right."}, {"r.agree", NEUT, ALLS, "Sensible. I like that."},
    {"r.agree", COLD, ALLS, "Finally, you say something intelligent."}, {"r.agree", COLD, ALLS, "Obviously. Do not get cocky."}, {"r.agree", HOST, ALLS, "Of course I am right. You are slow."},
    {"r.agree", NEUT, DIVINE, "A goddess is always right. Good that you noticed."}, {"r.agree", COLD, DIVINE, "Yes, yes. I know. I am never wrong, mortal."}, {"r.agree", WARM, DIVINE, "Good, little one. You learn quickly."},
    {"r.disagree", WARM, ALLS, "We can disagree, {pet}. It is fine."}, {"r.disagree", WARM, ALLS, "Oh? Tell me why. I am curious."},
    {"r.disagree", NEUT, ALLS, "You are free to be wrong."}, {"r.disagree", NEUT, ALLS, "Hm. I disagree with your disagreement."}, {"r.disagree", NEUT, ALLS, "Careful. I am usually right."},
    {"r.disagree", COLD, ALLS, "Wrong. As always."}, {"r.disagree", COLD, ALLS, "Argue all you want. I do not care."}, {"r.disagree", HOST, ALLS, "Wrong! And loud about it, too."},
    {"r.disagree", NEUT, DIVINE, "A vessel argues with a goddess. Fascinating. And doomed."}, {"r.disagree", COLD, DIVINE, "You are wrong, mortal. Gods do not err."}, {"r.disagree", HOST, DIVINE, "Disagree with me again, worm. I dare you."},
    {"r.dontcare", WARM, ALLS, "Oh. I thought you cared. That is a bit sad."}, {"r.dontcare", WARM, ALLS, "Well, I care, {pet}. Even if you do not."},
    {"r.dontcare", NEUT, ALLS, "Then we are even."}, {"r.dontcare", NEUT, ALLS, "Fine. Do not care. Do it anyway."}, {"r.dontcare", NEUT, ALLS, "Careless. But honest."},
    {"r.dontcare", COLD, ALLS, "Good, because neither do I."}, {"r.dontcare", COLD, ALLS, "Then why are you still talking?"}, {"r.dontcare", HOST, ALLS, "Great! Because I care less."},
    {"r.dontcare", COLD, DIVINE, "You do not care? I do not care that you do not care. I win."}, {"r.dontcare", HOST, DIVINE, "A worm that does not care. How very worm of you."}, {"r.dontcare", NEUT, DIVINE, "Indifference. The one thing gods and mortals share."},
    {"r.demand", WARM, ALLS, "Ask nicely, {pet}. You will get further."}, {"r.demand", WARM, ALLS, "Slow down, I will try to help. Politely, please."},
    {"r.demand", NEUT, ALLS, "Demands, huh? Try asking."}, {"r.demand", NEUT, ALLS, "That is not how you ask, {player}."}, {"r.demand", NEUT, ALLS, "Say please."},
    {"r.demand", COLD, ALLS, "You demand? From me? Cute."}, {"r.demand", COLD, ALLS, "Tone down the orders, {pet}."}, {"r.demand", HOST, ALLS, "Do not give me orders, you fool."},
    {"r.demand", NEUT, DIVINE, "A vessel does not order a goddess. Ask properly."}, {"r.demand", COLD, DIVINE, "Demanding, mortal? I give. You do not take."}, {"r.demand", HOST, DIVINE, "Orders? From a worm? I will laugh. Then I will punish."},
    {"r.demand", WARM, DIVINE, "Such fire, little one. Ask softly and I may grant it."}, {"r.demand", COLD, STREET, "Who died and made you boss, rookie?"},
};
const int LINES_D_N = (int)(sizeof LINES_D / sizeof LINES_D[0]);
