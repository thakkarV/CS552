to build either the busywait based or the irq based implementations, just cd into the folder and type make
no other config flags or command line args are required.

indmod the .ko file and run the compiled test ELF to test the code. 

the busywaait based module only works with x server disabled.

links used

http://tuxthink.blogspot.com/2011/04/wait-queues.html
http://www.linuxjournal.com/node/8144/print
https://www.kernel.org/doc/htmldocs/kernel-hacking/queues.html
https://elixir.free-electrons.com
http://www.tldp.org/LDP/lkmpg/2.6/html/x1256.html