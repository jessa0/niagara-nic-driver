#!/usr/bin/awk -f
BEGIN{ print "//DO NOT EDIT - AUTOMATICALLY GENERATED "}
BEGIN{ print "pci_card_t pci_cards[]={"}
/^#/{next}
NF<7{next}
{print "{\""$1"\","$2","$3","$4","$5","$6","$7"},"}
END{print "{0}"}
END{print "};"}
