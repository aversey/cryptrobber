cryptrobber: main.sts strans smack
	./smack $< | ./strans > $@
	chmod 0755 $@

smack: smack.c
	gcc $< -o $@
strans: strans.c
	gcc $< -o $@

.PHONY: clean
clean:
	rm -f smack strans cryptrobber
