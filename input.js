
track1 = Track("Mysterion.1.0.dll", "LEAD DRY", 0.2);

p1 = PatternGen( NoteGen("CMAJ_4_1", 100, 1), RestGen(1) );
track1.Play(p1);

