FROM gcc:14-bookworm@sha256:5e927c284bf55a7dc796262e311a0703344f62f41f5621eb56843111b1d37e15

COPY verify2.c /src/verify2.c
RUN gcc -O3 -fopenmp -Wall -o /usr/local/bin/verify2 /src/verify2.c -lm

ENTRYPOINT ["/usr/local/bin/verify2"]
