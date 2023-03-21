#!/bin/bash

# parsing part

declare -a array=()
declare -A dictionary=()

while [ $# -gt 0 ]; do
  case "$1" in
    -a | --archive)
        archive=$2
	shift 2
        ;;
    -s | --source)
        source=$2
	shift 2
        ;;
    -c | --compiler)
        array+=( "$2" )
        shift 2
        ;;
    *)
        shift 1
	;;
  esac
done

for item in "${array[@]}"
do
  IFS="=" read -r -a splitted_array <<< $item
  compiler=${splitted_array[-1]}
  unset splitted_array[-1]
  for item in "${splitted_array[@]}"
  do
    dictionary[$item]="$compiler"
  done
done


mkdir "$archive" 

for key in "${!dictionary[@]}"; do
  compiler=${dictionary[$key]}
  find "$source" -name "*.$key" | while read file
  do
    filename="$(basename -- "$file")"
    filename="${filename%.*}"
    buf=$(realpath --relative-to="$source" "$file")
    path=$(dirname "$buf")
    mkdir -p "$archive/$path"
    eval "$compiler" -o "$archive/$path/$filename.exe" "$file"
  done
done

tar -czf "$archive.tar.gz" "$archive" 

echo "complete"
