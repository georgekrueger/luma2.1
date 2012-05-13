
track1 = Track("4Front Piano.dll", "", 0.5);
//track1 = Track("4Front Rhode.dll", "Sequence Plucked", 0.8);

scale = "C_PENTAMIN"

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

scale2 = "C_MAJ"

p1 = PatternGen(
		NoteGen(WeightGen([scale2+"_6_1", 0.5], [scale2+"_6_3", 0.5], [scale2+"_6_4", 0.5], [scale2+"_6_5", 0.5]), 90, 2), 
		RestGen(1), 
		8);
p2 = PatternGen(p1.MakeStatic(), 16);
track1.Play(p2);


