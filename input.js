
//track1 = Track("4Front Piano.dll", "", 1.0);
//track1 = Track("4Front Rhode.dll", "", 2.0);
track1 = Track("Mysterion.1.0.dll", "WILD BASS", 0.7);
scale = "C_MAJ";

p2 = 
PatternGen(
	PatternGen( 
		NoteGen(WeightGen([scale+"_4_1", 1], [scale+"_4_2", 1], [scale+"_4_3", 1], [scale+"_4_4", 1], [scale+"_4_5", 1], [scale+"_4_6", 1], [scale+"_4_7", 1]), 80, 0.20), 
		RestGen(WeightGen([2,1],[0.5,8])), 
		4
	).MakeStatic(),
	2
);
p2 = PatternGen( p2, TransposeGen(p2, scale, 1), TransposeGen(p2, scale, 2),  TransposeGen(p2, scale, 3), 4);
track1.Play(p2);

/*
p1 = PatternGen( 
		NoteGen(WeightGen([scale+"_4_1", 0.5], [scale+"_4_2", 0.5], [scale+"_4_3", 0.5], [scale+"_4_4", 0.5], [scale+"_4_5", 0.5]), 100, 1), 
		RestGen(1), 
		8);
p2 = PatternGen(p1.MakeStatic(), 16);
track1.Play(p2);*/

p1 = PatternGen(
		RestGen(0.5),
		NoteGen(WeightGen([scale+"_6_1", 0.5], [scale+"_6_3", 0.5], [scale+"_6_4", 0.5], [scale+"_6_5", 0.5], [scale+"_7_1", 0.5], [scale+"_7_3", 0.5]), 60, 4), 
		RestGen(WeightGen([1,1],[4,1],[8,0.3])), 
		8);
p2 = PatternGen(p1.MakeStatic(), 16);
track1.Play(p2);
track1.Remove(p2);


