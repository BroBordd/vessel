/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * OTHER LANGUAGES, THE COMMON WORDS (docs/convo_plan.md, chunk 61). ten tongues, one array each, rows "pattern|intent|weight" like lang_pats_g15.c. every row maps to
 * the SAME intent the English phrase has (hola is a greet), and the matcher sets Heard.foreign to the tongue so a reply may remark on it. the intents: greet, bye,
 * thanks, yes, no, love, insult, sorry, howru, whoru (what is your name), confused (i do not understand).
 * rules: Latin letters only and no accents (the clean-up layer folds them: e acute -> e, ss for the sharp s, u umlaut -> u); Russian, Arabic, Japanese and Hindi are
 * written the way a player types them on a keyboard. a word that is also English (no, net, con, sin, la, me, do) is never a row on its own; a short ambiguous word
 * only appears inside a longer phrase or at the end of a sentence ($). insults stay mild (fool, stupid, ugly): no slur, no profanity here. a row must START with a
 * plain word, not a [a|b] group (the first-word index cannot reach such a row reliably: chunk 54b). */
#include "lang_data.h"

const char *const PATROWS_ES[] = {
    "hola|greet|6", "holi|greet|6", "buenas|greet|6", "buenos dias|greet|6", "buenas tardes|greet|6", "buenas noches|greet|6", "que tal|howru|6",
    "como estas|howru|6", "como esta usted|howru|6", "como te va|howru|6", "como andas|howru|6", "que haces|greet|3", "adios|bye|6", "hasta luego|bye|6", "hasta la vista|bye|6",
    "hasta manana|bye|6", "hasta pronto|bye|6", "nos vemos|bye|6", "chao|bye|6", "gracias|thanks|6", "muchas gracias|thanks|6", "mil gracias|thanks|6", "te lo agradezco|thanks|6",
    "si por favor|yes|6", "si claro|yes|6", "claro que si|yes|6", "por supuesto|yes|6", "si senor|yes|6", "si gracias|yes|6", "si si|yes|6",
    "nunca|no|4", "para nada|no|5", "de ninguna manera|no|5", "claro que no|no|6", "no gracias|no|6", "te amo|love|6", "te quiero|love|6", "te adoro|love|6",
    "eres tonto|insult|6", "eres tonta|insult|6", "eres estupido|insult|6", "eres estupida|insult|6", "eres idiota|insult|6", "idiota|insult|5", "estupido|insult|5", "estupida|insult|5",
    "tonto|insult|4", "eres feo|insult|6", "eres fea|insult|6", "que tonto|insult|6", "lo siento|sorry|6", "perdon|sorry|6", "perdoname|sorry|6", "disculpa|sorry|6",
    "disculpame|sorry|6", "cual es tu nombre|whoru|6", "como te llamas|whoru|6", "como se llama usted|whoru|6", "quien eres|whoru|6", "quien eres tu|whoru|6", "no entiendo|confused|6", "no comprendo|confused|6",
    "no te entiendo|confused|6", "no entendi|confused|6", "estoy confundido|confused|6", "estoy confundida|confused|6", "que quieres decir|confused|5", "como asi|confused|4",
    0 };

