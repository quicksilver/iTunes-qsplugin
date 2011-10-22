I'm not really up to writing documentation for all this, so I'll give you the very basics.

For nearly all of these classes, the way you'll use them is to:
1. Instantiate the class
2. Read in the file in question into a big huge char *.
3. Call iPod_class.parse() to convert the char * into objects and such. 

Now you can free up the big giant char *.

Then you modify the heck out of it using the various functions in each class/subclass. Or whatever, think of it like having the file in memory in a more useful form.

Finally, you want to write it back to disk. So you malloc your char * again, then:
1. Call iPod_class.write() to fill up the big char *.
2. Write the char * off to disk.

Simple as that.

Main bits that you'll instantiate yourself:

-iPod_mhbd - This corresponds to the iTunesDB file. You can tell because the first 4 bytes of this file are "mhbd".

-iPod_mhdp - Corresponds to the Play Counts file. This is, realistically, a read only file. You should never need to write it, which is why it has no write().

-iPod_mhpo - Corresponds to an OTGPlaylist file. It does have a write(), but I can't see any good reason to ever use it.

-iPod_mqed - Corresponds to the iTunesEQPresets file. iTunes does read and write this file, even though the iPod itself does not actually use it, as of this writing. Nevertheless, this supports it fully.

Any other classes you see in there are probably subclasses of something else. For example, iPod_pqed is a subclass of iPod_mqed. It's not something you need to use parse() or write() on. You could, if you wanted to, but it's just a piece of iPod_mqed, and iPod_mqed.write() will do the write() on all of it's iPod_pqed's.

The same is true of everything underneath iPod_mhbd, only more so. ;)

Enjoy! 

-Otto
