# http://www.regular-expressions.info/floatingpoint.html
sed 's/	[-+]\?[0-9]*.\?[0-9]\+\([eE][-+]\?[0-9]\+\)\?$//' "$@"
