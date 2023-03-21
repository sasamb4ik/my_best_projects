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

# parsing finished, now starting script

mkdir "$archive" # делаем папку из которой потом сделаем архив

for key in "${!dictionary[@]}"; do # перебираем расширения
  compiler=${dictionary[$key]} # каждому расширению сопоставляем компилятор
  find "$source" -name "*.$key" | while read file # перебор файлов по адресу $source с расширением $key
  do
    filename="$(basename -- "$file")"
    filename="${filename%.*}"
    buf=$(realpath --relative-to="$source" "$file")
    path=$(dirname "$buf")
    mkdir -p "$archive/$path"  # тут и 2 строчки выше копирую иерархию
    eval "$compiler" -o "$archive/$path/$filename.exe" "$file"
  done
done

tar -czf "$archive.tar.gz" "$archive" 

###
echo "complete"
