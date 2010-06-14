DIR1=$HOME/927/chronus/simplesim-3.0
DIR2=$HOME/SimpleScalar/simplesim-3.0

for i in *.h; do
    diff -q $i $DIR2/$i
done

diff symbol.h $DIR2/symbol.h
