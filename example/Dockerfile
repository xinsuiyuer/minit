# This Dockerfile example updates the ssh/apache example at
# <https://docs.docker.io/en/latest/examples/using_supervisord/> and makes use
# of minit (https://github.com/chazomaticus/minit) instead of supervisord.
#
# First run make in the parent directory.  Then build with:
#     cp -a ../minit .
#     sudo docker build -t minit-example .
#
# Run in the foreground with:
#     sudo docker run -i -t --sig-proxy minit-example
#
# Or in the background with:
#     container=$(sudo docker run -d minit-example)
#
# Stop the backgrounded container with:
#     sudo docker stop $container
#
#
# Here's what docker images reports for this example, compared to an equivalent
# one running supervisord:
#     REPOSITORY            TAG      ...   VIRTUAL SIZE
#     minit-example         latest   ...   301.9 MB
#     supervisord-example   latest   ...   326.9 MB
#
# That's 25M smaller without losing anything but supervisord's service
# monitoring.


FROM ubuntu:quantal
MAINTAINER Charles Lindsay <chaz@chazomatic.us>

RUN apt-get update
RUN apt-get upgrade -y

RUN apt-get install -y openssh-server apache2
RUN mkdir -p /var/run/sshd

ADD minit /sbin/minit
ADD startup /etc/minit/startup
# We don't need anything special on shutdown, since minit already supplies
# anything running with SIGTERM when it's shutting down.

EXPOSE 22 80

# Always use the [] form of CMD, else a shell will be pid 1 instead of minit.
CMD ["/sbin/minit"]
