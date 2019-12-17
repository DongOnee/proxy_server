FROM ubuntu:16.04
RUN apt-get update -y
RUN apt-get install gcc make -y
COPY src/ /usr/share/proxy_src
RUN cd /usr/share/proxy_src; make
ENTRYPOINT [ "/usr/share/proxy_src/proxy", "8000" ]

EXPOSE 8000