const char *const PATROWS_FR[] = {
    "bonjour|greet|6", "bonsoir|greet|6", "salut|greet|6", "coucou|greet|6", "allo|greet|4", "bonne nuit|bye|5", "bonne journee|bye|5", "bonne soiree|bye|5",
    "ca va|howru|6", "comment ca va|howru|6", "comment vas tu|howru|6", "comment allez vous|howru|6", "comment tu vas|howru|6", "tu vas bien|howru|6", "au revoir|bye|6", "a bientot|bye|6",
    "a demain|bye|6", "a plus tard|bye|6", "a plus|bye|4", "adieu|bye|6", "merci|thanks|6", "merci beaucoup|thanks|6", "merci bien|thanks|6", "mille mercis|thanks|6",
    "oui|yes|6", "oui bien sur|yes|6", "bien sur|yes|6", "d'accord|yes|5", "daccord|yes|5", "non merci|no|6", "pas du tout|no|6", "certainement pas|no|6",
    "jamais|no|3", "non non|no|6", "je t'aime|love|6", "je taime|love|6", "je t aime|love|6", "je t'adore|love|6", "je tadore|love|6", "tu es stupide|insult|6",
    "tu es bete|insult|6", "tu es idiot|insult|6", "tu es idiote|insult|6", "tu es moche|insult|6", "tu es nul|insult|6", "tu es nulle|insult|6", "stupide|insult|4",
    "tu es laid|insult|6", "tu es laide|insult|6", "desole|sorry|6", "desolee|sorry|6", "excusez moi|sorry|5", "je suis desole|sorry|6", "je suis desolee|sorry|6",
    "pardonne moi|sorry|6", "comment tu t'appelles|whoru|6", "comment tu t appelles|whoru|6", "comment t'appelles tu|whoru|6", "comment vous appelez vous|whoru|6", "tu t'appelles comment|whoru|6", "quel est ton nom|whoru|6", "qui es tu|whoru|6",
    "je ne comprends pas|confused|6", "je comprends pas|confused|6", "je n'ai pas compris|confused|6", "j'ai pas compris|confused|6", "je suis perdu|confused|5", "je suis perdue|confused|5", "tu veux dire quoi|confused|5",
    0 };

const char *const PATROWS_DE[] = {
    "hallo|greet|6", "guten morgen|greet|6", "guten tag|greet|6", "guten abend|greet|6", "moin|greet|5", "servus|greet|5", "gute nacht|bye|5", "wie geht's|howru|6",
    "wie gehts|howru|6", "wie geht es dir|howru|6", "wie geht es ihnen|howru|6", "wie gehts dir|howru|6", "alles gut|howru|3", "auf wiedersehen|bye|6", "tschuss|bye|6", "tschuess|bye|6",
    "bis bald|bye|6", "bis spater|bye|6", "bis morgen|bye|6", "danke|thanks|6", "danke schon|thanks|6", "danke sehr|thanks|6", "vielen dank|thanks|6", "dankeschon|thanks|6",
    "ja bitte|yes|6", "ja klar|yes|6", "ja danke|yes|6", "ja gerne|yes|6", "naturlich|yes|5", "natuerlich|yes|5", "genau|yes|3", "nein|no|6",
    "nein danke|no|6", "auf keinen fall|no|6", "niemals|no|5", "ich liebe dich|love|6", "ich hab dich lieb|love|6", "ich habe dich lieb|love|6", "ich mag dich|love|4", "du bist dumm|insult|6",
    "du bist blod|insult|6", "du bist bloed|insult|6", "du bist haesslich|insult|6", "du bist hasslich|insult|6", "dummkopf|insult|6", "du idiot|insult|6", "du bist ein idiot|insult|6", "blodmann|insult|5",
    "es tut mir leid|sorry|6", "tut mir leid|sorry|6", "entschuldigung|sorry|6", "entschuldige|sorry|6", "verzeihung|sorry|6", "wie heisst du|whoru|6", "wie heissen sie|whoru|6",
    "wer bist du|whoru|6", "ich verstehe nicht|confused|6", "ich versteh nicht|confused|6", "ich verstehe das nicht|confused|6", "ich verstehe dich nicht|confused|6", "ich bin verwirrt|confused|6", "was meinst du|confused|4",
    0 };

