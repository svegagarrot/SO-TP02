# Verificar si la imagen de Docker está descargada
docker_image="agodio/itba-so-multi-platform:3.0"
docker_container="SO-TP02"
if ! docker image inspect $docker_image > /dev/null 2>&1; then
    echo "[INFO] La imagen $docker_image no está descargada. Intentando descargar..."
    if ! docker pull $docker_image; then
        echo "[ERROR] No se pudo descargar la imagen $docker_image. Descárguela manualmente antes de continuar."
        exit 1
    fi
fi

# Verificar si el contenedor existe, si no, crearlo
docker inspect $docker_container > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "[INFO] El contenedor $docker_container no existe. Creándolo..."
    if ! docker run --name $docker_container -v ${PWD}:/root --security-opt seccomp=unconfined -dit $docker_image; then
        echo "[ERROR] No se pudo crear el contenedor $docker_container."
        exit 1
    fi
fi

# Permite elegir el memory manager: simple (default) o buddy
# Uso: ./compilar.sh buddy   o  ./compilar.sh MM=buddy
ARG=${1:-simple}
if [[ "$ARG" == MM=* ]]; then
	MM=${ARG#MM=}
else
	MM=$ARG
fi

docker start SO-TP02
docker exec -it SO-TP02 make clean -C /root/Toolchain
docker exec -it SO-TP02 make clean -C /root/
docker exec -it SO-TP02 make MM=$MM -C /root/Toolchain
docker exec -it SO-TP02 make MM=$MM -C /root/
docker stop SO-TP02

# docker run -v ${PWD}:/root --security-opt seccomp=unconfined -ti agodio/itba-so-multi-platform:3.0