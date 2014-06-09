#!/bin/bash
# Parse help texts from Makefile for `make help`.

TARGETS=$(egrep "^# target" Makefile)

echo -e "Compilation targets:"
echo "$TARGETS"|grep "^# target_comp:"|sed 's/^# target_comp: //'|sed 's/ - /\t/'|column -s $'\t' -t
echo -e "\nGeneric targets:"
echo "$TARGETS"|grep "^# target:"|sed 's/^# target: //'|sed 's/ - /\t\t/'|column -s $'\t' -t
echo -e "\nTest targets:"
echo "$TARGETS"|grep "^# target_test:"|sed 's/^# target_test: //'|sed 's/ - /\t\t/'|column -s $'\t' -t
echo -e "\nDocumentation targets:"
echo "$TARGETS"|grep "^# target_doc:"|sed 's/^# target_doc: //'|sed 's/ - /\t\t/'|column -s $'\t' -t
echo -e "\nClean targets:"
echo "$TARGETS"|grep "^# target_clean:"|sed 's/^# target_clean: //'|sed 's/ - /\t\t/'|column -s $'\t' -t

