
//track1 = Track("Mysterion.1.0.dll", "LEAD DRY", 0.2);
track1 = Track("4Front Rhode.dll", 0.3);
//track1 = Track("shortcircuit.dll");

//WeightGen(["CMAJ_4_1", 0.5], ["CMAJ_4_3", 0.5]);
//WeightGen({"CMAJ_4_1": 0.6, 100: 0.4});

//p1 = 
//PatternGen( 
//	NoteGen(
//		WeightGen({"CMAJ_4_1": 0.5, "CMAJ_4_3": 0.5}), 
//		WeightGen({100: 0.6, 30: 0.4}), 
//		WeightGen({1: 0.5, 2: 0.5})
//	), 
//	RestGen(WeightGen({1: 0.5, 2: 0.5})) 
//);

//p2 = PatternGen( NoteGen("C_MAJ_4_1", 100, 1), RestGen(WeightGen([2, 0.5], [1.5, 0.5])), 128);
//

scale = "C_PENTAMIN"

p2 = 
PatternGen( 
	NoteGen(
		WeightGen(
			[scale+"_4_1", 0.5], [scale+"_4_2", 0.5], [scale+"_4_3", 0.5], 
			[scale+"_4_4", 0.5], [scale+"_4_5", 0.5]
		), 
		WeightGen([100, 0.1], [80, 0.3], [60, 0.8], [40, 1.0]), 
		4
	), 
	WeightGen(
		[ RestGen(2), 0.6],
		[	NoteGen(
				WeightGen(
					[scale+"_6_1", 0.5], [scale+"_6_2", 0.5], [scale+"_6_3", 0.5], 
					[scale+"_6_4", 0.5], [scale+"_6_5", 0.5]), 
				WeightGen([100, 0.1], [80, 0.3], [60, 0.8], [40, 1.0]), 
				8), 0.3 ]), 
	 256
);

p3 = 
PatternGen( 
	NoteGen(
		WeightGen(
			[scale+"_4_1", 0.5], [scale+"_4_2", 0.5], [scale+"_4_3", 0.5], 
			[scale+"_4_4", 0.5], [scale+"_4_5", 0.5]
		), 
		WeightGen([100, 0.1], [80, 0.3], [60, 0.8], [40, 1.0]), 
		2
	), 
	RestGen(2),
	4
);

//p4 = PatternGen(p3.MakeStatic(), 4);

track1.Play(PatternGen(p3.MakeStatic(), 4));

//p2 = PatternGen( NoteGen("CMAJ_4_1", 100, 1), RestGen(1) );

//track1.Play(p2);

