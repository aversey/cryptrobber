all: cryptrobber encrypt test
.PHONY: all clean cryptrobber encrypt test

cryptrobber: smack strans
	./smack main.sts | ./strans > $@
	chmod 0755 $@

encrypt: smack strans
	./smack encrypt.sts | ./strans > $@
	chmod 0755 $@

test: smack strans
	./smack test.sts | ./strans > $@
	chmod 0755 $@

smack: smack.c
	gcc $< -o $@

strans: strans.c
	gcc $< -o $@

clean:
	rm -f smack strans cryptrobber encrypt
