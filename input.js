
n1 = note([["CMAJ_4_1", 0.5], ["CMAJ_4_5", 0.5]], [[100, 0.8], [80, 0.2]], [[0.5, 0.5], [1, 0.5]]);
print("Pitch: ", n1.pitch);

p = pattern( pattern(n1, rest(1), 4), pattern(n1, rest(1), 4), 4 );

play( track1, p );