const char *const PATROWS_PT[] = {
    "bom dia|greet|6", "boa tarde|greet|6", "boa noite|greet|5", "ola|greet|6", "oi tudo bem|greet|6", "oi gente|greet|6", "e ai|greet|4", "tudo bem|howru|6",
    "como vai|howru|6", "como vai voce|howru|6", "como voce esta|howru|6", "como esta|howru|5", "como estas|howru|5", "tchau|bye|6", "ate logo|bye|6", "ate amanha|bye|6",
    "ate mais|bye|6", "ate breve|bye|6", "adeus|bye|6", "falou|bye|3", "obrigado|thanks|6", "obrigada|thanks|6", "muito obrigado|thanks|6", "muito obrigada|thanks|6",
    "valeu|thanks|5", "sim claro|yes|6", "claro que sim|yes|6", "sim sim|yes|6", "com certeza|yes|6", "sim obrigado|yes|6", "pois sim|yes|4", "nao|no|6",
    "nao obrigado|no|6", "de jeito nenhum|no|6", "jamais|no|3", "claro que nao|no|6", "nunca|no|3", "eu te amo|love|6", "te amo|love|6", "amo voce|love|6",
    "amo te|love|5", "eu gosto de voce|love|4", "voce e burro|insult|6", "voce e burra|insult|6", "voce e idiota|insult|6", "voce e feio|insult|6", "voce e feia|insult|6", "voce e estupido|insult|6",
    "burro|insult|4", "burra|insult|4", "palhaco|insult|4", "desculpa|sorry|6", "desculpe|sorry|6", "me desculpe|sorry|6", "me perdoa|sorry|6", "perdao|sorry|6",
    "sinto muito|sorry|6", "eu sinto muito|sorry|6", "como voce se chama|whoru|6", "como te chamas|whoru|6", "qual e o seu nome|whoru|6", "qual e o teu nome|whoru|6", "quem e voce|whoru|6", "quem es tu|whoru|6",
    "eu nao entendo|confused|6", "nao entendo|confused|6", "nao entendi|confused|6", "eu nao entendi|confused|6", "nao te entendo|confused|6", "estou confuso|confused|6", "estou confusa|confused|6", "o que voce quer dizer|confused|5",
    0 };

const char *const PATROWS_IT[] = {
    "ciao a tutti|greet|6", "salve|greet|5", "buongiorno|greet|6", "buonasera|greet|6", "buonanotte|bye|5", "buon pomeriggio|greet|5", "come stai|howru|6",
    "come va|howru|6", "come sta|howru|6", "come state|howru|6", "come te la passi|howru|6", "tutto bene|howru|4", "arrivederci|bye|6", "arrivederla|bye|6", "a presto|bye|6",
    "a dopo|bye|6", "a domani|bye|6", "ci vediamo|bye|6", "addio|bye|6", "ciao ciao|bye|6", "grazie|thanks|6", "grazie mille|thanks|6", "molte grazie|thanks|6",
    "mille grazie|thanks|6", "ti ringrazio|thanks|6", "si grazie|yes|6", "si certo|yes|6", "certo|yes|4", "ma certo|yes|6", "certamente|yes|6", "assolutamente si|yes|6",
    "no grazie|no|6", "assolutamente no|no|6", "per niente|no|6", "neanche per sogno|no|6", "ti amo|love|6", "ti voglio bene|love|6", "ti adoro|love|6",
    "sei stupido|insult|6", "sei stupida|insult|6", "sei scemo|insult|6", "sei scema|insult|6", "sei brutto|insult|6", "sei brutta|insult|6", "sei cretino|insult|6", "sei un idiota|insult|6",
    "scemo|insult|4", "cretino|insult|4", "stupido|insult|4", "scusa|sorry|6", "scusami|sorry|6", "scusi|sorry|6", "mi dispiace|sorry|6", "perdonami|sorry|6",
    "come ti chiami|whoru|6", "come si chiama|whoru|6", "come ti chiamano|whoru|6", "qual e il tuo nome|whoru|6", "chi sei|whoru|6", "chi sei tu|whoru|6", "non capisco|confused|6", "non ho capito|confused|6",
    "non ti capisco|confused|6", "non ci capisco niente|confused|6", "sono confuso|confused|6", "sono confusa|confused|6", "che vuoi dire|confused|5", "cosa intendi|confused|5",
    0 };

