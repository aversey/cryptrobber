all: cryptrobber encrypt test
.PHONY: all clean cryptrobber encrypt test

cryptrobber: enki/smack enki/strans
	enki/smack main.sts | enki/strans > $@
	chmod 0755 $@

encrypt: enki/smack enki/strans
	enki/smack encrypt.sts | enki/strans > $@
	chmod 0755 $@

test: enki/smack enki/strans
	enki/smack test.sts | enki/strans > $@
	chmod 0755 $@

enki/smack: enki/smack.c
	gcc $< -o $@

enki/strans: enki/strans.c
	gcc $< -o $@

clean:
	rm -f enki/smack enki/strans cryptrobber encrypt test
