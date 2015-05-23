# compacted version, see info sed for the commented version
h;:b
$b
N; /^\(.*\)\n\1$/ { g; bb
}
$b
P; D