const char *const PATROWS_TR[] = {
    "merhaba|greet|6", "selam|greet|6", "gunaydin|greet|6", "iyi gunler|greet|6", "iyi aksamlar|greet|6", "iyi geceler|bye|5", "nasilsin|howru|6", "nasilsiniz|howru|6",
    "naber|howru|6", "ne haber|howru|6", "iyi misin|howru|6", "nasil gidiyor|howru|6", "ne var ne yok|howru|5", "hosca kal|bye|6", "hoscakal|bye|6", "gule gule|bye|6",
    "gorusuruz|bye|6", "gorusmek uzere|bye|6", "yarin gorusuruz|bye|6", "tesekkurler|thanks|6", "tesekkur ederim|thanks|6", "cok tesekkurler|thanks|6", "sagol|thanks|6", "sag ol|thanks|6",
    "eyvallah|thanks|5", "evet|yes|6", "evet lutfen|yes|6", "tabii|yes|6", "tabi ki|yes|6", "elbette|yes|6", "olur|yes|5", "hayir|no|6",
    "hayir tesekkurler|no|6", "kesinlikle hayir|no|6", "asla|no|5", "yok|no|3", "seni seviyorum|love|6", "seni cok seviyorum|love|6", "askim|love|5", "canim|love|3",
    "aptal|insult|5", "salak|insult|5", "sen aptalsin|insult|6", "sen salaksin|insult|6", "sen cirkinsin|insult|6", "gerizekali|insult|6", "ozur dilerim|sorry|6", "uzgunum|sorry|6",
    "kusura bakma|sorry|6", "affedersin|sorry|6", "affedersiniz|sorry|6", "adin ne|whoru|6", "adiniz ne|whoru|6", "ismin ne|whoru|6", "ismin nedir|whoru|6",
    "sen kimsin|whoru|6", "anlamiyorum|confused|6", "anlamadim|confused|6", "anlayamadim|confused|6", "kafam karisti|confused|6", "ne demek istiyorsun|confused|5", "tamam|ok|4",
    0 };

const char *const PATROWS_RU[] = {
    "privet|greet|6", "privetik|greet|6", "zdravstvuyte|greet|6", "zdravstvuy|greet|6", "zdrastvuyte|greet|6", "dobryy den|greet|6", "dobryi den|greet|6", "dobroe utro|greet|6",
    "dobryy vecher|greet|6", "dobryi vecher|greet|6", "spokoynoy nochi|bye|5", "spokoinoi nochi|bye|5", "kak dela|howru|6", "kak ty|howru|5", "kak pozhivaesh|howru|6", "kak zhizn|howru|5",
    "kak u tebya dela|howru|6", "kak vy pozhivaete|howru|6", "poka poka|bye|6", "do svidaniya|bye|6", "do svidanya|bye|6", "do vstrechi|bye|6", "do zavtra|bye|6", "proshchay|bye|5",
    "vsego dobrogo|bye|6", "spasibo|thanks|6", "bolshoe spasibo|thanks|6", "spasibo bolshoe|thanks|6", "blagodaryu|thanks|6", "da konechno|yes|6", "da da|yes|5", "da horosho|yes|6",
    "konechno|yes|5", "da$|yes|2", "nyet|no|6", "niet|no|5", "nikogda|no|5", "ni za chto|no|6", "ya tebya lyublyu|love|6", "ya lyublyu tebya|love|6",
    "ya tebya lublu|love|6", "lyublyu tebya|love|6", "ty durak|insult|6", "ty dura|insult|6", "durak|insult|5", "ty glupyy|insult|6", "ty glupyi|insult|6", "ty urod|insult|6",
    "ty tupoy|insult|6", "izvini|sorry|6", "izvinite|sorry|6", "prosti menya|sorry|6", "proshu proshcheniya|sorry|6", "mne zhal|sorry|6", "kak tebya zovut|whoru|6", "kak vas zovut|whoru|6",
    "kak tvoyo imya|whoru|6", "kto ty|whoru|6", "ya ne ponimayu|confused|6", "ya ne ponyal|confused|6", "ya ne ponyala|confused|6", "ne ponimayu|confused|5", "ya zaputalsya|confused|6", "chto ty hochesh skazat|confused|5",
    0 };

