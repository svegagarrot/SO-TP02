# TP2 - Kernel de Sistema Operativo (ITBA)

Este proyecto implementa un kernel básico x86-64 con soporte para multitarea, gestión de memoria, IPC y syscalls.

## Imagen Docker utilizada
El script de compilación utiliza la siguiente imagen Docker:

```
agodio/itba-so-multi-platform:3.0
```

Esta imagen contiene todas las herramientas necesarias para compilar el kernel.

## Uso del script de compilación

### Compilación
```bash
./compilar.sh [tipo_mm]
```

Donde `tipo_mm` puede ser:
- `simple` (por defecto): Utiliza el memory manager simple
- `buddy`: Utiliza el memory manager buddy system

### Ejemplos
```bash
./compilar.sh           # Compila con MM simple
./compilar.sh buddy     # Compila con MM buddy
./compilar.sh MM=buddy  # Sintaxis alternativa
```

### Ejecución
```bash
./run.sh       # Ejecuta normalmente
./run.sh gdb   # Ejecuta en modo debug con GDB
```

### Limpieza
```bash
make clean
```

## Comandos disponibles

| Comando | Descripción | Ejemplo |
|---------|-------------|---------|
| `help` | Muestra los comandos disponibles | `help` |
| `clear` | Limpia la pantalla | `clear` |
| `set-user` | Setea el nombre de usuario (solicita el nombre interactivamente) | `set-user` (enter) `svega` |
| `exit` | Sale del shell | `exit` |
| `ps` | Lista todos los procesos activos | `ps` |
| `loop` | Imprime su PID cada N segundos | `loop 2` |
| `kill` | Mata un proceso por PID | `kill 3` |
| `nice` | Cambia la prioridad de un proceso | `nice 3 1` |
| `block` | Bloquea/desbloquea un proceso | `block 4` |
| `mem` | Muestra el estado de la memoria | `mem` |
| `mmtype` | Muestra el tipo de MM activo | `mmtype` |
| `cat` | Lee de stdin y escribe a stdout | `cat` |
| `wc` | Cuenta líneas del input | `ps \| wc` |
| `filter` | Filtra las vocales del input | `ps \| filter` |
| `mvar` | Simula MVar con N escritores y M lectores | `mvar 3 2` |
| `testmm` | Test de estrés del memory manager | `testmm 4096` |
| `test_proceses` | Test de creación masiva de procesos | `test_proceses 8` |
| `test_priority` | Test del sistema de prioridades | `test_priority 1000` |
| `test_synchro` | Test con semáforos (resultado = 0) | `test_synchro 10000` |
| `test_no_synchro` | Test sin semáforos (condición de carrera) | `test_no_synchro 10000` |
| `exceptions` | Prueba excepciones del sistema | `exceptions zero` |

## Combinaciones de teclas

- **Shift izquierda**: Necesario para mayúsculas y los caracteres `|` y `&`
- **Ctrl + C**: Mata el proceso en foreground
- **Ctrl + D**: Envía EOF al proceso en foreground

**Nota:** El teclado está mapeado como teclado en inglés.

## Caracteres especiales

- **`|`** (pipe): Conecta la salida de un proceso con la entrada de otro
- **`&`** (ampersand): Ejecuta el proceso en background

### Ejemplos de pipes
```bash
ps | wc          # Cuenta las líneas de la salida de ps
ps | filter      # Muestra ps sin vocales
```

### Ejemplos de background
```bash
loop 5 &         # Ejecuta loop en background
ps               # El proceso loop aparecerá listado
```

## Descripción de comandos

### Comandos de gestión de procesos

- **`loop <segundos>`**: Crea un proceso que imprime "Hola! Soy el proceso con ID X" cada N segundos
- **`ps`**: Lista todos los procesos mostrando PID, nombre, estado, prioridad, RSP, RBP y si es foreground
- **`kill <pid>`**: Termina un proceso específico
- **`nice <pid> <prioridad>`**: Cambia la prioridad de un proceso (0-4, donde 0 es la más alta)
- **`block <pid>`**: Alterna el estado de un proceso entre bloqueado y listo

