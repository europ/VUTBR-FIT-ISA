#!/bin/bash

thetime=$(date | awk '{print $4}')
maildir="maildir"
subdir[0]="new"
subdir[1]="cur"
subdir[2]="tmp"

rm -rf "$maildir/new"
rm -rf "$maildir/cur"
rm -rf "$maildir/tmp"
mkdir "${maildir}/${subdir[0]}" "${maildir}/${subdir[1]}" "${maildir}/${subdir[2]}"

for i in {0..2}; do
	for j in {1..3}; do
		touch "${maildir}/${subdir[$i]}/file${j}_${subdir[$i]}_${thetime}.f"
	done
done

tree "$maildir"
