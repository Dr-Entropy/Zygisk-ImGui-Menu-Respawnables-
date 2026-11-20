// gg_features.h - generated from the Respawnables GG script (all values kept as in the script)
#pragma once
#include "gg_engine.h"

static std::vector<gg::Feature> BuildFeatures() {
    std::vector<gg::Feature> v;
    v.push_back({1, "4 Hacks", "Combat", "", 0, {{gg::ALLOC, gg::DWORD, "981;982;983;986", 600, "20", gg::DWORD}}});
    v.push_back({2, "FOV 360°", "Combat", "", 0, {{gg::ALLOC, gg::FLOAT, "0.99619472027", 1000, "-3.4e+38", gg::FLOAT}}});
    v.push_back({3, "Rocket map Hack", "Spawns", "Go to the rocket, enable, jump to be above", 0, {{gg::ALLOC, gg::FLOAT, "5", 1000, "8", gg::FLOAT}}});
    v.push_back({4, "Map Hack", "Spawns", "", 0, {{gg::ALLOC, gg::BYTE, ":01_arab_town", 1500, ":12_west_side_district", gg::BYTE}}});
    v.push_back({5, "+ Time", "Time", "", 0, {{gg::CODE_APP, gg::DWORD, "150", 300, "300", gg::DWORD}}});
    v.push_back({6, "Perfect Aimbot", "Combat", "", 0, {{gg::ALLOC, gg::FLOAT, "0.99619472027;0.96592581272;0.17453293502", 10, "-3.4e-38;-3.4e38;3.4e38", gg::FLOAT}}});
    v.push_back({7, "Nuclear Hack", "Combat", "", 0, {{gg::CODE_APP, gg::FLOAT, "9.99999997e-7", 1500, "-3.4e+38", gg::FLOAT}}});
    v.push_back({8, "WallHack", "Combat", "", 0, {{gg::CODE_APP, gg::FLOAT, "0.00001", 2000, "999999999", gg::FLOAT}}});
    v.push_back({9, "1 Hit Kill", "Combat", "", 0, {{gg::DATA, gg::QWORD, "335007449165", 1, "10", gg::QWORD}}});
    v.push_back({10, "Health Hack", "Combat", "Equip all health clothes first", 0, {{gg::CODE_APP, gg::FLOAT, "100", 1000, "0.0000000000000001", gg::FLOAT}}});
    v.push_back({11, "Inf Trap Time", "Spawns", "", 0, {{gg::ALLOC, gg::BYTE, ":_out", 1000, "0", gg::BYTE}}});
    v.push_back({12, "No Bots Moving", "Spawns", "Enable before the loading screen", 0, {{gg::CODE_APP, gg::BYTE, ":launcher", 1500, "0", gg::BYTE}}});
    v.push_back({13, "Flare Gun Hack", "Spawns", "Equip Flare Gun first", 0, {{gg::ALLOC, gg::BYTE, ":flare_gun_projectile", 200, ":wind_trap_01_open", gg::BYTE}}});
    v.push_back({14, "Change Ammo Hack 2", "Spawns", "Equip Energy Stinger first", 0, {{gg::ALLOC, gg::BYTE, ":rocket_launcher_energy_projectile", 10000, ":noisy_cricket_projectile", gg::BYTE}}});
    v.push_back({15, "Fall Pose", "Spawns", "", 0, {{gg::ALLOC, gg::BYTE, ":fall", 200, "0", gg::BYTE}}});
    v.push_back({16, "Sleep Pose", "Spawns", "", 0, {{gg::ALLOC, gg::BYTE, ":sleep", 200, "0", gg::BYTE}}});
    v.push_back({17, "Gold Spawn", "Spawns", "", 0, {{gg::ALLOC, gg::BYTE, ":dogtag_enemy", 1000, ":loot_gold", gg::BYTE}}});
    v.push_back({18, "Extended HitBox", "Combat", "Must be in a match", 0, {{gg::ALLOC, gg::FLOAT, "0.08", 500, "100", gg::FLOAT}}});
    v.push_back({19, "No Gadget Cooldown", "Time", "", 0, {{gg::ALLOC, gg::DWORD, "1819242339;1853321060", 10000, "-100", gg::DWORD}}});
    v.push_back({20, "Change Ammo Hack 3", "Spawns", "Equip Little Friend first", 0, {{gg::ALLOC, gg::BYTE, ":assault_trombone_01_projectile", 1500, ":flare_gun_projectile", gg::BYTE}}});
    v.push_back({21, "2 Hacks", "Combat", "Immortal with any outfit; fire rate with Raid Hulk armor", 0, {{gg::ALLOC, gg::BYTE, ":wait", 1500, "0", gg::BYTE}}});
    v.push_back({22, "Twister Spawn", "Spawns", "Equip Twister gadget first", 0, {{gg::ALLOC, gg::BYTE, ":dogtag_enemy", 2000, ":wind_trap_01", gg::BYTE}}});
    v.push_back({23, "Death Pose", "Spawns", "", 0, {{gg::ALLOC, gg::BYTE, ":death", 1000, "0", gg::BYTE}}});
    v.push_back({24, "Cant Be Stunned", "Combat", "", 0, {{gg::ALLOC, gg::BYTE, ":slow", 1500, "0", gg::BYTE}}});
    v.push_back({25, "Inf Range Hack", "Time", "", 0, {{gg::DATA, gg::QWORD, "236223201334", 1, "10", gg::QWORD}}});
    v.push_back({26, "Inf Spotter Time", "Time", "", 0, {{gg::DATA, gg::QWORD, "283467841601", 1, "10", gg::QWORD}}});
    v.push_back({27, "Extended HitBox V4", "Combat", "", 0, {{gg::DATA, gg::QWORD, "240518168631", 1, "10", gg::QWORD}}});
    v.push_back({28, "Inf Stun Hack", "Combat", "", 0, {{gg::DATA, gg::QWORD, "296352743492", 1, "10", gg::QWORD}}});
    v.push_back({29, "Perfect Immortal", "Combat", "", 0, {{gg::DATA, gg::QWORD, "300647710789", 1, "10", gg::QWORD}}});
    v.push_back({30, "FPS Increase", "Time", "", 0, {{gg::DATA, gg::DWORD, "257~260", 13, "0", gg::DWORD}}});
    v.push_back({31, "Inf Speed Boost", "Time", "Equip Speed Boost items and use Medikit", 0, {{gg::CODE_APP, gg::QWORD, "-1644574793607282688", 1, "-1644574793858140227", gg::QWORD}}});
    v.push_back({32, "Inf Ammo Hack", "Combat", "", 0, {{gg::DATA, gg::DWORD, "9", 20, "10", gg::DWORD}}});
    v.push_back({33, "Fire Rate Hack", "Combat", "", 0, {{gg::DATA, gg::DWORD, "8", 40, "10", gg::DWORD}}});
    v.push_back({34, "No Reload", "Combat", "", 0, {{gg::DATA, gg::DWORD, "168", 2, "0", gg::DWORD}}});
    v.push_back({35, "Insta Respawn", "Spawns", "", 0, {{gg::DATA, gg::DWORD, "154", 2, "0", gg::DWORD}}});
    v.push_back({36, "No Shield Spawn", "Spawns", "", 0, {{gg::ALLOC, gg::DWORD, "5000", 30, "0", gg::DWORD}}});
    v.push_back({37, "BOSS Update", "Spawns", "", 0, {{gg::CODE_APP, gg::QWORD, "-1900799589116135491", 1, "-1900799588872568832", gg::QWORD}}});
    v.push_back({38, "Speed Hack", "Combat", "", 0, {{gg::CODE_APP, gg::FLOAT, "100", 999, "5", gg::FLOAT}}});
    v.push_back({39, "Free Shop Hack", "Shop", "", 0, {{gg::DATA, gg::DWORD, "298", 1, "10", gg::DWORD}}});
    v.push_back({40, "Inf Bullet Speed", "Combat", "", 0, {{gg::DATA, gg::DWORD, "98", 3, "10", gg::DWORD}}});
    v.push_back({41, "End Match (Missions)", "Time", "", 0, {{gg::CODE_APP, gg::DWORD, "150", 70, "0", gg::DWORD}}});
    v.push_back({42, "Perfect Accuracy", "Combat", "", 0, {{gg::DATA, gg::DWORD, "42", 4, "65", gg::DWORD}}});
    v.push_back({43, "Inf Effect Hack", "Combat", "", 0, {{gg::DATA, gg::DWORD, "81", 4, "10", gg::DWORD}}});
    v.push_back({44, "Insta Bot Respawn", "Spawns", "", 0, {{gg::DATA, gg::DWORD, "299", 1, "10", gg::DWORD}}});
    v.push_back({45, "Spawn Hack", "Spawns", "", 0, {{gg::ALLOC, gg::BYTE, ":spawn", 17000, "0", gg::BYTE}}});
    v.push_back({46, "Skin Hack", "Shop", "Enable at the loading screen", 0, {{gg::DATA, gg::DWORD, "298", 1, "10", gg::DWORD}}});
    v.push_back({47, "Event Reset (tier)", "Shop", "Be in the shop", 0, {{gg::CODE_APP, gg::BYTE, ":tier", 112, "0", gg::BYTE}}});
    v.push_back({48, "Event Reset (event)", "Shop", "Be in the shop", 0, {{gg::CODE_APP, gg::BYTE, ":event", 250, "0", gg::BYTE}}});
    v.push_back({49, "Inf Pickup Range", "Combat", "", 0, {{gg::DATA, gg::DWORD, "146;147", 4, "0", gg::DWORD}}});
    v.push_back({50, "XP Prestige 1", "XP", "Play 1 match to level up", 0, {{gg::ALLOC, gg::DWORD, "500;1;050;2;250;4300;6;300;8;700;11;500;14;700;18,300;22;300;26;700;31;500;36;700;42;300;48;300;54;700;61;500;68;700;76;300;84;300;92;700;101;500;11", 48, "0", gg::DWORD}}});
    v.push_back({51, "XP Prestige 2", "XP", "Play 1 match to level up", 0, {{gg::ALLOC, gg::DWORD, "640,925;641,613;643,426;645,675;648,175;651,175;654,675;658,675;663,175;668,175;673,675;679,675;686,175;693,175;700,675;708,675;717,175;726,175;73", 48, "0", gg::DWORD}}});
    v.push_back({52, "XP Prestige 3", "XP", "Play 1 match to level up", 0, {{gg::ALLOC, gg::DWORD, "1,441,425;1,442,250;1,444,427;1,447,125;1,450,125;1,453,725;1,457,925;1,462,725;1,468,125;1,474,125;1,480,725;1,487,925;1,495,725;1,504,125;1,513,125", 48, "0", gg::DWORD}}});
    v.push_back({53, "XP Prestige 4", "XP", "Play 1 match to level up", 0, {{gg::ALLOC, gg::DWORD, "2,402,000;2,402,963;2,405,502;2,408,650;2,412,150;2,416,350;2,421,250;2,426,850;2,433,150;2,440,150;2,447,850;2,456,250;2,465,350;2,475,150;2,485,650", 48, "0", gg::DWORD}}});
    v.push_back({54, "XP Prestige 5", "XP", "Play 1 match to level up", 0, {{gg::ALLOC, gg::DWORD, "3,522,650;3,523,750;3,526,652;3,530,250;3,534,250;3,539,050;3,544,650;3,551,050;3,558,250;3,566,250;3,575,050;3,584,650;3,595,050;3,606,250;3,618,250", 48, "0", gg::DWORD}}});
    v.push_back({55, "XP Prestige 6", "XP", "Play 1 match to level up", 0, {{gg::ALLOC, gg::DWORD, "4,803,375;4,804,613;4,807,877;4,811,925;4,816,425;4,821,825;4,828,125;4,835,325;4,843,425;4,852,425;4,862,325;4,873,125;4,884,825;4,897,425;4,910,925", 48, "0", gg::DWORD}}});
    v.push_back({56, "XP Prestige 7", "XP", "Play 1 match to level up", 0, {{gg::ALLOC, gg::DWORD, "6,244,175;6,245,550;6,249,178;6,253,675;6,258,675;6,264,675;6,271,675;6,279,675;6,288,675;6,298,675;6,309,675;6,321,675;6,334,675;6,348,675;6,363,675", 48, "0", gg::DWORD}}});
    v.push_back({57, "XP Prestige 8", "XP", "Play 1 match to level up", 0, {{gg::ALLOC, gg::DWORD, "7,845,050;7,846,563;7,850,553;7,855,500;7,861,000;7,867,600;7,875,300;7,884,100;7,894,000;7,905,000;7,917,100;7,930,300;7,944,600;7,960,000;7,976,500", 48, "0", gg::DWORD}}});
    v.push_back({58, "Temporary Max Level", "XP", "", 0, {{gg::ALLOC, gg::DWORD, "9,606,000;9,607,650;9,612,003;9,617,400;9,623,400;9,630,600;9,639,000;9,648,600;9,659,400;9,671,400;9,684,600;9,699,000;9,714,600;9,731,400;9,749,400", 49, "0", gg::DWORD}}});
    v.push_back({59, "Direct Match XP Boost", "XP", "Finish a match to gain XP", 4, {{gg::ALLOC | gg::DATA, gg::DWORD, "150", 100, "999999", gg::DWORD}}});
    return v;
}