### Comandos de pipes

- **`cat`**: Lee del stdin y lo imprime tal cual al stdout
- **`wc`**: Lee del stdin y cuenta la cantidad de líneas, imprimiendo el resultado
- **`filter`**: Lee del stdin y elimina todas las vocales (mayúsculas y minúsculas) del texto
- **`mvar <escritores> <lectores>`**: Simula una MVar con múltiples procesos. Los escritores escriben letras (A-Z) y los lectores las leen y muestran con diferentes colores

### Comandos de memoria

- **`mem`**: Muestra estadísticas de memoria (capacidad, usado, libre, cantidad de allocaciones)
- **`mmtype`**: Indica qué tipo de memory manager está activo (simple o buddy)
- **`testmm <bytes>`**: Ejecuta un test de estrés del memory manager

### Tests de sistema

- **`test_proceses <max>`**: Crea y testea múltiples procesos simultáneamente
- **`test_priority <max_value>`**: Verifica el correcto funcionamiento del scheduler con diferentes prioridades. Los procesos con mayor prioridad deben imprimir con más frecuencia
- **`test_synchro <repeticiones>`**: Test de sincronización usando semáforos. El resultado final siempre debe ser 0
- **`test_no_synchro <repeticiones>`**: Test sin sincronización que demuestra condiciones de carrera. El resultado varía entre ejecuciones

### Otros comandos

- **`exceptions [zero|invalidOpcode]`**: Prueba el manejo de excepciones del kernel
  - `zero`: División por cero
  - `invalidOpcode`: Código de operación inválido

## GDB

Para usar GDB:

1. Ejecutar el kernel en modo debug:
```bash
./run.sh gdb
```

2. En otra terminal dentro del contenedor Docker, ejecutar:
```bash
gdb
```

## Estructura del proyecto

```
/Kernel              # Código del kernel
  /mm                # Memory managers (simple y buddy)
  /proc              # Gestión de procesos y scheduler
  /ipc               # Pipes y semáforos
  /syscalls          # Implementación de syscalls
  /drivers           # Drivers de teclado y video
  /interrupts        # Manejo de interrupciones
  
/Userland            # Código de usuario
  /SampleCodeModule
    /shell           # Implementación del shell
    /tests           # Tests del sistema
    /lib             # Biblioteca userland

/Bootloader          # Pure64 y BMFS
/Image               # Generación de imagen booteable
/Toolchain           # ModulePacker
```

## Limitaciones

### Procesos
- Máximo 16 file descriptors por proceso
- Nombre de proceso limitado a 32 caracteres
- Stacks de tamaño fijo: 16 KB para kernel stack y 16 KB para user stack
- Sin límite máximo de procesos (puede agotar memoria)

### Pipes
- Solo un pipe por comando (no se pueden encadenar: `cmd1 | cmd2 | cmd3`)
- No soportan ejecución en background (no se puede usar `&` con pipes)
- Solo funcionan con comandos externos (no se pueden usar comandos built-in como `help`, `clear`, `exit`, etc.)
- Máximo 64 pipes simultáneos en el sistema
- Buffer limitado a 4096 bytes por pipe

### Semáforos
- Máximo 64 semáforos simultáneos en el sistema
- Nombres compartidos globalmente

### Scheduler
- Solo 3 niveles de prioridad (0, 1, 2)
- Quantum fijo de 8 ticks (no configurable en runtime)
- Sin soporte para múltiples CPUs (SMP)

### Memory Manager
- Heap limitado a 512 MB
- mm_simple: Puede sufrir fragmentación externa (first-fit)
- mm_buddy: Desperdicio por alineación (redondea a potencias de 2)

### Shell
- En ocasiones la shell se traba y no permite escribir. Al cerrar y volver a entrar, funciona correctamente. La causa del error no ha sido identificada

## Integrantes

Sebastian Vega - 64111
Julia Priolo - 63743
Martina Nudel - 64155

---

**Materia:** Sistemas Operativos - ITBA  
**Cuatrimestre:** 2do Cuatrimestre 2024  
**Docente:** Ariel Godio