const char *const PATROWS_AR[] = {
    "salam|greet|6", "salaam|greet|6", "as salamu alaykum|greet|6", "assalamu alaikum|greet|6", "salam alaykum|greet|6", "marhaba|greet|6", "marhaban|greet|6", "ahlan|greet|6",
    "ahlan wa sahlan|greet|6", "ahlan wasahlan|greet|6", "sabah al khair|greet|6", "sabah alkhir|greet|6", "masa al khair|greet|6", "masa alkhir|greet|6", "tisbah ala khair|bye|5", "kayf halak|howru|6",
    "kayf halik|howru|6", "kif halak|howru|6", "kif halek|howru|6", "kifak|howru|6", "shlonak|howru|6", "shlonik|howru|6", "shu akhbarak|howru|6", "ma akhbarak|howru|6",
    "ezayak|howru|6", "izzayak|howru|6", "ma salama|bye|6", "ma'a salama|bye|6", "maa salama|bye|6", "ila al liqa|bye|6", "shukran|thanks|6", "shukran jazilan|thanks|6",
    "shokran|thanks|6", "jazak allah khair|thanks|6", "naam|yes|6", "na'am|yes|6", "aywa|yes|6", "aiwa|yes|6", "bi kul tasrur|yes|6", "la shukran|no|6",
    "abadan|no|5", "mish momken|no|5", "ana uhibbuk|love|6", "ana ahebbak|love|6", "ana bahebak|love|6", "bahebak|love|6", "uhibbuk|love|6", "ya kalb|insult|6",
    "ya homar|insult|6", "ya hmar|insult|6", "anta ghabi|insult|6", "anti ghabiya|insult|6", "ya ghabi|insult|6", "ana asif|sorry|6", "ana asfa|sorry|6", "ana aasef|sorry|6",
    "ma'zira|sorry|6", "ismak eh|whoru|6", "ismak ay|whoru|6", "ma ismak|whoru|6", "ma ismuka|whoru|6", "shu ismak|whoru|6", "min anta|whoru|6", "ana la afham|confused|6",
    "ana mish fahem|confused|6", "ana mish fahma|confused|6", "mish fahem|confused|5", "la afham|confused|6", "ma fahimt|confused|6", "tamam|ok|4",
    0 };

