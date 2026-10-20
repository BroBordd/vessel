/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE VOCABULARY, ranks 4501-5000 (docs/convo_plan.md, chunk 49d; this closes chunk 49, the 5,000 most common words). Rows "word|rank|pos" as in
 * lang_vocab_a.c. No names, no text-speak or dialect keys, no duplicates of ranks 1-4500, no words of 3 letters or less (they are too easy a
 * spell-repair target). Chunk 50 continues from rank 5001. Every new word is also a spell repair and split target: run the gates after any change. */
#include "lang_vocab.h"

const char *const VOCAB_A4[] = {
  "headstrong|4501|a", "icicle|4502|n", "reprimand|4503|v", "errant|4504|a", "basil|4505|n", "jangle|4506|v", "scrawny|4507|a", "bluebird|4508|n", "erupt|4509|v", "irate|4510|a", "tarn|4511|n", "garble|4512|v", "livid|4513|a",
  "thimble|4514|n", "forsake|4515|v", "morose|4516|a", "nightingale|4517|n", "exasperate|4518|v", "aghast|4519|a", "elderberry|4520|n", "exhort|4521|v", "guileless|4522|a", "saber|4523|n", "palpitate|4524|v", "amiable|4525|a", "jasmine|4526|n",
  "parry|4527|v", "edgy|4528|a", "bakery|4529|n", "fathom|4530|v", "callow|4531|a", "heron|4532|n", "peruse|4533|v", "lurid|4534|a", "jackal|4535|n", "decree|4536|v", "forlorn|4537|a", "spruce|4538|n", "sally|4539|v",
  "lucid|4540|a", "trowel|4541|n", "demean|4542|v", "urbane|4543|a", "stronghold|4544|n", "commandeer|4545|v", "oafish|4546|a", "drawbridge|4547|n", "smite|4548|v", "ethereal|4549|a", "fieldstone|4550|n", "burnish|4551|v", "grizzled|4552|a",
  "dormouse|4553|n", "dupe|4554|v", "bleary|4555|a", "cutlass|4556|n", "seethe|4557|v", "paltry|4558|a", "molasses|4559|n", "quell|4560|v", "demure|4561|a", "cinder|4562|n", "jumble|4563|v", "frugal|4564|a", "cypress|4565|n",
  "gripe|4566|v", "exuberant|4567|a", "ledger|4568|n", "reek|4569|v", "stalwart|4570|a", "thatch|4571|n", "singe|4572|v", "austere|4573|a", "bayonet|4574|n", "clamber|4575|v", "belligerent|4576|a", "rattlesnake|4577|n", "abduct|4578|v",
  "cadaverous|4579|a", "cobblestone|4580|n", "skulk|4581|v", "droll|4582|a", "buckskin|4583|n", "rant|4584|v", "nonchalant|4585|a", "tapestry|4586|n", "quibble|4587|v", "cloying|4588|a", "marigold|4589|n", "crinkle|4590|v", "crestfallen|4591|a",
  "daffodil|4592|n", "wheedle|4593|v", "svelte|4594|a", "cauldron|4595|n", "blare|4596|v", "emaciated|4597|a", "beret|4598|n", "confer|4599|v", "covert|4600|a", "hilltop|4601|n", "denote|4602|v", "sultry|4603|a", "harpoon|4604|n",
  "impugn|4605|v", "strident|4606|a", "tombstone|4607|n", "bludgeon|4608|v", "fickle|4609|a", "toadstool|4610|n", "bobble|4611|v", "derelict|4612|a", "fawn|4613|n", "yelp|4614|v", "pious|4615|a", "prow|4616|n", "batter|4617|v",
  "mottled|4618|a", "shipyard|4619|n", "bewail|4620|v", "plaintive|4621|a", "casket|4622|n", "stoke|4623|v", "luscious|4624|a", "pulpit|4625|n", "entomb|4626|v", "rancid|4627|a", "orchid|4628|n", "expound|4629|v", "glum|4630|a",
  "crypt|4631|n", "espouse|4632|v", "corpulent|4633|a", "rosemary|4634|n", "emulate|4635|v", "obstinate|4636|a", "bookshelf|4637|n", "tarry|4638|v", "elusive|4639|a", "rooftop|4640|n", "blurt|4641|v", "jubilant|4642|a", "saffron|4643|n",
  "glean|4644|v", "wizened|4645|a", "stonework|4646|n", "hobble|4647|v", "querulous|4648|a", "oriole|4649|n", "incline|4650|v", "rakish|4651|a", "locksmith|4652|n", "quaff|4653|v", "docile|4654|a", "scaffold|4655|n", "languish|4656|v",
  "copious|4657|a", "hourglass|4658|n", "feign|4659|v", "surly|4660|a", "wrought|4661|n", "rebuff|4662|v", "harried|4663|a", "wisp|4664|n", "detest|4665|v", "taciturn|4666|a", "truffle|4667|n", "slink|4668|v", "unkempt|4669|a",
  "moonlight|4670|n", "degrade|4671|v", "sable|4672|a", "pewter|4673|n", "swaddle|4674|v", "florid|4675|a", "slingshot|4676|n", "gouge|4677|v", "dour|4678|a", "twine|4679|n", "defile|4680|v", "earnest|4681|a", "limestone|4682|n",
  "sputter|4683|v", "inept|4684|a", "lichen|4685|n", "instill|4686|v", "wily|4687|a", "canary|4688|n", "toil|4689|v", "sodden|4690|a", "crevice|4691|n", "bloat|4692|v", "recluse|4693|a", "cockpit|4694|n", "grapple|4695|v",
  "fractious|4696|a", "firefly|4697|n", "muse|4698|v", "aught|4699|a", "sickle|4700|n", "intone|4701|v", "benign|4702|a", "wetland|4703|n", "straddle|4704|v", "squalid|4705|a", "mural|4706|n", "wilt|4707|v", "despondent|4708|a",
  "plover|4709|n", "evoke|4710|v", "opulent|4711|a", "bagpipe|4712|n", "harry|4713|v", "dank|4714|a", "petunia|4715|n", "ensnare|4716|v", "mulish|4717|a", "hammock|4718|n", "bustle|4719|v", "timorous|4720|a", "candlestick|4721|n",
  "recount|4722|v", "parched|4723|a", "dandelion|4724|n", "clobber|4725|v", "lanky|4726|a", "armory|4727|n", "outrun|4728|v", "hoary|4729|a", "cuff|4730|n", "brandish|4731|v", "haggard|4732|a", "sycamore|4733|n", "sidle|4734|v",
  "dingy|4735|a", "viper|4736|n", "scowl|4737|v", "hapless|4738|a", "lintel|4739|n", "snicker|4740|v", "dulcet|4741|a", "minnow|4742|n", "amble|4743|v", "crass|4744|a", "gull|4745|n", "deplore|4746|v", "astute|4747|a",
  "satchel|4748|n", "jest|4749|v", "raucous|4750|a", "vineyard|4751|n", "decant|4752|v", "spry|4753|a", "scepter|4754|n", "slacken|4755|v", "crimson|4756|a", "brooch|4757|n", "mope|4758|v", "portly|4759|a", "cedar|4760|n",
  "bolt|4761|v", "fallow|4762|a", "gorge|4763|n", "cower|4764|v", "dilapidated|4765|a", "abacus|4766|n", "gird|4767|v", "cantankerous|4768|a", "barracks|4769|n", "cleave|4770|v", "gawky|4771|a", "lilac|4772|n", "moor|4773|v",
  "pungent|4774|a", "centaur|4775|n", "topple|4776|v", "murky|4777|a", "sundial|4778|n", "tussle|4779|v", "trite|4780|a", "amulet|4781|n", "chide|4782|v", "drab|4783|a", "grotto|4784|n", "vaunt|4785|v", "fetid|4786|a",
  "gingerbread|4787|n", "heave|4788|v", "stark|4789|a", "pendant|4790|n", "trounce|4791|v", "arduous|4792|a", "lynx|4793|n", "carouse|4794|v", "erratic|4795|a", "rubble|4796|n", "emblazon|4797|v", "insolent|4798|a", "stalactite|4799|n",
  "scuttle|4800|v", "fervent|4801|a", "cider|4802|n", "entreat|4803|v", "somber|4804|a", "crossbow|4805|n", "scour|4806|v", "fastidious|4807|a", "partridge|4808|n", "nuzzle|4809|v", "ephemeral|4810|a", "jawbone|4811|n", "squirm|4812|v",
  "tepid|4813|a", "pantomime|4814|n", "encase|4815|v", "dogged|4816|a", "mantis|4817|n", "deign|4818|v", "pensive|4819|a", "helmet|4820|n", "flay|4821|v", "bedraggled|4822|a", "conch|4823|n", "tinker|4824|v", "puny|4825|a",
  "moorland|4826|n", "scavenge|4827|v", "swarthy|4828|a", "stirrup|4829|n", "cull|4830|v", "petulant|4831|a", "griffin|4832|n", "tamper|4833|v", "dawning|4834|a", "whetstone|4835|n", "rile|4836|v", "bogus|4837|a", "headstone|4838|n",
  "masquerade|4839|v", "decrepit|4840|a", "trellis|4841|n", "meander|4842|v", "sheepish|4843|a", "dumpster|4844|n", "quicken|4845|v", "sullen|4846|a", "fresco|4847|n", "exorcise|4848|v", "verdant|4849|a", "ballroom|4850|n", "imply|4851|v",
  "blithe|4852|a", "hickory|4853|n", "debase|4854|v", "wanton|4855|a", "trinket|4856|n", "chafe|4857|v", "dapper|4858|a", "pavilion|4859|n", "hurtle|4860|v", "capricious|4861|a", "camphor|4862|n", "douse|4863|v", "buoyant|4864|a",
  "gravestone|4865|n", "despise|4866|v", "genial|4867|a", "antelope|4868|n", "confide|4869|v", "homely|4870|a", "cardboard|4871|n", "inscribe|4872|v", "staid|4873|a", "lavender|4874|n", "browbeat|4875|v", "comely|4876|a", "paintbrush|4877|n",
  "subdue|4878|v", "fidgety|4879|a", "iris|4880|n", "prod|4881|v", "gilded|4882|a", "knapsack|4883|n", "prattle|4884|v", "complacent|4885|a", "goblet|4886|n", "bleach|4887|v", "sanguine|4888|a", "cavalry|4889|n", "pare|4890|v",
  "seething|4891|a", "tinderbox|4892|n", "devise|4893|v", "shifty|4894|a", "cactus|4895|n", "dabble|4896|v", "unruly|4897|a", "lagoon|4898|n", "inundate|4899|v", "lethargic|4900|a", "hogshead|4901|n", "curtsy|4902|v", "baleful|4903|a",
  "catfish|4904|n", "blanch|4905|v", "perky|4906|a", "puffin|4907|n", "barter|4908|v", "mistletoe|4909|n", "cajole|4910|v", "stagecoach|4911|n", "incite|4912|v", "brewery|4913|n", "rummage|4914|v", "osprey|4915|n", "gallop|4916|v",
  "saddlebag|4917|n", "enfold|4918|v", "aqueduct|4919|n", "wallop|4920|v", "breadcrumb|4921|n", "goad|4922|v", "ferret|4923|n", "chasten|4924|v", "weevil|4925|n", "cudgel|4926|v", "cormorant|4927|n", "fritter|4928|v", "horseshoe|4929|n",
  "pomp|4930|v", "sedan|4931|n", "brood|4932|v", "cashmere|4933|n", "snuff|4934|v", "seagull|4935|n", "pilfer|4936|v", "honeycomb|4937|n", "saunter|4938|v", "gauntlet|4939|n", "foxglove|4940|n", "bassoon|4941|n", "pitchfork|4942|n",
  "shutter|4943|n", "vestibule|4944|n", "sparrowhawk|4945|n", "anchovy|4946|n", "puddle|4947|n", "gargoyle|4948|n", "periwinkle|4949|n", "bungalow|4950|n", "gristle|4951|n", "beehive|4952|n", "spindle|4953|n", "archway|4954|n", "mulberry|4955|n",
  "gladiator|4956|n", "seashell|4957|n", "outpost|4958|n", "cottonwood|4959|n", "ashtray|4960|n", "radish|4961|n", "firewood|4962|n", "reed|4963|n", "farmhouse|4964|n", "sidewalk|4965|n", "porridge|4966|n", "rapier|4967|n", "hearth|4968|n",
  "magpie|4969|n", "marionette|4970|n", "finch|4971|n", "signpost|4972|n", "hawthorn|4973|n", "windmill|4974|n", "chestnut|4975|n", "homestead|4976|n", "obelisk|4977|n", "vagabond|4978|n", "lighthouse|4979|n", "nettle|4980|n", "banjo|4981|n",
  "decanter|4982|n", "jetty|4983|n", "larch|4984|n", "woodshed|4985|n", "bookcase|4986|n", "reindeer|4987|n", "weathervane|4988|n", "kestrel|4989|n", "compost|4990|n", "talisman|4991|n", "footstool|4992|n", "clover|4993|n", "minstrel|4994|n",
  "lullaby|4995|n", "silo|4996|n", "hazelnut|4997|n", "greenhouse|4998|n", "turret|4999|n", "alcove|5000|n",
  0 };
