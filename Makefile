.PHONY: cryptrobber clean

cryptrobber:
	./smack main.sts | ./strans > $@
	chmod 0755 $@

smack: smack.c
	gcc $< -o $@

strans: strans.c
	gcc $< -o $@

clean:
	rm -f smack strans cryptrobber