const char *const PATROWS_JA[] = {
    "konnichiwa|greet|6", "konnichi wa|greet|6", "konnichiha|greet|6", "konbanwa|greet|6", "konban wa|greet|6", "ohayou|greet|6", "ohayo|greet|6", "ohayou gozaimasu|greet|6",
    "ohayo gozaimasu|greet|6", "moshi moshi|greet|6", "oyasumi|bye|5", "oyasuminasai|bye|5", "ogenki desu ka|howru|6", "o genki desu ka|howru|6", "genki desu ka|howru|6", "ikaga desu ka|howru|6",
    "sayounara|bye|6", "sayonara|bye|6", "jaa ne|bye|6", "ja ne|bye|6", "mata ne|bye|6", "mata ashita|bye|6", "dewa mata|bye|6", "arigatou|thanks|6",
    "arigato|thanks|6", "arigatou gozaimasu|thanks|6", "arigato gozaimasu|thanks|6", "doumo arigatou|thanks|6", "domo arigato|thanks|6", "domo|thanks|3", "hai wakarimashita|yes|6", "hai hai|yes|5", "hai arigatou|yes|5",
    "hai sou desu|yes|6", "sou desu|yes|5", "iie|no|6", "iya|no|5", "chigau|no|5", "chigaimasu|no|6", "dame desu|no|5", "daisuki|love|6",
    "daisuki desu|love|6", "aishiteru|love|6", "aishiteru yo|love|6", "suki desu|love|6", "anata ga suki|love|6", "baka|insult|6", "bakayaro|insult|6", "baka yarou|insult|6",
    "aho|insult|4", "anata wa baka|insult|6", "omae wa baka|insult|6", "kimi wa baka|insult|6", "gomen|sorry|6", "gomen ne|sorry|6", "gomennasai|sorry|6", "gomen nasai|sorry|6",
    "sumimasen|sorry|6", "moushiwake arimasen|sorry|6", "shitsurei shimashita|sorry|6", "onamae wa|whoru|6", "o namae wa|whoru|6", "onamae wa nandesuka|whoru|6", "o namae wa nan desu ka|whoru|6", "anata wa dare|whoru|6",
    "wakarimasen|confused|6", "wakaranai|confused|6", "wakannai|confused|6", "wakarimasen deshita|confused|6",
    0 };

const char *const PATROWS_HI[] = {
    "namaste|greet|6", "namaskar|greet|6", "pranam|greet|5", "ram ram|greet|5", "sat sri akal|greet|6", "shubh ratri|bye|5", "shubh raatri|bye|5", "kaise ho|howru|6",
    "kaise hain aap|howru|6", "aap kaise hain|howru|6", "tum kaise ho|howru|6", "kya haal hai|howru|6", "kya hal hai|howru|6", "kya haal|howru|5", "aur batao|howru|4", "kaisa chal raha hai|howru|5",
    "alvida|bye|6", "phir milenge|bye|6", "phir milte hain|bye|6", "kal milte hain|bye|6", "khuda hafiz|bye|6", "shukriya|thanks|6", "bahut shukriya|thanks|6", "dhanyavaad|thanks|6",
    "dhanyawad|thanks|6", "bahut dhanyavaad|thanks|6", "haan ji|yes|6", "ji haan|yes|6", "haan|yes|5", "bilkul|yes|5", "zaroor|yes|5", "nahi ji|no|6",
    "nahin|no|5", "bilkul nahi|no|6", "kabhi nahi|no|5", "nahi|no|6", "main tumse pyaar karta hoon|love|6", "main tumse pyar karta hu|love|6", "main tumse pyaar karti hoon|love|6", "main tumse pyar karti hu|love|6",
    "mujhe tumse pyaar hai|love|6", "mujhe tumse pyar hai|love|6", "tumse pyaar|love|5", "tumse pyar|love|5", "tum pagal ho|insult|6", "tum bevakoof ho|insult|6", "tum bewakoof ho|insult|6", "tum gadhe ho|insult|6",
    "pagal|insult|4", "bevakoof|insult|5", "bewakoof|insult|5", "gadha|insult|4", "tum badsurat ho|insult|6", "maaf karo|sorry|6", "maaf kijiye|sorry|6", "mujhe maaf karo|sorry|6",
    "mujhe maaf kar do|sorry|6", "maaf kar do|sorry|6", "mujhe afsos hai|sorry|6", "tumhara naam kya hai|whoru|6", "tumhara naam kya|whoru|6", "aapka naam kya hai|whoru|6", "tera naam kya hai|whoru|6", "tum kaun ho|whoru|6",
    "aap kaun hain|whoru|6", "mujhe samajh nahi aaya|confused|6", "mujhe samajh nahin aaya|confused|6", "samajh nahi aaya|confused|6", "main samajh nahi paya|confused|6", "main confuse hoon|confused|6", "tumhara kya matlab hai|confused|5",
    "theek hai|ok|4",
    0 };
