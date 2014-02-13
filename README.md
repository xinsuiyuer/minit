Minit
=====

Minit is a minimalist `init(8)` implementation designed for use inside
containers, for instance as the root process in a
[Docker](https://www.docker.io/) image.

<https://github.com/chazomaticus/minit>

Justification
-------------

Docker
[recommends](https://docs.docker.io/en/latest/examples/using_supervisord/)
running [Supervisor](http://supervisord.org/) as your container's root process
if you need to start multiple services inside a single container.  Sometimes,
though, you don't want the overhead of a full Python stack tagging along with
your nice clean container image.

*Advantages vs. Supervisor:*
 * No dependencies
 * Only about 6K in size (vs. about 17M for Python and Supervisor reported by
   apt-get on a clean Quantal image)
 * Allows arbitrary commands in container startup and shutdown
 * Easier to control daemons that can't run in the foreground like Postfix

*Disadvantages vs. Supervisor:*
 * Doesn't monitor or restart services

Use
---

Build minit by running `make` (or otherwise compiling the `.c` file into an
executable).  Put the resulting executable in your container's filesystem and
set it to run as the root process when your container starts.

When minit starts up, it `exec`'s `/etc/minit/startup`, then goes to sleep
`wait`ing on children until it gets a `SIGTERM` (or `SIGINT`, so you can stop
your container with `Ctrl+C` if you ran it in the foreground).  When it gets
one of those signals, it `exec`'s `/etc/minit/shutdown` and waits for it to
finish, then `kill`s all remaining processes with `SIGTERM`.

You can override the startup and shutdown scripts with command line arguments:
the first, if non-empty, is run instead of `/etc/minit/startup`, and the second
is run instead of `/etc/minit/shutdown`.  Minit's environment is passed
unmolested to the scripts, so if you need to pass arguments to them, do it
environmentally.

Check out the `example` directory for a Docker example.  I believe minit is
relatively portable, and should work inside most Unices' version of containers.


Enjoy!

Thanks to [Arachsys init](https://github.com/arachsys/init) for ideas and
inspiration.
