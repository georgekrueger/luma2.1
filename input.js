
//track1 = track("Abakos\\Abakos.dll", "Sun Cult [KEYS]");
//track1 = track("JEM SX1000 - VST - .dll");
//track1 = track("Abakos\\Abakos.dll", "Neon Lights [KEYS]");
//track1 = track("Grizzly.dll", "");
//track2 = track("Mysterion.1.0.dll", "MAXIMUM BLEEP", 0.2);
track3 = track("Mysterion.1.0.dll", "LEAD DRY", 0.2);

n1 = note([["CMAJ_3_1", 0.5], ["CMAJ_3_5", 0.25], ["CMAJ_3_4", 0.25]], [[100, 0.8], [80, 0.2]], [[6, 0.5], [8, 0.5]]);
n2 = note([["CMAJ_5_1", 0.5], ["CMAJ_5_4", 0.5], ["CMAJ_6_1", 0.1]], [[60, 0.6], [70, 0.2], [40, 0.2]], 4);

p1 = pattern( n1, n1, rest([[4, 0.2], [6, 0.2], [8, 0.2], [10, 0.2]]), 64 );
p2 = pattern( n2, rest([[1, 0.9], [2, 0.2], [8, 0.1]]), 64 );
p3 = pattern( note("CMAJ_6_1", [30, 40, 50, 60], 1), rest(0.5), note(["CMAJ_6_5", "CMAJ_6_4"], [30, 40, 50, 60], 1), rest(0.5), 256 )

p4 = pattern( note([["CMAJ_4_1", 10], ["CMAJ_4_3", 1], ["CMAJ_4_4", 1], ["CMAJ_4_5", 1], ["CMAJ_4_7", 1]], [40, 60, 80], 2), rest([1, 2, 4]), 500 );
p5 = pattern( note([["CMAJ_3_1", 10], ["CMAJ_3_3", 1], ["CMAJ_3_4", 1], ["CMAJ_3_5", 1], ["CMAJ_3_7", 1]], [40, 60, 80], 2), rest([1, 2, 4]), 500 );
p6 = pattern( note([["CMAJ_4_1", 10], ["CMAJ_5_3", 1], ["CMAJ_5_4", 1], ["CMAJ_5_5", 1], ["CMAJ_5_7", 1]], [40, 60, 80], 2), rest([1, 2, 4]), 500 );

//track2.play(p4);
//track2.play(p5);
//track2.play(p6);

p7 = pattern(
		pattern( note([["CMAJ_3_1", 10], ["CMAJ_3_4", 3]], [60, 80], 0.25), rest([0.5, 1]), 8 ),
		pattern( note([["CMAJ_3_4", 10], ["CMAJ_4_1", 2]], [60, 80], 0.25), rest([0.5, 1]), 8 ),
		32);
//track3.play(p7);

p8 = pattern( note([["CMAJ_4_1", 10], ["CMAJ_4_5", 1], ["CMAJ_5_1", 10], ["CMAJ_5_5", 1], ["CMAJ_6_1", 10], ["CMAJ_6_5", 1]], [40, 60, 80], 2),
			  rest([[0, 2], [0.5, 10]]),
			  500 );
//p8.transpose(-2);
track3.play(p8);