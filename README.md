Ex-it
=====

Ex-it is short for *execute and be init*.  The idea is to have a well-behaved
init inside containers that just need to run one foreground process.

Run like:

    ex-it COMMAND [ARGS]

E.g.:

    ex-it sleep 5
    ex-it sh -c "sleep 5; echo DONE"

So far this is just an idea and proof-of-concept.  I'm not sure how useful it
might end up being.


Enjoy!
