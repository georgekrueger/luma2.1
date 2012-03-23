
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

p2 = PatternGen( WeightGen([NoteGen("CMAJ_4_1", 100, 1), 0.5], [NoteGen("CMAJ_4_4", 70, 1), 0.5]), RestGen(WeightGen([0.5, 0.8], [1, 0.2])));
//p2 = PatternGen( NoteGen("CMAJ_4_1", 100, 1), RestGen(1) );

track1.Play(p2);

