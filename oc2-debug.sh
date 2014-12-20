#!/bin/bash
xxd -g4 $1 | sed -e "s/eeeeee/------/g" -e "s/\(......\)ff /-\1- /g" -e "s/00 /   /g" -e "s/00 /   /g" -e "s/00 /   /g"
