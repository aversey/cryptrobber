all: cryptrobber encrypt
.PHONY: all clean cryptrobber encrypt

cryptrobber: base/smack base/strans
	base/smack cryptrobber.sts | base/strans > $@
	chmod 0755 $@

encrypt: base/smack base/strans
	base/smack encrypt.sts | base/strans > $@
	chmod 0755 $@

base/smack: base/smack.c
	gcc $< -o $@

base/strans: base/strans.c
	gcc $< -o $@

clean:
	rm -f base/smack base/strans cryptrobber encrypt
