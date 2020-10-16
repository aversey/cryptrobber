all: cryptrobber encrypt
.PHONY: all clean encrpyt cryptrobber

cryptrobber: smack strans
	./smack main.sts | ./strans > $@
	chmod 0755 $@

encrypt: smack strans
	./smack encrypt.sts | ./strans > $@
	chmod 0755 $@

smack: smack.c
	gcc $< -o $@

strans: strans.c
	gcc $< -o $@

clean:
	rm -f smack strans cryptrobber encrypt
