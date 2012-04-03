
//track1 = Track("Mysterion.1.0.dll", "LEAD DRY", 0.2);
track1 = Track("4Front Piano.dll");

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
p2 = PatternGen( NoteGen(
					WeightGen(["D_PENTAMIN_4_1", 0.5], ["D_PENTAMIN_4_2", 0.5], ["D_PENTAMIN_4_3", 0.5], 
							  ["D_PENTAMIN_4_4", 0.5], ["D_PENTAMIN_4_5", 0.5]), 
							  100, 1), 
					RestGen(0.5),
				 128);
//p2 = PatternGen( NoteGen("CMAJ_4_1", 100, 1), RestGen(1) );

track1.Play(p2);

