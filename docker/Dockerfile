FROM rockylinux/rockylinux:latest
ENV container docker
# gcc
RUN dnf install gcc -y
# make
RUN dnf install epel-release -y
RUN dnf install make -y
RUN dnf install findutils -y
# unittest
RUN dnf install unittest* -y
# git
RUN dnf install git -y
# json
RUN dnf install cjson -y
# redis
RUN dnf install hiredis -y




RUN export LD_LIBRARY_PATH=/usr/local/custom/lib/
