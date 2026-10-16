/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * KNOWN FIRST NAMES, a starter list of 374 (docs/convo_plan.md, chunk 20; chunk 51 grows it to >= 1,500 from many cultures).
 * Rows are the lower case name (letters only). A name is never "corrected" by the spell repair, and a name is not a vocabulary word:
 * vocab_is_name(w) is the question (lang_vocab.h). */
#include "lang_vocab.h"

const char *const NAMES_A[] = {
  "dea", "alex", "mia", "noah", "liam", "emma", "olivia", "ava", "sophia", "isabella", "amelia", "harper", "evelyn", "abigail", "emily", "ella",
  "elizabeth", "sofia", "avery", "scarlett", "grace", "chloe", "victoria", "riley", "aria", "lily", "aurora", "zoey", "nora", "hazel", "luna",
  "violet", "stella", "ellie", "lucy", "anna", "maya", "claire", "samantha", "leah", "nova", "james", "john", "robert", "michael", "william",
  "david", "richard", "joseph", "thomas", "charles", "daniel", "matthew", "anthony", "mark", "donald", "steven", "paul", "andrew", "joshua",
  "kenneth", "kevin", "brian", "george", "timothy", "ronald", "edward", "jason", "jeffrey", "ryan", "jacob", "gary", "nicholas", "eric", "jonathan",
  "stephen", "larry", "justin", "scott", "brandon", "benjamin", "samuel", "frank", "gregory", "raymond", "patrick", "jack", "dennis", "jerry",
  "tyler", "aaron", "henry", "adam", "nathan", "peter", "zachary", "kyle", "walter", "harold", "ethan", "jeremy", "lucas", "oliver", "elijah",
  "logan", "mary", "patricia", "jennifer", "linda", "barbara", "susan", "jessica", "sarah", "karen", "nancy", "lisa", "betty", "margaret", "sandra",
  "ashley", "kimberly", "donna", "michelle", "carol", "amanda", "melissa", "deborah", "stephanie", "rebecca", "sharon", "laura", "cynthia",
  "kathleen", "amy", "angela", "shirley", "brenda", "pamela", "nicole", "katherine", "helen", "rachel", "carolyn", "janet", "catherine", "maria",
  "heather", "diane", "julie", "joyce", "virginia", "kelly", "christina", "joan", "judith", "megan", "cheryl", "andrea", "hannah", "jacqueline",
  "martha", "gloria", "teresa", "sara", "janice", "julia", "marie", "madison", "kathryn", "mohammed", "muhammad", "ahmed", "ali", "omar", "hassan",
  "hussein", "ibrahim", "yusuf", "khalid", "tariq", "karim", "amir", "fatima", "aisha", "zainab", "layla", "mariam", "noor", "huda", "salma",
  "yasmin", "leila", "nadia", "samir", "wei", "li", "zhang", "wang", "chen", "yang", "liu", "lin", "xiao", "ming", "jun", "hui", "yan", "mei", "lan",
  "jing", "hua", "chang", "raj", "amit", "rahul", "vijay", "arjun", "rohan", "priya", "anita", "sunita", "deepa", "kavita", "pooja", "anjali",
  "ravi", "sanjay", "anil", "sunil", "ashok", "hiroshi", "takeshi", "kenji", "yuki", "sakura", "akira", "haruto", "yuto", "sora", "hana", "rin",
  "ren", "kaito", "satoshi", "kazuki", "juan", "carlos", "jose", "luis", "miguel", "pedro", "diego", "pablo", "javier", "sergio", "rafael", "mateo",
  "santiago", "sebastian", "alejandro", "fernando", "carmen", "rosa", "lucia", "elena", "paula", "isabel", "marta", "valeria", "camila", "daniela",
  "ivan", "dmitri", "sergei", "alexei", "vladimir", "nikolai", "boris", "oleg", "natasha", "olga", "tatiana", "svetlana", "anastasia", "katya",
  "irina", "masha", "pierre", "jean", "louis", "francois", "henri", "antoine", "luc", "marc", "hugo", "camille", "juliette", "manon", "ines", "hans",
  "klaus", "lukas", "felix", "jonas", "leon", "finn", "max", "anja", "greta", "heidi", "ingrid", "sven", "lars", "bjorn", "giovanni", "marco",
  "luca", "matteo", "paolo", "giulia", "chiara", "francesca", "kwame", "kofi", "amara", "chidi", "emeka", "ngozi", "abena", "thandi", "sipho", "sam",
  "tom", "ben", "joe", "jim", "bob", "dan", "ted", "ron", "don", "ken", "ray", "roy", "sid", "ian", "lee", "kim", "jay", "jo", "al", "ed", "pat",
  "chris", "jordan", "taylor", "jamie", "casey", "morgan", "quinn", "robin", "sky", "ash", "bella", "charlie", "lola", "milo", "oscar", "teddy",
  "rocky", "buddy", "daisy", "molly",
  0 };
