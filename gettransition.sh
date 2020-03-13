ATTFILE="$1"
shift

for i in $@
do
    head -n $i "$ATTFILE" | tail -n 1
done | cut -f4 | tr -d '\n'
echo
