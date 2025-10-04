docker start SO-TP02
docker exec -it SO-TP02 make clean -C /root/Toolchain
docker exec -it SO-TP02 make clean -C /root/
docker exec -it SO-TP02 make -C /root/Toolchain
docker exec -it SO-TP02 make -C /root/
docker stop SO-TP02

# docker run -v ${PWD}:/root --security-opt seccomp=unconfined -ti agodio/itba-so-multi-platform:3.0