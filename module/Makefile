modules modules_install help : pci_cards.inc 
	make -C /lib/modules/`uname -r`/build/ M=${CURDIR} EXTRA_CFLAGS="$(EXTRA_CFLAGS)" $@

pci_cards.inc: ../supported_cards.txt parse_cards.awk
	./parse_cards.awk <$< >$@

clean:
	make -C /lib/modules/`uname -r`/build/ M=${CURDIR} clean
	rm -rf pci_cards.inc
