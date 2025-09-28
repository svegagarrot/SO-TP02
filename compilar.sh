docker start SO-TP02
docker exec -it SO-TP02 make clean -C /root/Toolchain
docker exec -it SO-TP02 make clean -C /root/
docker exec -it SO-TP02 make -C /root/Toolchain
docker exec -it SO-TP02 make -C /root/
docker stop SO-TP02