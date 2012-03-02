
track1 = track("Abakos\\Abakos.dll", "Sun Cult [KEYS]");
track2 = track("4Front Piano.dll");

n1 = note([["CMAJ_3_1", 0.5], ["CMAJ_3_5", 0.25], ["CMAJ_3_4", 0.25]], [[100, 0.8], [80, 0.2]], [[6, 0.5], [8, 0.5]]);
n2 = note([["CMAJ_5_1", 0.5], ["CMAJ_5_4", 0.5], ["CMAJ_6_1", 0.1]], [[60, 0.6], [70, 0.2], [40, 0.2]], 4);

p = pattern( n1, n1, rest([[4, 0.2], [6, 0.2], [8, 0.2], [10, 0.2]]), 16 );
p2 = pattern( n2, rest([[1, 0.9], [2, 0.2], [8, 0.1]]), 64 );
p3 = pattern( note("CMAJ_6_1", [30, 40, 50, 60], 1), rest(0.5), note(["CMAJ_6_5", "CMAJ_6_4"], [30, 40, 50, 60], 1), rest(0.5), 128 )

track1.play(p);
//track2.play(p2)
track2.play(p3);
