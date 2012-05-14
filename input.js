
//track1 = Track("4Front Piano.dll", "", 0.5);
//track1 = Track("4Front Rhode.dll", "", 0.8);
track1 = Track("Mysterion.1.0.dll", "WILD BASS", 0.4);

scale = "D_PENTAMAJ"

p1 = PatternGen( 
		NoteGen(WeightGen([scale+"_3_1", 0.5], [scale+"_3_2", 0.5], [scale+"_3_3", 0.5], [scale+"_3_4", 0.5], [scale+"_3_5", 0.5]), 80, 6), 
		RestGen(4), 
		8);
p2 = PatternGen(p1.MakeStatic(), 16);
track1.Play(p2);

/*
p1 = PatternGen( 
		NoteGen(WeightGen([scale+"_4_1", 0.5], [scale+"_4_2", 0.5], [scale+"_4_3", 0.5], [scale+"_4_4", 0.5], [scale+"_4_5", 0.5]), 100, 1), 
		RestGen(1), 
		8);
p2 = PatternGen(p1.MakeStatic(), 16);
track1.Play(p2);*/

scale2 = "D_PENTAMAJ"

p1 = PatternGen(
		RestGen(0.5),
		NoteGen(WeightGen([scale2+"_6_1", 0.5], [scale2+"_6_3", 0.5], [scale2+"_6_4", 0.5], [scale2+"_6_5", 0.5], [scale2+"_7_1", 0.5], [scale2+"_7_3", 0.5]), 60, 4), 
		RestGen(WeightGen([1,0.5],[4,1])), 
		8);
p2 = PatternGen(p1.MakeStatic(), 16);
track1.Play(p2);


