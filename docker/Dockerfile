FROM rockylinux/rockylinux:latest
ENV container docker
# gcc
RUN dnf install gcc -y
# make
RUN dnf install epel-release -y
RUN dnf install make -y
RUN dnf install findutils -y
# unittest
RUN dnf --enablerepo=powertools install CUnit-devel -y
# git
RUN dnf install git -y
# json
RUN dnf install cjson* -y
# redis
RUN dnf install hiredis* -y
# valgrind
RUN dnf install valgrind -y
# telnet
RUN dnf install telnet -y
# syslogd
RUN dnf install rsyslog -y
RUN dnf install procps -y




RUN export LD_LIBRARY_PATH=/usr/local/custom/lib/
